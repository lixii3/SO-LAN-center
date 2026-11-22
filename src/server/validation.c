#include "./server.h"

/*
** Validazione dei comandi del protocollo (vedi PROTOCOL.md).
**
** Qui si controlla SOLO cio' che si puo' verificare guardando la riga:
**   - il comando esiste? l'utente ha i permessi per usarlo?
**   - il numero di parametri e' giusto? i formati (data, ora, id...) sono validi?
**   - le regole del LAN center che non dipendono dai file (orari di apertura,
**     durata, data non passata)
** Quello che richiede i file (la postazione esiste? l'id esiste? c'e' un
** conflitto?) lo controlla chi esegue il comando, con il lock preso.
**
** Tutte le check_* ritornano NULL se va tutto bene, altrimenti il codice di
** errore da mandare al client (ERR_SYNTAX, ERR_INVALID_DATE, ...).
*/

// --- funzioni di supporto ----------------------------------------------------

// 1 se str e' un id valido (intero > 0), 0 altrimenti
static int	is_valid_id(const char *str)
{
	int	id;

	return (str_to_int(str, &id) == 0 && id > 0);
}

// 1 se str non e' vuota ed entra in un campo grande size ('\0' compreso)
static int	fits(const char *str, size_t size)
{
	return (str[0] != '\0' && strlen(str) < size);
}

// controlla un filtro "chiave=valore" di MY / ALL.
// allow_user = 1 se e' ammesso anche il filtro user= (solo ALL)
static const char	*check_filter(const char *filter, int allow_user)
{
	const char		*eq;
	const char		*value;
	size_t			key_len;
	t_date			d;
	enum e_status	st;

	eq = strchr(filter, '=');
	if (!eq || eq == filter || eq[1] == '\0') // manca '=', la chiave o il valore
		return (ERR_SYNTAX);
	key_len = eq - filter; // in bytes
	value = eq + 1; // puntatore al valore
	if (key_len == 2 && strncmp(filter, "id", 2) == 0)
		return (is_valid_id(value) ? NULL : ERR_SYNTAX);
	if (key_len == 7 && strncmp(filter, "station", 7) == 0)
		return (fits(value, MAX_STATION) ? NULL : ERR_SYNTAX);
	if (key_len == 4 && strncmp(filter, "date", 4) == 0)
		return (str_to_date(value, &d) == 0 ? NULL : ERR_SYNTAX);
	if (key_len == 6 && strncmp(filter, "status", 6) == 0)
		return (str_to_status(value, &st) == 0 ? NULL : ERR_SYNTAX);
	if (allow_user && key_len == 4 && strncmp(filter, "user", 4) == 0)
		return (fits(value, MAX_USERNAME) ? NULL : ERR_SYNTAX);
	return (ERR_SYNTAX); // chiave sconosciuta
}

// controlla tutti i filtri da argv[1] in poi
static const char	*check_filters(char **argv, int argc, int allow_user)
{
	const char	*err;
	int			i;

	i = 1;
	while (i < argc)
	{
		err = check_filter(argv[i], allow_user);
		if (err)
			return (err);
		i++;
	}
	return (NULL);
}

// --- comandi utente ----------------------------------------------------------

// LOGIN <username> <password>
static const char	*check_login(char **argv, int argc)
{
	if (argc != 3 || !fits(argv[1], MAX_USERNAME) || !fits(argv[2], MAX_PASSWORD))
		return (ERR_SYNTAX);
	return (NULL);
}

// BOOK <postazione> <data> <inizio> <fine>
// (che la postazione esista lo controlla chi esegue il comando)
static const char	*check_book(char **argv, int argc)
{
	t_date	date;
	t_time	start;
	t_time	end;
	int		duration;
	int		past;

	if (argc != 5 || !fits(argv[1], MAX_STATION))
		return (ERR_SYNTAX);
	if (str_to_date(argv[2], &date) < 0)
		return (ERR_INVALID_DATE); // formato sbagliato o data inesistente
	past = is_past(&date, NULL);
	if (past < 0)
		return (ERR_SERVER);
	if (past)
		return (ERR_INVALID_DATE); // giorno gia' passato
	if (str_to_time(argv[3], &start) < 0 || str_to_time(argv[4], &end) < 0)
		return (ERR_INVALID_TIME);
	if ((start.min != ZERO && start.min != THIRTY)
		|| (end.min != ZERO && end.min != THIRTY))
		return (ERR_INVALID_TIME); // solo :00 e :30
	if (start.h < OPENING_H || time_to_min(&end) > CLOSING_H * 60)
		return (ERR_INVALID_TIME); // fuori dall'orario di apertura
	duration = time_to_min(&end) - time_to_min(&start);
	if (duration < MIN_DURATION || duration > MAX_DURATION)
		return (ERR_INVALID_TIME); // comprende anche fine <= inizio
	past = is_past(&date, &start);
	if (past < 0)
		return (ERR_SERVER);
	if (past)
		return (ERR_INVALID_TIME); // l'ora di inizio e' gia' passata
	return (NULL);
}

// 1 se cmd e' un comando utente (usabile da tutti)
static int	is_user_cmd(const char *cmd)
{
	return (strcmp(cmd, CMD_LOGIN) == 0 || strcmp(cmd, CMD_LOGOUT) == 0
		|| strcmp(cmd, CMD_QUIT) == 0 || strcmp(cmd, CMD_STATIONS) == 0
		|| strcmp(cmd, CMD_BOOK) == 0 || strcmp(cmd, CMD_MY) == 0);
}

// comandi utente: LOGIN LOGOUT QUIT STATIONS BOOK MY
// ritorna ERR_UNKNOWN_COMMAND se argv[0] non e' un comando utente
static const char	*check_user_cmd(char **argv, int argc)
{
	if (strcmp(argv[0], CMD_LOGIN) == 0)
		return (check_login(argv, argc));
	if (strcmp(argv[0], CMD_LOGOUT) == 0 || strcmp(argv[0], CMD_QUIT) == 0
		|| strcmp(argv[0], CMD_STATIONS) == 0)
		return (argc == 1 ? NULL : ERR_SYNTAX); // nessun parametro
	if (strcmp(argv[0], CMD_BOOK) == 0)
		return (check_book(argv, argc));
	if (strcmp(argv[0], CMD_MY) == 0)
		return (check_filters(argv, argc, 0));
	return (ERR_UNKNOWN_COMMAND);
}

// --- comandi admin -----------------------------------------------------------

// SET <id> <stato>
static const char	*check_set(char **argv, int argc)
{
	enum e_status	st;

	if (argc != 3 || !is_valid_id(argv[1]))
		return (ERR_SYNTAX);
	if (str_to_status(argv[2], &st) < 0)
		return (ERR_INVALID_STATUS);
	return (NULL);
}

// 1 se argv[0] e' un comando riservato all'admin
static int	is_admin_cmd(const char *cmd)
{
	return (strcmp(cmd, CMD_ALL) == 0 || strcmp(cmd, CMD_APPROVE) == 0
		|| strcmp(cmd, CMD_REJECT) == 0 || strcmp(cmd, CMD_SET) == 0
		|| strcmp(cmd, CMD_CONFLICTS) == 0);
}

// comandi admin: ALL APPROVE REJECT SET CONFLICTS
static const char	*check_admin_cmd(char **argv, int argc)
{
	if (strcmp(argv[0], CMD_ALL) == 0)
		return (check_filters(argv, argc, 1));
	if (strcmp(argv[0], CMD_APPROVE) == 0 || strcmp(argv[0], CMD_REJECT) == 0)
		return (argc == 2 && is_valid_id(argv[1]) ? NULL : ERR_SYNTAX);
	if (strcmp(argv[0], CMD_SET) == 0)
		return (check_set(argv, argc));
	if (strcmp(argv[0], CMD_CONFLICTS) == 0)
		return (argc == 1 ? NULL : ERR_SYNTAX);
	return (ERR_UNKNOWN_COMMAND);
}

// --- funzioni pubbliche ------------------------------------------------------

// divide line in parole separate da UN singolo spazio (line viene modificata:
// argv[i] punta dentro line). Toglie l'eventuale '\r' finale (telnet).
// ritorna il numero di parole, -1 se sono piu' di MAX_ARGS o se la riga e' vuota
int	split_cmd(char *line, char **argv)
{
	size_t	len;

	if (!line || !argv)
		return (-1);
	len = strlen(line);
	if (len > 0 && line[len - 1] == '\r')
		line[--len] = '\0';
	if (len == 0)
		return (-1);
	return (split_fields(line, ' ', argv, MAX_ARGS));
}

// controlla un comando gia' diviso in parole (vedi split_cmd), tenendo conto
// dello stato della sessione (loggato o no, user o admin).
// ritorna NULL se il comando puo' essere eseguito, altrimenti il codice di errore
const char	*check_cmd(char **argv, int argc, const t_session *s)
{
	int	is_admin;

	if (!argv || !s || argc < 1)
		return (ERR_SYNTAX);
	if (!is_user_cmd(argv[0]) && !is_admin_cmd(argv[0]))
		return (ERR_UNKNOWN_COMMAND);
	// da non loggati si possono usare solo LOGIN e QUIT
	if (!s->role && strcmp(argv[0], CMD_LOGIN) != 0 && strcmp(argv[0], CMD_QUIT) != 0)
		return (ERR_NOT_LOGGED);
	if (s->role && strcmp(argv[0], CMD_LOGIN) == 0)
		return (ERR_ALREADY_LOGGED);
	if (!s->role && strcmp(argv[0], CMD_LOGIN) == 0 && s->failed_logins >= 3)
		return (ERR_TOO_MANY_ATTEMPTS);
	if (is_admin_cmd(argv[0]))
	{
		is_admin = (strcmp(s->role, ROLE_ADMIN) == 0);
		if (!is_admin)
			return (ERR_UNAUTHORIZED);
		return (check_admin_cmd(argv, argc));
	}
	return (check_user_cmd(argv, argc));
}

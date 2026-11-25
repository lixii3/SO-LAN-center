#include "./server.h"

char	*exec_login(char **argv, t_session *session); 
char	*exec_logout(char **argv, t_session *session);
char	*exec_book(char **argv, t_session *session);
char	*exec_stations(t_session *session);
char	*exec_quit(t_session *session);
char	*exec_my(char **argv, int argc, t_session *session);
char	*exec_all(char **argv, int argc, t_session *session);
char	*exec_approve(char **argv, t_session *session);
char	*exec_reject(char **argv, t_session *session);
char	*exec_set(char **argv, t_session *session);
char	*exec_conflicts(t_session *session);

// Prende la riga di comando già divisa in argc argomenti e lo esegue ritornando
// una stringa di risposta RESP_.
// Oppure lo rigetta con un codice di errore ERR_.
char	*handle_cmd(char **argv, int argc, t_session *session)
{
	const char	*returnCode;
	char		resp[MAX_LINE];
	char		*cmd;

	if ((returnCode = check_cmd(argv, argc, session)))
	{
		snprintf(resp, sizeof(resp), "%s %s", RESP_ERR, returnCode);
		return (strdup(resp));
	}
	cmd = argv[0];
	// le stringhe si confrontano con strcmp: "==" confronta gli indirizzi
	if (strcmp(cmd, CMD_LOGIN) == 0)
	{
		return (exec_login(argv, session));
	}
	if (strcmp(cmd, CMD_STATIONS) == 0)
		return (exec_stations(session));
	if (strcmp(cmd, CMD_LOGOUT) == 0)
		return (exec_logout(argv, session));
	if (strcmp(cmd, CMD_QUIT) == 0)
		return (exec_quit(session));
	if (strcmp(cmd, CMD_BOOK) == 0)
		return (exec_book(argv, session));
	if (strcmp(cmd, CMD_MY) == 0)
		return (exec_my(argv, argc, session));
	if (strcmp(cmd, CMD_ALL) == 0)
		return (exec_all(argv, argc, session));
	if (strcmp(cmd, CMD_APPROVE) == 0)
		return (exec_approve(argv, session));
	if (strcmp(cmd, CMD_REJECT) == 0)
		return (exec_reject(argv, session));
	if (strcmp(cmd, CMD_SET) == 0)
		return (exec_set(argv, session));
	if (strcmp(cmd, CMD_CONFLICTS) == 0)
		return (exec_conflicts(session));
	return (strdup(RESP_ERR " " ERR_UNKNOWN_COMMAND));
}

// LOGIN: risponde con "OK <role>",
// esegue il login modificando i dati in session, ritorna una risposta.
// Al 3o tentativo fallito risponde TOO_MANY_ATTEMPTS e fa chiudere la connessione.
char	*exec_login(char **argv, t_session *session)
{
	char	resp[MAX_LINE];
	int		is_admin;
	int		ret;

	ret = storage_check_credentials(session->users_fd, argv[1], argv[2], &is_admin);
	if (ret == STORAGE_NOT_FOUND)
	{
		session->failed_logins += 1;
		if (session->failed_logins >= MAX_LOGIN_TRIES)
		{
			session->quit = 1;
			return (strdup(RESP_ERR " " ERR_TOO_MANY_ATTEMPTS));
		}
		return (strdup(RESP_ERR " " ERR_BAD_CREDENTIALS));
	}
	if (ret < 0) // archivio illeggibile: non e' colpa dell'utente, non si conta
		return (strdup(RESP_ERR " " ERR_SERVER));
	// la password non serve piu': nella sessione si ricorda solo chi e' l'utente
	strcpy(session->user.username, argv[1]);
	session->user.is_admin = is_admin;
	session->failed_logins = 0;
	if (is_admin)
		session->role = ROLE_ADMIN;
	else
		session->role = ROLE_USER;
	printf("User %s logged in as %s\n", session->user.username, session->role);
	snprintf(resp, sizeof(resp), "%s %s", RESP_OK, session->role);
	return (strdup(resp));
}

// LOGOUT: esegue il logout dalla sessione se attiva.
char	*exec_logout(char **argv, t_session *session)
{
	(void)argv;
	if (session->role)
	{
		session->role = NULL;
		clear_user(&session->user);
		session->failed_logins = 0;
	}
	return (strdup(RESP_OK));
}

// QUIT: termina il processo client che lo esegue.
char	*exec_quit(t_session *session)
{
	close(session->users_fd);
	close(session->booking_fd);
	clear_session(session);
	session->quit = 1;
	return (strdup(RESP_OK " " RESP_BYE));
}

// --- funzioni di supporto ----------------------------------------------------

// traduce un codice STORAGE_* nella risposta "ERR ..." da mandare al client.
// conflict_id serve solo per STORAGE_CONFLICT
static char	*storage_err(int code, int conflict_id)
{
	char	resp[MAX_LINE];

	if (code == STORAGE_NOT_FOUND)
		return (strdup(RESP_ERR " " ERR_NOT_FOUND));
	if (code == STORAGE_CONFLICT)
	{
		snprintf(resp, sizeof(resp), "%s %s %d", RESP_ERR, ERR_CONFLICT, conflict_id);
		return (strdup(resp));
	}
	return (strdup(RESP_ERR " " ERR_SERVER));
}

// aggiunge "\n" + row in fondo a *body (che puo' essere NULL: lista vuota).
// ritorna 0 se ok, -1 in caso di errore (*body e' gia' stata liberata e vale NULL)
static int	append_row(char **body, const char *row)
{
	char	*with_nl;

	with_nl = strjoin(*body, "\n");
	free(*body);
	*body = NULL;
	if (!with_nl)
		return (-1);
	*body = strjoin(with_nl, row);
	free(with_nl);
	if (!*body)
		return (-1);
	return (0);
}

// compone "OK <count>" + le righe accumulate con append_row, e libera body
static char	*list_response(int count, char *body)
{
	char	head[MAX_LINE];
	char	*resp;

	snprintf(head, sizeof(head), "%s %d", RESP_OK, count);
	resp = strjoin(head, body);
	free(body);
	if (!resp)
		return (strdup(RESP_ERR " " ERR_SERVER));
	return (resp);
}

// 1 se la prenotazione soddisfa il filtro "chiave=valore", 0 altrimenti.
// La sintassi del filtro e' gia' stata controllata da check_cmd
static int	matches_filter(const t_booking *b, const char *filter)
{
	const char		*value;
	int				id;
	t_date			date;
	enum e_status	status;

	value = strchr(filter, '=');
	if (!value)
		return (0);
	value++;
	if (strncmp(filter, "id=", 3) == 0)
		return (str_to_int(value, &id) == 0 && b->id == id);
	if (strncmp(filter, "station=", 8) == 0)
		return (strcmp(b->station, value) == 0);
	if (strncmp(filter, "date=", 5) == 0)
		return (str_to_date(value, &date) == 0 && datecmp(&b->date, &date) == 0);
	if (strncmp(filter, "status=", 7) == 0)
		return (str_to_status(value, &status) == 0 && b->status == status);
	if (strncmp(filter, "user=", 5) == 0)
		return (strcmp(b->username, value) == 0);
	return (0);
}

// 1 se la prenotazione soddisfa TUTTI i filtri (da argv[1] in poi) e, se owner
// non e' NULL, appartiene a owner
static int	matches_all(const t_booking *b, char **argv, int argc, const char *owner)
{
	int	i;

	if (owner && strcmp(b->username, owner) != 0)
		return (0);
	i = 1;
	while (i < argc)
	{
		if (!matches_filter(b, argv[i]))
			return (0);
		i++;
	}
	return (1);
}

// parte comune di MY e ALL: "OK <n>" + n righe "BOOKING ...".
// owner = utente di cui mostrare le prenotazioni, NULL = tutti
static char	*list_bookings(char **argv, int argc, t_session *session, const char *owner)
{
	t_booking	*arr;
	char		row[MAX_LINE];
	char		*body;
	int			count;
	int			found;
	int			i;

	arr = storage_list_bookings(session->booking_fd, &count);
	if (!arr)
		return (strdup(RESP_ERR " " ERR_SERVER));
	body = NULL;
	found = 0;
	i = 0;
	while (i < count)
	{
		if (matches_all(&arr[i], argv, argc, owner))
		{
			if (booking_to_row(&arr[i], row, sizeof(row)) < 0
				|| append_row(&body, row) < 0)
			{
				free(arr);
				free(body);
				return (strdup(RESP_ERR " " ERR_SERVER));
			}
			found++;
		}
		i++;
	}
	free(arr);
	return (list_response(found, body));
}

// parte comune di APPROVE, REJECT e SET: cambia lo stato della prenotazione.
// Se show_rejected vale 1 aggiunge alla risposta il numero di richieste
// rifiutate automaticamente (solo APPROVE)
static char	*change_status(t_session *session, const char *id_str,
	enum e_status status, int show_rejected)
{
	char	resp[MAX_LINE];
	int		id;
	int		conflict_id;
	int		ret;

	if (str_to_int(id_str, &id) < 0)
		return (strdup(RESP_ERR " " ERR_SYNTAX));
	conflict_id = 0;
	ret = storage_set_booking_status(session->booking_fd, id, status, &conflict_id);
	if (ret < 0)
		return (storage_err(ret, conflict_id));
	if (show_rejected)
		snprintf(resp, sizeof(resp), "%s %d %s %d", RESP_OK, id, status_to_str(status), ret);
	else
		snprintf(resp, sizeof(resp), "%s %d %s", RESP_OK, id, status_to_str(status));
	return (strdup(resp));
}

// --- comandi utente ----------------------------------------------------------

// STATIONS: risponde con "OK <n>" seguito da n righe "STATION <id> <tipo>"
char	*exec_stations(t_session *session)
{
	t_lines	lines;
	char	row[MAX_LINE];
	char	*body;
	int		count;
	int		i;

	(void)session;
	if (storage_read_stations(&lines) < 0)
		return (strdup(RESP_ERR " " ERR_SERVER));
	body = NULL;
	i = 0;
	while (i < lines.count)
	{
		if (station_row(lines.lines[i], row, sizeof(row)) < 0
			|| append_row(&body, row) < 0)
		{
			fprintf(stderr, "stations: errore alla riga \"%s\"\n", lines.lines[i]);
			storage_free_lines(&lines);
			free(body);
			return (strdup(RESP_ERR " " ERR_SERVER));
		}
		i++;
	}
	count = lines.count;
	storage_free_lines(&lines);
	return (list_response(count, body));
}

// BOOK <postazione> <data> <inizio> <fine>: risponde con "OK <id> PENDING".
// Il richiedente e' SEMPRE l'utente della sessione, non lo decide il client.
// Il controllo dei conflitti e l'inserimento avvengono in storage_add_booking,
// sotto un unico lock in scrittura.
char	*exec_book(char **argv, t_session *session)
{
	t_booking	*booking;
	t_date		date;
	t_time		start;
	t_time		end;
	char		resp[MAX_LINE];
	int			conflict_id;
	int			ret;

	ret = storage_find_station(argv[1]);
	if (ret == STORAGE_NOT_FOUND)
		return (strdup(RESP_ERR " " ERR_NO_SUCH_STATION));
	if (ret < 0)
		return (strdup(RESP_ERR " " ERR_SERVER));
	// check_cmd ha gia' validato i formati: qui si converte soltanto
	if (str_to_date(argv[2], &date) < 0)
		return (strdup(RESP_ERR " " ERR_INVALID_DATE));
	if (str_to_time(argv[3], &start) < 0 || str_to_time(argv[4], &end) < 0)
		return (strdup(RESP_ERR " " ERR_INVALID_TIME));
	booking = init_booking(argv[1], session->user.username, &date,
			&start, &end, STATUS_PENDING);
	if (!booking)
		return (strdup(RESP_ERR " " ERR_SERVER));
	conflict_id = 0;
	ret = storage_add_booking(session->booking_fd, booking, &conflict_id);
	free(booking);
	// privacy: a un utente standard non si dice l'id della prenotazione altrui
	if (ret == STORAGE_CONFLICT && !session->user.is_admin)
		return (strdup(RESP_ERR " " ERR_CONFLICT));
	if (ret < 0)
		return (storage_err(ret, conflict_id));
	snprintf(resp, sizeof(resp), "%s %d %s", RESP_OK, ret, status_to_str(STATUS_PENDING));
	return (strdup(resp));
}

// MY [filtri...]: solo le prenotazioni dell'utente loggato
char	*exec_my(char **argv, int argc, t_session *session)
{
	return (list_bookings(argv, argc, session, session->user.username));
}

// --- comandi admin (i permessi li controlla check_cmd) -----------------------

// ALL [filtri...]: le prenotazioni di tutti gli utenti
char	*exec_all(char **argv, int argc, t_session *session)
{
	return (list_bookings(argv, argc, session, NULL));
}

// APPROVE <id>: risponde con "OK <id> APPROVED <k>", k = richieste PENDING in
// conflitto rifiutate automaticamente
char	*exec_approve(char **argv, t_session *session)
{
	return (change_status(session, argv[1], STATUS_APPROVED, 1));
}

// REJECT <id>: risponde con "OK <id> REJECTED"
char	*exec_reject(char **argv, t_session *session)
{
	return (change_status(session, argv[1], STATUS_REJECTED, 0));
}

// SET <id> <stato>: risponde con "OK <id> <stato>"
char	*exec_set(char **argv, t_session *session)
{
	enum e_status	status;

	if (str_to_status(argv[2], &status) < 0)
		return (strdup(RESP_ERR " " ERR_INVALID_STATUS));
	return (change_status(session, argv[1], status, 0));
}

// CONFLICTS: "OK <n>" + n righe "CONFLICT <id1> <id2>" con id1 < id2.
// Confronta ogni prenotazione solo con le successive (j > i), cosi' ogni
// coppia compare una volta sola
char	*exec_conflicts(t_session *session)
{
	t_booking	*arr;
	char		row[MAX_LINE];
	char		*body;
	int			count;
	int			found;
	int			i;
	int			j;

	arr = storage_list_bookings(session->booking_fd, &count);
	if (!arr)
		return (strdup(RESP_ERR " " ERR_SERVER));
	body = NULL;
	found = 0;
	i = 0;
	while (i < count)
	{
		j = i + 1;
		while (j < count)
		{
			if (booking_overlaps(&arr[i], &arr[j]))
			{
				// il file potrebbe non essere ordinato per id
				snprintf(row, sizeof(row), "%s %d %d", ERR_CONFLICT,
					arr[i].id < arr[j].id ? arr[i].id : arr[j].id,
					arr[i].id < arr[j].id ? arr[j].id : arr[i].id);
				if (append_row(&body, row) < 0)
				{
					free(arr);
					return (strdup(RESP_ERR " " ERR_SERVER));
				}
				found++;
			}
			j++;
		}
		i++;
	}
	free(arr);
	return (list_response(found, body));
}

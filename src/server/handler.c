#include "./server.h"

char	*exec_login(char **argv, t_session *session); 
char	*exec_logout(char **argv, t_session *session);
char	*exec_book(char **argv, t_session *session);
char	*exec_stations(t_session *session);
char	*exec_quit(t_session *session);
char	*exec_my(char **argv, t_session *session);
char	*exec_all(char **argv, t_session *session);
char	*exec_approve(char **argv, t_session *session);
char	*exec_reject(char **argv, t_session *session);
char	*exec_set(char **argv, t_session *session);
char	*exec_conflicts(char **argv, t_session *session);

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
	// TODO: compara cmd con tutti i comandi ed esegui il corrispettivo exec_
	return (strdup("TESTING"));
}

// esegue il login modificando i dati in session, ritorna una risposta
char	*exec_login(char **argv, t_session *session)
{
	t_user	user;
	char	resp[MAX_LINE];

	if (storage_find_user(session->users_fd, argv[1], &user) < 0)
		return (strdup(RESP_ERR " " ERR_BAD_CREDENTIALS));
	strcpy(session->user.username, user.username);
	strcpy(session->user.password, user.password);
	session->user.is_admin = user.is_admin;
	if (user.is_admin)
		session->role = ROLE_ADMIN;
	else
		session->role = ROLE_USER;
	snprintf(resp, sizeof(resp), "%s %s", RESP_OK, session->role);
	return (strdup(resp));
}

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

// campi di una riga di stations_db.csv: id,nome,descrizione
enum e_station_field
{
	ST_ID,
	ST_NAME,
	ST_DESC,
	ST_FIELDS
};

// libera quello che exec_stations ha allocato e ritorna ERR SERVER
static char	*stations_error(t_lines *lines, char *body)
{
	storage_free_lines(lines);
	free(body);
	return (strdup(RESP_ERR " " ERR_SERVER));
}

// trasforma la riga CSV "2,pc-01,postazione pc" in "\nSTATION pc-01 postazione pc"
// (con il '\n' DAVANTI: cosi' l'ultima riga non ha '\n' e lo aggiunge send_line)
// ritorna 0 se ok, -1 se la riga e' malformata
static int	station_row(const char *csv, char *row, size_t size)
{
	char	copy[MAX_LINE];
	char	*fields[ST_FIELDS];
	char	*desc;

	if (copy_str(copy, csv, sizeof(copy)) < 0
		|| split_fields(copy, ',', fields, ST_FIELDS) != ST_FIELDS
		|| fields[ST_NAME][0] == '\0')
		return (-1);
	desc = fields[ST_DESC];
	while (*desc == ' ') // nel CSV ci sono descrizioni come " ps5"
		desc++;
	if (snprintf(row, size, "\nSTATION %s %s", fields[ST_NAME], desc) >= (int)size)
		return (-1);
	return (0);
}

// STATIONS: risponde con "OK <n>" seguito da n righe "STATION <nome> <descrizione>",
// tutte in un'unica stringa separata da '\n'.
char	*exec_stations(t_session *session)
{
	t_lines	lines;
	char	row[MAX_LINE];
	char	head[MAX_LINE];
	char	*body;
	char	*tmp;
	int		fd;
	int		i;

	(void)session;
	fd = storage_open(STATIONS_PATH, HEADER_STATIONS);
	if (fd < 0)
		return (strdup(RESP_ERR " " ERR_SERVER));
	if (lock_file(fd, F_RDLCK) < 0)
	{
		close(fd);
		return (strdup(RESP_ERR " " ERR_SERVER));
	}
	i = storage_read_lines(fd, HEADER_STATIONS, &lines);
	unlock_file(fd);
	close(fd);
	if (i < 0)
		return (strdup(RESP_ERR " " ERR_SERVER));
	body = strdup("");
	if (!body)
		return (stations_error(&lines, NULL));
	i = 0;
	while (i < lines.count)
	{
		if (station_row(lines.lines[i], row, sizeof(row)) < 0)
		{
			fprintf(stderr, "stations: riga malformata: \"%s\"\n", lines.lines[i]);
			return (stations_error(&lines, body));
		}
		tmp = body;
		body = strjoin(body, row);
		free(tmp);
		if (!body)
			return (stations_error(&lines, NULL));
		i++;
	}
	snprintf(head, sizeof(head), "%s %d", RESP_OK, lines.count);
	storage_free_lines(&lines);
	tmp = body;
	body = strjoin(head, body);
	free(tmp);
	if (!body)
		return (strdup(RESP_ERR " " ERR_SERVER));
	return (body);
}

char	*exec_quit(t_session *session)
{
	close(session->users_fd);
	close(session->booking_fd);
	clear_session(session);
	return (strdup(RESP_OK " " RESP_BYE));
}


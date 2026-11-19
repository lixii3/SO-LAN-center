#include "./server.h"

// inizializza la struttura t_session con failed_logins = 0,
// username e role vuoti.
// Nel caso di errore ritorna una struttura con failed_logins == -1;
t_session	init_session(int bfd, int ufd)
{
	t_session	session;

	session.failed_logins = -1;
	if (bfd < 0 || ufd < 0)
		return session;
	session.booking_fd = ufd;
	session.users_fd = bfd;
	session.role = NULL;
	bzero(session.user.username, MAX_USERNAME);
	bzero(session.user.password, MAX_PASSWORD);
	session.failed_logins = 0;
	return (session);
}

// imposta il ruolo dell'utente in role. Se role è NULL,
// il ruolo viene assegnato ricavandolo dal file ufd contenente i login.
// Ritorna NULL per il successo oppure un codice di errore ERR_NOT_FOUND
// o ERR_SYNTAX se il ruolo passato non è valido.
const char	*set_role(t_session *session, char *role, int ufd, char	*username)
{
	if (!session)
		return (ERR_SERVER);
	if (role)
	{
		if (strcmp(role, ROLE_ADMIN) == 0)
			session->role = ROLE_ADMIN;
		else if (strcmp(role, ROLE_USER) == 0)
			session->role = ROLE_USER;
		else
			return (ERR_SYNTAX);
		return (NULL);
	}
	if (storage_find_user(ufd, username, &session->user) != 0)
		return (ERR_NOT_FOUND);
	if (session->user.is_admin)
		session->role = ROLE_ADMIN;
	else
		session->role = ROLE_USER;
	return (NULL);
}

// inizializza un server. Esegue anche il bind.
// Ritorna il server oppure esegue una exit;
t_server	init_server(short port, const char *address)
{
	t_server	server;
	// apertura socket del server 
	if ((server.socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error in server socket()");
		exit(EXIT_FAILURE);
	}
	// preparazione dell'indirizzo locale del server 
	server.address.sin_family = AF_INET;
	server.address.sin_port = htons(port);
	server.address.sin_addr.s_addr = inet_addr(address);

	// binding dell'indirizzo al socket
	if (bind(server.socket, (struct sockaddr*)	&server.address, sizeof(server.address)) == -1)
	{
		perror("Error in server socket bind()");
		exit(EXIT_FAILURE);
	}
	return (server);
}

// Gestisce la sessione, chiama handle_cmd per
// eseguire il comando ricevuto dal client connsesso.
// Ritorna NULL oppure un codice di errore ERR_.
const char	*session(int connectSocket, struct sockaddr_in clientAddress)
{
	t_session	session;
	int			book_fd;
	int			us_fd;
	char		*cmd[MAX_ARGS];
	char		*clientIP;
	int			returnCode;
	const char	*resp;
	char		buffer[MAX_LINE];
	int			argc;

	book_fd = storage_open(BOOKINGS_PATH, HEADER_BOOKING);
	us_fd = storage_open(USERS_PATH, HEADER_USERS);
	session = init_session(book_fd, us_fd);
	if (session.failed_logins < 0)
		return (ERR_SERVER);
	// memorizzo l'indirizzo del client in formato dots
	clientIP = inet_ntoa(clientAddress.sin_addr);
	printf("\nClient @ %s connects on socket %d\n", clientIP, connectSocket);
		// riceve i messaggi
	while ((returnCode = recv_line(connectSocket, buffer, MAX_LINE)) != 0)
	{
		if (returnCode == NET_TOO_LONG)
			send_line(connectSocket, RESP_ERR " " ERR_LINE_TOO_LONG);
		else if (returnCode < 0)
			send_line(connectSocket,  RESP_ERR " " ERR_SERVER);
		else
		{
			if ((argc = split_cmd(buffer, cmd)) <= 0)
				send_line(connectSocket, RESP_ERR " " ERR_SYNTAX);
			resp = handle_cmd(cmd, argc, &session);
			send_line(connectSocket, resp);
		}
	}
	return (NULL);
}

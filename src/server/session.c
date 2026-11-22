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
	session.booking_fd = bfd;
	session.users_fd = ufd;
	session.role = NULL;
	bzero(session.user.username, MAX_USERNAME);
	bzero(session.user.password, MAX_PASSWORD);
	session.failed_logins = 0;
	return (session);
}

void	clear_session(t_session *session)
{
	if (!session)
		return;
	session->booking_fd = -1;
	session->users_fd = -1;
	clear_user(&session->user);
	session->role = NULL;
	session->failed_logins = 0;
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

// inizializza un server. Esegue anche il bind() e listen() con max_conn.
// address e' un IPv4 in formato dots: "127.0.0.1" accetta solo connessioni
// dalla stessa macchina, "0.0.0.0" (INADDR_ANY) da tutte le interfacce.
// Ritorna il server oppure esegue una exit;
t_server	init_server(unsigned short port, const char *address, int max_conn)
{
	t_server	server;
	int			reuse;

	// apertura socket del server
	if ((server.socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error in server socket()");
		exit(EXIT_FAILURE);
	}
	// preparazione dell'indirizzo locale del server 
	server.address.sin_family = AF_INET;
	server.address.sin_port = htons(port);
	// inet_pton al posto di inet_addr: inet_addr non distingue un indirizzo
	// non valido da "255.255.255.255" (ritornano entrambi INADDR_NONE)
	if (inet_pton(AF_INET, address, &server.address.sin_addr) != 1)
	{
		fprintf(stderr, "Indirizzo IPv4 non valido: %s\n", address);
		close(server.socket);
		exit(EXIT_FAILURE);
	}

	// permette di rifare bind() sulla stessa porta subito dopo
	// il riavvio del server se quello vecchio è in TIMEWAIT;
	reuse = 1;
	if (setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR,
			&reuse, sizeof(reuse)) == -1)
	{
		perror("Error in server setsockopt()");
		close(server.socket);
		exit(EXIT_FAILURE);
	}
	// binding dell'indirizzo al socket
	if (bind(server.socket, (struct sockaddr*)	&server.address, sizeof(server.address)) == -1)
	{
		perror("Error in server socket bind()");
		close(server.socket);
		exit(EXIT_FAILURE);
	}
	if (listen(server.socket, max_conn) < 0)
	{
		perror("Error in server listen()");
		close(server.socket);
		exit(EXIT_FAILURE);
	}
	return (server);
}

// Gestisce la sessione, chiama handle_cmd per
// eseguire il comando ricevuto dal client connsesso.
// Ritorna NULL se il client chiude la sessione regolareìmente,
// oppure un codice di errore ERR_.
const char	*session(int connectSocket, struct sockaddr_in clientAddress)
{
	t_session	session;
	int			book_fd;
	int			us_fd;
	char		*argv[MAX_ARGS];
	char		*clientIP;
	int			returnCode;
	char		*resp;
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
	while (1)
	{
		returnCode = recv_line(connectSocket, buffer, MAX_LINE);
		if (returnCode == NET_TOO_LONG)
			send_line(connectSocket, RESP_ERR " " ERR_LINE_TOO_LONG);
		else if (returnCode == NET_EMPTY_LINE)
			send_line(connectSocket, RESP_ERR " " ERR_SYNTAX);
		else if (returnCode == NET_CLOSED)
			return (NULL);
		else if (returnCode == NET_ERROR)
			return (ERR_SERVER);
		else
		{
			if ((argc = split_cmd(buffer, argv)) <= 0)
			{
				send_line(connectSocket, RESP_ERR " " ERR_SYNTAX);
				continue;
			}
			resp = handle_cmd(argv, argc, &session);
			send_line(connectSocket, resp);
			// se viene eseguito un QUIT
			if (strcmp(resp, RESP_OK " " RESP_BYE) == 0)
			{
				free(resp);
				return (NULL);
			}
			free(resp);
		}
	}
}

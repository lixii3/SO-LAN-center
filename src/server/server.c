#include "../headers/net.h"
#include "../headers/utils.h"
#include "../headers/storage.h"
#include "./server.h"

// socket di ascolto, globale perche' deve vederlo l'handler di SIGINT
static int	g_listen_socket = -1;

void		childHandler(int sig);
static void	intHandler(int sig);
static int	check_data_files(void);
static int	parse_args(int argc, char **argv, const char **address,
				unsigned short *port);

// uso: ./server [indirizzo] [porta]
//   ./server                 -> 127.0.0.1:12345 (solo connessioni locali)
//   ./server 0.0.0.0         -> tutte le interfacce (client da altre macchine)
//   ./server 0.0.0.0 4000    -> tutte le interfacce, porta 4000
int main(int argc, char **argv) {
	t_server			server_info;
	int					connectSocket;
	socklen_t			clientAddressLen;
	struct sockaddr_in	clientAddress;
	int					pid;
	const char			*address;
	unsigned short		port;

	if (parse_args(argc, argv, &address, &port) < 0)
		return (EXIT_FAILURE);
	// controllo i file prima di aprire il socket
	if (check_data_files() < 0)
		return (EXIT_FAILURE);

	// gestione segnali
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, childHandler);
	signal(SIGINT, intHandler);

	server_info = init_server(port, address, MAX_CONN);
	g_listen_socket = server_info.socket;
	printf("Server ready su %s:%u (CTRL-C per terminare)\n", address, port);
	clientAddressLen = sizeof(clientAddress);
	// attesa di connessione
	while (1) {
		connectSocket = accept(server_info.socket, (struct sockaddr *)&clientAddress, &clientAddressLen);
		// se accept è stata interrotta da SIGCHLD: riprovo
		if (connectSocket == -1)
		{
			if (errno == EINTR)
				continue;
			perror("Error in accept()");
			exit(-1);
		}
		// svuoto il buffer di printf prima della fork, altrimenti ogni figlio
		// ne eredita una copia e la ristampa quando fa exit()
		fflush(stdout);
		pid = fork();
		if (pid == 0)
		{
			// il figlio non deve eseguire intHandler
			signal(SIGINT, SIG_DFL);
			close(server_info.socket);
			session(connectSocket, clientAddress);
			exit(0);
		}
		else if (pid < 0)
			perror("Error in fork()");
		close(connectSocket);
	}
}

void	childHandler(int sig)
{
	int	saved_errno;

	(void)sig; // silenzio -Wextra
	saved_errno = errno;
	while (waitpid(-1, NULL, WNOHANG) > 0) // -1 = qualsiasi figlio; ripeto finche' ce ne sono
		continue;
	errno = saved_errno;
}

// CTRL-C sul server chiude il socket di ascolto e termina.
static void	intHandler(int sig)
{
	const char	msg[] = "\nServer terminato (SIGINT)\n";

	(void)sig; // silenzio -Wextra
	if (g_listen_socket >= 0)
		close(g_listen_socket);
	write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	_exit(EXIT_SUCCESS);
}

// controlla che i file dell'archivio si possano aprire (o creare).
// I percorsi sono relativi
// ritorna 0 se ok, -1 altrimenti
static int	check_data_files(void)
{
	const char	*paths[] = {BOOKINGS_PATH, USERS_PATH, STATIONS_PATH};
	const char	*headers[] = {HEADER_BOOKING, HEADER_USERS, HEADER_STATIONS};
	int			npaths;
	int			fd;
	int			i;

	i = 0;
	npaths = 3;
	while (i < npaths)
	{
		fd = storage_open(paths[i], headers[i]);
		if (fd < 0)
		{
			perror(paths[i]);
			fprintf(stderr, "Impossibile aprire l'archivio: lanciare il server "
				"dalla root del progetto (es. make run-server)\n");
			return (-1);
		}
		close(fd);
		i++;
	}
	return (0);
}

// legge gli argomenti opzionali [indirizzo] [porta]. Senza argomenti usa
// SERVER_ADDRESS e SERVER_PORT di protocol.h.
// ritorna 0 se ok, -1 altrimenti
static int	parse_args(int argc, char **argv, const char **address,
	unsigned short *port)
{
	struct in_addr	tmp;
	int				n;

	*address = SERVER_ADDRESS;
	*port = SERVER_PORT;
	if (argc > 3)
	{
		fprintf(stderr, "Uso: %s [indirizzo] [porta]\n", argv[0]);
		return (-1);
	}
	if (argc >= 2)
	{
		if (inet_pton(AF_INET, argv[1], &tmp) != 1)
		{
			fprintf(stderr, "Indirizzo IPv4 non valido: %s\n", argv[1]);
			return (-1);
		}
		*address = argv[1];
	}
	if (argc == 3)
	{
		if (str_to_int(argv[2], &n) < 0 || n < 1 || n > 65535)
		{
			fprintf(stderr, "Porta non valida: %s (1-65535)\n", argv[2]);
			return (-1);
		}
		*port = (unsigned short)n;
	}
	return (0);
}

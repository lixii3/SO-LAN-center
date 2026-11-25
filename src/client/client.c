#include "./client.h"
#include <signal.h>

static void	intHandler(int sig);
static int	is_list_cmd(const char *msg);

// uso: ./client [indirizzo-server]
// La sessione termina solo con: QUIT, CTRL-C, CTRL-D (equivale a QUIT),
// troppi login falliti, connessione con il server persa.
int main(int argc, char *argv[]) {
	t_client	client;
	t_resp		resp;
	char		code[MAX_LINE];
	const char	*serverIP;
	char		msg[MAX_LINE]; // 255 caratteri + '\0'
	int			ret;
	int			i;

	if (argc > 2)
	{
		fprintf(stderr, "Usage: %s [server-ip]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	serverIP = SERVER_ADDRESS;
	if (argc == 2)
		serverIP = argv[1];

	// se il server cade, scrivere sul socket genera SIGPIPE, che terminerebbe
	// il client senza messaggi: ignorandolo e' send_line a ritornare errore
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, intHandler);

	bzero(&client, sizeof(client));
	client.role = R_GUEST;
	client.fd = connect_server(serverIP);
	printf("Client connesso a %s:%d\n", serverIP, SERVER_PORT);
	print_banner(BANNER_PATH);
	printf("\nScrivi un comando (QUIT o CTRL-C per uscire)\n");
	while (1)
	{
		ret = read_line("TU: ", msg, sizeof(msg));
		if (ret == RET_CANCEL) // riga vuota: niente da inviare
			continue;
		if (ret == RET_EOF) // CTRL-D: chiudo la sessione regolarmente
			copy_str(msg, CMD_QUIT, sizeof(msg));

		ret = request(&client, msg, is_list_cmd(msg), &resp);
		if (ret == RET_LOST)
		{
			fprintf(stderr, "Connessione con il server persa\n");
			free_resp(&resp);
			close(client.fd);
			exit(EXIT_FAILURE);
		}
		printf("S: %s\n", resp.first);
		i = 0;
		while (i < resp.n)
			printf("S: %s\n", resp.rows[i++]);
		free_resp(&resp); // libera solo le righe: resp.first resta valida

		// strcmp ritorna 0 se le stringhe sono uguali
		if (strcmp(resp.first, RESP_OK " " RESP_BYE) == 0)
		{
			close(client.fd);
			exit(EXIT_SUCCESS);
		}
		// dopo questa risposta il server chiude la connessione
		resp_code(&resp, code, sizeof(code));
		if (strcmp(code, ERR_TOO_MANY_ATTEMPTS) == 0)
		{
			fprintf(stderr, "Troppi login falliti: sessione terminata\n");
			close(client.fd);
			exit(EXIT_FAILURE);
		}
	}
}

// CTRL-C: termina il client. Il socket viene chiuso dal kernel, quindi il
// server riceve NET_CLOSED e chiude la sessione.
static void	intHandler(int sig)
{
	const char	msg[] = "\nClient terminato (SIGINT)\n";

	(void)sig; // silenzio -Wextra
	// solo funzioni async-signal-safe: niente printf o exit
	write_all(STDOUT_FILENO, msg, sizeof(msg) - 1);
	_exit(EXIT_SUCCESS);
}

// ritorna 1 se la risposta al comando msg e' un elenco, 0 altrimenti.
static int	is_list_cmd(const char *msg)
{
	const char	*cmds[] = {CMD_STATIONS, CMD_MY, CMD_ALL, CMD_CONFLICTS};
	size_t		len;
	int			i;

	len = strcspn(msg, " \r"); // lunghezza della prima parola
	i = 0;
	while (i < 4)
	{
		if (strlen(cmds[i]) == len && strncmp(msg, cmds[i], len) == 0)
			return (1);
		i++;
	}
	return (0);
}

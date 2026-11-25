#include "./client.h"

/*
** Comunicazione con il server (vedi PROTOCOL.md).
** Le funzioni di questo file non stampano le risposte: lo fa chi le chiama.
*/

// --- funzioni di supporto ----------------------------------------------------

// 1 se la prima parola di str e' word, 0 altrimenti.
// Es: "OK" e "OK 17 PENDING" iniziano con la parola "OK", "OKAY" no
static int	first_word_is(const char *str, const char *word)
{
	size_t	len;

	len = strlen(word);
	return (strncmp(str, word, len) == 0
		&& (str[len] == '\0' || str[len] == ' '));
}

// legge le n righe di un elenco e le salva in out->rows.
// out->n conta le righe gia' salvate: se qualcosa va storto a meta' elenco,
// free_resp libera solo quelle.
// ritorna RET_OK oppure RET_LOST
static int	recv_rows(int fd, t_resp *out, int n)
{
	char	line[MAX_LINE];

	if (n == 0) // elenco vuoto: rows resta NULL
		return (RET_OK);
	out->rows = malloc((size_t)n * sizeof(char *));
	if (!out->rows)
		return (RET_LOST);
	while (out->n < n)
	{
		if (recv_line(fd, line, sizeof(line)) < 0)
			return (RET_LOST);
		out->rows[out->n] = strdup(line);
		if (!out->rows[out->n])
			return (RET_LOST);
		out->n++;
	}
	return (RET_OK);
}


// apre il socket e lo connette al server.
// Ritorna il socket oppure esegue una exit
int	connect_server(const char *serverIP)
{
	int					clientSocket;
	struct sockaddr_in	serverAddress;

	/* apertura socket del client */
	if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error in socket()");
		exit(EXIT_FAILURE);
	}

	/* preparazione dell'indirizzo del server con cui connettere il socket */
	bzero(&serverAddress, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);
	if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) != 1)
	{
		fprintf(stderr, "Invalid server IPv4 address: %s\n", serverIP);
		close(clientSocket);
		exit(EXIT_FAILURE);
	}

	/* connessione del socket al server */
	if (connect(clientSocket, (struct sockaddr *)&serverAddress,
			sizeof(serverAddress)) == -1)
	{
		perror("Error in connect()");
		close(clientSocket);
		exit(EXIT_FAILURE);
	}
	return (clientSocket);
}

// invia il comando, riceve la risposta e la scrive in out.
// Ritorna RET_OK se il server ha risposto.
// Ritorna RET_LOST se la connessione non e' piu' utilizzabile.
// Dopo la chiamata va sempre usata free_resp(out), qualunque sia il ritorno.
int	request(t_client *c, const char *cmd, int is_list, t_resp *out)
{
	int	n;

	if (!out)
		return (RET_LOST);
	bzero(out, sizeof(*out)); // rows = NULL: free_resp e' sempre sicura
	if (!c || !cmd)
		return (RET_LOST);
	if (send_line(c->fd, cmd) < 0)
		return (RET_LOST);
	if (recv_line(c->fd, out->first, sizeof(out->first)) < 0)
		return (RET_LOST);
	out->ok = first_word_is(out->first, RESP_OK);
	if (!out->ok)
	{
		if (first_word_is(out->first, RESP_ERR))
			return (RET_OK);
		return (RET_LOST); // ne' OK ne' ERR
	}
	if (!is_list)
		return (RET_OK);
	// elenco: dopo "OK" ci deve essere uno spazio e il numero di righe
	if (out->first[strlen(RESP_OK)] != ' '
		|| str_to_int(out->first + strlen(RESP_OK) + 1, &n) < 0)
		return (RET_LOST);
	return (recv_rows(c->fd, out, n));
}

// libera le righe dell'elenco.
void	free_resp(t_resp *r)
{
	if (!r)
		return ;
	while (r->n > 0)
		free(r->rows[--r->n]);
	free(r->rows);
	r->rows = NULL;
}

// copia in buf il codice di errore della risposta.
// Es: "ERR CONFLICT 17" -> "CONFLICT"
// buf diventa la stringa vuota se la risposta non e' un errore o se il codice
// manca o non entra in buf.
void	resp_code(const t_resp *r, char *buf, size_t size)
{
	const char	*code;
	size_t		len;

	if (!buf || size == 0)
		return ;
	buf[0] = '\0';
	if (!r || !first_word_is(r->first, RESP_ERR)
		|| r->first[strlen(RESP_ERR)] == '\0')
		return ;
	code = r->first + strlen(RESP_ERR) + 1;
	len = strcspn(code, " "); // il codice finisce al primo spazio o a '\0'
	if (len >= size)
		return ;
	memcpy(buf, code, len);
	buf[len] = '\0';
}

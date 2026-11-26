#include "./client.h"

/*
** Stampa a schermo: colori, tabelle, messaggi di errore.
** Qui non si legge e non si scrive sul socket: le risposte arrivano gia'
** ricevute da request() (vedi request.c).
*/

// colonne della tabella delle prenotazioni. Le righe del protocollo sono
// "BOOKING <id> <postazione> <data> <inizio> <fine> <utente> <stato>":
// la prima parola non si stampa, le altre 7 sono le colonne.
#define BOOKING_FIELDS	8
#define BOOKING_COLS	7

// --- funzioni di supporto ----------------------------------------------------

// stampa una linea orizzontale: +------+----------+ ...
static void	print_hline(const int *widths, int ncols)
{
	int	i;
	int	j;

	i = 0;
	while (i < ncols)
	{
		printf("+");
		j = 0;
		while (j < widths[i] + 2) // +2: lo spazio prima e dopo il campo
		{
			printf("-");
			j++;
		}
		i++;
	}
	printf("+\n");
}

// colore di un campo: verde/giallo/rosso per gli stati, nessuno per il resto
static const char	*field_color(const char *field)
{
	if (strcmp(field, "APPROVED") == 0)
		return (col(C_GREEN));
	if (strcmp(field, "PENDING") == 0)
		return (col(C_YELLOW));
	if (strcmp(field, "REJECTED") == 0)
		return (col(C_RED));
	return ("");
}

// stampa una riga: | campo1 | campo2 | ...
// I codici del colore stanno FUORI da %-*s: sono caratteri anche loro, e
// dentro la larghezza sposterebbero le colonne.
static void	print_cells(const char *const *fields, const int *widths, int ncols)
{
	const char	*color;
	int			i;

	i = 0;
	while (i < ncols)
	{
		color = field_color(fields[i]);
		// con color == "" anche il C_RESET va tolto
		printf("| %s%-*s%s ", color, widths[i], fields[i],
			color[0] ? col(C_RESET) : "");
		i++;
	}
	printf("|\n");
}

// --- funzioni pubbliche ------------------------------------------------------

// ritorna code se stdout e' un terminale, altrimenti "".
// Se l'output e' rediretto su file (./client > log) i codici ANSI non vengono
// interpretati e finirebbero nel file come caratteri strani.
const char	*col(const char *code)
{
	if (isatty(STDOUT_FILENO))
		return (code);
	return ("");
}

// stampa il contenuto di un file di testo.
// Se il file non si puo' leggere non stampa niente
void	print_banner(const char *filepath)
{
	char	*buff;
	int		fd;

	fd = open(filepath, O_RDONLY);
	if (fd < 0)
		return ;
	buff = read_all(fd, NULL);
	close(fd);
	if (!buff)
		return ;
	printf("%s", buff); // "%s": un '%' nel file non va letto come formato
	free(buff);
}

// pulisce lo schermo e porta il cursore in alto a sinistra
void	clear_screen(void)
{
	// "\033[2J" pulisce lo schermo, "\033[H" porta il cursore in riga 1 colonna 1
	printf("%s%s", col("\033[2J"), col("\033[H"));
	fflush(stdout);
}

// aspetta che l'utente prema INVIO.
// ritorna RET_OK oppure RET_EOF (CTRL-D)
int	wait_enter(void)
{
	char	buf[MAX_LINE];

	// read_line scarta anche quello che l'utente ha scritto prima di INVIO
	if (read_line("\nPremi INVIO per continuare...", buf, sizeof(buf)) == RET_EOF)
		return (RET_EOF);
	return (RET_OK);
}

// stampa in italiano l'errore contenuto in una risposta "ERR <CODICE> ..."
void	print_error(const t_resp *r)
{
	// coppie codice -> messaggio, terminate da NULL
	static const char	*msgs[][2] = {
	{ERR_UNKNOWN_COMMAND, "Comando sconosciuto"},
	{ERR_SYNTAX, "Parametri mancanti o scritti male"},
	{ERR_LINE_TOO_LONG, "Comando troppo lungo"},
	{ERR_NOT_LOGGED, "Devi prima fare il login"},
	{ERR_ALREADY_LOGGED, "Hai gia' fatto il login"},
	{ERR_BAD_CREDENTIALS, "Username o password errati"},
	{ERR_TOO_MANY_ATTEMPTS, "Troppi tentativi falliti: connessione chiusa"},
	{ERR_UNAUTHORIZED, "Operazione riservata all'amministratore"},
	{ERR_NO_SUCH_STATION, "La postazione non esiste"},
	{ERR_INVALID_DATE, "Data non valida o gia' passata"},
	{ERR_INVALID_TIME, "Orario non valido (10:00-23:00, da 30 minuti a 4 ore, "
		"solo :00 e :30)"},
	{ERR_INVALID_STATUS, "Stato non valido"},
	{ERR_NOT_FOUND, "Prenotazione inesistente"},
	{ERR_CONFLICT, "Si sovrappone a una prenotazione gia' approvata"},
	{ERR_SERVER, "Errore interno del server, riprova piu' tardi"},
	{NULL, NULL}};
	char		code[MAX_LINE];
	const char	*extra;
	int			i;

	if (!r)
		return ;
	resp_code(r, code, sizeof(code));
	i = 0;
	while (msgs[i][0] && strcmp(msgs[i][0], code) != 0)
		i++;
	printf("%sErrore:%s ", col(C_RED), col(C_RESET));
	if (!msgs[i][0]) // codice che non conosco: stampo la risposta cosi' com'e'
	{
		printf("%s\n", r->first);
		return ;
	}
	printf("%s", msgs[i][1]);
	// "ERR CONFLICT 17": dopo il codice puo' esserci l'id della prenotazione
	extra = r->first + strlen(RESP_ERR) + 1 + strlen(code);
	if (strcmp(code, ERR_CONFLICT) == 0 && extra[0] == ' ')
		printf(" (prenotazione %s)", extra + 1);
	printf("\n");
}

// stampa come tabella le righe "BOOKING ..." di una risposta di MY o ALL.
void	print_bookings(const t_resp *r)
{
	static const char	*head[BOOKING_COLS] = {"ID", "Postazione", "Data",
		"Inizio", "Fine", "Utente", "Stato"};
	static const int	widths[BOOKING_COLS] = {4, 10, 10, 6, 5, 12, 8};
	char				*fields[BOOKING_FIELDS];
	char				copy[MAX_LINE];
	int					i;

	if (!r)
		return ;
	if (r->n == 0)
	{
		printf("Nessuna prenotazione trovata\n");
		return ;
	}
	print_hline(widths, BOOKING_COLS);
	print_cells(head, widths, BOOKING_COLS);
	print_hline(widths, BOOKING_COLS);
	i = 0;
	while (i < r->n)
	{
		// split_fields modifica la stringa (anche quando fallisce): lavoro su
		// una copia, cosi' una riga malformata si puo' stampare intera
		if (copy_str(copy, r->rows[i], sizeof(copy)) == 0
			&& split_fields(copy, ' ', fields, BOOKING_FIELDS) == BOOKING_FIELDS
			&& strcmp(fields[0], "BOOKING") == 0)
			print_cells((const char *const *)fields + 1, widths, BOOKING_COLS);
		else
			printf("%s\n", r->rows[i]);
		i++;
	}
	print_hline(widths, BOOKING_COLS);
}

#include "./client.h"
#include <ctype.h>

/*
** Lettura da tastiera.
**
** Tutte le funzioni ritornano:
**   RET_OK      valore letto
**   RET_CANCEL  riga vuota (l'utente ha premuto solo INVIO)
**   RET_EOF     CTRL-D
** Le ask_* ripetono la domanda finche' il valore non e' valido. I controlli
** servono solo a correggere subito gli errori di battitura: quelli che contano
** li rifa' il server.
*/

// --- funzioni di supporto ----------------------------------------------------

// 1 se str contiene uno spazio o un carattere di controllo (TAB, frecce...).
// Uno spazio dividerebbe il campo in due parametri del comando.
static int	has_bad_chars(const char *str)
{
	while (*str)
	{
		// il cast serve perche' con un char negativo (es. lettere accentate)
		// il comportamento di isspace e iscntrl non e' definito
		if (isspace((unsigned char)*str) || iscntrl((unsigned char)*str))
			return (1);
		str++;
	}
	return (0);
}

// --- funzioni pubbliche ------------------------------------------------------

// stampa prompt e legge una riga dentro buf, senza il '\n' finale.
// Se la riga ha piu' di size - 1 caratteri la tronca: il resto viene scartato.
int	read_line(const char *prompt, char *buf, size_t size)
{
	char	*newline;
	int		c;

	printf("%s", prompt);
	fflush(stdout); // senza '\n' printf non svuota il buffer
	if (fgets(buf, (int)size, stdin) == NULL)
	{
		buf[0] = '\0';
		printf("\n");
		return (RET_EOF);
	}
	newline = strchr(buf, '\n');
	if (newline)
		*newline = '\0';
	else
	{
		// fgets si e' fermata prima del '\n': il resto della riga e' ancora in
		// stdin e va consumato, altrimenti diventerebbe la riga successiva
		while ((c = getchar()) != '\n' && c != EOF)
			continue;
	}
	if (buf[0] == '\0')
		return (RET_CANCEL);
	return (RET_OK);
}

// chiede un campo obbligatorio senza spazi, al massimo size - 1 caratteri.
// Un campo troppo lungo viene rifiutato e non troncato: altrimenti si
// invierebbe un valore diverso da quello digitato. buf cambia solo con RET_OK.
int	ask_field(const char *label, char *buf, size_t size)
{
	char	prompt[MAX_LINE];
	char	line[MAX_LINE];
	int		ret;

	snprintf(prompt, sizeof(prompt), "%s: ", label);
	while (1)
	{
		ret = read_line(prompt, line, sizeof(line));
		if (ret != RET_OK)
			return (ret);
		if (has_bad_chars(line))
			printf("Il campo non puo' contenere spazi o caratteri speciali\n");
		else if (copy_str(buf, line, size) < 0)
			printf("Campo troppo lungo (massimo %zu caratteri)\n", size - 1);
		else
			return (RET_OK);
	}
}

// come ask_field, ma il campo si puo' lasciare vuoto: in quel caso ritorna
// RET_OK e buf e' la stringa vuota.
int	ask_optional(const char *label, char *buf, size_t size)
{
	int	ret;

	ret = ask_field(label, buf, size);
	if (ret == RET_CANCEL)
	{
		buf[0] = '\0';
		return (RET_OK);
	}
	return (ret);
}

// chiede una data GG-MM-AAAA. buf deve avere almeno DATE_LEN byte.
int	ask_date(const char *label, char *buf)
{
	char	line[MAX_LINE];
	t_date	date;
	int		ret;

	while (1)
	{
		ret = ask_field(label, line, sizeof(line));
		if (ret != RET_OK)
			return (ret);
		if (str_to_date(line, &date) == 0)
		{
			copy_str(buf, line, DATE_LEN);
			return (RET_OK);
		}
		printf("Data non valida (formato GG-MM-AAAA, es. 20-12-2026)\n");
	}
}

// chiede un'ora HH:MM. buf deve avere almeno TIME_LEN byte.
int	ask_time(const char *label, char *buf)
{
	char	line[MAX_LINE];
	t_time	time;
	int		ret;

	while (1)
	{
		ret = ask_field(label, line, sizeof(line));
		if (ret != RET_OK)
			return (ret);
		if (str_to_time(line, &time) == 0)
		{
			copy_str(buf, line, TIME_LEN);
			return (RET_OK);
		}
		printf("Ora non valida (formato HH:MM, es. 18:30)\n");
	}
}

// chiede un id (intero > 0) e lo scrive in *id.
int	ask_id(const char *label, int *id)
{
	char	line[MAX_LINE];
	int		n;
	int		ret;

	while (1)
	{
		ret = ask_field(label, line, sizeof(line));
		if (ret != RET_OK)
			return (ret);
		if (str_to_int(line, &n) == 0 && n > 0)
		{
			*id = n;
			return (RET_OK);
		}
		printf("ID non valido (numero intero maggiore di 0)\n");
	}
}

// chiede uno stato (maiuscole/minuscole indifferenti). In *status scrive un
// puntatore alla parola del protocollo, che e' una costante: niente free.
int	ask_status(const char *label, const char **status)
{
	char			line[MAX_LINE];
	enum e_status	st;
	int				ret;
	int				i;

	while (1)
	{
		ret = ask_field(label, line, sizeof(line));
		if (ret != RET_OK)
			return (ret);
		i = 0;
		while (line[i])
		{
			line[i] = (char)toupper((unsigned char)line[i]);
			i++;
		}
		if (str_to_status(line, &st) == 0)
		{
			*status = status_to_str(st);
			return (RET_OK);
		}
		printf("Stato non valido (PENDING, APPROVED o REJECTED)\n");
	}
}

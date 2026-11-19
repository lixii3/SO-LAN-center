#include "../headers/storage.h"

/*
** Funzioni generiche sui file CSV, usate sia da booking_op.c che da user_op.c.
**
** Ogni operazione segue lo schema:
**
**   lock -> storage_read_lines -> converte le righe in struct -> lavora
**        -> (se e' una modifica) storage_rewrite + storage_write_line -> unlock
*/

// --- lock --------------------------------------------------------------------

// blocca il file.
//type = F_RDLCK (lettura, condiviso: piu' lettori
// insieme) oppure F_WRLCK (scrittura, esclusivo: nessun altro, ne' lettori
// ne' scrittori). F_SETLKW = se il file e' gia' bloccato, ASPETTA.
// ritorna 0 se ok, -1 in caso di errore
int	lock_file(int fd, short type)
{
	struct flock	fl;

	memset(&fl, 0, sizeof(fl));
	fl.l_type = type;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0; // 0 = fino alla fine del file
	while (fcntl(fd, F_SETLKW, &fl) == -1)
	{
		if (errno != EINTR) // EINTR = attesa interrotta da un segnale: riprovo
			return (-1);
	}
	return (0);
}

void	unlock_file(int fd)
{
	struct flock	fl;

	memset(&fl, 0, sizeof(fl));
	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	fcntl(fd, F_SETLK, &fl);
}

// --- apertura ----------------------------------------------------------------

// apre (o crea, se non esiste) un file CSV. Se il file e' vuoto ci scrive
// l'intestazione header. Ritorna il file descriptor, -1 in caso di errore.
// Ogni processo figlio deve chiamarla per conto suo DOPO la fork, cosi' ha
// un fd tutto suo (con la propria posizione di lettura/scrittura).
int	storage_open(const char *path, const char *header)
{
	int			fd;
	struct stat	st;
	int			ret;

	if (!path || !header)
		return (-1);
	fd = open(path, O_RDWR | O_CREAT, 0644);
	if (fd == -1)
		return (-1);
	if (lock_file(fd, F_WRLCK) < 0)
	{
		close(fd);
		return (-1);
	}
	ret = 0;
	if (fstat(fd, &st) == -1)
		ret = -1;
	else if (st.st_size == 0)
		ret = storage_rewrite(fd, header); // file nuovo: solo l'intestazione
	unlock_file(fd);
	if (ret < 0)
	{
		close(fd);
		return (-1);
	}
	return (fd);
}

// --- lettura -----------------------------------------------------------------

// legge tutto il file e lo divide in righe, saltando l'intestazione (header)
// e le righe vuote, e togliendo l'eventuale '\r' finale (a capo di Windows).
// Riempie *out: alla fine va liberato con storage_free_lines.
// ritorna 0 se ok, -1 in caso di errore.
// DA CHIAMARE CON IL LOCK GIA' PRESO.
int	storage_read_lines(int fd, const char *header, t_lines *out)
{
	char	*line;
	char	*save;
	int		max_lines;
	size_t	len;
	long	i;

	if (!out || lseek(fd, 0, SEEK_SET) == -1)
		return (-1);
	out->buf = read_all(fd, NULL);
	if (!out->buf)
		return (-1);
	max_lines = 1; // l'ultima riga potrebbe non avere '\n'
	i = 0;
	while (out->buf[i])
		if (out->buf[i++] == '\n')
			max_lines++;
	out->lines = malloc(max_lines * sizeof(char *));
	if (!out->lines)
	{
		free(out->buf);
		return (-1);
	}
	out->count = 0;
	line = strtok_r(out->buf, "\n", &save); // spezza il testo riga per riga
	while (line)
	{
		len = strlen(line);
		if (len > 0 && line[len - 1] == '\r')
			line[len - 1] = '\0';
		if (line[0] != '\0' && (!header || strcmp(line, header) != 0))
			out->lines[out->count++] = line;
		line = strtok_r(NULL, "\n", &save); // prende la prossima riga
	}
	return (0);
}

void	storage_free_lines(t_lines *l)
{
	if (!l)
		return ;
	free(l->lines);
	free(l->buf);
	l->lines = NULL;
	l->buf = NULL;
	l->count = 0;
}

// --- scrittura ---------------------------------------------------------------

// svuota il file e ci riscrive solo l'intestazione.
// Ritorna 0 se ok, -1 in caso di errore.
// DA CHIAMARE CON IL LOCK IN SCRITTURA GIA' PRESO.
int	storage_rewrite(int fd, const char *header)
{
	if (ftruncate(fd, 0) == -1 || lseek(fd, 0, SEEK_SET) == -1)
		return (-1);
	return (storage_write_line(fd, header));
}

// scrive line + '\n' alla posizione corrente del file.
// ritorna 0 se ok, -1 in caso di errore
int	storage_write_line(int fd, const char *line)
{
	if (!line)
		return (-1);
	if (write_all(fd, line, strlen(line)) < 0 || write_all(fd, "\n", 1) < 0)
		return (-1);
	return (0);
}

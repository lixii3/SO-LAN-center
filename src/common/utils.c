#include "../headers/utils.h"

// scrive tutti i len byte di buf su fd: 
// ritorna 0 se ok, -1 in caso di errore
int	write_all(int fd, const char *buf, size_t len)
{
	size_t	sent;
	ssize_t	w;

	sent = 0;
	while (sent < len)
	{
		w = write(fd, buf + sent, len - sent);
		if (w < 0)
			return (-1);
		sent += (size_t)w;
	}
	return (0);
}

// legge tutto il contenuto di fd, dalla posizione corrente fino alla fine
// del file, e lo restituisce come stringa terminata da '\0' (da liberare con
// free). Se len non e' NULL ci scrive il numero di byte letti.
// ritorna NULL in caso di errore
char	*read_all(int fd, size_t *len)
{
	char	*buf;
	char	*tmp;
	size_t	size;
	size_t	cap;
	ssize_t	r;

	cap = 1024;
	size = 0;
	buf = malloc(cap);
	if (!buf)
		return (NULL);
	while (1)
	{
		if (size + 1 >= cap) // spazio esaurito (+1 per il '\0'): raddoppio
		{
			cap *= 2;
			tmp = realloc(buf, cap);
			if (!tmp)
			{
				free(buf);
				return (NULL);
			}
			buf = tmp;
		}
		r = read(fd, buf + size, cap - size - 1);
		if (r < 0)
		{
			free(buf);
			return (NULL);
		}
		if (r == 0) // fine del file
			break ;
		size += (size_t)r;
	}
	buf[size] = '\0';
	if (len)
		*len = size;
	return (buf);
}

// divide str in campi separati da sep, sostituendo ogni sep con '\0'
// (str viene modificata).
// Es: "a,b,,c" con ',' -> fields = {"a", "b", "", "c"}
// ritorna il numero di campi trovati, -1 se sono piu' di max_fields
int	split_fields(char *str, char sep, char **fields, int max_fields)
{
	int	count;

	if (!str || max_fields < 1)
		return (-1);
	fields[0] = str;
	count = 1;
	while (*str)
	{
		if (*str == sep)
		{
			if (count == max_fields)
				return (-1);
			*str = '\0';
			fields[count++] = str + 1;
		}
		str++;
	}
	return (count);
}

// converte una stringa composta SOLO da cifre (es. "17") in un int >= 0.
// stringhe alfanumeriche sono rifiutate.
// ritorna 0 se ok, altrimenti -1.
int	str_to_int(const char *str, int *out)
{
	long	n;

	if (!str || !out || *str == '\0')
		return (-1);
	n = 0;
	while (*str)
	{
		if (*str < '0' || *str > '9')
			return (-1);
		n = n * 10 + (*str - '0');
		if (n > INT_MAX)
			return (-1);
		str++;
	}
	*out = (int)n;
	return (0);
}

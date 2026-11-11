#include "../headers/net.h"

int	recv_line(int fd, char *line, size_t size)
{
	char	c;
	size_t	i;
	ssize_t	r;
	int		too_long;

	if (!line || size == 0)
		return (NET_ERROR);
	bzero(line, size);
	i = 0;
	too_long = 0;
	while (1)
	{
		r = read(fd, &c, 1);
		if (r < 0)
			return (NET_ERROR);
		if (r == 0)
			return (NET_CLOSED);
		if ( c == '\n')
			break;
		if (i < size - 1)
			line[i++] = c;
		else
			too_long = 1;
	}
	if (too_long)
		return (NET_TOO_LONG);
	return ((int)i);
}


static int	write_all(int fd, const char *buf, size_t len)
{
	size_t	sent;
	ssize_t	w;

	sent = 0;
	while (sent < len)
	{
		w = write(fd, buf + sent, len - sent);
		if (w < 0)
			return (NET_ERROR);
		sent += w;
	}
	return (sent);
}

int	send_line(int fd, const char *line)
{
	if (!line)
		return (NET_ERROR);
	if (write_all(fd, line, strlen(line)) < 0 || write_all(fd, "\n", 1) < 0)
		return (NET_ERROR);
	return (0);
}
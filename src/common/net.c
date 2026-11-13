#include "../headers/net.h"
#include "../headers/utils.h"

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


int	send_line(int fd, const char *line)
{
	if (!line)
		return (NET_ERROR);
	if (write_all(fd, line, strlen(line)) < 0 || write_all(fd, "\n", 1) < 0)
		return (NET_ERROR);
	return (0);
}
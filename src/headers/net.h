#ifndef NET_H
#define NET_H
#include "./protocol.h"

#define NET_CLOSED -1
#define NET_ERROR -2
#define NET_TOO_LONG -3

int	recv_line(int fd, char *line, size_t size);
int	send_line(int fd, const char *line);

#endif
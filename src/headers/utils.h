#ifndef UTILS_H
#define UTILS_H

// --- utils ---
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>
#define BANNER_PATH "src/data/banner.txt"
#define WELCOME_PATH "src/data/welcome.txt"

int		write_all(int fd, const char *buf, size_t len);
char	*read_all(int fd, size_t *len);
int		split_fields(char *str, char sep, char **fields, int max_fields);
int		str_to_int(const char *str, int *out);
int		copy_str(char *dst, const char *src, size_t size);
char	*strjoin(char const *s1, char const *s2);
void	print_banner(char *filepath);

#endif

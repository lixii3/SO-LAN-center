#include "../headers/storage.h"
#include <strings.h> // strcasecmp

// campi di una riga CSV di users
enum e_user_field
{
	USR_USERNAME,
	USR_PASSWD,
	USR_ROLE,
	USR_FIELDS // numero di campi (vale 3)
};

// --- funzioni di supporto ----------------------

// legge una riga "username,password,ruolo" e la scrive in *user.
// il ruolo nel file e' "admin" o "user" (maiuscole/minuscole indifferenti).
// ritorna 0 se ok, -1 se la riga e' malformata
static int	user_from_csv(const char *line, t_user *user)
{
	char	copy[MAX_LINE];
	char	*fields[USR_FIELDS];

	// split_fields modifica la stringa: lavoro su una copia
	if (copy_str(copy, line, sizeof(copy)) < 0)
		return (-1);
	if (split_fields(copy, ',', fields, USR_FIELDS) != USR_FIELDS)
		return (-1);
	if (copy_str(user->username, fields[USR_USERNAME], MAX_USERNAME) < 0
		|| copy_str(user->password, fields[USR_PASSWD], MAX_PASSWORD) < 0)
		return (-1);
	if (strcasecmp(fields[USR_ROLE], ROLE_ADMIN) == 0)
		user->is_admin = 1;
	else if (strcasecmp(fields[USR_ROLE], ROLE_USER) == 0)
		user->is_admin = 0;
	else
		return (-1);
	return (0);
}

// --- operazioni pubbliche ----------------------------------------------------

// cerca username nel file degli utenti e, se lo trova, lo copia in *out.
// ritorna 0 se trovato, STORAGE_NOT_FOUND oppure STORAGE_ERROR
int	storage_find_user(int fd, const char *username, t_user *out)
{
	t_lines	file;
	t_user	user;
	int		ret;
	int		i;

	if (!username || !out)
		return (STORAGE_ERROR);
	if (lock_file(fd, F_RDLCK) < 0)
		return (STORAGE_ERROR);
	if (storage_read_lines(fd, HEADER_USERS, &file) < 0)
	{
		unlock_file(fd);
		return (STORAGE_ERROR);
	}
	ret = STORAGE_NOT_FOUND;
	i = 0;
	while (i < file.count && ret == STORAGE_NOT_FOUND)
	{
		if (user_from_csv(file.lines[i], &user) < 0)
		{
			fprintf(stderr, "users: riga malformata: \"%s\"\n", file.lines[i]);
			ret = STORAGE_ERROR;
		}
		else if (strcmp(user.username, username) == 0)
		{
			*out = user;
			ret = 0;
		}
		i++;
	}
	storage_free_lines(&file);
	unlock_file(fd);
	return (ret);
}

// controlla username e password. Se sono giusti e is_admin non e' NULL,
// ci scrive 1 se l'utente e' admin, 0 altrimenti.
// ritorna 0 se ok, STORAGE_NOT_FOUND (utente inesistente o password
// sbagliata: al client non si dice quale dei due) oppure STORAGE_ERROR
int	storage_check_credentials(int fd, const char *username,
	const char *pass, int *is_admin)
{
	t_user	user;
	int		ret;

	if (!pass)
		return (STORAGE_ERROR);
	ret = storage_find_user(fd, username, &user);
	if (ret < 0)
		return (ret);
	if (strcmp(user.password, pass) != 0)
		return (STORAGE_NOT_FOUND);
	if (is_admin)
		*is_admin = user.is_admin;
	return (0);
}

void	clear_user(t_user *user)
{
	if (!user)
		return;
	bzero(user->password, MAX_PASSWORD);
	bzero(user->username, MAX_USERNAME);
	user->is_admin = 0;
}
#ifndef SERVER_H
#define SERVER_H

#include "../headers/net.h"
#include "../headers/storage.h"
#include "../headers/utils.h"
#include <signal.h>
#include <sys/wait.h>

#define MAX_ARGS		8	// massimo numero di parole in un comando
#define MAX_LOGIN_TRIES	3	// al 3o login fallito si chiude la connessione

// struttura che contiene i dati di una sessione (= una connessione di un client)

typedef	struct s_server
{
	int					socket;
	struct sockaddr_in	address;
} t_server;


typedef struct s_session
{
	int			booking_fd;
	int			users_fd;
	t_user		user;
	const char	*role; // NULL = non loggato, altrimenti ROLE_USER o ROLE_ADMIN
	int			failed_logins;
}	t_session;

// --- session.c

t_server	init_server(unsigned short port, const char *address, int max_conn);
t_session	init_session(int bfd, int ufd);
const char	*set_role(t_session *session, char *role, int ufd, char	*username);
const char	*session(int connectSocket, struct sockaddr_in clientAddress);
void		clear_session(t_session *session);

// --- validation.c: validazione dei comandi ---

int			split_cmd(char *line, char **argv);
const char	*check_cmd(char **argv, int argc, const t_session *s);

// --- handler.c

char	*handle_cmd(char **cmd, int argc, t_session *session);
#endif

#ifndef CLIENT_H
#define CLIENT_H

#include "../headers/net.h"
#include "../headers/structs.h"
#include "../headers/utils.h"

// chi puo' vedere una voce: bit, cosi' si combinano con |
#define R_GUEST	1
#define R_USER	2
#define R_ADMIN	4

// ritorno di input e azioni (prefisso diverso da CMD_, gia' usato dal protocollo)
#define RET_OK		0
#define RET_CANCEL	1	// campo vuoto: torna al menu
#define RET_EOF		2	// CTRL-D
#define RET_LOST	3	// connessione persa
#define RET_END		4	// fine sessione (QUIT, TOO_MANY_ATTEMPTS)

typedef struct s_client
{
	int		fd;
	int		role;					// R_GUEST, R_USER o R_ADMIN
	char	username[MAX_USERNAME];
}	t_client;

typedef struct s_resp
{
	int		ok;						// 1 = OK, 0 = ERR
	char	first[MAX_LINE];		// prima riga
	char	**rows;					// righe dell'elenco, NULL se non c'e'
	int		n;						// righe ricevute
}	t_resp;

typedef struct s_item
{
	const char	*label;
	int			roles;
	int			(*action)(t_client *);
}	t_item;

// --- request.c: comunicazione con il server ---

int		connect_server(const char *serverIP);
int		request(t_client *c, const char *cmd, int is_list, t_resp *out);
void	free_resp(t_resp *r);
void	resp_code(const t_resp *r, char *buf, size_t size);

// --- input.c: lettura da tastiera ---

int		read_line(const char *prompt, char *buf, size_t size);
int		ask_field(const char *label, char *buf, size_t size);
int		ask_optional(const char *label, char *buf, size_t size);
int		ask_date(const char *label, char *buf);
int		ask_time(const char *label, char *buf);
int		ask_id(const char *label, int *id);
int		ask_status(const char *label, const char **status);

#endif

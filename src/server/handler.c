#include "./server.h"

const char	*exec_login(char **argv, t_session *session);
const char	*exec_logout(char **argv, t_session *session);
const char	*exec_book(char **argv, t_session *session);
const char	*exec_stations(char **argv, t_session *session);
const char	*exec_quit(char **argv, t_session *session);
const char	*exec_my(char **argv, t_session *session);
const char	*exec_all(char **argv, t_session *session);
const char	*exec_approve(char **argv, t_session *session);
const char	*exec_reject(char **argv, t_session *session);
const char	*exec_set(char **argv, t_session *session);
const char	*exec_conflicts(char **argv, t_session *session);

// Prende la riga di comando già divisa in argc argomenti e lo esegue ritornando
// una stringa di risposta RESP_.
// Oppure lo rigetta con un codice di errore ERR_.
const char	*handle_cmd(char **argv, int argc, t_session *session)
{
	const char	*returnCode;
	char		resp[50];

	if ((returnCode = check_cmd(argv, argc, session)))
	{
		snprintf(resp, 50, "%s%s", RESP_ERR " ", returnCode);
		return (strdup(resp));
	}
	// TODO: compara cmd con tutti i comandi ed esegui il corrispettivo exec_
	return (strdup(resp));
}



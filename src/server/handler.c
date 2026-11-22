#include "./server.h"

char	*exec_login(char **argv, t_session *session); 
char	*exec_logout(char **argv, t_session *session);
char	*exec_book(char **argv, t_session *session);
char	*exec_stations(char **argv, t_session *session);
char	*exec_quit(char **argv, t_session *session);
char	*exec_my(char **argv, t_session *session);
char	*exec_all(char **argv, t_session *session);
char	*exec_approve(char **argv, t_session *session);
char	*exec_reject(char **argv, t_session *session);
char	*exec_set(char **argv, t_session *session);
char	*exec_conflicts(char **argv, t_session *session);

// Prende la riga di comando già divisa in argc argomenti e lo esegue ritornando
// una stringa di risposta RESP_.
// Oppure lo rigetta con un codice di errore ERR_.
char	*handle_cmd(char **argv, int argc, t_session *session)
{
	const char	*returnCode;
	char		resp[MAX_LINE];
	char		*cmd;

	if ((returnCode = check_cmd(argv, argc, session)))
	{
		snprintf(resp, sizeof(resp), "%s%s", RESP_ERR " ", returnCode);
		return (strdup(resp));
	}
	cmd = argv[0];
	// le stringhe si confrontano con strcmp: "==" confronta gli indirizzi
	if (strcmp(cmd, CMD_LOGIN) == 0)
	{
		if (session->failed_logins >= 3)
			return (strdup(RESP_ERR " " ERR_TOO_MANY_ATTEMPTS));
		return (exec_login(argv, session));
	}
	// TODO: compara cmd con tutti i comandi ed esegui il corrispettivo exec_
	return (strdup("TESTING"));
}

// TODO: da implementare 
char	*exec_login(char **argv, t_session *session)
{
	(void)argv;
	(void)session;
	return (strdup(RESP_ERR " " ERR_BAD_CREDENTIALS));
}



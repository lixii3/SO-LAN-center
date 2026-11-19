#include "../headers/net.h"
#include "../headers/utils.h"
#include "../headers/storage.h"
#include "./server.h"

int main(void) {
	signal(SIGPIPE, SIG_IGN);
	t_server			server_info;
	int					connectSocket;
	socklen_t			clientAddressLen;
	struct sockaddr_in	clientAddress;
	int					pid;

	server_info = init_server(SERVER_PORT, SERVER_ADDRESS);
	printf("Server ready (CTRL-C per terminare)\n");
	clientAddressLen = sizeof(clientAddress);
	// ciclo infinito in attesa di connessione
	while (1) {
		// accetta le connessioni 
		if ((connectSocket = accept(server_info.socket,
			(struct sockaddr *)&clientAddress, &clientAddressLen)) == -1) {
			perror("Error in accept()");
			close(server_info.socket);
			exit(-1);
		}
		pid = fork();
		if (pid == 0)
		{
			session(connectSocket, clientAddress);
			exit(0);
		}
		else
			close(connectSocket);
	}
}

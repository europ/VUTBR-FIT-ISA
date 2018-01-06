/*
 * IPK.2015L
 *
 * Demonstration of trivial TCP communication.
 *
 * Ondrej Rysavy (rysavy@fit.vutbr.cz)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main (int argc, const char * argv[]) {
	int rc;
	int welcome_socket;
	struct sockaddr_in6 sa;
	struct sockaddr_in6 sa_client;
	char str[INET6_ADDRSTRLEN];
    int port_number;

    if (argc != 2) {
       fprintf(stderr,"usage: %s <port>\n", argv[0]);
       exit(EXIT_FAILURE);
    }
    port_number = atoi(argv[1]);


	socklen_t sa_client_len=sizeof(sa_client);
	if ((welcome_socket = socket(PF_INET6, SOCK_STREAM, 0)) < 0)
	{
		perror("ERROR: socket");
		exit(EXIT_FAILURE);
	}

	memset(&sa,0,sizeof(sa));
	sa.sin6_family = AF_INET6;
	sa.sin6_addr = in6addr_any;
	sa.sin6_port = htons(port_number);



	if ((rc = bind(welcome_socket, (struct sockaddr*)&sa, sizeof(sa))) < 0)
	{
		perror("ERROR: bind");
		exit(EXIT_FAILURE);
	}
	if ((listen(welcome_socket, 1)) < 0)
	{
		perror("ERROR: listen");
		exit(EXIT_FAILURE);
	}
	while(1)
	{
		int comm_socket = accept(welcome_socket, (struct sockaddr*)&sa_client, &sa_client_len);
		if (comm_socket > 0)
		{
			if(inet_ntop(AF_INET6, &sa_client.sin6_addr, str, sizeof(str))) {
				printf("INFO: New connection:\n");
				printf("INFO: Client address is %s\n", str);
				printf("INFO: Client port is %d\n", ntohs(sa_client.sin6_port));
			}

			char buff[1024];
			int res = 0;
			for (;;)
			{
				res = recv(comm_socket, buff, 1024,0);
                if (res <= 0)
                    break;

			    send(comm_socket, buff, strlen(buff), 0);
			}
		}
		else
		{
			printf(".");
		}
		printf("Connection to %s closed\n",str);
		close(comm_socket);
	}
}

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#define MAXLEN 100

int main(int argc, char const* argv[]) {
	const char *host = "192.168.0.194";
	const char *port = "3490";	
	int client_sock;
	int in;
	char buffer[MAXLEN];
	struct addrinfo hints, *res, *rec;
	memset(&hints, 0, sizeof(struct addrinfo));
	
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if ((getaddrinfo(host, port, &hints, &res)) != 0) {
		perror("Failed to translate client socket");
		exit(EXIT_FAILURE);
	}

	printf("Client socket translated\n");

	for (rec = res; rec != NULL; rec = rec->ai_next) {
		client_sock = socket(rec->ai_family, rec->ai_socktype, rec->ai_protocol);
		if (client_sock == -1) continue;
		if (connect(client_sock, rec->ai_addr, rec->ai_addrlen) != -1) break;
		close(client_sock);
	}
	
	if (rec == NULL) {
		perror("Failed to create or connect client socket");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);

	if(recv(client_sock, buffer, MAXLEN-1, 0) == -1) {
		perror("Failed to accept client socket.");
		exit(EXIT_FAILURE);
	}
	
	printf("Received: '%s'", buffer);
	
	close(client_sock);

	return 0;
}

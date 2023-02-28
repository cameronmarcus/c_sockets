#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10
#define MAXLEN 65536

// AF_INET for sockaddr_in struct
// PF_INET for call to socket()

int main(int argc, char const* argv[]) {
	int s;
	const char *host = "10.0.2.15";
	const char *port = "3490";
	struct addrinfo hints, *res, *rec;
	struct sockaddr address;
	struct sockaddr_in src, dst;
	int addrlen = sizeof(address);
	int data_size;
	int server_sock;
	int opt = 1;
	char *buffer = (unsigned char *)malloc(MAXLEN);
	char *msg = "This wont work :(";

	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_RAW;

	if((getaddrinfo(host, port, &hints, &res)) != 0) {
		perror("Failed to translate server sock");
		exit(EXIT_FAILURE);
	}

	printf("Server socket translated.\n");

	for (rec = res; rec != NULL; rec = rec->ai_next){
		server_sock = socket(rec->ai_family, rec->ai_socktype, rec->ai_protocol);
		if (server_sock == -1) continue;
		//int enable = 1;
		//setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
		//if(bind(server_sock, rec->ai_addr, rec->ai_addrlen) == 0) break;
		//printf("Not supposed to get here\n");
		//close(server_sock);
	}
	
	data_size = recvfrom(server_sock, buffer, MAXLEN, 0, &address, &addrlen);

	if (data_size < 0) {
		perror("Failed to get packets");
		exit(EXIT_FAILURE);
	}

	//Parse Packet and check for SYN flag
	
	struct iphdr *iph = (struct iphdr *)buffer;
	src.sin_addr.s_addr = iph->saddr;
	iphdrlen = iph->ihl * 4;

	struct tcphdr *tcph = (struct tcphdr*)(buffer + iphdrlen);

	if(tcph->syn) {
		char addr, port;
		addr = inet_ntoa(src.sin_addr);
		port = inet_ntoa(tcph->source);
		deny_connection(server_sock, addr, port);
	}

	close(server_sock);

	return 0;


	void deny_connection(int sockfd, char src_addr, char src_port) {
		
	}


	/*
	
	OLD SERVER SOCK->

	if (rec == NULL) {
		perror("Failed to create or connect client socket");
		exit(EXIT_FAILURE);
	}
	
	if (listen(server_sock, BACKLOG) == -1) {
		perror("Failed to start server socket listen");
		exit(EXIT_FAILURE);
	}
	
	int client_sock;
	struct sockaddr clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	if((client_sock = accept(server_sock, &clientAddr, &clientAddrLen)) < 0) {
		perror("Failed to accept client socket");
		exit(EXIT_FAILURE);
	}
	
	printf("client socket accepted\n");

	send(client_sock, msg, strlen(msg) , 0);
	printf("Sent Message: '%s'", msg);
	
	close(client_sock);
	
	*/

	shutdown(server_sock, SHUT_RDWR);

	return 0;
}

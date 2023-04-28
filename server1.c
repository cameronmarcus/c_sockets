#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAXLEN 100
#define BACKLOG 5

int main(int argc, char const* argv[]) {
    const char *host = "192.168.0.184";
    const char *port = "3490";
    int sockfd, newfd;
    int in;
    int yes = 1;
    char buffer[MAXLEN];
    char s[INET_ADDRSTRLEN];
    struct addrinfo hints, *res, *rec;
    struct sockaddr_storage clientaddr;
    socklen_t sin_size;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if ((getaddrinfo(host, port, &hints, &res)) != 0) {
        perror("Failed to translate client socket");
        exit(EXIT_FAILURE);
    }

    printf("Client socket translated\n");

    for (rec = res; rec != NULL; rec = rec->ai_next) {
        //Create Socket File Descriptor
        if ((sockfd = socket(rec->ai_family, rec->ai_socktype, rec->ai_protocol)) == -1) {
            perror("Failed to create or connect server socket");
            exit(EXIT_FAILURE);
        }

        //Make socket reuse port if already in use
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("setsockopt error");
            exit(EXIT_FAILURE);
        }

        //make bind connection
        if (bind(sockfd, rec->ai_addr, rec->ai_addrlen) == -1) {
            perror("Failed to bind socket");
            exit(EXIT_FAILURE);
        }

        break;

    }

    printf("Socket bound to %d:%d", *host, *port);

    freeaddrinfo(res);

    if (rec == NULL) {
        perror("Failed to create or connect client socket");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("Failed to listen on socket");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for connections!");
    while(1){
        sin_size = sizeof(clientaddr);
        newfd = accept(sockfd, (struct sockaddr *)&clientaddr, &sin_size);
        if(newfd == -1){
            perror("Could not creat client socket file descriptor");
            exit(EXIT_FAILURE);
            continue;
        }
         //inet_ntop(clientaddr.ss_family, get_in_addr((struct sockaddr *)&clientaddr), s, sizeof s);
         if(send(newfd, "HIIII", 13, 0) == -1){
             perror("Could not send to client");
             exit(EXIT_FAILURE);
         }
    }
    return 0;
}

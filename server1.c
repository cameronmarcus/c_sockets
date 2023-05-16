#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAXLEN 1024
#define BACKLOG 5

#include <stdio.h>
#include <stdlib.h>

char *get_filename(int selection);

char *read_file(const char *filename);

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

    printf("Server socket translated\n");

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

    printf("Socket bound to %d:%d\n", *host, *port);

    freeaddrinfo(res);

    if (rec == NULL) {
        perror("Failed to create or connect client socket");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("Failed to listen on socket");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for connections!\n");


    while(1){
        sin_size = sizeof(clientaddr);
        newfd = accept(sockfd, (struct sockaddr *)&clientaddr, &sin_size);
        if(newfd == -1){
            perror("Could not create client socket file descriptor");
            exit(EXIT_FAILURE);
            continue;
        }
        char *msg = "Welcome to my library! \n"
                    "Please select and option:\n"
                    "1). Alice's Adventures in Wonderland\n"
                    "2). Moby Dick\n"
                    "3). Romeo and Juliet\n"
                    "4). The Great Gatsby\n"
                    "5). Pride and Prejudice\n";


        if(send(newfd, msg, 1024, 0) == -1){
            perror("Could not send to client");
            exit(EXIT_FAILURE);
        }

        printf("Sent Books\n");

        char buffer[10];
        recv(newfd, buffer, 1000, 0);

        printf( "received selection: %s", buffer);

        char *filename = get_filename(atoi(buffer));

        printf("Filename: %s\n", filename);

        char *file = read_file(filename);

        if(send(newfd, filename, 1024, 0) == -1){
            perror("Could not send to client");
            exit(EXIT_FAILURE);
        }


    }
    return 0;
}


char *read_file(const char *filename) {
    // open the file for reading
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Could not open file\n");
        return NULL;
    }

    // get the size of the file
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // allocate a buffer to hold the file contents
    char *buffer = (char *)malloc(file_size);
    if (buffer == NULL) {
        printf("Memory allocation error\n");
        fclose(fp);
        return NULL;
    }

    // read the file contents into the buffer
    fread(buffer, file_size, 1, fp);

    // close the file
    fclose(fp);

    // copy the buffer to a string variable
    char *string = (char *)malloc(file_size + 1);
    if (string == NULL) {
        printf("Memory allocation error\n");
        free(buffer);
        return NULL;
    }
    memcpy(string, buffer, file_size);
    string[file_size] = '\0';

    // free the buffer
    free(buffer);

    // return the string
    return string;
}


char *get_filename(int selection) {
    char *filename;
    switch (selection) {
        case 1:
            filename = "Alice's Adventures in Wonderland.txt";
            break;
        case 2:
            filename = "Moby Dick.txt";
            break;
        case 3:
            filename = "Romeo and Juliet.txt";
            break;
        case 4:
            filename = "The Great Gatsby.txt";
            break;
        case 5:
            filename = "Pride and Prejudice.txt";
            break;
    }
    return filename;
}
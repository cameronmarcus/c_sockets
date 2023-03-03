#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include<netinet/tcp.h>
#include<netinet/ip.h>
#include <net/if.h>

#include <net/ethernet.h>
#define MAXLEN 2048

int main() {
    setbuf(stdout, NULL);
	struct sockaddr_in address;
	int server_sock;
	char *buffer = (unsigned char *)malloc(MAXLEN);

    server_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (server_sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct ifreq ifr;
    strcpy(ifr.ifr_name, "eth0");

    // bind socket to interface
    if (setsockopt(server_sock, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    int numbytes;
    struct sockaddr_in sender_addr;
    struct in_addr tmp_addr;
    unsigned short tmp_port
    socklen_t addrlen = sizeof(sender_addr);

    while(1) {
        numbytes = recvfrom(server_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &sender_addr, &addrlen);
        if (numbytes == -1) {
            perror("recvfrom");
        }

        printf("Recieved %i Bytes of data\n", numbytes);

        struct iphdr *iph = (struct iphdr *) buffer;
        struct tcphdr *tcph = (struct tcphdr *) (buffer + (iph->ihl * 4));
        if (tcph->syn) { // check if SYN flag is set
            printf("Connection Attempted\n");
            // swap source and destination addresses and ports
            tmp_addr = sender_addr.sin_addr;
            sender_addr.sin_addr = address.sin_addr;
            address.sin_addr = tmp_addr;
            tmp_port = sender_addr.sin_port;
            sender_addr.sin_port = address.sin_port;
            address.sin_port = tmp_port;
            // set RST flag and send packet back
            tcph->rst = 1;
            tcph->syn = 0;
            tcph->ack = 0;
            sendto(server_sock, buffer, numbytes, 0, (struct sockaddr *) &sender_addr, sizeof(sender_addr));

        } else {
            printf("No Connection Attempted\n");
        }
    }
    return 0;

}
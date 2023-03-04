
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <net/ethernet.h>
#define MAXLEN 2048

struct pseudo_header {
    uint32_t src_addr;
    uint32_t dest_addr;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t len;
};

unsigned short calculate_tcp_checksum(struct iphdr* iph, struct tcphdr* tcph, char* data, int data_len) {
    char buf[sizeof(struct pseudo_header) + sizeof(struct tcphdr) + data_len];
    struct pseudo_header* pHeader = (struct pseudo_header*)buf;
    struct tcphdr* header = (struct tcphdr*)(buf + sizeof(struct pseudo_header));
    char* payload = buf + sizeof(struct pseudo_header) + sizeof(struct tcphdr);

    // Fill in pseudo-header
    pHeader->src_addr = iph->saddr;
    pHeader->dest_addr = iph->daddr;
    pHeader->reserved = 0;
    pHeader->protocol = IPPROTO_TCP;
    pHeader->len = htons(sizeof(struct tcphdr) + data_len);

    // Fill in TCP header and payload
    memcpy(header, tcph, sizeof(struct tcphdr));
    memcpy(payload, data, data_len);

    // Calculate checksum over pseudo-header, TCP header, and payload
    int total_len = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + data_len;
    unsigned short* buf16 = (unsigned short*)buf;
    unsigned long sum = 0;
    for (int i = 0; i < total_len / 2; i++) {
        sum += buf16[i];
    }
    if (total_len % 2) {
        sum += buf[total_len - 1];
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return ~sum;
}

int main() {
    setbuf(stdout, NULL);
	struct sockaddr_in address;
	int server_sock;
    char buffer[2049];
    memset (buffer, 0, 4096);

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
    unsigned short tmp_port;
    socklen_t addrlen = sizeof(sender_addr);

    while(1) {
        numbytes = recvfrom(server_sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &sender_addr, &addrlen);
        if (numbytes == -1) {
            perror("recvfrom");
        }

        //printf("Recieved %i Bytes of data\n", numbytes);

        struct iphdr *iph = (struct iphdr *) buffer;
        struct tcphdr *tcph = (struct tcphdr *) (buffer + (iph->ihl * 4));
        unsigned short checksum;
        if ((unsigned int)tcph->syn) { // check if SYN flag is set
            printf("Connection Attempted from %s \n", inet_ntoa(sender_addr.sin_addr));
            tmp_addr = sender_addr.sin_addr;
            sender_addr.sin_addr = address.sin_addr;
            address.sin_addr = tmp_addr;
            tmp_port = sender_addr.sin_port;
            sender_addr.sin_port = address.sin_port;
            address.sin_port = tmp_port;
            tcph->rst = 1;
            tcph->syn = 0;
            tcph->ack = 0;
            checksum = calculate_tcp_checksum(iph, tcph, buffer, sizeof(buffer));
            tcph->check = checksum;
            printf("Connection Denied\n");
            sendto(server_sock, buffer, numbytes, 0, (struct sockaddr *) &address, sizeof(address));

        } else {
            //printf("No Connection Attempted\n");
        }
    }
    return 0;

}

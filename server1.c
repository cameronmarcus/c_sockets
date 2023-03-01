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
#include<netinet/tcp.h>
#include<netinet/ip.h>

#define BACKLOG 10
#define MAXLEN 65536

// AF_INET for sockaddr_in struct
// PF_INET for call to socket()

void deny_connection(int sockfd, char src_addr, unsigned int src_port);

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

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_RAW;

    int rv;
	if((rv = getaddrinfo(host, NULL, &hints, &res)) != 0) {
		perror("Failed to translate server sock");
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
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

    while(1) {
        data_size = recvfrom(server_sock, buffer, MAXLEN, 0, &address, &addrlen);

        if (data_size < 0) {
            perror("Failed to get packets");
            exit(EXIT_FAILURE);
        }

        //Parse Packet and check for SYN flag

        unsigned short iphdrlen;
        struct sockaddr_in source;
        struct iphdr *iph = (struct iphdr *) buffer;
        iphdrlen = iph->ihl * 4;

        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = iph->saddr;

        struct tcphdr *tcph = (struct tcphdr *) (buffer + iphdrlen);

        if ((unsigned int) tcph->syn) {
            deny_connection(server_sock, *inet_ntoa(source.sin_addr), ntohs(tcph->source));
        }
    }

	close(server_sock);
	shutdown(server_sock, SHUT_RDWR);

	return 0;
}

unsigned short csum(unsigned short *ptr,int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }

    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;

    return(answer);
}


void deny_connection(int sockfd, char src_addr, unsigned int src_port) {
    struct pseudo_header
    {
        u_int32_t source_address;
        u_int32_t dest_address;
        u_int8_t placeholder;
        u_int8_t protocol;
        u_int16_t tcp_length;
    };

    char datagram[4096] , source_ip[32] , *data , *pseudogram;

    //zero out the packet buffer
    memset (datagram, 0, 4096);

    //IP header
    struct iphdr *iph = (struct iphdr *) datagram;

    //TCP header
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
    struct sockaddr_in sin;
    struct pseudo_header psh;

    //some address resolution
    strcpy(source_ip , &src_addr);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80);
    sin.sin_addr.s_addr = inet_addr ("1.2.3.4");

    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct iphdr) + sizeof (struct tcphdr) + strlen(data);
    iph->id = htonl (54321);	//Id of this packet
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;		//Set to 0 before calculating checksum
    iph->saddr = inet_addr ( source_ip );	//Spoof the source ip address
    iph->daddr = sin.sin_addr.s_addr;

    //Ip checksum
    iph->check = csum ((unsigned short *) datagram, iph->tot_len);

    //TCP Header
    tcph->source = htons (src_port);
    tcph->dest = htons (src_port);
    tcph->seq = 0;
    tcph->ack_seq = 0;
    tcph->doff = 5;	//tcp header size
    tcph->fin=0;
    tcph->syn=0;
    tcph->rst=1;
    tcph->psh=0;
    tcph->ack=0;
    tcph->urg=0;
    tcph->window = htons (5840);	/* maximum allowed window size */
    tcph->check = 0;	//leave checksum 0 now, filled later by pseudo header
    tcph->urg_ptr = 0;

    //Now the TCP checksum
    psh.source_address = inet_addr( source_ip );
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + strlen(data) );

    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + strlen(data);
    pseudogram = malloc(psize);

    memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header) , tcph , sizeof(struct tcphdr) + strlen(data));

    tcph->check = csum( (unsigned short*) pseudogram , psize);

    //IP_HDRINCL to tell the kernel that headers are included in the packet
    int one = 1;
    const int *val = &one;

    if (setsockopt (sockfd, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
    {
        perror("Error setting IP_HDRINCL");
        exit(0);
    }

    if (sendto (sockfd, datagram, iph->tot_len ,	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
    {
        perror("sendto failed");
    }
        //Data send successfully
    else
    {
        printf ("Packet Send. Length : %d \n" , iph->tot_len);
    }
}
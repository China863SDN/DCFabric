#include "net.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>


int send_msg(int sock, const void *buf, int len, char* ip, int port) {
	int ret = 0, tmp = 0;
	struct sockaddr_in6 hostaddr;
	memset(&hostaddr, 0, sizeof(hostaddr));
	hostaddr.sin6_family = AF_INET6;  
	hostaddr.sin6_port = htons(port);  
	inet_pton(AF_INET6, ip, (void*)&hostaddr.sin6_addr);
	socklen_t addrlen = sizeof(hostaddr);
	
    while(len > ret) {
        tmp = sendto(sock, buf+ret, len-ret, 0, (struct sockaddr *)&hostaddr, addrlen);
        if(-1 == tmp) {
            printf("error occurs in Send(), errno = %d, %s\n", errno, strerror(errno));
            return -1;
        }
        ret += tmp;
    }
    return ret;
}

int recv_msg(int sock, void *buf, int len) {
	int ret = 0, tmp;
    while(len != ret) {
        tmp = recvfrom(sock, buf+ret, len-ret, 0, 0, 0);
        if(0 > tmp && errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN) {
            printf("error occurs in Recv(), errno = %d, %s\n", errno, strerror(errno));
            return -1;
        } else if(0 == tmp) {
            return 0;
        } else if(0 > tmp && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)) {
		continue;
	}
        ret += tmp;
    }
    return ret;
}

int get_server_socket(int port) {
	int sock;
	struct sockaddr_in6 hostaddr;
	memset(&hostaddr, 0, sizeof(hostaddr));
	hostaddr.sin6_family = AF_INET6;  
	hostaddr.sin6_port = htons(port);  
	hostaddr.sin6_addr = in6addr_any;  
	
	sock = socket(AF_INET6, SOCK_DGRAM, 0); 
	if(-1 == sock) {
		printf("error occurs in socket()\n");
		return -1;
	}
	
	if(bind(sock, (struct sockaddr*)&hostaddr, sizeof(hostaddr)) == -1) {
		printf("error occurs in bind()\n");
		close(sock);
		return -1;
	}
	
	return sock;
}

int get_client_socket() {
	int sock;

	sock = socket(AF_INET6, SOCK_DGRAM, 0);
	if(-1 == sock) {
		printf("error occurs in socket()\n");
		return -1;
	}
	
	return sock;
}
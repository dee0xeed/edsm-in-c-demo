
#ifndef TCP_SOCKETS_H
#define TCP_SOCKETS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <poll.h>
#include <fcntl.h>

//#include "dots-types.h"

#define IP_STR_LEN 32

/* for 'server' */
int new_l_sock(struct in_addr *ip, unsigned short port, int *l_sock);
int new_d_sock(int l_sock, int *d_sock, char *ip_str);

/* for 'client' */
int new_c_sock(char *host, char *service, struct addrinfo **ai);
int c_sock_start_connect(int sock, struct addrinfo *ai);
int c_sock_check_connect(int sock, struct addrinfo *ai);
int c_sock_check_state(int sock);
/* see http://linuxgazette.tuwien.ac.at/136/pfeiffer.html */

#endif

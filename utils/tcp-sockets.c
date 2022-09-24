
#include <sys/ioctl.h>
#include "tcp-sockets.h"
#include "logging.h"

int new_l_sock(struct in_addr *ip, unsigned short port, int *l_sock)
{
	   int sk, err;
	struct sockaddr_in our_addr = {0};
	   int on = 1;

	sk = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == sk) {
		log_sys_msg("%s() - socket(): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	err = setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	if (-1 == err) {
		log_sys_msg("%s() - setsockopt(SO_REUSEADDR): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	our_addr.sin_family = AF_INET;
	if (ip) {
		if (0 == ip->s_addr)
			our_addr.sin_addr.s_addr = INADDR_ANY;
		else
			our_addr.sin_addr.s_addr = ip->s_addr;
	}
	our_addr.sin_port = htons(port);

	err = bind(sk, (struct sockaddr*)&our_addr, sizeof(our_addr));
	if (-1 == err) {
		log_sys_msg("%s() - bind(): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	err = listen(sk, 128);
	if (-1 == err) {
		log_sys_msg("%s() - listen(): %s\n", __func__, strerror(errno));
		goto __failure;
	}

	/* success */
	*l_sock = sk;
	return 0;

      __failure:
	if (sk != -1)
		close(sk);
	return 1;
}

int new_d_sock(int l_sock, int *d_sock, char *ip_str)
{
	   struct sockaddr_in peer;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	      int sk;

	sk = accept(l_sock, (struct sockaddr*)&peer, &addrlen);
	if (-1 == sk) {
		log_sys_msg("%s() - accept(): %s\n", __func__, strerror(errno));
		return 1;
	}

	*d_sock = sk;
	if (ip_str)
		inet_ntop(AF_INET, &peer.sin_addr, ip_str, IP_STR_LEN);
	return 0;
}

int new_c_sock(char *host, char *service, struct addrinfo **addrinfo)
{
	struct addrinfo hints;
	struct addrinfo *ai = NULL;
	   int sock = -1;
	   int r;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	*addrinfo = NULL;
	r = getaddrinfo(host, service, &hints, &ai);
	if (r) {
		log_sys_msg("%s() - getaddrinfo('%s:%s'): %s\n", __func__, host, service, gai_strerror(r));
		return sock;
	}

	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (-1 == sock) {
		log_sys_msg("%s() - socket(): %s\n", __func__, strerror(errno));
		return sock;
	};

	// TODO: add bind to interface

	*addrinfo = ai;
	return sock;
}

int c_sock_start_connect(int sock, struct addrinfo *ai)
{
	int flags, r;

	// REVISIT: this is already done by event capture engine...
	// not always (?)
	// upon attach == EPOLL_CTL_ADD
	flags = fcntl(sock, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);

	r = connect(sock, ai->ai_addr, ai->ai_addrlen);
	if (-1 == r) {
		if (errno != EINPROGRESS)
			log_sys_msg("%s() - connect(): %s\n", __func__, strerror(errno));
	}
	return 0;
}

int c_sock_check_connect(int s, struct addrinfo *ai)
{
	int sockerr = 0;
	socklen_t optlen;
	int r;

	optlen = sizeof(sockerr);
	r = getsockopt(s, SOL_SOCKET, SO_ERROR, &sockerr, &optlen);
	if (-1 == r) {
		log_sys_msg("%s() - getsockopt(): %s\n", __func__, strerror(errno));
		return 1;
	}
	if (sockerr) {
		log_sys_msg("%s() - connect(): %s\n", __func__, strerror(sockerr));
		return 1;
	}
	r = connect(s, ai->ai_addr, ai->ai_addrlen);
	if (-1 == r) {
		if (errno != EISCONN) {
			log_sys_msg("%s() - connect(): %s\n", __func__, strerror(errno));
			return 1;
		} else {
			return 0;
		}
	} else {
		log_dbg_msg("%s() - WOW!!! This is just impossible!!!\n", __func__);
		// but it is possible...
		return 0;
	}
}

/* returns 1 if state is ESTABLISHED, 0 otherwise */
int c_sock_check_state(int sock)
{
	struct tcp_info tcpi;
	socklen_t tcpi_len = sizeof(struct tcp_info);
	int r;

	r = getsockopt(sock, SOL_TCP, TCP_INFO, &tcpi, &tcpi_len);
	if (-1 == r) {
		log_sys_msg("%s() - getsockopt(%d, SOL_TCP, TCP_INFO): %s\n", __func__, sock, strerror(errno));
		return 0; /* ? */
	}

	if (tcpi.tcpi_state != TCP_ESTABLISHED)
		return 0;

	/* Ok, connection is alive */
	return 1;
}

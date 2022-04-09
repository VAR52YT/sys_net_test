#ifndef TEST_PARAMETERS_H
#define TEST_PARAMETERS_H

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netex/errno.h>
#include <netex/net.h>

typedef struct _test_parameters {
	int socket_type;
	struct sockaddr addr;
	struct sockaddr client_addr;
	int need_connect;
	int test_poll;
	int test_select;
} test_parameters;

#endif

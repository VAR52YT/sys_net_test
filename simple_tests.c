#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timer.h>
#include <cell/atomic.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netex/errno.h>
#include <netex/net.h>

void bind_socket(char *name, int type, struct sockaddr *addr)
{
	printf("Test bind_socket(%s)\n", name);

	int ret_value = 0;
	int ret = 0;
	int sock = 0;

	sock = socket(AF_INET, type, 0);
	if (sock < 0)
	{
		printf("[Simple] Failed to create socket: %d\n", sys_net_errno);
		ret_value = -1;
		goto cleanup;
	}

	printf("[Simple] Socket created\n");

	ret = bind(sock, addr, sizeof(struct sockaddr));
	if (ret < 0)
	{
		printf("[Simple] Failed to bind socket: %d\n", sys_net_errno);
		ret_value = -1;
		goto cleanup;
	}

	printf("[Simple] Socket bound\n");

	struct sockaddr res_addr;
	memset(&res_addr, 0, sizeof(res_addr));
	socklen_t size = sizeof(res_addr);
	ret = getsockname(sock, &res_addr, &size);
	if (ret < 0)
	{
		printf("[Simple] Failed to getsockname: %d\n", sys_net_errno);
		ret_value = -1;
		goto cleanup;
	}

	struct sockaddr_in_p2p *in_p2p_addr = (struct sockaddr_in_p2p *)&res_addr;

	printf("Resulting addr = %s, port = %d, vport = %d\n", inet_ntoa(in_p2p_addr->sin_addr), in_p2p_addr->sin_port, in_p2p_addr->sin_vport);

cleanup:
	if (sock)
	{
		socketclose(sock);
	}

	printf("Test %s ended with result %d\n", name, ret_value);
}

int run_simple_tests()
{
	struct sockaddr addr;
	struct sockaddr_in *in_addr = (struct sockaddr_in *)&addr;
	struct sockaddr_in_p2p *in_p2p_addr = (struct sockaddr_in_p2p *)&addr;

	memset(&addr, 0, sizeof(addr));
	in_addr->sin_family = AF_INET;
	in_addr->sin_port = 0;
	inet_aton("127.0.0.1", &in_addr->sin_addr);

	bind_socket("SOCK_STREAM", SOCK_STREAM, &addr);
	bind_socket("SOCK_DGRAM", SOCK_DGRAM, &addr);

	in_p2p_addr->sin_port = 3658;
	in_p2p_addr->sin_vport = 0;
	bind_socket("SOCK_DGRAM_P2P(port = 3658, vport = 0)", SOCK_DGRAM_P2P, &addr);
	in_p2p_addr->sin_port = 0;
	in_p2p_addr->sin_vport = 7000;
	bind_socket("SOCK_DGRAM_P2P(port = 0, vport = 7000)", SOCK_DGRAM_P2P, &addr);
	in_p2p_addr->sin_port = 0;
	in_p2p_addr->sin_vport = 0;
	bind_socket("SOCK_DGRAM_P2P(port = 0, vport = 0)", SOCK_DGRAM_P2P,& addr);

	in_p2p_addr->sin_port = 0;
	in_p2p_addr->sin_vport = 3658;
	bind_socket("SOCK_STREAM_P2P(port = 0, vport = 3658)", SOCK_STREAM_P2P, &addr);
	in_p2p_addr->sin_port = 7000;
	in_p2p_addr->sin_vport = 0;
	bind_socket("SOCK_STREAM_P2P(port = 7000, vport = 0)", SOCK_STREAM_P2P, &addr);
	in_p2p_addr->sin_port = 0;
	in_p2p_addr->sin_vport = 0;
	bind_socket("SOCK_STREAM_P2P(port = 0, vport = 0)", SOCK_STREAM_P2P, &addr);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timer.h>
#include <cell/atomic.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ppu_thread.h>

#include "test_parameters.h"

static uint64_t atomic_state __attribute__((aligned(128)));

int recvn(int sock, char *buf, int size)
{
	int total_size = size;
	int cur_left = size;

	while (cur_left != 0)
	{
		int res = recv(sock, buf + (total_size - cur_left), cur_left, 0);
		if (res < 0)
		{
			return -1;
		}
		cur_left -= res;
	}

	return total_size;
}

void server_thread(uint64_t arg)
{
	test_parameters *tparams = (test_parameters *)(uint32_t)arg;

	int s_server = 0, s_client = 0, ret = 0;
	uint64_t ret_value = 0;
	char buf[100];

	memset(buf, 0, sizeof(buf));

	s_server = socket(AF_INET, tparams->socket_type, 0);
	if (s_server < 0)
	{
		printf("[Server] Failed to create socket: %d\n", sys_net_errno);
		ret_value = -1;
		goto cleanup;
	}

	printf("[Server] Socket created\n");

	ret = bind(s_server, (const struct sockaddr *)&tparams->addr, sizeof(tparams->addr));
	if (ret < 0)
	{
		printf("[Server] Failed to bind socket: %d\n", sys_net_errno);
		ret_value = -1;
		goto cleanup;
	}

	printf("[Server] Socket bound\n");

	if (tparams->need_connect)
	{
		ret = listen(s_server, 1);
		if (ret < 0)
		{
			printf("[Server] Failed to listen: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Server] Socket listening\n");

		cellAtomicStore64(&atomic_state, 1);

		s_client = accept(s_server, NULL, 0);
		if (s_client < 0)
		{
			printf("[Server] Failed to accept: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Server] Accepted a client\n");

		if (tparams->test_poll)
		{
			struct pollfd pfds[1];
			pfds[0].fd = s_client;
			pfds[0].events = POLLIN;
			pfds[0].revents = 0;
			ret = socketpoll(pfds, 1, 5000);

			if (ret <= 0)
			{
				printf("[Server] Poll failed: %d\n", sys_net_errno);
				ret_value = -1;
				goto cleanup;
			}

			printf("[Server] Poll succeeded\n");
		}

		if (tparams->test_select)
		{
			struct fd_set fds;
			FD_ZERO(&fds);
			FD_SET(s_client, &fds);

			struct timeval tv;
			memset(&tv, 0, sizeof(tv));
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			ret = socketselect(s_client + 1, &fds, NULL, NULL, &tv);
			if (ret <= 0)
			{
				printf("[Server] Select failed: %d\n", sys_net_errno);
				ret_value = -1;
				goto cleanup;
			}

			printf("[Server] Select succeeded\n");
		}

		ret = recvn(s_client, buf, 100);
		if (ret < 0)
		{
			printf("[Server] Failed to recv: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Server] Received 100 bytes\n");

		ret = send(s_client, buf, 100, 0);
		if (ret < 0 || ret != 100)
		{
			printf("[Server] Failed to send back the data: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Server] Sent back 100 bytes\n");
	}
	else
	{
		cellAtomicStore64(&atomic_state, 1);

		if (tparams->test_poll)
		{
			struct pollfd pfds[1];
			pfds[0].fd = s_server;
			pfds[0].events = POLLIN;
			pfds[0].revents = 0;
			ret = socketpoll(pfds, 1, 5000);

			if (ret <= 0)
			{
				printf("[Server] Poll failed: %d\n", sys_net_errno);
				ret_value = -1;
				goto cleanup;
			}

			printf("[Server] Poll succeeded\n");
		}

		if (tparams->test_select)
		{
			struct fd_set fds;
			FD_ZERO(&fds);
			FD_SET(s_server, &fds);

			struct timeval tv;
			memset(&tv, 0, sizeof(tv));
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			ret = socketselect(s_server + 1, &fds, NULL, NULL, &tv);
			if (ret <= 0)
			{
				printf("[Server] Select failed: %d\n", sys_net_errno);
				ret_value = -1;
				goto cleanup;
			}

			printf("[Server] Select succeeded\n");
		}

		struct sockaddr source_addr;
		socklen_t addr_len = sizeof(source_addr);
		ret = recvfrom(s_server, buf, 100, 0, &source_addr, &addr_len);
		if (ret < 0 || ret != 100)
		{
			printf("[Server] Failed to recv: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Server] Received 100 bytes\n");

		ret = sendto(s_server, buf, 100, 0, (const struct sockaddr *)&tparams->client_addr, sizeof(tparams->client_addr));
		if (ret < 0 || ret != 100)
		{
			printf("[Server] Failed to send: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Server] Sent back 100 bytes\n");
	}

cleanup:
	if (s_client)
	{
		socketclose(s_client);
	}

	if (s_server)
	{
		socketclose(s_server);
	}

	while (cellAtomicCompareAndSwap64(&atomic_state, 3, 4) != 3)
	{
	}

	sys_ppu_thread_exit(ret_value);
}

void client_thread(uint64_t arg)
{
	test_parameters *tparams = (test_parameters *)(uint32_t)arg;

	int sock = 0, ret = 0;
	uint64_t ret_value = 0;
	char buf[100], buf2[100];

	memset(buf, 0, sizeof(buf));
	memset(buf2, 0, sizeof(buf2));

	sock = socket(AF_INET, tparams->socket_type, 0);
	if (sock < 0)
	{
		printf("[Client] Failed to create socket: %d\n", sys_net_errno);
		ret_value = -1;
		goto cleanup;
	}

	printf("[Client] Socket created\n");

	while (cellAtomicCompareAndSwap64(&atomic_state, 1, 2) != 1)
	{
	}

	if (tparams->need_connect)
	{
		ret = connect(sock, (const struct sockaddr *)&tparams->addr, sizeof(tparams->addr));
		if (ret < 0)
		{
			printf("[Client] Failed to connect: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Client] Connected to server\n");

		srand(0);
		for (int i = 0; i < 100; i++)
		{
			buf[i] = rand();
		}

		ret = send(sock, buf, 100, 0);
		if (ret < 0 || ret != 100)
		{
			printf("[Client] Failed to send: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Client] Sent 100 bytes\n");

		ret = recvn(sock, buf2, 100);
		if (ret < 0)
		{
			printf("[Client] Failed to recv: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Client] Received 100 bytes\n");
	}
	else
	{
		ret = bind(sock, (const struct sockaddr *)&tparams->client_addr, sizeof(tparams->client_addr));
		if (ret < 0)
		{
			printf("[Client] Failed to bind socket: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Client] Socket bound\n");

		ret = sendto(sock, buf, 100, 0, (const struct sockaddr *)&tparams->addr, sizeof(tparams->addr));
		if (ret < 0 || ret != 100)
		{
			printf("[Client] Failed to send: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Client] Sent 100 bytes\n");

		struct sockaddr source_addr;
		socklen_t addr_len = sizeof(source_addr);
		ret = recvfrom(sock, buf2, 100, 0, &source_addr, &addr_len);
		if (ret < 0 || ret != 100)
		{
			printf("[Client] Failed to recv: %d\n", sys_net_errno);
			ret_value = -1;
			goto cleanup;
		}

		printf("[Client] Received 100 bytes\n");
	}

	if (memcmp(buf, buf2, 100) != 0)
	{
		printf("[Client] data mismatch!\n");
		ret_value = -1;
		goto cleanup;
	}

	printf("[Client] Data matched\n");

cleanup:
	if (sock)
	{
		socketclose(sock);
	}

	while (cellAtomicCompareAndSwap64(&atomic_state, 2, 3) != 2)
	{
	}

	sys_ppu_thread_exit(ret_value);
}

int run_test(char *name, test_parameters *tparams)
{
	int ret = 0;
	sys_ppu_thread_t thread_server = 0, thread_client = 0;
	uint64_t ret_value = 0;

	printf("Running test %s\n", name);

	cellAtomicStore64(&atomic_state, 0);

	ret = sys_ppu_thread_create(&thread_server, server_thread, (uint64_t)(uint32_t)tparams, 1000, 0x1000, SYS_PPU_THREAD_CREATE_JOINABLE, "Server Thread");
	if (ret < 0)
	{
		goto cleanup;
	}

	ret = sys_ppu_thread_create(&thread_client, client_thread, (uint64_t)(uint32_t)tparams, 1000, 0x1000, SYS_PPU_THREAD_CREATE_JOINABLE, "Client Thread");
	if (ret < 0)
	{
		goto cleanup;
	}

	while (cellAtomicCompareAndSwap64(&atomic_state, 4, 5) != 4)
	{
		sys_timer_sleep(1);
	}

cleanup:
	if (thread_server)
	{
		sys_ppu_thread_join(thread_server, &ret_value);
		printf("Joined server thread with ret value %d\n", ret_value);
		if (ret_value != 0 && ret == 0)
		{
			ret = ret_value;
		}
	}

	if (thread_client)
	{
		sys_ppu_thread_join(thread_client, &ret_value);
		printf("Joined client thread with ret value %d\n", ret_value);
		if (ret_value != 0 && ret == 0)
		{
			ret = ret_value;
		}
	}

	printf("Test %s completed with return value %d\n", name, ret);

	return ret;
}

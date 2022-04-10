#include <stdio.h>
#include <string.h>
#include <netex/net.h>
#include <cell/sysmodule.h>
#include "test_parameters.h"

int run_test(char *name, test_parameters *tparams);
int run_simple_tests();

#define TEST_POLL 1
#define TEST_SELECT 0

int main()
{
	int ret = 0;

	printf("sys_net test v0.3.0 by GalCiv\n");

	ret = cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
	if (ret < 0)
	{
		goto cleanup;
	}

	sys_net_initialize_network();

	run_simple_tests();

	test_parameters tparams;
	struct sockaddr_in *in_addr = (struct sockaddr_in *)&tparams.addr;
	struct sockaddr_in_p2p *in_p2p_addr = (struct sockaddr_in_p2p *)&tparams.addr;
	struct sockaddr_in *c_in_addr = (struct sockaddr_in *)&tparams.client_addr;
	struct sockaddr_in_p2p *c_in_p2p_addr = (struct sockaddr_in_p2p *)&tparams.client_addr;

	memset(&tparams, 0, sizeof(tparams));
	tparams.need_connect = 1;
	tparams.test_poll = TEST_POLL;
	tparams.test_select = TEST_SELECT;
	tparams.socket_type = SOCK_STREAM;
	inet_aton("127.0.0.1", &in_addr->sin_addr);
	in_addr->sin_port = htons(3456);
	in_addr->sin_family = AF_INET;
	run_test("TCP", &tparams);

	memset(&tparams, 0, sizeof(tparams));
	tparams.need_connect = 1;
	tparams.test_poll = TEST_POLL;
	tparams.test_select = TEST_SELECT;
	tparams.socket_type = SOCK_STREAM_P2P;
	inet_aton("127.0.0.1", &in_p2p_addr->sin_addr);
	in_p2p_addr->sin_port = htons(3555);
	in_p2p_addr->sin_vport = htons(3658);
	in_addr->sin_family = AF_INET;
	run_test("UDP2TCP", &tparams);

	memset(&tparams, 0, sizeof(tparams));
	tparams.need_connect = 0;
	tparams.test_poll = TEST_POLL;
	tparams.test_select = TEST_SELECT;
	tparams.socket_type = SOCK_DGRAM;
	inet_aton("127.0.0.1", &in_addr->sin_addr);
	in_addr->sin_port = htons(3456);
	in_addr->sin_family = AF_INET;
	inet_aton("127.0.0.1", &c_in_addr->sin_addr);
	c_in_addr->sin_port = htons(3457);
	c_in_addr->sin_family = AF_INET;
	run_test("UDP", &tparams);

	memset(&tparams, 0, sizeof(tparams));
	tparams.need_connect = 0;
	tparams.test_poll = TEST_POLL;
	tparams.test_select = TEST_SELECT;
	tparams.socket_type = SOCK_DGRAM_P2P;
	inet_aton("127.0.0.1", &in_p2p_addr->sin_addr);
	in_p2p_addr->sin_port = htons(3658);
	in_p2p_addr->sin_vport = htons(3456);
	in_p2p_addr->sin_family = AF_INET;
	inet_aton("127.0.0.1", &c_in_p2p_addr->sin_addr);
	c_in_p2p_addr->sin_port = htons(3658);
	c_in_p2p_addr->sin_vport = htons(3457);
	c_in_p2p_addr->sin_family = AF_INET;
	run_test("UDPP2P", &tparams);

	cleanup:
	sys_net_finalize_network();
	return ret;
}

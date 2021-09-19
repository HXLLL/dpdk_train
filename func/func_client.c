/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2015 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../utils/argparse.h"

#include "func.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define MAX_REQUEST 10000000

int64_t (*pw)(int64_t, int64_t, int64_t) = pw_Ologn;

int ARGS_port = 0;
int ARGS_n = 10;
int ARGS_outstanding = 32;
int ARGS_base = 2;
int ARGS_mod = 1e9+7;

struct argparse_option options[] = {
    OPT_INTEGER('p', "port", &ARGS_port, "port to send & recv"),
    OPT_INTEGER('n', NULL, &ARGS_n, "nth item to be calculated"),
    OPT_INTEGER(0, "outstanding", &ARGS_outstanding, "maximum outstanding requests"),
    OPT_INTEGER('b', "base", &ARGS_base, "power base"),
    OPT_INTEGER('m', "mod", &ARGS_mod, "mod"),
};

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_LEN }
};

/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;

	if (port >= rte_eth_dev_count_avail())
		return -1;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, nb_txd,
				rte_eth_dev_socket_id(port), NULL);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	struct rte_ether_addr addr;
	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			(unsigned)port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	rte_eth_promiscuous_enable(port);

	return 0;
}

struct rte_mempool *mbuf_pool;

uint64_t *msg;

struct rte_ether_addr s_addr = {{0xb8, 0xce, 0xf6, 0x83, 0xa5, 0x9a}};
struct rte_ether_addr d_addr = {{0xb8, 0xce, 0xf6, 0x83, 0xb2, 0xea}};
uint16_t ether_type = 0x0a00;
struct rte_mbuf *send_buffer[BURST_SIZE];
struct rte_mbuf *recv_buffer[BURST_SIZE];
int64_t std_ans[MAX_REQUEST];
uint64_t req_cnt;
int outstanding;


/* make request */
/* | ethernet_header | func_id |  req_id  | parameters               |
 * | 14 bytes        | 4bytes  |  8bytes  | 8bytes + 8bytes + 8bytes |
 * |                 |         |          | x        y        MOD    |
 *
 ***/
struct rte_mbuf *make_request(int64_t n) {
    struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool);

    struct rte_ether_hdr *eth_hdr;
    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
    eth_hdr->d_addr = d_addr;
    eth_hdr->s_addr = s_addr;
    eth_hdr->ether_type = ether_type;

    struct func_request *func_req;
    func_req = ether_mtod(pkt, struct func_request*);
    func_req->func_id = 1;
    func_req->req_id = ++req_cnt;
    func_req->x = ARGS_base;
    func_req->y = n;
    func_req->MOD = ARGS_mod;

    int pkt_size = sizeof(struct func_request) + sizeof(struct rte_ether_hdr);
    pkt->data_len = pkt_size;
    pkt->pkt_len = pkt_size;

    std_ans[func_req->req_id] = pw(func_req->x, func_req->y, func_req->MOD);

    return pkt;
}


int send_request(int n) {
    send_buffer[0] = make_request(n);

    int nb_tx = rte_eth_tx_burst(ARGS_port, 0, send_buffer, 1);
    return 0;
}

int check_response(struct rte_mbuf* resp) {
    struct func_response* r;
    r = ether_mtod(resp, struct func_response*);
    return r->ans == std_ans[r->req_id];
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int main(int argc, char *argv[])
{
    unsigned nb_ports;
    uint8_t portid;

    /* Initialize the Environment Abstraction Layer (EAL). */
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

    argc -= ret;
    argv += ret;

    /* Parse custom parameters. */
    struct argparse argparse;
    argparse_init(&argparse, options, NULL, 0);
    argc = argparse_parse(&argparse, argc, argv);

    /* Check that there is an even number of ports to send/receive on. */
    nb_ports = rte_eth_dev_count_avail();
    if (nb_ports < 2 || (nb_ports & 1))
        rte_exit(EXIT_FAILURE, "Error: number of ports must be even\n");

    /* Creates a new mempool in memory to hold the mbufs. */
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS * nb_ports,
            MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    /* Initialize all ports. */
    for (portid = 0; portid < nb_ports; portid++)
        if (port_init(portid, mbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
                    portid);

    if (rte_lcore_count() > 1)
        printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

    int i;
    uint64_t last_report = rte_rdtsc(), start_time = rte_rdtsc();
    double every_second = 1.0;
    uint64_t every = (int)(every_second * rte_get_tsc_hz());
    uint64_t report_cnt = 0;
    INFO("%s", "begin to send reqs");
    for (i=1;i<=ARGS_n;++i) {
        sleep(1);
        int nb_rx = rte_eth_rx_burst(ARGS_port, 0, recv_buffer, BURST_SIZE);
        outstanding -= nb_rx;
        report_cnt += nb_rx;
        INFO("received %d resp, outstanding=%d", nb_rx, outstanding);
        for (int i=0;i!=nb_rx;++i) {
            rte_pktmbuf_free(recv_buffer[i]);
            recv_buffer[i] = NULL;
        }
        for (int j=0;j!=nb_rx;++j) {
            int flag = check_response(recv_buffer[j]);
            if (!flag) {
                INFO("%s", "ERROR!");
                return 0;
            }
        }

        if (rte_rdtsc() - last_report > every) {
            printf("tput: %.3lf\n", report_cnt / every_second);
            report_cnt = 0;
            last_report = rte_rdtsc();
        }

        if (outstanding == ARGS_outstanding) continue;

        INFO("sent req %d", i);
        send_request(i);
        ++outstanding;
    }
    printf("avg rtt: %.3lf us\n", ARGS_n / start_time / rte_get_tsc_hz() * 1e6);

    return 0;
}

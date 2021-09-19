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

int64_t pw(int64_t x, int64_t y, int64_t MOD) {
    int64_t ans=1;
    for (;y;y>>=1) { if (y&1) ans=ans*x%MOD; x=x*x%MOD; }
    return ans;
}

int ARGS_port=0;
int ARGS_cnt=4;
int ARGS_n=10;
int ARGS_outstanding=32;
int ARGS_base=2;
int ARGS_mod=1e9+7;

struct argparse_option options[] = {
    OPT_INTEGER('p', "port", &ARGS_port, "port to send & recv"),
    OPT_INTEGER('c', "cnt", &ARGS_cnt, "amount of data to send"),
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
struct rte_mbuf *pkt[BURST_SIZE];
int64_t ans[MAX_REQUEST];
struct __attribute__ ((packed)) func_request {
    int id;
    uint64_t x;
    uint64_t y;
    uint64_t MOD;
};

/* make request */
/* | ethernet_header | function_id | parameters               |
 * | 14 bytes        | 4bytes      | 8bytes + 8bytes + 8bytes |
 * |                 |             | x        y        MOD    |
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
    func_req = rte_pktmbuf_mtod(pkt, struct func_request*);
    func_req->id = 1;
    func_req->x = ARGS_base;
    func_req->y = n;
    func_req->MOD = ARGS_mod;
    return pkt;
}

int send_request(void) {
    make_request(&pkt[0]);
    pkt[0] = rte_pktmbuf_alloc(mbuf_pool);
    int pkt_size = sizeof(uint64_t) + sizeof(struct rte_ether_hdr);
    pkt[0]->data_len = pkt_size;
    pkt[0]->pkt_len = pkt_size;

    int nb_tx = rte_eth_tx_burst(2, 0, pkt, 1);

    rte_pktmbuf_free(pkt[0]);
    return 0;
}
uint64_t recv_response(void) {
    int nb_rx=0;

    pkt[0] = rte_pktmbuf_alloc(mbuf_pool);
    while (!nb_rx) {
        nb_rx = rte_eth_rx_burst(2, 0, pkt, BURST_SIZE);
    }
    msg = (uint64_t*)(rte_pktmbuf_mtod(pkt[0], char*) + sizeof(struct rte_ether_hdr));
    rte_pktmbuf_free(pkt[0]);
    return *msg;
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
    int
main(int argc, char *argv[])
{
    printf("%d\n", sizeof(struct func_request));
    return 0;
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

    printf("cnt:%d port:%d\n", ARGS_cnt, ARGS_port);

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

    int times = 4;
    if (argc > 1) {
        times = strtol(argv[1], NULL, 10);
    }
    int i;
    double total = 0;
    for (i=1;i<=times;++i) {
        uint64_t start = send_timestamp();
        recv_response();
        uint64_t rtt = rte_rdtsc() - start;
        printf("iter %d: rtt = %.3lf us\n", i, 1.0 * rtt / rte_get_tsc_hz() * 1e6);
        total += rtt;
        sleep(1);
    }
    printf("avg rtt: %.3lf us\n", total / times / rte_get_tsc_hz() * 1e6);

    return 0;
}

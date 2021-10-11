#ifndef MAIN_H
#define MAIN_H

#include "../utils/argparse.h"

#include <gperftools/profiler.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

/* Constants */

#define MAX_THREADS 512

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32
#define MAX_REQUEST 100000000

static struct rte_ether_addr client_addr = {{0xb8, 0xce, 0xf6, 0x83, 0xb2, 0xeb}};
static struct rte_ether_addr server_addr = {{0xb8, 0xce, 0xf6, 0x83, 0xa5, 0x9b}};

//static struct rte_ether_addr server_addr = {{0xb8, 0xce, 0xf6, 0x83, 0xb2, 0xea}};
//static struct rte_ether_addr client_addr = {{0xb8, 0xce, 0xf6, 0x83, 0xa5, 0x9a}};
//
static const uint16_t ether_type = 0x0a00;

/* Parameters */

extern int ARGS_port;
extern int ARGS_n;
extern int ARGS_outstanding;
extern int ARGS_base;
extern int ARGS_mod;
extern int ARGS_isclient;
extern int ARGS_thread;

static struct argparse_option options[] = {
    OPT_HELP(),
    OPT_INTEGER('p', "port", &ARGS_port, "port to send & recv"),
    OPT_INTEGER('n', NULL, &ARGS_n, "nth item to be calculated"),
    OPT_INTEGER('o', "outstanding", &ARGS_outstanding, "maximum outstanding requests"),
    OPT_INTEGER('b', "base", &ARGS_base, "power base"),
    OPT_INTEGER('m', "mod", &ARGS_mod, "mod"),
    OPT_INTEGER('c', "isclient", &ARGS_isclient, "instance is client"),
    OPT_INTEGER('t', "thread", &ARGS_thread, "number of threads"),
    OPT_END(),
};

/* Global Variables */

/* Structures */

struct __attribute__ ((packed)) func_request {
    int func_id;
    uint64_t req_id;
    int64_t x;
    int64_t y;
    int64_t MOD;
};
struct __attribute__ ((packed)) func_response {
    int func_id;
    uint64_t req_id;
    int64_t ans;
};

struct thread_params_t {
  size_t id;
  struct rte_ether_addr *s_addr, *d_addr;
  struct rte_mempool *mbuf_pool;
};

struct ctrl_blk_t {
    size_t port_id;
    size_t queue_id;
    size_t lid;

    struct rte_mempool *mbuf_pool;

    struct rte_ether_addr *s_addr;
    struct rte_ether_addr *d_addr;

    struct rte_mbuf *send_buffer[BURST_SIZE];
    struct rte_mbuf *recv_buffer[BURST_SIZE];

    uint64_t req_cnt;
    int64_t *std_ans;
    int outstanding;
};

/* Functions */

void *run_client(void* p);
void *run_server(void* p);

/* Utils */

int port_init(uint8_t port, struct rte_mempool *mbuf_pool[], uint32_t nb_rx_queue, uint32_t nb_tx_queue);
struct ctrl_blk_t* ctrl_blk_init();


#define dump_eth(eth_hdr) printf("packet received, from MAC: %02" PRIx8 " %02" PRIx8\
                " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 " :",\
                eth_hdr->s_addr.addr_bytes[0],\
                eth_hdr->s_addr.addr_bytes[1],\
                eth_hdr->s_addr.addr_bytes[2],\
                eth_hdr->s_addr.addr_bytes[3],\
                eth_hdr->s_addr.addr_bytes[4],\
                eth_hdr->s_addr.addr_bytes[5]);\

#define ether_mtod(pkt, type) (type)(rte_pktmbuf_mtod(pkt, char*) + sizeof(struct rte_ether_hdr))

#define INFO( fmt,  ... ) fprintf(stderr, "[INFO] " fmt "\n",  __VA_ARGS__)

static int64_t pw_Ologn(int64_t x, int64_t y, int64_t MOD) {
    int64_t ans=1;
    for (;y;y>>=1) { if (y&1) ans=ans*x%MOD; x=x*x%MOD; }
    return ans;
}
static int64_t pw_On(int64_t x, int64_t y, int64_t MOD) {
    int64_t ans=1;
    for (int i=0;i!=y;++i) ans = ans*x % MOD;
    return ans;
}
#endif
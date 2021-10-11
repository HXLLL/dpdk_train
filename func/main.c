#include "main.h"

int ARGS_port = 0;
int ARGS_n = 10;
int ARGS_outstanding = 32;
int ARGS_base = 2;
int ARGS_mod = 1e9+7;
int ARGS_isclient = 0;

struct rte_mempool *mbuf_pool;

struct rte_mbuf *send_buffer[BURST_SIZE];
struct rte_mbuf *recv_buffer[BURST_SIZE];

struct rte_ether_addr* s_addr;
struct rte_ether_addr* d_addr;

int main(int argc, char *argv[]) {
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
    printf("%d\n", argc);
    argc = argparse_parse(&argparse, argc, argv);

    nb_ports = rte_eth_dev_count_avail();

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

    if (ARGS_isclient) {
        s_addr = &client_addr;
        d_addr = &server_addr;
        run_client(NULL);
    } else {
        s_addr = &server_addr;
        d_addr = &client_addr;
        run_server(NULL);
    }
}
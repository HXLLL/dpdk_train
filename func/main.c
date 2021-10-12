#include "main.h"

int ARGS_port = 0;
int ARGS_n = 10;
int ARGS_outstanding = 64;
int ARGS_base = 2;
int ARGS_mod = 1e9+7;
int ARGS_isclient = 0;
int ARGS_thread = 1;

struct rte_mempool *mbuf_pool[MAX_THREADS];
char mbuf_pool_names[MAX_THREADS][30];

pthread_t thread_arr[MAX_THREADS];
struct thread_params_t thread_params[MAX_THREADS];

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
    argc = argparse_parse(&argparse, argc, argv);

    nb_ports = rte_eth_dev_count_avail();

    for (int i=0;i!=1;++i) {
        sprintf(mbuf_pool_names[i], "MBUF_POOL_%d", i);
        mbuf_pool[i] = rte_pktmbuf_pool_create(mbuf_pool_names[i], NUM_MBUFS * nb_ports * ARGS_thread,
                MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, 0);
        if (mbuf_pool[i] == NULL) {
            INFO("%s", rte_strerror(rte_errno));
            rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
        }
    }

    /* Initialize all ports. */
    for (portid = 0; portid < nb_ports; portid++)
        if (port_init(portid, mbuf_pool, ARGS_thread, ARGS_thread) != 0)
            rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n",
                    portid);

    if (ARGS_isclient) {
        for (int i=0;i<ARGS_thread;++i) {
            thread_params[i].id = i;
            thread_params[i].s_addr = &client_addr;
            thread_params[i].d_addr = &server_addr;
            thread_params[i].mbuf_pool = mbuf_pool[0];
            pthread_create(&thread_arr[i], NULL, run_client, &thread_params[i]);
        }
    } else {
        for (int i=0;i<ARGS_thread;++i) {
            thread_params[i].id = i;
            thread_params[i].s_addr = &server_addr;
            thread_params[i].d_addr = &client_addr;
            thread_params[i].mbuf_pool = mbuf_pool[0];
            pthread_create(&thread_arr[i], NULL, run_server, &thread_params[i]);
        }
    }
    for (int i=0;i!=ARGS_thread;++i) {
        pthread_join(thread_arr[i], NULL);
    }
}
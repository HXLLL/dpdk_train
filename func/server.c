#include "main.h"

static int64_t (*pw)(int64_t, int64_t, int64_t) = pw_On;

/* make response */
/* | ethernet_header | req_id  | result |
 * | 14 bytes        | 8bytes  | 8bytes |
 * |                 |         | ans    |
 *
 ***/
struct rte_mbuf *make_response(struct ctrl_blk_t* cb, struct rte_mbuf *req) {
    struct func_request *r;
    r = ether_mtod(req, struct func_request*);
    // uint64_t ans = pw(r->x, r->y, r->MOD);
    uint64_t ans = 0;

    struct rte_mbuf *pkt = rte_pktmbuf_alloc(cb->mbuf_pool);

    struct rte_ether_hdr *eth_hdr;
    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
    eth_hdr->d_addr = *cb->d_addr;
    eth_hdr->s_addr = *cb->s_addr;
    eth_hdr->ether_type = ether_type;

    struct func_response *r2;
    r2 = ether_mtod(pkt, struct func_response*);
    r2->req_id = r->req_id;
    r2->ans = ans;

    int pkt_size = sizeof(struct func_request) + sizeof(struct rte_ether_hdr);
    pkt->data_len = pkt_size;
    pkt->pkt_len = pkt_size;

    return pkt;
}

void *run_server(void* p) {
    struct thread_params_t* params = (struct thread_params_t*)p;

    struct ctrl_blk_t *cb=ctrl_blk_init();

    cb->mbuf_pool = params->mbuf_pool;
    cb->s_addr = params->s_addr;
    cb->d_addr = params->d_addr;

    uint64_t last_report = rte_rdtsc(), start_time = rte_rdtsc();
    double every_second = 1.0;
    uint64_t every = (uint64_t)(every_second * rte_get_tsc_hz());
    uint64_t temp_cnt = 0, total_cnt = 0;
    uint64_t nb_burst = 0, temp_burst = 0;
    int nb_report = 0;
    INFO("%s", "start listening");

    struct rte_eth_burst_mode mode;
    rte_eth_rx_burst_mode_get(ARGS_port, 0, &mode);
    printf("%ld %s\n", mode.flags, mode.info);


    for (;;) {
        int nb_rx = rte_eth_rx_burst(ARGS_port, 0, cb->recv_buffer, BURST_SIZE);
        ++nb_burst; ++temp_burst;
        // INFO("received %d reqs", nb_rx);
        for (int i=0;i!=nb_rx;++i) {
            cb->send_buffer[i] = make_response(cb, cb->recv_buffer[i]);
            rte_pktmbuf_free(cb->recv_buffer[i]);
            cb->recv_buffer[i] = NULL;
        }
        int nb_tx = rte_eth_tx_burst(ARGS_port, 0, cb->send_buffer, nb_rx);
        // INFO("sent %d resps", nb_tx);

        temp_cnt += nb_tx;
        total_cnt += nb_tx;

        // sleep(1);
        if (rte_rdtsc() - last_report > every) {
            printf("tput: %.3lf, avg burst: %.3lf\n", temp_cnt / every_second, 1.0*temp_cnt / temp_burst);
            temp_cnt = 0; temp_burst = 0;
            last_report = rte_rdtsc();

            if (total_cnt >= 100000) {
                if (nb_report == 0) ProfilerStart("server.perf");
                nb_report += 1;
            }
            if (nb_report == 10) {
                ProfilerStop();
            }

        }
    }
    printf("avg rtt: %.3lf us\n", total_cnt / start_time / rte_get_tsc_hz() * 1e6);

    return 0;
}
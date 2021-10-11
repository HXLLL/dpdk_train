#include "main.h"

static int64_t (*pw)(int64_t, int64_t, int64_t) = pw_On;

/* make response */
/* | ethernet_header | req_id  | result |
 * | 14 bytes        | 8bytes  | 8bytes |
 * |                 |         | ans    |
 *
 ***/
struct rte_mbuf *make_response(struct rte_mbuf *req) {
    struct func_request *r;
    r = ether_mtod(req, struct func_request*);
    // uint64_t ans = pw(r->x, r->y, r->MOD);
    uint64_t ans = 0;

    struct rte_mbuf *pkt = rte_pktmbuf_alloc(mbuf_pool);

    struct rte_ether_hdr *eth_hdr;
    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
    eth_hdr->d_addr = *d_addr;
    eth_hdr->s_addr = *s_addr;
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

int run_server(struct thread_params_t* params)
{
    uint64_t last_report = rte_rdtsc(), start_time = rte_rdtsc();
    uint64_t total_cnt;
    double every_second = 1.0;
    uint64_t every = (uint64_t)(every_second * rte_get_tsc_hz());
    uint64_t report_cnt = 0;
    INFO("%s", "start listening");
    for (;;) {
        int nb_rx = rte_eth_rx_burst(ARGS_port, 0, recv_buffer, BURST_SIZE);
        // INFO("received %d reqs", nb_rx);
        for (int i=0;i!=nb_rx;++i) {
            send_buffer[i] = make_response(recv_buffer[i]);
            rte_pktmbuf_free(recv_buffer[i]);
            recv_buffer[i] = NULL;
        }
        int nb_tx = rte_eth_tx_burst(ARGS_port, 0, send_buffer, nb_rx);
        // INFO("sent %d resps", nb_tx);

        report_cnt += nb_tx;
        // sleep(1);
        if (rte_rdtsc() - last_report > every) {
            printf("tput: %.3lf\n", report_cnt / every_second);
            report_cnt = 0;
            last_report = rte_rdtsc();
        }
    }
    printf("avg rtt: %.3lf us\n", total_cnt / start_time / rte_get_tsc_hz() * 1e6);

    return 0;
}
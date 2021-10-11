#include "main.h"

static int64_t (*pw)(int64_t, int64_t, int64_t) = pw_Ologn;

static int64_t std_ans[MAX_REQUEST];
static uint64_t req_cnt;
static int outstanding;

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
    eth_hdr->d_addr = *d_addr;
    eth_hdr->s_addr = *s_addr;
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

    // std_ans[func_req->req_id] = pw(func_req->x, func_req->y, func_req->MOD);

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
int run_client(struct thread_params_t* params)
{
    int i;
    uint64_t last_report = rte_rdtsc(), start_time = rte_rdtsc();
    double every_second = 1.0;
    uint64_t every = (int)(every_second * rte_get_tsc_hz());
    uint64_t report_cnt = 0;
    INFO("%s", "begin to send reqs");
    for (i=1;i<=ARGS_n;) {
        int nb_rx = rte_eth_rx_burst(ARGS_port, 0, recv_buffer, BURST_SIZE);
        outstanding -= nb_rx;
        report_cnt += nb_rx;
        // INFO("received %d resp, outstanding=%d", nb_rx, outstanding);
        for (int j=0;j!=nb_rx;++j) {
            // int flag = check_response(recv_buffer[j]);
            int flag=true;
            if (!flag) {
                INFO("%s", "ERROR!");
                return 0;
            }
            rte_pktmbuf_free(recv_buffer[j]);
            recv_buffer[j] = NULL;
        }

        if (rte_rdtsc() - last_report > every) {
            printf("tput: %.3lf\n", report_cnt / every_second);
            report_cnt = 0;
            last_report = rte_rdtsc();
        }

        if (outstanding == ARGS_outstanding) continue;

        // INFO("sent req %d", i);
        send_request(i);
        ++outstanding;

        ++i;
    }
    sleep(2);
    printf("avg rtt: %.3lf us\n", ARGS_n / start_time / rte_get_tsc_hz() * 1e6);

    return 0;
}

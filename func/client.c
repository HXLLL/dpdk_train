#include "main.h"

static int64_t (*pw)(int64_t, int64_t, int64_t) = pw_Ologn;

/* make request */
/* | ethernet_header | func_id |  req_id  | parameters               |
 * | 14 bytes        | 4bytes  |  8bytes  | 8bytes + 8bytes + 8bytes |
 * |                 |         |          | x        y        MOD    |
 *
 ***/
struct rte_mbuf *make_request(struct ctrl_blk_t* cb, int64_t n) {
    struct rte_mbuf *pkt = rte_pktmbuf_alloc(cb->mbuf_pool);

    struct rte_ether_hdr *eth_hdr;
    eth_hdr = rte_pktmbuf_mtod(pkt, struct rte_ether_hdr*);
    eth_hdr->d_addr = *cb->d_addr;
    eth_hdr->s_addr = *cb->s_addr;
    eth_hdr->ether_type = ether_type;

    struct func_request *func_req;
    func_req = ether_mtod(pkt, struct func_request*);
    func_req->func_id = 1;
    func_req->req_id = ++cb->req_cnt;
    func_req->x = ARGS_base;
    func_req->y = n;
    func_req->MOD = ARGS_mod;

    int pkt_size = sizeof(struct func_request) + sizeof(struct rte_ether_hdr);
    pkt->data_len = pkt_size;
    pkt->pkt_len = pkt_size;

    // cb->std_ans[func_req->req_id] = pw(func_req->x, func_req->y, func_req->MOD);

    return pkt;
}


int send_request(struct ctrl_blk_t* cb, int n) {
    cb->send_buffer[0] = make_request(cb, n);

    int nb_tx = rte_eth_tx_burst(ARGS_port, cb->lid, cb->send_buffer, 1);
    return 0;
}

int check_response(struct ctrl_blk_t* cb, struct rte_mbuf* resp) {
    struct func_response* r;
    r = ether_mtod(resp, struct func_response*);
    return r->ans == cb->std_ans[r->req_id];
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
void *run_client(void *p) {
    struct thread_params_t *params = (struct thread_params_t *)p;
    
    struct ctrl_blk_t* cb=ctrl_blk_init();

    cb->mbuf_pool = params->mbuf_pool;
    cb->s_addr = params->s_addr;
    cb->d_addr = params->d_addr;
    cb->lid = params->id;
    cb->std_ans = (uint64_t*)malloc(sizeof(uint64_t) * ARGS_n);

    int i;
    uint64_t last_report = rte_rdtsc(), start_time = rte_rdtsc();
    double every_second = 1.0;
    uint64_t every = (uint64_t)(every_second * rte_get_tsc_hz());
    uint64_t report_cnt = 0;
    int nb_report = 0;

    INFO("%s", "begin to send reqs");
    for (i=1;i<=ARGS_n;) {
        if (rte_rdtsc() - last_report > every) {
            printf("tput: %.3lf\n", report_cnt / every_second);
            report_cnt = 0;
            last_report = rte_rdtsc();

            //if (i >= 10000) {
                //nb_report += 1;
                //ProfilerStart("client.perf");
            //}
            //if (nb_report == 10) {
                //ProfilerStop();
            //}
        }

        int nb_rx = rte_eth_rx_burst(ARGS_port, cb->lid, cb->recv_buffer, BURST_SIZE);
        cb->outstanding -= nb_rx;
        report_cnt += nb_rx;
        // INFO("received %d resp, cb->outstanding=%d", nb_rx, cb->outstanding);
        for (int j=0;j!=nb_rx;++j) {
            // int flag = check_response(recv_buffer[j]);
            int flag=true;
            if (!flag) {
                INFO("%s", "ERROR!");
                return 0;
            }
            rte_pktmbuf_free(cb->recv_buffer[j]);
            cb->recv_buffer[j] = NULL;
        }

        if (cb->outstanding == ARGS_outstanding) continue;

        // INFO("sent req %d", i);
        send_request(cb, i);
        ++cb->outstanding;

        ++i;
    }
    sleep(2);
    printf("avg rtt: %.3lf us\n", ARGS_n / start_time / rte_get_tsc_hz() * 1e6);

    return 0;
}

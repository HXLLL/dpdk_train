#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

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

int64_t pw_Ologn(int64_t x, int64_t y, int64_t MOD) {
    int64_t ans=1;
    for (;y;y>>=1) { if (y&1) ans=ans*x%MOD; x=x*x%MOD; }
    return ans;
}
int64_t pw_On(int64_t x, int64_t y, int64_t MOD) {
    int64_t ans=1;
    for (int i=0;i!=y;++i) ans = ans*x % MOD;
    return ans;
}

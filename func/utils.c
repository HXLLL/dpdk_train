#include "main.h"

static const struct rte_eth_conf port_conf_default = {
	// .rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_LEN }
	.rxmode = { .mq_mode = ETH_MQ_RX_RSS , .max_rx_pkt_len = RTE_ETHER_MAX_LEN },
	.rx_adv_conf = { .rss_conf = NULL },
};

int port_init(uint8_t port, struct rte_mempool *mbuf_pool[], uint32_t nb_rx_queue, uint32_t nb_tx_queue)
{
	struct rte_eth_conf port_conf = port_conf_default;
	uint16_t nb_rxd = RX_RING_SIZE;
	uint16_t nb_txd = TX_RING_SIZE;
	int retval;
	uint16_t q;

	if (port >= rte_eth_dev_count_avail())
		return -1;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, nb_rx_queue, nb_tx_queue, &port_conf);
	if (retval != 0)
		return retval;

	retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < nb_rx_queue; q++) {
		retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool[0]);
		if (retval < 0)
			return retval;
	}

	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < nb_tx_queue; q++) {
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

struct ctrl_blk_t* ctrl_blk_init() {
	struct ctrl_blk_t* cb=(struct ctrl_blk_t*)malloc(sizeof(struct ctrl_blk_t));
	memset(cb, 0, sizeof(struct ctrl_blk_t));
	return cb;
}
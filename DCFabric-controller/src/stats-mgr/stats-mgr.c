/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
*                                                                             *
*   File Name   : stats-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "gnflush-types.h"
#include "gn_inet.h"
#include "stats-mgr.h"
#include "timer.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../conn-svr/conn-svr.h"
#include "../flow-mgr/flow-mgr.h"
#include <sys/prctl.h>   

UINT4 g_switch_bandwidth = 100000;  //100Mb/s
UINT4 g_stats_mgr_interval = 5;

static pthread_t g_stats_mgr_threadid = 0;
static UINT1 g_runing_flag = 0;
extern void of13_parse_match(struct ofpx_match *of_match, gn_match_t *gn_match);
//static UINT4 format_timestamp(UINT4 time)
//{
//    return time / g_stats_mgr_interval * g_stats_mgr_interval;
//}

static void of10_convert_port_info(struct ofp_port_stats *net_stats, struct ofp_port_stats *host_stats)
{
    host_stats->port_no = ntohs(net_stats->port_no);
    host_stats->rx_packets = gn_ntohll(net_stats->rx_packets);
    host_stats->tx_packets = gn_ntohll(net_stats->tx_packets);
    host_stats->rx_bytes = gn_ntohll(net_stats->rx_bytes);
    host_stats->tx_bytes = gn_ntohll(net_stats->tx_bytes);
    host_stats->rx_dropped = gn_ntohll(net_stats->rx_dropped);
    host_stats->tx_dropped = gn_ntohll(net_stats->tx_dropped);
    host_stats->rx_errors = gn_ntohll(net_stats->rx_errors);
    host_stats->tx_errors = gn_ntohll(net_stats->tx_errors);
    host_stats->rx_frame_err = gn_ntohll(net_stats->rx_frame_err);
    host_stats->rx_over_err = gn_ntohll(net_stats->rx_over_err);
    host_stats->rx_crc_err = gn_ntohll(net_stats->rx_crc_err);
    host_stats->collisions = gn_ntohll(net_stats->collisions);
}

static void of13_convert_port_info(struct ofp13_port_stats *net_stats, struct ofp13_port_stats *host_stats)
{
    host_stats->port_no = ntohl(net_stats->port_no);
    host_stats->rx_packets = gn_ntohll(net_stats->rx_packets);
    host_stats->tx_packets = gn_ntohll(net_stats->tx_packets);
    host_stats->rx_bytes = gn_ntohll(net_stats->rx_bytes);
    host_stats->tx_bytes = gn_ntohll(net_stats->tx_bytes);
    host_stats->rx_dropped = gn_ntohll(net_stats->rx_dropped);
    host_stats->tx_dropped = gn_ntohll(net_stats->tx_dropped);
    host_stats->rx_errors = gn_ntohll(net_stats->rx_errors);
    host_stats->tx_errors = gn_ntohll(net_stats->tx_errors);
    host_stats->rx_frame_err = gn_ntohll(net_stats->rx_frame_err);
    host_stats->rx_over_err = gn_ntohll(net_stats->rx_over_err);
    host_stats->rx_crc_err = gn_ntohll(net_stats->rx_crc_err);
    host_stats->collisions = gn_ntohll(net_stats->collisions);
    host_stats->duration_sec = ntohl(net_stats->duration_sec);
    host_stats->duration_nsec = ntohl(net_stats->duration_nsec);
}

static void update_flow_stats(flow_stats_t *stats, UINT8 byte_count, UINT8 packet_count, UINT4 duration_sec)
{
    if(( 0 == stats->timestamp) || (0 == duration_sec))
    {
        stats->timestamp = g_cur_sys_time.tv_sec;
        stats->byte_count = byte_count;
        stats->packet_count = packet_count;
        stats->duration_sec = duration_sec;
    }
    else
    {
        UINT4 time_intval = duration_sec - stats->duration_sec;
        if(0 >= time_intval)
        {
            return;
        }

        stats->timestamp = g_cur_sys_time.tv_sec;
        stats->kbps = (byte_count - stats->byte_count)/time_intval;
        stats->kpps = (packet_count - stats->packet_count)/time_intval;
        stats->byte_count = byte_count;
        stats->packet_count = packet_count;
        stats->duration_sec = duration_sec;
		
		LOG_PROC("OF13", "%s -- OFPMP_FLOW",FN);
		/*
        printf("   kbps: %d\n", stats->kbps);
        printf("   kpps: %d\n", stats->kpps);
        printf("   byte_count: %u\n", stats->byte_count);
        printf("   packet_count: %u\n", stats->packet_count);
        printf("   duration_sec: %d\n", stats->duration_sec);
        printf("   timestamp: %d\n", stats->timestamp);
		*/
		LOG_PROC("OF13", "%s -- OFPMP_FLOW",FN);
    }
}

void of10_proc_port_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts)
{
    struct ofp_port_stats *port_stats = (struct ofp_port_stats*)stats;
    struct ofp_port_stats new_stats;
    UINT4 timestamp = g_cur_sys_time.tv_sec;
    UINT4 duration_sec = 0;
    int idx_stats = 0, idx = 0;

    for(;idx_stats < counts; idx_stats++)
    {
        for(idx = 0; idx < sw->n_ports; idx++)
        {
            if(sw->ports[idx].port_no == ntohs(port_stats[idx_stats].port_no))
            {
                of10_convert_port_info(&(port_stats[idx_stats]), &new_stats);
                duration_sec = timestamp - sw->ports[idx].stats.timestamp;
                if(0 == duration_sec)
                {
                    return;
                }

                if(0 != sw->ports[idx].stats.timestamp)
                {
                    sw->ports[idx].stats.rx_kbps = (new_stats.rx_bytes - sw->ports[idx].stats.rx_bytes)/duration_sec;
                    sw->ports[idx].stats.tx_kbps = (new_stats.tx_bytes - sw->ports[idx].stats.tx_bytes)/duration_sec;
                    sw->ports[idx].stats.rx_kpps = (new_stats.rx_packets - sw->ports[idx].stats.rx_packets)/duration_sec;
                    sw->ports[idx].stats.tx_kpps = (new_stats.tx_packets - sw->ports[idx].stats.tx_packets)/duration_sec;
                }

                sw->ports[idx].stats.rx_packets = new_stats.rx_packets;
                sw->ports[idx].stats.tx_packets = new_stats.tx_packets;
                sw->ports[idx].stats.rx_bytes = new_stats.rx_bytes;
                sw->ports[idx].stats.tx_bytes = new_stats.tx_bytes;
                sw->ports[idx].stats.timestamp = timestamp;

//                printf("%s. Port :%s, rx_kbps: %d, tx_kbps: %d, rx_kpps: %d, tx_kpps: %d\n", sw->ports[idx].name,
//                        FN, sw->ports[idx].stats.rx_kbps, sw->ports[idx].stats.tx_kbps, sw->ports[idx].stats.rx_kpps, sw->ports[idx].stats.tx_kpps);
//
//                printf("  rx_packets: %d\n", new_stats.rx_packets);
//                printf("  tx_packets: %d\n", new_stats.tx_packets);
//                printf("  rx_bytes: %d\n", new_stats.rx_bytes);
//                printf("  tx_bytes: %d\n", new_stats.tx_bytes);
//                printf("  duration_sec: %d\n\n", duration_sec);
            }
        }
    }

//    printf("\n\n\n");
}

void of10_proc_flow_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts)
{
    struct ofp_flow_stats *flow_stats = (struct ofp_flow_stats*)stats;
    gn_flow_t *p_flow = NULL;
    gn_flow_t gn_flow;

    memset(&gn_flow, 0, sizeof(gn_flow_t));
    gn_flow.priority = ntohs(flow_stats->priority);
    gn_flow.table_id = flow_stats->table_id;
    sw->msg_driver.convertter->oxm_convertter((UINT1 *)&(flow_stats->match), &(gn_flow.match.oxm_fields));

    p_flow = find_flow_entry(sw, &gn_flow);
    if(p_flow)
    {
        update_flow_stats(&(p_flow->stats), gn_ntohll(flow_stats->byte_count),
                gn_ntohll(flow_stats->packet_count), ntohl(flow_stats->duration_sec));
    }
}
//by:yhy openflow1.3 处理端口状态
void of13_proc_port_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts)
{
    struct ofp13_port_stats *port_stats = (struct ofp13_port_stats*)stats;
    struct ofp13_port_stats new_stats;
    UINT4 timestamp = g_cur_sys_time.tv_sec;
    UINT4 duration_sec = 0;
    int idx_stats = 0, idx = 0;

    for(;idx_stats < counts; idx_stats++)
    {
        for(idx = 0; idx < sw->n_ports; idx++)
        {
            if(sw->ports[idx].port_no == ntohl(port_stats[idx_stats].port_no))
            {
                of13_convert_port_info(&(port_stats[idx_stats]), &new_stats);
                duration_sec = new_stats.duration_sec - sw->ports[idx].stats.duration_sec;
                if(0 == duration_sec)
                {
                    return;
                }

                if(0 != sw->ports[idx].stats.timestamp)
                {
                    sw->ports[idx].stats.rx_kbps = (new_stats.rx_bytes - sw->ports[idx].stats.rx_bytes)/duration_sec;
                    sw->ports[idx].stats.tx_kbps = (new_stats.tx_bytes - sw->ports[idx].stats.tx_bytes)/duration_sec;
                    sw->ports[idx].stats.rx_kpps = (new_stats.rx_packets - sw->ports[idx].stats.rx_packets)/duration_sec;
                    sw->ports[idx].stats.tx_kpps = (new_stats.tx_packets - sw->ports[idx].stats.tx_packets)/duration_sec;
                }

                sw->ports[idx].stats.rx_packets = new_stats.rx_packets;
                sw->ports[idx].stats.tx_packets = new_stats.tx_packets;
                sw->ports[idx].stats.rx_bytes = new_stats.rx_bytes;
                sw->ports[idx].stats.tx_bytes = new_stats.tx_bytes;
                sw->ports[idx].stats.duration_sec = new_stats.duration_sec;
                sw->ports[idx].stats.timestamp = timestamp;
				LOG_PROC("OF13", "%s -- OFPMP_PORT_STATS",FN);
				/*
                printf("%s. Port :%s, rx_kbps: %e, tx_kbps: %e, rx_kpps: %e, tx_kpps: %e\n", sw->ports[idx].name,
                        FN, sw->ports[idx].stats.rx_kbps, sw->ports[idx].stats.tx_kbps, sw->ports[idx].stats.rx_kpps, sw->ports[idx].stats.tx_kpps);

                printf("  rx_packets: %ld\n", new_stats.rx_packets);
                printf("  tx_packets: %ld\n", new_stats.tx_packets);
                printf("  rx_bytes: %ld\n", new_stats.rx_bytes);
                printf("  tx_bytes: %ld\n", new_stats.tx_bytes);
                printf("  duration_sec: %d\n", duration_sec);
				*/
				LOG_PROC("OF13", "%s -- OFPMP_PORT_STATS",FN);
            }
        }
    }

//    printf("\n\n\n");
}


gn_flow_t * find_flow_entry_gn(gn_switch_t *sw, gn_flow_t *flow)
{
    gn_flow_t *p_flow = sw->flow_entries;

    if(NULL == flow)
    {
        return NULL;
    }

    while(p_flow)
    {
        if(p_flow->table_id == flow->table_id && match_compare_strict(&(p_flow->match), &(flow->match)))
        {
            return p_flow;
        }

        p_flow = p_flow->next;
    }

    return NULL;
}

static UINT1 of13_oxm_convertter_gn(UINT1 *oxm, gn_oxm_t *gn_oxm)
{
    struct ofp_oxm_header *of_oxm = (struct ofp_oxm_header *)oxm;
    switch(of_oxm->oxm_field_hm >> 1)
    {
        case (OFPXMT_OFB_IN_PORT):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IN_PORT);
            gn_oxm->in_port = ntohl(*(UINT4 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_IN_PHY_PORT):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IN_PHY_PORT);
            gn_oxm->in_phy_port = ntohl(*(UINT4 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_METADATA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_METADATA);
            gn_oxm->metadata = gn_ntohll(*(UINT8 *)(of_oxm->data));
            if(of_oxm->length > 8)
            {
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_METADATA_MASK);
                gn_oxm->metadata_mask = gn_ntohll(*(UINT8 *)(of_oxm->data + 8));
            }
            break;
        }
        case (OFPXMT_OFB_ETH_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_DST);
            memcpy(gn_oxm->eth_dst, of_oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ETH_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_SRC);
            memcpy(gn_oxm->eth_src, of_oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ETH_TYPE):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_TYPE);
            gn_oxm->eth_type = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_VLAN_VID):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_VLAN_VID);
            gn_oxm->vlan_vid = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_VLAN_PCP):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_VLAN_PCP);
            gn_oxm->vlan_pcp = (*(UINT1 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_IP_DSCP):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IP_DSCP);
            gn_oxm->ip_dscp = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_IP_ECN):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IP_ECN);
            gn_oxm->ip_ecn = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_IP_PROTO):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IP_PROTO);
            gn_oxm->ip_proto = *(UINT1 *)(of_oxm->data);
            break;
        }
		/*
        case (OFPXMT_OFB_IPV4_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC);
            gn_oxm->ipv4_src = ntohl(*(UINT4 *)(of_oxm->data));

            if(of_oxm->length > 4)
            {
                UINT4 netmask = ntohl(*(UINT4 *)(of_oxm->data + 4));
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC_PREFIX);
                if(netmask == 0xff000000)
                {
                    gn_oxm->ipv4_src_prefix = 8;
                }
                else if(netmask == 0xffff0000)
                {
                    gn_oxm->ipv4_src_prefix = 16;
                }
                else if(netmask == 0xffffff00)
                {
                    gn_oxm->ipv4_src_prefix = 24;
                }
            }

            break;
        }
	*/
        case (OFPXMT_OFB_IPV4_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC);
            gn_oxm->ipv4_src = ntohl(*(UINT4 *)(of_oxm->data));
        }
		break;
		case (OFPXMT_OFB_IPV4_SRC_PREFIX):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC_PREFIX);
            gn_oxm->ipv4_src_prefix = ntohl(*(UINT4 *)(of_oxm->data));
        }
		break;
		/*
        case (OFPXMT_OFB_IPV4_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST);
            gn_oxm->ipv4_dst = ntohl(*(UINT4 *)(of_oxm->data));

            if(of_oxm->length > 4)
            {
                UINT4 netmask = ntohl(*(UINT4 *)(of_oxm->data + 4));
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST_PREFIX);
                if(netmask == 0xff000000)
                {
                    gn_oxm->ipv4_dst_prefix = 8;
                }
                else if(netmask == 0xffff0000)
                {
                    gn_oxm->ipv4_dst_prefix = 16;
                }
                else if(netmask == 0xffffff00)
                {
                    gn_oxm->ipv4_dst_prefix = 24;
                }
            }

            break;
        }
	*/
        case (OFPXMT_OFB_IPV4_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST);
            gn_oxm->ipv4_dst = ntohl(*(UINT4 *)(of_oxm->data));
        }
		break;
		case (OFPXMT_OFB_IPV4_DST_PREFIX):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST_PREFIX);
            gn_oxm->ipv4_dst_prefix = ntohl(*(UINT4 *)(of_oxm->data));
        }
		break;
        case (OFPXMT_OFB_TCP_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_TCP_SRC);
            gn_oxm->tcp_src = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_TCP_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_TCP_DST);
            gn_oxm->tcp_dst = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_UDP_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_UDP_SRC);
            gn_oxm->udp_src = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_UDP_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_UDP_DST);
            gn_oxm->udp_dst = ntohs(*(UINT2 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_SCTP_SRC):
        {
            //
            break;
        }
        case (OFPXMT_OFB_SCTP_DST):
        {
            //
            break;
        }
        case (OFPXMT_OFB_ICMPV4_TYPE):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ICMPV4_TYPE);
            gn_oxm->icmpv4_type = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_ICMPV4_CODE):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ICMPV4_CODE);
            gn_oxm->icmpv4_code = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_ARP_OP):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_OP);
            gn_oxm->arp_op = *(UINT1 *)(of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_ARP_SPA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_SPA);
            gn_oxm->arp_spa = ntohl(*(UINT4 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_ARP_TPA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_TPA);
            gn_oxm->arp_tpa = ntohl(*(UINT4 *)(of_oxm->data));
            break;
        }
        case (OFPXMT_OFB_ARP_SHA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_SHA);
            memcpy(gn_oxm->arp_sha, of_oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ARP_THA):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ARP_THA);
            memcpy(gn_oxm->arp_tha, of_oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_IPV6_SRC):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV6_SRC);
            memcpy(gn_oxm->ipv6_src, of_oxm->data, OFPXMT_OFB_IPV6_SZ);

            if(of_oxm->length > OFPXMT_OFB_IPV6_SZ)
            {
//                UINT4 mask[4] = {0};
//                mask[0] = ntohl(*(UINT4 *)(of_oxm->data + 4));
//                mask[1] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4));
//                mask[2] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4 + 4));
//                mask[3] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4 + 4 + 4));
//                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV6_SRC_PREFIX);
//
//                //todo
            }
            break;
        }
        case (OFPXMT_OFB_IPV6_DST):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV6_DST);
            memcpy(gn_oxm->ipv6_dst, of_oxm->data, OFPXMT_OFB_IPV6_SZ);

            if(of_oxm->length > OFPXMT_OFB_IPV6_SZ)
            {
//                UINT4 mask[4] = {0};
//                mask[0] = ntohl(*(UINT4 *)(of_oxm->data + 4));
//                mask[1] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4));
//                mask[2] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4 + 4));
//                mask[3] = ntohl(*(UINT4 *)(of_oxm->data + 4 + 4 + 4 + 4));
//                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV6_DST_PREFIX);
//
//                //todo
            }
            break;
        }
        case (OFPXMT_OFB_IPV6_FLABEL):
        {
            break;
        }
        case (OFPXMT_OFB_ICMPV6_TYPE):
        {
            break;
        }
        case (OFPXMT_OFB_ICMPV6_CODE):
        {
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_TARGET):
        {
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_SLL):
        {
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_TLL):
        {
            break;
        }
        case (OFPXMT_OFB_MPLS_LABEL):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_MPLS_LABEL);
            gn_oxm->mpls_label = ntohl(*(UINT4 *)of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_MPLS_TC):
        {
            break;
        }
        case (OFPXMT_OFP_MPLS_BOS):
        {
            break;
        }
        case (OFPXMT_OFB_PBB_ISID):
        {
            break;
        }
        case (OFPXMT_OFB_TUNNEL_ID):
        {
            gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_TUNNEL_ID);
            gn_oxm->tunnel_id = ntohl(*(UINT4 *)of_oxm->data);
            break;
        }
        case (OFPXMT_OFB_IPV6_EXTHDR):
        {
            break;
        }
        default:break;
    }

    return of_oxm->length;
}

static void of13_parse_match_gn(struct ofpx_match *of_match, gn_match_t *gn_match)
{
    UINT2 oxm_tot_len = ntohs(of_match->length);
    UINT2 oxm_len = 4;
    UINT1 field_len = 0;
    UINT1 *oxm = of_match->oxm_fields;

    memset(gn_match, 0, sizeof(gn_match_t));
    while(ALIGN_8(oxm_len) < oxm_tot_len)
    {
        field_len = of13_oxm_convertter_gn((UINT1 *)oxm, &(gn_match->oxm_fields));
        oxm += sizeof(struct ofp_oxm_header) + field_len;
        oxm_len += sizeof(struct ofp_oxm_header) + field_len;
    }
}



//by:yhy 处理流表状态
void of13_proc_flow_stats(gn_switch_t *sw, UINT1 *stats, UINT2 length)
{
    
    UINT2 offset = 0;                       
    gn_flow_t *p_flow = NULL;
	gn_flow_t gn_flow = {0};

    struct ofp13_flow_stats *flow_stats = (struct ofp13_flow_stats*)stats;
    UINT1 *p_stats = stats;
    offset = length;
    while(offset)
    {
        memset(&gn_flow, 0x00, sizeof(gn_flow_t));
        gn_flow.priority = ntohs(flow_stats->priority);
        gn_flow.table_id = flow_stats->table_id;
        of13_parse_match_gn(&(flow_stats->match), &gn_flow.match);

        p_flow = find_flow_entry_gn(sw, &gn_flow);
        if(p_flow)
        {
            update_flow_stats(&(p_flow->stats), gn_ntohll(flow_stats->byte_count), gn_ntohll(flow_stats->packet_count), ntohl(flow_stats->duration_sec));
        }

        offset -= ntohs(flow_stats->length);
        p_stats += ntohs(flow_stats->length);
        flow_stats = (struct ofp13_flow_stats *)p_stats;
    }
}

//by:yhy openflow  发送用于获取switch的端口状态信息的请求
static void of_send_port_stats(gn_switch_t *sw)
{
    stats_req_info_t stats_req_info;
    port_stats_req_data_t port_stats_req_data;

    stats_req_info.flags = 0;
    stats_req_info.xid = 0;
    stats_req_info.data = (UINT1 *)&port_stats_req_data;

    if(sw->ofp_version == OFP10_VERSION)
    {
        stats_req_info.type = OFPST_PORT;
        port_stats_req_data.port_no = 0xffffffff;
        sw->msg_driver.msg_handler[OFPT_STATS_REQUEST](sw, (UINT1 *)&stats_req_info);
    }
    else if (sw->ofp_version == OFP13_VERSION)
    {
        stats_req_info.type = OFPMP_PORT_STATS;
        port_stats_req_data.port_no = 0xffffffff;
        sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&stats_req_info);
    }
}
//by:yhy openflow 发送用于获取switch当前流表状态信息的请求
static void of_send_flow_stats(gn_switch_t *sw)
{
    stats_req_info_t stats_req_info;
    flow_stats_req_data_t flow_stats_req_data;

    stats_req_info.flags = 0;
    stats_req_info.xid = 0;
    stats_req_info.data = (UINT1 *)&flow_stats_req_data;

    if(sw->ofp_version == OFP10_VERSION)
    {
        stats_req_info.type = OFPST_FLOW;
        flow_stats_req_data.out_port = OFPP_NONE;
        flow_stats_req_data.table_id = OFPTT_ALL;
        sw->msg_driver.msg_handler[OFPT_STATS_REQUEST](sw, (UINT1 *)&stats_req_info);
    }
    else if (sw->ofp_version == OFP13_VERSION)
    {
        stats_req_info.type = OFPMP_FLOW;
        flow_stats_req_data.out_port = OFPP13_ANY;
        flow_stats_req_data.out_group = OFPG_ANY;
        flow_stats_req_data.table_id = OFPTT_ALL;
        sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&stats_req_info);
    }
}

//by:yhy 吞吐率检测
void *get_throughput()
{
    gn_switch_t *sw = NULL;
    UINT4 num = 0;
  
    if(g_runing_flag)
    {
		
		for(num=0; num < g_server.max_switch; num++)
		{
			sw = &g_server.switches[num];
			if(CONNECTED == sw->conn_state)
			{
				of_send_port_stats(sw);
				//of_send_flow_stats(sw);
			}
		}
		
    }

    return NULL;
}


//by:yhy 周期性发送用于获取switch端口状态/流表信息的请求包
INT4 init_stats_mgr()
{   
	char *val = NULL;

    val = get_value(g_controller_configure, "[stats_conf]", "sampling_interval");
    g_stats_mgr_interval = (NULL == val  ? 5 : atoi(val));
	g_runing_flag = 1;
    //pthread_create(&g_stats_mgr_threadid, NULL, get_throughput, NULL);
    return GN_OK;
}

//
void fini_stats_mgr()
{
    g_runing_flag = 0;
}

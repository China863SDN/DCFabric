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

#include "stats-mgr.h"
#include "timer.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../conn-svr/conn-svr.h"
#include "../flow-mgr/flow-mgr.h"

UINT4 g_switch_bandwidth = 100000;  //100Mb/s
UINT4 g_stats_mgr_interval = 5;

static pthread_t g_stats_mgr_threadid = 0;
static UINT1 g_runing_flag = 0;

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

//        printf("   kbps: %d\n", stats->kbps);
//        printf("   kpps: %d\n", stats->kpps);
//        printf("   byte_count: %d\n", stats->byte_count);
//        printf("   packet_count: %d\n", stats->packet_count);
//        printf("   duration_sec: %d\n", stats->duration_sec);
//        printf("   timestamp: %d\n", stats->timestamp);
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

void of13_proc_flow_stats(gn_switch_t *sw, UINT1 *stats, UINT2 length)
{
//    struct ofp13_flow_stats *flow_stats = (struct ofp13_flow_stats*)stats;
//    struct ofp13_flow_stats *p_stats = flow_stats;
//
//    UINT2 offset = 0;                       //记录所有flow_stats处理偏移
//    UINT2 cur_offset = 0;                   //记录当前flow_stats处理偏移
//    UINT2 stats_len = 0;
//    gn_flow_t *p_flow = NULL;
//    struct ofp_oxm_header *oxm = NULL;
//    UINT2 oxm_tot_len = 0;
//    UINT2 oxm_len = 4;
//
//    while(offset != length)
//    {
//        cur_offset = 0;
//        stats_len = ntohs(p_stats->length);
//
//        p_flow = (gn_flow_t *)mem_get(g_gnflow_mempool_id);
//        memset(p_flow, 0, sizeof(gn_flow_t));
//        oxm_tot_len = ntohs(p_stats->match.length);
//        oxm = (struct ofp_oxm_header *)(flow_stats->match.oxm_fields);
//
//        strncpy(p_flow->creater, "controller", strlen("controller") + 1);
//        p_flow->priority = ntohs(p_stats->priority);
//        p_flow->table_id = p_stats->table_id;
//        p_flow->idle_timeout = ntohs(p_stats->idle_timeout);
//        p_flow->hard_timeout = ntohs(p_stats->hard_timeout);
//        p_flow->match.type = OFPMT_OXM;
//
//        while(ALIGN_8(oxm_len) < oxm_tot_len)
//        {
//            sw->msg_driver.convertter->oxm_convertter((UINT1 *)oxm, &(p_flow->match.oxm_fields));
//            oxm += sizeof(struct ofp_oxm_header) + oxm->length;
//            oxm_len += sizeof(struct ofp_oxm_header) + oxm->length;
//        }
//
//        cur_offset += sizeof(struct ofp13_flow_stats);
//        struct ofp_instruction *instructions = p_stats + ALIGN_8(sizeof(struct ofp13_flow_stats));
//
//
//        p_flow = add_flow_entry(sw, &p_flow);
//        if(p_flow)
//        {
//            update_flow_stats(&(p_flow->stats), gn_ntohll(p_stats->byte_count),
//                    gn_ntohll(p_stats->packet_count), ntohl(p_stats->duration_sec));
//        }
//    }


    struct ofp13_flow_stats *flow_stats = (struct ofp13_flow_stats*)stats;
    gn_flow_t *p_flow = NULL;
    gn_flow_t gn_flow;
//    UINT2 offset = 0;
    UINT2 oxm_tot_len = ntohs(flow_stats->match.length);
    UINT2 oxm_len = 4;
    struct ofp_oxm_header *oxm = (struct ofp_oxm_header *)(flow_stats->match.oxm_fields);

    memset(&gn_flow, 0, sizeof(gn_flow_t));
    gn_flow.priority = ntohs(flow_stats->priority);
    gn_flow.table_id = flow_stats->table_id;
    while(ALIGN_8(oxm_len) < oxm_tot_len)
    {
        sw->msg_driver.convertter->oxm_convertter((UINT1 *)oxm, &(gn_flow.match.oxm_fields));
        oxm += sizeof(struct ofp_oxm_header) + oxm->length;
        oxm_len += sizeof(struct ofp_oxm_header) + oxm->length;
    }

    p_flow = find_flow_entry(sw, &gn_flow);
    if(p_flow)
    {
        update_flow_stats(&(p_flow->stats), gn_ntohll(flow_stats->byte_count),
                gn_ntohll(flow_stats->packet_count), ntohl(flow_stats->duration_sec));
    }
}

static void of_send_port_stats(gn_switch_t *sw)
{
    stats_req_info_t stats_req_info;
    port_stats_req_data_t port_stats_req_data;

    stats_req_info.flags = 0;
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

static void of_send_flow_stats(gn_switch_t *sw)
{
    stats_req_info_t stats_req_info;
    flow_stats_req_data_t flow_stats_req_data;

    stats_req_info.flags = 0;
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

//定时获取所有交换机所有的所有流表/网口统计信息
void *get_throughput(void *args)
{
    gn_switch_t *sw = NULL;
    UINT4 num = 0;

    while(g_runing_flag)
    {
        sleep(g_stats_mgr_interval);

        for(num=0; num < g_server.max_switch; num++)
        {
            if(g_server.switches[num].state)
            {
                sw = &g_server.switches[num];
                if(sw->state == 1)
                {
                    of_send_port_stats(sw);
                    of_send_flow_stats(sw);
                }
            }
        }
    }

    return NULL;
}


//初始化
INT4 init_stats_mgr()
{
    g_runing_flag = 1;
    pthread_create(&g_stats_mgr_threadid, NULL, get_throughput, NULL);

    return GN_OK;
}

//反初始化
void fini_stats_mgr()
{
    g_runing_flag = 0;
}

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
*   File Name   : l3.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "l3.h"
#include "forward-mgr.h"
#include "timer.h"
#include "../topo-mgr/topo-mgr.h"
#include "../conn-svr/conn-svr.h"
#include "../flow-mgr/flow-mgr.h"
#include "gn_inet.h"
#include "mod-types.h"
#include "openflow-10.h"
#include "openflow-13.h"

const UINT1 brodcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const UINT1 zero_mac[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
subnet_t g_subnet_info[MAX_L3_SUBNET];

static UINT2 g_l3_flow_entry_idle_time = 0;         //网络字节序
static UINT2 g_l3_flow_entry_hard_time = 0;         //网络字节序

typedef struct l3_send_buf
{
    UINT4 dst_ip;
    UINT4 buffer_id;
    gn_switch_t *sw;
    UINT8 time_smp;
    UINT1 *pkt_data;
    UINT4 pkt_len;
    struct l3_send_buf *pre;
    struct l3_send_buf *next;
}l3_send_buf_t;

static l3_send_buf_t *g_l3_first_packets = NULL;
static pthread_t g_timeout_thread_id;
static pthread_mutex_t g_timeout_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

//新纪录放到最后面
static inline void queue_in_packet(l3_send_buf_t *new_packet)
{
    l3_send_buf_t *tmp = NULL;

    pthread_mutex_lock(&g_timeout_thread_mutex);
    if(NULL == g_l3_first_packets)
    {
        g_l3_first_packets = new_packet;
        pthread_mutex_unlock(&g_timeout_thread_mutex);
        return;
    }

    tmp = g_l3_first_packets;
    while(tmp->next)
    {
        tmp = tmp->next;
    }
    tmp->next = new_packet;
    new_packet->pre = tmp;

    g_l3_first_packets = new_packet;
    pthread_mutex_unlock(&g_timeout_thread_mutex);
}

//每次取最前面一条记录
static l3_send_buf_t * queue_out_packet(UINT4 dst_ip)
{
    l3_send_buf_t *p_buf = g_l3_first_packets;

    pthread_mutex_lock(&g_timeout_thread_mutex);
    while(p_buf)
    {
        if(p_buf->dst_ip == dst_ip)
        {
            if(p_buf == g_l3_first_packets)
            {
                g_l3_first_packets = p_buf->next;
                if(g_l3_first_packets)
                {
                    g_l3_first_packets->pre = NULL;
                }
            }
            else
            {
                p_buf->pre->next = p_buf->next;
                p_buf->next->pre = p_buf->pre;
            }

            pthread_mutex_unlock(&g_timeout_thread_mutex);
            return p_buf;
        }

        p_buf = p_buf->next;
    }

    pthread_mutex_unlock(&g_timeout_thread_mutex);
    return NULL;
}

//每2s清空一次
void *timeout_packets()
{
    l3_send_buf_t *p_buf = NULL;
    l3_send_buf_t *tmp = NULL;

    while(1)
    {
        pthread_mutex_lock(&g_timeout_thread_mutex);
        p_buf = g_l3_first_packets;
        while(p_buf)
        {
            if(g_cur_sys_time.tv_sec - p_buf->time_smp >= 2)
            {
                if(p_buf == g_l3_first_packets)
                {
                    g_l3_first_packets = p_buf->next;
                    if(g_l3_first_packets)
                    {
                        g_l3_first_packets->pre = NULL;
                    }
                }
                else
                {
                    p_buf->pre->next = p_buf->next;
                    p_buf->next->pre = p_buf->pre;
                }

                tmp = p_buf->next;
                free(p_buf);
                p_buf = tmp;
                continue;
            }

            p_buf = p_buf->next;
        }

        pthread_mutex_unlock(&g_timeout_thread_mutex);
        sleep(2);
    }
}

void l3_forward_first_packet(gn_switch_t *sw, UINT4 dst_ip, UINT1 *dst_mac, UINT4 outport)
{
    packout_req_info_t packout_req_info;
    ether_t *p_ether = NULL;
    l3_send_buf_t *p_buf = queue_out_packet(dst_ip);
    if(NULL == p_buf)
    {
        return;
    }

    if(NO_PATH == g_short_path[p_buf->sw->index][sw->index])
    {
        goto EXIT;
    }

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = 0xfffffffd;
    packout_req_info.outport = outport;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = 0;
    packout_req_info.data_len = p_buf->pkt_len;
    packout_req_info.data = p_buf->pkt_data;

    p_ether = (ether_t *)packout_req_info.data;
    memcpy(p_ether->dest, dst_mac, 6);

    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
    }

EXIT:
    free(p_buf->pkt_data);
    free(p_buf);
}

void arp_request(gn_switch_t *sw, UINT4 src_ip, UINT4 dst_ip, UINT4 outport)
{
    printf("%s\n", FN);
    packout_req_info_t packout_req_info;
    arp_t new_arp_pkt;

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = 0xfffffffd;
    packout_req_info.outport = outport;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = 0;
    packout_req_info.data_len = sizeof(arp_t);
    packout_req_info.data = (UINT1 *)&new_arp_pkt;

    memcpy(new_arp_pkt.eth_head.src, g_controller_mac, 6);
    memcpy(new_arp_pkt.eth_head.dest, brodcast_mac, 6);
    new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
    new_arp_pkt.hardwaretype = htons(1);
    new_arp_pkt.prototype = htons(ETHER_IP);
    new_arp_pkt.hardwaresize = 0x6;
    new_arp_pkt.protocolsize = 0x4;
    new_arp_pkt.opcode = htons(1);
    new_arp_pkt.sendip = htonl(src_ip);
    new_arp_pkt.targetip = htonl(dst_ip);
    memcpy(new_arp_pkt.sendmac, g_controller_mac, 6);
    memcpy(new_arp_pkt.targetmac, zero_mac, 6);

    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
    }
}

void arp_reply(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    packout_req_info_t packout_req_info;
    arp_t *p_arp_req = (arp_t *)(packet_in_info->data);
    arp_t arp_rep;

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = 0xfffffffd;
    packout_req_info.outport = packet_in_info->inport;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = packet_in_info->xid;
    packout_req_info.data_len = sizeof(arp_t);
    packout_req_info.data = (UINT1 *)&arp_rep;

    memcpy(&arp_rep, p_arp_req, sizeof(arp_t));
    memcpy(arp_rep.eth_head.src, g_controller_mac, 6);
    memcpy(arp_rep.eth_head.dest, p_arp_req->eth_head.src, 6);
    arp_rep.eth_head.proto = htons(ETHER_ARP);
    arp_rep.opcode = htons(2);
    arp_rep.sendip = p_arp_req->targetip;
    arp_rep.targetip = p_arp_req->sendip;
    memcpy(arp_rep.sendmac, g_controller_mac, 6);
    memcpy(arp_rep.targetmac, p_arp_req->sendmac, 6);

    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
    }
}

static INT4 l3_install_flow_local(gn_switch_t *sw, UINT4 gw_ip, UINT4 inport, ether_t *ether)
{
    printf("%s\n", FN);
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t *flow = NULL;
    gn_instruction_actions_t *instruction = NULL;
    gn_action_output_t *action_outport = NULL;
    gn_action_set_field_t *action_set_field = NULL;
    INT4 ret = 0;
    UINT4 src_ip = 0;
    UINT4 dst_ip = 0;
//    UINT1 dpid[8];
//    ulli64_to_uc8(sw->dpid, dpid);
//    printf("Flow mod to local sw[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n", dpid[0], dpid[1],
//            dpid[2] , dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);
    if(ether->proto == htons(ETHER_ARP))
    {
        arp_t *p_arp = (arp_t *)(ether);
        src_ip = ntohl(p_arp->sendip);
        dst_ip = ntohl(p_arp->targetip);
    }
    else if(ether->proto == htons(ETHER_IP))
    {
        ip_t *p_ip = (ip_t *)(ether);
        src_ip = ntohl(p_ip->src);
        dst_ip = ntohl(p_ip->dest);
    }
    else
    {
        return GN_ERR;
    }

    memset(&sw->flowmod_helper, 0, sizeof(gn_flowmod_helper_t));
//    flow = (gn_flow_t *)gn_malloc(sizeof(gn_flow_t));
    flow = &sw->flowmod_helper.flow;
    strncpy(flow->creater, FLOW_L3_CREATER, sizeof(FLOW_L3_CREATER));
    flow->create_time = g_cur_sys_time.tv_sec;
    flow->table_id = 0;
    flow->idle_timeout = g_l3_flow_entry_idle_time;
    flow->hard_timeout = g_l3_flow_entry_hard_time;
    flow->priority = 1;
    flow->match.type = OFPMT_OXM;

    flow->match.oxm_fields.ipv4_dst = src_ip;
    flow->match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);

    flow->match.oxm_fields.eth_type = ETHER_IP;
    flow->match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

//    instruction = (gn_instruction_actions_t *)gn_malloc(sizeof(gn_instruction_actions_t));
    instruction = &sw->flowmod_helper.instruction;
    instruction->type = OFPIT_APPLY_ACTIONS;
    instruction->next = flow->instructions;
    flow->instructions = (gn_instruction_t *)instruction;

//    action_outport = (gn_action_output_t *)gn_malloc(sizeof(gn_action_output_t));
    action_outport = &sw->flowmod_helper.action_output;
    action_outport->type = OFPAT13_OUTPUT;
    action_outport->port = inport;
    action_outport->next = NULL;

//    action_set_field = (gn_action_set_field_t *)gn_malloc(sizeof(gn_action_set_field_t));
    action_set_field = &sw->flowmod_helper.action_set_field;
    action_set_field->type = OFPAT13_SET_FIELD;
    memcpy(action_set_field->oxm_fields.eth_dst, ether->src, 6);
    action_set_field->oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
    action_set_field->next = (gn_action_t *)action_outport;
    instruction->actions = (gn_action_t *)action_set_field;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = flow;

//    add_flow_entry(sw, flow);
    if(sw->ofp_version == OFP10_VERSION)
    {
        ret = sw->msg_driver.msg_handler[OFPT_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    }
    else
    {
        ret = sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    }

    //arp查找目的IP
    if(gw_ip)
    {
        UINT4 port_idx = 0;
        for(port_idx = 0; port_idx < sw->n_ports; port_idx++)
        {
           if(!(sw->neighbor[port_idx]))            //从非SW互联端口寻找
           {
               arp_request(sw, ntohl(gw_ip), dst_ip, sw->ports[port_idx].port_no);
           }
        }
    }

    return ret;
}

static INT4 l3_install_flow(gn_switch_t *sw, UINT4 gw_ip, UINT4 inport, ether_t *ether)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t *flow = NULL;
    gn_instruction_actions_t *instruction = NULL;
    gn_action_output_t *action_outport = NULL;
    INT4 ret = 0;
    UINT4 src_ip = 0;
    UINT4 dst_ip = 0;

    if(ether->proto == htons(ETHER_ARP))
    {
        arp_t *p_arp = (arp_t *)(ether);
        src_ip = ntohl(p_arp->sendip);
        dst_ip = ntohl(p_arp->targetip);
    }
    else if(ether->proto == htons(ETHER_IP))
    {
        ip_t *p_ip = (ip_t *)(ether);
        src_ip = ntohl(p_ip->src);
        dst_ip = ntohl(p_ip->dest);
    }
    else
    {
        return GN_ERR;
    }

    memset(&sw->flowmod_helper, 0, sizeof(gn_flowmod_helper_t));
//    flow = (gn_flow_t *)gn_malloc(sizeof(gn_flow_t));
    flow = &sw->flowmod_helper.flow;
    strncpy(flow->creater, FLOW_L3_CREATER, sizeof(FLOW_L3_CREATER));
    flow->create_time = g_cur_sys_time.tv_sec;
    flow->table_id = 0;
    flow->idle_timeout = g_l3_flow_entry_idle_time;
    flow->hard_timeout = g_l3_flow_entry_hard_time;
    flow->priority = 1;
    flow->match.type = OFPMT_OXM;

    flow->match.oxm_fields.ipv4_dst = src_ip;
    flow->match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);

    flow->match.oxm_fields.eth_type = ETHER_IP;
    flow->match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

//    instruction = (gn_instruction_actions_t *)gn_malloc(sizeof(gn_instruction_actions_t));
    instruction = &sw->flowmod_helper.instruction;
    instruction->type = OFPIT_APPLY_ACTIONS;
    instruction->next = flow->instructions;
    flow->instructions = (gn_instruction_t *)instruction;

//    action_outport = (gn_action_output_t *)gn_malloc(sizeof(gn_action_output_t));
    action_outport = &sw->flowmod_helper.action_output;
    action_outport->port = inport;
    action_outport->next = instruction->actions;
    instruction->actions = (gn_action_t *)action_outport;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = flow;

//    add_flow_entry(sw, flow);
    if(sw->ofp_version == OFP10_VERSION)
    {
        ret = sw->msg_driver.msg_handler[OFPT_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    }
    else
    {
        ret = sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    }

    //arp查找目的IP
    if(gw_ip)
    {
        UINT4 port_idx = 0;

        for(port_idx = 0; port_idx < sw->n_ports; port_idx++)
        {
           if(!(sw->neighbor[port_idx]))            //从非SW互联端口寻找
           {
               arp_request(sw, ntohl(gw_ip), dst_ip, sw->ports[port_idx].port_no);
           }
        }
    }

    return ret;
}

void l3_flowmod_chain(UINT4 gw_ip, UINT8 src_topo_id, UINT8 dst_topo_id, UINT4 inport, ether_t *ether)
{
//    UINT1 dpid[8];
    gn_switch_t *sw_pre = NULL;
    UINT4 port_pre = 0;
    INT4 id_tmp = 0;

    if((src_topo_id < 0) || (dst_topo_id < 0))
    {
        goto END;
    }

    id_tmp = g_short_path[src_topo_id][dst_topo_id];
    if(NO_PATH == id_tmp)
    {
        goto END;
    }

    id_tmp = g_short_path[dst_topo_id][src_topo_id];
    if(NO_PATH == id_tmp)
    {
        goto END;
    }

    id_tmp = src_topo_id;
    do
    {
        src_topo_id = id_tmp;
        id_tmp = g_short_path[src_topo_id][dst_topo_id];
        if(NO_PATH == id_tmp)
        {
            goto END;
        }

        //当前交换机
        sw_pre = g_adac_matrix.sw[src_topo_id][id_tmp];
        if((NULL == sw_pre) || (0 == sw_pre->state))
        {
            goto END;
        }

        //当前交换机出口
        port_pre = g_adac_matrix.src_port[src_topo_id][id_tmp];

//        //show source switch info
//        ulli64_to_uc8(sw_pre->dpid, dpid);
//        printf("Flow mod to sw[%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n", dpid[0], dpid[1],
//                dpid[2] , dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);

        //给相邻交换机下发流表
        l3_install_flow(sw_pre, gw_ip, port_pre, ether);

    }while (dst_topo_id != g_short_path[src_topo_id][dst_topo_id]);

END:
    return;
}

void l3_proc(gn_switch_t *sw, packet_in_info_t *packet_in_info, UINT4 gw_ip)
{
    UINT4 j = 0;
    ether_t *ether = (ether_t *)packet_in_info->data;

    if(ether->proto == htons(ETHER_IP))
    {
        l3_send_buf_t *new_packet = (l3_send_buf_t *)gn_malloc(sizeof(l3_send_buf_t));
        new_packet->dst_ip = ((ip_t *)ether)->dest;
        new_packet->buffer_id = packet_in_info->buffer_id;
        new_packet->pkt_len = packet_in_info->data_len;
        new_packet->pkt_data = (UINT1 *)gn_malloc(new_packet->pkt_len);
        if(new_packet->pkt_data)
        {
            memcpy(new_packet->pkt_data, ether, new_packet->pkt_len);
        }
        new_packet->sw = sw;
        new_packet->time_smp = g_cur_sys_time.tv_sec;
        queue_in_packet(new_packet);
    }
    else if(ether->proto == htons(ETHER_ARP))
    {
        arp_t *arp = (arp_t *)packet_in_info->data;

        //发送缓存的首包并检查租户
        l3_forward_first_packet(sw, arp->sendip, arp->sendmac, packet_in_info->inport);
    }

    l3_install_flow_local(sw, gw_ip, packet_in_info->inport, ether);

    for(j = 0; j < g_server.max_switch; j++)
    {
        if(0 > sw->index)
        {
            return;
        }

        if (g_short_weight[j][sw->index] == 0)
        {
            continue;
        }

        l3_flowmod_chain(gw_ip, j, sw->index, packet_in_info->inport, ether);
    }
}

UINT4 find_gateway_ip(UINT4 ip)
{
    UINT1 i;
    UINT4 ip_h = ntohl(ip);
    UINT4 gw_ip = 0;

    for(i = 0; i < MAX_L3_SUBNET; i++)
    {
        //判断目的ip属于哪个网段,用哪个网关去请求目的ip的网关的MAC
        if((ip_h >= ntohl(g_subnet_info[i].gw_minip)) && (ip_h <= ntohl(g_subnet_info[i].gw_maxip)))
        {
            gw_ip = g_subnet_info[i].gw_ip;
            return gw_ip;
        }
    }

    return gw_ip;
}

subnet_t *search_l3_subnet(UINT4 ip)
{
    UINT1 i;

    for(i = 0; i < MAX_L3_SUBNET; i++)
    {
        if((g_subnet_info[i].is_using) && (ip == g_subnet_info[i].gw_ip)
                && (ip >= g_subnet_info[i].gw_minip) && (ip <= g_subnet_info[i].gw_maxip))
        {
            return &g_subnet_info[i];
        }
    }

    return NULL;
}

INT4 create_l3_subnet(INT1 *name, INT1 *masked_ip)
{
    net_mask_t net_mask;
    UINT1 i;

    masked_ip_parser(masked_ip, &net_mask);
    if((0 == net_mask.prefix) || (0 == net_mask.ip))
    {
        return GN_ERR;
    }

    if(search_l3_subnet(net_mask.ip))
    {
        return GN_ERR;
    }

    for(i = 0; i < MAX_L3_SUBNET; i++)
    {
        if(g_subnet_info[i].is_using == FALSE)
        {
            strncpy(g_subnet_info[i].name, name, 64 - 1);
            strncpy(g_subnet_info[i].netmask, masked_ip, 16 - 1);
            g_subnet_info[i].gw_ip = net_mask.ip;
            g_subnet_info[i].gw_prefix = net_mask.prefix;
            g_subnet_info[i].gw_minip = net_mask.minip;
            g_subnet_info[i].gw_maxip = net_mask.maxip;
            g_subnet_info[i].is_using = TRUE;
            return GN_OK;
        }
    }

    return GN_ERR;
}

INT4 destory_l3_subnet(INT1 *masked_ip)
{
    UINT1 i;
    net_mask_t net_mask;

    masked_ip_parser(masked_ip, &net_mask);
    for(i = 0; i < MAX_L3_SUBNET; i++)
    {
        if((FALSE == g_subnet_info[i].is_using) && (net_mask.ip == g_subnet_info[i].gw_ip)
                && (net_mask.ip >= g_subnet_info[i].gw_minip) && (net_mask.ip <= g_subnet_info[i].gw_minip))
        {
            g_subnet_info[i].is_using = FALSE;
        }
    }

    return GN_OK;
}

INT4 init_l3()
{
    INT1 *value = NULL;
    UINT4 i;

    value = get_value(g_controller_configure, "[controller]", "l3_flow_entry_idle_time");
    g_l3_flow_entry_idle_time = (NULL == value) ? 200 : atoi(value);

    value = get_value(g_controller_configure, "[controller]", "l3_flow_entry_hard_time");
    g_l3_flow_entry_hard_time = (NULL == value) ? 200 : atoi(value);

    for(i = 0; i < MAX_L3_SUBNET; i++)
    {
        g_subnet_info[i].is_using = FALSE; //未使用
    }

    if(pthread_create(&g_timeout_thread_id, NULL, timeout_packets, NULL) != 0)
    {
        printf("L3 module init failed!\n");
    }

    return GN_OK;
}

void fini_l3()
{
    return;
}

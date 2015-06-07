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
*   File Name   : forward-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "forward-mgr.h"
#include "l2.h"
#include "l3.h"
#include "fabric_arp.h"
#include "fabric_impl.h"
#include "../topo-mgr/topo-mgr.h"
#include "../user-mgr/user-mgr.h"
#include "../tenant-mgr/tenant-mgr.h"
#include "gn_inet.h"
#include "openflow-10.h"
#include "openflow-13.h"

forward_handler_t g_default_forward_handler;

static INT4 arp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    arp_t *arp = (arp_t *)(packet_in_info->data);
    UINT4 host_ip = ntohl(arp->sendip);
    mac_user_t *p_user_dst = NULL;

    if(get_fabric_state()){
		fabric_arp_handle(sw, packet_in_info);
		return GN_OK;
    }
    //记录源MAC到MAC端口表中
    mac_user_t *p_user_src = search_mac_user(arp->eth_head.src);
    if (NULL == p_user_src)
    {
        p_user_src = create_mac_user(sw, arp->eth_head.src, packet_in_info->inport, host_ip, NULL);
    }
    else
    {
        if(p_user_src->sw == sw)
        {
            if(p_user_src->port != packet_in_info->inport)  //如果入口和之前记录的入口不一样，说明风暴了，不再转发
            {
                return GN_ERR;
            }
        }
        else
        {
            if(g_adac_matrix.src_port[sw->index][p_user_src->sw->index] != packet_in_info->inport)  //入口非最优入口，说明是环路多余报文，不再转发
            {
                return GN_ERR;
            }
        }
        
        if(p_user_src->ipv4 != host_ip)
        {
            p_user_src->ipv4 = host_ip;
        }
    }

    if(arp->opcode == htons(1))         //arp request
    {
        if (arp->targetip == 0)
        {
            return GN_ERR;
        }

        if (search_l3_subnet(arp->targetip))                      //请求的是网关的MAC
        {
            packout_req_info_t packout_req_info;
            arp_t new_arp_pkt;

            packout_req_info.buffer_id = 0xffffffff;
            packout_req_info.inport = 0xfffffffd;
            packout_req_info.outport = packet_in_info->inport;
            packout_req_info.max_len = 0xff;
            packout_req_info.xid = packet_in_info->xid;
            packout_req_info.data_len = sizeof(arp_t);
            packout_req_info.data = (UINT1 *)&new_arp_pkt;

            memcpy(&new_arp_pkt, arp, sizeof(arp_t));
            memcpy(new_arp_pkt.eth_head.src, g_controller_mac, 6);
            memcpy(new_arp_pkt.eth_head.dest, arp->eth_head.src, 6);
            new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
            new_arp_pkt.opcode = htons(2);
            new_arp_pkt.sendip = arp->targetip;
            new_arp_pkt.targetip = arp->sendip;
            memcpy(new_arp_pkt.sendmac, g_controller_mac, 6);
            memcpy(new_arp_pkt.targetmac, arp->sendmac, 6);

            if(sw->ofp_version == OFP10_VERSION)
            {
                return sw->msg_driver.msg_handler[OFPT_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
            }
            else if(sw->ofp_version == OFP13_VERSION)
            {
                return sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
            }
        }
    }
    else if(arp->opcode == htons(2))    //arp reply
    {
        if(0 == memcmp(arp->eth_head.dest, g_controller_mac, 6))   //如果是对控制器网关发出的arp请求的回应
        {
            l3_proc(sw, packet_in_info, 0);
            return GN_OK;
        }

        p_user_dst = search_mac_user(arp->eth_head.dest);
        if(NULL == p_user_dst)
        {
            printf("####No user:[%s]\n", inet_htoa(ntohl(arp->targetip)));
        }
    }

    l2_proc(sw, p_user_src, p_user_dst, packet_in_info);
    return GN_OK;
}

static INT4 ip_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    ip_t *p_ip = (ip_t *)(packet_in_info->data);
    UINT4 host_ip = ntohl(p_ip->src);
    mac_user_t *p_user_src = NULL;
    mac_user_t *p_user_dst = NULL;

    if(get_fabric_state()){
    	fabric_ip_handle(sw,packet_in_info);
    	return GN_ERR;
    }

    //记录源MAC到MAC端口表中
    p_user_src = search_mac_user(p_ip->eth_head.src);
    if (NULL == p_user_src)
    {
        p_user_src = create_mac_user(sw, p_ip->eth_head.src, packet_in_info->inport, host_ip, NULL);
    }
    else
    {
        if(p_user_src->sw == sw)
        {
            if(p_user_src->port != packet_in_info->inport)  //如果入口和之前记录的入口不一样，说明风暴了，不再转发
            {
                return GN_ERR;
            }
        }
        else
        {
            if(g_adac_matrix.src_port[sw->index][p_user_src->sw->index] != packet_in_info->inport)  //入口非最优入口，说明是环路多余报文，不再转发
            {
                return GN_ERR;
            }
        }
        
        if(p_user_src->ipv4 != host_ip)
        {
            p_user_src->ipv4 = host_ip;
        }
    }

    if(0 == memcmp(p_ip->eth_head.dest, g_controller_mac, 6))   //如果目的MAC为控制器网关的MAC, 则为跨网段转发
    {
        UINT4 gw_ip = find_gateway_ip(p_ip->dest);                 //查找目的ip对应的网关
        if(0 == gw_ip)
        {
            return GN_ERR;
        }

        l3_proc(sw, packet_in_info, gw_ip);
        return GN_OK;
    }

    p_user_dst = search_mac_user(p_ip->eth_head.dest);
    if(NULL == p_user_dst)
    {
        printf("####No user:[%s]\n", inet_htoa(ntohl(p_ip->dest)));
    }

    l2_proc(sw, p_user_src, p_user_dst, packet_in_info);
    return GN_OK;
}

static INT4 ipv6_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    return GN_OK;
}

INT4 packet_in_process(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    ether_t *ether_header = (ether_t *)packet_in_info->data;

    if(ether_header->proto == htons(ETHER_LLDP))
    {
        return g_default_forward_handler.lldp(sw, packet_in_info);
    }
    else if(ether_header->proto == htons(ETHER_ARP))
    {
        return g_default_forward_handler.arp(sw, packet_in_info);
    }
    else if(ether_header->proto == htons(ETHER_IP))
    {
        return g_default_forward_handler.ip(sw, packet_in_info);
    }
    else if(ether_header->proto == htons(ETHER_IPV6))
    {
        return g_default_forward_handler.ipv6(sw, packet_in_info);
    }
    else
    {
        LOG_PROC("ERROR", "%s: ether type [%d] has no handler!\n", FN, ether_header->proto);
        return GN_ERR;
    }
}

forward_handler_t g_default_forward_handler =
{
    .lldp = lldp_packet_handler,
    .arp = arp_packet_handler,
    .ip = ip_packet_handler,
    .ipv6 = ipv6_packet_handler
};

INT1 register_handler_ether_packets(UINT2 eth_type, packet_in_proc_t packet_handler)
{
    if(eth_type == ETHER_LLDP)
    {
        g_default_forward_handler.lldp = packet_handler;
    }
    else if(eth_type == ETHER_ARP)
    {
        g_default_forward_handler.arp = packet_handler;
    }
    else if(eth_type == ETHER_IP)
    {
        g_default_forward_handler.ip = packet_handler;
    }
    else if(eth_type == ETHER_IPV6)
    {
        g_default_forward_handler.ipv6 = packet_handler;
    }
    else
    {
        printf("%s: ether type [0x%x] has no handler!\n", FN, eth_type);
        return GN_ERR;
    }

    return GN_OK;
}

INT4 init_forward_mgr()
{
    INT4 ret = init_l2();
    ret += init_l3();

    return ret;
}

void fini_forward_mgr()
{
    fini_l2();
    fini_l3();
}

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
#include "fabric_openstack_arp.h"
#include "../topo-mgr/topo-mgr.h"
#include "../user-mgr/user-mgr.h"
#include "../tenant-mgr/tenant-mgr.h"
#include "../overload-mgr/overload-mgr.h"

#include "gn_inet.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "mem_pool.h"

forward_handler_t g_default_forward_handler;
extern UINT4 g_openstack_on;
extern UINT1 g_fabric_start_flag;

#define PARAM_SET_MAX_COUNT	1000

void *g_param_set_id = NULL;
void *g_security_param_id = NULL;

void init_forward_param_list()
{
	if (NULL != g_param_set_id) {
		mem_destroy(g_param_set_id);
	}

	if (NULL != g_security_param_id) {
		mem_destroy(g_security_param_id);
	}

	g_param_set_id = mem_create(sizeof(param_set_t), PARAM_SET_MAX_COUNT);
	g_security_param_id = mem_create(sizeof(security_param_t), PARAM_SET_MAX_COUNT);
}


static INT4 arp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	if (0 != g_openstack_on)
		return 0;
    arp_t *arp = (arp_t *)(packet_in_info->data);
    UINT4 host_ip = ntohl(arp->sendip);
    mac_user_t *p_user_dst = NULL;
    // printf("####arp_packet_handler  src ip:[%s]\n", inet_htoa(host_ip));

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

static INT4 fabric_arp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    if (0 == of131_fabric_impl_get_tag_sw(sw))
    {
        return GN_OK;
    }
    
    //TODO
    arp_t *arp = (arp_t *)(packet_in_info->data);
    p_fabric_host_node src_port =NULL;
    p_fabric_host_node dst_port=NULL;
    if(arp->opcode == htons(1)){
        //arp request handle
        //save arp_request src info
        src_port = g_default_arp_handler.save_src_port(sw,arp->sendmac,arp->sendip,packet_in_info->inport);

        //get arp_request dst info
        dst_port = g_default_arp_handler.find_dst_port(src_port,arp->targetip);

        if(src_port==NULL && dst_port==NULL){
	        return GN_OK;
        }
        if(dst_port!=NULL){
        	g_default_arp_handler.arp_reply(src_port,dst_port,arp->sendip,arp->targetip,packet_in_info);
        }
        if(dst_port==NULL || dst_port->sw==NULL){
        	g_default_arp_handler.arp_flood(src_port,arp->sendip,arp->targetip,packet_in_info);
        }
    }else{
    	//save arp_request src info
    	src_port = g_default_arp_handler.save_src_port(sw,arp->sendmac,arp->sendip,packet_in_info->inport);
    	//get arp_request dst info
    	dst_port = g_default_arp_handler.find_dst_port(src_port,arp->targetip);
    	if(dst_port!=NULL){
    		g_default_arp_handler.arp_remove_ip_from_flood_list(arp->sendip);
    		g_default_arp_handler.arp_reply_output(src_port,dst_port,arp->targetip,packet_in_info);
    	}
		else {
			g_default_arp_handler.arp_remove_ip_from_flood_list(arp->sendip);
		}
    }
    return GN_OK;
}

param_set_p create_param_set()
{
	param_set_p g_param_set = (param_set_p)mem_get(g_param_set_id);
	security_param_p src_security = (security_param_p)mem_get(g_security_param_id);
	security_param_p dst_security = (security_param_p)mem_get(g_security_param_id);
	if ((NULL == g_param_set) || (NULL == src_security)|| (NULL == dst_security)) {
		return NULL;
	}
	memset(g_param_set, 0, sizeof(param_set_t));
	memset(src_security, 0, sizeof(security_param_t));
	memset(dst_security, 0, sizeof(security_param_t));
	g_param_set->src_security = src_security;
	g_param_set->dst_security = dst_security;

	return g_param_set;
}

void destory_param_set(param_set_p param)
{
	if (NULL != param) {
		if (NULL != param->src_security) {
			mem_free(g_security_param_id, param->src_security);
		}
		if (NULL != param->dst_security) {
			mem_free(g_security_param_id, param->dst_security);
		}
		mem_free(g_param_set_id, param);
	}
}

static INT4 fabric_ip_packet_handle(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    if (0 == of131_fabric_impl_get_tag_sw(sw))
    {
        return GN_OK;
    }

	param_set_p param = create_param_set();
	if (NULL == param) {
		LOG_PROC("INFO", "Can't create param list!");
		return 0;
	}

	ip_t *p_ip = (ip_t *)(packet_in_info->data);

	p_fabric_host_node src_port =NULL;
	p_fabric_host_node dst_port=NULL;
	INT4 foward_type = IP_DROP;

//	nat_show_ip(p_ip->src);
//	nat_show_ip(p_ip->dest);
//	nat_show_ip(sw->sw_ip);
//	nat_show_mac(p_ip->eth_head.src);
//	nat_show_mac(p_ip->eth_head.dest);
//	printf("port is %d\n", packet_in_info->inport);
//	if (IPPROTO_UDP == p_ip->proto)
//	{
//		udp_t* udp = (udp_t*)p_ip->data;
//		printf("udp port is %d\n", ntohs(udp->dport));
//	}

	//save arp_request src info
	src_port = g_default_ip_handler.save_src_port_ip(sw,p_ip->eth_head.src,p_ip->src,packet_in_info->inport);

	//get arp_request dst info
	dst_port = g_default_ip_handler.find_dst_port_ip(src_port, p_ip->dest);

	if ((src_port == NULL && dst_port == NULL) && (-1 != p_ip->dest))
	{
		// return GN_ERR;
	}
	else {
		//check if scr_port can access dst_port
		INT4 gn_access_result = g_default_ip_handler.ip_packet_check_access(src_port, dst_port, packet_in_info, param);

		if(gn_access_result == GN_ERR)
		{
			// printf(" access denied !\n");
			g_default_ip_handler.ip_install_deny_flow(sw, p_ip);
				// return GN_ERR;
		}
		else {
			param->src_port = src_port;
			param->dst_port = dst_port;

			//compute src & dst info
			foward_type = g_default_ip_handler.ip_packet_compute_src_dst_forward(src_port,dst_port,packet_in_info,param);
            //发往外部的icmp消息不统计
            if (!(foward_type == CONTROLLER_FORWARD && IPPROTO_ICMP == p_ip->proto))
            {
                add_msg_counter(sw, p_ip->dest, p_ip->eth_head.dest);
            }

			// printf("foward_type is %d\n", foward_type);

			if(foward_type==CONTROLLER_FORWARD){
				//controller forward
				fabric_openstack_packet_output(param->dst_sw, packet_in_info, param->dst_inport);
				//TODO
			}else if  (foward_type==IP_FLOOD) {
				//ip flood
				g_default_ip_handler.ip_flood(src_port,param->src_ip, param->dst_ip, param->src_mac,packet_in_info);
			}else if (foward_type==BROADCAST_DHCP) {
				//can access but don't know where is dst_port ->flood
				openstack_ip_p_broadcast(packet_in_info);
				//DHCP handle
				//TODO
			}else if  (foward_type==IP_HANDLE_ERR) {
				// printf(" something happened! ");
				// return GN_ERR;
			}
			else if (foward_type == IP_PACKET) {
				// printf(" packet out! \n");
			}
			else if (foward_type == IP_DROP) {
				// return GN_ERR;
			}
			else if ((Internal_port_flow == foward_type) || (Internal_out_subnet_flow == foward_type)) {
				g_default_ip_handler.ip_packet_install_flow(param, foward_type);
				fabric_openstack_packet_output(param->dst_port->sw,packet_in_info,param->dst_port->port);
				// fabric_openstack_packet_output(sw, packet_in_info, OFPP13_TABLE);
			}
			else
			{
				if (OFPP13_CONTROLLER != packet_in_info->inport)
				{
					//install other flows
					g_default_ip_handler.ip_packet_install_flow(param, foward_type);
					fabric_openstack_packet_output(sw, packet_in_info, OFPP13_TABLE);
				}
			}
		}
	}

	destory_param_set(param);
	return GN_OK;
}
static INT4 ip_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	if (0 != g_openstack_on)
		return 0;
    ip_t *p_ip = (ip_t *)(packet_in_info->data);
    UINT4 host_ip = ntohl(p_ip->src);
    // printf("####ip_packet_handler  src ip:[%s]\n", inet_htoa(host_ip));
    mac_user_t *p_user_src = NULL;
    mac_user_t *p_user_dst = NULL;


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
	/*
	 * temp added for ipv6
	 * by lxf@2016.1.11
	 */
#if 0
	if (0 == g_fabric_start_flag) {
		return 0;
	}

	UINT1 local_node[16] = {0};
	ipv6_str_to_number("FF01::1", local_node);
	UINT1 local_link[16] = {0};
	ipv6_str_to_number("FF02::1", local_link);
	UINT1 solicited_node[16] = {0};
	ipv6_str_to_number("FF02::1:FF00:0", solicited_node);
	UINT1 local_link_src[16] = {0};
	ipv6_str_to_number("fe80::", local_link_src);

	param_set_p param = create_param_set();
	if (NULL == param) {
		LOG_PROC("INFO", "Can't create param list!");
		return 0;
	}

	ipv6_t* p_ip = (ipv6_t*)(packet_in_info->data+14);

	p_fabric_host_node src_port = NULL;
	p_fabric_host_node dst_port= NULL;

	UINT1 src_mac[6] = {0};
	UINT1 dst_mac[6] = {0};

	memcpy(dst_mac, packet_in_info->data, 6);
	memcpy(src_mac, packet_in_info->data+6, 6);

//	nat_show_ipv6(p_ip->src);
//	nat_show_ipv6(p_ip->dest);
//	nat_show_ip(sw->sw_ip);
//	nat_show_mac(src_mac);
//	nat_show_mac(dst_mac);

	src_port = openstack_save_host_info_ipv6(sw, src_mac, p_ip->src, packet_in_info->inport);

	dst_port = openstack_find_ip_dst_port_ipv6(src_port, p_ip->dest);

	if (NULL != dst_port) {
		// printf("find dest!\n");
		fabric_openstack_install_fabric_flows_ipv6(src_port, dst_port, NULL, NULL);
	}

	if (0 == memcmp(src_mac+3, dst_mac+3, 3)) {
		// printf("same mac\n");
		destory_param_set(param);
		return 0;
	}

	if (((0 == memcmp(local_node, p_ip->dest, 16))
			|| (0 == memcmp(local_link, p_ip->dest, 16)))
			|| (0 == memcmp(solicited_node, p_ip->dest, 13))) {
		// printf("local link flood\n");
		openstack_ip_p_broadcast(packet_in_info);
	}

	destory_param_set(param);

#endif
    return GN_OK;
}
///Added by Xu yanwei in 2015-08-13
static INT4 vlan_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	if(get_fabric_state()){
			fabric_vlan_handle(sw, packet_in_info);
			return GN_OK;
	    }
    return GN_OK;
}

INT4 packet_in_process(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    ether_t *ether_header = (ether_t *)packet_in_info->data;
    // printf("####eth header ip type:[%d]\n", ether_header->proto);
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
    else if(ether_header->proto == htons(ETHER_VLAN))
    {
        return g_default_forward_handler.vlan(sw, packet_in_info);
    }
    else
    {
        // LOG_PROC("ERROR", "%s: ether type [%d] has no handler!\n", FN, ether_header->proto);
        return GN_ERR;
    }
}

forward_handler_t g_default_forward_handler =
{
    .lldp = lldp_packet_handler,
    .arp = arp_packet_handler,
    .ip = ip_packet_handler,
    .ipv6 = ipv6_packet_handler,
	.vlan= vlan_packet_handler
};

arp_handler_t g_default_arp_handler =
{
    .save_src_port=fabric_save_host_info,
	.find_dst_port=fabric_find_dst_port,
	.arp_flood=fabric_arp_flood,
	.arp_reply=fabric_arp_reply,
	.arp_remove_ip_from_flood_list=fabric_arp_remove_ip_from_flood_list,
	.arp_reply_output=fabric_arp_reply_output
};
ip_handler_t g_default_ip_handler =
{
    .save_src_port_ip=fabric_save_host_info,
	.find_dst_port_ip=fabric_find_dst_port,
	.ip_packet_output=fabric_ip_packet_output,
	.ip_packet_check_access=fabric_ip_packet_check_access,
	.ip_flood=fabric_ip_p_flood,
	.ip_packet_install_flow=fabric_ip_p_install_flow,
	.ip_packet_compute_src_dst_forward=fabric_compute_src_dst_forward,
	.ip_install_deny_flow = fabric_ip_install_deny_flow,
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
    else if(eth_type == ETHER_VLAN)
    {
        g_default_forward_handler.vlan = packet_handler;
    }
    else
    {
        // printf("%s: ether type [0x%x] has no handler!\n", FN, eth_type);
        return GN_ERR;
    }

    return GN_OK;
}

void init_handler(){
	if(get_fabric_state()){

		init_forward_param_list();

		g_default_forward_handler.ip=fabric_ip_packet_handle;
		g_default_forward_handler.arp=fabric_arp_packet_handler;
		if(g_openstack_on==0){
			g_default_arp_handler.arp_flood=fabric_arp_flood;
			g_default_arp_handler.arp_remove_ip_from_flood_list=fabric_arp_remove_ip_from_flood_list;
			g_default_arp_handler.arp_reply=fabric_arp_reply;
			g_default_arp_handler.arp_reply_output=fabric_arp_reply_output;
			g_default_arp_handler.find_dst_port=fabric_find_dst_port;
			g_default_arp_handler.save_src_port=fabric_save_host_info;

			g_default_ip_handler.save_src_port_ip=fabric_save_host_info;
			g_default_ip_handler.find_dst_port_ip=fabric_find_dst_port_ip;
			g_default_ip_handler.ip_packet_output=fabric_ip_packet_output;
			g_default_ip_handler.ip_flood=fabric_ip_p_flood;
			g_default_ip_handler.ip_packet_install_flow=fabric_ip_p_install_flow;
			g_default_ip_handler.ip_packet_check_access=fabric_ip_packet_check_access;
			g_default_ip_handler.ip_packet_compute_src_dst_forward = fabric_compute_src_dst_forward;
			g_default_ip_handler.ip_install_deny_flow = fabric_ip_install_deny_flow;
		}else{
			g_default_arp_handler.arp_flood=openstack_arp_flood;
			g_default_arp_handler.arp_remove_ip_from_flood_list=openstack_arp_remove_ip_from_flood_list;
			g_default_arp_handler.arp_reply=openstack_arp_reply;
			g_default_arp_handler.arp_reply_output=openstack_arp_reply_output;
			g_default_arp_handler.find_dst_port=openstack_find_dst_port;
			g_default_arp_handler.save_src_port=openstack_save_host_info;

			g_default_ip_handler.save_src_port_ip=openstack_save_host_info;
			g_default_ip_handler.find_dst_port_ip=openstack_find_ip_dst_port;
			g_default_ip_handler.ip_packet_output=openstack_ip_packet_output;
			g_default_ip_handler.ip_flood=openstack_ip_p_flood;
			g_default_ip_handler.ip_packet_install_flow=openstack_ip_p_install_flow;
			g_default_ip_handler.ip_packet_check_access = openstack_ip_packet_check_access;
			g_default_ip_handler.ip_packet_compute_src_dst_forward = openstack_ip_packet_compute_src_dst_forward;
			g_default_ip_handler.ip_install_deny_flow = openstack_ip_install_deny_flow;
			// g_default_ip_handler.ip_broadcast = openstack_ip_p_broadcast;
		}
	}
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




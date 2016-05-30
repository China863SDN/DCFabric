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
*   File Name   : forward-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef FORWARD_MGR_H_
#define FORWARD_MGR_H_

#include "gnflush-types.h"
#include "fabric_host.h"
#include "fabric_openstack_external.h"

#define MAX_L3_SUBNET 10
#define MAX_L3_SUBNET_NAME_LEN 64

#pragma pack(1)
typedef struct subnet
{
    BOOL is_using;       //是否已占用
    INT1 name[64];       //网关名
    INT1 netmask[16];    //子网掩码
    UINT4 gw_prefix;     //子网掩码
    UINT4 gw_ip;         //网关ip      网络字节序
    UINT4 gw_minip;      //最小ip      网络字节序
    UINT4 gw_maxip;      //最大ip      网络字节序
}subnet_t;

typedef struct forward_handler
{
    packet_in_proc_t lldp;
    packet_in_proc_t arp;
    packet_in_proc_t ip;
    packet_in_proc_t ipv6;
    packet_in_proc_t vlan;
}forward_handler_t;

typedef struct security_param_set
{
	 UINT1 ip_proto;        	/* IP protocol*/
	 UINT2 tcp_port_num;        /* port number*/
	 UINT2 udp_port_num;        /* port number*/
	 UINT1 imcp_type; 	    	/* ICMP type. */
	 UINT1 icmp_code;   	  	/* ICMP code. */
}security_param_t, *security_param_p;

typedef struct param_set
{
	UINT4 src_ip;
	UINT4 dst_ip;
	UINT1 src_mac[6];
	UINT1 dst_mac[6];
	gn_switch_t* src_sw;
	gn_switch_t* dst_sw;
	gn_switch_t* out_sw;
	UINT1 proto;
	UINT2 src_port_no;
	UINT2 dst_port_no;
	UINT4 src_inport;
	UINT4 dst_inport;
	UINT1 src_gateway_mac[6];
	UINT1 dst_gateway_mac[6];
	p_fabric_host_node src_port;
	p_fabric_host_node dst_port;
	p_fabric_host_node src_gateway;
	p_fabric_host_node dst_gateway;
	UINT4 mod_src_ip;
	UINT4 mod_dst_ip;
	UINT4 src_vlanid;
	UINT4 dst_vlanid;
	UINT1 packet_src_mac[6];
	UINT1 packet_dst_mac[6];
	UINT4 outer_ip;
	UINT1 outer_mac[6];
	UINT1 outer_gateway_mac[6];
	UINT4 dst_gateway_output;
	external_floating_ip_p floatingip;
	security_param_p src_security;
	security_param_p dst_security;
	UINT4 vip;
	UINT1 vip_mac[6];
	UINT4 vip_tcp_port_no;
}param_set_t, *param_set_p;

typedef p_fabric_host_node (*save_src_info)(gn_switch_t *sw,UINT1* sendmac,UINT4 sendip,UINT4 inport);
typedef p_fabric_host_node (*find_dst_port)(p_fabric_host_node src_node,UINT4 targetip);
typedef INT4 (*flood_t)(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
typedef INT4 (*ip_packet_flood_t)(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,UINT1* srcmac,packet_in_info_t *packet_in);
typedef INT4 (*ip_packet_install_flow_t)(param_set_p param_set, INT4 foward_type);
typedef INT4 (*reply_t)(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
typedef INT4 (*check_access_t)(p_fabric_host_node src_port,p_fabric_host_node dst_port, packet_in_info_t* packet_in, param_set_p param);
typedef INT4 (*compute_forward_t)(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in, param_set_p param_set);
typedef INT4 (*output_t)(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
typedef INT4 (*remove_ip_from_flood_list_t)(UINT4 sendip);
typedef INT4 (*reply_output_t)(p_fabric_host_node src,p_fabric_host_node dst,UINT4 targetIP, packet_in_info_t *packet_in);
typedef p_fabric_host_node (*find_ip_dst_port)(p_fabric_host_node src_node,UINT4 targetip);
typedef INT4 (*ip_install_deny_flow_t)(gn_switch_t *sw, ip_t* ip);

typedef struct arp_handler
{
	save_src_info save_src_port;
	find_dst_port find_dst_port;
	flood_t arp_flood;
	reply_t arp_reply;
	remove_ip_from_flood_list_t arp_remove_ip_from_flood_list;
	reply_output_t arp_reply_output;
}arp_handler_t;

typedef struct ip_handler
{
	save_src_info save_src_port_ip;
	find_ip_dst_port find_dst_port_ip;
	output_t ip_packet_output;
	check_access_t ip_packet_check_access;
	ip_packet_flood_t ip_flood;
	ip_packet_install_flow_t ip_packet_install_flow;
	compute_forward_t ip_packet_compute_src_dst_forward;
	ip_install_deny_flow_t ip_install_deny_flow;
}ip_handler_t;

#pragma pack()


//考虑提供处理函数注册，不同的交换机使用不通的处理逻辑
extern forward_handler_t g_default_forward_handler;
extern arp_handler_t g_default_arp_handler;
extern ip_handler_t g_default_ip_handler;
//L3子网信息
extern subnet_t g_subnet_info[];

UINT4 find_gateway_ip(UINT4 ip);
subnet_t *search_l3_subnet(UINT4 ip);
INT4 create_l3_subnet(INT1 *name, INT1 *masked_ip);
INT4 destory_l3_subnet(INT1 *masked_ip);

INT4 packet_in_process(gn_switch_t *sw, packet_in_info_t *packet_in_info);
INT4 init_forward_mgr();
void fini_forward_mgr();
void init_handler();

// initialize forward param
void init_forward_param_list();
#endif /* FORWARD_MGR_H_ */

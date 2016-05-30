/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
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

/*
 * fabric_host.h
 *
 *  Created on: Apr 4, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */

#ifndef INC_FABRIC_FABRIC_HOST_H_
#define INC_FABRIC_FABRIC_HOST_H_
#include "gnflush-types.h"
#include "gn_inet.h"
//#include "openstack_host.h"


/************************************
 * mem pool num
 ************************************/
#define FABRIC_HOST_LIST_MAX_NUM 20480
#define FABRIC_HOST_QUEUE_MAX_NUM 20480
#define FABRIC_ARP_REQUEST_LIST_MAX_NUM 20480
#define FABRIC_ARP_FLOOD_QUEUE_MAX_NUM 20480
#define FABRIC_IP_FLOOD_QUEUE_MAX_NUM 20480
#define FABRIC_FLOW_QUEUE_MAX_NUM 20480
#define FABRIC_HOST_IP_MAX_NUM 10
/************************************
 * host node & functions
 * part 1: list part
 *    this part store the total hosts
 *    other's can find the host by this part's function
 *    this part's node memory is create/free by stand memory function
 *    this part functions are thread safe
 * part 2: queue part
 *    this part store the hosts want to store to list
 *    only to transfer the host information to thread
 *    this part's node memory is create/free by memory pool
 *    this part functions are thread safe
 ************************************/
typedef struct fabric_host_node{
	gn_switch_t* sw;
	UINT8 dpid;
	UINT4 port;
	UINT1 mac[6];
	UINT4 ip_list[FABRIC_HOST_IP_MAX_NUM];
	UINT1 ipv6[16][FABRIC_HOST_IP_MAX_NUM];
	UINT1 ip_count;
	void* data;
	struct fabric_host_node* next;
	UINT1 type;
	UINT1 check_status;
}t_fabric_host_node,* p_fabric_host_node;

typedef struct fabric_host_list{
	UINT4 list_num;
	p_fabric_host_node list;
}t_fabric_host_list,*p_fabric_host_list;

typedef struct fabric_host_queue{
	p_fabric_host_node head;
	p_fabric_host_node rear;
    UINT4 queue_num;
}t_fabric_host_queue;

////////////////////////////////////////////////////////////////////////

p_fabric_host_node create_fabric_host_list_node(gn_switch_t* sw,UINT4 port,UINT1* mac,UINT4 ip, UINT1* ipv6);
void delete_fabric_host_list_node(p_fabric_host_node node);
p_fabric_host_node copy_fabric_host_node(p_fabric_host_node node_p);

////////////////////////////////////////////////////////////////////////

void init_fabric_host_list();
p_fabric_host_node get_fabric_host_from_list_by_ip(UINT4 ip);
/*
 * temp added for ipv6
 * by lxf@2016.1.11
 */
#if 1
p_fabric_host_node get_fabric_host_from_list_by_ipv6(UINT1* ip);
#endif

p_fabric_host_node get_fabric_host_from_list_by_mac(UINT1* mac);
void insert_fabric_host_into_list(p_fabric_host_node node);
p_fabric_host_node insert_fabric_host_into_list_paras(gn_switch_t* sw,UINT8 dpid,UINT4 port,UINT1* mac,UINT4 ip, UINT1* ipv6);
p_fabric_host_node remove_fabric_host_from_list_by_ip(UINT4 ip);
p_fabric_host_node remove_fabric_host_from_list_by_mac(UINT1* mac);
void delete_fabric_host_from_list_by_sw(gn_switch_t* sw);
void destroy_fabric_host_list();
UINT4 is_fabric_host_list_empty();
////////////////////////////////////////////////////////////////////////
//Multiple IPs for one host.   added by xuyanwei at 2015.8.13

void add_fabric_host_ip(p_fabric_host_node node,UINT4 newIP);
BOOL check_IP_in_fabric_host(p_fabric_host_node node,UINT4 IP);
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////

p_fabric_host_node create_fabric_host_queue_node(gn_switch_t* sw,UINT4 port,UINT1* mac,UINT4 ip);
void delete_fabric_host_queue_node(p_fabric_host_node node);

////////////////////////////////////////////////////////////////////////
void init_fabric_host_queue();
void push_fabric_host_into_queue(p_fabric_host_node node);
p_fabric_host_node pop_fabric_host_from_queue();
p_fabric_host_node get_head_fabric_host_from_queue();
void destroy_fabric_host_queue();
UINT4 is_fabric_host_queue_empty();
////////////////////////////////////////////////////////////////////////

/************************************
 * arp request node & functions
 ************************************/
typedef struct fabric_arp_request_node{
	UINT4 dst_IP;
	UINT4 src_IP;
	p_fabric_host_node src_req;
	struct fabric_arp_request_node* next;
}t_fabric_arp_request_node,* p_fabric_arp_request_node;

typedef struct fabric_arp_request_list{
	UINT4 list_num;
	p_fabric_arp_request_node list;
}t_fabric_arp_request_list,*p_fabric_arp_request_list;

////////////////////////////////////////////////////////////////////////

p_fabric_arp_request_node create_fabric_arp_request_list_node(p_fabric_host_node src,UINT4 src_IP,UINT4 dst_IP);
p_fabric_arp_request_node delete_fabric_arp_request_list_node(p_fabric_arp_request_node node);

////////////////////////////////////////////////////////////////////////

void init_fabric_arp_request_list();
//p_fabric_arp_request_node get_fabric_arp_request_by_dstip(UINT4 dst_ip);
void insert_fabric_arp_request_into_list(p_fabric_arp_request_node node);
p_fabric_arp_request_node remove_fabric_arp_request_from_list_by_dstip(UINT4 dst_ip);
void destroy_fabric_arp_request_list();
void fabric_add_into_arp_request(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip);


////////////////////////////////////////////////////////////////////////
/************************************
 * ip flood node & functions
 ************************************/
typedef struct fabric_arp_flood_node{
	UINT4 ip;
	packet_in_info_t packet_in_info;
//	arp_t arp_data;
	struct fabric_arp_flood_node* next;
}t_fabric_arp_flood_node,* p_fabric_arp_flood_node;
typedef struct
{
	p_fabric_arp_flood_node head;
	p_fabric_arp_flood_node rear;
    UINT4 queue_num;
}t_fabric_arp_flood_queue;
////////////////////////////////////////////////////////////////////////
p_fabric_arp_flood_node create_fabric_arp_flood_node(packet_in_info_t * packet_in_info,UINT4 ip);
void delete_fabric_arp_flood_node(p_fabric_arp_flood_node node);

void init_fabric_arp_flood_queue();
void push_fabric_arp_flood_into_queue(p_fabric_arp_flood_node node);
p_fabric_arp_flood_node pop_fabric_arp_flood_from_queue();
p_fabric_arp_flood_node get_fabric_arp_flood_from_queue_by_ip(UINT4 ip);
p_fabric_arp_flood_node get_head_fabric_arp_flood_from_queue();
void destory_fabric_arp_flood_queue();
UINT4 is_fabric_arp_flood_queue_empty();

////////////////////////////////////////////////////////////////////////
/************************************
 * ip flood node & functions
 ************************************/
typedef struct fabric_ip_flood_node{
	UINT4 ip;
	packet_in_info_t packet_in_info;
	ip_t ip_data;
	struct fabric_ip_flood_node* next;
}t_fabric_ip_flood_node,* p_fabric_ip_flood_node;
typedef struct
{
	p_fabric_ip_flood_node head;
	p_fabric_ip_flood_node rear;
    UINT4 queue_num;
}t_fabric_ip_flood_queue;
////////////////////////////////////////////////////////////////////////

p_fabric_ip_flood_node create_fabric_ip_flood_node(packet_in_info_t * packet_in_info,UINT4 ip);
void delete_fabric_ip_flood_node(p_fabric_ip_flood_node node);

void init_fabric_ip_flood_queue();
void push_fabric_ip_flood_into_queue(p_fabric_ip_flood_node node);
p_fabric_ip_flood_node pop_fabric_ip_flood_from_queue();
p_fabric_ip_flood_node get_fabric_ip_flood_from_queue_by_ip(UINT4 ip);
p_fabric_ip_flood_node get_head_fabric_ip_flood_from_queue();
void destory_fabric_ip_flood_queue();
UINT4 is_fabric_ip_flood_queue_empty();

////////////////////////////////////////////////////////////////////////
/************************************
 * fabric flow node & functions
 ************************************/
typedef struct fabric_flow_node{
	p_fabric_host_node src_host;
	UINT4 src_IP;
	UINT4 src_tag;
	p_fabric_host_node dst_host;
	UINT4 dst_IP;
	UINT4 dst_tag;
	struct fabric_flow_node* next;
}t_fabric_flow_node,* p_fabric_flow_node;
typedef struct
{
	p_fabric_flow_node head;
	p_fabric_flow_node rear;
    UINT4 queue_num;
}t_fabric_flow_queue;


extern t_fabric_host_list g_fabric_host_list;


////////////////////////////////////////////////////////////////////////
p_fabric_flow_node create_fabric_flow_node(p_fabric_host_node src_host,
		UINT4 src_IP,
		UINT4 src_tag,
		p_fabric_host_node dst_host,
		UINT4 dst_IP,
		UINT4 dst_tag);
void delete_fabric_flow_node(p_fabric_flow_node node);

void init_fabric_flow_queue();
void push_fabric_flow_into_queue(p_fabric_flow_node node);
p_fabric_flow_node pop_fabric_flow_from_queue();
p_fabric_flow_node get_fabric_flow_from_queue(p_fabric_host_node src_host, UINT4 src_IP , p_fabric_host_node dst_host,UINT4 dst_IP);
p_fabric_flow_node get_head_fabric_flow_from_queue();
void destroy_fabric_flow_queue();
UINT4 is_fabric_flow_queue_empty();
////////////////////////////////////////////////////////////////////////

void set_fabric_host_port_portno(const UINT1 *mac, UINT4 ofport_no);
p_fabric_host_node get_fabric_host_from_list_by_sw_port(UINT8 dpid, UINT4 port);

#endif /* INC_FABRIC_FABRIC_HOST_H_ */

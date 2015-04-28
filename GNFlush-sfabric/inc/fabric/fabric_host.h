/*
 * fabric_host.h
 *
 *  Created on: Apr 4, 2015
 *      Author: joe
 */

#ifndef INC_FABRIC_FABRIC_HOST_H_
#define INC_FABRIC_FABRIC_HOST_H_
#include "gnflush-types.h"
#include "gn_inet.h"

/************************************
 * mem pool num
 ************************************/
#define FABRIC_HOST_LIST_MAX_NUM 20480
#define FABRIC_HOST_QUEUE_MAX_NUM 20480
#define FABRIC_ARP_REQUEST_LIST_MAX_NUM 20480
#define FABRIC_ARP_FLOOD_QUEUE_MAX_NUM 20480
#define FABRIC_IP_FLOOD_QUEUE_MAX_NUM 20480
#define FABRIC_FLOW_QUEUE_MAX_NUM 20480
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
	UINT4 port;
	UINT1 mac[6];
	UINT4 ip;
	struct fabric_host_node* next;
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

p_fabric_host_node create_fabric_host_list_node(gn_switch_t* sw,UINT4 port,UINT1* mac,UINT4 ip);
void delete_fabric_host_list_node(p_fabric_host_node node);

////////////////////////////////////////////////////////////////////////

void init_fabric_host_list();
p_fabric_host_node get_fabric_host_from_list_by_ip(UINT4 ip);
p_fabric_host_node get_fabric_host_from_list_by_mac(UINT1* mac);
void insert_fabric_host_into_list(p_fabric_host_node node);
void insert_fabric_host_into_list_paras(gn_switch_t* sw,UINT4 port,UINT1* mac,UINT4 ip);
p_fabric_host_node remove_fabric_host_from_list_by_ip(UINT4 ip);
p_fabric_host_node remove_fabric_host_from_list_by_mac(UINT1* mac);
void destroy_fabric_host_list();
UINT4 is_fabric_host_list_empty();
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
	UINT4 dst_ip;
	p_fabric_host_node src_req;
	struct fabric_arp_request_node* next;
}t_fabric_arp_request_node,* p_fabric_arp_request_node;

typedef struct fabric_arp_request_list{
	UINT4 list_num;
	p_fabric_arp_request_node list;
}t_fabric_arp_request_list,*p_fabric_arp_request_list;

////////////////////////////////////////////////////////////////////////

p_fabric_arp_request_node create_fabric_arp_request_list_node(p_fabric_host_node src,UINT4 dst_ip);
p_fabric_arp_request_node delete_fabric_arp_request_list_node(p_fabric_arp_request_node node);

////////////////////////////////////////////////////////////////////////

void init_fabric_arp_request_list();
//p_fabric_arp_request_node get_fabric_arp_request_by_dstip(UINT4 dst_ip);
void insert_fabric_arp_request_into_list(p_fabric_arp_request_node node);
p_fabric_arp_request_node remove_fabric_arp_request_from_list_by_dstip(UINT4 dst_ip);
void destroy_fabric_arp_request_list();
////////////////////////////////////////////////////////////////////////
/************************************
 * ip flood node & functions
 ************************************/
typedef struct fabric_arp_flood_node{
	UINT4 ip;
	packet_in_info_t packet_in_info;
	arp_t arp_data;
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
	UINT4 src_tag;
	p_fabric_host_node dst_host;
	UINT4 dst_tag;
	struct fabric_flow_node* next;
}t_fabric_flow_node,* p_fabric_flow_node;
typedef struct
{
	p_fabric_flow_node head;
	p_fabric_flow_node rear;
    UINT4 queue_num;
}t_fabric_flow_queue;
////////////////////////////////////////////////////////////////////////
p_fabric_flow_node create_fabric_flow_node(p_fabric_host_node src_host,
		UINT4 src_tag,
		p_fabric_host_node dst_host,
		UINT4 dst_tag);
void delete_fabric_flow_node(p_fabric_flow_node node);

void init_fabric_flow_queue();
void push_fabric_flow_into_queue(p_fabric_flow_node node);
p_fabric_flow_node pop_fabric_flow_from_queue();
p_fabric_flow_node get_fabric_flow_from_queue_by_ip(p_fabric_host_node src_host,p_fabric_host_node dst_host);
p_fabric_flow_node get_head_fabric_flow_from_queue();
void destroy_fabric_flow_queue();
UINT4 is_fabric_flow_queue_empty();
////////////////////////////////////////////////////////////////////////

#endif /* INC_FABRIC_FABRIC_HOST_H_ */

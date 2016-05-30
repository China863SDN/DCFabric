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
 * fabric_host.c
 *
 *  Created on: Apr 4, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#include "fabric_host.h"
#include "mem_pool.h"
#include <pthread.h>
#include "openstack_host.h"

/*********************************
 * Global verbs
 *********************************/
t_fabric_host_list g_fabric_host_list;
t_fabric_host_queue g_fabric_host_queue;
t_fabric_arp_request_list g_arp_request_list;
t_fabric_arp_flood_queue g_arp_flood_queue;
t_fabric_ip_flood_queue g_ip_flood_queue;
t_fabric_flow_queue g_fabric_flow_queue;

static pthread_mutex_t g_fabric_host_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_fabric_arp_request_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_fabric_arp_flood_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_fabric_ip_flood_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_fabric_flow_thread_mutex = PTHREAD_MUTEX_INITIALIZER;

void *g_fabric_host_list_mem_id = NULL;
void *g_fabric_host_queue_mem_id = NULL;
void *g_fabric_arp_request_list_mem_id = NULL;
void *g_fabric_arp_flood_queue_mem_id = NULL;
void *g_fabric_ip_flood_queue_mem_id = NULL;
void *g_fabric_flow_queue_mem_id = NULL;
////////////////////////////////////////////////////////
// Host list functions
////////////////////////////////////////////////////////
/*
 * create a host node
 * gn_switch_t* sw: the switch is the host connect to
 * UINT4 port:		the port is the host connect to switch's port
 * UINT* mac:		the host's mac address
 * UINT4 ip:		the host's ip address
 */

p_fabric_host_node create_fabric_host_list_node(gn_switch_t* sw, UINT4 port, UINT1* mac, UINT4 ip, UINT1* ipv6)
{
	p_fabric_host_node ret = NULL;
	//ret = (p_fabric_host_node)gn_malloc(sizeof(t_fabric_host_node));
	ret = (p_fabric_host_node)mem_get(g_fabric_host_list_mem_id);
	if (NULL != ret) {
		ret->sw = sw;
		ret->port = port;
		ret->ip_count=0;
		//ret->ip_list[ip_count]=ip
		add_fabric_host_ip(ret,ip);
		memcpy(ret->mac, mac, 6);
		if (ipv6)
			memcpy(ret->ipv6[0], ipv6, 16);
	}
	else {
		LOG_PROC("ERROR", "Fabric host: Fail to create list node, Can't get memory");
	}
	return ret;
};

p_fabric_host_node copy_fabric_host_node(p_fabric_host_node node_p)
{
	p_fabric_host_node ret = NULL;
	ret = (p_fabric_host_node)mem_get(g_fabric_host_list_mem_id);
	if (NULL != ret) {
		memset(ret, 0, sizeof(t_fabric_host_node));
		if (NULL != node_p) {
			ret->sw = node_p->sw;
			ret->port = node_p->port;
			ret->ip_count= node_p->ip_count;
			add_fabric_host_ip(ret, node_p->ip_list[0]);
			memcpy(ret->mac, node_p->mac, 6);
			memcpy(ret->ipv6[0], node_p->ipv6[0], 16);
		}
	}
	else {
		LOG_PROC("ERROR", "Fabric host: Fail to create list node, Can't get memory");
	}
	return ret;
}


/*
 * delete a host node
 * p_fabric_host_node node : the node need to delete
 */
void delete_fabric_host_list_node(p_fabric_host_node node){
	if(node != NULL){
		mem_free(g_fabric_host_list_mem_id,node);
		//free(node);
	}
	return;
};

/*
 * initialize the fabric host list which store the host nodes
 * delete all nodes and set the number to zero
 */
void init_fabric_host_list(){
//	t_fabric_host_node sentinel;
//	p_fabric_host_node p_sentinel = &sentinel,temp;
//
//	p_sentinel->next = g_fabric_host_list.list;
//
//	while(p_sentinel->next != NULL){
//		temp = p_sentinel->next;
//		p_sentinel->next = temp->next;
//		free(temp);
//	}
//	g_fabric_host_list.list = NULL;
//	g_fabric_host_list.list_num = 0;
	if(g_fabric_host_list_mem_id != NULL){
		mem_destroy(g_fabric_host_list_mem_id);
	}
	g_fabric_host_list_mem_id = mem_create(sizeof(t_fabric_host_node), FABRIC_HOST_LIST_MAX_NUM);
	g_fabric_host_list.list = NULL;
	g_fabric_host_list.list_num = 0;
	return;
};
/*
 * get the host object by ip address
 * if not found, return NULL
 * UINT4 ip : host's ip address
 */
p_fabric_host_node get_fabric_host_from_list_by_ip(UINT4 ip){
	p_fabric_host_node ret = NULL;
	ret = g_fabric_host_list.list;
	while(ret != NULL)
	{
		if(check_IP_in_fabric_host(ret,ip))
		{
			return ret;
		}
		ret = ret->next;
	}
	return NULL;
};


/*
 * temp added for ipv6
 * by lxf@2016.1.11
 */
#if 1
/*
 * get the host object by ipv6 address
 * if not found, return NULL
 * UINT4 ip : host's ipv6 address
 */
p_fabric_host_node get_fabric_host_from_list_by_ipv6(UINT1* ip)
{
	p_fabric_host_node ret = NULL;
	ret = g_fabric_host_list.list;
	while(ret != NULL)
	{
		if ((NULL != ret->ipv6[0]) && (0 == memcmp(ret->ipv6[0], ip, 16)))
			return ret;

		ret = ret->next;
	}
	return NULL;
};
#endif

p_fabric_host_node get_fabric_host_from_list_by_sw_port(UINT8 dpid, UINT4 port)
{
	p_fabric_host_node ret = NULL;
	ret = g_fabric_host_list.list;
	while(ret != NULL)
	{
		if ((NULL != ret->sw) && (ret->sw->dpid == dpid) && (ret->port == port))
			return ret;
		ret = ret->next;
	}
	return NULL;
};

/*
 * get the host object by mac address
 * if not found, return NULL
 * UINT1* mac : host's mac address
 */
p_fabric_host_node get_fabric_host_from_list_by_mac(UINT1* mac){
	p_fabric_host_node ret = NULL;
	ret = g_fabric_host_list.list;
	while(ret != NULL){
		if(0 == memcmp(ret->mac, mac, 6)){
			return ret;
		}
		ret = ret->next;
	}
	return NULL;
};
/*
 * add a node with host info to the list
 * p_fabric_host_node node : host's info node
 */
void insert_fabric_host_into_list(p_fabric_host_node node){
	pthread_mutex_lock(&g_fabric_host_thread_mutex);
	if(node != NULL){
		node->next = g_fabric_host_list.list;
		g_fabric_host_list.list = node;
		g_fabric_host_list.list_num++;
	}
//	printf("NUM of hosts:%d\n",g_fabric_host_list.list_num);
	pthread_mutex_unlock(&g_fabric_host_thread_mutex);
	return;
};
/*
 * add a node with host info to the list by parameters
 * create a node first
 * gn_switch_t* sw: the switch is the host connect to
 * UINT4 port:		the port is the host connect to switch's port
 * UINT* mac:		the host's mac address
 * UINT4 ip:		the host's ip address
 */
p_fabric_host_node insert_fabric_host_into_list_paras(gn_switch_t* sw,UINT8 dpid,UINT4 port,UINT1* mac,UINT4 ip,UINT1* ipv6){
	p_fabric_host_node node = NULL;
	pthread_mutex_lock(&g_fabric_host_thread_mutex);
	node = create_fabric_host_list_node(sw,port,mac,ip,ipv6);
	if(node != NULL){
		node->next = g_fabric_host_list.list;
		g_fabric_host_list.list = node;
		g_fabric_host_list.list_num++;
	}
	pthread_mutex_unlock(&g_fabric_host_thread_mutex);
	return node;
};
/*
 * remove the host object by ip address
 * if not found, return NULL
 * UINT4 ip : host's ip address
 */
p_fabric_host_node remove_fabric_host_from_list_by_ip(UINT4 ip){
	t_fabric_host_node sentinel;
	p_fabric_host_node p_sentinel = &sentinel, ret=NULL;
	pthread_mutex_lock(&g_fabric_host_thread_mutex);
	p_sentinel->next = g_fabric_host_list.list;
	while(p_sentinel->next != NULL){
		if(check_IP_in_fabric_host( p_sentinel->next,ip))
		{
			ret = p_sentinel->next;
			p_sentinel->next = ret->next;
			ret->next = NULL;
			g_fabric_host_list.list = sentinel.next;
			g_fabric_host_list.list_num--;
			return ret;
		}
		p_sentinel = p_sentinel->next;
	}
	g_fabric_host_list.list = sentinel.next;
	pthread_mutex_unlock(&g_fabric_host_thread_mutex);
	return ret;
};
/*
 * remove the host object by mac address
 * if not found, return NULL
 * UINT1* mac : host's mac address
 */
p_fabric_host_node remove_fabric_host_from_list_by_mac(UINT1* mac){
	t_fabric_host_node sentinel;
	p_fabric_host_node p_sentinel = &sentinel, ret=NULL;
	pthread_mutex_lock(&g_fabric_host_thread_mutex);
	p_sentinel->next = g_fabric_host_list.list;
	while(p_sentinel->next != NULL){
		if(0 == memcmp(p_sentinel->next->mac, mac, 6)){
			ret = p_sentinel->next;
			p_sentinel->next = ret->next;
			ret->next = NULL;
			g_fabric_host_list.list = sentinel.next;
			g_fabric_host_list.list_num--;
			return ret;
		}
		p_sentinel = p_sentinel->next;
	}
	g_fabric_host_list.list = sentinel.next;
	pthread_mutex_unlock(&g_fabric_host_thread_mutex);
	return ret;
};
/*
 * delete the host object by sw
 */
void delete_fabric_host_from_list_by_sw(gn_switch_t* sw){
	t_fabric_host_node sentinel;
	p_fabric_host_node p_sentinel = &sentinel,temp_host=NULL;

	pthread_mutex_lock(&g_fabric_host_thread_mutex);
	p_sentinel->next = g_fabric_host_list.list;
	while(p_sentinel->next != NULL){
		if(p_sentinel->next->sw == sw){

			temp_host = p_sentinel->next;
			p_sentinel->next = temp_host->next;
			temp_host->next = NULL;
			delete_fabric_host_list_node(temp_host);
		}else{
			p_sentinel = p_sentinel->next;
		}
	}
	g_fabric_host_list.list = sentinel.next;
	pthread_mutex_unlock(&g_fabric_host_thread_mutex);
	return;
};
void destroy_fabric_host_list(){
	if(g_fabric_host_list_mem_id != NULL){
		mem_destroy(g_fabric_host_list_mem_id);
		g_fabric_host_list_mem_id = NULL;
	}
	return;
};
UINT4 is_fabric_host_list_empty(){
	return g_fabric_host_list.list_num;
};
/////////////////////////////////////////////////////////////
/*
 * create a host queue node
 * gn_switch_t* sw: the switch is the host connect to
 * UINT4 port:		the port is the host connect to switch's port
 * UINT* mac:		the host's mac address
 * UINT4 ip:		the host's ip address
 */
p_fabric_host_node create_fabric_host_queue_node(gn_switch_t* sw,UINT4 port,UINT1* mac,UINT4 ip){
	p_fabric_host_node ret = NULL;
	ret = mem_get(g_fabric_host_queue_mem_id);
	if (NULL != ret) {
		ret->sw = sw;
		ret->port = port;
		ret->ip_count=0;
		add_fabric_host_ip(ret,ip);
		memcpy(ret->mac, mac, 6);
	}
	else {
		LOG_PROC("ERROR", "Create fabric host queue node: Can't get memory");
	}
	return ret;
};
void delete_fabric_host_queue_node(p_fabric_host_node node){
	// free the space
	if(NULL != node){
		mem_free(g_fabric_host_queue_mem_id,node);
	}
	return;
};

////////////////////////////////////////////////////////////////////////
void init_fabric_host_queue(){
	if(g_fabric_host_queue_mem_id != NULL){
		mem_destroy(g_fabric_host_queue_mem_id);
	}
	g_fabric_host_queue_mem_id = mem_create(sizeof(t_fabric_host_node), FABRIC_HOST_QUEUE_MAX_NUM);
	g_fabric_host_queue.head = NULL;
	g_fabric_host_queue.rear = NULL;
	g_fabric_host_queue.queue_num = 0;
	return;
};
void push_fabric_host_into_queue(p_fabric_host_node node){
	pthread_mutex_lock(&g_fabric_host_thread_mutex);
	if(node != NULL){

		if(g_fabric_host_queue.queue_num == 0){
			g_fabric_host_queue.head = node;
		}
		else{
			g_fabric_host_queue.rear->next = node;
		}
		g_fabric_host_queue.rear = node;
		g_fabric_host_queue.queue_num++;
	}
	pthread_mutex_unlock(&g_fabric_host_thread_mutex);
	return;
};
p_fabric_host_node pop_fabric_host_from_queue(){
	p_fabric_host_node ret = NULL;
	pthread_mutex_lock(&g_fabric_host_thread_mutex);
	if(g_fabric_host_queue.queue_num > 0){
		ret = g_fabric_host_queue.head;
		g_fabric_host_queue.head = ret->next;
		g_fabric_host_queue.queue_num--;
		if(g_fabric_host_queue.queue_num == 0){
			g_fabric_host_queue.rear = NULL;
		}
	}
	pthread_mutex_unlock(&g_fabric_host_thread_mutex);
	return ret;
};
p_fabric_host_node get_head_fabric_host_from_queue(){
	return g_fabric_host_queue.head;
};
void destroy_fabric_host_queue(){
	if(g_fabric_host_queue_mem_id != NULL){
		mem_destroy(g_fabric_host_queue_mem_id);
		g_fabric_host_queue_mem_id = NULL;
	}
	return;
};
UINT4 is_fabric_host_queue_empty(){
	return g_fabric_host_queue.queue_num;
};
////////////////////////////////////////////////////////
// ARP request functions
////////////////////////////////////////////////////////
/*
 * create an arp node
 * p_fabric_host_node src:	the host which send the request
 * UINT4 ip:				the arp request's ip address
 */
p_fabric_arp_request_node create_fabric_arp_request_list_node(p_fabric_host_node src,UINT4 src_IP,UINT4 dst_IP){

	p_fabric_arp_request_node ret = NULL;
	ret = (p_fabric_arp_request_node)mem_get(g_fabric_arp_request_list_mem_id);
	if (NULL != ret) {
		ret->src_req = src;
		ret->dst_IP = dst_IP;
		ret->src_IP=src_IP;
	}
	else {
		LOG_PROC("ERROR", "Create fabric arp request list node: Can't get memory.");
	}
	return ret;
};
/*
 * delete a arp node
 * return the next node
 * p_fabric_arp_request_node node : the node need to delete
 */
p_fabric_arp_request_node delete_fabric_arp_request_list_node(p_fabric_arp_request_node node){
	// initialize the variables
	p_fabric_arp_request_node ret = NULL;

	// free the space and get next node
	if(NULL != node){
		ret = node->next;
		mem_free(g_fabric_arp_request_list_mem_id,node);
	}
	return ret;
};
/*
 * initialize the fabric arp list which store the arp request nodes
 * delete all nodes and set the number to zero
 */
void init_fabric_arp_request_list(){
	if(g_fabric_arp_request_list_mem_id != NULL){
		mem_destroy(g_fabric_arp_request_list_mem_id);
	}
	g_fabric_arp_request_list_mem_id = mem_create(sizeof(t_fabric_arp_request_node), FABRIC_ARP_REQUEST_LIST_MAX_NUM);
	g_arp_request_list.list = NULL;
	g_arp_request_list.list_num = 0;
	return;
};
//p_fabric_arp_request_node get_fabric_arp_request_by_dstip(UINT4 dst_ip);
/*
 * add a node with arp info to the list
 * p_fabric_arp_request_node node : arp's info node
 */
void insert_fabric_arp_request_into_list(p_fabric_arp_request_node node){
	pthread_mutex_lock(&g_fabric_arp_request_thread_mutex);
	if(node != NULL){
		node->next = g_arp_request_list.list;
		g_arp_request_list.list = node;
		g_arp_request_list.list_num++;
	}
	pthread_mutex_unlock(&g_fabric_arp_request_thread_mutex);
	return;
};
/*
 * remove all arp object by ip address
 * if not found, return NULL
 * UINT4 dst_ip : arp request's ip address
 */
p_fabric_arp_request_node remove_fabric_arp_request_from_list_by_dstip(UINT4 dst_ip){
	t_fabric_arp_request_node sentinel,ret_sentinel;
	p_fabric_arp_request_node p_sentinel = NULL,p_ret_sentinel=NULL;

	pthread_mutex_lock(&g_fabric_arp_request_thread_mutex);
	p_sentinel = &sentinel;
	p_ret_sentinel = &ret_sentinel;
	p_sentinel->next = g_arp_request_list.list;
	p_ret_sentinel->next = NULL;
	while(p_sentinel->next != NULL){
		if(p_sentinel->next->dst_IP == dst_ip){
			p_ret_sentinel->next = p_sentinel->next;
			p_sentinel->next = p_ret_sentinel->next->next;
			p_ret_sentinel = p_ret_sentinel->next;
			p_ret_sentinel->next = NULL;
			g_arp_request_list.list_num--;
		}else{
			p_sentinel = p_sentinel->next;
		}
	}
	g_arp_request_list.list = sentinel.next;
	p_sentinel = ret_sentinel.next;
	pthread_mutex_unlock(&g_fabric_arp_request_thread_mutex);
	return p_sentinel;
};

p_fabric_arp_request_node check_fabric_arp_request_exist(UINT4 src_ip, UINT4 dst_ip)
{
	p_fabric_arp_request_node node_p = g_arp_request_list.list;
	while (node_p) {
		if ((node_p->src_IP == src_ip) && (node_p->dst_IP == dst_ip)) {
			return node_p;
		}
		node_p = node_p->next;
	}
	return NULL;
}

void fabric_add_into_arp_request(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip)
{
	INT1* value = get_value(g_controller_configure, "[openvstack_conf]", "arp_debug_on");
	INT4 flag_arp_debug = (NULL == value) ? 0: atoi(value);
	
	if ((flag_arp_debug) && (NULL == check_fabric_arp_request_exist(sendip, targetip))) {
		p_fabric_arp_request_node arp_node = create_fabric_arp_request_list_node(src_port,sendip,targetip);
		insert_fabric_arp_request_into_list(arp_node);
	}
}

void destroy_fabric_arp_request_list(){
	if(g_fabric_arp_request_list_mem_id != NULL){
		mem_destroy(g_fabric_arp_request_list_mem_id);
		g_fabric_arp_request_list_mem_id = NULL;
	}
	return;
};
////////////////////////////////////////////////////////
// arp flood queue functions
////////////////////////////////////////////////////////
/*
 * create a arp flood node
 * packet_in_info_t * packet_in_info : packet object
 * UINT4 ip: search host's ip
 */
p_fabric_arp_flood_node create_fabric_arp_flood_node(packet_in_info_t * packet_in_info,UINT4 ip){

	p_fabric_arp_flood_node ret = NULL;
	//arp_t *arp_data = NULL;
	if(packet_in_info != NULL){
		ret = (p_fabric_arp_flood_node)mem_get(g_fabric_arp_flood_queue_mem_id);
		//arp_data =  (arp_t *)(packet_in_info->data);
		if (NULL != ret) {
			ret->ip = ip;
			//memcpy(&ret->arp_data,arp_data, sizeof(arp_t));
			memcpy(&ret->packet_in_info,packet_in_info, sizeof(packet_in_info_t));
			//ret->packet_in_info.data = (UINT1*)&ret->arp_data;
		}
		else {
			LOG_PROC("ERROR", "Create fabric arp flood node: Can't get memory.");
		}
	}
	return ret;
};
/*
 * delete a arp flood node
 * return null
 * p_fabric_flood_node node : the node need to delete
 */
void delete_fabric_arp_flood_node(p_fabric_arp_flood_node node){
	// free the space
	if(NULL != node){
		mem_free(g_fabric_arp_flood_queue_mem_id,node);
	}
	return;
};
/*
 * initialize the fabric arp flood queue which store the arp flood nodes
 * delete all nodes and set the number to zero
 */
void init_fabric_arp_flood_queue(){

	if(g_fabric_arp_flood_queue_mem_id != NULL){
		mem_destroy(g_fabric_arp_flood_queue_mem_id);
	}
	g_fabric_arp_flood_queue_mem_id = mem_create(sizeof(t_fabric_arp_flood_node), FABRIC_ARP_FLOOD_QUEUE_MAX_NUM);
	g_arp_flood_queue.head = NULL;
	g_arp_flood_queue.rear = NULL;
	g_arp_flood_queue.queue_num = 0;
	return;
};
/*
 * add a flood node to the rear
 */
void push_fabric_arp_flood_into_queue(p_fabric_arp_flood_node node){
	pthread_mutex_lock(&g_fabric_arp_flood_thread_mutex);
	if(node != NULL){

		if(g_arp_flood_queue.queue_num == 0)
		{
			g_arp_flood_queue.head = node;
		}
		else
		{
			g_arp_flood_queue.rear->next = node;
		}
		g_arp_flood_queue.rear = node;
		g_arp_flood_queue.queue_num++;
	}
	pthread_mutex_unlock(&g_fabric_arp_flood_thread_mutex);
	return;
};
/*
 * remove the head fabric node
 */
p_fabric_arp_flood_node pop_fabric_arp_flood_from_queue(){
	p_fabric_arp_flood_node ret = NULL;
	pthread_mutex_lock(&g_fabric_arp_flood_thread_mutex);
	if(g_arp_flood_queue.queue_num > 0){
		ret = g_arp_flood_queue.head;
		g_arp_flood_queue.head = ret->next;
		g_arp_flood_queue.queue_num--;
		if(g_arp_flood_queue.queue_num == 0){
			g_arp_flood_queue.rear = NULL;
		}
	}
	pthread_mutex_unlock(&g_fabric_arp_flood_thread_mutex);
	return ret;
};
/*
 * find the arp flood node by the ip(which is searching)
 */
p_fabric_arp_flood_node get_fabric_arp_flood_from_queue_by_ip(UINT4 ip){
	p_fabric_arp_flood_node ret = NULL;
	ret = g_arp_flood_queue.head;
	while(ret != NULL){
		if(ret->ip == ip){
			return ret;
		}
		ret = ret->next;
	}
	return NULL;
};
/*
 * get head node
 */
p_fabric_arp_flood_node get_head_fabric_arp_flood_from_queue(){
	return g_arp_flood_queue.head;
};

void destory_fabric_arp_flood_queue(){
	if(g_fabric_arp_flood_queue_mem_id != NULL){
		mem_destroy(g_fabric_arp_flood_queue_mem_id);
		g_fabric_arp_flood_queue_mem_id = NULL;
	}
	return;
};
UINT4 is_fabric_arp_flood_queue_empty(){
	return g_arp_flood_queue.queue_num;
};
////////////////////////////////////////////////////////
// ip flood functions
////////////////////////////////////////////////////////
/*
 * create a ip flood node
 * packet_in_info_t * packet_in_info : packet object
 * UINT4 ip: search host's ip
 */
p_fabric_ip_flood_node create_fabric_ip_flood_node(packet_in_info_t * packet_in_info,UINT4 ip){

	p_fabric_ip_flood_node ret = NULL;
	ip_t *ip_data = NULL;
	if(packet_in_info != NULL){
		ret = (p_fabric_ip_flood_node)mem_get(g_fabric_ip_flood_queue_mem_id);
		if (NULL != ret) {
			ip_data =  (ip_t *)(packet_in_info->data);
			ret->ip = ip;
			memcpy(&ret->ip_data,ip_data, sizeof(ip_t));
			memcpy(&ret->packet_in_info,packet_in_info, sizeof(packet_in_info_t));
			ret->packet_in_info.data = (UINT1*)&ret->ip_data;
		}
		else {
			LOG_PROC("ERROR", "Create fabric ip flood node: Can't get memory.");
		}
	}
	return ret;
};
/*
 * delete a ip flood node
 * return null
 * p_fabric_flood_node node : the node need to delete
 */
void delete_fabric_ip_flood_node(p_fabric_ip_flood_node node){
	// free the space
	if(NULL != node){
		mem_free(g_fabric_ip_flood_queue_mem_id,node);
	}
	return;
};
/*
 * initialize the fabric ip flood queue which store the ip flood nodes
 * delete all nodes and set the number to zero
 */
void init_fabric_ip_flood_queue(){

	if(g_fabric_ip_flood_queue_mem_id != NULL){
		mem_destroy(g_fabric_ip_flood_queue_mem_id);
	}
	g_fabric_ip_flood_queue_mem_id = mem_create(sizeof(t_fabric_ip_flood_node), FABRIC_IP_FLOOD_QUEUE_MAX_NUM);
	g_ip_flood_queue.head = NULL;
	g_ip_flood_queue.rear = NULL;
	g_ip_flood_queue.queue_num = 0;
	return;
};
/*
 * add a flood node to the rear
 */
void push_fabric_ip_flood_into_queue(p_fabric_ip_flood_node node){
	pthread_mutex_lock(&g_fabric_ip_flood_thread_mutex);
	if(node != NULL){

		if(g_ip_flood_queue.queue_num == 0)
		{
			g_ip_flood_queue.head = node;
		}
		else
		{
			g_ip_flood_queue.rear->next = node;
		}
		g_ip_flood_queue.rear = node;
		g_ip_flood_queue.queue_num++;
	}
	pthread_mutex_unlock(&g_fabric_ip_flood_thread_mutex);
	return;
};
/*
 * remove the head fabric node
 */
p_fabric_ip_flood_node pop_fabric_ip_flood_from_queue(){
	p_fabric_ip_flood_node ret = NULL;
	pthread_mutex_lock(&g_fabric_ip_flood_thread_mutex);
	if(g_ip_flood_queue.queue_num > 0){
		ret = g_ip_flood_queue.head;
		g_ip_flood_queue.head = ret->next;
		g_ip_flood_queue.queue_num--;
		if(g_ip_flood_queue.queue_num == 0){
			g_ip_flood_queue.rear = NULL;
		}
	}
	pthread_mutex_unlock(&g_fabric_ip_flood_thread_mutex);
	return ret;
};
/*
 * find the ip flood node by the ip(which is searching)
 */
p_fabric_ip_flood_node get_fabric_ip_flood_from_queue_by_ip(UINT4 ip){
	p_fabric_ip_flood_node ret = NULL;
	ret = g_ip_flood_queue.head;
	while(ret != NULL){
		if(ret->ip == ip){
			return ret;
		}
		ret = ret->next;
	}
	return NULL;
};
/*
 * get head node
 */
p_fabric_ip_flood_node get_head_fabric_ip_flood_from_queue(){
	return g_ip_flood_queue.head;
};

void destory_fabric_ip_flood_queue(){
	if(g_fabric_ip_flood_queue_mem_id != NULL){
		mem_destroy(g_fabric_ip_flood_queue_mem_id);
		g_fabric_ip_flood_queue_mem_id = NULL;
	}
	return;
};
UINT4 is_fabric_ip_flood_queue_empty(){
	return g_ip_flood_queue.queue_num;
};
////////////////////////////////////////////////////////
// flow queue functions
////////////////////////////////////////////////////////
/*
 * create a flow node
 */
p_fabric_flow_node create_fabric_flow_node(p_fabric_host_node src_host,
		UINT4 src_IP,
		UINT4 src_tag,
		p_fabric_host_node dst_host,
		UINT4 dst_IP,
		UINT4 dst_tag){
	p_fabric_flow_node ret = NULL;
	ret = (p_fabric_flow_node)mem_get(g_fabric_flow_queue_mem_id);
	if (NULL != ret) {
		ret->src_host = src_host;
		ret->src_tag = src_tag;
		ret->src_IP=src_IP;
		ret->dst_host = dst_host;
		ret->dst_tag = dst_tag;
		ret->dst_IP=dst_IP;
	}
	else {
		LOG_PROC("ERROR", "Fabric Flow node: Create failed, Can't get memory.");
	}

	return ret;

};
/*
 * create a flow node
 * return null
 */
void delete_fabric_flow_node(p_fabric_flow_node node){
	// free the space
	if(NULL != node){
		mem_free(g_fabric_flow_queue_mem_id,node);
	}
	return;
};

/*
 * initialize the fabric flow queue which store the fabric flow nodes
 * delete all nodes and set the number to zero
 */
void init_fabric_flow_queue(){
	if(g_fabric_flow_queue_mem_id != NULL){
		mem_destroy(g_fabric_flow_queue_mem_id);
	}
	g_fabric_flow_queue_mem_id = mem_create(sizeof(t_fabric_flow_node), FABRIC_FLOW_QUEUE_MAX_NUM);
	g_fabric_flow_queue.head = NULL;
	g_fabric_flow_queue.rear = NULL;
	g_fabric_flow_queue.queue_num = 0;
	return;
};
/*
 * add a fabric flow node to the rear
 */
void push_fabric_flow_into_queue(p_fabric_flow_node node){
	pthread_mutex_lock(&g_fabric_flow_thread_mutex);
	if(node != NULL){

		if(g_fabric_flow_queue.queue_num == 0)
		{
			g_fabric_flow_queue.head = node;
		}
		else
		{
			g_fabric_flow_queue.rear->next = node;
		}
		g_fabric_flow_queue.rear = node;
		g_fabric_flow_queue.queue_num++;
	}
	pthread_mutex_unlock(&g_fabric_flow_thread_mutex);
	return;
};
/*
 * remove the head fabric flow node
 */
p_fabric_flow_node pop_fabric_flow_from_queue(){
	p_fabric_flow_node ret = NULL;
	pthread_mutex_lock(&g_fabric_flow_thread_mutex);
	if(g_fabric_flow_queue.queue_num > 0){
		ret = g_fabric_flow_queue.head;
		g_fabric_flow_queue.head = ret->next;
		g_fabric_flow_queue.queue_num--;
		if(g_fabric_flow_queue.queue_num == 0){
			g_fabric_flow_queue.rear = NULL;
		}
	}
	pthread_mutex_unlock(&g_fabric_flow_thread_mutex);
	return ret;
};
/*
 * find the flow flood node by src & dst host
 */
p_fabric_flow_node get_fabric_flow_from_queue(p_fabric_host_node src_host,UINT4 src_IP,p_fabric_host_node dst_host,UINT4 dst_IP){
	p_fabric_flow_node ret = NULL;
	ret = g_fabric_flow_queue.head;
	while(ret != NULL){
		if(  (ret->src_host == src_host && ret->src_IP == src_IP
				&& ret->dst_host == dst_host && ret->dst_IP == dst_IP) ||
				(ret->src_host == dst_host && ret->src_IP == dst_IP
				 && ret->dst_host == src_host && ret->dst_IP == src_IP)  )
		{
			return ret;
		}
		ret = ret->next;
	}
	return NULL;
};
/*
 * get head node
 */
p_fabric_flow_node get_head_fabric_flow_from_queue(){
	return g_fabric_flow_queue.head;
};

void destroy_fabric_flow_queue(){
	if(g_fabric_flow_queue_mem_id != NULL){
		mem_destroy(g_fabric_flow_queue_mem_id);
		g_fabric_flow_queue_mem_id = NULL;
	}
	return;
};
UINT4 is_fabric_flow_queue_empty(){
	return g_fabric_flow_queue.queue_num;
};


/*add new ip*/
void add_fabric_host_ip(p_fabric_host_node node,UINT4 newIP)
{
	if(node->ip_count<FABRIC_HOST_IP_MAX_NUM)
	{
		node->ip_list[node->ip_count++]=newIP;
		//printf("%s : new ip:%s \n",FN,inet_htoa(ntohl(newIP)));
	}
};

/*check IP in the host ip list*/
BOOL check_IP_in_fabric_host(p_fabric_host_node node,UINT4 IP)
{
	int i;
	for(i=0;i<node->ip_count;i++)
	   if(node->ip_list[i]==IP)
		   return TRUE;
	return FALSE;
}

void set_fabric_host_port_portno(const UINT1 *mac, UINT4 ofport_no)
{
	p_fabric_host_node ret = NULL;
	ret = g_fabric_host_list.list;
	pthread_mutex_lock(&g_fabric_host_thread_mutex);
	while(ret != NULL)
	{
		 if(memcmp(ret->mac, mac, 6) == 0)
		{
			 ret->port = ofport_no;
			return;
		}
		ret = ret->next;
	}
	pthread_mutex_unlock(&g_fabric_host_thread_mutex);
}

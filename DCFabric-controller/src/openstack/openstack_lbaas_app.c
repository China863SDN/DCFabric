/*
 * openstack_lbaas_app.c
 *
 *  Created on: 1 7, 2016
 *      Author: yang
 */
#include "openstack_lbaas_app.h"
#include "mem_pool.h"
#include "stdlib.h"
#include "timer.h"
#include "openstack_app.h"
#include "fabric_openstack_arp.h"
#include "../restful-svr/openstack-server.h"
#include "../cluster-mgr/hbase_sync.h"
#include "openflow-common.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "fabric_floating_ip.h"


#define LBAAS_CONNECT_PORT_MIN_VALUE		59200
#define LBAAS_CONNECT_PORT_MAX_VALUE		59700

void *g_openstack_lbaas_pools_id = NULL;
void *g_openstack_lbaas_members_id = NULL;
void *g_openstack_lbaas_listener_id = NULL;
void *g_openstack_lbaas_node_id = NULL;
void *g_openstack_lbaas_connect_id = NULL;
void *g_openstack_lbaas_listener_member_id = NULL;

openstack_lbaas_node_p g_openstack_lbaas_pools_list = NULL;
openstack_lbaas_node_p g_openstack_lbaas_members_list = NULL;
openstack_lbaas_node_p g_openstack_lbaas_listener_list = NULL;

openstack_lbaas_node_p last_used_member_node = NULL;

UINT4 g_test_flag = 0;

/* internal function */
void add_node_into_list(void* data, enum lbass_node_type node_type);
void remove_node_from_list(void* data, enum lbass_node_type node_type);
void remove_all_node_from_list();

openstack_lbaas_pools_p create_openstack_lbaas_pools_by_poolrest(
		char* tenant_id,
		char* pool_id,
		UINT1 protocol,
		UINT1 status,
		UINT1 lbaas_method);

openstack_lbaas_pools_p create_openstack_lbaas_pools_by_viprest(
		char* pool_id,
		UINT4 protocol_port,
		UINT4 ipaddress,
		UINT1 connect_limit,
		UINT1 vips_status,
		UINT1 session_persistence);

openstack_lbaas_members_p create_openstack_lbaas_member(
		char* member_id,
		char* tenant_id,
		char* pool_id,
		UINT1 weight,
		UINT4 protocol_port,
		UINT1 status,
		UINT4 fixed_ip);

openstack_lbaas_listener_p create_openstack_lbaas_listener(
		char* listener_id,
		UINT1 type,
		UINT4 check_frequency,
		UINT4 overtime,
		UINT1 retries);

void destory_openstack_lbaas_pool_node(openstack_lbaas_node_p lb_pools);
void destory_openstack_lbaas_member_node(openstack_lbaas_node_p node);
void destory_openstack_lbaas_listener_node(openstack_lbaas_node_p node);

openstack_lbaas_pools_p find_openstack_lbaas_pool_by_pool_id(char* pool_id);
openstack_lbaas_members_p find_openstack_lbaas_member_by_member_id(char* member_id);
openstack_lbaas_listener_p find_openstack_lbaas_listener_by_listener_id(char* listener_id);

void create_icmp_packet(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT1* dst_mac, gn_switch_t* sw, UINT4 outport,
						UINT1 type, UINT1 code, UINT2 id, UINT2 seq);

void* lbaas_listener_thread(void* param);
openstack_lbaas_listener_member_p create_lbaas_listener_member_listener(openstack_lbaas_listener_p listener_p);

UINT4 get_random_weight(UINT4 weight,UINT4 weight_count);
openstack_lbaas_members_p get_lbass_member_by_method_round_robin(char* pool_id);
openstack_lbaas_members_p get_lbass_member_by_method_least_connection(char* pool_id);

openstack_lbaas_members_p get_lbaas_member_by_pool_id(char* pool_id,UINT1 lbaas_method);

openstack_lbaas_connect_p find_openstack_lbass_connect_by_extip(UINT4 ext_ip, UINT4 inside_ip, UINT4 vip, UINT4 src_port_no);
openstack_lbaas_connect_p find_openstack_lbass_connect_by_extip_portno(UINT4 ext_ip, UINT4 src_port_no);

void add_lbaas_member_into_listener_member_listener(openstack_lbaas_members_p member_p);


void show_all_lbaas();
void reset_floating_lbaas_group_flow_installed_flag(openstack_lbaas_members_p lb_member);

void remove_floating_lbaas_member_flow(openstack_lbaas_members_p lb_member);



// this function is called to initialize the mem pool
void init_openstack_lbaas()
{
	if (g_openstack_lbaas_pools_id != NULL) {
		mem_destroy(g_openstack_lbaas_pools_id);
		g_openstack_lbaas_pools_id = NULL;
	}
	g_openstack_lbaas_pools_id = mem_create(sizeof(openstack_lbaas_pools), OPENSTACK_LBAAS_POOLS_MAX_NUM);

	if (g_openstack_lbaas_members_id != NULL) {
		mem_destroy(g_openstack_lbaas_members_id);
		g_openstack_lbaas_members_id = NULL;
	}
	g_openstack_lbaas_members_id = mem_create(sizeof(openstack_lbaas_members), OPENSTACK_LBAAS_MEMBERS_MAX_NUM);
	if (g_openstack_lbaas_listener_id != NULL) {
		mem_destroy(g_openstack_lbaas_listener_id);
		g_openstack_lbaas_listener_id = NULL;
	}
	g_openstack_lbaas_listener_id = mem_create(sizeof(openstack_lbaas_listener), OPENSTACK_LBAAS_LISTENER_MAX_NUM);
	if (g_openstack_lbaas_node_id != NULL) {
		mem_destroy(g_openstack_lbaas_node_id);
		g_openstack_lbaas_node_id = NULL;
	}
	g_openstack_lbaas_node_id = mem_create(sizeof(openstack_lbaas_node), OPENSTACK_LBAAS_NODE_MAX_NUM);

	if (g_openstack_lbaas_connect_id != NULL) {
		mem_destroy(g_openstack_lbaas_connect_id);
	}
	g_openstack_lbaas_connect_id = mem_create(sizeof(openstack_lbaas_connect_t), OPENSTACK_LBAAS_CONNECT_MAX_NUM);

	if (g_openstack_lbaas_listener_member_id != NULL) {
		mem_destroy(g_openstack_lbaas_listener_member_id);
	}
	g_openstack_lbaas_listener_member_id = mem_create(sizeof(openstack_lbaas_listener_member_t), OPENSTACK_LBAAS_LISTENER_MEMBER_MAX_NUM);

	g_openstack_lbaas_pools_list = NULL;
	g_openstack_lbaas_members_list = NULL;
	g_openstack_lbaas_listener_list = NULL;
	return;
}

// this function is called to destroy the mem pool
void destory_openstack_lbaas()
{
	if (g_openstack_lbaas_pools_id != NULL) {
		mem_destroy(g_openstack_lbaas_pools_id);
		g_openstack_lbaas_pools_id = NULL;
	}
	if (g_openstack_lbaas_members_id != NULL) {
		mem_destroy(g_openstack_lbaas_members_id);
		g_openstack_lbaas_members_id = NULL;
	}
	if (g_openstack_lbaas_listener_id != NULL) {
		mem_destroy(g_openstack_lbaas_listener_id);
		g_openstack_lbaas_listener_id = NULL;
	}
	if (g_openstack_lbaas_node_id != NULL) {
		mem_destroy(g_openstack_lbaas_node_id);
		g_openstack_lbaas_node_id = NULL;
	}
	if (g_openstack_lbaas_connect_id != NULL) {
		mem_destroy(g_openstack_lbaas_connect_id);
		g_openstack_lbaas_connect_id = NULL;
	}

	if (g_openstack_lbaas_listener_member_id != NULL) {
		mem_destroy(g_openstack_lbaas_listener_member_id);
		g_openstack_lbaas_listener_member_id = NULL;
	}

	g_openstack_lbaas_pools_list = NULL;
	g_openstack_lbaas_members_list = NULL;
	g_openstack_lbaas_listener_list = NULL;
	return;
}

// this function is used to create node to save data and add into specific list
void add_node_into_list(void* data, enum lbass_node_type node_type)
{
	openstack_lbaas_node_p* list_p = NULL;

	switch (node_type) {
		case LBAAS_NODE_POOL:
			list_p = &g_openstack_lbaas_pools_list;
			break;
		case LBAAS_NODE_MEMBER:
			list_p = &g_openstack_lbaas_members_list;
			break;
		case LBAAS_NODE_LISTENER:
			list_p = &g_openstack_lbaas_listener_list;
			break;
		case LBAAS_NODE_POOL_MEMBER:
		{
			openstack_lbaas_members_p member_p = (openstack_lbaas_members_p)data;
			openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_pool_id(member_p->pool_id);
			if (pool_p) {
				list_p = &pool_p->pool_member_list;
				pool_p->weight_count += member_p->weight;
			}
		}
		break;
		case LBAAS_NODE_CONNECT:
		{
			openstack_lbaas_connect_p connect_p = (openstack_lbaas_connect_p)data;
			openstack_lbaas_members_p member_p = find_openstack_lbaas_member_by_ip(connect_p->vip, connect_p->inside_ip);
			if (member_p) {
				list_p = &member_p->connect_ips;
			}
		}
		break;
		default:
			break;
	}

	openstack_lbaas_node_p node_p = (openstack_lbaas_node_p)mem_get(g_openstack_lbaas_node_id);
	if (NULL != node_p) {
		node_p->data = data;
		node_p->next = *list_p;
		*list_p = node_p;
	}
}

// this function is used to remove data from specific list
void remove_node_from_list(void* data, enum lbass_node_type node_type)
{
	openstack_lbaas_node_p* list_p = NULL;

	switch (node_type) {
		case LBAAS_NODE_POOL:
			list_p = &g_openstack_lbaas_pools_list;
			break;
		case LBAAS_NODE_MEMBER:
			list_p = &g_openstack_lbaas_members_list;
			break;
		case LBAAS_NODE_LISTENER:
			list_p = &g_openstack_lbaas_listener_list;
			break;
		case LBAAS_NODE_POOL_MEMBER:
		{
			openstack_lbaas_members_p member_p = (openstack_lbaas_members_p)data;
			openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_pool_id(member_p->pool_id);
			if (pool_p) {
				list_p = &pool_p->pool_member_list;
			}
		}
		break;
		case LBAAS_NODE_CONNECT:
		{
			openstack_lbaas_connect_p connect_p = (openstack_lbaas_connect_p)data;
			openstack_lbaas_members_p member_p = find_openstack_lbaas_member_by_ip(connect_p->vip, connect_p->inside_ip);
			if (member_p) {
				list_p = &member_p->connect_ips;
				member_p->connect_numbers--;
			}
		}
		break;
		default:
			break;
	}

	openstack_lbaas_node_p** head_p = &list_p;
	openstack_lbaas_node_p prev_p = **head_p;
	openstack_lbaas_node_p temp_p = NULL;
	openstack_lbaas_node_p next_p = NULL;

	if (prev_p->data == data) {
		**head_p = prev_p->next;
		temp_p = prev_p;
		mem_free(g_openstack_lbaas_node_id, temp_p);
		temp_p = NULL;
	}
	else {
		next_p = prev_p->next;

		while (next_p) {
			if (next_p->data == data) {
				temp_p = next_p;
				prev_p->next = next_p->next;
				mem_free(g_openstack_lbaas_node_id, temp_p);
				temp_p = NULL;
			}
			prev_p = prev_p->next;
			if (prev_p) {
				next_p = prev_p->next;
			}
			else {
				next_p = NULL;
			}
		}
	}
}

// this functon is called to remove all node from specific list
void remove_all_node_from_list()
{
	// TBD
}

// this function is used to create lbaas poool by pool id
openstack_lbaas_pools_p create_openstack_lbaas_pools_by_poolrest(
		char* tenant_id,
		char* pool_id,
		UINT1 protocol,
		UINT1 status,
		UINT1 lbaas_method)
{
	openstack_lbaas_pools_p ret = NULL;
	ret = (openstack_lbaas_pools_p)mem_get(g_openstack_lbaas_pools_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_lbaas_pools));
		strcpy(ret->tenant_id, tenant_id);
		strcpy(ret->pools_id, pool_id);
		ret->protocol = protocol;
		ret->status = status;
		ret->lbaas_method = lbaas_method;
		//TODO
	}
	else {
		LOG_PROC("ERROR", "Create openstack lbaas pools: Can't get memory.");
	}
	return ret;
};

// this function is used to create lbaas pool by vip
openstack_lbaas_pools_p create_openstack_lbaas_pools_by_viprest(
		char* pool_id,
		UINT4 protocol_port,
		UINT4 ipaddress,
		UINT1 connect_limit,
		UINT1 vips_status,
		UINT1 session_persistence)
{
	openstack_lbaas_pools_p ret = NULL;
	ret = (openstack_lbaas_pools_p)mem_get(g_openstack_lbaas_pools_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_lbaas_pools));
		strcpy(ret->pools_id, pool_id);
		ret->protocol_port = protocol_port;
		ret->vips_status = vips_status;
		ret->ipaddress = ipaddress;
		ret->connect_limit = connect_limit;
		ret->session_persistence = session_persistence;
	}
	else {
		LOG_PROC("ERROR", "Create openstack lbaas pools: Can't get memory.");
	}
	return ret;
};

INT4 compare_openstack_lbaas_pools_by_viprest(
		char* pool_id,
		UINT4 protocol_port,
		UINT4 ipaddress,
		UINT1 connect_limit,
		UINT1 vips_status,
		UINT1 session_persistence,
		openstack_lbaas_pools_p pool_p)
{
	if ((pool_p)
		&& compare_str(pool_id, pool_p->pools_id)
		&& (protocol_port == pool_p->protocol_port)
		&& (vips_status == pool_p->vips_status)
		&& (ipaddress == pool_p->ipaddress)
		&& (connect_limit == pool_p->connect_limit)
		&& (session_persistence == pool_p->session_persistence)) {
		return 1;
	}

	return 0;
}

INT4 compare_openstack_lbaas_pool_by_poolrest(
		char* tenant_id,
		char* pool_id,
		UINT1 status,
		UINT1 protocol,
		UINT1 lbaas_method,
		openstack_lbaas_pools_p pool_p)
{
	if ((pool_p)
		&& compare_str(tenant_id, pool_p->tenant_id)
		&& compare_str(pool_id, pool_p->pools_id)
		&& (status == pool_p->status)
		&& (protocol == pool_p->protocol)
		&& (lbaas_method == pool_p->lbaas_method)) {
		return 1;
	}

	return 0;
}

// this function is used to update lbaas pool by pool id
openstack_lbaas_pools_p update_openstack_lbaas_pool_by_poolrest(
		char* tenant_id,
		char* pool_id,
		UINT1 status,
		UINT1 protocol,
		UINT1 lbaas_method)
{
	openstack_lbaas_pools_p lb_pool = NULL;
	lb_pool = find_openstack_lbaas_pool_by_pool_id(pool_id);
	if(lb_pool == NULL) {
		lb_pool = create_openstack_lbaas_pools_by_poolrest(tenant_id,pool_id,status,protocol,lbaas_method);
		if (lb_pool) {
			add_node_into_list(lb_pool, LBAAS_NODE_POOL);
			lb_pool->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_lbaas_pool_by_poolrest(tenant_id,pool_id,status,protocol,lbaas_method,lb_pool)) {
		lb_pool->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		strcpy(lb_pool->tenant_id, tenant_id);
		strcpy(lb_pool->pools_id, pool_id);
		lb_pool->status = status;
		lb_pool->protocol = protocol;
		lb_pool->lbaas_method = lbaas_method;
		lb_pool->check_status = (UINT2)CHECK_UPDATE;
	}
	return lb_pool;
};

// this function is used to update lbaas pool by vip
openstack_lbaas_pools_p update_openstack_lbaas_pool_by_viprest(
		char* pool_id,
		UINT4 protocol_port,
		UINT4 ipaddress,
		UINT1 connect_limit,
		UINT1 vips_status,
		UINT1 session_persistence)
{
	openstack_lbaas_pools_p lb_pool = NULL;
	lb_pool = find_openstack_lbaas_pool_by_pool_id(pool_id);

	if(lb_pool == NULL) {
		lb_pool = create_openstack_lbaas_pools_by_viprest(pool_id,protocol_port,ipaddress,connect_limit,vips_status,session_persistence);
		if (lb_pool) {
			add_node_into_list((void*)lb_pool, LBAAS_NODE_POOL);
			lb_pool->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_lbaas_pools_by_viprest(pool_id,protocol_port,ipaddress,
								connect_limit,vips_status,session_persistence,lb_pool)) {
		lb_pool->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		strcpy(lb_pool->pools_id, pool_id);
		lb_pool->protocol_port = protocol_port;
		lb_pool->vips_status = vips_status;
		lb_pool->ipaddress = ipaddress;
		lb_pool->connect_limit = connect_limit;
		lb_pool->session_persistence = session_persistence;
		lb_pool->check_status = (UINT2)CHECK_UPDATE;
	}
	return lb_pool;
};

// this function is used to create lbaas member
openstack_lbaas_members_p create_openstack_lbaas_member(
		char* member_id,
		char* tenant_id,
		char* pool_id,
		UINT1 weight,
		UINT4 protocol_port,
		UINT1 status,
		UINT4 fixed_ip)
{
	openstack_lbaas_members_p ret = NULL;
	ret = (openstack_lbaas_members_p)mem_get(g_openstack_lbaas_members_id);

	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_lbaas_members));
		strcpy(ret->member_id, member_id);
		strcpy(ret->tenant_id, tenant_id);
		strcpy(ret->pool_id, pool_id);
		ret->weight = weight;
		ret->protocol_port = protocol_port;
		ret->status = status;
		ret->fixed_ip = fixed_ip;
	}
	else {
		LOG_PROC("ERROR", "Create openstack lbaas member: Can't get memory.");
	}
	return ret;
};

INT4 compare_openstack_lbaas_member(
		char* member_id,
		char* tenant_id,
		char* pool_id,
		UINT1 weight,
		UINT4 protocol_port,
		UINT1 status,
		UINT4 fixed_ip,
		openstack_lbaas_members_p member_p)
{
	// printf("1.%s,%s,%s,%d,%d,%d,%d\n",member_id,tenant_id,pool_id,weight,protocol_port,status,fixed_ip);
	// printf("2.%s,%s,%s,%d,%d,%d,%d\n",member_p->member_id,member_p->tenant_id,member_p->pool_id,member_p->weight,
	//	member_p->protocol_port,member_p->status,member_p->fixed_ip);

	if ((member_p)
		&& compare_str(member_id, member_p->member_id)
		&& compare_str(tenant_id, member_p->tenant_id)
		&& compare_str(pool_id, member_p->pool_id)
		&& (weight == member_p->weight)
		&& (protocol_port == member_p->protocol_port)
		// && (status == member_p->status)
		&& (fixed_ip == member_p->fixed_ip)) {
		return 1;
	}

	return 0;
}



// this function is used to update lbaas member
openstack_lbaas_members_p update_openstack_lbaas_member_by_rest(
		char* member_id,
		char* tenant_id,
		char* pool_id,
		UINT1 weight,
		UINT4 protocol_port,
		UINT1 status,
		UINT4 fixed_ip)
{
	openstack_lbaas_members_p lb_member = NULL;
	lb_member = find_openstack_lbaas_member_by_member_id(member_id);

	if (lb_member == NULL) {
		lb_member = create_openstack_lbaas_member(member_id,tenant_id,pool_id,weight,protocol_port,status,fixed_ip);
		if (lb_member) {
			add_node_into_list((void*)lb_member, LBAAS_NODE_MEMBER);
			add_node_into_list((void*)lb_member, LBAAS_NODE_POOL_MEMBER);
			add_lbaas_member_into_listener_member_listener(lb_member);
			lb_member->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_lbaas_member(member_id,tenant_id,pool_id,weight,protocol_port,status,fixed_ip,lb_member)) {
		lb_member->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		strcpy(lb_member->member_id, member_id);
		strcpy(lb_member->tenant_id, tenant_id);
		strcpy(lb_member->pool_id, pool_id);
		lb_member->weight = weight;
		lb_member->protocol_port = protocol_port;
		lb_member->status = status;
		lb_member->fixed_ip = fixed_ip;
		lb_member->check_status = (UINT2)CHECK_UPDATE;
	}

    if (is_check_status_changed(lb_member->check_status)) {
        reset_floating_lbaas_group_flow_installed_flag(lb_member);
    }
	
	return lb_member;
};

// this function is used to crate lbaas listener
openstack_lbaas_listener_p create_openstack_lbaas_listener(
		char* listener_id,
		UINT1 type,
		UINT4 check_frequency,
		UINT4 overtime,
		UINT1 retries)
{
	openstack_lbaas_listener_p ret = NULL;
	ret = (openstack_lbaas_listener_p)mem_get(g_openstack_lbaas_listener_id);

	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_lbaas_listener));
		strcpy(ret->listener_id, listener_id);
		ret->type = type;
		ret->check_frequency = check_frequency;
		ret->overtime = overtime;
		ret->retries = retries;
	}
	else {
		LOG_PROC("ERROR", "Create openstack lbaas listener: Can't get memory.");
	}
	return ret;
};

INT4 compare_openstack_lbaas_listener(
		char* listener_id,
		UINT1 type,
		UINT4 check_frequency,
		UINT4 overtime,
		UINT1 retries,
		openstack_lbaas_listener_p listener_p)
{
	if ((listener_p)
		&& compare_str(listener_id, listener_p->listener_id)
		&& (type == listener_p->type)
		&& (check_frequency == listener_p->check_frequency)
		&& (overtime == listener_p->overtime)
		&& (retries == listener_p->retries)) {
		return 1;
	}

	return 0;
}

// this function to update lbaas listener
openstack_lbaas_listener_p update_openstack_lbaas_listener_by_rest(
		char* listener_id,
		UINT1 type,
		UINT4 check_frequency,
		UINT4 overtime,
		UINT1 retries)
{
	openstack_lbaas_listener_p lb_listener = NULL;
	lb_listener = find_openstack_lbaas_listener_by_listener_id(listener_id);

	if (lb_listener == NULL) {
		lb_listener = create_openstack_lbaas_listener(listener_id,type,check_frequency,overtime,retries);
		if (lb_listener) {
			add_node_into_list((void*)lb_listener, LBAAS_NODE_LISTENER);
			lb_listener->listener_member_list = create_lbaas_listener_member_listener(lb_listener);
			lb_listener->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_lbaas_listener(listener_id,type,check_frequency,overtime,retries,lb_listener)) {
		lb_listener->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		strcpy(lb_listener->listener_id, listener_id);
		lb_listener->type = type;
		lb_listener->check_frequency = check_frequency;
		lb_listener->overtime = overtime;
		lb_listener->retries = retries;
		lb_listener->check_status = (UINT2)CHECK_UPDATE;
	}
	
	return lb_listener;
};

// destroy lbaas pool node
void destory_openstack_lbaas_pool_node(openstack_lbaas_node_p node)
{
	mem_free(g_openstack_lbaas_pools_id,node);
	return;
};

// destroy lbaas member node
void destory_openstack_lbaas_member_node(openstack_lbaas_node_p node)
{
	mem_free(g_openstack_lbaas_members_id,node);
	return;
};

// destroy listener node
void destory_openstack_lbaas_listener_node(openstack_lbaas_node_p node)
{
	mem_free(g_openstack_lbaas_listener_id,node);
	return;
};

// this function is used to find lbaas pool by pool id
openstack_lbaas_pools_p find_openstack_lbaas_pool_by_pool_id(char* pool_id)
{
	openstack_lbaas_pools_p lb_pool = NULL;
	openstack_lbaas_node_p node_p = g_openstack_lbaas_pools_list;

	while(node_p != NULL) {
		lb_pool = (openstack_lbaas_pools_p)node_p->data;
		if ((NULL != lb_pool) && (strcmp(lb_pool->pools_id,pool_id) == 0)) {
			return lb_pool;
		}
		node_p = node_p->next;
	}
	return NULL;
};

// this function is used to find lbaas pool by ip
openstack_lbaas_pools_p find_openstack_lbaas_pool_by_ip(UINT4 ip)
{
	openstack_lbaas_pools_p lb_pool = NULL;
	openstack_lbaas_node_p node_p = g_openstack_lbaas_pools_list;

	while(node_p != NULL) {
		lb_pool = (openstack_lbaas_pools_p)node_p->data;
		if ((NULL != lb_pool) && (lb_pool->ipaddress == ip)) {
			return lb_pool;
		}
		node_p = node_p->next;
	}
	return NULL;
};

// this function is used to remove pool by pool id
openstack_lbaas_pools_p remove_openstack_lbaas_pool_by_pool_id(char* pool_id)
{
	openstack_lbaas_pools_p lb_pool = NULL;
	openstack_lbaas_node node;
	openstack_lbaas_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_lbaas_pools_list;

	while(node_p->next != NULL){
		lb_pool = (openstack_lbaas_pools_p)(node_p->next->data);
		if ((NULL!= lb_pool) && (strcmp(lb_pool->pools_id,pool_id) == 0)) {
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_lbaas_pool_node(temp_p);
			g_openstack_lbaas_pools_list = node.next;
			return lb_pool;
		}
		node_p = node_p->next;
	}
	g_openstack_lbaas_pools_list = node.next;
	return NULL;
};

// this function is used to find lbaas member by ip
openstack_lbaas_members_p find_openstack_lbaas_member_by_ip(UINT4 vip, UINT4 inside_ip)
{
	openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_ip(vip);
	if (NULL == pool_p) {
		return NULL;
	}
	openstack_lbaas_node_p node_p = pool_p->pool_member_list;
	openstack_lbaas_members_p member_p = NULL;
	while (node_p) {
		 member_p = (openstack_lbaas_members_p)node_p->data;

		 if ((member_p) && (member_p->fixed_ip == inside_ip)) {
			 return member_p;
		 }
		 node_p = node_p->next;
	}

	return NULL;
};

// this function is used to find lbaas member by member id
openstack_lbaas_members_p find_openstack_lbaas_member_by_member_id(char* member_id)
{
	openstack_lbaas_members_p lb_member = NULL;
	openstack_lbaas_node_p node_p = g_openstack_lbaas_members_list;

	while(node_p != NULL) {
		lb_member = (openstack_lbaas_members_p)node_p->data;
		if ((NULL != lb_member) && (strcmp(lb_member->member_id,member_id) == 0)) {
			return lb_member;
		}
		node_p = node_p->next;
	}
	return NULL;
};

// this function is used to remove lbaas member by member id
openstack_lbaas_members_p remove_openstack_lbaas_member_by_member_id(char* member_id)
{
	openstack_lbaas_members_p lb_member = NULL;
	openstack_lbaas_node node;
	openstack_lbaas_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_lbaas_members_list;

	while(node_p->next != NULL) {
		lb_member = (openstack_lbaas_members_p)(node_p->next->data);
		if ((NULL!= lb_member) && (strcmp(lb_member->member_id,member_id) == 0)) {
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_lbaas_member_node(temp_p);
			g_openstack_lbaas_members_list = node.next;
			return lb_member;
		}
		node_p = node_p->next;
	}
	g_openstack_lbaas_members_list = node.next;
	return NULL;
};

// this function is used to find lbaas listener by id
openstack_lbaas_listener_p find_openstack_lbaas_listener_by_listener_id(char* listener_id)
{
	openstack_lbaas_listener_p lb_listener = NULL;
	openstack_lbaas_node_p node_p = g_openstack_lbaas_listener_list;
	while(node_p != NULL) {
		lb_listener = (openstack_lbaas_listener_p)node_p->data;
		if ((NULL != lb_listener) && (strcmp(lb_listener->listener_id,listener_id) == 0)) {
			return lb_listener;
		}
		node_p = node_p->next;
	}
	return NULL;
};

// this function is used to remove lbaas listener by listener id
openstack_lbaas_listener_p remove_openstack_lbaas_listener_by_listener_id(char* listener_id)
{
	openstack_lbaas_listener_p lb_listener = NULL;
	openstack_lbaas_node node;
	openstack_lbaas_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_lbaas_listener_list;

	while (node_p->next != NULL) {
		lb_listener = (openstack_lbaas_listener_p)(node_p->next->data);
		if ((NULL!= lb_listener) && (strcmp(lb_listener->listener_id,listener_id) == 0)) {
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_lbaas_listener_node(temp_p);
			g_openstack_lbaas_listener_list = node.next;
			return lb_listener;
		}
		node_p = node_p->next;
	}
	g_openstack_lbaas_listener_list = node.next;
	return NULL;
};

// this function is used to get lbaas ip by ip and proto
UINT4 get_openstack_lbaas_ip_by_ip_proto(UINT4 ip, UINT1 proto)
{
	/*
	 * temp code! not judge protocol! only support TCP!
	 */
	if (IPPROTO_TCP != proto) {
		return 0;
	}

	openstack_lbaas_pools_p lb_pool = NULL;
	openstack_lbaas_node_p node_p = g_openstack_lbaas_pools_list;

	while (node_p) {
		lb_pool = (openstack_lbaas_pools_p)node_p->data;

		if ((lb_pool) && (ip == lb_pool->ipaddress) /*&& (proto == lb_pool->protocol)*/) {
			openstack_lbaas_members_p member_p = get_lbaas_member_by_pool_id(lb_pool->pools_id, lb_pool->lbaas_method);
			if (member_p) {
				return member_p->fixed_ip;
			}
		}
		node_p = node_p->next;
	}
	return 0;
}

// this function is used to create icmp packet and packet out
void create_icmp_packet(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT1* dst_mac, gn_switch_t* sw, UINT4 outport,
						UINT1 type, UINT1 code, UINT2 id, UINT2 seq)
{
	packet_in_info_t packout_req_info;
	UINT1 data_len = sizeof(ip_t) + sizeof(icmp_t);

	ip_t* new_ip = (ip_t*)malloc(data_len);
	memset(new_ip, 0, data_len);
	icmp_t icmp_pkt;
	icmp_t* new_icmp = &icmp_pkt;
	memset(new_icmp, 0, sizeof(icmp_t));

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = -3;
	packout_req_info.xid = 0;
	packout_req_info.data_len = data_len;
	packout_req_info.data = (UINT1 *)new_ip;

	new_icmp->type = type;
	new_icmp->code = code;
	new_icmp->id = id;
	new_icmp->seq = seq;
	new_icmp->cksum = 0;
	new_icmp->cksum = calc_ip_checksum((UINT2*)&new_icmp->type, sizeof(icmp_t));

	memcpy(new_ip->eth_head.src, src_mac, 6);
	memcpy(new_ip->eth_head.dest, dst_mac, 6);
	new_ip->eth_head.proto = ntohs(0x0800);

	new_ip->hlen = 0x45;
	new_ip->ttl = 64;
	new_ip->len = ntohs(data_len - sizeof(ether_t));
	new_ip->src = src_ip;
	new_ip->dest = dst_ip;
	new_ip->ipid = ntohs(0x02);
	new_ip->fragoff = 0;
	new_ip->proto = IPPROTO_ICMP;
	new_ip->cksum = 0;
	new_ip->cksum = calc_ip_checksum((UINT2*)&new_ip->hlen, sizeof(ip_t));
	memcpy(new_ip->data, new_icmp, sizeof(icmp_t));

	fabric_openstack_packet_output(sw, &packout_req_info, outport);

	free(new_ip);
}

void create_tcp_packet(UINT4 src_ip, UINT4 dst_ip, UINT2 dst_port, UINT1* src_mac, UINT1* dst_mac, 
    gn_switch_t* sw, UINT4 outport, UINT2 seq, UINT2 code)
{
    packet_in_info_t packout_req_info;
    
    UINT1 data_len = sizeof(ip_t) + sizeof(tcp_t);

    ip_t* new_ip = (ip_t*)malloc(data_len);
    memset(new_ip, 0, data_len);

    tcp_t tcp_pkt;
    tcp_t* new_tcp = &tcp_pkt;
    memset(new_tcp, 0, sizeof(tcp_t));

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = -3;
    packout_req_info.xid = 0;
    packout_req_info.data_len = data_len+sizeof(packet_in_info_t);
    packout_req_info.data = (UINT1 *)new_ip;

    memcpy(new_ip->eth_head.src, src_mac, 6);
	memcpy(new_ip->eth_head.dest, dst_mac, 6);
    new_ip->eth_head.proto = ntohs(0x0800);   
    new_ip->hlen = 0x45;
    new_ip->ttl = 64;
    new_ip->len = ntohs(data_len - sizeof(ether_t));
    new_ip->src = src_ip;
    new_ip->dest = dst_ip;
    new_ip->ipid = ntohs(0x02);
    new_ip->fragoff = 0;
    new_ip->proto = IPPROTO_TCP;
    new_ip->cksum = 0;
    new_ip->cksum = calc_ip_checksum((UINT2*)&new_ip->hlen, sizeof(ip_t));

    new_tcp->sport = ntohs(12345);
    new_tcp->dport = ntohs(dst_port);
    new_tcp->seq = ntohl(seq);
    new_tcp->ack = 0;
    new_tcp->offset = 0x50;
    new_tcp->code = code;
    new_tcp->window = ntohs(65535);
    new_tcp->urg = ntohs(0);
    new_tcp->cksum = calc_tcp_checksum(new_ip, new_tcp);

    memcpy(new_ip->data, new_tcp, sizeof(tcp_t));

    fabric_openstack_packet_output(sw, &packout_req_info, outport);

    free(new_ip);

}

void create_tcp_rst_packet(UINT4 src_ip, UINT4 dst_ip, UINT2 dst_port, UINT1* src_mac, UINT1* dst_mac, 
    gn_switch_t* sw, UINT4 outport, UINT2 seq)
{
   create_tcp_packet(src_ip, dst_ip, dst_port, src_mac, dst_mac, sw, outport, seq, 0x04);
}

void create_tcp_syn_packet(UINT4 src_ip, UINT4 dst_ip, UINT2 dst_port, UINT1* src_mac, UINT1* dst_mac, 
    gn_switch_t* sw, UINT4 outport, UINT2 seq)
{
   create_tcp_packet(src_ip, dst_ip, dst_port, src_mac, dst_mac, sw, outport, seq, 0x02);
}



// this function is used monitor the member and update status
void* lbaas_listener_thread(void* param)
{
	/*
	 * This function is used to monitor members.
	 */

	openstack_lbaas_listener_p listener_p = (openstack_lbaas_listener_p)(param);
	openstack_lbaas_listener_member_p lb_p = NULL;

	if (listener_p) {
		while (1)
		{
			sleep(listener_p->check_frequency);

			pthread_mutex_lock(&listener_p->listener_mutex);
			// printf("lbaas time: %d %s\n", (INT4)g_cur_sys_time.tv_sec, listener_p->listener_id);

			lb_p = (openstack_lbaas_listener_member_p)listener_p->listener_member_list;

			while ((lb_p) && (lb_p->member_p))
			{
				// set sequence id
				lb_p->seq_id++;
				// printf("lbaas: wait status: %d request times:%d ip:%d\n", lb_p->wait_status, lb_p->request_time, lb_p->dst_ip);

				// if last request not receive response
                if (0 == lb_p->wait_status) {
                    lb_p->request_time = 0;
                    if (LBAAS_LISTENER_PING == listener_p->type)  {
                        lb_p->member_p->ping_status = LBAAS_LISTENER_MEMBER_ACTIVE;
                    }
                    else {
                        openstack_lbaas_pools_p lb_pool = find_openstack_lbaas_pool_by_pool_id(lb_p->member_p->pool_id);

                        if (lb_p->member_p->status != LBAAS_LISTENER_MEMBER_ACTIVE) {
                            lb_p->member_p->group_flow_installed = 0;
                            lb_pool->group_flow_installed = 0;
                            lb_p->member_p->status = LBAAS_LISTENER_MEMBER_ACTIVE;
                            create_proactive_floating_lbaas_flow_by_member_ip(lb_pool->ipaddress, lb_p->member_p->fixed_ip, lb_p->member_p->protocol_port);
                        }
                    }
                }
                else {
                    if (lb_p->request_time > listener_p->retries) {
                        // printf("lbaas: set status inactive: %d\n", lb_p->dst_ip);
                        if (LBAAS_LISTENER_PING == listener_p->type)  {
                            lb_p->member_p->ping_status = LBAAS_LISTENER_MEMBER_INACTIVE;
                        }
                        else {
                            openstack_lbaas_pools_p lb_pool = find_openstack_lbaas_pool_by_pool_id(lb_p->member_p->pool_id);
                            
                            if (lb_p->member_p->status != LBAAS_LISTENER_MEMBER_INACTIVE) {
                                lb_p->member_p->status = LBAAS_LISTENER_MEMBER_INACTIVE;
                                lb_pool->group_flow_installed = 0;

                                remove_proactive_floating_lbaas_flow_by_member_ip(lb_pool->ipaddress, lb_p->member_p->fixed_ip, lb_p->member_p->protocol_port);
                            }
                        }                                              
                    }
                    lb_p->request_time++;
                }

				if (NULL == lb_p->dst_port) {
					lb_p->dst_port = get_fabric_host_from_list_by_ip(lb_p->dst_ip);
				}
				else if ((lb_p->dst_port) && ((NULL == lb_p->dst_port->sw) || (0 == lb_p->dst_port->port)))	{
					p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(lb_p->dst_port);
					if (gateway_p) {
						fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], lb_p->dst_port->ip_list[0], gateway_p->mac);
					}
				}
				else {
					lb_p->init_time = (UINT4)g_cur_sys_time.tv_sec;

                    if (LBAAS_LISTENER_PING == listener_p->type) {  
                        UINT2 seq = lb_p->seq_id;
                        create_icmp_packet(ntohl(g_reserve_ip), lb_p->dst_ip, g_reserve_mac, lb_p->dst_port->mac,
                                lb_p->dst_port->sw, lb_p->dst_port->port, 8, 0, ntohs(seq), ntohs(0));
                    }
                    else if (LBAAS_LISTENER_TCP == listener_p->type) {
                        // printf("create tcp packet: id:%d, port:%d\n", lb_p->seq_id, lb_p->protocol_port);
                        create_tcp_syn_packet(ntohl(g_reserve_ip), lb_p->dst_ip, lb_p->protocol_port, g_reserve_mac, lb_p->dst_port->mac,
                              lb_p->dst_port->sw, lb_p->dst_port->port, lb_p->seq_id);
                    }
                    else if ((LBAAS_LISTENER_HTTP == listener_p->type) || (LBAAS_LISTENER_HTTPS == listener_p->type)) {
                        create_tcp_syn_packet(ntohl(g_reserve_ip), lb_p->dst_ip, 80, g_reserve_mac, lb_p->dst_port->mac,
                              lb_p->dst_port->sw, lb_p->dst_port->port, lb_p->seq_id);
                    }
                    else {
                        // do nothing
                    }
				}

				lb_p->wait_status = 1;

				lb_p = lb_p->next;
			}

			pthread_mutex_unlock(&listener_p->listener_mutex);
		}
	}
	return NULL;
}

// this function is used to update listener member status
void update_openstack_lbaas_listener_member_status(UINT1 type, p_fabric_host_node dst_port, UINT4 seq_id, UINT2 port_no, UINT1 code)
{
	openstack_lbaas_node_p list_p = g_openstack_lbaas_listener_list;
	openstack_lbaas_listener_p listener_p = NULL;
	openstack_lbaas_listener_member_p lb_p = NULL;

	while (list_p) {
		listener_p = (openstack_lbaas_listener_p)list_p->data;

		if (listener_p) {
			pthread_mutex_lock(&listener_p->listener_mutex);

			lb_p = listener_p->listener_member_list;

            // printf("%d,%d,%d,%d\n", type, seq_id, port_no, code);
            
			while (lb_p) {
                if ((LBAAS_LISTENER_PING == type) && (type == listener_p->type)) {
                    if (lb_p->init_time + listener_p->overtime >= (UINT4)g_cur_sys_time.tv_sec) {
                        if ((lb_p->dst_port == dst_port) && (lb_p->seq_id == seq_id))
                        {
                            lb_p->wait_status = 0;
                        }
                    }
                }
                else if (((LBAAS_LISTENER_TCP == type) || (LBAAS_LISTENER_HTTP == type) || (LBAAS_LISTENER_HTTPS == type))
                    && (type == listener_p->type)) {
                    if ((lb_p->dst_port == dst_port) && (lb_p->seq_id == seq_id - 1) && (lb_p->protocol_port == port_no))
                    {
                        // receive ACK
                        if (0x12 == code) {
                            lb_p->wait_status = 0;
							// create RST packet
                            create_tcp_rst_packet(ntohl(g_reserve_ip), lb_p->dst_ip, lb_p->protocol_port, g_reserve_mac, 
                                lb_p->dst_port->mac, lb_p->dst_port->sw, lb_p->dst_port->port, seq_id);
                        }
                    }
                }
                else {
                    }
                

				lb_p = lb_p->next;
			}

			pthread_mutex_unlock(&listener_p->listener_mutex);
		}

		list_p = list_p->next;
	}
}

// this function is used to start lbaas listener thread
void start_openstack_lbaas_listener()
{
	openstack_lbaas_node_p list_p = g_openstack_lbaas_listener_list;
	openstack_lbaas_listener_p listener_p = NULL;

	while (list_p) {
		listener_p = (openstack_lbaas_listener_p)list_p->data;

		if (listener_p) {
			pthread_create(&(listener_p->listener_pid), NULL, lbaas_listener_thread, (void*)listener_p);
			pthread_mutex_init(&listener_p->listener_mutex, NULL);
		}
		list_p = list_p->next;
	}
}

openstack_lbaas_listener_member_p create_listener_member(openstack_lbaas_members_p member_p)
{
	openstack_lbaas_listener_member_p listener_member_p = (openstack_lbaas_listener_member_p)mem_get(g_openstack_lbaas_listener_member_id);

	if (listener_member_p) {
        strcpy(listener_member_p->member_id, member_p->member_id);
        listener_member_p->protocol_port = member_p->protocol_port;
		listener_member_p->dst_ip = member_p->fixed_ip;
		listener_member_p->dst_port = get_fabric_host_from_list_by_ip(member_p->fixed_ip);
		listener_member_p->init_time = g_cur_sys_time.tv_sec;
		listener_member_p->request_time = 0;
		listener_member_p->member_p = member_p;
		listener_member_p->member_p->status = LBAAS_LISTENER_MEMBER_INITIALIZE;
        listener_member_p->wait_status = 1;
	}

	return listener_member_p;
}


void add_lbaas_member_into_listener_member_listener(openstack_lbaas_members_p member_p)
{
	openstack_lbaas_node_p list_p =  g_openstack_lbaas_listener_list;
	while (list_p) {
		openstack_lbaas_listener_p listener_p = (openstack_lbaas_listener_p)list_p->data;

		if (listener_p) {
			openstack_lbaas_listener_member_p listener_member_p = listener_p->listener_member_list;

			UINT1 check_exist = GN_OK;
			while (listener_member_p) {
				if (0 == strcmp(member_p->member_id, listener_member_p->member_id)) {
					check_exist = GN_ERR;
				}
				listener_member_p = listener_member_p->next;
			}

			if (GN_OK == check_exist)
			{
				openstack_lbaas_listener_member_p listener_new_member_p = create_listener_member(member_p);

				if (listener_new_member_p) {
					listener_new_member_p->next = listener_p->listener_member_list;
					listener_p->listener_member_list = listener_new_member_p;
				}
			}
		}
		list_p = list_p->next;
	}
}


// create lbaas listener member
openstack_lbaas_listener_member_p create_lbaas_listener_member_listener(openstack_lbaas_listener_p listener_p)
{
	openstack_lbaas_node_p member_list_p = g_openstack_lbaas_members_list;
	openstack_lbaas_members_p member_p = NULL;
	openstack_lbaas_listener_member_p listener_member_p = NULL;

	if (listener_p) {
		while (member_list_p) {
			member_p = (openstack_lbaas_members_p)member_list_p->data;

			if (member_p) {
				listener_member_p = create_listener_member(member_p);

				// add_node_into_list()
				if (listener_member_p) {
					listener_member_p->next = listener_p->listener_member_list;
					listener_p->listener_member_list = listener_member_p;
				}
			}

			member_list_p = member_list_p->next;
		}
	}
	return listener_member_p;
}


// this function is used to get random weight
UINT4 get_random_weight(UINT4 weight,UINT4 weight_count)
{
	int rand_value = 0;
	if (weight_count) {
		rand_value = random()%weight_count + 1 + weight;
		// printf("random:%d, current: %d, all: %d\n", rand_value, weight, weight_count);
	}

	return rand_value;
}

// this function is used to get lbaas member by round robin
openstack_lbaas_members_p get_lbass_member_by_method_round_robin(char* pool_id)
{
	openstack_lbaas_members_p return_p = NULL;
	openstack_lbaas_node_p node_p = NULL;

	openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_pool_id(pool_id);

	if ((NULL == pool_p) || (NULL == pool_p->pool_member_list)) {
		LOG_PROC("INFO", "Can't get lbaas pool member list: %s  ", pool_id);
		return NULL;
	}

	if (NULL == pool_p->last_round_robin_member) {
		pool_p->last_round_robin_member = pool_p->pool_member_list;
	}

	node_p = pool_p->last_round_robin_member->next;

	while (node_p != pool_p->last_round_robin_member) {
		if (NULL == node_p) {
			node_p = pool_p->pool_member_list;
		}

		return_p = (openstack_lbaas_members_p)node_p->data;

		if (LBAAS_LISTENER_MEMBER_INACTIVE != return_p->status) {
			pool_p->last_round_robin_member = node_p;
			break;
		}

		node_p = node_p->next;
	}

	if ((node_p) && (node_p == pool_p->last_round_robin_member)) {
		return_p = (openstack_lbaas_members_p)node_p->data;
	}

	if ((return_p) && (LBAAS_LISTENER_MEMBER_ACTIVE!= return_p->status)) {
	    return_p = NULL;
    }

	return return_p;
}

// this function is used to get lbaas member by round robin
openstack_lbaas_members_p get_lbass_member_by_method_least_connection(char* pool_id)
{
	openstack_lbaas_node_p node_p = NULL;
	openstack_lbaas_members_p lb_member = NULL;
	openstack_lbaas_members_p least_member = NULL;

	UINT1 least_connect_number = 0;
	UINT4 least_weight_check = 0;
	UINT4 least_double_weight_check = 0;
	UINT1 lbaas_weight_count = 0;

	openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_pool_id(pool_id);

	if (pool_p) {
		lbaas_weight_count = pool_p->weight_count;
		node_p = pool_p->pool_member_list;

		while (node_p) {
			lb_member = (openstack_lbaas_members_p)node_p->data;

			if (LBAAS_LISTENER_MEMBER_INACTIVE == lb_member->status) {
				node_p = node_p->next;
				continue;
			}

			least_connect_number = lb_member->connect_numbers;
			least_member = lb_member;
			break;
		}

		while (node_p) {
			lb_member = (openstack_lbaas_members_p)node_p->data;

			if (lb_member) {
				UINT4 temp_weight_check = get_random_weight(lb_member->weight, lbaas_weight_count);
				UINT4 temp_double_weight_check = get_random_weight(1, 10000);
				if (LBAAS_LISTENER_MEMBER_INACTIVE == lb_member->status) {
					node_p = node_p->next;
					continue;
				}

				if (lb_member->connect_numbers < least_connect_number) {
					least_connect_number = lb_member->connect_numbers;
					least_weight_check = temp_weight_check;
					least_double_weight_check = temp_double_weight_check;
					least_member = lb_member;
				}
				else if (lb_member->connect_numbers == least_connect_number) {
					if (temp_weight_check > least_weight_check) {
						least_weight_check = temp_weight_check;
						least_double_weight_check = temp_double_weight_check;
						least_member = lb_member;
					}
					else if (temp_weight_check == least_weight_check) {
						if (temp_double_weight_check > least_double_weight_check) {
							least_double_weight_check = temp_double_weight_check;
							least_member = lb_member;
						}
					}
				}
				else {
					// do nothing
				}
			}
			node_p = node_p->next;
		}
	}

    if ((least_member) && (LBAAS_LISTENER_MEMBER_ACTIVE!= least_member->status)) {
        least_member = NULL;
    }    

	return least_member;
}

// get member by pool id and method
openstack_lbaas_members_p get_lbaas_member_by_pool_id(char* pool_id,UINT1 lbaas_method)
{
	openstack_lbaas_members_p lb_member = NULL;

	if (lbaas_method == LB_M_ROUNT_ROBIN) {
		lb_member = get_lbass_member_by_method_round_robin(pool_id);
	}
	else if (lbaas_method == LB_M_LEAST_CONNECTIONS) {
		lb_member = get_lbass_member_by_method_least_connection(pool_id);
	}
	else {
		LOG_PROC("INFO", "Not support lb method");
	}
	return lb_member;
}

// this function is used to create openstack lbaas connect
UINT4 create_openstack_lbaas_connect(UINT4 ext_ip, UINT4 inside_ip, UINT4 vip, UINT4 src_port_no)
{
	openstack_lbaas_connect_p find_p = find_openstack_lbass_connect_by_extip(ext_ip, inside_ip, vip, src_port_no);

	if (NULL == find_p) {
		openstack_lbaas_members_p member_p = find_openstack_lbaas_member_by_ip(vip, inside_ip);

		if (member_p) {
			openstack_lbaas_node_p current_node_p = member_p->connect_ips;
			openstack_lbaas_node_p prev_node_p = NULL;
			UINT4 current_max_port_no = LBAAS_CONNECT_PORT_MAX_VALUE + 1;

			while(NULL != current_node_p)
			{
				openstack_lbaas_connect_p current_p = (openstack_lbaas_connect_p)current_node_p->data;
				if (current_p->ext_port_no != current_max_port_no - 1) {
					break;
				}
				else
				{
					current_max_port_no = current_p->ext_port_no;
					prev_node_p = current_node_p;
					current_node_p = current_node_p->next;
				}
			}

			// printf("**** create no: %d *****\n", current_max_port_no - 1);

			if (LBAAS_CONNECT_PORT_MIN_VALUE >= current_max_port_no) {
				LOG_PROC("INFO", "create lbaas connect port");
				return 0;
			}

			find_p = (openstack_lbaas_connect_p)mem_get(g_openstack_lbaas_connect_id);

			if (NULL != find_p) {
				memset(find_p, 0, sizeof(openstack_lbaas_connect_t));
				find_p->ext_ip = ext_ip;
				find_p->inside_ip = inside_ip;
				find_p->vip = vip;
				find_p->ext_port_no = current_max_port_no - 1;
				find_p->src_port_no = src_port_no;

				// add_node_into_list((void*)find_p, LBAAS_NODE_CONNECT);

				openstack_lbaas_node_p node_p = (openstack_lbaas_node_p)mem_get(g_openstack_lbaas_node_id);
				if (NULL != node_p) {
					node_p->data = (void*)find_p;
					if (prev_node_p) {
						node_p->next = current_node_p;
						prev_node_p->next = node_p;
					}
					else {
						node_p->next = member_p->connect_ips;
						member_p->connect_ips = node_p;
					}
					member_p->connect_numbers++;
				}
                
                if (g_controller_role == OFPCR_ROLE_MASTER)
                {
                    persist_fabric_openstack_lbaas_members_single(OPERATE_ADD, find_p);
                }

				return find_p->ext_port_no;
			}
			else {
				LOG_PROC("INFO", "create lbaas connect failed, Can't get memory");
			}
		}
	}
	return 0;
}

// this function is used to find lbaas connect by ip
openstack_lbaas_connect_p find_openstack_lbass_connect_by_extip(UINT4 ext_ip, UINT4 inside_ip, UINT4 vip, UINT4 src_port_no)
{
	openstack_lbaas_members_p member_p = find_openstack_lbaas_member_by_ip(vip, inside_ip);

	if (member_p) {
		openstack_lbaas_node_p node_p = member_p->connect_ips;

		while (node_p) {
			openstack_lbaas_connect_p connect_p = (openstack_lbaas_connect_p)node_p->data;
			if ((connect_p->ext_ip == ext_ip) && (connect_p->src_port_no == src_port_no)) {
				return connect_p;
			}
			node_p = node_p->next;
		}
	}

	return NULL;
}

// this function is used to find connect by ip and port no
openstack_lbaas_connect_p find_openstack_lbass_connect_by_extip_portno(UINT4 ext_ip, UINT4 src_port_no)
{
	openstack_lbaas_members_p member_p = NULL;
	openstack_lbaas_node_p list_p = g_openstack_lbaas_members_list;

	while (list_p) {
		member_p = (openstack_lbaas_members_p)list_p->data;

		if (member_p) {
		openstack_lbaas_node_p node_p = member_p->connect_ips;

		while (node_p) {
			openstack_lbaas_connect_p connect_p = (openstack_lbaas_connect_p)node_p->data;
			if ((connect_p->ext_ip == ext_ip) && (connect_p->src_port_no == src_port_no)) {
				return connect_p;
			}
			node_p = node_p->next;
			}
		}
		list_p = list_p->next;
	}

	return NULL;
}

// this function is used to remove lbaas connect by ip
void remove_openstack_lbaas_connect(UINT4 ext_ip, UINT4 inside_ip, UINT4 vip, UINT4 src_port_no)
{
	openstack_lbaas_connect_p connect_p = find_openstack_lbass_connect_by_extip(ext_ip, inside_ip, vip, src_port_no);

	if (connect_p) 
    {
        
        if (g_controller_role == OFPCR_ROLE_MASTER)
        {
            persist_fabric_openstack_lbaas_members_single(OPERATE_DEL, connect_p);
        }
		remove_node_from_list((void*)connect_p, LBAAS_NODE_CONNECT);
	}
}

// this function is used to remove lbaas connect by ip and port no
void remove_openstack_lbaas_connect_by_ext_ip_portno(UINT4 ext_ip, UINT4 src_port_no)
{
	openstack_lbaas_connect_p connect_p = find_openstack_lbass_connect_by_extip_portno(ext_ip, src_port_no);

	if (connect_p) 
    {
        if (g_controller_role == OFPCR_ROLE_MASTER)
        {
            persist_fabric_openstack_lbaas_members_single(OPERATE_DEL, connect_p);
        }
		remove_node_from_list((void*)connect_p, LBAAS_NODE_CONNECT);
	}
}

void rest_openstack_lbaas_update_flag()
{
	openstack_lbaas_pools_p lb_pool = NULL;
	openstack_lbaas_node_p node_p = g_openstack_lbaas_pools_list;

	while(node_p != NULL) {
		lb_pool = (openstack_lbaas_pools_p)node_p->data;
		lb_pool->check_status = (UINT2)CHECK_UNCHECKED;
		node_p = node_p->next;
	}
	
	openstack_lbaas_members_p lb_member = NULL;
	node_p = g_openstack_lbaas_members_list;
	while(node_p != NULL) {
		lb_member = (openstack_lbaas_members_p)node_p->data;
		lb_member->check_status = (UINT2)CHECK_UNCHECKED;
		node_p = node_p->next;
	}

	openstack_lbaas_listener_p lb_listener = NULL;
	node_p = g_openstack_lbaas_listener_list;
	while(node_p != NULL) {
		lb_listener = (openstack_lbaas_listener_p)node_p->data;
		lb_listener->check_status = (UINT2)CHECK_UNCHECKED;
		node_p = node_p->next;
	}

}

// this function is used to remove pool by flag
void remove_openstack_lbaas_pool_by_check_status()
{
	openstack_lbaas_pools_p lb_pool = NULL;
	openstack_lbaas_node_p head_p = g_openstack_lbaas_pools_list;
	openstack_lbaas_node_p prev_p = head_p;

	if (NULL == head_p)
	{
		return ;
	}

	openstack_lbaas_node_p next_p = prev_p->next;

	while (next_p) {
		lb_pool = (openstack_lbaas_pools_p)next_p->data;
		
		if ((lb_pool) && (lb_pool->check_status == (UINT2)CHECK_UNCHECKED)) {
            // process_floating_lbaas_group_flow_by_pool(lb_pool, 2);
			prev_p->next = next_p->next;
			mem_free(g_openstack_lbaas_pools_id, lb_pool);
			mem_free(g_openstack_lbaas_node_id, next_p);
		}
		else {
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	lb_pool = (openstack_lbaas_pools_p)head_p->data;
	if ((lb_pool) && (lb_pool->check_status == (UINT2)CHECK_UNCHECKED)) {
        // process_floating_lbaas_group_flow_by_pool(lb_pool, 2);
		next_p = head_p->next;
		mem_free(g_openstack_lbaas_pools_id, head_p->data);
		mem_free(g_openstack_lbaas_node_id, head_p);
		g_openstack_lbaas_pools_list = next_p;
	}	
}

// this function is used to remove pool by flag
void remove_openstack_lbaas_pool_member_by_check_status()
{
	openstack_lbaas_pools_p lb_pool = NULL;
	openstack_lbaas_node_p list_p = g_openstack_lbaas_pools_list;
	
	while (list_p) {
		lb_pool = (openstack_lbaas_pools_p)list_p->data;
		if (lb_pool) {
			openstack_lbaas_members_p lb_member = NULL;
			openstack_lbaas_node_p head_p = lb_pool->pool_member_list;
			openstack_lbaas_node_p prev_p = head_p;
		
			if (NULL != head_p)
			{
				openstack_lbaas_node_p next_p = prev_p->next;
		
				while (next_p) {
					lb_member = (openstack_lbaas_members_p)next_p->data;

					if ((lb_member) && (lb_member->check_status== (UINT2)CHECK_UNCHECKED)) {
                        remove_floating_lbaas_member_flow(lb_member);
						prev_p->next = next_p->next;
						mem_free(g_openstack_lbaas_node_id, next_p);
					}
					else {
						prev_p = prev_p->next;
					}
					next_p = prev_p->next;
				}
			
				lb_member = (openstack_lbaas_members_p)head_p->data;
				if ((lb_member) && (lb_member->check_status== (UINT2)CHECK_UNCHECKED)) {
                    remove_floating_lbaas_member_flow(lb_member);
					next_p = head_p->next;
					mem_free(g_openstack_lbaas_node_id, head_p);
					lb_pool->pool_member_list = next_p;
				}
			}	
		}
		list_p = list_p->next;
	}
}

// this function is used to remove pool by flag
void remove_openstack_lbaas_listener_by_check_status()
{
	openstack_lbaas_listener_p lb_listener = NULL;
	openstack_lbaas_node_p head_p = g_openstack_lbaas_listener_list;
	openstack_lbaas_node_p prev_p = head_p;

	if (NULL == head_p)
	{
		return ;
	}

	openstack_lbaas_node_p next_p = prev_p->next;

	while (next_p) {
		lb_listener = (openstack_lbaas_listener_p)next_p->data;
		
		if ((lb_listener) && (lb_listener->check_status== (UINT2)CHECK_UNCHECKED)) {
			prev_p->next = next_p->next;
			mem_free(g_openstack_lbaas_listener_id, lb_listener);
			mem_free(g_openstack_lbaas_node_id, next_p);
		}
		else {
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	lb_listener = (openstack_lbaas_listener_p)head_p->data;
	if ((lb_listener) && (lb_listener->check_status== (UINT2)CHECK_UNCHECKED)) {
		next_p = head_p->next;
		mem_free(g_openstack_lbaas_listener_id, head_p->data);
		mem_free(g_openstack_lbaas_node_id, head_p);
		g_openstack_lbaas_listener_list = next_p;
	}	
}

// this function is used to remove pool by flag
void remove_openstack_lbaas_member_by_check_status()
{
	openstack_lbaas_members_p lb_member = NULL;
	openstack_lbaas_node_p head_p = g_openstack_lbaas_members_list;
	openstack_lbaas_node_p prev_p = head_p;

	if (NULL == head_p)
	{
		return ;
	}

	openstack_lbaas_node_p next_p = prev_p->next;

	while (next_p) {
		lb_member = (openstack_lbaas_members_p)next_p->data;
		
		if ((lb_member) && (lb_member->check_status== (UINT2)CHECK_UNCHECKED)) {
			prev_p->next = next_p->next;
			mem_free(g_openstack_lbaas_members_id, lb_member);
			mem_free(g_openstack_lbaas_node_id, next_p);
		}
		else {
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	lb_member = (openstack_lbaas_members_p)head_p->data;
	if ((lb_member) && (lb_member->check_status== (UINT2)CHECK_UNCHECKED)) {
		next_p = head_p->next;
		mem_free(g_openstack_lbaas_members_id, head_p->data);
		mem_free(g_openstack_lbaas_node_id, head_p);
		g_openstack_lbaas_members_list= next_p;
	}	
}

void remove_openstack_lbaas_listener_member_by_check_status()
{
	openstack_lbaas_members_p lb_member = NULL;
	openstack_lbaas_node_p list_p =  g_openstack_lbaas_listener_list;
	while (list_p) {
		openstack_lbaas_listener_p listener_p = (openstack_lbaas_listener_p)list_p->data;

		if (listener_p) {
			openstack_lbaas_listener_member_p head_p = listener_p->listener_member_list;
			openstack_lbaas_listener_member_p prev_p = head_p;

			if (head_p) {
				openstack_lbaas_listener_member_p next_p = prev_p->next;

				while (next_p) {
					lb_member = next_p->member_p;
					if ((lb_member) && (lb_member->check_status== (UINT2)CHECK_UNCHECKED)) {
						prev_p->next = next_p->next;
						mem_free(g_openstack_lbaas_listener_member_id, next_p);
					}
					else {
						prev_p = prev_p->next;
					}
	
					next_p = prev_p->next;
				}

				lb_member = head_p->member_p;
				if ((lb_member) && (lb_member->check_status== (UINT2)CHECK_UNCHECKED)) {
					next_p = head_p->next;
					mem_free(g_openstack_lbaas_listener_member_id, head_p);
					listener_p->listener_member_list = next_p;
				}
				
			}

		}
		list_p = list_p->next;
	}
}

void reset_floating_lbaas_group_flow_installed_flag(openstack_lbaas_members_p lb_member)
{
    if (lb_member) {
        openstack_lbaas_pools_p lb_pool = find_openstack_lbaas_pool_by_pool_id(lb_member->pool_id);
        
        if (lb_pool) {
            lb_pool->group_flow_installed = 0;
        }
        
        lb_member->group_flow_installed = 0;
    }
}

void remove_floating_lbaas_member_flow(openstack_lbaas_members_p lb_member)
{
    if (lb_member) {
        openstack_lbaas_pools_p lb_pool = find_openstack_lbaas_pool_by_pool_id(lb_member->pool_id);
        
        if (lb_pool) {
            lb_pool->group_flow_installed = 0;
            remove_proactive_floating_lbaas_flow_by_member_ip(lb_pool->ipaddress, lb_member->fixed_ip, lb_member->protocol_port);
        }
        
        lb_member->group_flow_installed = 0;
    }
}

void clear_openstack_lbaas_info()
{
	rest_openstack_lbaas_update_flag();	
}

void reload_openstack_lbaas_info()
{
	// this function will reload lbaas info from openstack
	clear_openstack_lbaas_info();
	reoad_lbaas_info();

	// reset upchecked flag
	remove_openstack_lbaas_pool_by_check_status();
	remove_openstack_lbaas_pool_member_by_check_status();
	remove_openstack_lbaas_listener_member_by_check_status();
	
	remove_openstack_lbaas_member_by_check_status();
	remove_openstack_lbaas_listener_by_check_status();
}

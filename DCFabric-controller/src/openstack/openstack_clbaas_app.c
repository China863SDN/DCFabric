/*
 * openstack_lbaas_app.c
 *
 *  Created on: 1 7, 2016
 *      Author: yang
 */
#include "openstack_lbaas_app.h"
#include "openstack_clbaas_app.h"
#include "mem_pool.h"
#include "stdlib.h"
#include "timer.h"
#include "openstack_app.h"
#include "fabric_flows.h"
#include "fabric_openstack_arp.h"
#include "../restful-svr/openstack-server.h"
#include "../cluster-mgr/redis_sync.h"
#include "openflow-common.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "fabric_floating_ip.h"
#include <sys/prctl.h>   


//by:yhy openstack中负载均衡服务(lbaas)的相关全局变量
/*
void *g_openstack_lbaas_pools_id = NULL;
void *g_openstack_lbaas_members_id = NULL;
void *g_openstack_lbaas_listener_id = NULL;
void *g_openstack_lbaas_node_id = NULL;
void *g_openstack_lbaas_connect_id = NULL;
void *g_openstack_lbaas_listener_member_id = NULL;
*/
extern void *g_openstack_lbaas_pools_id ;
extern void *g_openstack_lbaas_members_id ;
extern void *g_openstack_lbaas_listener_id ;
extern void *g_openstack_lbaas_node_id;

extern openstack_lbaas_node_p g_openstack_lbaas_pools_list ;
extern openstack_lbaas_node_p g_openstack_lbaas_members_list ;
extern openstack_lbaas_node_p g_openstack_lbaas_listener_list ;

extern openstack_lbaas_node_p last_used_member_node ;



void *g_openstack_clbaas_node_id;
void *g_openstack_clbaas_routersubnet_id ; 
void *g_openstack_clbaas_vipfloatingip_id ; 
void *g_openstack_clbaas_loadbalancer_id ; 
void *g_openstack_clbaas_backendip_id ; 

openstack_lbaas_node_p g_openstack_clbaas_routersubnets_list;
openstack_lbaas_node_p g_openstack_clbaas_vipfloatingips_list;
openstack_lbaas_node_p g_openstack_clbaas_loadbalancers_list;
openstack_lbaas_node_p g_openstack_clbaas_backendips_list;


void init_openstack_clbaas()
{
	if (g_openstack_clbaas_routersubnet_id != NULL) {
		mem_destroy(g_openstack_clbaas_routersubnet_id);
		g_openstack_clbaas_routersubnet_id = NULL;
	}
	g_openstack_clbaas_routersubnet_id = mem_create(sizeof(openstack_clbaas_routersubnet_t), OPENSTACK_CLBAAS_POOLS_MAX_NUM);
	if(NULL == g_openstack_clbaas_routersubnet_id)
	{
		
		LOG_PROC("ERROR", "%s %d: Can't get memory.", FN, LN);
	}
	
	if (g_openstack_clbaas_vipfloatingip_id != NULL) {
		mem_destroy(g_openstack_clbaas_vipfloatingip_id);
		g_openstack_clbaas_vipfloatingip_id = NULL;
	}
	g_openstack_clbaas_vipfloatingip_id = mem_create(sizeof(openstack_clbaas_vipfloating_t), OPENSTACK_CLBAAS_POOLS_MAX_NUM);
	if(NULL == g_openstack_clbaas_vipfloatingip_id)
	{
		
		LOG_PROC("ERROR", "%s %d: Can't get memory.", FN, LN);
	}
	
	if (g_openstack_clbaas_loadbalancer_id != NULL) {
		mem_destroy(g_openstack_clbaas_loadbalancer_id);
		g_openstack_clbaas_loadbalancer_id = NULL;
	}
	
	g_openstack_clbaas_loadbalancer_id = mem_create(sizeof(openstack_clbass_loadbalancer_t), OPENSTACK_CLBAAS_POOLS_MAX_NUM*2);
	if(NULL == g_openstack_clbaas_loadbalancer_id)
	{
		
		LOG_PROC("ERROR", "%s %d: Can't get memory.", FN, LN);
	}
	if (g_openstack_clbaas_backendip_id != NULL) {
		mem_destroy(g_openstack_clbaas_backendip_id);
		g_openstack_clbaas_backendip_id = NULL;
	}
	
	g_openstack_clbaas_backendip_id = mem_create(sizeof(openstack_clbass_backend_t), OPENSTACK_CLBAAS_MEMBERS_MAX_NUM);
	if(NULL == g_openstack_clbaas_backendip_id)
	{
		
		LOG_PROC("ERROR", "%s %d: Can't get memory.", FN, LN);
	}
	
	
	if (g_openstack_clbaas_node_id != NULL) {
		mem_destroy(g_openstack_clbaas_node_id);
		g_openstack_clbaas_node_id = NULL;
	}
	g_openstack_clbaas_node_id = mem_create(sizeof(openstack_lbaas_node), OPENSTACK_CLBAAS_NODE_MAX_NUM);
	if(NULL == g_openstack_clbaas_node_id)
	{
		
		LOG_PROC("ERROR", "%s %d: Can't get memory.", FN, LN);
	}
	
	g_openstack_clbaas_routersubnets_list = NULL;
	g_openstack_clbaas_vipfloatingips_list = NULL;
	g_openstack_clbaas_loadbalancers_list = NULL;
	g_openstack_clbaas_backendips_list = NULL;
	
	return;
}
void destory_openstack_clbaas()
{
	if (g_openstack_clbaas_routersubnet_id != NULL) {
		mem_destroy(g_openstack_clbaas_routersubnet_id);
		g_openstack_clbaas_routersubnet_id = NULL;
	}
	if (g_openstack_clbaas_vipfloatingip_id != NULL) {
		mem_destroy(g_openstack_clbaas_vipfloatingip_id);
		g_openstack_clbaas_vipfloatingip_id = NULL;
	}


	if (g_openstack_clbaas_loadbalancer_id != NULL) {
		mem_destroy(g_openstack_clbaas_loadbalancer_id);
		g_openstack_clbaas_loadbalancer_id = NULL;
	}

	if (g_openstack_clbaas_backendip_id != NULL) {
		mem_destroy(g_openstack_clbaas_backendip_id);
		g_openstack_clbaas_backendip_id = NULL;
	}
	
	if (g_openstack_clbaas_node_id != NULL) {
		mem_destroy(g_openstack_clbaas_node_id);
		g_openstack_clbaas_node_id = NULL;
	}

	
	
	g_openstack_clbaas_routersubnets_list = NULL;
	g_openstack_clbaas_vipfloatingips_list = NULL;
	g_openstack_clbaas_loadbalancers_list = NULL;
	g_openstack_clbaas_backendips_list = NULL;
	return ;
}

void add_node_into_clblist(void* data, enum lbass_node_type node_type)
{
	openstack_lbaas_node_p* list_p = NULL;

	switch (node_type) {
		case CLBAAS_NODE_ROUTERSUBNET:
			list_p = &g_openstack_clbaas_routersubnets_list;
			break;
		case CLBAAS_NODE_VIPFLOATINGIP:
			list_p = &g_openstack_clbaas_vipfloatingips_list;
			break;
		case CLBAAS_NODE_INTERFACE:
			list_p = &g_openstack_clbaas_loadbalancers_list;
			break;
		case CLBAAS_NODE_BACKENDIP:
			list_p = &g_openstack_clbaas_backendips_list;
			break;
		default:
			break;
	}

	openstack_lbaas_node_p node_p = (openstack_lbaas_node_p)mem_get(g_openstack_clbaas_node_id);
	if (NULL != node_p) {
		node_p->data = data;
		node_p->next = *list_p;
		*list_p = node_p;
	}
}

void remove_node_from_clblist(void* data, enum lbass_node_type node_type)
{
	openstack_lbaas_node_p* list_p = NULL;

	switch (node_type) {
		case CLBAAS_NODE_ROUTERSUBNET:
			list_p = &g_openstack_clbaas_routersubnets_list;
			break;
		case CLBAAS_NODE_VIPFLOATINGIP:
			list_p = &g_openstack_clbaas_vipfloatingips_list;
			break;
		case CLBAAS_NODE_INTERFACE:
			list_p = &g_openstack_clbaas_loadbalancers_list;
			break;
		case CLBAAS_NODE_BACKENDIP:
			list_p = &g_openstack_clbaas_backendips_list;
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
		mem_free(g_openstack_clbaas_node_id, temp_p);
		temp_p = NULL;
	}
	else {
		next_p = prev_p->next;

		while (next_p) {
			if (next_p->data == data) {
				temp_p = next_p;
				prev_p->next = next_p->next;
				mem_free(g_openstack_clbaas_node_id, temp_p);
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

openstack_clbaas_routersubnet_p create_openstack_clbaas_subnet_by_routerrest(
		char* router_id,
		char* subnet_id )
{

	openstack_clbaas_routersubnet_p ret = NULL;

	if((NULL == router_id)|| (NULL == subnet_id))
	{
		return NULL;
	}
	ret = (openstack_clbaas_routersubnet_p)mem_get(g_openstack_clbaas_routersubnet_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_clbaas_routersubnet_t));
		strcpy(ret->router_id, router_id);
		strcpy(ret->subnet_id, subnet_id);
	}
	else {
		LOG_PROC("ERROR", "Create openstack subnet pools: Can't get memory.");
	}
	return ret;
}

INT4 compare_openstack_clbaas_subnet_by_routerrest(
		char* router_id,
		char* subnet_id,
		openstack_clbaas_routersubnet_p subnetpool_p)
{
	if((NULL == router_id)|| (NULL == subnet_id))
	{
		return NULL;
	}
	if ((subnetpool_p)
		&& compare_str(router_id, subnetpool_p->router_id)
		&& compare_str(subnet_id, subnetpool_p->subnet_id))
	{
		return 1;
	}

	return 0;
}

openstack_clbaas_routersubnet_p find_openstack_clbaas_subnet_by_router_id(char* router_id, char* subnet_id)
{
	openstack_clbaas_routersubnet_p lb_subnet = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_routersubnets_list;
	
	if((NULL == router_id)|| (NULL == subnet_id))
	{
		return NULL;
	}
	while(node_p != NULL) 
	{
		lb_subnet = (openstack_clbaas_routersubnet_p)node_p->data;
		if ((NULL != lb_subnet) && (strcmp(lb_subnet->router_id,router_id) == 0)&&(strcmp(lb_subnet->subnet_id ,subnet_id) == 0)) 
		{
			return lb_subnet;
		}
		node_p = node_p->next;
	}
	return NULL;
};

openstack_clbaas_routersubnet_p find_openstack_clbaas_subnet_by_subnet_id(char* subnet_id)
{
	openstack_clbaas_routersubnet_p lb_subnet = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_routersubnets_list;
	
	if(NULL == subnet_id)
	{
		return NULL;
	}
	while(node_p != NULL) 
	{
		lb_subnet = (openstack_clbaas_routersubnet_p)node_p->data;
		if ((NULL != lb_subnet) && (strcmp(lb_subnet->subnet_id,subnet_id) == 0)) 
		{
			return lb_subnet;
		}
		node_p = node_p->next;
	}
	return NULL;
};

openstack_clbaas_routersubnet_p update_openstack_clbaas_subnet_by_routerrest(
		char* router_id,
		char* subnet_id )
{
	openstack_clbaas_routersubnet_p lb_subnet = NULL;
	
	if((NULL == router_id)|| (NULL == subnet_id))
	{
		return NULL;
	}
	lb_subnet = find_openstack_clbaas_subnet_by_router_id(router_id, subnet_id);
	if(lb_subnet == NULL) {
		lb_subnet = create_openstack_clbaas_subnet_by_routerrest(router_id, subnet_id);
		if (lb_subnet) {
			add_node_into_clblist(lb_subnet, CLBAAS_NODE_ROUTERSUBNET);
			lb_subnet->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_clbaas_subnet_by_routerrest(router_id, subnet_id, lb_subnet)) {
		lb_subnet->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		strcpy(lb_subnet->router_id, router_id);
		strcpy(lb_subnet->subnet_id , subnet_id);
		lb_subnet->check_status = (UINT2)CHECK_UPDATE;
	}
	return lb_subnet;
}


openstack_clbaas_vipfloating_p create_openstack_clbaas_vipfloatingpool_by_vipsrest(
		char* pool_id,
		char* router_id,
		char* port_id,
		UINT4 ipv4Addr,
		UINT1 is_publictype)
{
	openstack_clbaas_vipfloating_p ret = NULL;

	if(NULL == pool_id)
	{
		return NULL;
	}
	ret = (openstack_clbaas_vipfloating_p)mem_get(g_openstack_clbaas_vipfloatingip_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_clbaas_vipfloating_t));
		if(pool_id)
		{
			strcpy(ret->pool_id, pool_id);
		}
		if(router_id)
		{
			strcpy(ret->router_id, router_id);
		}
		if(is_publictype)
		{
			ret->ext_ip =  ipv4Addr;
			strcpy(ret->ex_port_id , port_id);
			ret->ext_status = (UINT2)CHECK_CREATE;
		}
		else
		{
			ret->inside_ip =  ipv4Addr;
			ret->inside_status = (UINT2)CHECK_CREATE;
		}
		
	}
	else {
		LOG_PROC("ERROR", "Create openstack vipfloatingpool: Can't get memory.");
	}
	return ret;
}


INT4 compare_openstack_clbaas_vipfloatingpool_by_vipsrest(
		char* pool_id,
		char* router_id,
		UINT4 ipv4Addr,
		openstack_clbaas_vipfloating_p vipfloatingpool_p)
{
	if ((vipfloatingpool_p)
		&& compare_str(pool_id, vipfloatingpool_p->pool_id)
		//&& compare_str(router_id, vipfloatingpool_p->router_id)
		//&& compare_str(port_id, vipfloatingpool_p->ex_port_id)
		&&((ipv4Addr == vipfloatingpool_p->ext_ip)||(ipv4Addr == vipfloatingpool_p->inside_ip)))
	{
		return 1;
	}

	return 0;
}


openstack_clbaas_vipfloating_p find_openstack_clbaas_vipfloating_by_vips_id(char* pool_id)
{
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_vipfloatingips_list;

	while(node_p != NULL) 
	{
		lb_vipfloating = (openstack_clbaas_vipfloating_p)node_p->data;
		if ((NULL != lb_vipfloating) && (strcmp(lb_vipfloating->pool_id ,pool_id) == 0)) 
		{
			return lb_vipfloating;
		}
		node_p = node_p->next;
	}
	return NULL;
};

openstack_clbaas_vipfloating_p find_openstack_clbaas_vipfloating_by_ip(UINT4 fixedIp, UINT4 floatingip)
{
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_vipfloatingips_list;
	
	while(node_p != NULL) 
	{
		lb_vipfloating = (openstack_clbaas_vipfloating_p)node_p->data;
		if ((NULL != lb_vipfloating) && (fixedIp == lb_vipfloating->inside_ip)&&(floatingip == lb_vipfloating->ext_ip)) 
		{
			return lb_vipfloating;
		}
		node_p = node_p->next;
	}
	return NULL;
};

openstack_clbaas_vipfloating_p update_openstack_clbaas_vipfloatingpool_by_vipsrest(
		char* pool_id,
		char* router_id,
		char* port_id,
		UINT4 ipv4Addr,
		UINT1 is_publictype)
{
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	
	if(NULL == pool_id)
	{
		return NULL;
	}
	lb_vipfloating = find_openstack_clbaas_vipfloating_by_vips_id(pool_id);
	if(lb_vipfloating == NULL) {
		lb_vipfloating = create_openstack_clbaas_vipfloatingpool_by_vipsrest(pool_id, router_id, port_id, ipv4Addr, is_publictype);
		if (lb_vipfloating) {
			add_node_into_clblist(lb_vipfloating, CLBAAS_NODE_VIPFLOATINGIP);
		}
	}
	else if (compare_openstack_clbaas_vipfloatingpool_by_vipsrest(pool_id ,router_id, ipv4Addr, lb_vipfloating)) {
		if(is_publictype)
		{
			lb_vipfloating->ext_ip =  ipv4Addr;
			strcpy(lb_vipfloating->ex_port_id , port_id);
			lb_vipfloating->ext_status = (UINT2)CHECK_LATEST;
		}
		else
		{
			lb_vipfloating->inside_ip =  ipv4Addr;
			lb_vipfloating->inside_status = (UINT2)CHECK_LATEST;
		}
		
	}
	else {
		strcpy(lb_vipfloating->pool_id, pool_id);
		if(router_id)
		{
			strcpy(lb_vipfloating->router_id, router_id);
		}
		if(is_publictype)
		{
			lb_vipfloating->ext_ip =  ipv4Addr;
			strcpy(lb_vipfloating->ex_port_id , port_id);
			lb_vipfloating->ext_status = (UINT2)CHECK_UPDATE;
		}
		else
		{
			lb_vipfloating->inside_ip =  ipv4Addr;
		}
		lb_vipfloating->inside_status = (UINT2)CHECK_UPDATE;
	}
	return lb_vipfloating;
}

openstack_clbaas_vipfloating_p find_openstack_clbaas_vipfloatingpool_by_extip(UINT4 ext_ip)
{
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_vipfloatingips_list;

	while(node_p != NULL) 
	{
		lb_vipfloating = (openstack_clbaas_vipfloating_p)node_p->data;
		if ((NULL != lb_vipfloating) &&ext_ip&&(ext_ip == lb_vipfloating->ext_ip))
		{
			return lb_vipfloating;
		}
		node_p = node_p->next;
	}
	return NULL;
}

UINT4 find_openstack_clbaas_vipfloatingpool_insideip_by_extip(UINT4 ext_ip)
{
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	
	lb_vipfloating = find_openstack_clbaas_vipfloatingpool_by_extip(ext_ip);
	if(lb_vipfloating)
	{
		return lb_vipfloating->inside_ip;
	}
	return 0;
}
void reset_openstack_clbaas_vipfloatingpoolflag()
{

	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_vipfloatingips_list;

	while(node_p != NULL) 
	{
		lb_vipfloating = (openstack_clbaas_vipfloating_p)node_p->data;
		if (NULL != lb_vipfloating) 
		{
			lb_vipfloating->ext_status = (UINT2)CHECK_UNCHECKED;
			lb_vipfloating->inside_status = (UINT2)CHECK_UNCHECKED;
		}
		node_p = node_p->next;
	}
	return ;
}


void remove_openstack_clbaas_vipfloatingpool_unchecked()
{
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	openstack_lbaas_node_p head_p = g_openstack_clbaas_vipfloatingips_list;
	openstack_lbaas_node_p prev_p = head_p;
	
	if (NULL == head_p)
	{
		return ;
	}
	
	openstack_lbaas_node_p next_p = prev_p->next;
	
	while (next_p)
	{
		lb_vipfloating = (openstack_clbaas_vipfloating_p)next_p->data;
		
		if ((lb_vipfloating) && ((lb_vipfloating->ext_status == (UINT2)CHECK_UNCHECKED)||(lb_vipfloating->inside_status == (UINT2)CHECK_UNCHECKED))) 
		{
			prev_p->next = next_p->next;
			mem_free(g_openstack_clbaas_vipfloatingip_id, lb_vipfloating);
			mem_free(g_openstack_clbaas_node_id, next_p);
		}
		else 
		{
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}
	lb_vipfloating = (openstack_clbaas_vipfloating_p)head_p->data;
	if ((lb_vipfloating) && ((lb_vipfloating->ext_status == (UINT2)CHECK_UNCHECKED)||(lb_vipfloating->inside_status == (UINT2)CHECK_UNCHECKED))) 
	{
		next_p = head_p->next;
		mem_free(g_openstack_clbaas_vipfloatingip_id, lb_vipfloating);
		mem_free(g_openstack_clbaas_node_id, head_p);
		g_openstack_clbaas_vipfloatingips_list = next_p;
	}	
	
	return ;
}



openstack_clbass_loadbalancer_p find_openstack_clbaas_loadbalancer_by_HAInterfaceIp(UINT4 HAInterfaceIp)
{
	openstack_clbass_loadbalancer_p lb_loadbalancer = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_loadbalancers_list;

	while(node_p != NULL) 
	{
		lb_loadbalancer = (openstack_clbass_loadbalancer_p)node_p->data;
		if ((NULL != lb_loadbalancer) && HAInterfaceIp&&(HAInterfaceIp == lb_loadbalancer->HA_interfaceIp)) 
		{
			return lb_loadbalancer;
		}
		node_p = node_p->next;
	}
	return NULL;
};
openstack_clbass_loadbalancer_p find_openstack_clbaas_loadbalancer_by_PoolidAndHAInterfaceIp(char* pool_id, UINT4 HAInterfaceIp)
{
	openstack_clbass_loadbalancer_p lb_loadbalancer = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_loadbalancers_list;

	while(node_p != NULL) 
	{
		lb_loadbalancer = (openstack_clbass_loadbalancer_p)node_p->data;
		if ((NULL != lb_loadbalancer) && (strcmp(lb_loadbalancer->pool_id,pool_id) == 0)&&HAInterfaceIp&&(HAInterfaceIp != lb_loadbalancer->HA_interfaceIp)) 
		{
			return lb_loadbalancer;
		}
		node_p = node_p->next;
	}
	return NULL;
}
openstack_clbass_loadbalancer_p find_openstack_clbaas_loadbalancer_by_loadbalancer_id(char* loadbalancer_id)
{
	openstack_clbass_loadbalancer_p lb_loadbalancer = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_loadbalancers_list;

	while(node_p != NULL) 
	{
		lb_loadbalancer = (openstack_clbass_loadbalancer_p)node_p->data;
		if ((NULL != lb_loadbalancer) && (strcmp(lb_loadbalancer->loadbalancer_id,loadbalancer_id) == 0)) 
		{
			return lb_loadbalancer;
		}
		node_p = node_p->next;
	}
	return NULL;
};

openstack_clbass_loadbalancer_p create_openstack_clbaas_loadbalancer(
		char* loadbalancer_id,
		char* pool_id,
		char* tenant_id,
		UINT4 HAInterfaceIp
		)
{
	if(NULL == loadbalancer_id)
	{
		return NULL;
	}
	openstack_clbass_loadbalancer_p ret = NULL;
	ret = (openstack_clbass_loadbalancer_p)mem_get(g_openstack_clbaas_loadbalancer_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_clbass_loadbalancer_t));
		if(pool_id)
		{
			strcpy(ret->pool_id, pool_id);
		}
		if(loadbalancer_id)
		{
			strcpy(ret->loadbalancer_id, loadbalancer_id);
		}
		if(tenant_id)
		{
			strcpy(ret->tenant_id, tenant_id);
		}
		if(HAInterfaceIp)
		{
			ret->HA_interfaceIp =  HAInterfaceIp;
		}
		
		ret->check_status = (UINT2)CHECK_CREATE;
	}
	else {
		LOG_PROC("ERROR", "Create openstack loadbalancer: Can't get memory.");
	}
	return ret;
}



openstack_clbass_loadbalancer_p update_openstack_clbaas_loadbalancer_by_loadbalancerrest
	(
		char* loadbalancer_id,
		char* pool_id,
		char* tenant_id
	)
{
	if((NULL == loadbalancer_id)||(NULL == tenant_id))
	{
		return NULL;
	}
	openstack_clbass_loadbalancer_p lb_loadbalancer = NULL;
	lb_loadbalancer = find_openstack_clbaas_loadbalancer_by_loadbalancer_id(loadbalancer_id);
	if(NULL == lb_loadbalancer)
	{
		lb_loadbalancer = create_openstack_clbaas_loadbalancer(loadbalancer_id, pool_id, tenant_id, 0);
		if(lb_loadbalancer)
		{
			add_node_into_clblist(lb_loadbalancer, CLBAAS_NODE_INTERFACE);
		}
	}
	else
	{
		if(pool_id)
		{
			strcpy(lb_loadbalancer->pool_id, pool_id);
		}
		if(tenant_id)
		{
			strcpy(lb_loadbalancer->tenant_id, tenant_id);
		}

		lb_loadbalancer->check_status = (UINT2)CHECK_UPDATE;
	}
	return lb_loadbalancer;
}

openstack_clbass_loadbalancer_p update_openstack_clbaas_loadbalancer_by_clbinterfacerest
	(
		char* loadbalancer_id,
		char* tenant_id,
		UINT4 HAInterfaceIp
	)
{
	if((NULL == loadbalancer_id)||(NULL == tenant_id))
	{
		return NULL;
	}
	openstack_clbass_loadbalancer_p lb_loadbalancer = NULL;
	lb_loadbalancer = find_openstack_clbaas_loadbalancer_by_loadbalancer_id(loadbalancer_id);
	if(NULL == lb_loadbalancer)
	{
		lb_loadbalancer = create_openstack_clbaas_loadbalancer(loadbalancer_id, NULL, tenant_id, HAInterfaceIp);
		if(lb_loadbalancer)
		{
			add_node_into_clblist(lb_loadbalancer, CLBAAS_NODE_INTERFACE);
		}
	}
	else
	{
		if(tenant_id)
		{
			strcpy(lb_loadbalancer->tenant_id, tenant_id);
		}
		if(HAInterfaceIp)
		{
			lb_loadbalancer->HA_interfaceIp =  HAInterfaceIp;
		}
		lb_loadbalancer->check_status = (UINT2)CHECK_UPDATE;
	}
	return lb_loadbalancer;
}



void reset_openstack_clbaas_loadbalancerflag()
{

	openstack_clbass_loadbalancer_p lb_loadbalancer = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_loadbalancers_list;

	while(node_p != NULL) 
	{
		lb_loadbalancer = (openstack_clbass_loadbalancer_p)node_p->data;
		if (NULL != lb_loadbalancer) 
		{
			lb_loadbalancer->check_status = (UINT2)CHECK_UNCHECKED;
		}
		node_p = node_p->next;
	}
	return ;
}

void remove_openstack_clbaas_loadbalancer_unchecked()
{
	openstack_clbass_loadbalancer_p lb_loadbalancer = NULL;
	openstack_lbaas_node_p head_p = g_openstack_clbaas_loadbalancers_list;
	openstack_lbaas_node_p prev_p = head_p;
	
	if (NULL == head_p)
	{
		return ;
	}
	
	openstack_lbaas_node_p next_p = prev_p->next;
	
	while (next_p)
	{
		lb_loadbalancer = (openstack_clbass_loadbalancer_p)next_p->data;
		
		if ((lb_loadbalancer) && (lb_loadbalancer->check_status == (UINT2)CHECK_UNCHECKED)) 
		{
			prev_p->next = next_p->next;
			mem_free(g_openstack_clbaas_loadbalancer_id, lb_loadbalancer);
			mem_free(g_openstack_clbaas_node_id, next_p);
		}
		else 
		{
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}
	lb_loadbalancer = (openstack_clbass_loadbalancer_p)head_p->data;
	if ((lb_loadbalancer) && (lb_loadbalancer->check_status == (UINT2)CHECK_UNCHECKED)) 
	{
		next_p = head_p->next;
		mem_free(g_openstack_clbaas_loadbalancer_id, lb_loadbalancer);
		mem_free(g_openstack_clbaas_node_id, head_p);
		g_openstack_clbaas_loadbalancers_list = next_p;
	}	
	
	return ;
}


openstack_clbass_backend_p update_openstack_clbaas_backendip_by_backendrest
	(
		UINT4 backendip
	)
{
	openstack_clbass_backend_p lb_backendip = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_backendips_list;

	while(node_p != NULL) 
	{
		lb_backendip = (openstack_clbass_backend_p)node_p->data;
		if((NULL != lb_backendip) &&backendip&&( backendip == lb_backendip->backendip))
		{
			lb_backendip->check_status = (UINT2)CHECK_UPDATE;
			return  lb_backendip;
		}
		node_p = node_p->next;
	}
	
	lb_backendip = (openstack_clbass_backend_p)mem_get(g_openstack_clbaas_backendip_id);
	if(NULL != lb_backendip)
	{
		lb_backendip->backendip = backendip;
		lb_backendip->check_status = (UINT2)CHECK_CREATE;
		add_node_into_clblist(lb_backendip, CLBAAS_NODE_BACKENDIP);	
	}
	return  lb_backendip;
}

void reset_openstack_clbaas_backendflag()
{
	openstack_clbass_backend_p lb_backendip = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_backendips_list;

	while(node_p != NULL) 
	{
		lb_backendip = (openstack_clbass_backend_p)node_p->data;
		if (NULL != lb_backendip) 
		{
			lb_backendip->check_status = (UINT2)CHECK_UNCHECKED;
		}
		node_p = node_p->next;
	}
	return ;
}

void remove_openstack_clbaas_backend_unchecked()
{
	openstack_clbass_backend_p lb_backendip = NULL;
	openstack_lbaas_node_p head_p = g_openstack_clbaas_backendips_list;
	openstack_lbaas_node_p prev_p = head_p;
	
	if (NULL == head_p)
	{
		return ;
	}
	
	openstack_lbaas_node_p next_p = prev_p->next;
	
	while (next_p)
	{
		lb_backendip = (openstack_clbass_backend_p)next_p->data;
		
		if ((lb_backendip) && (lb_backendip->check_status == (UINT2)CHECK_UNCHECKED)) 
		{
			prev_p->next = next_p->next;
			mem_free(g_openstack_clbaas_backendip_id, lb_backendip);
			mem_free(g_openstack_clbaas_node_id, next_p);
		}
		else 
		{
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}
	lb_backendip = (openstack_clbass_backend_p)head_p->data;
	if ((lb_backendip) && (lb_backendip->check_status == (UINT2)CHECK_UNCHECKED)) 
	{
		next_p = head_p->next;
		mem_free(g_openstack_clbaas_backendip_id, lb_backendip);
		mem_free(g_openstack_clbaas_node_id, head_p);
		g_openstack_clbaas_backendips_list = next_p;
	}	
	
	return ;
}

void remove_openstack_clbaas_backend_internalflows_byIp(UINT4 destip)
{
	p_fabric_host_node hostNode = NULL ;
	
	openstack_clbass_backend_p lb_backendip = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_backendips_list;

	while(node_p != NULL) 
	{
		lb_backendip = (openstack_clbass_backend_p)node_p->data;
		if ((NULL != lb_backendip)&&(0 != lb_backendip->backendip))
		{
			hostNode = get_fabric_host_from_list_by_ip(lb_backendip->backendip);
			if(hostNode&&hostNode->sw)
			{
				//LOG_PROC("INFO", "###############################%s hostNode->sw->sw_ip=0x%x destip=0x%x lb_backendip->backendip=0x%x",FN, hostNode->sw->sw_ip, destip,lb_backendip->backendip);
				remove_clbforward_flow_by_IpAndMac(hostNode->sw, hostNode->mac, destip);
			}
		}
		node_p = node_p->next;
	}
	return ;
}

void remove_openstack_clbaas_backend_internalflows_byMac(gn_switch_t *sw, char *mac)
{
	p_fabric_host_node hostNode = NULL ;
	
	openstack_clbass_backend_p lb_backendip = NULL;
	openstack_lbaas_node_p node_p = g_openstack_clbaas_backendips_list;

	if((NULL == sw)||(NULL == mac))
	{
		return ;
	}
	while(node_p != NULL) 
	{
		lb_backendip = (openstack_clbass_backend_p)node_p->data;
		if ((NULL != lb_backendip)&&(0 != lb_backendip->backendip))
		{

				//LOG_PROC("INFO", "###############################%s sw->sw_ip=0x%x lb_backendip->backendip=0x%x",FN,sw->sw_ip,lb_backendip->backendip);
				remove_clbforward_flow_by_IpAndMac(sw, mac, lb_backendip->backendip);
		}
		node_p = node_p->next;
	}
	return ;
}



// this function is used to create lbaas poool by pool id
openstack_lbaas_pools_p create_openstack_clbaas_pools_by_clbpoolrest(
		char* tenant_id,
		char* pool_id,
		UINT4 conn_limit )
{
	openstack_lbaas_pools_p ret = NULL;
	ret = (openstack_lbaas_pools_p)mem_get(g_openstack_lbaas_pools_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_lbaas_pools));
		strcpy(ret->tenant_id, tenant_id);
		strcpy(ret->pools_id, pool_id);
		ret->connect_limit = conn_limit;
		//TODO
	}
	else {
		LOG_PROC("ERROR", "Create openstack lbaas pools: Can't get memory.");
	}
	return ret;
};

// this function is used to create lbaas pool by vip
openstack_lbaas_pools_p create_openstack_clbaas_pools_by_clbviprest(
		char* pool_id,
		char* tenant_id,
		UINT4 ipaddress)
{
	openstack_lbaas_pools_p ret = NULL;
	ret = (openstack_lbaas_pools_p)mem_get(g_openstack_lbaas_pools_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_lbaas_pools));
		strcpy(ret->pools_id, pool_id);
		strcpy(ret->tenant_id, tenant_id);
		ret->ipaddress = ipaddress;
	}
	else {
		LOG_PROC("ERROR", "Create openstack lbaas pools: Can't get memory.");
	}
	return ret;
};

INT4 compare_openstack_clbaas_pools_by_clbviprest(
		char* pool_id,
		char* tenant_id,
		UINT4 ipaddress,
		openstack_lbaas_pools_p pool_p)
{
	if ((pool_p)
		&& compare_str(pool_id, pool_p->pools_id)
		&& compare_str(tenant_id, pool_p->tenant_id)
		&& (ipaddress == pool_p->ipaddress))
	{
		return 1;
	}

	return 0;
}

INT4 compare_openstack_clbaas_pool_by_clbpoolrest(
		char* tenant_id,
		char* pool_id,
		UINT4 conn_limit ,
		openstack_lbaas_pools_p pool_p)
{
	if ((pool_p)
		&& compare_str(tenant_id, pool_p->tenant_id)
		&& compare_str(pool_id, pool_p->pools_id)
		&& (conn_limit == pool_p->connect_limit))
		{
			return 1;
		}
	return 0;
}

// this function is used to update lbaas pool by pool id
openstack_lbaas_pools_p update_openstack_clbaas_pool_by_clbpoolrest(
		char* tenant_id,
		char* pool_id,
		UINT4 conn_limit )
{
	openstack_lbaas_pools_p lb_pool = NULL;
	lb_pool = find_openstack_lbaas_pool_by_pool_id(pool_id);
	if(lb_pool == NULL) {
		lb_pool = create_openstack_clbaas_pools_by_clbpoolrest(tenant_id,pool_id,conn_limit);
		if (lb_pool) {
			add_node_into_list(lb_pool, LBAAS_NODE_POOL);
			lb_pool->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_clbaas_pool_by_clbpoolrest(tenant_id,pool_id,conn_limit,lb_pool)) {
		lb_pool->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		strcpy(lb_pool->tenant_id, tenant_id);
		strcpy(lb_pool->pools_id, pool_id);
		lb_pool->connect_limit = conn_limit;
		lb_pool->check_status = (UINT2)CHECK_UPDATE;
	}
	return lb_pool;
};

openstack_lbaas_pools_p update_openstack_clbaas_pool_by_clblistenrest(
		char* tenant_id,
		char* pool_id,
		UINT1 protocol,
		UINT4 protocol_port,
		UINT4 conn_limit,
		UINT1 lbaas_method,
		UINT1 session_persistence)
{
		openstack_lbaas_pools_p lb_pool = NULL;
		lb_pool = find_openstack_lbaas_pool_by_pool_id(pool_id);
		if(lb_pool == NULL) {
			lb_pool = create_openstack_clbaas_pools_by_clbpoolrest(tenant_id,pool_id,conn_limit);
			if (lb_pool) {
				
				add_node_into_list(lb_pool, LBAAS_NODE_POOL);
				lb_pool->check_status = (UINT2)CHECK_CREATE;
			}
		}
		else if (compare_openstack_clbaas_pool_by_clbpoolrest(tenant_id,pool_id,conn_limit,lb_pool))
		{
			lb_pool->protocol = protocol;
			lb_pool->protocol_port = protocol_port;
			lb_pool->lbaas_method = lbaas_method;
			lb_pool->session_persistence = session_persistence;
			
			lb_pool->check_status = (UINT2)CHECK_UPDATE;
		}
		else
		{
			strcpy(lb_pool->tenant_id, tenant_id);
			strcpy(lb_pool->pools_id, pool_id);
			lb_pool->connect_limit = conn_limit;
			lb_pool->protocol = protocol;
			lb_pool->protocol_port = protocol_port;
			lb_pool->lbaas_method = lbaas_method;
			lb_pool->session_persistence = session_persistence;
			lb_pool->check_status = (UINT2)CHECK_UPDATE;
		}
}

// this function is used to update lbaas pool by vip
openstack_lbaas_pools_p update_openstack_clbaas_pool_by_clbviprest(
		char* tenant_id,
		char* pool_id,
		UINT4 ipaddress )
{
	openstack_lbaas_pools_p lb_pool = NULL;
	lb_pool = find_openstack_lbaas_pool_by_pool_id(pool_id);

	if(lb_pool == NULL) {
		lb_pool = create_openstack_clbaas_pools_by_clbviprest(pool_id,tenant_id,ipaddress);
		if (lb_pool) {
			add_node_into_list((void*)lb_pool, LBAAS_NODE_POOL);
			lb_pool->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_clbaas_pools_by_clbviprest(pool_id,tenant_id,ipaddress, lb_pool)) {
		lb_pool->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		strcpy(lb_pool->pools_id, pool_id);
		strcpy(lb_pool->tenant_id, tenant_id);
		lb_pool->ipaddress = ipaddress;
		lb_pool->check_status = (UINT2)CHECK_UPDATE;
	}
	return lb_pool;
};

// this function is used to create lbaas member
openstack_lbaas_members_p create_openstack_clbaas_clbmember(
		char* member_id,
		char* tenant_id,
		UINT4 protocol_port,
		UINT4 fixed_ip)
{
	openstack_lbaas_members_p ret = NULL;
	ret = (openstack_lbaas_members_p)mem_get(g_openstack_lbaas_members_id);

	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_lbaas_members));
		strcpy(ret->member_id, member_id);
		strcpy(ret->tenant_id, tenant_id);
		ret->protocol_port = protocol_port;
		ret->fixed_ip = fixed_ip;
	}
	else {
		LOG_PROC("ERROR", "Create openstack lbaas member: Can't get memory.");
	}
	return ret;
};

INT4 compare_openstack_clbaas_member(
		char* member_id,
		char* tenant_id,
		//char* pool_id,
		UINT4 protocol_port,
		UINT4 fixed_ip,
		openstack_lbaas_members_p member_p)
{

	if ((member_p)
		&& compare_str(member_id, member_p->member_id)
		&& compare_str(tenant_id, member_p->tenant_id)
		//&& compare_str(pool_id, member_p->pool_id)
		&& (protocol_port == member_p->protocol_port)
		// && (status == member_p->status)
		&& (fixed_ip == member_p->fixed_ip)) {
		return 1;
	}

	return 0;
}



// this function is used to update lbaas member
openstack_lbaas_members_p update_openstack_clbaas_member_by_backendrest(
		char* member_id,
		char* tenant_id,
		//char* pool_id,
		UINT4 protocol_port,
		UINT4 fixed_ip)
{
	openstack_lbaas_members_p lb_member = NULL;
	if((NULL == member_id)||(NULL == tenant_id)||('\0'==member_id)||('\0'==tenant_id))
	{
		return NULL;
	}
	lb_member = find_openstack_lbaas_member_by_member_id(member_id);

	if (lb_member == NULL) {
		lb_member = create_openstack_clbaas_clbmember(member_id,tenant_id,protocol_port,fixed_ip);
		if (lb_member) {
			add_node_into_list((void*)lb_member, LBAAS_NODE_MEMBER);
			add_node_into_list((void*)lb_member, LBAAS_NODE_POOL_MEMBER);
			add_lbaas_member_into_listener_member_listener(lb_member);
			lb_member->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_clbaas_member(member_id,tenant_id,protocol_port,fixed_ip,lb_member)) {
		lb_member->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		strcpy(lb_member->member_id, member_id); //backend id
		strcpy(lb_member->tenant_id, tenant_id);
		//strcpy(lb_member->pool_id, pool_id);
		lb_member->protocol_port = protocol_port;
		lb_member->fixed_ip = fixed_ip;
		lb_member->check_status = (UINT2)CHECK_UPDATE;
	}

    if (is_check_status_changed(lb_member->check_status)) {
        reset_floating_lbaas_group_flow_installed_flag(lb_member);
    }
	
	return lb_member;
};

openstack_lbaas_members_p update_openstack_clbaas_member_by_backendbindrest(
		char* member_id,
		char* tenant_id,
		UINT4 weight)
{
	openstack_lbaas_members_p lb_member = NULL;
	lb_member = find_openstack_lbaas_member_by_member_id(member_id);

	if (lb_member == NULL) {
		return NULL;
	}
	else {
		strcpy(lb_member->member_id, member_id);
		strcpy(lb_member->tenant_id, tenant_id);
		lb_member->weight = weight;
		lb_member->check_status = (UINT2)CHECK_UPDATE;
	}

    if (is_check_status_changed(lb_member->check_status)) {
        reset_floating_lbaas_group_flow_installed_flag(lb_member);
    }
	
	return lb_member;
};


// this function is used to crate lbaas listener
openstack_lbaas_listener_p create_openstack_clbaas_clblistener(
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

INT4 compare_openstack_clbaas_clblistener(
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
openstack_lbaas_listener_p update_openstack_clbaas_listener_by_clblistenrest(
		char* listener_id,
		UINT1 type,
		UINT4 check_frequency,
		UINT4 overtime,
		UINT1 retries)
{
	openstack_lbaas_listener_p lb_listener = NULL;
	lb_listener = find_openstack_lbaas_listener_by_listener_id(listener_id);

	if (lb_listener == NULL) {
		lb_listener = create_openstack_clbaas_clblistener(listener_id,type,check_frequency, overtime,retries);
		if (lb_listener) {
			add_node_into_list((void*)lb_listener, LBAAS_NODE_LISTENER);
			lb_listener->listener_member_list = create_lbaas_listener_member_listener(lb_listener);
			lb_listener->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (compare_openstack_clbaas_clblistener(listener_id,type,check_frequency, overtime,retries,lb_listener)) {
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



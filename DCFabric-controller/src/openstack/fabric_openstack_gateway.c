
/*
 * fabric_openstack_gateway.c
 *
 *  Created on: 6 20, 2017
 *      Author: yang
 */
 
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
#include "fabric_openstack_gateway.h"


void *g_openstack_router_outerinterface_id	= NULL;
void *g_openstack_router_gateway_node_id 		= NULL;
void *g_openstack_router_nodeid	= NULL;


openstack_router_outerinterface_node_p g_openstack_router_outerinterface_list = NULL ;


void init_openstack_router_gateway()
{
	if (g_openstack_router_outerinterface_id != NULL) {
			mem_destroy(g_openstack_router_outerinterface_id);
			g_openstack_router_outerinterface_id = NULL;
		}
		g_openstack_router_outerinterface_id = mem_create(sizeof(openstack_router_outerinterface_t), OPENSTACK_ROUTER_INTERFACE_NUM);

		if(NULL == g_openstack_router_outerinterface_id)
		{
			
			LOG_PROC("ERROR", "%s %d: Can't get memory.", FN, LN);
		}
		//memset(g_openstack_router_outerinterface_id, 0x00, sizeof(openstack_router_outerinterface_t)*OPENSTACK_ROUTER_INTERFACE_NUM);
		
		if (g_openstack_router_gateway_node_id != NULL) {
			mem_destroy(g_openstack_router_gateway_node_id);
			g_openstack_router_gateway_node_id = NULL;
		}
		g_openstack_router_gateway_node_id = mem_create(sizeof(openstack_router_gateway_t), OPENSTATCK_ROUTER_GATEWAY_NUM);
		if(NULL == g_openstack_router_gateway_node_id)
		{
			
			LOG_PROC("ERROR", "%s %d: Can't get memory.", FN, LN);
		}
		//memset(g_openstack_router_gateway_node_id, 0x00, sizeof(openstack_router_gateway_t)*OPENSTATCK_ROUTER_GATEWAY_NUM);

		
		
		if (g_openstack_router_nodeid != NULL) {
			mem_destroy(g_openstack_router_nodeid);
			g_openstack_router_nodeid = NULL;
		}
		g_openstack_router_nodeid = mem_create(sizeof(openstack_router_outerinterface_node_t), OPENSTACK_ROUTER_INTERFACE_NUM);
		if(NULL == g_openstack_router_nodeid)
		{
			
			LOG_PROC("ERROR", "%s %d: Can't get memory.", FN, LN);
		}
		
		//memset(g_openstack_router_nodeid, 0x00, sizeof(openstack_router_outerinterface_node_t)*OPENSTACK_ROUTER_INTERFACE_NUM);

		g_openstack_router_outerinterface_list = NULL ;
}

void destroy_openstack_router_gateway()
{
	if (g_openstack_router_outerinterface_id != NULL) {
		mem_destroy(g_openstack_router_outerinterface_id);
		g_openstack_router_outerinterface_id = NULL;
	}
	if (g_openstack_router_gateway_node_id != NULL) {
			mem_destroy(g_openstack_router_gateway_node_id);
			g_openstack_router_gateway_node_id = NULL;
	}
	
	if (g_openstack_router_gateway_node_id != NULL) {
		mem_destroy(g_openstack_router_gateway_node_id);
		g_openstack_router_gateway_node_id = NULL;
	}

	g_openstack_router_outerinterface_list = NULL ;
}

void add_node_into_router_outerinterface_list(void* data, enum openstack_router_outerinterface_node_type node_type)
{
	openstack_router_outerinterface_node_p* list_p = NULL;

	switch (node_type) {
		case ROUTER_OUTERINTERFACE_NODE_ROUTER:
			list_p = &g_openstack_router_outerinterface_list;
			break;
		default:
			break;
	}

	openstack_router_outerinterface_node_p node_p = (openstack_router_outerinterface_node_p)mem_get(g_openstack_router_nodeid);
	if (NULL != node_p) {
		node_p->data = data;
		node_p->next = *list_p;
		*list_p = node_p;
	}
}

void remove_node_from_outerinterface_list(void* data, enum openstack_router_outerinterface_node_type node_type)
{
	openstack_router_outerinterface_node_p* list_p = NULL;

	switch (node_type) {
		case ROUTER_OUTERINTERFACE_NODE_ROUTER:
			list_p = &g_openstack_router_outerinterface_list;
			break;
	
		default:
			break;
	}

	openstack_router_outerinterface_node_p** head_p = &list_p;
	openstack_router_outerinterface_node_p prev_p = **head_p;
	openstack_router_outerinterface_node_p temp_p = NULL;
	openstack_router_outerinterface_node_p next_p = NULL;

	if (prev_p->data == data) {
		**head_p = prev_p->next;
		temp_p = prev_p;
		mem_free(g_openstack_router_nodeid, temp_p);
		temp_p = NULL;
	}
	else {
		next_p = prev_p->next;

		while (next_p) {
			if (next_p->data == data) {
				temp_p = next_p;
				prev_p->next = next_p->next;
				mem_free(g_openstack_router_nodeid, temp_p);
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

UINT4 find_openstack_router_outerinterfaceip_by_router(openstack_router_outerinterface_p node_router_outerinterface, UINT4 *outer_interface_ip)
{
	openstack_router_gateway_p     	node_gateway = NULL;
	
	if((NULL == node_router_outerinterface)||(NULL == outer_interface_ip))
	{
		return GN_ERR;
	}
	
	node_gateway = (openstack_router_gateway_p)node_router_outerinterface->data;
	while(NULL != node_gateway)
	{
		if (OPENSTACK_PORT_TYPE_GATEWAY == node_gateway->port_type)
		{
			*outer_interface_ip = node_gateway->interfaceip;
			return GN_OK;
		}
		node_gateway = node_gateway->next;
	}
	return GN_ERR;
}
openstack_router_outerinterface_p    find_openstack_router_outerinterface_by_deviceid(char* deviceid)
{
	openstack_router_outerinterface_p node_router_outerinterface = NULL;
	openstack_router_outerinterface_node_p node_p = g_openstack_router_outerinterface_list;
	
	if((NULL == deviceid)||(0 == strlen(deviceid)))
	{
		return NULL;
	}
	while(node_p != NULL) 
	{
		node_router_outerinterface = (openstack_router_outerinterface_p)node_p->data;
		if(node_router_outerinterface && (strcmp(node_router_outerinterface->device_id,deviceid) == 0)) 
		{
			return node_router_outerinterface;
		}
		node_p = node_p->next;
	}
	return NULL;
}
 
openstack_router_outerinterface_p    find_openstack_router_outerinterface_by_networkid(char* network_id)
{
	openstack_router_gateway_p     	node_gateway = NULL;
	openstack_router_outerinterface_p node_router_outerinterface = NULL;
	openstack_router_outerinterface_node_p node_p = g_openstack_router_outerinterface_list;
	
	if((NULL == network_id)||(0 == strlen(network_id)))
	{
		return NULL;
	}
	while(node_p != NULL) 
	{
		node_router_outerinterface = (openstack_router_outerinterface_p)node_p->data;
		if(NULL != node_router_outerinterface)
		{
			node_gateway = (openstack_router_gateway_p)node_router_outerinterface->data;
			while(NULL != node_gateway)
			{
				if ((strcmp(node_gateway->network_id,network_id) == 0))
				{
					return node_router_outerinterface;
				}
				node_gateway = node_gateway->next;
			}
		}
		node_p = node_p->next;
	}
	return NULL;
}
 
openstack_router_outerinterface_p    find_openstack_router_outerinterface_by_subnetid(char* subnet_id)
{
	openstack_router_gateway_p     	node_gateway = NULL;
	openstack_router_outerinterface_p node_router_outerinterface = NULL;
	openstack_router_outerinterface_node_p node_p = g_openstack_router_outerinterface_list;
	
	if((NULL == subnet_id)||(0 == strlen(subnet_id)))
	{
		return NULL;
	}
	while(node_p != NULL) 
	{
		node_router_outerinterface = (openstack_router_outerinterface_p)node_p->data;
		if(NULL != node_router_outerinterface)
		{
			node_gateway = (openstack_router_gateway_p)node_router_outerinterface->data;
			while(NULL != node_gateway)
			{
				if ((strcmp(node_gateway->subnet_id,subnet_id) == 0)&&(OPENSTACK_PORT_TYPE_GATEWAY != node_gateway->port_type))
				{
					return node_router_outerinterface;
				}
				node_gateway = node_gateway->next;
			}
		}
		node_p = node_p->next;
	}
	return NULL;
}

openstack_router_outerinterface_p    find_openstack_router_outerinterface_by_networkAndsubnetid(char* network_id, char* subnet_id)
{
	openstack_router_gateway_p		node_gateway = NULL;
	openstack_router_outerinterface_p node_router_outerinterface = NULL;
	openstack_router_outerinterface_node_p node_p = g_openstack_router_outerinterface_list;
	
	if((NULL == network_id)||(0 == strlen(network_id))||(NULL == subnet_id)||(0 == strlen(subnet_id)))
	{
		return NULL;
	}
	while(node_p != NULL) 
	{
		node_router_outerinterface = (openstack_router_outerinterface_p)node_p->data;
		if(NULL != node_router_outerinterface)
		{
			node_gateway = (openstack_router_gateway_p)node_router_outerinterface->data;
			while(NULL != node_gateway)
			{
				if ((strcmp(node_gateway->network_id,network_id) == 0)&&(strcmp(node_gateway->subnet_id,subnet_id) == 0)/*&&(OPENSTACK_PORT_TYPE_GATEWAY != node_gateway->port_type)*/)
				{
					return node_router_outerinterface;
				}
				node_gateway = node_gateway->next;
			}
		}
		node_p = node_p->next;
	}
	return NULL;
}


openstack_router_outerinterface_p    create_openstack_router_outerinterface(char* device_id, char* network_id, char* subnet_id, UINT4 port_ip, UINT4 port_type )
{
	openstack_router_gateway_p     	node_gateway = NULL;
	openstack_router_outerinterface_p ret = NULL;

	if((NULL == device_id)||(NULL == network_id)||(NULL == subnet_id))
	{
		return NULL;
	}
	ret = (openstack_router_outerinterface_p)mem_get(g_openstack_router_outerinterface_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_router_outerinterface_t));
		if(device_id)
		{
			strcpy(ret->device_id, device_id);
		}
		node_gateway = (openstack_router_gateway_p)mem_get(g_openstack_router_gateway_node_id);
		if(NULL != node_gateway)
		{
			if(network_id)
			{
				strcpy(node_gateway->network_id, network_id);
			}
			if(subnet_id)
			{
				strcpy(node_gateway->subnet_id, subnet_id);
			}
			node_gateway->interfaceip = port_ip;
			node_gateway->port_type = port_type;
			node_gateway->check_status = CHECK_CREATE;
		}
		else
		{
			
			LOG_PROC("ERROR", "Create openstack router_gateway: Can't get memory.");
		}
		ret->data = node_gateway;	
		ret->check_status = CHECK_CREATE;
		
	}
	else {
		LOG_PROC("ERROR", "Create openstack router_outerinterface: Can't get memory.");
	}
	return ret;
}

UINT4    update_openstack_router_gateway(openstack_router_outerinterface_p node_outerinterface, char* network_id, char* subnet_id, UINT4 port_ip, UINT4 port_type )
{
	openstack_router_gateway_p     	node_gateway = NULL;
	openstack_router_gateway_p     	node_gateway_prev = NULL;
	if((NULL == node_outerinterface)||(NULL == network_id)||(NULL == subnet_id)||(0 == strlen(network_id))|| (0 == strlen(subnet_id)))
	{
		return GN_ERR;
	}

	node_gateway = (openstack_router_gateway_p)node_outerinterface->data;
	while(NULL != node_gateway)
	{
		if((0 == strcmp(node_gateway->network_id, network_id))&&(0 == strcmp(node_gateway->subnet_id, subnet_id))&&(port_ip == node_gateway->interfaceip)&&(port_type == node_gateway->port_type))
		{
			node_gateway->check_status = CHECK_UPDATE ;
			return GN_OK;
		}
		
		node_gateway_prev = node_gateway;
		node_gateway = node_gateway->next;
	}

	node_gateway = (openstack_router_gateway_p)mem_get(g_openstack_router_gateway_node_id);
	if(NULL != node_gateway)
	{
		if(network_id)
		{
			strcpy(node_gateway->network_id, network_id);
		}
		if(subnet_id)
		{
			strcpy(node_gateway->subnet_id, subnet_id);
		}
		node_gateway->interfaceip = port_ip;
		node_gateway->port_type = port_type;
		node_gateway->check_status = CHECK_CREATE;
	}
	
	if(node_gateway_prev)
	{
		node_gateway_prev->next = node_gateway;
		return GN_OK;
	}
	
	node_outerinterface->data = node_gateway;
	return GN_OK;
}
openstack_router_outerinterface_p    update_openstack_router_outerinterface(char* device_id, char* network_id, char* subnet_id, UINT4 port_ip, UINT4 port_type )
{
	openstack_router_outerinterface_p node_outerinterface = NULL;
	//LOG_PROC("INFO", "%s %d device_id=%s network_id=%s subnet_id=%s port_ip=0x%x port_type=%d",FN,LN ,device_id,network_id,subnet_id, port_ip,port_type );

	node_outerinterface = find_openstack_router_outerinterface_by_deviceid( device_id );

	if(NULL == node_outerinterface)
	{
		//CREATE
		node_outerinterface = create_openstack_router_outerinterface(device_id, network_id, subnet_id, port_ip, port_type);
		if(NULL != node_outerinterface)
		{
			add_node_into_router_outerinterface_list(node_outerinterface, ROUTER_OUTERINTERFACE_NODE_ROUTER);
		}
	}
	else
	{
		//compare + insert

		update_openstack_router_gateway(node_outerinterface, network_id, subnet_id, port_ip, port_type);
		node_outerinterface->check_status = CHECK_UPDATE;
		
	}
	remove_openstack_router_outerinterface_from_olddeviceid(device_id, port_ip);
	return node_outerinterface;
	
}
 
void	reset_openstack_router_outerinterface_checkflag()
{
	openstack_router_gateway_p     	node_gateway = NULL;
	openstack_router_outerinterface_p node_router_outerinterface = NULL;
	openstack_router_outerinterface_node_p node_p = g_openstack_router_outerinterface_list;
	
	while(node_p != NULL) 
	{
		node_router_outerinterface = (openstack_router_outerinterface_p)node_p->data;
		if(NULL != node_router_outerinterface)
		{
			node_gateway = (openstack_router_gateway_p)node_router_outerinterface->data;
			while(NULL != node_gateway)
			{
				node_gateway->check_status =  CHECK_UNCHECKED;
				node_gateway = node_gateway->next;
			}
			
			node_router_outerinterface->check_status = CHECK_UNCHECKED ;
		}
		node_p = node_p->next;
	}
}
 
void	remove_openstack_router_outerinterface_uncheck()
{
	openstack_router_gateway_p     		node_gateway = NULL;
	openstack_router_gateway_p     		node_gateway_next = NULL;
	openstack_router_gateway_p     		node_gateway_prev = NULL;
	openstack_router_gateway_p     		node_gateway_head = NULL;
	openstack_router_outerinterface_p 	node_router_outerinterface = NULL;
	
	
	openstack_router_outerinterface_node_p node_p = g_openstack_router_outerinterface_list;
	openstack_router_outerinterface_node_p node_header = node_p;
	openstack_router_outerinterface_node_p node_prev = node_p;
	openstack_router_outerinterface_node_p node_next = NULL;
	if(NULL == node_p)
	{
		return ;
	}
		
	while(node_p != NULL)   // free gateway
	{
		node_router_outerinterface = (openstack_router_outerinterface_p)node_p->data;
		if(NULL != node_router_outerinterface)
		{
			node_gateway = (openstack_router_gateway_p)node_router_outerinterface->data;
			if(NULL != node_gateway)
			{
				node_gateway_head = node_gateway;
				node_gateway_prev = node_gateway;
				node_gateway_next = node_gateway->next;
				while(NULL != node_gateway_next)
				{
					if( CHECK_UNCHECKED == node_gateway_next->check_status )
					{
						
						node_gateway_prev->next = node_gateway_next->next;
						//free node_gateway->next
						mem_free(g_openstack_router_gateway_node_id, node_gateway_next);
					}
					else
					{
						node_gateway_prev = node_gateway_prev->next;
					}
					
					node_gateway_next = node_gateway_prev->next;
				}
				
				if(CHECK_UNCHECKED == node_gateway_head->check_status )
				{
					node_gateway_next = node_gateway_head->next;
					mem_free(g_openstack_router_gateway_node_id, node_gateway_head);
					node_router_outerinterface->data = node_gateway_next;
				}
				
			}			
			
		}
		node_p = node_p->next;
	}
	
	node_next = node_header->next ;
	while(node_next != NULL)
	{
		node_router_outerinterface = (openstack_router_outerinterface_p)node_next->data;
		
		if ((node_router_outerinterface) && (node_router_outerinterface->check_status == CHECK_UNCHECKED)) 
		{
			node_prev->next = node_next->next;
			mem_free(g_openstack_router_outerinterface_id, node_router_outerinterface);
			mem_free(g_openstack_router_nodeid, node_next);
		}
		else 
		{
			node_prev = node_prev->next;
		}
		node_next = node_prev->next;
	}
	node_router_outerinterface = (openstack_router_outerinterface_p)node_header->data;
	if ((node_router_outerinterface) && (node_router_outerinterface->check_status == CHECK_UNCHECKED)) 
	{
		node_next = node_header->next;
		mem_free(g_openstack_router_outerinterface_id, node_router_outerinterface);
		mem_free(g_openstack_router_nodeid, node_header);
		g_openstack_router_outerinterface_list = node_next;
	}	
	
	return ;
}

void	remove_openstack_router_outerinterface_from_olddeviceid(char* device_id,  UINT4 port_ip)
{
	openstack_router_gateway_p			node_gateway = NULL;
	openstack_router_gateway_p			node_gateway_next = NULL;
	openstack_router_gateway_p			node_gateway_prev = NULL;
	openstack_router_gateway_p			node_gateway_head = NULL;
	openstack_router_outerinterface_p	node_router_outerinterface = NULL;
	
	
	openstack_router_outerinterface_node_p node_p = g_openstack_router_outerinterface_list;

	if(NULL == node_p)
	{
		return ;
	}
		
	while(node_p != NULL)	// free gateway
	{
		node_router_outerinterface = (openstack_router_outerinterface_p)node_p->data;
		if(NULL != node_router_outerinterface)
		{
			node_gateway = (openstack_router_gateway_p)node_router_outerinterface->data;
			if(NULL != node_gateway)
			{
				node_gateway_head = node_gateway;
				node_gateway_prev = node_gateway;
				node_gateway_next = node_gateway->next;
				while(NULL != node_gateway_next)
				{
					if(( port_ip == node_gateway_next->interfaceip)&&(0 != strcmp(device_id, node_router_outerinterface->device_id)))
					{
						
						node_gateway_prev->next = node_gateway_next->next;
						//free node_gateway->next
						mem_free(g_openstack_router_gateway_node_id, node_gateway_next);
					}
					else
					{
						node_gateway_prev = node_gateway_prev->next;
					}
					
					node_gateway_next = node_gateway_prev->next;
				}
				if(( port_ip == node_gateway_head->interfaceip)&&(0 != strcmp(device_id, node_router_outerinterface->device_id)))
				{
					node_gateway_next = node_gateway_head->next;
					mem_free(g_openstack_router_gateway_node_id, node_gateway_head);
					node_router_outerinterface->data = node_gateway_next;
				}
				
			}			
			
		}
		node_p = node_p->next;
	}

	return ;
}


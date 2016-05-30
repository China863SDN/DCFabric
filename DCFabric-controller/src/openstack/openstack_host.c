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
 * openstack_host.c
 *
 *  Created on: Jun 16, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 */

#include "openstack_host.h"
#include "mem_pool.h"
#include "fabric_openstack_arp.h"
#include "timer.h"
#include "openstack_app.h"
#include "../ovsdb/ovsdb.h"
#include "../restful-svr/openstack-server.h"
#include "fabric_floating_ip.h"

extern t_fabric_host_list g_fabric_host_list;

void *g_openstack_host_network_id = NULL;
void *g_openstack_host_subnet_id = NULL;
void *g_openstack_host_port_id = NULL;
void *g_openstack_host_node_id = NULL;
void *g_openstack_host_security_id = NULL;

void *g_openstack_security_group_id = NULL;
void *g_openstack_security_rule_id = NULL;

openstack_node_p g_openstack_host_network_list = NULL;
openstack_node_p g_openstack_host_subnet_list = NULL;
openstack_node_p g_openstack_host_port_list = NULL;
openstack_security_p g_openstack_security_list = NULL;


void *g_host_check_timerid = NULL;
UINT4 g_host_check_interval = 20;
void *g_host_check_timer = NULL;

void reset_openstack_host_network_flag();

////////////////////////////////////////////////////////////////////////
void openstack_show_port(openstack_node_p node){
	openstack_port_p p = (openstack_port_p)(node->data);
	printf("port id: %s | subnet: %s | network: %s | tenant: %s |\n", p->port_id,p->subnet_id,p->network_id,p->tenant_id);
};
void openstack_show_subnet(openstack_node_p node){
	openstack_subnet_p p = (openstack_subnet_p)(node->data);
	printf("Subnet: | subnet: %s | network: %s | tenant: %s |\n",p->subnet_id,p->network_id,p->tenant_id);
};

void openstack_show_network(openstack_node_p node){
	openstack_network_p p = (openstack_network_p)(node->data);
	printf("Network: | network: %s | tenant: %s |\n",p->network_id,p->tenant_id);

};

void openstack_show_security_rule(openstack_security_rule_p rule_p)
{
	printf("direction: %s\n", rule_p->direction);
	printf("ethertype: %s\n", rule_p->ethertype);
	printf("rule_id: %s\n", rule_p->rule_id);
	printf("port_range_max: %u\n", rule_p->port_range_max);
	printf("port_range_min: %u\n", rule_p->port_range_min);
	printf("protocol: %s\n", rule_p->protocol);
	printf("remote_group_id: %s\n", rule_p->remote_group_id);
	printf("remote_ip_prefix: %s\n", rule_p->remote_ip_prefix);
	printf("tenant_id: %s\n", rule_p->tenant_id);
}


void openstack_show_all_port_security()
{

	p_fabric_host_node list = g_fabric_host_list.list;
	while (NULL != list) {
		openstack_port_p port_p = (openstack_port_p)list->data;
//		if ((ip2number("100.0.0.86") != list->ip_list[0]) && (ip2number("100.0.0.78") != list->ip_list[0]))
//		{
//			list = list->next;
//			continue;
//		}

//		printf("\n");
//		nat_show_ip(list->ip_list[0]);
//		printf("security number:%d\n", port_p->security_num);

		if (NULL != port_p) {
			openstack_node_p node_p = (openstack_node_p)port_p->security_data;
			while (NULL != node_p) {
				openstack_security_p security_p = (openstack_security_p)node_p->data;
				if (NULL != security_p) {
					openstack_security_rule_p rule_p = (openstack_security_rule_p)security_p->security_rule_p;
					while (NULL != rule_p) {
						openstack_show_security_rule(rule_p);
						rule_p = rule_p->next;
					}
					// security_p = security_p->next;
				}
				node_p = node_p->next;
			}
		}
		list = list->next;
	}
}

void openstack_show_all_security_group()
{
	openstack_security_p temp_p = g_openstack_security_list;
	while (NULL != temp_p) {
		printf("security group id:%s\n", temp_p->security_group);
		openstack_security_rule_p rule_p = temp_p->security_rule_p;
		while (NULL != rule_p) {
			openstack_show_security_rule(rule_p);
			rule_p = rule_p->next;
		}
		temp_p = temp_p->next;
	}
}

void show_all_fabric_host()
{
	p_fabric_host_node head = g_fabric_host_list.list;
	while (NULL != head) {
		printf("show openstack node:\n");
		nat_show_ip(head->ip_list[0]);
		nat_show_mac(head->mac);
		nat_show_ipv6(head->ipv6[0]);
		printf("type is:%d,dpid:%llu, port:%d", head->type, head->dpid, head->port);
		openstack_port_p p=  (openstack_port_p)head->data;
		if (NULL != p) {
			printf("port id: %s | subnet: %s | network: %s | tenant: %s |\n", p->port_id,p->subnet_id,p->network_id,p->tenant_id);
		}
		head=head->next;
	}
}

void show_openstack_total(){
	openstack_node_p node = NULL;
	UINT4 num = 0;
	node = g_openstack_host_network_list;
	while(node != NULL){
		openstack_show_network(node);
		node = node->next;
		num++;
	}
	printf("Network number: %d \n",num);

	num = 0;
	node = g_openstack_host_subnet_list;
	while(node != NULL){
		openstack_show_subnet(node);
		node = node->next;
		num++;
	}
	printf("Subnet number: %d \n",num);

	num = 0;
	node = g_openstack_host_port_list;
	while(node != NULL){
		openstack_show_port(node);
		node = node->next;
		num++;
	}
	printf("Port number: %d \n",num);
	return;
}
////////////////////////////////////////////////////////////////////////

void init_openstack_host(){
	if(g_openstack_host_network_id != NULL){
		mem_destroy(g_openstack_host_network_id);
	}
	g_openstack_host_network_id = mem_create(sizeof(openstack_network), OPENSTACK_NETWORK_MAX_NUM);

	if(g_openstack_host_subnet_id != NULL){
		mem_destroy(g_openstack_host_subnet_id);
	}
	g_openstack_host_subnet_id = mem_create(sizeof(openstack_subnet), OPENSTACK_SUBNET_MAX_NUM);

	if(g_openstack_host_port_id != NULL){
		mem_destroy(g_openstack_host_port_id);
	}
	g_openstack_host_port_id = mem_create(sizeof(openstack_port), OPENSTACK_PORT_MAX_NUM);


	if(g_openstack_host_node_id != NULL){
		mem_destroy(g_openstack_host_node_id);
	}
	g_openstack_host_node_id = mem_create(sizeof(openstack_node), OPENSTACK_NODE_MAX_NUM);

	if (g_openstack_security_group_id != NULL) {
		mem_destroy(g_openstack_security_group_id);
	}
	g_openstack_security_group_id = mem_create(sizeof(openstack_security), OPENSTACK_SECURITY_GROUP_MAX_NUM);

	if (g_openstack_host_security_id != NULL) {
		mem_destroy(g_openstack_host_security_id);
	}
	g_openstack_host_security_id = mem_create(sizeof(openstack_node), OPENSTACK_HOST_SECURITY_MAX_NUM);

	if (g_openstack_security_rule_id != NULL) {
		mem_destroy(g_openstack_security_rule_id);
	}
	g_openstack_security_rule_id = mem_create(sizeof(openstack_security_rule), OPENSTACK_SECURITY_RULE_MAX_NUM);

	g_openstack_host_network_list = NULL;
	g_openstack_host_subnet_list = NULL;
	g_openstack_host_port_list = NULL;
	return;
};

void destory_openstack_host(){
	if(g_openstack_host_network_id != NULL){
		mem_destroy(g_openstack_host_network_id);
		g_openstack_host_network_id = NULL;
	}

	if(g_openstack_host_subnet_id != NULL){
		mem_destroy(g_openstack_host_subnet_id);
		g_openstack_host_subnet_id = NULL;
	}

	if(g_openstack_host_port_id != NULL){
		mem_destroy(g_openstack_host_port_id);
		g_openstack_host_port_id = NULL;
	}

	if(g_openstack_host_node_id != NULL){
		mem_destroy(g_openstack_host_node_id);
		g_openstack_host_node_id = NULL;
	}

	if (g_openstack_security_group_id != NULL) {
		mem_destroy(g_openstack_security_group_id);
		g_openstack_security_list = NULL;
	}

	if (g_openstack_host_security_id != NULL) {
		mem_destroy(g_openstack_host_security_id);
		g_openstack_host_security_id = NULL;
	}

	if (g_openstack_security_rule_id != NULL) {
		mem_destroy(g_openstack_security_rule_id);
		g_openstack_security_rule_id = NULL;
	}

	// clear
	g_openstack_host_network_list = NULL;
	g_openstack_host_subnet_list = NULL;
	g_openstack_host_port_list = NULL;
	g_openstack_security_list = NULL;
	return;
};
////////////////////////////////////////////////////////////////////////
openstack_network_p create_openstack_host_network(
		char* tenant_id,
		//char* network_name,
		char* network_id,
		UINT1 shared,
		UINT1 external)
{
	openstack_network_p ret = NULL;
	ret = (openstack_network_p)mem_get(g_openstack_host_network_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_network));
		strcpy(ret->tenant_id,tenant_id);
		//strcpy(ret->network_name,network_name);
		strcpy(ret->network_id,network_id);
		ret->shared = shared;
		ret->external = external;
	}
	else {
		LOG_PROC("ERROR", "Create openstack host network: Can't get memory.");
	}
	return ret;
};
void destory_openstack_host_network(openstack_network_p network){
	mem_free(g_openstack_host_network_id,network);
	return;
};

openstack_network_p find_openstack_host_network_by_network_id(char* network_id){
	openstack_network_p network = NULL;
	openstack_node_p node_p = g_openstack_host_network_list;
	while(node_p != NULL){
		network = (openstack_network_p)node_p->data;
		if ((NULL != network) && (strcmp(network->network_id,network_id) == 0)){
			return network;
		}
		node_p = node_p->next;
	}
	return NULL;
};

void reset_openstack_host_network_flag()
{
	openstack_node_p node_p = g_openstack_host_network_list;
	openstack_network_p network_p = NULL;

	while(node_p != NULL) {
		network_p = (openstack_network_p)node_p->data;
		if (network_p) 
			network_p->check_status = (UINT2)CHECK_UNCHECKED;
		node_p = node_p->next;
	}
}

void reset_openstack_host_subnet_flag()
{
	openstack_node_p node_p = g_openstack_host_subnet_list;
	openstack_subnet_p subnet_p = NULL;

	while(node_p != NULL) {
		subnet_p = (openstack_subnet_p)node_p->data;
		if (subnet_p) 
			subnet_p->check_status = (UINT2)CHECK_UNCHECKED;
		node_p = node_p->next;
	}
}

void remove_openstack_host_network_by_check_flag()
{
	openstack_network_p network_p = NULL;
	openstack_node_p head_p = g_openstack_host_network_list;
	openstack_node_p prev_p = head_p;

	if (NULL == head_p)
	{
		return ;
	}

	openstack_node_p next_p = prev_p->next;

	while (next_p) {
		network_p = (openstack_network_p)next_p->data;
		
		if ((network_p) && (network_p->check_status == CHECK_UNCHECKED)) {
			prev_p->next = next_p->next;
			remove_external_port_by_networkid(network_p->network_id);
			mem_free(g_openstack_host_network_id, network_p);
			mem_free(g_openstack_host_node_id, next_p);
		}
		else {
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	network_p = (openstack_network_p)head_p->data;
	if ((network_p) && (network_p->check_status == CHECK_UNCHECKED)) {
		next_p = head_p->next;
		remove_external_port_by_networkid(network_p->network_id);
		mem_free(g_openstack_host_network_id, head_p->data);
		mem_free(g_openstack_host_node_id, head_p);
		g_openstack_host_network_list= next_p;
	}	
}

void remove_openstack_host_subnet_by_check_flag()
{
	openstack_subnet_p subnet_p = NULL;
	openstack_node_p head_p = g_openstack_host_subnet_list;
	openstack_node_p prev_p = head_p;

	if (NULL == head_p)
	{
		return ;
	}

	openstack_node_p next_p = prev_p->next;

	while (next_p) {
		subnet_p = (openstack_subnet_p)next_p->data;
		
		if ((subnet_p) && (subnet_p->check_status == CHECK_UNCHECKED)) {
			prev_p->next = next_p->next;
			remove_proactive_floating_internal_subnet_flow_by_subnet(subnet_p);
			mem_free(g_openstack_host_subnet_id, subnet_p);
			mem_free(g_openstack_host_node_id, next_p);
		}
		else {
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	subnet_p = (openstack_subnet_p)head_p->data;
	if ((subnet_p) && (subnet_p->check_status == CHECK_UNCHECKED)) {
		next_p = head_p->next;
		remove_proactive_floating_internal_subnet_flow_by_subnet(subnet_p);
		mem_free(g_openstack_host_subnet_id, head_p->data);
		mem_free(g_openstack_host_node_id, head_p);
		g_openstack_host_subnet_list= next_p;
	}	
}


void reload_openstack_host_network()
{
	reset_openstack_host_network_flag();
	reset_openstack_host_subnet_flag();
	reload_net_info();
	remove_openstack_host_network_by_check_flag();
	remove_openstack_host_subnet_by_check_flag();
}

void add_openstack_host_network(openstack_network_p network){
	openstack_network_p network_p = NULL;
	openstack_node_p node_p = NULL;
	if(network == NULL){
		return;
	}

	network_p = find_openstack_host_network_by_network_id(network->network_id);
	if(network_p != NULL){
		return;
	}
	node_p = create_openstack_host_node((UINT1*)network);
	if (NULL !=  node_p) {
		node_p->next = g_openstack_host_network_list;
		g_openstack_host_network_list = node_p;
	}
//	openstack_show_test();
	return;
};
openstack_network_p remove_openstack_host_network_by_network_id(char* network_id){
	openstack_network_p network = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_network_list;

	while(node_p->next != NULL){
		network = (openstack_network_p)(node_p->next->data);
		if ((NULL!= network) && (strcmp(network->network_id,network_id) == 0)) {
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			g_openstack_host_network_list = node.next;
			return network;
		}
		node_p = node_p->next;
	}
	g_openstack_host_network_list = node.next;
	return NULL;
};
////////////////////////////////////////////////////////////////////////
openstack_subnet_p create_openstack_host_subnet(
		char* tenant_id,
		char* network_id,
		//char* subnet_name,
		char* subnet_id,
		UINT4 gateway_ip,
		UINT4 start_ip,
		UINT4 end_ip,
		UINT1* gateway_ipv6,
		UINT1* start_ipv6,
		UINT1* end_ipv6,
		char* cidr,
		UINT1 external)
{
	openstack_subnet_p ret = NULL;
	ret = (openstack_subnet_p)mem_get(g_openstack_host_subnet_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_subnet));

		strcpy(ret->tenant_id,tenant_id);
		strcpy(ret->network_id,network_id);
		//strcpy(ret->subnet_name,subnet_name);
		strcpy(ret->subnet_id, subnet_id);
		ret->gateway_ip = gateway_ip;
		ret->start_ip = start_ip;
		ret->end_ip = end_ip;
		if (gateway_ipv6)
			memcpy(ret->gateway_ipv6, gateway_ipv6, 16);
		if (start_ipv6)
			memcpy(ret->start_ipv6, start_ipv6, 16);
		if (end_ipv6)
			memcpy(ret->end_ipv6, end_ipv6, 16);
		strcpy(ret->cidr, cidr);
		ret->external = external;
	}
	else {
		LOG_PROC("ERROR", "Openstack Subnet: Create failed, Can't get memory.");
	}

	return ret;
};
void destory_openstack_host_subnet(openstack_subnet_p subnet){
	mem_free(g_openstack_host_subnet_id,subnet);
	return;
};
void add_openstack_host_subnet(openstack_subnet_p subnet){
	openstack_subnet_p subnet_p = NULL;
	openstack_node_p node_p = NULL;
	if(subnet == NULL){
		return;
	}

	subnet_p = remove_openstack_host_subnet_by_subnet_id(subnet->subnet_id);
	if(subnet_p != NULL){
		return;
	}
	node_p = create_openstack_host_node((UINT1*)subnet);
	if (NULL != node_p) {
		node_p->next = g_openstack_host_subnet_list;
		g_openstack_host_subnet_list = node_p;
	}

	return;
};
openstack_subnet_p find_openstack_host_subnet_by_subnet_id(char* subnet_id){
	openstack_subnet_p subnet = NULL;
	openstack_node_p node_p = g_openstack_host_subnet_list;
	while(node_p != NULL){
		subnet = (openstack_subnet_p)node_p->data;
		if ((NULL != subnet) && (strcmp(subnet->subnet_id,subnet_id) == 0)) {
			return subnet;
		}
		node_p = node_p->next;
	}
	return NULL;
};
openstack_subnet_p find_openstack_host_subnet_by_geteway_ip(UINT4 gateway_ip){
	openstack_subnet_p subnet = NULL;
	openstack_node_p node_p = g_openstack_host_subnet_list;
	while(node_p != NULL){
		subnet = (openstack_subnet_p)node_p->data;
		if(subnet->gateway_ip == gateway_ip){
			return subnet;
		}
		node_p = node_p->next;
	}
	return NULL;
};

p_fabric_host_node find_openstack_host_by_srcport_ip(p_fabric_host_node host_p, UINT4 ip)
{
	p_fabric_host_node ret = NULL;
	ret = g_fabric_host_list.list;
	openstack_port_p port_p = (openstack_port_p)host_p->data;
	if ((NULL == port_p) || (0 == strlen(port_p->tenant_id))) {
		return NULL;
	}
	
	while(ret != NULL)
	{
		openstack_port_p list_port_p = (openstack_port_p)ret->data;
		if ((NULL != list_port_p) && (strlen(list_port_p->tenant_id)) && (check_IP_in_fabric_host(ret,ip))) 
		{
			if (0 == strcmp(port_p->tenant_id, list_port_p->tenant_id)) {
				return ret;
			}

			if (strlen(list_port_p->network_id)) {
				openstack_network_p network_p = find_openstack_host_network_by_network_id(list_port_p->network_id);
				if (1 == network_p->shared) {
					return ret;
				}
			}
		}
		ret = ret->next;
	}
	return NULL;
};

openstack_subnet_p remove_openstack_host_subnet_by_subnet_id(char* subnet_id){
	openstack_subnet_p subnet = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_subnet_list;

	while(node_p->next != NULL){
		subnet = (openstack_subnet_p)(node_p->next->data);
		if ((NULL != subnet) && (strcmp(subnet->subnet_id,subnet_id) == 0)) {
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			g_openstack_host_subnet_list = node.next;
			return subnet;
		}
		node_p = node_p->next;
	}
	g_openstack_host_subnet_list = node.next;
	return NULL;
};

void delete_openstack_host_subnet_by_tenant_id(char* tenant_id){
	openstack_subnet_p subnet = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_subnet_list;

	while(node_p->next != NULL){
		subnet = (openstack_subnet_p)(node_p->next->data);
		if ((NULL != subnet) && (strcmp(subnet->tenant_id,tenant_id) == 0)) {
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			destory_openstack_host_subnet(subnet);
		}else{
			node_p = node_p->next;
		}
	}
	g_openstack_host_subnet_list = node.next;
	return;
};
void delete_openstack_host_subnet_by_network_id(char* network_id){
	openstack_subnet_p subnet = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_subnet_list;

	while(node_p->next != NULL){
		subnet = (openstack_subnet_p)(node_p->next->data);
		if ((NULL != subnet) && (strcmp(subnet->network_id,network_id) == 0)) {
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			destory_openstack_host_subnet(subnet);
		}else{
			node_p = node_p->next;
		}
	}
	g_openstack_host_subnet_list = node.next;
	return;
};
void delete_openstack_host_subnet_by_subnet_id(char* subnet_id){
	openstack_subnet_p subnet = NULL;
	subnet = remove_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet != NULL){
		destory_openstack_host_subnet(subnet);
	}
	return;
};

////////////////////////////////////////////////////////////////////////
void* create_openstack_host_port(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id,
		UINT2 security_num,
		UINT1* security_data){
	UINT8 dpid = 0;
	p_fabric_host_node node = NULL;
	openstack_port_p ret = NULL;
	ret = (openstack_port_p)mem_get(g_openstack_host_port_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_port));

		if (NULL != tenant_id)
			strcpy(ret->tenant_id,tenant_id);
		if (NULL != network_id)
			strcpy(ret->network_id,network_id);
		if (NULL != port_id)
			strcpy(ret->port_id,port_id);
		if (NULL != subnet_id)
			strcpy(ret->subnet_id, subnet_id);

		if (NULL != sw) {
			dpid = sw->dpid;
		}
		ret->security_num = security_num;
		if (NULL != security_data)
			ret->security_data = security_data;

		node = insert_fabric_host_into_list_paras(sw, dpid, port, mac, ip, ipv6);
		if (NULL != node) {
			node->data = (void*)ret;
			node->type = port_type;
		}
		else {
			mem_free(g_openstack_host_port_id, ret);
		}
	}
	else {
		LOG_PROC("ERROR", "Create openstack host port: Can't get memory.");
	}
	return (void*)node;
};
void destory_openstack_host_port(openstack_port_p port){
	mem_free(g_openstack_host_port_id,port);
	return;
};

void update_openstack_host_port_by_mac(UINT1* mac, gn_switch_t* sw, UINT4 port)
{
	p_fabric_host_node host = get_fabric_host_from_list_by_mac(mac);
	if ((sw) && (host) && (port)) {
		host->sw = sw;
		host->port = port;
	}
}

//
//void add_openstack_host_port(openstack_port_p port){
//	openstack_port_p port_p = NULL;
//	openstack_node_p node_p = NULL;
//	if(port == NULL){
//		return;
//	}
//
//	port_p = find_openstack_host_port_by_port_id(port->port_id);
//	if(port_p != NULL){
//		return;
//	}
//	node_p = create_openstack_host_node((UINT1*)port);
//
//	if (NULL != node_p) {
//		node_p->next = g_openstack_host_port_list;
//		g_openstack_host_port_list = node_p;
//	}
//	return;
//};

//void set_openstack_host_port_portno(const UINT1 *mac, UINT4 ofport_no)
//{
//    openstack_port_p port = NULL;
//    openstack_node_p node_p = g_openstack_host_port_list;
//    while(node_p != NULL)
//    {
//        port = (openstack_port_p)node_p->data;
//        if(memcmp(port->mac, mac, 6) == 0)
//        {
//            port->ofport_no = ofport_no;
//            return;
//        }
//        node_p = node_p->next;
//    }
//    t_fabric_host_node sentinel;
//	set_fabric_host_port_portno(mac, ofport_no);
//}

//openstack_port_p find_openstack_host_port_by_port_id(char* port_id){
//	openstack_port_p port = NULL;
//	openstack_node_p node_p = g_openstack_host_port_list;
//	while(node_p != NULL){
//		port = (openstack_port_p)node_p->data;
//		if(strcmp(port->port_id,port_id) == 0){
//			return port;
//		}
//		node_p = node_p->next;
//	}
//	return NULL;
//};
//
//openstack_port_p remove_openstack_host_port_by_port_id(char* port_id){
//	openstack_port_p port = NULL;
//	openstack_node node;
//	openstack_node_p node_p = &node,temp_p = NULL;
//	node_p->next = g_openstack_host_subnet_list;
//
//	while(node_p->next != NULL){
//		port = (openstack_port_p)(node_p->next->data);
//		if(0 == strcmp(port->port_id,port_id)){
//			temp_p = node_p->next;
//			node_p->next = temp_p->next;
//			destory_openstack_host_node(temp_p);
//			g_openstack_host_port_list = node.next;
//			return port;
//		}
//		node_p = node_p->next;
//	}
//	g_openstack_host_port_list = node.next;
//	return NULL;
//};


//openstack_port_p find_openstack_host_port_by_mac(UINT1* mac){
//	openstack_port_p port = NULL;
//	openstack_node_p node_p = g_openstack_host_port_list;
//	while(node_p != NULL){
//		port = (openstack_port_p)node_p->data;
//		if(0 == memcmp(port->mac, mac, 6)){
//			return port;
//		}
//		node_p = node_p->next;
//	}
//	return NULL;
//};


//openstack_port_p remove_openstack_host_port_by_mac(UINT1* mac){
//	openstack_port_p port = NULL;
//	openstack_node node;
//	openstack_node_p node_p = &node,temp_p = NULL;
//	node_p->next = g_openstack_host_subnet_list;
//
//	while(node_p->next != NULL){
//		port = (openstack_port_p)(node_p->next->data);
//		if(0 == memcmp(port->mac, mac, 6)){
//			temp_p = node_p->next;
//			node_p->next = temp_p->next;
//			destory_openstack_host_node(temp_p);
//			g_openstack_host_port_list = node.next;
//			return port;
//		}
//		node_p = node_p->next;
//	}
//	g_openstack_host_port_list = node.next;
//	return NULL;
//};

//openstack_port_p find_openstack_host_port_by_ip_tenant(UINT4 ip,char* tenant_id){
//	openstack_port_p port = NULL;
//	openstack_node_p node_p = g_openstack_host_port_list;
//	while(node_p != NULL){
//		port = (openstack_port_p)node_p->data;
//		if(port->ip == ip && 0 == strcmp(port->tenant_id,tenant_id)){
//			return port;
//		}
//		node_p = node_p->next;
//	}
//	return NULL;
//};
//openstack_port_p find_openstack_host_port_by_ip_network(UINT4 ip,char* network_id){
//	openstack_port_p port = NULL;
//	openstack_node_p node_p = g_openstack_host_port_list;
//	while(node_p != NULL){
//		port = (openstack_port_p)node_p->data;
//		if(port->ip == ip && 0 == strcmp(port->network_id,network_id)){
//			return port;
//		}
//		node_p = node_p->next;
//	}
//	return NULL;
//};
//void find_openstack_network_by_floating_ip(UINT4 floating_ip,char* network_id){
//	openstack_port_p port = NULL;
//	openstack_node_p node_p = g_openstack_host_port_list;
//	while(node_p != NULL){
//		port = (openstack_port_p)node_p->data;
//		if(port->ip==floating_ip){
//			strcpy(network_id,port->network_id);
//			return;
//		}
//		node_p = node_p->next;
//	}
//}


//openstack_port_p find_openstack_host_port_by_ip_subnet(UINT4 ip,char* subnet_id){
//	openstack_port_p port = NULL;
//	openstack_node_p node_p = g_openstack_host_port_list;
//	while(node_p != NULL){
//		port = (openstack_port_p)node_p->data;
//		if(port->ip == ip && 0 == strcmp(port->subnet_id,subnet_id)){
//			return port;
//		}
//		node_p = node_p->next;
//	}
//	return NULL;
//};

//UINT4 find_openstack_host_ports_by_subnet_id(char* subnet_id,openstack_port_p* host_list){
//	UINT4 ret = 0;
//	openstack_port_p port = NULL;
//	openstack_node_p node_p = NULL;
//	node_p = g_openstack_host_port_list;
//
//	while(node_p != NULL){
//		port = (openstack_port_p)(node_p->data);
//		if(strcmp(port->subnet_id,subnet_id) == 0){
//			host_list[ret] = port;
//			ret++;
//		}
//		node_p = node_p->next;
//	}
//	return ret;
//};
//
//void delete_openstack_host_port_by_tenant_id(char* tenant_id){
//	openstack_port_p port = NULL;
//	openstack_node node;
//	openstack_node_p node_p = &node,temp_p = NULL;
//	node_p->next = g_openstack_host_port_list;
//
//	while(node_p->next != NULL){
//		port = (openstack_port_p)(node_p->next->data);
//		if(strcmp(port->tenant_id,tenant_id) == 0){
//			temp_p = node_p->next;
//			node_p->next = temp_p->next;
//			destory_openstack_host_node(temp_p);
//			destory_openstack_host_port(port);
//		}else{
//			node_p = node_p->next;
//		}
//	}
//	g_openstack_host_port_list = node.next;
//	return;
//};
//void delete_openstack_host_port_by_network_id(char* network_id){
//	openstack_port_p port = NULL;
//	openstack_node node;
//	openstack_node_p node_p = &node,temp_p = NULL;
//	node_p->next = g_openstack_host_port_list;
//
//	while(node_p->next != NULL){
//		port = (openstack_port_p)(node_p->next->data);
//		if(strcmp(port->network_id,network_id) == 0){
//			temp_p = node_p->next;
//			node_p->next = temp_p->next;
//			destory_openstack_host_node(temp_p);
//			destory_openstack_host_port(port);
//		}else{
//			node_p = node_p->next;
//		}
//	}
//	g_openstack_host_port_list = node.next;
//	return;
//};
//void delete_openstack_host_port_by_subnet_id(char* subnet_id){
//	openstack_port_p port = NULL;
//	openstack_node node;
//	openstack_node_p node_p = &node,temp_p = NULL;
//	node_p->next = g_openstack_host_port_list;
//
//	while(node_p->next != NULL){
//		port = (openstack_port_p)(node_p->next->data);
//		if(strcmp(port->subnet_id,subnet_id) == 0){
//			temp_p = node_p->next;
//			node_p->next = temp_p->next;
//			destory_openstack_host_node(temp_p);
//			destory_openstack_host_port(port);
//		}else{
//			node_p = node_p->next;
//		}
//	}
//	g_openstack_host_port_list = node.next;
//	return;
//};
//void delete_openstack_host_port_by_port_id(char* port_id){
//	openstack_port_p port = NULL;
//	port = remove_openstack_host_port_by_port_id(port_id);
//	if(port != NULL){
//		destory_openstack_host_port(port);
//	}
//	return;
//};
////////////////////////////////////////////////////////////////////////
openstack_node_p create_openstack_host_node(UINT1* data){
	openstack_node_p ret = NULL;
	ret = (openstack_node_p)mem_get(g_openstack_host_node_id);
	if (NULL != ret) {
		memset(ret,0,sizeof(openstack_node));
		ret->data = data;
	}
	else {
		LOG_PROC("ERROR", "Create openstack host node: Create fail, can't get memory.");
	}
	return ret;
};
void destory_openstack_host_node(openstack_node_p node){
	mem_free(g_openstack_host_node_id,node);
	return;
};

openstack_security_p update_openstack_security_group(char* security_group)
{
	openstack_security_p security_p = g_openstack_security_list;

	while (security_p) {
		if (0 == strcmp(security_p->security_group, security_group)) {
			return security_p;
		}
		security_p = security_p->next;
	}

	// create
	security_p = (openstack_security_p)mem_get(g_openstack_security_group_id);
	if (NULL != security_p) {
		memset(security_p->security_group, 0, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(security_p->security_group, security_group, OPENSTACK_SECURITY_GROUP_LEN);
		security_p->next = g_openstack_security_list;
		g_openstack_security_list = security_p;
	}
	else {
		LOG_PROC("INFO", "Security: Get memeory fail!");
	}

	return security_p;
}

openstack_node_p add_openstack_host_security_node(UINT1* data, openstack_node_p head_p)
{
	openstack_node_p node_p = head_p;

	if (NULL == data) {
		return head_p;
	}
	while (node_p) {
		if (data == node_p->data) {
			return head_p;
		}
		node_p = node_p->next;
	}

	openstack_node_p ret = NULL;
	ret = (openstack_node_p)mem_get(g_openstack_host_security_id);
	if (NULL == ret) {
		return head_p;
	}

	memset(ret, 0, sizeof(openstack_node));
	ret->data = data;
	ret->next = head_p;
	return ret;
};

void clear_openstack_host_security_node(UINT1* head_p)
{
	openstack_node_p node_p = (openstack_node_p)head_p;
	if (NULL == node_p)
	{
		return;
	}
	openstack_node_p temp_p = node_p->next;

	while (temp_p) {
		mem_free(g_openstack_host_security_id, node_p);
		node_p = temp_p;
		temp_p = temp_p->next;
	}

	mem_free(g_openstack_host_security_id, node_p);
}

void clear_all_security_group_info()
{
	if (g_openstack_security_group_id != NULL) {
		mem_destroy(g_openstack_security_group_id);
	}
	g_openstack_security_group_id = mem_create(sizeof(openstack_security), OPENSTACK_SECURITY_GROUP_MAX_NUM);

	if (g_openstack_host_security_id != NULL) {
		mem_destroy(g_openstack_host_security_id);
	}
	g_openstack_host_security_id = mem_create(sizeof(openstack_node), OPENSTACK_HOST_SECURITY_MAX_NUM);

	if (g_openstack_security_rule_id != NULL) {
		mem_destroy(g_openstack_security_rule_id);
	}
	g_openstack_security_rule_id = mem_create(sizeof(openstack_security_rule), OPENSTACK_SECURITY_RULE_MAX_NUM);
	
	p_fabric_host_node list = g_fabric_host_list.list;
	while (list) {
		openstack_port_p port_p = (openstack_port_p)list->data;
		if (NULL != port_p) {
			port_p->security_data = NULL;
		}
		list = list->next;
	}

	g_openstack_security_list = NULL;
}

openstack_security_rule_p find_security_rule_by_rule_id(char* group_id, char* rule_id)
{
	openstack_security_p security_p = g_openstack_security_list;
	while (NULL !=  security_p) {
		if (0 == strcmp(group_id, security_p->security_group)) {
			openstack_security_rule_p rule_p = security_p->security_rule_p;
			while (rule_p) {
				if (0 == strcmp(rule_p->rule_id, rule_id)) {
					return rule_p;
				}
				rule_p = rule_p->next;
			}
		}
		security_p = security_p->next;
	}
	
	return NULL;
}

INT4 compare_security_rule(char* group_id, char* rule_id, char* direction, char* ethertype, char* port_range_max,
		char* port_range_min, char* protocol, char* remote_group_id, char* remote_ip_prefix, char* tenant_id, openstack_security_rule_p rule_p)
{

	UINT4 port_max = (0 == strcmp(port_range_max, "")) ? 0:atoi(port_range_max);
	UINT4 port_min = (0 == strcmp(port_range_min, "")) ? 0:atoi(port_range_min);

	if ((rule_p)
		&& compare_str(group_id, rule_p->group_id)
		&& compare_str(rule_id, rule_p->rule_id)
		&& compare_str(direction, rule_p->direction)
		&& compare_str(ethertype, rule_p->ethertype)
		&& (port_max == rule_p->port_range_max)
		&& (port_min == rule_p->port_range_min)
		&& compare_str(protocol, rule_p->protocol)
		&& compare_str(remote_group_id, rule_p->remote_group_id)
		&& compare_str(remote_ip_prefix, rule_p->remote_ip_prefix)
		&& compare_str(tenant_id, rule_p->tenant_id)) {
		return 1;
	}

	return 0;
}

openstack_security_rule_p create_security_rule(char* group_id, char* rule_id, char* direction, char* ethertype, char* port_range_max,
		char* port_range_min, char* protocol, char* remote_group_id, char* remote_ip_prefix, char* tenant_id)
{
	openstack_security_rule_p rule_p = mem_get(g_openstack_security_rule_id);
	if (NULL != rule_p) {
		memcpy(rule_p->direction, direction, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->ethertype, ethertype, OPENSTACK_SECURITY_GROUP_LEN);
		rule_p->port_range_max = (0 == strcmp(port_range_max, "")) ? 0:atoi(port_range_max);
		rule_p->port_range_min = (0 == strcmp(port_range_min, "")) ? 0:atoi(port_range_min);
		memcpy(rule_p->protocol, protocol, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->remote_group_id, remote_group_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->remote_ip_prefix, remote_ip_prefix, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->tenant_id, tenant_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->rule_id, rule_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->group_id, group_id, OPENSTACK_SECURITY_GROUP_LEN);
	}

	return rule_p;
}

openstack_security_p add_security_rule_into_group(char* group_id, openstack_security_rule_p rule_p)
{
	openstack_security_p security_p = g_openstack_security_list;
	while (NULL !=	security_p) {
		if (0 == strcmp(group_id, security_p->security_group)) {
			rule_p->next = security_p->security_rule_p;
			security_p->security_rule_p = rule_p;
			return security_p;
		}
		security_p = security_p->next;
	}

	if (NULL == security_p) {
		security_p = update_openstack_security_group(group_id);
		if (security_p) {
			rule_p->next = security_p->security_rule_p;
			security_p->security_rule_p = rule_p;
			return security_p;
		}
	}
	
	return NULL;
}

openstack_security_rule_p update_security_rule(char* group_id, char* rule_id, char* direction, char* ethertype, char* port_range_max,
		char* port_range_min, char* protocol, char* remote_group_id, char* remote_ip_prefix, char* tenant_id)
{
	openstack_security_rule_p rule_p = find_security_rule_by_rule_id(group_id, rule_id);
	if (NULL == rule_p) {
		rule_p = create_security_rule(group_id, rule_id, direction, ethertype, port_range_max, port_range_min, protocol,
							 remote_group_id, remote_ip_prefix, tenant_id);
		if (rule_p) {
			rule_p->check_status = (UINT2)CHECK_CREATE;
			add_security_rule_into_group(group_id, rule_p);
		}
	}
	else if (compare_security_rule(group_id, rule_id, direction, ethertype, port_range_max, port_range_min, protocol,
							 remote_group_id, remote_ip_prefix, tenant_id, rule_p)) {
		rule_p->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		memcpy(rule_p->direction, direction, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->ethertype, ethertype, OPENSTACK_SECURITY_GROUP_LEN);
		rule_p->port_range_max = (0 == strcmp(port_range_max, "")) ? 0:atoi(port_range_max);
		rule_p->port_range_min = (0 == strcmp(port_range_min, "")) ? 0:atoi(port_range_min);
		memcpy(rule_p->protocol, protocol, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->remote_group_id, remote_group_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->remote_ip_prefix, remote_ip_prefix, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->tenant_id, tenant_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->rule_id, rule_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->group_id, group_id, OPENSTACK_SECURITY_GROUP_LEN);
		rule_p->check_status = (UINT2)CHECK_UPDATE;
	}

	return rule_p;
}


p_fabric_host_node find_fabric_host_port_by_port_id(char* port_id)
{
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;
	while(port != NULL){
		openstack_port_p port_p = (openstack_port_p)port->data;
		if ((NULL != port_p) && (strcmp(port_p->port_id,port_id) == 0)) {
			return port;
		}
		port = port->next;
	}
	return NULL;
}

p_fabric_host_node find_fabric_host_port_by_tenant_id(UINT4 ip, char* tenant_id)
{
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;
	while(port != NULL){
		openstack_port_p port_p = (openstack_port_p)port->data;
		if (((NULL != port_p) &&  (ip == port->ip_list[0]) && (strcmp(port_p->tenant_id,tenant_id) == 0))){
			return port;
		}
		port = port->next;
	}
	return NULL;
}

p_fabric_host_node find_fabric_host_port_by_network_id(UINT4 ip, char* network_id)
{
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;
	while(port != NULL){
		openstack_port_p port_p = (openstack_port_p)port->data;
		if ((NULL != port_p) && (ip == port->ip_list[0]) && (strcmp(port_p->network_id,network_id) == 0)){
			return port;
		}
		port = port->next;
	}
	return NULL;
}

UINT4 find_fabric_host_ports_by_subnet_id(char* subnet_id,p_fabric_host_node* host_list)
{
	UINT4 ret = 0;
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;
	while(port != NULL){
		openstack_port_p port_p = (openstack_port_p)port->data;
		if ((NULL != port_p) && (strcmp(port_p->subnet_id,subnet_id)) == 0){
			host_list[ret] = port;
			ret++;
		}
		port = port->next;
	}
	return ret;
};

p_fabric_host_node find_fabric_host_port_by_subnet_id(UINT4 ip, char* subnet_id)
{
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;
	while(port != NULL){
		openstack_port_p port_p = (openstack_port_p)port->data;
		if ((NULL != port_p) && (ip == port->ip_list[0]) && (strcmp(port_p->subnet_id,subnet_id) == 0)){
			return port;
		}
		port = port->next;
	}
	return NULL;
}

void find_fabric_network_by_floating_ip(UINT4 floating_ip,char* network_id)
{
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;
	while(port != NULL){
		if(floating_ip == port->ip_list[0]){
			openstack_port_p port_p = (openstack_port_p)port->data;
			if (NULL != port_p) {
				strcpy(network_id,port_p->network_id);
				return;
			}
		}
		port = port->next;
	}
};

UINT1 get_openstack_port_type(char* type_str)
{
	enum openstack_port_type returnValue = OPENSTACK_PORT_TYPE_OTHER;

	if ((0 == strcmp("compute:nova", type_str)) || (0 == strcmp("compute:None", type_str))) {
		returnValue =  OPENSTACK_PORT_TYPE_HOST;
	}
	else if (0 == strcmp("network:router_interface", type_str)) {
		returnValue = OPENSTACK_PORT_TYPE_ROUTER_INTERFACE;
	}
	else if (0 == strcmp("network:router_gateway", type_str)) {
		returnValue = OPENSTACK_PORT_TYPE_GATEWAY;
	}
	else if (0 == strcmp("network:floatingip", type_str)) {
		returnValue = OPENSTACK_PORT_TYPE_FLOATINGIP;
	}
	else if (0 == strcmp("network:dhcp", type_str)) {
		returnValue = OPENSTACK_PORT_TYPE_DHCP;
	}
	else if (0 == strcmp("neutron:LOADBALANCER", type_str)) {
		returnValue = OPENSTACK_PORT_TYPE_LOADBALANCER;
	}
	else {
		returnValue = OPENSTACK_PORT_TYPE_OTHER;
	}

	return (UINT1)returnValue;
}



void init_host_check_mgr()
{
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	UINT4 flag_openstack_on = (NULL == value) ? 0: atoi(value);
	value = get_value(g_controller_configure, "[openvstack_conf]", "host_check_on");
	UINT4 flag_host_check_on = (NULL == value) ? 0: atoi(value);
	value = get_value(g_controller_configure, "[openvstack_conf]", "host_check_interval");
	g_host_check_interval = (NULL == value) ? 20: atoi(value);

	if ((flag_openstack_on) && (flag_host_check_on) && (g_host_check_interval)){
		// call flood function
		host_check_tx_timer(NULL, NULL);
	
		// set the timer
		g_host_check_timerid = timer_init(1);
		timer_creat(g_host_check_timerid, g_host_check_interval, NULL, &g_host_check_timer, host_check_tx_timer);
	}
}

void host_check_tx_timer(void *para, void *tid)
{
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "host_check_ovsdb");
	UINT4 flag_ovsdb = (NULL == value) ? 0: atoi(value);
	value = get_value(g_controller_configure, "[openvstack_conf]", "host_check_arp");
	UINT4 flag_arp = (NULL == value) ? 0: atoi(value);


	p_fabric_host_node head = NULL;
	p_fabric_host_node gateway_p = NULL;

	head = g_fabric_host_list.list;

	while (head) {
		// printf("show openstack node:\n");

		if (((OPENSTACK_PORT_TYPE_HOST == head->type) || (OPENSTACK_PORT_TYPE_DHCP == head->type))
			&& (NULL == head->sw) && (0 != head->ip_list[0])) {
			if (flag_arp) {
				gateway_p = find_openstack_app_gateway_by_host(head);
				
				if (NULL != gateway_p) {
					// nat_show_ip(head->ip_list[0]);
					fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], head->ip_list[0], gateway_p->mac);
				}
				else {
					fabric_opnestack_create_arp_flood(g_reserve_ip, head->ip_list[0], g_reserve_mac);
				}
			}

			if (flag_ovsdb) {
				search_host_in_ovsdb_by_mac(head->mac);
			}			
		}
		head = head->next;
	}

}




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

#include "mem_pool.h"
#include "fabric_openstack_arp.h"
#include "timer.h"
#include "openstack_app.h"
#include "openflow-common.h"
#include "../ovsdb/ovsdb.h"
#include "../restful-svr/openstack-server.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "fabric_floating_ip.h"
#include "openstack_security_app.h"
#include "openstack_host.h"
extern t_fabric_host_list g_fabric_host_list;
//by:yhy why?这些全局变量的具体用途存疑
void *g_openstack_host_network_id 	= NULL;
void *g_openstack_host_subnet_id 	= NULL;
void *g_openstack_host_port_id 		= NULL;
void *g_openstack_host_node_id 		= NULL;

//by:yhy openstack中host_network列表
openstack_node_p g_openstack_host_network_list = NULL;
openstack_node_p g_openstack_host_subnet_list = NULL;
openstack_node_p g_openstack_host_port_list = NULL;



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
//by:yhy 初始化openstack_host所需要的一些全局变量
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
	//LOG_PROC("INFO", "%s_%d:g_openstack_security_group_id  %p   %d",FN,LN,g_openstack_security_group_id,sizeof(openstack_security));

	if (g_openstack_security_group_id_temp != NULL) {
		mem_destroy(g_openstack_security_group_id_temp);
	}
	g_openstack_security_group_id_temp = mem_create(sizeof(openstack_security), OPENSTACK_SECURITY_GROUP_MAX_NUM);

	if (g_openstack_security_rule_id != NULL) {
		mem_destroy(g_openstack_security_rule_id);
	}
	g_openstack_security_rule_id = mem_create(sizeof(openstack_security_rule), OPENSTACK_SECURITY_RULE_MAX_NUM);
	
	if (g_openstack_host_security_id != NULL) {
		mem_destroy(g_openstack_host_security_id);
	}
	g_openstack_host_security_id = mem_create(sizeof(openstack_node), OPENSTACK_HOST_SECURITY_MAX_NUM);
	
	

	// clear
	g_openstack_host_network_list = NULL;
	g_openstack_host_subnet_list = NULL;
	g_openstack_host_port_list = NULL;
	g_openstack_security_list = NULL;
	return;
};
////////////////////////////////////////////////////////////////////////
//by:yhy 根据给定的参数创建openstack的host network
openstack_network_p create_openstack_host_network(
		char* tenant_id,
		//char* network_name,
		char* network_id,
		UINT1 shared,
		UINT1 external)
{
	openstack_network_p ret = NULL;
	ret = (openstack_network_p)mem_get(g_openstack_host_network_id);
	if (NULL != ret) 
	{
		memset(ret,0,sizeof(openstack_network));
		strcpy(ret->tenant_id,tenant_id);
		//strcpy(ret->network_name,network_name);
		strcpy(ret->network_id,network_id);
		ret->shared = shared;
		ret->external = external;
	}
	else 
	{
		LOG_PROC("ERROR", "Create openstack host network: Can't get memory.");
	}
	return ret;
};
void destory_openstack_host_network(openstack_network_p network){
	mem_free(g_openstack_host_network_id,network);
	return;
};
//by:yhy 通过network_id查找openstack_host_network
openstack_network_p find_openstack_host_network_by_network_id(char* network_id)
{
	openstack_network_p network = NULL;
	openstack_node_p node_p = g_openstack_host_network_list;
	while(node_p != NULL)
	{
		network = (openstack_network_p)node_p->data;
		if ((NULL != network) && (strcmp(network->network_id,network_id) == 0))
		{
			return network;
		}
		node_p = node_p->next;
	}
	return NULL;
};
//by:yhy 将g_openstack_host_network_list中所有openstack_network_p的check_status属性复位成CHECK_UNCHECKED
void reset_openstack_host_network_flag()
{
	openstack_node_p node_p = g_openstack_host_network_list;
	openstack_network_p network_p = NULL;

	while(node_p != NULL) 
	{
		network_p = (openstack_network_p)node_p->data;
		if (network_p) 
		{
			network_p->check_status = (UINT2)CHECK_UNCHECKED;
		}
		node_p = node_p->next;
	}
}
//by:yhy 将g_openstack_host_subnet_list中所有openstack_subnet_p的check_status属性复位成CHECK_UNCHECKED
void reset_openstack_host_subnet_flag()
{
	openstack_node_p node_p = g_openstack_host_subnet_list;
	openstack_subnet_p subnet_p = NULL;

	while(node_p != NULL) 
	{
		subnet_p = (openstack_subnet_p)node_p->data;
		if (subnet_p) 
		{
			subnet_p->check_status = (UINT2)CHECK_UNCHECKED;
		}
		node_p = node_p->next;
	}
}
//by:yhy 根据g_openstack_host_network_list中各节点的data->check_status状态确定是否删除该节点
//同时删除对应的host配置信息及流表
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

	while (next_p)//by:yhy 遍历 g_openstack_host_network_list
	{
		network_p = (openstack_network_p)next_p->data;
		
		if ((network_p) && (network_p->check_status == CHECK_UNCHECKED)) 
		{
			//by:yhy 删除该节点
			prev_p->next = next_p->next;
			
			LOG_PROC("INFO", "%s %d network_id=%s",FN,LN,network_p->network_id);
			remove_external_port_by_networkid(network_p->network_id);
			mem_free(g_openstack_host_network_id, network_p);
			mem_free(g_openstack_host_node_id, next_p);
		}
		else 
		{
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	network_p = (openstack_network_p)head_p->data;
	if ((network_p) && (network_p->check_status == CHECK_UNCHECKED)) 
	{
		//by:yhy 检查头结点是否删除
		next_p = head_p->next;
		LOG_PROC("INFO", "%s %d network_id=%s",FN,LN,network_p->network_id);
		remove_external_port_by_networkid(network_p->network_id);
		mem_free(g_openstack_host_network_id, head_p->data);
		mem_free(g_openstack_host_node_id, head_p);
		g_openstack_host_network_list= next_p;
	}	
}
//by:yhy 根据g_openstack_host_subnet_list中各节点的data->check_status状态确定是否删除该节点
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

	while (next_p) 
	{
		//by:yhy 遍历list确定删除项
		subnet_p = (openstack_subnet_p)next_p->data;
		
		if ((subnet_p) && (subnet_p->check_status == CHECK_UNCHECKED)) 
		{
			prev_p->next = next_p->next;
			//remove_proactive_floating_internal_subnet_flow_by_subnet(subnet_p);
			mem_free(g_openstack_host_subnet_id, subnet_p);
			mem_free(g_openstack_host_node_id, next_p);
		}
		else 
		{
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	subnet_p = (openstack_subnet_p)head_p->data;
	if ((subnet_p) && (subnet_p->check_status == CHECK_UNCHECKED)) 
	{
		//by:yhy 判断是否是删除头结点
		next_p = head_p->next;
		//remove_proactive_floating_internal_subnet_flow_by_subnet(subnet_p);
		mem_free(g_openstack_host_subnet_id, head_p->data);
		mem_free(g_openstack_host_node_id, head_p);
		g_openstack_host_subnet_list= next_p;
	}	
}

//by:yhy openstack中定时刷新的host_network相关
void reload_openstack_host_network()
{
	INT4 iRet = GN_ERR;
	reset_openstack_host_network_flag();
	reset_openstack_host_subnet_flag();
	iRet = reload_net_info();
	if(GN_OK == iRet)
	{
		remove_openstack_host_network_by_check_flag();
		remove_openstack_host_subnet_by_check_flag();
	}
}
//by:yhy 将network添加到g_openstack_host_network_list中
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
//by:yhy 在g_openstack_host_subnet_id中添加一个以给定参数构成的openstack_subnet_p
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
	if (NULL != ret) 
	{
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
	else 
	{
		LOG_PROC("ERROR", "Openstack Subnet: Create failed, Can't get memory.");
	}

	return ret;
};
void destory_openstack_host_subnet(openstack_subnet_p subnet){
	mem_free(g_openstack_host_subnet_id,subnet);
	return;
};
//by:yhy 根据给定的参数,新增一个openstack的host节点
void add_openstack_host_subnet(openstack_subnet_p subnet)
{
	openstack_subnet_p subnet_p = NULL;
	openstack_node_p node_p = NULL;
	if(subnet == NULL)
	{
		return;
	}

	subnet_p = remove_openstack_host_subnet_by_subnet_id(subnet->subnet_id);
	if(subnet_p != NULL)
	{
		return;
	}
	node_p = create_openstack_host_node((UINT1*)subnet);
	if (NULL != node_p) 
	{
		node_p->next = g_openstack_host_subnet_list;
		g_openstack_host_subnet_list = node_p;
	}

	return;
};
//by:yhy 根据subnet_id 在g_openstack_host_subnet_list中查找对应的(openstack_subnet_p)subnet
openstack_subnet_p find_openstack_host_subnet_by_subnet_id(char* subnet_id)
{
	openstack_subnet_p subnet = NULL;
	openstack_node_p node_p = g_openstack_host_subnet_list;
	while(node_p != NULL)
	{
		subnet = (openstack_subnet_p)node_p->data;
		if ((NULL != subnet) && (strcmp(subnet->subnet_id,subnet_id) == 0)) 
		{
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
//by:yhy 根据源主机和IP查找对应的openstack主机
p_fabric_host_node find_openstack_host_by_srcport_ip(p_fabric_host_node host_p, UINT4 ip)
{
	p_fabric_host_node ret = NULL;
	ret = g_fabric_host_list.list;
	openstack_port_p port_p = (openstack_port_p)host_p->data;
	if ((NULL == port_p) || (0 == strlen(port_p->tenant_id)))
	{
		return NULL;
	}
	
	while(ret != NULL)
	{//by:yhy 遍历g_fabric_host_list.list下每个主机的openstack_port_p
		openstack_port_p list_port_p = (openstack_port_p)ret->data;
		if ((NULL != list_port_p) && (strlen(list_port_p->tenant_id)) && (check_IP_in_fabric_host(ret,ip))) 
		{
			if (0 == strcmp(port_p->tenant_id, list_port_p->tenant_id)) 
			{
				return ret;
			}

			if (strlen(list_port_p->network_id)) 
			{
				openstack_network_p network_p = find_openstack_host_network_by_network_id(list_port_p->network_id);
				if (1 == network_p->shared) 
				{
					return ret;
				}
			}
		}
		ret = ret->next;
	}
	return NULL;
};
//by:yhy 在g_openstack_host_subnet_list中查找对应subnet_id为给定参数的那个openstack_node_p并删除
openstack_subnet_p remove_openstack_host_subnet_by_subnet_id(char* subnet_id)
{
	openstack_subnet_p subnet = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_subnet_list;

	while(node_p->next != NULL)
	{
		subnet = (openstack_subnet_p)(node_p->next->data);
		if ((NULL != subnet) && (strcmp(subnet->subnet_id,subnet_id) == 0)) 
		{
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
//by:yhy 根据给定参数创建 (p_fabric_host_node)host节点
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
		UINT1* security_data)
{
	UINT8 dpid = 0;
	p_fabric_host_node node = NULL;
	openstack_port_p ret = NULL;
	ret = (openstack_port_p)mem_get(g_openstack_host_port_id);
	if (NULL != ret) 
	{
		memset(ret,0,sizeof(openstack_port));
		if (NULL != tenant_id)
		{
			strcpy(ret->tenant_id,tenant_id);
		}
		if (NULL != network_id)
		{
			strcpy(ret->network_id,network_id);
		}
		if (NULL != port_id)
		{
			strcpy(ret->port_id,port_id);
		}
		if (NULL != subnet_id)
		{
			strcpy(ret->subnet_id, subnet_id);
		}

		if (NULL != sw)
		{
			dpid = sw->dpid;
		}
		ret->security_num = security_num;
		if (NULL != security_data)
		{
			ret->security_data = security_data;
		}

		node = insert_fabric_host_into_list_paras(sw, dpid, port, mac, ip, ipv6);
		if (NULL != node) 
		{
			node->data = (void*)ret;
			node->type = port_type;
		}
		else 
		{
			mem_free(g_openstack_host_port_id, ret);
		}
	}
	else 
	{
		LOG_PROC("ERROR", "Create openstack host port: Can't get memory.");
	}
	return (void*)node;
};
void destory_openstack_host_port(openstack_port_p port){
	mem_free(g_openstack_host_port_id,port);
	return;
};
//by:yhy <费解>  将sw,port更新入mac在g_fabric_host_list.list中对应的host节点
void update_openstack_host_port_by_mac(UINT1* mac, gn_switch_t* sw, UINT4 port)
{
	p_fabric_host_node host = get_fabric_host_from_list_by_mac(mac);
	if ((sw) && (host) && (port)) 
	{
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
//by:yhy 在g_openstack_host_node_id中添加一个openstack_node_p节点并赋值
openstack_node_p create_openstack_host_node(UINT1* data)
{
	openstack_node_p ret = NULL;
	ret = (openstack_node_p)mem_get(g_openstack_host_node_id);
	if (NULL != ret) 
	{
		memset(ret,0,sizeof(openstack_node));
		ret->data = data;
	}
	else 
	{
		LOG_PROC("ERROR", "Create openstack host node: Create fail, can't get memory.");
	}
	return ret;
};
void destory_openstack_host_node(openstack_node_p node){
	mem_free(g_openstack_host_node_id,node);
	return;
};








//by:yhy find_fabric_host_port_by_port_id
p_fabric_host_node find_fabric_host_port_by_port_id(char* port_id)
{
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;
	while(port != NULL)
	{
		openstack_port_p port_p = (openstack_port_p)port->data;
		if ((NULL != port_p) && (strcmp(port_p->port_id,port_id) == 0)) 
		{
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
//by:yhy 通过subnet_id和目标IP查找对应fabric_host_port
p_fabric_host_node find_fabric_host_port_by_subnet_id(UINT4 ip, char* subnet_id)
{
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;
	while(port != NULL)
	{
		openstack_port_p port_p = (openstack_port_p)port->data;
		if ((NULL != port_p) && (ip == port->ip_list[0]) && (strcmp(port_p->subnet_id,subnet_id) == 0))
		{
			return port;
		}
		port = port->next;
	}
	return NULL;
}
//by:yhy 根据floating_ip在g_fabric_host_list.list查找与之对应的p_fabric_host_node节点,找到其network_id
INT4 find_fabric_network_by_floating_ip(UINT4 floating_ip,char* network_id, char* subnet_id)
{
	p_fabric_host_node port = NULL;
	port = g_fabric_host_list.list;

	if((NULL == network_id)||(NULL == subnet_id))
	{
		return GN_ERR;
	}
	
	while(port != NULL)
	{
		if(floating_ip == port->ip_list[0])
		{
			openstack_port_p port_p = (openstack_port_p)port->data;
			if (NULL != port_p) 
			{
				strcpy(network_id,port_p->network_id);
				strcpy(subnet_id ,port_p->subnet_id);
				return GN_OK;
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
	else if(0 == strcmp("network:LOADBALANCER_VIP", type_str))
	{
		returnValue =  OPENSTACK_PORT_TYPE_CLBLOADBALANCER;
	}
	else if(0 == strcmp("network:LOADBALANCER_HA", type_str))
	{
		returnValue = OPENSTACK_PORT_TYPE_CLBLOADBALANCER_HA;
	}
	else {
		returnValue = OPENSTACK_PORT_TYPE_OTHER;
	}

	return (UINT1)returnValue;
}


//by:yhy 主机检查(更倾向于通过泛洪和检查ovsdb来定期刷新主机)
void init_host_check_mgr()
{
	void *clbviphost_check_timer = NULL;
	void *clbviphost_check_timerid = NULL;
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	UINT4 flag_openstack_on = (NULL == value) ? 0: atoi(value);
	value = get_value(g_controller_configure, "[openvstack_conf]", "host_check_on");
	UINT4 flag_host_check_on = (NULL == value) ? 0: atoi(value);
	value = get_value(g_controller_configure, "[openvstack_conf]", "host_check_interval");
	g_host_check_interval = (NULL == value) ? 20: atoi(value);

	if ((flag_openstack_on) && (flag_host_check_on) && (g_host_check_interval))
	{
		host_check_tx_timer(NULL, NULL);
		clbviphost_check_tx_timer(NULL, NULL);
		g_host_check_timerid = timer_init(1);
		clbviphost_check_timerid = timer_init(1);
		timer_creat(g_host_check_timerid, g_host_check_interval, NULL, &g_host_check_timer, host_check_tx_timer);
		timer_creat(clbviphost_check_timerid, 10, NULL, &clbviphost_check_timer, clbviphost_check_tx_timer);
	}
}
//by:yhy 定期检查host主机
//host_check_ovsdb配置开,则在ovsdb中查询主机MAC
//host_check_arp配置开,则洪泛主机MAC
void host_check_tx_timer(void *para, void *tid)
{
	LOG_PROC("TIMER", "host_check_tx_timer - START");
	UINT4 inside_vipIp = 0;
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "host_check_ovsdb");
	UINT4 flag_ovsdb = (NULL == value) ? 0: atoi(value);
	value = get_value(g_controller_configure, "[openvstack_conf]", "host_check_arp");
	UINT4 flag_arp = (NULL == value) ? 0: atoi(value);


	p_fabric_host_node head = NULL;
	p_fabric_host_node gateway_p = NULL;
	p_fabric_host_node inside_viphost = NULL;

	if (OFPCR_ROLE_SLAVE == g_controller_role) 
    {
         return ;
    }
	head = g_fabric_host_list.list;

	while (head) 
	{//by:yhy 遍历g_fabric_host_list.list
		if (((OPENSTACK_PORT_TYPE_HOST == head->type) || (OPENSTACK_PORT_TYPE_DHCP == head->type))&& (NULL == head->sw)&& (0 != head->ip_list[0]))
		{
			if (flag_arp) 
			{
				gateway_p = find_openstack_app_gateway_by_host(head);
				
				if (NULL != gateway_p) 
				{//by:yhy 若找到网关,则在网关的IP与MAC为源做洪泛
					//nat_show_ip(head->ip_list[0]);
					fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], head->ip_list[0], gateway_p->mac);
				}
				else 
				{//by:yhy 若未找到网关,则在控制器的IP与MAC为源做洪泛
					fabric_opnestack_create_arp_flood(htonl(g_reserve_ip), head->ip_list[0], g_reserve_mac);
				}
			}

			if (flag_ovsdb) 
			{
				search_host_in_ovsdb_by_mac(head->mac);
			}			
		}
	
			
		head = head->next;
	}
	LOG_PROC("TIMER", "host_check_tx_timer - STOP");
	fabric_firewall_RefreshSwitch();
}

void clbviphost_check_tx_timer(void *para, void *tid)
{
	UINT4 inside_vipIp = 0;
	p_fabric_host_node head = NULL;
	p_fabric_host_node inside_viphost = NULL;
	external_port_p epp = NULL;
	gn_switch_t * external_sw = NULL;
	p_fabric_host_node gateway_p = NULL;
	//openstack_clbass_vips_sw_list_p vips_sw_list_node = NULL;

	if (OFPCR_ROLE_SLAVE == g_controller_role) 
    {
         return ;
    }
	head = g_fabric_host_list.list;
	while (head) 
	{

		// add by ycy for 华云
		if ((OPENSTACK_PORT_TYPE_CLBLOADBALANCER == head->type)&& (0 != head->ip_list[0])) //&& (NULL == head->sw) 
		{
			
			inside_vipIp = find_openstack_clbaas_vipfloatingpool_insideip_by_extip(head->ip_list[0]);
			if(inside_vipIp)
			{
				//LOG_PROC("INFO", "***************%s %d head->ip_list[0]=0x%x inside_vipIp=0x%x",FN,LN,head->ip_list[0], inside_vipIp);
				inside_viphost =  get_fabric_host_from_list_by_ip(inside_vipIp);
				if(inside_viphost)//&&inside_viphost->sw
				{
					#if 0
					//vips_sw_list_node = find_openstack_vips_sw_list_byip(inside_vipIp);
					if(head->sw&&(inside_viphost->sw != head->sw))
					{
						LOG_PROC("INFO", "%s %d #################HA####################inside_viphost->sw->sw_ip=0x%x head->sw->sw_ip=0x%x",FN,LN,inside_viphost->sw,head->sw->sw_ip);

						vips_sw_list_node = update_openstack_vips_sw_list( inside_vipIp, inside_viphost->sw);
						remove_openstack_clbaas_backend_internalflows_byMac(inside_viphost->sw, inside_viphost->mac);

						remove_openstack_clbaas_backend_internalflows_byIp(inside_vipIp);
						epp  = get_external_port_by_hostip(head->ip_list[0]);
						if(epp)
						{
							external_sw = get_ext_sw_by_dpid(epp->external_dpid);
							if(external_sw)
							{
								
								LOG_PROC("INFO", "%s %d #################HA####################",FN,LN);
								remove_clbforward_flow_by_SrcIp(external_sw, head->ip_list[0]);
								
							}
						}
						//head->sw = inside_viphost->sw;
					}
					#endif
					
					fabric_opnestack_create_icmp_flood_allsw( htonl(g_reserve_ip), head->ip_list[0], g_reserve_mac, head->mac);
					
					//LOG_PROC("INFO", "***************%s %d inside_vipIp=0x%x inside_viphost->sw->sw_ip=0x%x g_reserve_ip=0x%x",FN,LN, inside_vipIp,inside_viphost->sw->sw_ip,g_reserve_ip);
					//icmp flood to sw
					//fabric_opnestack_create_icmp_flood(inside_viphost->sw, htonl(g_reserve_ip), head->ip_list[0], g_reserve_mac, head->mac);
					
					gateway_p = find_openstack_app_gateway_by_host(inside_viphost);

					if((NULL != gateway_p)&&head->sw&&(NULL==inside_viphost->sw))
					{
						
						fabric_opnestack_create_clb_arpflood( head->sw, gateway_p->ip_list[0], inside_viphost->ip_list[0], gateway_p->mac, inside_viphost->mac);
					}
					
				}
			}
		}
	
		if((OPENSTACK_PORT_TYPE_CLBLOADBALANCER_HA == head->type)&&(0 != head->ip_list[0])&&(NULL == head->sw))	
		{
				gateway_p = find_openstack_app_gateway_by_host( head);
				if(NULL != gateway_p)
				{
					fabric_opnestack_create_icmp_flood_allsw( gateway_p->ip_list[0], head->ip_list[0], gateway_p->mac, head->mac);
					
					LOG_PROC("INFO","###############%s %d head->ip_list[0]=0x%x",FN,LN, head->ip_list[0]);
				}
				
		}
	
		
		head = head->next;
	}
}



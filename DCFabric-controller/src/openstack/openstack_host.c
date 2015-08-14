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

void *g_openstack_host_network_id = NULL;
void *g_openstack_host_subnet_id = NULL;
void *g_openstack_host_port_id = NULL;
void *g_openstack_host_node_id = NULL;

openstack_node_p g_openstack_host_network_list = NULL;
openstack_node_p g_openstack_host_subnet_list = NULL;
openstack_node_p g_openstack_host_port_list = NULL;
////////////////////////////////////////////////////////////////////////
void openstack_show_port(openstack_node_p node){
	openstack_port_p p = (openstack_port_p)(node->data);
	printf("Port: | port_no: %d | port: %s | subnet: %s | network: %s | tenant: %s |\n",p->port,p->port_id,p->subnet_id,p->network_id,p->tenant_id);
};
void openstack_show_subnet(openstack_node_p node){
	openstack_subnet_p p = (openstack_subnet_p)(node->data);
	printf("Subnet: | subnet: %s | network: %s | tenant: %s |\n",p->subnet_id,p->network_id,p->tenant_id);
};

void openstack_show_network(openstack_node_p node){
	openstack_network_p p = (openstack_network_p)(node->data);
	printf("Network: | network: %s | tenant: %s |\n",p->network_id,p->tenant_id);

};
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
	// clear
	g_openstack_host_network_list = NULL;
	g_openstack_host_subnet_list = NULL;
	g_openstack_host_port_list = NULL;
	return;
};
////////////////////////////////////////////////////////////////////////
openstack_network_p create_openstack_host_network(
		char* tenant_id,
		//char* network_name,
		char* network_id,
		UINT1 shared){
	openstack_network_p ret = NULL;
	ret = (openstack_network_p)mem_get(g_openstack_host_network_id);
	memset(ret,0,sizeof(openstack_network));
	strcpy(ret->tenant_id,tenant_id);
	//strcpy(ret->network_name,network_name);
	strcpy(ret->network_id,network_id);
	ret->shared = shared;
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
		if(strcmp(network->network_id,network_id) == 0){
			return network;
		}
		node_p = node_p->next;
	}
	return NULL;
};
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
	node_p->next = g_openstack_host_network_list;
	g_openstack_host_network_list = node_p;

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
		if(strcmp(network->network_id,network_id) == 0){
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
		UINT4 end_ip){
	openstack_subnet_p ret = NULL;
	ret = (openstack_subnet_p)mem_get(g_openstack_host_subnet_id);
	memset(ret,0,sizeof(openstack_subnet));

	strcpy(ret->tenant_id,tenant_id);
	strcpy(ret->network_id,network_id);
	//strcpy(ret->subnet_name,subnet_name);
	strcpy(ret->subnet_id, subnet_id);
	ret->gateway_ip = gateway_ip;
	ret->start_ip = start_ip;
	ret->end_ip = end_ip;
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
	node_p->next = g_openstack_host_subnet_list;
	g_openstack_host_subnet_list = node_p;
	return;
};
openstack_subnet_p find_openstack_host_subnet_by_subnet_id(char* subnet_id){
	openstack_subnet_p subnet = NULL;
	openstack_node_p node_p = g_openstack_host_subnet_list;
	while(node_p != NULL){
		subnet = (openstack_subnet_p)node_p->data;
		if(strcmp(subnet->subnet_id,subnet_id) == 0){
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


openstack_subnet_p remove_openstack_host_subnet_by_subnet_id(char* subnet_id){
	openstack_subnet_p subnet = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_subnet_list;

	while(node_p->next != NULL){
		subnet = (openstack_subnet_p)(node_p->next->data);
		if(strcmp(subnet->subnet_id,subnet_id) == 0){
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
		if(strcmp(subnet->tenant_id,tenant_id) == 0){
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
		if(strcmp(subnet->network_id,network_id) == 0){
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
openstack_port_p create_openstack_host_port(
		gn_switch_t* sw,
		UINT4 port,
		UINT4 ip,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p ret = NULL;
	ret = (openstack_port_p)mem_get(g_openstack_host_port_id);
	memset(ret,0,sizeof(openstack_port));

	strcpy(ret->tenant_id,tenant_id);
	strcpy(ret->network_id,network_id);
	strcpy(ret->port_id,port_id);
	strcpy(ret->subnet_id, subnet_id);
	ret->ip = ip;
	ret->sw = sw;
	if(sw != NULL)
		ret->dpid = sw->dpid;
	ret->port = port;
	memcpy(ret->mac, mac, 6);
	return ret;
};
void destory_openstack_host_port(openstack_port_p port){
	mem_free(g_openstack_host_port_id,port);
	return;
};
void add_openstack_host_port(openstack_port_p port){
	openstack_port_p port_p = NULL;
	openstack_node_p node_p = NULL;
	if(port == NULL){
		return;
	}

	port_p = find_openstack_host_port_by_port_id(port->port_id);
	if(port_p != NULL){
		return;
	}
	node_p = create_openstack_host_node((UINT1*)port);
	node_p->next = g_openstack_host_port_list;
	g_openstack_host_port_list = node_p;
	return;
};

void set_openstack_host_port_portno(const UINT1 *mac, UINT4 ofport_no)
{
    openstack_port_p port = NULL;
    openstack_node_p node_p = g_openstack_host_port_list;
    while(node_p != NULL)
    {
        port = (openstack_port_p)node_p->data;
        if(memcmp(port->mac, mac, 6) == 0)
        {
            port->ofport_no = ofport_no;
            return;
        }
        node_p = node_p->next;
    }
}

openstack_port_p find_openstack_host_port_by_port_id(char* port_id){
	openstack_port_p port = NULL;
	openstack_node_p node_p = g_openstack_host_port_list;
	while(node_p != NULL){
		port = (openstack_port_p)node_p->data;
		if(strcmp(port->port_id,port_id) == 0){
			return port;
		}
		node_p = node_p->next;
	}
	return NULL;
};
openstack_port_p remove_openstack_host_port_by_port_id(char* port_id){
	openstack_port_p port = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_subnet_list;

	while(node_p->next != NULL){
		port = (openstack_port_p)(node_p->next->data);
		if(0 == strcmp(port->port_id,port_id)){
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			g_openstack_host_port_list = node.next;
			return port;
		}
		node_p = node_p->next;
	}
	g_openstack_host_port_list = node.next;
	return NULL;
};
openstack_port_p find_openstack_host_port_by_mac(UINT1* mac){
	openstack_port_p port = NULL;
	openstack_node_p node_p = g_openstack_host_port_list;
	while(node_p != NULL){
		port = (openstack_port_p)node_p->data;
		if(0 == memcmp(port->mac, mac, 6)){
			return port;
		}
		node_p = node_p->next;
	}
	return NULL;
};

openstack_port_p remove_openstack_host_port_by_mac(UINT1* mac){
	openstack_port_p port = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_subnet_list;

	while(node_p->next != NULL){
		port = (openstack_port_p)(node_p->next->data);
		if(0 == memcmp(port->mac, mac, 6)){
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			g_openstack_host_port_list = node.next;
			return port;
		}
		node_p = node_p->next;
	}
	g_openstack_host_port_list = node.next;
	return NULL;
};

openstack_port_p find_openstack_host_port_by_ip_tenant(UINT4 ip,char* tenant_id){
	openstack_port_p port = NULL;
	openstack_node_p node_p = g_openstack_host_port_list;
	while(node_p != NULL){
		port = (openstack_port_p)node_p->data;
		if(port->ip == ip && 0 == strcmp(port->tenant_id,tenant_id)){
			return port;
		}
		node_p = node_p->next;
	}
	return NULL;
};
openstack_port_p find_openstack_host_port_by_ip_network(UINT4 ip,char* network_id){
	openstack_port_p port = NULL;
	openstack_node_p node_p = g_openstack_host_port_list;
	while(node_p != NULL){
		port = (openstack_port_p)node_p->data;
		if(port->ip == ip && 0 == strcmp(port->network_id,network_id)){
			return port;
		}
		node_p = node_p->next;
	}
	return NULL;
};

openstack_port_p find_openstack_host_port_by_ip_subnet(UINT4 ip,char* subnet_id){
	openstack_port_p port = NULL;
	openstack_node_p node_p = g_openstack_host_port_list;
	while(node_p != NULL){
		port = (openstack_port_p)node_p->data;
		if(port->ip == ip && 0 == strcmp(port->subnet_id,subnet_id)){
			return port;
		}
		node_p = node_p->next;
	}
	return NULL;
};

UINT4 find_openstack_host_ports_by_subnet_id(char* subnet_id,openstack_port_p* host_list){
	UINT4 ret = 0;
	openstack_port_p port = NULL;
	openstack_node_p node_p = NULL;
	node_p = g_openstack_host_port_list;

	while(node_p != NULL){
		port = (openstack_port_p)(node_p->data);
		if(strcmp(port->subnet_id,subnet_id) == 0){
			host_list[ret] = port;
			ret++;
		}
		node_p = node_p->next;
	}
	return ret;
};

void delete_openstack_host_port_by_tenant_id(char* tenant_id){
	openstack_port_p port = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_port_list;

	while(node_p->next != NULL){
		port = (openstack_port_p)(node_p->next->data);
		if(strcmp(port->tenant_id,tenant_id) == 0){
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			destory_openstack_host_port(port);
		}else{
			node_p = node_p->next;
		}
	}
	g_openstack_host_port_list = node.next;
	return;
};
void delete_openstack_host_port_by_network_id(char* network_id){
	openstack_port_p port = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_port_list;

	while(node_p->next != NULL){
		port = (openstack_port_p)(node_p->next->data);
		if(strcmp(port->network_id,network_id) == 0){
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			destory_openstack_host_port(port);
		}else{
			node_p = node_p->next;
		}
	}
	g_openstack_host_port_list = node.next;
	return;
};
void delete_openstack_host_port_by_subnet_id(char* subnet_id){
	openstack_port_p port = NULL;
	openstack_node node;
	openstack_node_p node_p = &node,temp_p = NULL;
	node_p->next = g_openstack_host_port_list;

	while(node_p->next != NULL){
		port = (openstack_port_p)(node_p->next->data);
		if(strcmp(port->subnet_id,subnet_id) == 0){
			temp_p = node_p->next;
			node_p->next = temp_p->next;
			destory_openstack_host_node(temp_p);
			destory_openstack_host_port(port);
		}else{
			node_p = node_p->next;
		}
	}
	g_openstack_host_port_list = node.next;
	return;
};
void delete_openstack_host_port_by_port_id(char* port_id){
	openstack_port_p port = NULL;
	port = remove_openstack_host_port_by_port_id(port_id);
	if(port != NULL){
		destory_openstack_host_port(port);
	}
	return;
};
////////////////////////////////////////////////////////////////////////
openstack_node_p create_openstack_host_node(UINT1* data){
	openstack_node_p ret = NULL;
	ret = (openstack_node_p)mem_get(g_openstack_host_node_id);
	memset(ret,0,sizeof(openstack_node));
	ret->data = data;
	return ret;
};
void destory_openstack_host_node(openstack_node_p node){
	mem_free(g_openstack_host_node_id,node);
	return;
};



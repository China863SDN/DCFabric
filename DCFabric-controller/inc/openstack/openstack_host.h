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
 * openstack_host.h
 *
 *  Created on: Jun 16, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */

#ifndef INC_OPENSTACK_OPENSTACK_HOST_H_
#define INC_OPENSTACK_OPENSTACK_HOST_H_
#include "gnflush-types.h"
// tenant
#define OPENSTACK_TENANT_ID_LEN 48

// cidr
#define OPENSTACK_CIDR_ID_LEN 48

// network
#define OPENSTACK_NETWORK_ID_LEN 48
#define OPENSTACK_NETWORK_NAME_LEN 48
#define OPENSTACK_NETWORK_MAX_NUM 128

// subnet
#define OPENSTACK_SUBNET_ID_LEN 48
#define OPENSTACK_SUBNET_NAME_LEN 48
#define OPENSTACK_SUBNET_MAX_NUM 1024

// port
#define OPENSTACK_PORT_ID_LEN 48
#define OPENSTACK_PORT_MAX_NUM 10240

// security group
#define OPENSTACK_SECURITY_GROUP_LEN 48
#define OPENSTACK_SECURITY_GROUP_MAX_NUM 128
#define OPENSTACK_SECURITY_RULE_MAX_NUM 1024
//firewall
#define OPENSTACK_FIREWALL_LEN 48
#define OPENSTACK_FIREWALL_MAX_NUM 128
#define OPENSTACK_FIREWALL_RULE_MAX_NUM 1024

// port security
#define OPENSTACK_HOST_SECURITY_MAX_NUM 2048

// node
#define OPENSTACK_NODE_MAX_NUM (OPENSTACK_NETWORK_MAX_NUM+OPENSTACK_SUBNET_MAX_NUM+OPENSTACK_PORT_MAX_NUM)

#include "fabric_host.h"

////////////////////////////////////////////////////////////////////////
typedef struct _openstack_port{
//	gn_switch_t* sw;
//	UINT8 dpid;
//	UINT4 port;
//	UINT4 ip;
//	UINT1 mac[6];
//	UINT4 ofport_no;
	char tenant_id[OPENSTACK_TENANT_ID_LEN];
	char network_id[OPENSTACK_NETWORK_ID_LEN];
	char subnet_id[OPENSTACK_SUBNET_ID_LEN];
	char port_id[OPENSTACK_PORT_ID_LEN];
	UINT2 security_num;
	UINT1* security_data;
}openstack_port,* openstack_port_p;

typedef struct _openstack_subnet{
	char tenant_id[OPENSTACK_TENANT_ID_LEN];
	char network_id[OPENSTACK_NETWORK_ID_LEN];
	//char subnet_name[OPENSTACK_SUBNET_NAME_LEN];
	char subnet_id[OPENSTACK_SUBNET_ID_LEN];
	UINT4 gateway_ip;
	UINT1 gateway_ipv6[16];
	UINT4 start_ip;
	UINT4 end_ip;
	UINT1 start_ipv6[16];
	UINT1 end_ipv6[16];
	char cidr[OPENSTACK_CIDR_ID_LEN];
	void* gateway_port;
	void* dhcp_port;
	UINT4 port_num;
	UINT1 external;
	UINT1 check_status;
    UINT4 installed_flow_count;
}openstack_subnet,* openstack_subnet_p;

typedef struct _openstack_network{
	char tenant_id[OPENSTACK_TENANT_ID_LEN];
	char network_id[OPENSTACK_NETWORK_ID_LEN];
	UINT4 subnet_num;
	UINT1 shared;
	UINT1 external;
	UINT1 check_status;
}openstack_network,* openstack_network_p;

typedef struct _openstack_node{
	UINT1* data;
	struct _openstack_node* next;
}openstack_node,*openstack_node_p;

typedef struct _openstack_security_rule {
	char group_id[OPENSTACK_SECURITY_GROUP_LEN];
	char direction[OPENSTACK_SECURITY_GROUP_LEN];
	char ethertype[OPENSTACK_SECURITY_GROUP_LEN];
	char rule_id[OPENSTACK_SECURITY_GROUP_LEN];
	UINT4 port_range_max;
	UINT4 port_range_min;
	char protocol[OPENSTACK_SECURITY_GROUP_LEN];
	char remote_group_id[OPENSTACK_SECURITY_GROUP_LEN];
	char remote_ip_prefix[OPENSTACK_SECURITY_GROUP_LEN];
	char tenant_id[OPENSTACK_SECURITY_GROUP_LEN];
	UINT2 check_status;
	struct _openstack_security_rule* next;
} openstack_security_rule, *openstack_security_rule_p;


typedef struct _openstack_firewall_rule {
	char direction[OPENSTACK_FIREWALL_LEN];
	char ethertype[OPENSTACK_FIREWALL_LEN];
	char rule_id[OPENSTACK_FIREWALL_LEN];
	UINT4 port_range_max;
	UINT4 port_range_min;
	char protocol[OPENSTACK_FIREWALL_LEN];
	char remote_group_id[OPENSTACK_FIREWALL_LEN];
	char remote_ip_prefix[OPENSTACK_FIREWALL_LEN];
	char tenant_id[OPENSTACK_FIREWALL_LEN];
	struct _openstack_firewall_rule* next;
} openstack_firewall_rule, *openstack_firewall_rule_p;

typedef struct _openstack_security {
	char security_group[OPENSTACK_SECURITY_GROUP_LEN];
	openstack_security_rule_p security_rule_p;
	struct _openstack_security* next;
} openstack_security, *openstack_security_p;


typedef struct _openstack_firewall {
	char firewall_fwaas[OPENSTACK_FIREWALL_LEN];
	openstack_firewall_rule_p firewall_rule_p;
	struct _openstack_firewall* next;
} openstack_firewall, *openstack_firewall_p;
////////////////////////////////////////////////////////////////////////
/*
 * [openstack/neutron.git] / neutron / common / constants.py
 *
 *   DEVICE_OWNER_ROUTER_HA_INTF = "network:router_ha_interface"
 *   DEVICE_OWNER_ROUTER_INTF = "network:router_interface"
 *   DEVICE_OWNER_ROUTER_GW = "network:router_gateway"
 *   DEVICE_OWNER_FLOATINGIP = "network:floatingip"
 *   DEVICE_OWNER_DHCP = "network:dhcp"
 *   DEVICE_OWNER_DVR_INTERFACE = "network:router_interface_distributed"
 *   DEVICE_OWNER_AGENT_GW = "network:floatingip_agent_gateway"
 *   DEVICE_OWNER_ROUTER_SNAT = "network:router_centralized_snat"
 *   DEVICE_OWNER_LOADBALANCER = "neutron:LOADBALANCER"
 */

enum openstack_port_type
{
	OPENSTACK_PORT_TYPE_OTHER = 0,			// 0: other
	OPENSTACK_PORT_TYPE_HOST, 				// 1: "compute:nova"
	OPENSTACK_PORT_TYPE_ROUTER_INTERFACE,	// 2: "network:router_interface"
	OPENSTACK_PORT_TYPE_GATEWAY,			// 3: "network:router_gateway"
	OPENSTACK_PORT_TYPE_FLOATINGIP,			// 4: "network:floatingip"
	OPENSTACK_PORT_TYPE_DHCP,				// 5: "network:dhcp"
	OPENSTACK_PORT_TYPE_LOADBALANCER,		// 6: "neutron:LOADBALANCER"
};

UINT1 get_openstack_port_type(char* type_str);
//////////////////////////////////////////////////////////////////////



void init_openstack_host();
void show_openstack_total();
void destory_openstack_host();

////////////////////////////////////////////////////////////////////////
openstack_network_p create_openstack_host_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external);
void destory_openstack_host_network(openstack_network_p network);
void reload_openstack_host_network();
void add_openstack_host_network(openstack_network_p network);
openstack_network_p find_openstack_host_network_by_network_id(char* network_id);
openstack_network_p remove_openstack_host_network_by_network_id(char* network_id);


////////////////////////////////////////////////////////////////////////
openstack_subnet_p create_openstack_host_subnet(
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		UINT4 gateway_ip,
		UINT4 start_ip,
		UINT4 end_ip,
		UINT1* gateway_ipv6,
		UINT1* start_ipv6,
		UINT1* end_ipv6,
		char* cidr,
		UINT1 external);
void destory_openstack_host_subnet(openstack_subnet_p subnet);
void add_openstack_host_subnet(openstack_subnet_p subnet);
openstack_subnet_p find_openstack_host_subnet_by_subnet_id(char* subnet_id);
openstack_subnet_p remove_openstack_host_subnet_by_subnet_id(char* subnet_id);
openstack_subnet_p find_openstack_host_subnet_by_geteway_ip(UINT4 gateway_ip);
//void find_openstack_network_by_floating_ip(UINT4 floating_ip,char* network_id);

void delete_openstack_host_subnet_by_tenant_id(char* tenant_id);
void delete_openstack_host_subnet_by_network_id(char* network_id);
void delete_openstack_host_subnet_by_subnet_id(char* subnet_id);
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
		UINT1* security_data);
void destory_openstack_host_port(openstack_port_p port);
//void add_openstack_host_port(openstack_port_p port);
//void set_openstack_host_port_portno(const UINT1 *mac, UINT4 ofport_no);
//openstack_port_p find_openstack_host_port_by_port_id(char* port_id);
//openstack_port_p remove_openstack_host_port_by_port_id(char* port_id);
//openstack_port_p find_openstack_host_port_by_mac(UINT1* mac);
//openstack_port_p remove_openstack_host_port_by_mac(UINT1* mac);
//openstack_port_p find_openstack_host_port_by_ip_tenant(UINT4 ip,char* tenant_id);
//openstack_port_p find_openstack_host_port_by_ip_network(UINT4 ip,char* network_id);
//openstack_port_p find_openstack_host_port_by_ip_subnet(UINT4 ip,char* subnet_id);

//UINT4 find_openstack_host_ports_by_subnet_id(char* subnet_id,openstack_port_p* host_list);

//void delete_openstack_host_port_by_tenant_id(char* tenant_id);
//void delete_openstack_host_port_by_network_id(char* network_id);
//void delete_openstack_host_port_by_subnet_id(char* subnet_id);
//void delete_openstack_host_port_by_port_id(char* port_id);
////////////////////////////////////////////////////////////////////////

openstack_node_p create_openstack_host_node(UINT1* data);
void destory_openstack_host_node(openstack_node_p node);

openstack_security_p update_openstack_security_group(char* security_group);
openstack_node_p add_openstack_host_security_node(UINT1* data, openstack_node_p head_p);
void clear_openstack_host_security_node(UINT1* head_p);
void clear_all_security_group_info();
openstack_security_rule_p update_security_rule(char* security_group, char* rule_id, char* direction, char* ethertype, char* port_range_max,
char* port_range_min, char* protocol, char* remote_group_id, char* remote_ip_prefix, char* tenant_id);



p_fabric_host_node find_fabric_host_port_by_port_id(char* port_id);
p_fabric_host_node find_fabric_host_port_by_tenant_id(UINT4 ip, char* tenant_id);
p_fabric_host_node find_fabric_host_port_by_network_id(UINT4 ip, char* network_id);
UINT4 find_fabric_host_ports_by_subnet_id(char* subnet_id,p_fabric_host_node* host_list);
p_fabric_host_node find_fabric_host_port_by_subnet_id(UINT4 ip, char* subnet_id);
p_fabric_host_node find_openstack_host_by_srcport_ip(p_fabric_host_node host_p, UINT4 ip);
void find_fabric_network_by_floating_ip(UINT4 floating_ip,char* network_id);
void update_openstack_host_port_by_mac(UINT1* mac, gn_switch_t* sw, UINT4 port);


void init_host_check_mgr();
void host_check_tx_timer(void *para, void *tid);

#endif /* INC_OPENSTACK_OPENSTACK_HOST_H_ */




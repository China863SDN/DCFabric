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
 * openstack_app.c
 *
 *  Created on: Jun 18, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */
#include "openstack_app.h"
#include "fabric_openstack_arp.h"
#include "fabric_floating_ip.h"

void update_openstack_external_by_host(p_fabric_host_node host);

openstack_network_p create_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external)
{
	openstack_network_p network = NULL;
	//printf("Tenant id: %s | Network_id: %s | Shared: %d \n",tenant_id,network_id,shared);
	network = create_openstack_host_network(tenant_id,network_id,shared,external);
	add_openstack_host_network(network);
	return network;
};

INT4 compare_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external,
		openstack_network_p network_p)
{
	if ((network_p) 
		&& compare_str(tenant_id, network_p->tenant_id)
		&& compare_str(network_id, network_p->network_id)
		&& (external == network_p->external)
		&& (shared == network_p->shared)) {
		return 1;
	}

	return 0;
}

openstack_network_p update_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external)
{
	openstack_network_p network = NULL;
	network = find_openstack_host_network_by_network_id(network_id);
	
	if(network == NULL) {
		network = create_openstack_app_network(tenant_id,network_id,shared,external);
//		add_openstack_host_network(network);
		if (network)
			network->check_status = (UINT1)CHECK_CREATE;
	}
	else if (compare_openstack_app_network(tenant_id, network_id, shared, external, network)) {
		network->check_status = (UINT1)CHECK_LATEST;
	}
	else {
		strcpy(network->tenant_id,tenant_id);
//		strcpy(network->network_id,network_id);
		network->external = external;
		network->shared = shared;
		network->check_status = (UINT1)CHECK_UPDATE;
	}

	return network;
};

openstack_subnet_p create_openstack_app_subnet(
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
		UINT1 external)
{
	openstack_subnet_p subnet = NULL;
	openstack_network_p network = NULL;
	subnet = create_openstack_host_subnet(tenant_id,network_id,subnet_id,gateway_ip,start_ip,end_ip, gateway_ipv6, start_ipv6, end_ipv6, cidr, external);
	add_openstack_host_subnet(subnet);
	// find network
	network = find_openstack_host_network_by_network_id(network_id);
	if(network == NULL){
		network = create_openstack_app_network(tenant_id,network_id,0,0);
	}
	network->subnet_num++;

	return subnet;
};

INT4 compare_openstack_app_subnet(
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
		UINT1 external,
		openstack_subnet_p subnet_p)
{
	 // printf("1.%s,%s,%s,%d,%d,%d,%s,%s,%s,%s,%d\n",tenant_id, network_id, subnet_id, gateway_ip, start_ip, end_ip,
	 //			gateway_ipv6, start_ipv6, end_ipv6, cidr,external);
	 // printf("2.%s,%s,%s,%d,%d,%d,%s,%s,%s,%s,%d\n",subnet_p->tenant_id, subnet_p->network_id, subnet_p->subnet_id, 
	 //	subnet_p->gateway_ip, subnet_p->start_ip, subnet_p->end_ip,
	 //		subnet_p->gateway_ipv6, subnet_p->start_ipv6, subnet_p->end_ipv6, subnet_p->cidr,subnet_p->external);	

	if ((subnet_p)
		&& compare_str(tenant_id, subnet_p->tenant_id)
		&& compare_str(network_id, subnet_p->network_id)
		&& compare_str(subnet_id, subnet_p->subnet_id)
		&& (gateway_ip == subnet_p->gateway_ip)
		&& (start_ip== subnet_p->start_ip)
		&& (end_ip == subnet_p->end_ip)
		&& compare_array(gateway_ipv6, subnet_p->gateway_ipv6, 16)
		&& compare_array(start_ipv6, subnet_p->start_ipv6, 16)
		&& compare_array(end_ipv6, subnet_p->end_ipv6, 16)
		&& (external == subnet_p->external)
		&& compare_str(cidr, subnet_p->cidr)) {
		return 1;
	}
	
	return 0;
		
}

openstack_subnet_p update_openstack_app_subnet(
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		UINT4 gateway_ip,
		UINT4 start_ip,
		UINT4 end_ip,
		UINT1* gateway_ipv6,
		UINT1* start_ipv6,
		UINT1* end_ipv6,
		char* cidr)
{
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);

	UINT1 external = 0;
	openstack_network_p network_p = find_openstack_host_network_by_network_id(network_id);
	if (network_p && (network_p->external)) {
		external = network_p->external;
	}
	if(subnet == NULL) {
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,gateway_ip,start_ip, end_ip,
						gateway_ipv6, start_ipv6, end_ipv6,cidr,external);
//		add_openstack_host_subnet(subnet);
		if (subnet) 
			subnet->check_status = (UINT2)CHECK_CREATE;
	}
	else if (compare_openstack_app_subnet(tenant_id, network_id, subnet_id, gateway_ip, start_ip, end_ip,
				gateway_ipv6, start_ipv6, end_ipv6, cidr, external, subnet)) {
		subnet->check_status = (UINT2)CHECK_LATEST;
	}
	else{
		strcpy(subnet->tenant_id,tenant_id);
		strcpy(subnet->network_id,network_id);
//		strcpy(subnet->subnet_id, subnet_id);
		subnet->gateway_ip = gateway_ip;
		subnet->start_ip = start_ip;
		subnet->end_ip = end_ip;
		strcpy(subnet->cidr, cidr);
		subnet->external = external;
		subnet->check_status = (UINT2)CHECK_UPDATE;
	}

	if (subnet && is_check_status_changed(subnet->check_status)) {
		create_proactive_floating_internal_subnet_flow_by_subnet(subnet);
	}
	
	return subnet;
};
p_fabric_host_node create_openstack_app_port(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id,
		UINT2 security_num,
		UINT1* security_group){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id, security_num, security_group);
	//add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,ipv6,NULL,NULL,"",0);
	}
	subnet->port_num++;
	return port;
};

openstack_port_p create_openstack_app_gateway(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
	// add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,ipv6,NULL,NULL,"",0);
	}
	subnet->gateway_port = port;
	subnet->port_num++;
	return port;
};

openstack_port_p create_openstack_app_dhcp(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
	// add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,0,0,0,NULL,NULL,NULL,"",0);
	}
	subnet->dhcp_port = port;
	subnet->port_num++;
	return port;
};

INT4 compare_openstack_app_host_by_rest(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id,
		UINT2 security_num,
		UINT1* security_group,
		p_fabric_host_node host_p)
{
	if ((host_p)
		// && compare_pointer(sw, host_p->sw)
		&& (port_type == host_p->type)
		// && (port_no == host_p->port)
		&& (ip == host_p->ip_list[0])
		&& compare_array(ipv6, host_p->ipv6, 16)
		&& compare_array(mac, host_p->mac, 6)) {
		openstack_port_p port_p = (openstack_port_p)host_p->data;
		if ((port_p)
			&& compare_str(tenant_id, port_p->tenant_id)
			&& compare_str(network_id, port_p->network_id)
			&& compare_str(subnet_id, port_p->subnet_id)
			&& compare_str(port_id, port_p->port_id)) {
			/* need security?? */
			return 1;
		}
	}
	

	return 0;
}

p_fabric_host_node update_openstack_app_host_by_rest(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id,
		UINT2 security_num,
		UINT1* security_group) 
{
	p_fabric_host_node port = NULL;
//	LOG_PROC("INFO","PORT UPDATE!");
	port = get_fabric_host_from_list_by_mac(mac);
	if(port == NULL){
		port = create_openstack_app_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,security_num,security_group);
//		add_openstack_host_port(port);
		if (port) 
			port->check_status = (UINT2)CHECK_CREATE;
	}
	else if (compare_openstack_app_host_by_rest(sw,port_type,port_no,ip,ipv6,mac,tenant_id,
				network_id,subnet_id,port_id,security_num,security_group,port)) {
        openstack_port_p port_p = (openstack_port_p)port->data;
        port_p->security_num = security_num;
		clear_openstack_host_security_node(port_p->security_data);
		port_p->security_data = security_group;		
		port->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		//printf("update app port\n");
		openstack_port_p port_p = (openstack_port_p)port->data;
        port->type = port_type;
		port->ip_list[0] = ip;
		if (ipv6)
			memcpy(port->ipv6[0], ipv6, 16);
		strcpy(port_p->tenant_id,tenant_id);
		strcpy(port_p->network_id,network_id);
		strcpy(port_p->port_id,port_id);
		strcpy(port_p->subnet_id, subnet_id);
		port_p->security_num = security_num;
		clear_openstack_host_security_node(port_p->security_data);
		port_p->security_data = security_group;
		port->check_status = (UINT2)CHECK_UPDATE;
	}

	if (is_check_status_changed(port->check_status)) {
		openstack_ip_remove_deny_flow(mac);
		update_openstack_external_by_host(port);
	}
	return port;
};

p_fabric_host_node update_openstack_app_gateway_by_rest(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
//	LOG_PROC("INFO","Gateway UPDATE!");
	port = update_openstack_app_host_by_rest(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,ipv6,NULL,NULL,"",0);
	}
    
    if (OPENSTACK_PORT_TYPE_ROUTER_INTERFACE == port_type)
	{
	    subnet->gateway_port = port;
    }
	return port;
};

p_fabric_host_node update_openstack_app_dhcp_by_rest(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id)
{
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
//	LOG_PROC("INFO","DHCP UPDATE!");
	port = update_openstack_app_host_by_rest(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL) {
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,ipv6,NULL,NULL,"",0);
	}
	subnet->dhcp_port = port;
	return port;
};
p_fabric_host_node update_openstack_app_host_by_sdn(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac){
	p_fabric_host_node port = NULL;
	port = get_fabric_host_from_list_by_mac(mac);
	if(port == NULL){
		port = create_openstack_app_port(sw,port_type,port_no,ip,ipv6,mac,"","","","",0,NULL);
//		add_openstack_host_port(port);
	}else{
		port->ip_list[0] = ip;
		memcpy(port->ipv6[0], ipv6, 16);
		port->sw = sw;
		port->port = port_no;
	}
	return port;
};

openstack_network_p find_openstack_app_network_by_network_id(char* network_id){
	return find_openstack_host_network_by_network_id(network_id);
};

openstack_subnet_p find_openstack_app_subnet_by_subnet_id(char* subnet_id){
	return find_openstack_host_subnet_by_subnet_id(subnet_id);
};

//openstack_port_p find_openstack_app_host_by_mac(UINT1* mac){
//	return find_openstack_host_port_by_mac(mac);
//};

//openstack_port_p find_openstack_app_host_by_ip_tenant(UINT4 ip,char* tenant_id){
//	return find_openstack_host_port_by_ip_tenant(ip,tenant_id);
//};

//openstack_port_p find_openstack_app_host_by_ip_network(UINT4 ip,char* network_id){
//	return find_openstack_host_port_by_ip_network(ip,network_id);
//};
//
//openstack_port_p find_openstack_app_host_by_ip_subnet(UINT4 ip,char* subnet_id){
//	return find_openstack_host_port_by_ip_subnet(ip,subnet_id);
//};

p_fabric_host_node find_openstack_app_gateway_by_host(p_fabric_host_node host){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	openstack_port_p port_p = (openstack_port_p)host->data;
	subnet = find_openstack_host_subnet_by_subnet_id(port_p->subnet_id);
	port = subnet->gateway_port;
	return port;
};

p_fabric_host_node find_openstack_app_gateway_by_subnet_id(char* subnet_id){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	port = subnet->gateway_port;
	return port;
};
p_fabric_host_node find_openstack_app_dhcp_by_host(openstack_port_p host){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(host->subnet_id);
	port = subnet->dhcp_port;
	return port;
};
p_fabric_host_node find_openstack_app_dhcp_by_subnet_id(char* subnet_id){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	port = subnet->dhcp_port;
	return port;
};

UINT4 find_openstack_app_hosts_by_subnet_id(char* subnet_id, p_fabric_host_node* host_list){
	return find_fabric_host_ports_by_subnet_id(subnet_id,host_list);
};


UINT4 check_openstack_app_host_is_gateway_by_mac(UINT1* mac){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	port = get_fabric_host_from_list_by_mac(mac);
	if(port != NULL){
		subnet = find_openstack_host_subnet_by_geteway_ip(port->ip_list[0]);
		if(subnet != NULL){
			return subnet->gateway_ip;
		}
	}
	return 0;
};

void update_openstack_external_by_host(p_fabric_host_node host)
{
	if ((host) && (OPENSTACK_PORT_TYPE_GATEWAY == host->type)) {
		openstack_port_p port_p = (openstack_port_p)host->data;
		if (port_p)
			update_openstack_external_by_outer_interface(host->ip_list[0], host->mac, port_p->network_id);
	}
}

INT4 is_check_status_changed(UINT1 check_status)
{
	if (((UINT2)CHECK_CREATE == check_status) || ((UINT2)CHECK_UPDATE == check_status)) {
		return 1;
	}

	return 0;
}


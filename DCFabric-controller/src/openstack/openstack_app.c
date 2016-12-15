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
#include "openstack_host.h"
#include "openstack_app.h"

openstack_network_p create_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared){
	openstack_network_p network = NULL;
	//printf("Tenant id: %s | Network_id: %s | Shared: %d \n",tenant_id,network_id,shared);
	network = create_openstack_host_network(tenant_id,network_id,shared);
	add_openstack_host_network(network);
	return network;
};

openstack_network_p update_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared){
	openstack_network_p network = NULL;
	network = find_openstack_host_network_by_network_id(network_id);
	if(network == NULL){
		network = create_openstack_app_network(tenant_id,network_id,shared);
//		add_openstack_host_network(network);
	}else{
		strcpy(network->tenant_id,tenant_id);
//		strcpy(network->network_id,network_id);
		network->shared = shared;
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
		char* cidr){
	openstack_subnet_p subnet = NULL;
	openstack_network_p network = NULL;
	subnet = create_openstack_host_subnet(tenant_id,network_id,subnet_id,gateway_ip,start_ip,end_ip);
	add_openstack_host_subnet(subnet);
	// find network
	network = find_openstack_host_network_by_network_id(network_id);
	if(network == NULL){
		network = create_openstack_app_network(tenant_id,network_id,0);
	}
	network->subnet_num++;

	return subnet;
};
openstack_subnet_p update_openstack_app_subnet(
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		UINT4 gateway_ip,
		UINT4 start_ip,
		UINT4 end_ip,
		char* cidr){
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,gateway_ip,start_ip,end_ip,"");
//		add_openstack_host_subnet(subnet);
	}else{
		strcpy(subnet->tenant_id,tenant_id);
		strcpy(subnet->network_id,network_id);
//		strcpy(subnet->subnet_id, subnet_id);
		subnet->gateway_ip = gateway_ip;
		subnet->start_ip = start_ip;
		subnet->end_ip = end_ip;
	}
	return subnet;
};
openstack_port_p create_openstack_app_port(
		gn_switch_t* sw,
		UINT4 port_no,
		UINT4 ip,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_no,ip,mac,tenant_id,network_id,subnet_id,port_id);
	add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,"");
	}
	subnet->port_num++;
	return port;
};

openstack_port_p create_openstack_app_gateway(
		gn_switch_t* sw,
		UINT4 port_no,
		UINT4 ip,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_no,ip,mac,tenant_id,network_id,subnet_id,port_id);
	add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,"");
	}
	subnet->gateway_port = port;
	subnet->port_num++;
	return port;
};

openstack_port_p create_openstack_app_dhcp(
		gn_switch_t* sw,
		UINT4 port_no,
		UINT4 ip,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_no,ip,mac,tenant_id,network_id,subnet_id,port_id);
	add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,0,0,0,"");
	}
	subnet->dhcp_port = port;
	subnet->port_num++;
	return port;
};

openstack_port_p update_openstack_app_host_by_rest(
		gn_switch_t* sw,
		UINT4 port_no,
		UINT4 ip,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
//	LOG_PROC("INFO","PORT UPDATE!");
	port = find_openstack_host_port_by_mac(mac);
	if(port == NULL){
		port = create_openstack_app_port(sw,port_no,ip,mac,tenant_id,network_id,subnet_id,port_id);
//		add_openstack_host_port(port);
	}else{
		port->ip = ip;
		strcpy(port->tenant_id,tenant_id);
		strcpy(port->network_id,network_id);
		strcpy(port->port_id,port_id);
		strcpy(port->subnet_id, subnet_id);
	}
	return port;
};

openstack_port_p update_openstack_app_gateway_by_rest(
		gn_switch_t* sw,
		UINT4 port_no,
		UINT4 ip,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
//	LOG_PROC("INFO","Gateway UPDATE!");
	port = update_openstack_app_host_by_rest(sw,port_no,ip,mac,tenant_id,network_id,subnet_id,port_id);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,"");
	}
	subnet->gateway_port = port;
	return port;
};

openstack_port_p update_openstack_app_dhcp_by_rest(
		gn_switch_t* sw,
		UINT4 port_no,
		UINT4 ip,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
//	LOG_PROC("INFO","DHCP UPDATE!");
	port = update_openstack_app_host_by_rest(sw,port_no,ip,mac,tenant_id,network_id,subnet_id,port_id);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,"");
	}
	subnet->dhcp_port = port;
	return port;
};
openstack_port_p update_openstack_app_host_by_sdn(
		gn_switch_t* sw,
		UINT4 port_no,
		UINT4 ip,
		UINT1* mac){
	openstack_port_p port = NULL;
	port = find_openstack_host_port_by_mac(mac);
	if(port == NULL){
		port = create_openstack_app_port(sw,port_no,ip,mac,"","","","");
//		add_openstack_host_port(port);
	}else{
		port->ip = ip;
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

openstack_port_p find_openstack_app_host_by_mac(UINT1* mac){
	return find_openstack_host_port_by_mac(mac);
};

openstack_port_p find_openstack_app_host_by_ip_tenant(UINT4 ip,char* tenant_id){
	return find_openstack_host_port_by_ip_tenant(ip,tenant_id);
};

openstack_port_p find_openstack_app_host_by_ip_network(UINT4 ip,char* network_id){
	return find_openstack_host_port_by_ip_network(ip,network_id);
};

openstack_port_p find_openstack_app_host_by_ip_subnet(UINT4 ip,char* subnet_id){
	return find_openstack_host_port_by_ip_subnet(ip,subnet_id);
};

openstack_port_p find_openstack_app_gateway_by_host(openstack_port_p host){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(host->subnet_id);
	port = subnet->gateway_port;
	return port;
};

openstack_port_p find_openstack_app_gateway_by_subnet_id(char* subnet_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	port = subnet->gateway_port;
	return port;
};
openstack_port_p find_openstack_app_dhcp_by_host(openstack_port_p host){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(host->subnet_id);
	port = subnet->dhcp_port;
	return port;
};
openstack_port_p find_openstack_app_dhcp_by_subnet_id(char* subnet_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	port = subnet->dhcp_port;
	return port;
};

UINT4 find_openstack_app_hosts_by_subnet_id(char* subnet_id,openstack_port_p* host_list){
	return find_openstack_host_ports_by_subnet_id(subnet_id,host_list);
};


UINT4 check_openstack_app_host_is_gateway_by_mac(UINT1* mac){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	port = find_openstack_host_port_by_mac(mac);
	if(port != NULL){
		subnet = find_openstack_host_subnet_by_geteway_ip(port->ip);
		if(subnet != NULL){
			return subnet->gateway_ip;
		}
	}
	return 0;
};


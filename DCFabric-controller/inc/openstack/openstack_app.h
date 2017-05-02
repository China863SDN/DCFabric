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
 * openstack_app.h
 *
 *  Created on: Jun 18, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */

#ifndef INC_OPENSTACK_OPENSTACK_APP_H_
#define INC_OPENSTACK_OPENSTACK_APP_H_
#include "openstack_host.h"
#include "fabric_host.h"

/*******************************************************
 * create update delete
 ********************************************************/
/*
 * create network
 */
openstack_network_p create_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external);

/*
 * create & update network
 */
openstack_network_p update_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external);

/*
 * create subnet
 */
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
		UINT1 external);

/*
 * create & update subnet
 */
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
		char* cidr);

/*
 * create normal port
 */
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
		UINT1* security_group);
/*
 * create gateway port
 */
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
		char* port_id);
/*
 * create dhcp port
 */
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
		char* port_id);
/*
 * create & update port
 */
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
		UINT1* security_group);


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
		char* port_id);
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
		char* port_id);

/*
 * create & update port by sdn
 */
p_fabric_host_node update_openstack_app_host_by_sdn(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac);
/*******************************************************
 * find
 ********************************************************/
/*
 * find network
 */
openstack_network_p find_openstack_app_network_by_network_id(char* network_id);
/*
 * find subnet
 */
openstack_subnet_p find_openstack_app_subnet_by_subnet_id(char* subnet_id);
/*
 * find host
 */
//openstack_port_p find_openstack_app_host_by_mac(UINT1* mac);
//openstack_port_p find_openstack_app_host_by_ip_tenant(UINT4 ip,char* tenant_id);
//openstack_port_p find_openstack_app_host_by_ip_network(UINT4 ip,char* network_id);
//openstack_port_p find_openstack_app_host_by_ip_subnet(UINT4 ip,char* subnet_id);
p_fabric_host_node find_openstack_app_gateway_by_host(p_fabric_host_node host);
p_fabric_host_node find_openstack_app_gateway_by_subnet_id(char* subnet_id);
p_fabric_host_node find_openstack_app_dhcp_by_host(openstack_port_p host);
p_fabric_host_node find_openstack_app_dhcp_by_subnet_id(char* subnet_id);
UINT4 find_openstack_app_hosts_by_subnet_id(char* subnet_id,p_fabric_host_node* host_list);

UINT4 check_openstack_app_host_is_gateway_by_mac(UINT1* mac);

enum OPENSTACK_CHECK_STATUS {
	CHECK_UNKNOWN = 0,
	CHECK_CREATE = 1, 
	CHECK_UPDATE = 2,
	CHECK_LATEST = 3,
	CHECK_UNCHECKED = 4,
};

INT4 is_check_status_changed(UINT1 check_status);

#endif /* INC_OPENSTACK_OPENSTACK_APP_H_ */

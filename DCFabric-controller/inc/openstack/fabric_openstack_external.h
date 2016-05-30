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
 * fabric_openstack_external.h
 *
 *  Created on: sep 9, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: sep 9, 2015
 */
#ifndef INC_FABRIC_FABRIC_OPENSTACK_EXTERNAL_H_
#define INC_FABRIC_FABRIC_OPENSTACK_EXTERNAL_H_
#include "gnflush-types.h"

//external
#define OPENSTACK_EXTERNAL_ID_LEN 48
#define OPENSTACK_EXTERNAL_MAX_NUM 64
#define OPENSTACK_FLOATING_MAX_NUM 128
#define OPENSTACK_NAT_ICMP_MAX_NUM 128
#define OPENSTACK_EXTERNAL_NODE_MAX_NUM (OPENSTACK_EXTERNAL_MAX_NUM+OPENSTACK_FLOATING_MAX_NUM+OPENSTACK_NAT_ICMP_MAX_NUM)

/*外部网关端口结构体*/
typedef struct external_port
{
	char network_id[48];
	UINT4 external_gateway_ip;//public net gatway
	UINT4 external_outer_interface_ip;//外部接口
	UINT1 external_gateway_mac[6];
	UINT1 external_outer_interface_mac[6];
	UINT8 external_dpid; //openflow route dpid
	UINT4 external_port; //openflow route port
	INT4  status;
	UINT4 ID;	// 标识符
}external_port_t,*external_port_p;

//floating ip struct
typedef struct _external_floating_ip
{
	UINT4 fixed_ip;//inner ip
	UINT4 floating_ip;//outer ip
	char port_id[48];
	char router_id[48];
	UINT1 check_status;
	UINT1 flow_installed;
}external_floating_ip, *external_floating_ip_p;

//identifier host ip struct
typedef struct _nat_icmp_iden
{
	UINT2 identifier;//icmp packet identifier
	UINT4 host_ip;//icmp packet host ip
	UINT1 host_mac[6];//icmp packet host mac
	UINT8 sw_dpid;
	UINT4 inport;
}nat_icmp_iden, * nat_icmp_iden_p;

typedef struct _openstack_external_node{
	UINT1* data;
	struct _openstack_external_node* next;
}openstack_external_node,*openstack_external_node_p;

extern openstack_external_node_p g_openstack_external_list;
extern openstack_external_node_p g_nat_icmp_iden_list;


void init_openstack_external();
/*
  * all the get_external_port_by_xx function is provided as public interface,  and the status of result is Active.
  * all the find_external_port_by_xx function is provided as internal interface,  and the status of result is Active or inactive.
 */

//get external_port_p from mem by outer interface ip(such as 192.168.52.100)
external_port_p get_external_port_by_out_interface_ip(UINT4 external_outer_interface_ip);

//TODO
//get external_port_p from mem by floating ip
external_port_p get_external_port_by_floatip(UINT4 external_floatip);

//get external_port_p from mem by host mac address
external_port_p get_external_port_by_host_mac(UINT1* host_mac);

//get external_port_p from mem by load balancing
external_port_p get_external_port();

//get external_port_p from mem by floating ip(such as 192.168.52.201)
external_floating_ip_p get_external_floating_ip_by_floating_ip(UINT4 floating_ip);

//get external_floating_ip_p ip from mem by fixed ip(such as 10.0.0.20)
external_floating_ip_p get_external_floating_ip_by_fixed_ip(UINT4 fixed_ip);

//get external_floating_ip_p ip from mem by floating ip(such as 192.168.52.202)
external_floating_ip_p get_external_floating_ip_by_floating_ip(UINT4 floating_ip);

nat_icmp_iden_p get_nat_icmp_iden_by_host_ip(UINT4 host_ip);

//TODO use get_nat_icmp_iden_by_host_ip
nat_icmp_iden_p get_nat_icmp_iden_by_host_mac(UINT1* host_mac);

nat_icmp_iden_p get_nat_icmp_iden_by_identifier(UINT2 identifier);

//add external_port_p into mem from rest api
void create_external_port_by_rest(
		UINT4 external_gateway_ip,
		UINT1* external_gateway_mac,
        UINT4 external_outer_interface_ip,
        UINT1* external_outer_interface_mac,
        UINT8 external_dpid,
        UINT4 external_port,
		char* network_id);

external_port_p create_external_port(
        UINT4 external_gateway_ip,
		UINT1* external_gateway_mac,
        UINT4 external_outer_interface_ip,
		UINT1* external_outer_interface_mac,
		UINT8 external_dpid,
		UINT4 external_port,
		char* network_id);

void update_external_config(external_port_p epp);

nat_icmp_iden_p update_nat_icmp_iden(
		UINT2 identifier,
		UINT4 host_ip,
		UINT1* host_mac,
		UINT8 sw_dpid,
		UINT4 inport);

void remove_external_port_by_networkid(INT1* network_id);

// update external flows
void init_external_flows();

//add external_floating_ip_p into mem from rest api
external_floating_ip_p create_floatting_ip_by_rest(
		UINT4 fixed_ip,
        UINT4 floating_ip,
		char* port_id,
        char* router_id);

//add nat_imcp_iden_p into mem
nat_icmp_iden_p create_nat_imcp_iden_p(
		UINT2 identifier,
		UINT4 host_ip,
		UINT1* host_mac,
		UINT8 sw_dpid,
		UINT4 inport);

void update_floating_ip_mem_info();

void read_external_port_config();

openstack_external_node_p get_floating_list();

external_floating_ip_p find_external_floating_ip_by_floating_ip(UINT4 floating_ip);
external_port_p find_openstack_external_by_floating_ip(UINT4 external_floating_ip);

void reload_floating_ip();
void update_openstack_external_gateway_mac(UINT4 gateway_ip, UINT1* gateway_mac, UINT4 outer_ip, UINT1* outer_mac);
void update_openstack_external_by_outer_interface(UINT4 host_ip, UINT1* host_mac, char* network_id);
void start_external_mac_check(UINT4 check_on, UINT4 check_internal);
void stop_external_mac_check();
void init_external_mac_check_mgr();

#endif

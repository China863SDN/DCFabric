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
 * fabric_flows.h
 *
 *  Created on: Mar 27, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */

#ifndef INC_FABRIC_FABRIC_FLOWS_H_
#define INC_FABRIC_FABRIC_FLOWS_H_
#include "gnflush-types.h"
#include "forward-mgr.h"
#include "fabric_openstack_external.h"

#define FABRIC_IMPL_HARD_TIME_OUT 0
#define FABRIC_IMPL_IDLE_TIME_OUT 0
#define FABRIC_ARP_HARD_TIME_OUT 0
#define FABRIC_ARP_IDLE_TIME_OUT 100
#define FABRIC_FIND_HOST_IDLE_TIME_OUT 100

#define FABRIC_PRIORITY_HOST_INPUT_FLOW 5
#define FABRIC_PRIORITY_SWITCH_INPUT_FLOW 10
#define FABRIC_PRIORITY_ARP_MISSMATCH_INPUT_FLOW 10
#define FABRIC_PRIORITY_MISSMATCH_PUSHTAG_FLOW 0
#define FABRIC_PRIORITY_SWAPTAG_FLOW 20
#define FABRIC_PRIORITY_ARP_FLOW 15
#define FABRIC_PRIORITY_FLOATING_FLOW 16
#define FABRIC_PRIORITY_NAT_FLOW 17
#define FABRIC_PRIORITY_LOADBALANCE_FLOW 18

#define FABRIC_INPUT_TABLE 0
#define FABRIC_PUSHTAG_TABLE 1
#define FABRIC_SWAPTAG_TABLE 2
#define FABRIC_OUTPUT_TABLE 3

#define FABRIC_SWITCH_INPUT_MAX_FLOW_NUM 48

typedef struct action_param
{
	INT4 type;
	void* param;
	struct action_param *next;
}action_param_t, *action_param_p;

typedef struct flow_param
{
	gn_oxm_t* match_param;
	action_param_t* instruction_param;
<<<<<<< HEAD
	action_param_t* action_param;			/* OFPIT_APPLY_ACTIONS use*/
	action_param_t* write_action_param;		/* OFPIT_WRITE_ACTIONS use*/
}flow_param_t;


typedef struct flow_entry_json
{
    INT1 *flow_name;
    gn_switch_t *sw;
    
    UINT1 table_id;
    UINT2 idle_timeout;
    UINT2 hard_timeout;
    UINT2 priority;

    flow_param_t *flow_param;

    UINT8 *data;
=======
	action_param_t* action_param;
}flow_param_t;


/*=== BEGIN === Added by zgzhao for controller API requirement 2015-12-28*/
typedef struct flow_entry_json
{
    INT1 *flow_name;
    INT1 *actions;
    UINT2 priority;

    //match
    UINT4 in_port;
    UINT2 eth_type;
    UINT4 ipv4_src;
>>>>>>> bf54879025c15afe476208ca575ee15b66675acb

    struct flow_entry_json *pre;
    struct flow_entry_json *next;
} flow_entry_json_t;
<<<<<<< HEAD
=======
/*=== END === Added by zgzhao for controller API requirement 2015-12-28*/
>>>>>>> bf54879025c15afe476208ca575ee15b66675acb

////////////////////////////////////////////////////////////////////////
/*
 * interfaces
 */
////////////////////////////////////////////////////////////////////////
void install_fabric_base_flows(gn_switch_t * sw);
void install_fabric_last_flow(gn_switch_t * sw,UINT4 tag);
void install_fabric_first_flow(gn_switch_t * sw,UINT4 port,UINT4 tag);
void install_fabric_middle_flow(gn_switch_t * sw,UINT4 port,UINT4 tag);
void install_fabric_openstack_external_output_flow(gn_switch_t * sw,UINT4 port,UINT1* gateway_mac,UINT4 outer_interface_ip,UINT1 type);

void install_fabric_same_switch_flow(gn_switch_t * sw,UINT1* mac, UINT4 port);
void install_fabric_same_switch_out_subnet_flow(gn_switch_t * sw,UINT1* gateway_mac,UINT1* dst_mac,UINT4 dst_ip,UINT4 port, security_param_p security_param);
void install_fabric_output_flow(gn_switch_t * sw,UINT1* mac, UINT4 port);
void install_fabric_push_tag_flow(gn_switch_t * sw,UINT1* mac,UINT4 tag);
void install_fabric_push_tag_out_subnet_flow(gn_switch_t * sw,UINT1* gateway_mac,UINT1* dst_mac,UINT4 dst_ip,UINT4 tag, security_param_p security_param);
void fabric_openstack_floating_ip_install_set_vlan_out_flow(gn_switch_t * sw, UINT4 match_ip, UINT1* match_mac, UINT4 mod_src_ip, UINT1* mod_dst_mac, UINT4 vlan_id, security_param_t* src_security);
void fabric_openstack_floating_ip_install_set_vlan_in_flow(gn_switch_t * sw, UINT4 match_ip, UINT4 mod_dst_ip, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port);
<<<<<<< HEAD
=======
/*=== BEGIN === Added by zgzhao for controller API requirement 2015-12-28*/
BOOL install_fabric_json_flow(const gn_switch_t * sw, const flow_entry_json_t *entry);
void delete_fabric_json_flow(const gn_switch_t * sw, const flow_entry_json_t *entry);
/*=== END === Added by zgzhao for controller API requirement 2015-12-28*/
>>>>>>> bf54879025c15afe476208ca575ee15b66675acb

/*
 * 下发流表规则
 */
void install_fabric_nat_from_inside_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw, security_param_t* security_param);

void install_fabric_nat_from_external_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw);

void install_fabric_nat_from_external_fabric_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw, UINT4 out_port);

void install_fabric_nat_from_external_fabric_host_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw);

void install_fabric_same_switch_security_flow(gn_switch_t * sw,UINT1* mac, UINT4 port, security_param_p security_param);
void install_fabric_push_tag_security_flow(gn_switch_t * sw,UINT4 dst_ip,UINT1* mac, UINT4 tag, security_param_p security_param);

void init_fabric_flows();
void delete_fabric_flow(gn_switch_t *sw);
void delete_fabric_flow_by_sw(gn_switch_t *sw);
void delete_fabric_impl_flow(gn_switch_t *sw,UINT4 port_no,UINT4 tag,UINT1 table_id);

void delete_fabric_input_flow_by_ip(gn_switch_t* sw, UINT4 ip);
void delete_fabric_input_flow_by_mac_portno(gn_switch_t* sw, UINT1* dst_mac, UINT2 dst_port, UINT2 proto);
<<<<<<< HEAD
void delete_fabric_flow_by_ip(gn_switch_t* sw, UINT4 ip, UINT2 table_id);
void delete_fabric_flow_by_mac(gn_switch_t* sw, UINT1* mac, UINT2 table_id);
=======
void delete_fabrci_flow_by_ip(gn_switch_t* sw, UINT4 ip, UINT2 table_id);
void delete_fabrci_flow_by_mac(gn_switch_t* sw, UINT1* mac, UINT2 table_id);
>>>>>>> bf54879025c15afe476208ca575ee15b66675acb

UINT1 get_fabric_last_flow_table();
UINT1 get_fabric_first_flow_table();
UINT1 get_fabric_middle_flow_table();

flow_param_t* init_flow_param();
void add_action_param(action_param_t** action_param, INT4 type, void* param);
void clear_action_param(action_param_t* action_param);
void clear_flow_param(flow_param_t* flow_param);
void install_fabric_flows(gn_switch_t * sw, UINT2 idle_timeout, UINT2 hard_timeout, UINT2 priority, UINT1 table_id, UINT1 command, flow_param_t* flow_param);

void install_delete_fabric_flow(gn_switch_t* sw);

/*
 * temp added for ipv6
 * by lxf@2016.1.11
 */
#if 1
void install_fabric_push_tag_security_flow_ipv6(gn_switch_t * sw,UINT1* ipv6, UINT1* mac, UINT4 tag, security_param_p security_param);
#endif

void fabric_openstack_install_fabric_vip_flows(p_fabric_host_node src_port,p_fabric_host_node dst_port, UINT1 proto, UINT4 vip, UINT1* vip_mac,
		p_fabric_host_node src_gateway, p_fabric_host_node dst_gateway, UINT4 src_tcp_port_no, UINT4 vip_tcp_port_no, security_param_p src_security, security_param_p dst_security);

void fabric_openstack_install_fabric_vip_out_flows(gn_switch_t* ext_sw, UINT4 src_ip, UINT1* src_mac, UINT4 floating_ip, UINT1* floating_mac,
												p_fabric_host_node dst_port, UINT1 proto, UINT4 vip, UINT1* vip_mac,
												UINT1* src_gatemac, UINT1* dst_gatemac, UINT4 src_tcp_port_no, UINT4 vip_tcp_port_no, UINT1* ext_gw_mac,
												security_param_p src_security, security_param_p dst_security);

void install_fabric_ip_proto_flow(gn_switch_t * sw, UINT1 proto, UINT2 table_id, UINT2 priority,
									UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT1* dst_mac, UINT2 sport, UINT2 dport,
									UINT4 mod_src_ip, UINT4 mod_dst_ip, UINT1* mod_src_mac, UINT1* mod_dst_mac, UINT2 mod_sport, UINT2 mod_dport,
									UINT2 dst_tag, UINT4 outport, UINT2 goto_id, security_param_p security_p);

void fabric_openstack_install_fabric_floaing_vip_flows(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT1 proto, UINT4 vip, UINT1* vip_mac, UINT4 floatingip, UINT1* floatingmac,
		p_fabric_host_node src_gateway, p_fabric_host_node dst_gateway, UINT4 src_tcp_port_no, UINT4 vip_tcp_port_no, security_param_p src_security, security_param_p dst_security);

#endif /* INC_FABRIC_FABRIC_FLOWS_H_ */

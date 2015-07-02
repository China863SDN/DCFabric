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
 * fabric_flows.c
 *
 *  Created on: Mar 27, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#include "fabric_flows.h"
#include "fabric_path.h"
#include "../flow-mgr/flow-mgr.h"
#include "timer.h"
#include "../conn-svr/conn-svr.h"
#include "gn_inet.h"
#include "mod-types.h"
#include "openflow-10.h"
#include "openflow-13.h"
// flow functions
gn_flow_t * install_add_fabric_host_input_flow(gn_switch_t *sw);
gn_flow_t * install_add_fabric_miss_match_flow(gn_switch_t* sw);
gn_flow_t * install_add_fabric_ARP_miss_match_flow(gn_switch_t* sw);
gn_flow_t * install_add_fabric_switch_input_flow(gn_switch_t *sw,gn_port_t* sw_port);
gn_flow_t * install_add_fabric_impl_last_flow(gn_switch_t * sw,UINT4 tag);
gn_flow_t * install_add_fabric_impl_first_flow(gn_switch_t * sw,UINT4 port,UINT4 tag);
gn_flow_t * install_add_fabric_impl_middle_flow(gn_switch_t * sw,UINT4 port,UINT4 tag);

void install_delete_fabric_flow(gn_switch_t* sw);
void install_delete_fabric_impl_flow(gn_switch_t *sw,UINT4 port_no,UINT4 tag,UINT1 table_id);

// new ids for mem alloc
//extern void *g_gnflow_mempool_id;
//extern void *g_gninstruction_mempool_id;
//extern void *g_gnaction_mempool_id;
// temp fucntion
//UINT1 check_down_flow(gn_switch_t* sw){
//	if(sw->dpid == 128983883520 || sw->dpid == 128983886902){
//		return 0;
//	}
//	return 1;
//};
/**************************************
 * Interface functions
 **************************************/
/*
 * initialize fabric flows
 */
void init_fabric_flows(){
	//p_fabric_flow f_flow = NULL;

	return;
}
/*
 * clear fabric flows
 */
void delete_fabric_flow(gn_switch_t *sw){
	install_delete_fabric_flow(sw);
	return;
};
/*
 * clear fabric flows by sw
 */
void delete_fabric_flow_by_sw(gn_switch_t *sw){
	install_delete_fabric_flow(sw);
	return;
};
void delete_fabric_impl_flow(gn_switch_t *sw,UINT4 port_no,UINT4 tag,UINT1 table_id){
	if(sw->state != 0){
		install_delete_fabric_impl_flow(sw,port_no,tag,table_id);
	}
	return;
};

/*
 * install fabric base flows
 */
void install_fabric_base_flows(gn_switch_t * sw){
	install_add_fabric_host_input_flow(sw);
	install_add_fabric_ARP_miss_match_flow(sw);
	install_add_fabric_miss_match_flow(sw);
//	for(j=0; j<sw->n_ports; j++){
//		// check port state is ok and also not connect other switch(neighbor)
//		if(sw->neighbor[j] != NULL){
//			install_add_fabric_switch_input_flow(sw,&(sw->ports[j]));
//		}
//	}
	return;
};
/*
 * install last tag
 */
void install_fabric_last_flow(gn_switch_t * sw,UINT4 tag){
	install_add_fabric_impl_last_flow(sw,tag);
	return;
};
/*
 *install first tag
 */
void install_fabric_first_flow(gn_switch_t * sw,UINT4 port,UINT4 tag){
	install_add_fabric_impl_first_flow(sw,port,tag);
	return;
};
/*
 * install middle tag
 */
void install_fabric_middle_flow(gn_switch_t * sw,UINT4 port,UINT4 tag){
	install_add_fabric_impl_middle_flow(sw,port,tag);
	return;
};
/*
 * install fabric sam switch flow
 */
void install_fabric_same_switch_flow(gn_switch_t * sw,UINT1* mac, UINT4 port){
	flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction;
    gn_action_output_t action;

    memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
	flow.table_id = FABRIC_INPUT_TABLE;
	flow.match.type = OFPMT_OXM;

	memcpy(flow.match.oxm_fields.eth_dst, mac, 6);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);

	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    memset(&action, 0, sizeof(gn_action_t));
    action.port = port;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return;
};
void install_fabric_same_switch_out_subnet_flow(gn_switch_t * sw,UINT1* gateway_mac,UINT1* dst_mac,UINT4 dst_ip,UINT4 port){
	flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction;
    gn_action_output_t action;
    gn_action_set_field_t act_set_field_mac;
    printf("%s\n", FN);
    memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
	flow.table_id = FABRIC_INPUT_TABLE;
	flow.match.type = OFPMT_OXM;

	flow.match.oxm_fields.ipv4_dst=ntohl(dst_ip);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);

    flow.match.oxm_fields.eth_type = ETHER_IP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

	memcpy(flow.match.oxm_fields.eth_dst, gateway_mac, 6);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);


	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    memset(&action, 0, sizeof(gn_action_t));
    action.port = port;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;

    memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
    act_set_field_mac.type = OFPAT13_SET_FIELD;
    memcpy(act_set_field_mac.oxm_fields.eth_dst, dst_mac, 6);
    act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
    act_set_field_mac.next = instruction.actions;
    instruction.actions = (gn_action_t *)&act_set_field_mac;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return;
};
void install_fabric_output_flow(gn_switch_t * sw,UINT1* mac, UINT4 port){
	flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction;
    gn_action_output_t action;

    memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_FIND_HOST_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
	flow.table_id = FABRIC_OUTPUT_TABLE;
	flow.match.type = OFPMT_OXM;

	memcpy(flow.match.oxm_fields.eth_dst, mac, 6);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);

	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    memset(&action, 0, sizeof(gn_action_t));
    action.port = port;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return;
};
void install_fabric_push_tag_flow(gn_switch_t * sw,UINT1* mac, UINT4 tag){

	flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction_act;
	gn_instruction_goto_table_t instruction_goto;
    gn_action_set_field_t act_set_field;
    gn_action_t act_pushVlan;

    memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
	flow.table_id = FABRIC_PUSHTAG_TABLE;
	flow.match.type = OFPMT_OXM;

	memcpy(flow.match.oxm_fields.eth_dst, mac, 6);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);


	memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
	instruction_goto.type = OFPIT_GOTO_TABLE;
	instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
	instruction_goto.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_goto;

	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
	instruction_act.type = OFPIT_APPLY_ACTIONS;
	instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

    memset(&act_set_field, 0, sizeof(gn_action_set_field_t));
    act_set_field.type = OFPAT13_SET_FIELD;
    act_set_field.oxm_fields.vlan_vid = (UINT2)tag;
    act_set_field.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
    act_set_field.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field;

    memset(&act_pushVlan, 0, sizeof(gn_action_t));
    act_pushVlan.type = OFPAT13_PUSH_VLAN;
    act_pushVlan.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_pushVlan;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return;
};

void install_fabric_push_tag_out_subnet_flow(gn_switch_t * sw,UINT1* gateway_mac,UINT1* dst_mac,UINT4 dst_ip,UINT4 tag){
//	printf("%s\n", FN);
	flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction_act;
	gn_instruction_goto_table_t instruction_goto;
    gn_action_set_field_t act_set_field;
    gn_action_set_field_t act_set_field_mac;
    gn_action_t act_pushVlan;

    memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
	flow.table_id = FABRIC_PUSHTAG_TABLE;
	flow.match.type = OFPMT_OXM;

	memcpy(flow.match.oxm_fields.eth_dst, gateway_mac, 6);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);

    flow.match.oxm_fields.eth_type = ETHER_IP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

	flow.match.oxm_fields.ipv4_dst=ntohl(dst_ip);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);


	memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
	instruction_goto.type = OFPIT_GOTO_TABLE;
	instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
	instruction_goto.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_goto;

	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
	instruction_act.type = OFPIT_APPLY_ACTIONS;
	instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

    memset(&act_set_field, 0, sizeof(gn_action_set_field_t));
    act_set_field.type = OFPAT13_SET_FIELD;
    act_set_field.oxm_fields.vlan_vid = (UINT2)tag;
    act_set_field.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
    act_set_field.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field;

    memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
    act_set_field_mac.type = OFPAT13_SET_FIELD;
    memcpy(act_set_field_mac.oxm_fields.eth_dst, dst_mac, 6);
    act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
    act_set_field_mac.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field_mac;

    memset(&act_pushVlan, 0, sizeof(gn_action_t));
    act_pushVlan.type = OFPAT13_PUSH_VLAN;
    act_pushVlan.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_pushVlan;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return;
};

UINT1 get_fabric_last_flow_table(){
	return (UINT1)FABRIC_INPUT_TABLE;
};
UINT1 get_fabric_first_flow_table(){
	return (UINT1)FABRIC_SWAPTAG_TABLE;
};
UINT1 get_fabric_middle_flow_table(){
	return (UINT1)FABRIC_INPUT_TABLE;
};
/**************************************
 * Intern flow functions
 **************************************/
//add host input flow
/*
 * table 0;( input table)
 * match: ip ;
 * action: goto table 1 (push tag table)
 * priority: 0 (host in put flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
gn_flow_t * install_add_fabric_host_input_flow(gn_switch_t *sw){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_goto_table_t instruction;

	memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = g_cur_sys_time.tv_sec;
    flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_IMPL_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_HOST_INPUT_FLOW;
	flow.table_id = FABRIC_INPUT_TABLE;
	flow.match.type = OFPMT_OXM;
    flow.match.oxm_fields.eth_type = ETHER_IP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

    memset(&instruction, 0, sizeof(gn_instruction_goto_table_t));
    instruction.type = OFPIT_GOTO_TABLE;
    instruction.table_id = FABRIC_PUSHTAG_TABLE;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    return NULL;
}
//add ip missmatch push tag flow
/*
 * table 1;( push tag table)
 * match: ip ;
 * action: goto controller
 * priority: 0 (missmatch push tag flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
gn_flow_t * install_add_fabric_miss_match_flow(gn_switch_t *sw){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction;
	gn_action_output_t action;

	memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = g_cur_sys_time.tv_sec;
    flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_IMPL_HARD_TIME_OUT;
	flow.table_id = FABRIC_PUSHTAG_TABLE;
	flow.priority = FABRIC_PRIORITY_MISSMATCH_PUSHTAG_FLOW;
	flow.match.type = OFPMT_OXM;
    flow.match.oxm_fields.eth_type = ETHER_IP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

    memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    memset(&action, 0, sizeof(gn_action_output_t));
    action.port = OFPP13_CONTROLLER;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    return NULL;
}
//add arp missmatch push tag flow
/*
 * table 0;(input table)
 * match: arp;
 * action: goto controller
 * priority: 5 (arp missmatch push tag flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
gn_flow_t * install_add_fabric_ARP_miss_match_flow(gn_switch_t *sw){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction;
	gn_action_output_t action;

	memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = g_cur_sys_time.tv_sec;
    flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_IMPL_HARD_TIME_OUT;
	flow.table_id = FABRIC_INPUT_TABLE;
	flow.priority = FABRIC_PRIORITY_ARP_MISSMATCH_INPUT_FLOW;
	flow.match.type = OFPMT_OXM;
    flow.match.oxm_fields.eth_type = ETHER_ARP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

    memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    memset(&action, 0, sizeof(gn_action_output_t));
    action.port = OFPP13_CONTROLLER;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    return NULL;
}
//add switch input flows
/*
 * table 0;( input table)
 * match: ip ; input port;
 * action: goto table 1 (push tag table)
 * priority: 10 (switch in put flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
gn_flow_t * install_add_fabric_switch_input_flow(gn_switch_t *sw,gn_port_t* sw_port){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_goto_table_t instruction;

	memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = g_cur_sys_time.tv_sec;
    flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_IMPL_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_SWITCH_INPUT_FLOW;
	flow.table_id = FABRIC_INPUT_TABLE;
	flow.match.type = OFPMT_OXM;
//    flow.match.oxm_fields.eth_type = ETHER_IP;
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
	flow.match.oxm_fields.in_port = sw_port->port_no;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IN_PORT);

    memset(&instruction, 0, sizeof(gn_instruction_goto_table_t));
    instruction.type = OFPIT_CLEAR_ACTIONS;
    instruction.table_id = FABRIC_SWAPTAG_TABLE;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    return NULL;
}
/*
 * delete fabric flow real
 */
void install_delete_fabric_flow(gn_switch_t* sw){
	gn_flow_t flow;
	flow_mod_req_info_t flow_mod_req;

	// install delete command
	if(sw != NULL){
		memset(&flow, 0, sizeof(gn_flow_t));
		flow.idle_timeout = 0;
		flow.hard_timeout = 0;
		flow.priority = 0;
		flow.match.type = OFPMT_OXM;
		flow.table_id = 0xff;

	    flow_mod_req.xid = 0;
	    flow_mod_req.buffer_id = 0xffffffff;
	    flow_mod_req.out_port = 0xffffffff;
	    flow_mod_req.out_group = 0xffffffff;
	    flow_mod_req.command = OFPFC_DELETE;
	    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	    flow_mod_req.flow = &flow;
	    sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	}
	return;
};

void install_delete_fabric_impl_flow(gn_switch_t *sw,UINT4 port_no,UINT4 tag,UINT1 table_id){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction_act;
	gn_action_output_t act_output;

	memset(&flow, 0, sizeof(gn_flow_t));
	flow.table_id = table_id;
	flow.match.type = OFPMT_OXM;
	flow.match.oxm_fields.vlan_vid = (UINT2)tag;
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);

	memset(&act_output, 0, sizeof(gn_action_output_t));
	act_output.next = NULL;
	act_output.type = OFPAT13_OUTPUT;
	act_output.port = port_no;

	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.actions = (gn_action_t*)&act_output;
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_DELETE;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return;
};

gn_flow_t * install_add_fabric_impl_last_flow(gn_switch_t * sw,UINT4 tag){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_goto_table_t instruction_goto;
	gn_instruction_actions_t instruction_act;
	gn_action_t act_popVlan;

	memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_IMPL_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_SWAPTAG_FLOW;
	flow.table_id = FABRIC_INPUT_TABLE;
	flow.match.type = OFPMT_OXM;
	flow.match.oxm_fields.vlan_vid = (UINT2)tag;
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);

	memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
	instruction_goto.type = OFPIT_GOTO_TABLE;
	instruction_goto.table_id = FABRIC_OUTPUT_TABLE;
	instruction_goto.next = flow.instructions;
	flow.instructions = (gn_instruction_t *)&instruction_goto;

	memset(&act_popVlan, 0, sizeof(gn_action_t));
	act_popVlan.next = NULL;
	act_popVlan.type = OFPAT13_POP_VLAN;

	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.actions = &act_popVlan;
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return NULL;
};
gn_flow_t * install_add_fabric_impl_first_flow(gn_switch_t * sw,UINT4 port,UINT4 tag){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction_act;
	gn_action_output_t act_output;

	memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_IMPL_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_SWAPTAG_FLOW;
	flow.table_id = FABRIC_SWAPTAG_TABLE;
	flow.match.type = OFPMT_OXM;
	flow.match.oxm_fields.vlan_vid = (UINT2)tag;
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);

	memset(&act_output, 0, sizeof(gn_action_output_t));
	act_output.next = NULL;
	act_output.type = OFPAT13_OUTPUT;
	act_output.port = port;

	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.actions = (gn_action_t*)&act_output;
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return NULL;
};
gn_flow_t * install_add_fabric_impl_middle_flow(gn_switch_t * sw,UINT4 port,UINT4 tag){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction_act;
	gn_action_output_t act_output;

	memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_IMPL_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_SWAPTAG_FLOW;
	flow.table_id = FABRIC_INPUT_TABLE;
	flow.match.type = OFPMT_OXM;
	flow.match.oxm_fields.vlan_vid = (UINT2)tag;
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);

	memset(&act_output, 0, sizeof(gn_action_output_t));
	act_output.next = NULL;
	act_output.type = OFPAT13_OUTPUT;
	act_output.port = port;

	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.actions = (gn_action_t*)&act_output;
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	return NULL;
};

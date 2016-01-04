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
#include "fabric_openstack_nat.h"

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

void add_action_param(action_param_t** action_param, INT4 type, void* param);
flow_param_t* init_flow_param();
void clear_flow_param(flow_param_t* flow_param);

void install_fabric_flows(gn_switch_t * sw,
						  UINT2 idle_timeout,
						  UINT2 hard_timeout,
						  UINT2 priority,
						  UINT1 table_id,
						  UINT1 command,
						  flow_param_t* flow_param);
void set_security_match(gn_oxm_t* oxm, security_param_t* security_param);


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
void install_fabric_same_switch_flow(gn_switch_t * sw,UINT1* mac, UINT4 port)
{
	flow_param_t* flow_param = init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);
	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

	// printf("%s\n", FN);
	// nat_show_ip(sw->sw_ip);
//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction;
//    gn_action_output_t action;
//
//    memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
//	flow.table_id = FABRIC_INPUT_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//	memcpy(flow.match.oxm_fields.eth_dst, mac, 6);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//
//	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
//    instruction.type = OFPIT_APPLY_ACTIONS;
//    instruction.next = flow.instructions;
//    flow.instructions = (gn_instruction_t *)&instruction;
//
//    memset(&action, 0, sizeof(gn_action_t));
//    action.port = port;
//    action.type = OFPAT13_OUTPUT;
//    action.next = instruction.actions;
//    action.max_len = 0xffff;
//    instruction.actions = (gn_action_t *)&action;
//
//	flow_mod_req.xid = 0;
//	flow_mod_req.buffer_id = 0xffffffff;
//	flow_mod_req.out_port = 0xffffffff;
//	flow_mod_req.out_group = 0xffffffff;
//	flow_mod_req.command = OFPFC_ADD;
//	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//	flow_mod_req.flow = &flow;
//
//	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
//	return;
};

void install_fabric_same_switch_security_flow(gn_switch_t * sw,UINT1* mac, UINT4 port, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));


	memcpy(flow_param->match_param->eth_dst, mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);
	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}



void install_fabric_same_switch_out_subnet_flow(gn_switch_t * sw,UINT1* gateway_mac,UINT1* dst_mac,UINT4 dst_ip,UINT4 port, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, dst_mac, 6);

	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_dst, gateway_mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);


	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

//
//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction;
//    gn_action_output_t action;
//    gn_action_set_field_t act_set_field_mac;
//    // printf("%s\n", FN);
//    memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
//	flow.table_id = FABRIC_INPUT_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//	flow.match.oxm_fields.ipv4_dst=ntohl(dst_ip);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
//
//    flow.match.oxm_fields.eth_type = ETHER_IP;
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
//
//	memcpy(flow.match.oxm_fields.eth_dst, gateway_mac, 6);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//
//
//	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
//    instruction.type = OFPIT_APPLY_ACTIONS;
//    instruction.next = flow.instructions;
//    flow.instructions = (gn_instruction_t *)&instruction;
//
//    memset(&action, 0, sizeof(gn_action_t));
//    action.port = port;
//    action.type = OFPAT13_OUTPUT;
//    action.next = instruction.actions;
//    action.max_len = 0xffff;
//    instruction.actions = (gn_action_t *)&action;
//
//    memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
//    act_set_field_mac.type = OFPAT13_SET_FIELD;
//    memcpy(act_set_field_mac.oxm_fields.eth_dst, dst_mac, 6);
//    act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//    act_set_field_mac.next = instruction.actions;
//    instruction.actions = (gn_action_t *)&act_set_field_mac;
//
//	flow_mod_req.xid = 0;
//	flow_mod_req.buffer_id = 0xffffffff;
//	flow_mod_req.out_port = 0xffffffff;
//	flow_mod_req.out_group = 0xffffffff;
//	flow_mod_req.command = OFPFC_ADD;
//	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//	flow_mod_req.flow = &flow;
//
//	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
//	return;
};
void install_fabric_output_flow(gn_switch_t * sw,UINT1* mac, UINT4 port)
{
	flow_param_t* flow_param = init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);
	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_OUTPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
//
//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction;
//    gn_action_output_t action;
//
//    if ((NULL == sw) || (NULL == mac))
//    	return ;
//
//    memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
//	flow.table_id = FABRIC_OUTPUT_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//	memcpy(flow.match.oxm_fields.eth_dst, mac, 6);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//
//	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
//    instruction.type = OFPIT_APPLY_ACTIONS;
//    instruction.next = flow.instructions;
//    flow.instructions = (gn_instruction_t *)&instruction;
//
//    memset(&action, 0, sizeof(gn_action_t));
//    action.port = port;
//    action.type = OFPAT13_OUTPUT;
//    action.next = instruction.actions;
//    action.max_len = 0xffff;
//    instruction.actions = (gn_action_t *)&action;
//
//	flow_mod_req.xid = 0;
//	flow_mod_req.buffer_id = 0xffffffff;
//	flow_mod_req.out_port = 0xffffffff;
//	flow_mod_req.out_group = 0xffffffff;
//	flow_mod_req.command = OFPFC_ADD;
//	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//	flow_mod_req.flow = &flow;
//
//	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
//	return;
};
void install_fabric_push_tag_flow(gn_switch_t * sw,UINT1* mac, UINT4 tag)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)tag;

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	// flow_param->match_param->mask |= (1 << OFPXMT_OFB_ETH_DST);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return ;


//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction_act;
//	gn_instruction_goto_table_t instruction_goto;
//    gn_action_set_field_t act_set_field;
//    gn_action_t act_pushVlan;
//
//    memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
//	flow.table_id = FABRIC_PUSHTAG_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//	memcpy(flow.match.oxm_fields.eth_dst, mac, 6);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//
//	memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
//	instruction_goto.type = OFPIT_GOTO_TABLE;
//	instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
//	instruction_goto.next = flow.instructions;
//    flow.instructions = (gn_instruction_t *)&instruction_goto;
//
//	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
//	instruction_act.type = OFPIT_APPLY_ACTIONS;
//	instruction_act.next = flow.instructions;
//    flow.instructions = (gn_instruction_t *)&instruction_act;
//
//    memset(&act_set_field, 0, sizeof(gn_action_set_field_t));
//    act_set_field.type = OFPAT13_SET_FIELD;
//    act_set_field.oxm_fields.vlan_vid = (UINT2)tag;
//    act_set_field.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
//    act_set_field.next = instruction_act.actions;
//    instruction_act.actions = (gn_action_t *)&act_set_field;
//
//    memset(&act_pushVlan, 0, sizeof(gn_action_t));
//    act_pushVlan.type = OFPAT13_PUSH_VLAN;
//    act_pushVlan.next = instruction_act.actions;
//    instruction_act.actions = (gn_action_t *)&act_pushVlan;
//
//	flow_mod_req.xid = 0;
//	flow_mod_req.buffer_id = 0xffffffff;
//	flow_mod_req.out_port = 0xffffffff;
//	flow_mod_req.out_group = 0xffffffff;
//	flow_mod_req.command = OFPFC_ADD;
//	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//	flow_mod_req.flow = &flow;
//
//	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
//	return;
};

void install_fabric_push_tag_security_flow(gn_switch_t * sw,UINT4 dst_ip, UINT1* mac, UINT4 tag, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)tag;

	// memcpy(flow_param->match_param->eth_dst, mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}



void install_fabric_push_tag_out_subnet_flow(gn_switch_t * sw,UINT1* gateway_mac,UINT1* dst_mac,UINT4 dst_ip,UINT4 tag, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)tag;
	memcpy(oxm.eth_dst, dst_mac, 6);

	memcpy(flow_param->match_param->eth_dst, gateway_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst=ntohl(dst_ip);

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
			FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

	// printf("%s\n", FN);
//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction_act;
//	gn_instruction_goto_table_t instruction_goto;
//    gn_action_set_field_t act_set_field;
//    gn_action_set_field_t act_set_field_mac;
//    gn_action_t act_pushVlan;
//
//    memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
//	flow.table_id = FABRIC_PUSHTAG_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//	memcpy(flow.match.oxm_fields.eth_dst, gateway_mac, 6);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//
//    flow.match.oxm_fields.eth_type = ETHER_IP;
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
//
//	flow.match.oxm_fields.ipv4_dst=ntohl(dst_ip);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
//
//
//	memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
//	instruction_goto.type = OFPIT_GOTO_TABLE;
//	instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
//	instruction_goto.next = flow.instructions;
//    flow.instructions = (gn_instruction_t *)&instruction_goto;
//
//	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
//	instruction_act.type = OFPIT_APPLY_ACTIONS;
//	instruction_act.next = flow.instructions;
//    flow.instructions = (gn_instruction_t *)&instruction_act;
//
//    memset(&act_set_field, 0, sizeof(gn_action_set_field_t));
//    act_set_field.type = OFPAT13_SET_FIELD;
//    act_set_field.oxm_fields.vlan_vid = (UINT2)tag;
//    act_set_field.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
//    act_set_field.next = instruction_act.actions;
//    instruction_act.actions = (gn_action_t *)&act_set_field;
//
//    memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
//    act_set_field_mac.type = OFPAT13_SET_FIELD;
//    memcpy(act_set_field_mac.oxm_fields.eth_dst, dst_mac, 6);
//    act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//    act_set_field_mac.next = instruction_act.actions;
//    instruction_act.actions = (gn_action_t *)&act_set_field_mac;
//
//    memset(&act_pushVlan, 0, sizeof(gn_action_t));
//    act_pushVlan.type = OFPAT13_PUSH_VLAN;
//    act_pushVlan.next = instruction_act.actions;
//    instruction_act.actions = (gn_action_t *)&act_pushVlan;
//
//	flow_mod_req.xid = 0;
//	flow_mod_req.buffer_id = 0xffffffff;
//	flow_mod_req.out_port = 0xffffffff;
//	flow_mod_req.out_group = 0xffffffff;
//	flow_mod_req.command = OFPFC_ADD;
//	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//	flow_mod_req.flow = &flow;
//
//	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
//	return;
};


/*=== BEGIN === Added by zgzhao for controller API requirement 2015-12-28*/
BOOL install_fabric_json_flow(const gn_switch_t * sw, const flow_entry_json_t *entry)
{
	flow_param_t* flow_param = init_flow_param();

    flow_param->match_param->eth_type = entry->eth_type;
    flow_param->match_param->in_port = entry->in_port;
    flow_param->match_param->ipv4_src = htonl(entry->ipv4_src);
    
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

    //get all output: "OUTPUT=2,3,4"
    INT1 *actions = entry->actions;
    while ('\0' != *actions && '=' != *actions)
    {
        actions++;
    }

    if ('\0' == *actions || '\0' == *(actions + 1))
    {
        return FALSE;
    }

    actions++;

    INT1 *pre = actions;
    INT1 *cur = actions;
    INT1 tmp[1024] = {0};
    UINT4 *ports = (UINT4 *)gn_malloc(sizeof(UINT4) * 1024);
    INT4 i = 0;
    while ('\0' != *cur && i < 1024)
    {
        pre = cur;
        while (',' != *cur && '\0' != *cur)
        {
            cur++;
        }

        if (cur - pre <= 0)
        {
            gn_free((void *)(&ports));
            return FALSE;
        }
        
        memset(tmp, 0, 1024);
        strncpy(tmp, pre, cur - pre);
        if (!is_digit(tmp, 10))
        {
            gn_free((void *)(&ports));
            return FALSE;
        }

        ports[i] = (UINT4)strtoul(tmp, NULL, 10);

        add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&ports[i]);

        if (',' == *cur)
        {
            cur++;
        }

        i++;
    }

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, entry->priority,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);
    gn_free((void *)(&ports));
	clear_flow_param(flow_param);

    return TRUE;
}

void delete_fabric_json_flow(const gn_switch_t * sw, const flow_entry_json_t *entry)
{
    flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = entry->eth_type;
	flow_param->match_param->ipv4_src= htonl(entry->ipv4_src);
    flow_param->match_param->in_port= entry->in_port;

	install_fabric_flows(sw, 0, 0, entry->priority, FABRIC_INPUT_TABLE, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}
/*=== END === Added by zgzhao for controller API requirement 2015-12-28*/


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

void install_fabric_openstack_external_output_flow(gn_switch_t * sw,UINT4 port,UINT1* gateway_mac,UINT4 outer_interface_ip,UINT1 type){
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction;
	gn_action_output_t action;

	memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_IMPL_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_ARP_FLOW;
	flow.table_id = FABRIC_OUTPUT_TABLE;
	flow.match.type = OFPMT_OXM;

	if(type==1){
	    memcpy(flow.match.oxm_fields.eth_dst, gateway_mac, 6);
	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
	}else if(type==2){
		flow.match.oxm_fields.ipv4_src = ntohl(outer_interface_ip);
		flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);
		flow.match.oxm_fields.eth_type = ETHER_IP;
		flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
	}
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
}


void fabric_openstack_floating_ip_install_set_vlan_out_flow(gn_switch_t * sw, UINT4 match_ip, UINT1* match_mac, UINT4 mod_src_ip, UINT1* mod_dst_mac, UINT4 vlan_id, security_param_t* security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.ipv4_src = ntohl(mod_src_ip);
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.vlan_vid = vlan_id;

	memcpy(flow_param->match_param->eth_src, match_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_FLOATING_FLOW,
			FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);


	//printf("%s : sw ip is %s; vlan_id is %d \n", FN, inet_ntoa(*(struct in_addr*)&sw->sw_ip), vlan_id);
//	flow_mod_req_info_t flow_mod_req;
//	gn_flow_t flow;
//	gn_instruction_actions_t instruction_act;
//	gn_action_set_field_t act_set_field_mac;
//	gn_action_set_field_t act_set_field_ip;
//	gn_action_set_field_t act_set_field_vlan;
//	gn_action_t act_pushVlan;
//	gn_instruction_goto_table_t instruction_goto;
//
//	//match rule
//	memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_FLOATING_FLOW;
//	flow.table_id = FABRIC_PUSHTAG_TABLE;
//	flow.match.type = OFPMT_OXM;
//	flow.match.oxm_fields.eth_type = ETHER_IP;
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
//	flow.match.oxm_fields.ipv4_dst = ntohl(match_ip);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
//	memcpy(flow.match.oxm_fields.eth_src, match_mac, 6);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_SRC);
//
//	//set go-to-table
//	memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
//	instruction_goto.type = OFPIT_GOTO_TABLE;
//	instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
//	instruction_goto.next = flow.instructions;
//	flow.instructions = (gn_instruction_t *)&instruction_goto;
//
//	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
//	instruction_act.type = OFPIT_APPLY_ACTIONS;
//	instruction_act.next = flow.instructions;
//	flow.instructions = (gn_instruction_t *)&instruction_act;
//
//	//set dest mac
//	memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
//	act_set_field_mac.type = OFPAT13_SET_FIELD;
//	memcpy(act_set_field_mac.oxm_fields.eth_dst, mod_dst_mac, 6);
//	act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//	act_set_field_mac.next = instruction_act.actions;
//	instruction_act.actions = (gn_action_t *)&act_set_field_mac;
//
//	//set src ip
//	memset(&act_set_field_ip, 0, sizeof(gn_action_set_field_t));
//	act_set_field_ip.type = OFPAT13_SET_FIELD;
//	act_set_field_ip.oxm_fields.ipv4_src = ntohl(mod_src_ip);
//	act_set_field_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);
//	act_set_field_ip.next = instruction_act.actions;
//	instruction_act.actions = (gn_action_t *)&act_set_field_ip;
//
//	//set vlan
//	memset(&act_set_field_vlan, 0, sizeof(gn_action_set_field_t));
//	act_set_field_vlan.type = OFPAT13_SET_FIELD;
//	act_set_field_vlan.oxm_fields.vlan_vid = vlan_id;
//	act_set_field_vlan.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
//	act_set_field_vlan.next = instruction_act.actions;
//	instruction_act.actions = (gn_action_t *)&act_set_field_vlan;
//
//	memset(&act_pushVlan, 0, sizeof(gn_action_t));
//	act_pushVlan.type = OFPAT13_PUSH_VLAN;
//	act_pushVlan.next = instruction_act.actions;
//	instruction_act.actions = (gn_action_t *)&act_pushVlan;
//
//	flow_mod_req.xid = 0;
//	flow_mod_req.buffer_id = 0xffffffff;
//	flow_mod_req.out_port = 0xffffffff;
//	flow_mod_req.out_group = 0xffffffff;
//	flow_mod_req.command = OFPFC_ADD;
//	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//	flow_mod_req.flow = &flow;
//
//	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
}
void fabric_openstack_floating_ip_install_set_vlan_in_flow(gn_switch_t * sw, UINT4 match_ip, UINT4 mod_dst_ip, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_INPUT_TABLE;

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.ipv4_dst = ntohl(mod_dst_ip);
	oxm.vlan_vid = vlan_id;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (0 == get_nat_physical_switch_flag()) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);
	}

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_FLOATING_FLOW,
			flow_table_id, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

	//printf("%s : sw ip is %s; vlan_id is %d \n", FN, inet_ntoa(*(struct in_addr*)&sw->sw_ip), vlan_id);
//	flow_mod_req_info_t flow_mod_req;
//	gn_flow_t flow;
//	gn_instruction_actions_t instruction_act;
//	gn_action_set_field_t act_set_field_mac;
//	gn_action_set_field_t act_set_field_ip;
//	gn_action_set_field_t act_set_field_vlan;
//	gn_action_t act_pushVlan;
//	gn_instruction_goto_table_t instruction_goto;
//	gn_action_output_t output_action;
//
//	memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_FLOATING_FLOW;
//	flow.table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_INPUT_TABLE;
//	flow.match.type = OFPMT_OXM;
//	flow.match.oxm_fields.eth_type = ETHER_IP;
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
//	flow.match.oxm_fields.ipv4_dst = ntohl(match_ip);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
//
//
//	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
//	instruction_act.type = OFPIT_APPLY_ACTIONS;
//	instruction_act.next = flow.instructions;
//	flow.instructions = (gn_instruction_t *)&instruction_act;
//
//	if (0 == get_nat_physical_switch_flag()) {
//		memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
//		instruction_goto.type = OFPIT_GOTO_TABLE;
//		instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
//		instruction_goto.next = flow.instructions;
//		flow.instructions = (gn_instruction_t *)&instruction_goto;
//	}
//	else {
//		memset(&output_action, 0, sizeof(gn_action_output_t));
//		output_action.port = out_port;
//		output_action.type = OFPAT13_OUTPUT;
//		output_action.max_len = 0xffff;
//		output_action.next = instruction_act.actions;
//		instruction_act.actions = (gn_action_t *)&output_action;
//	}
//
//	memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
//	act_set_field_mac.type = OFPAT13_SET_FIELD;
//	memcpy(act_set_field_mac.oxm_fields.eth_dst, mod_dst_mac, 6);
//	act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//	act_set_field_mac.next = instruction_act.actions;
//	instruction_act.actions = (gn_action_t *)&act_set_field_mac;
//
//	memset(&act_set_field_ip, 0, sizeof(gn_action_set_field_t));
//	act_set_field_ip.type = OFPAT13_SET_FIELD;
//	act_set_field_ip.oxm_fields.ipv4_dst = ntohl(mod_dst_ip);
//	act_set_field_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
//	act_set_field_ip.next = instruction_act.actions;
//	instruction_act.actions = (gn_action_t *)&act_set_field_ip;
//
//	memset(&act_set_field_vlan, 0, sizeof(gn_action_set_field_t));
//	act_set_field_vlan.type = OFPAT13_SET_FIELD;
//	act_set_field_vlan.oxm_fields.vlan_vid = vlan_id;
//	act_set_field_vlan.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
//	act_set_field_vlan.next = instruction_act.actions;
//	instruction_act.actions = (gn_action_t *)&act_set_field_vlan;
//
//	memset(&act_pushVlan, 0, sizeof(gn_action_t));
//	act_pushVlan.type = OFPAT13_PUSH_VLAN;
//	act_pushVlan.next = instruction_act.actions;
//	instruction_act.actions = (gn_action_t *)&act_pushVlan;
//
//	flow_mod_req.xid = 0;
//	flow_mod_req.buffer_id = 0xffffffff;
//	flow_mod_req.out_port = 0xffffffff;
//	flow_mod_req.out_group = 0xffffffff;
//	flow_mod_req.command = OFPFC_ADD;
//	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//	flow_mod_req.flow = &flow;
//
//	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
}


// 下发流表规则
void install_fabric_nat_from_inside_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
								UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
								UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw, security_param_t* security_param)
{
	// LOG_PROC("INFO", "NAT: Start install_fabric_nat_flow !");
	/*
	 * Table 1
	 *
	 * Match:
	 * SRC_MAC  = VM.MAC
	 * DST_IP   = 外网IP
	 * Proto    = VM.Proto
	 * SRC_Port = VM.Port
	 *
	 * Action:
	 * SRC_MAC   = External.MAC
	 * SRC_IP    = External.IP
	 * SRC_Port  = new-port
	 * DST_MAC   = Gateway.MAC
	 * Vlan		 = Gateway.Vlan
	 *
	 * Goto:
	 * Table 2
	 ********************************************
	 * Table 1 (current switch is gateway switch)
	 *
	 * Match:
	 * SRC_MAC  = VM.MAC
	 * DST_IP   = 外网IP
	 * Proto    = VM.Proto
	 * SRC_Port = VM.Port
	 *
	 * Action:
	 * SRC_MAC   = External.MAC
	 * SRC_IP    = External.IP
	 * SRC_Port  = new-port
	 * output
	 */
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_src, external_mac, 6);
	oxm.ipv4_src = ntohl(external_ip);
	oxm.tcp_src = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	oxm.udp_src = (IPPROTO_UDP == proto_type) ? external_port_no : 0;
	oxm.vlan_vid = (sw->dpid != gateway_sw->dpid) ? (UINT2)gateway_vlan_vid : 0;
	memcpy(oxm.eth_dst, gateway_mac, 6);

	memcpy(flow_param->match_param->eth_src, packetin_src_mac, 6);
	flow_param->match_param->ipv4_dst = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) {
		add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	}
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (sw->dpid != gateway_sw->dpid) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&gateway_out_port);
	}

	set_security_match(flow_param->match_param, security_param);
	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_NAT_FLOW,
						 FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

	// This function is created to process NAT related flow table
//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction;
//
//    gn_action_set_field_t act_set_out_src_mac;
//    gn_action_set_field_t act_set_out_src_ip;
//    gn_action_set_field_t act_set_out_src_port;
//    gn_action_set_field_t act_set_out_dst_mac;
//    gn_action_set_field_t act_set_out_dst_vlan;
//    gn_instruction_goto_table_t act_set_out_goto_table;
//    gn_action_t act_out_push_vlan;
//    gn_action_output_t act_out_output;
//
//    memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_NAT_FLOW;
//	flow.table_id = FABRIC_PUSHTAG_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//	memcpy(flow.match.oxm_fields.eth_src, packetin_src_mac, 6);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_SRC);
//
//	flow.match.oxm_fields.ipv4_dst = ntohl(packetin_dst_ip);
//	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
//    flow.match.oxm_fields.eth_type = ETHER_IP;
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
//
//	if (IPPROTO_TCP == proto_type) {
//		flow.match.oxm_fields.ip_proto = IPPROTO_TCP;
//		flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IP_PROTO);
//		flow.match.oxm_fields.tcp_src = ntohs(packetin_src_port);
//		flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_TCP_SRC);
//	}
//	else if (IPPROTO_UDP == proto_type) {
//		flow.match.oxm_fields.ip_proto = IPPROTO_UDP;
//		flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IP_PROTO);
//		flow.match.oxm_fields.udp_src = ntohs(packetin_src_port);
//		flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_UDP_SRC);
//	}
//	else {
//		LOG_PROC("INFO","install_fabric_nat_flow: not TCP/UDP!");
//		return ;
//	}
//
//	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
//    instruction.type = OFPIT_APPLY_ACTIONS;
//    instruction.next = flow.instructions;
//    flow.instructions = (gn_instruction_t *)&instruction;
//
//    memset(&act_set_out_src_mac, 0, sizeof(gn_action_set_field_t));
//    act_set_out_src_mac.type = OFPAT13_SET_FIELD;
//    memcpy(act_set_out_src_mac.oxm_fields.eth_src, external_mac, 6);
//    act_set_out_src_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_SRC);
//    act_set_out_src_mac.next = instruction.actions;
//    instruction.actions = (gn_action_t *)&act_set_out_src_mac;
//
//    memset(&act_set_out_src_ip, 0, sizeof(gn_action_set_field_t));
//    act_set_out_src_ip.type = OFPAT13_SET_FIELD;
//    act_set_out_src_ip.oxm_fields.ipv4_src = ntohl(external_ip);
//    act_set_out_src_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);
//    act_set_out_src_ip.next = instruction.actions;
//    instruction.actions = (gn_action_t *)&act_set_out_src_ip;
//
//    if (IPPROTO_TCP == proto_type) {
//    	memset(&act_set_out_src_port, 0, sizeof(gn_action_set_field_t));
//		act_set_out_src_port.type = OFPAT13_SET_FIELD;
//		act_set_out_src_port.oxm_fields.tcp_src = external_port_no;
//		act_set_out_src_port.oxm_fields.mask |= (1 << OFPXMT_OFB_TCP_SRC);
//		act_set_out_src_port.next = instruction.actions;
//		instruction.actions = (gn_action_t *)&act_set_out_src_port;
//    }
//    else if (IPPROTO_UDP == proto_type) {
//    	memset(&act_set_out_src_port, 0, sizeof(gn_action_set_field_t));
//		act_set_out_src_port.type = OFPAT13_SET_FIELD;
//		act_set_out_src_port.oxm_fields.udp_src = external_port_no;
//		act_set_out_src_port.oxm_fields.mask |= (1 << OFPXMT_OFB_UDP_SRC);
//		act_set_out_src_port.next = instruction.actions;
//		instruction.actions = (gn_action_t *)&act_set_out_src_port;
//    }
//	else {
//		LOG_PROC("INFO","install_fabric_nat_flow: not TCP/UDP!");
//		return ;
//	}
//
//    if (sw->dpid != gateway_sw->dpid) {
//       	memset(&act_set_out_dst_vlan, 0, sizeof(gn_action_set_field_t));
//       	act_set_out_dst_vlan.type = OFPAT13_SET_FIELD;
//       	act_set_out_dst_vlan.oxm_fields.vlan_vid = (UINT2)gateway_vlan_vid;
//       	act_set_out_dst_vlan.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
//   		act_set_out_dst_vlan.next = instruction.actions;
//   		instruction.actions = (gn_action_t *)&act_set_out_dst_vlan;
//
//   		memset(&act_out_push_vlan, 0, sizeof(gn_action_t));
//   		act_out_push_vlan.type = OFPAT13_PUSH_VLAN;
//   		act_out_push_vlan.next = instruction.actions;
//   		instruction.actions = (gn_action_t *)&act_out_push_vlan;
//
//   		memset(&act_set_out_goto_table, 0, sizeof(gn_instruction_goto_table_t));
//   		act_set_out_goto_table.type = OFPIT_GOTO_TABLE;
//   		act_set_out_goto_table.table_id = FABRIC_SWAPTAG_TABLE;
//   		act_set_out_goto_table.next = flow.instructions;
//   		flow.instructions = (gn_instruction_t *)&act_set_out_goto_table;
//       }
//       else {
//   		memset(&act_out_output, 0, sizeof(gn_action_output_t));
//   		act_out_output.next = NULL;
//   		act_out_output.type = OFPAT13_OUTPUT;
//   		act_out_output.port = gateway_out_port;
//   		act_out_output.next = instruction.actions;
//   		act_out_output.max_len = 0xffff;
//   		instruction.actions = (gn_action_t *)&act_out_output;
//       }
//
//    memset(&act_set_out_dst_mac, 0, sizeof(gn_action_set_field_t));
//    act_set_out_dst_mac.type = OFPAT13_SET_FIELD;
//    memcpy(act_set_out_dst_mac.oxm_fields.eth_dst, gateway_mac, 6);
//    act_set_out_dst_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//    act_set_out_dst_mac.next = instruction.actions;
//    instruction.actions = (gn_action_t *)&act_set_out_dst_mac;
//
//	flow_mod_req.xid = 0;
//	flow_mod_req.buffer_id = 0xffffffff;
//	flow_mod_req.out_port = 0xffffffff;
//	flow_mod_req.out_group = 0xffffffff;
//	flow_mod_req.command = OFPFC_ADD;
//	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//	flow_mod_req.flow = &flow;
//
//    sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);

    // LOG_PROC("INFO", "NAT: Success install_fabric_nat_flow from inside !");
}

// 下发流表规则
void install_fabric_nat_from_external_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.ipv4_dst = ntohl(packetin_src_ip);
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;
	oxm.vlan_vid = (sw->dpid != gateway_sw->dpid) ? (UINT2)src_vlan_vid : 0;

	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ?  ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) {
		add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	}
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	if (sw->dpid != gateway_sw->dpid) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&gateway_out_port);
	}

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(gateway_sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_NAT_FLOW,
						 FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);



	// LOG_PROC("INFO", "NAT: Start install_fabric_nat_flow from external!");
	// This function is created to process NAT related flow table
//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction;

    /*
     * 匹配规则:
     * SRC_IP 		= external.IP
     * DST_Port 	= external.port_no
     *
     * Action:
     * DST_MAC		= VM.MAC
     * DST_IP		= VM.IP
     * DST_Port		= VM.Port
     * Vlan			= VM.Vlan
     *
     * Goto:
     * Table 2
     *********************************************
     * (Current switch is gateway switch)
     *
     * SRC_IP 		= external.IP
     * DST_Port 	= external.port_no
     *
     * Action:
     * DST_MAC		= VM.MAC
     * DST_IP		= VM.IP
     * DST_Port		= VM.Port
     * output
     */
//	memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_NAT_FLOW;
//	flow.table_id = FABRIC_PUSHTAG_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//    gn_action_set_field_t act_set_in_dst_mac;
//    gn_action_set_field_t act_set_in_dst_ip;
//    gn_action_set_field_t act_set_in_dst_port;
//    gn_action_set_field_t act_set_in_dst_vlan;
//    gn_instruction_goto_table_t act_set_in_goto_table;
//    gn_action_t act_in_push_vlan;
//    gn_action_output_t act_in_output;
//
//    flow.match.oxm_fields.ipv4_src = ntohl(packetin_dst_ip);
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);
//    flow.match.oxm_fields.eth_type = ETHER_IP;
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
//
//	if (IPPROTO_TCP == proto_type) {
//	    flow.match.oxm_fields.ip_proto = IPPROTO_TCP;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IP_PROTO);
//	    flow.match.oxm_fields.tcp_dst = external_port_no;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_TCP_DST);
//	}
//	else if (IPPROTO_UDP == proto_type) {
//	    flow.match.oxm_fields.ip_proto = IPPROTO_UDP;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IP_PROTO);
//	    flow.match.oxm_fields.udp_dst = external_port_no;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_UDP_DST);
//	}
//	else {
//		LOG_PROC("INFO","install_fabric_nat_flow: not TCP/UDP!");
//		return ;
//	}
//
//	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
//	instruction.type = OFPIT_APPLY_ACTIONS;
//	instruction.next = flow.instructions;
//	flow.instructions = (gn_instruction_t *)&instruction;
//
//	if (sw->dpid != gateway_sw->dpid) {
//		memset(&act_set_in_dst_vlan, 0, sizeof(gn_action_set_field_t));
//		act_set_in_dst_vlan.type = OFPAT13_SET_FIELD;
//		act_set_in_dst_vlan.oxm_fields.vlan_vid = (UINT2)src_vlan_vid;
//		act_set_in_dst_vlan.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
//		act_set_in_dst_vlan.next = instruction.actions;
//		instruction.actions = (gn_action_t *)&act_set_in_dst_vlan;
//
//		memset(&act_in_push_vlan, 0, sizeof(gn_action_t));
//		act_in_push_vlan.type = OFPAT13_PUSH_VLAN;
//		act_in_push_vlan.next = instruction.actions;
//		instruction.actions = (gn_action_t *)&act_in_push_vlan;
//
//		memset(&act_set_in_goto_table, 0, sizeof(gn_instruction_goto_table_t));
//		act_set_in_goto_table.type = OFPIT_GOTO_TABLE;
//		act_set_in_goto_table.table_id = FABRIC_SWAPTAG_TABLE;
//		act_set_in_goto_table.next = flow.instructions;
//		flow.instructions = (gn_instruction_t *)&act_set_in_goto_table;
//	}
//	else {
//		memset(&act_in_output, 0, sizeof(gn_action_output_t));
//		act_in_output.next = NULL;
//		act_in_output.type = OFPAT13_OUTPUT;
//		act_in_output.port = gateway_out_port;
//		act_in_output.next = instruction.actions;
//		act_in_output.max_len = 0xffff;
//		instruction.actions = (gn_action_t *)&act_in_output;
//	}
//
//    memset(&act_set_in_dst_mac, 0, sizeof(gn_action_set_field_t));
//    act_set_in_dst_mac.type = OFPAT13_SET_FIELD;
//    memcpy(act_set_in_dst_mac.oxm_fields.eth_dst, packetin_src_mac, 6);
//    act_set_in_dst_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//    act_set_in_dst_mac.next = instruction.actions;
//    instruction.actions = (gn_action_t *)&act_set_in_dst_mac;
//
//    memset(&act_set_in_dst_ip, 0, sizeof(gn_action_set_field_t));
//    act_set_in_dst_ip.type = OFPAT13_SET_FIELD;
//    act_set_in_dst_ip.oxm_fields.ipv4_dst = ntohl(packetin_src_ip);
//    act_set_in_dst_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
//    act_set_in_dst_ip.next = instruction.actions;
//    instruction.actions = (gn_action_t *)&act_set_in_dst_ip;
//
//    if (IPPROTO_TCP == proto_type) {
//    	memset(&act_set_in_dst_port, 0, sizeof(gn_action_set_field_t));
//		act_set_in_dst_port.type = OFPAT13_SET_FIELD;
//		act_set_in_dst_port.oxm_fields.tcp_dst = ntohs(packetin_src_port);
//		act_set_in_dst_port.oxm_fields.mask |= (1 << OFPXMT_OFB_TCP_DST);
//		act_set_in_dst_port.next = instruction.actions;
//		instruction.actions = (gn_action_t *)&act_set_in_dst_port;
//    }
//    else if (IPPROTO_UDP == proto_type) {
//    	memset(&act_set_in_dst_port, 0, sizeof(gn_action_set_field_t));
//		act_set_in_dst_port.type = OFPAT13_SET_FIELD;
//		act_set_in_dst_port.oxm_fields.udp_dst = ntohs(packetin_src_port);
//		act_set_in_dst_port.oxm_fields.mask |= (1 << OFPXMT_OFB_UDP_DST);
//		act_set_in_dst_port.next = instruction.actions;
//		instruction.actions = (gn_action_t *)&act_set_in_dst_port;
//    }
//	else {
//		LOG_PROC("INFO","install_fabric_nat_flow: not TCP/UDP!");
//		return ;
//	}
//
//  	flow_mod_req.xid = 0;
//  	flow_mod_req.buffer_id = 0xffffffff;
//  	flow_mod_req.out_port = 0xffffffff;
//  	flow_mod_req.out_group = 0xffffffff;
//  	flow_mod_req.command = OFPFC_ADD;
//  	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//  	flow_mod_req.flow = &flow;
//
//  	gateway_sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](gateway_sw, (UINT1 *)&flow_mod_req);

    // LOG_PROC("INFO", "NAT: Success install_fabric_nat_flow from external !");
}

// 下发流表规则
void install_fabric_nat_from_external_fabric_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw, UINT4 out_port)
{
	flow_param_t* flow_param = init_flow_param();

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.vlan_vid = (UINT2)src_vlan_vid;

	memcpy(flow_param->match_param->eth_dst, external_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto_type) ? external_port_no : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(gateway_sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_NAT_FLOW,
					     FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);


	// LOG_PROC("INFO", "NAT: Start install_fabric_nat_flow from external!");
	// This function is created to process NAT related flow table
//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction;
//    gn_action_output_t act_in_output;

    /*
     * 匹配规则:
     * DST_MAC 		= external.MAC
     * DST_Port 	= external.port_no
     *
     * Action:
     * Vlan			= VM.Vlan
     *
     * Goto:
     * Table 2
     */
//	memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_NAT_FLOW;
//	flow.table_id = FABRIC_INPUT_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//    gn_action_set_field_t act_set_in_dst_vlan;
//    gn_action_t act_in_push_vlan;
//
//    memcpy(flow.match.oxm_fields.eth_dst, external_mac, 6);
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//
//    flow.match.oxm_fields.eth_type = ETHER_IP;
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
//
//	if (IPPROTO_TCP == proto_type) {
//	    flow.match.oxm_fields.ip_proto = IPPROTO_TCP;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IP_PROTO);
//	    flow.match.oxm_fields.tcp_dst = external_port_no;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_TCP_DST);
//	}
//	else if (IPPROTO_UDP == proto_type) {
//	    flow.match.oxm_fields.ip_proto = IPPROTO_UDP;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IP_PROTO);
//	    flow.match.oxm_fields.udp_dst = external_port_no;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_UDP_DST);
//	}
//	else {
//		LOG_PROC("INFO","install_fabric_nat_flow: not TCP/UDP!");
//		return ;
//	}
//
//	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
//	instruction.type = OFPIT_APPLY_ACTIONS;
//	instruction.next = flow.instructions;
//	flow.instructions = (gn_instruction_t *)&instruction;
//
//	memset(&act_in_output, 0, sizeof(gn_action_output_t));
//	act_in_output.type = OFPAT13_OUTPUT;
//	act_in_output.port = out_port;
//	act_in_output.next = instruction.actions;
//	act_in_output.max_len = 0xffff;
//	instruction.actions = (gn_action_t *)&act_in_output;
//
//	memset(&act_set_in_dst_vlan, 0, sizeof(gn_action_set_field_t));
//	act_set_in_dst_vlan.type = OFPAT13_SET_FIELD;
//	act_set_in_dst_vlan.oxm_fields.vlan_vid = (UINT2)src_vlan_vid;
//	act_set_in_dst_vlan.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
//	act_set_in_dst_vlan.next = instruction.actions;
//	instruction.actions = (gn_action_t *)&act_set_in_dst_vlan;
//
//	memset(&act_in_push_vlan, 0, sizeof(gn_action_t));
//	act_in_push_vlan.type = OFPAT13_PUSH_VLAN;
//	act_in_push_vlan.next = instruction.actions;
//	instruction.actions = (gn_action_t *)&act_in_push_vlan;
//
//  	flow_mod_req.xid = 0;
//  	flow_mod_req.buffer_id = 0xffffffff;
//  	flow_mod_req.out_port = 0xffffffff;
//  	flow_mod_req.out_group = 0xffffffff;
//  	flow_mod_req.command = OFPFC_ADD;
//  	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//  	flow_mod_req.flow = &flow;
//
//  	gateway_sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](gateway_sw, (UINT1 *)&flow_mod_req);
//
//     LOG_PROC("INFO", "NAT: Success install_fabric_nat_flow from external !");
}

// 下发流表规则
void install_fabric_nat_from_external_fabric_host_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw)
{
	flow_param_t* flow_param = init_flow_param();

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.ipv4_dst = ntohl(packetin_src_ip);
	oxm.tcp_dst = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	oxm.udp_dst = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto_type) ? external_port_no : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&gateway_out_port);

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_NAT_FLOW,
						 FABRIC_OUTPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);



	// LOG_PROC("INFO", "NAT: Start install_fabric_nat_flow from external!");
	// This function is created to process NAT related flow table
//	flow_mod_req_info_t flow_mod_req;
//    gn_flow_t flow;
//    gn_instruction_actions_t instruction;

    /*
     * 匹配规则:
     * SRC_IP 		= external.IP
     * DST_Port 	= external.port_no
     *
     * Action:
     * DST_MAC		= VM.MAC
     * DST_IP		= VM.IP
     * DST_Port		= VM.Port
     * output
     */
//	memset(&flow, 0, sizeof(gn_flow_t));
//	flow.create_time = g_cur_sys_time.tv_sec;
//	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
//	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
//	flow.priority = FABRIC_PRIORITY_NAT_FLOW;
//	flow.table_id = FABRIC_OUTPUT_TABLE;
//	flow.match.type = OFPMT_OXM;
//
//    gn_action_set_field_t act_set_in_dst_mac;
//    gn_action_set_field_t act_set_in_dst_ip;
//    gn_action_set_field_t act_set_in_dst_port;
//    gn_action_output_t act_in_output;
//
//    flow.match.oxm_fields.ipv4_src = ntohl(packetin_dst_ip);
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);
//    flow.match.oxm_fields.eth_type = ETHER_IP;
//    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
//
//	if (IPPROTO_TCP == proto_type) {
//	    flow.match.oxm_fields.ip_proto = IPPROTO_TCP;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IP_PROTO);
//	    flow.match.oxm_fields.tcp_dst = external_port_no;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_TCP_DST);
//	}
//	else if (IPPROTO_UDP == proto_type) {
//	    flow.match.oxm_fields.ip_proto = IPPROTO_UDP;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IP_PROTO);
//	    flow.match.oxm_fields.udp_dst = external_port_no;
//	    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_UDP_DST);
//	}
//	else {
//		LOG_PROC("INFO","install_fabric_nat_flow: not TCP/UDP!");
//		return ;
//	}
//
//	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
//	instruction.type = OFPIT_APPLY_ACTIONS;
//	instruction.next = flow.instructions;
//	flow.instructions = (gn_instruction_t *)&instruction;
//
//	memset(&act_in_output, 0, sizeof(gn_action_output_t));
//	act_in_output.type = OFPAT13_OUTPUT;
//	act_in_output.port = gateway_out_port;
//	act_in_output.next = instruction.actions;
//	act_in_output.max_len = 0xffff;
//	instruction.actions = (gn_action_t *)&act_in_output;
//
//    memset(&act_set_in_dst_mac, 0, sizeof(gn_action_set_field_t));
//    act_set_in_dst_mac.type = OFPAT13_SET_FIELD;
//    memcpy(act_set_in_dst_mac.oxm_fields.eth_dst, packetin_src_mac, 6);
//    act_set_in_dst_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
//    act_set_in_dst_mac.next = instruction.actions;
//    instruction.actions = (gn_action_t *)&act_set_in_dst_mac;
//
//    memset(&act_set_in_dst_ip, 0, sizeof(gn_action_set_field_t));
//    act_set_in_dst_ip.type = OFPAT13_SET_FIELD;
//    act_set_in_dst_ip.oxm_fields.ipv4_dst = ntohl(packetin_src_ip);
//    act_set_in_dst_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
//    act_set_in_dst_ip.next = instruction.actions;
//    instruction.actions = (gn_action_t *)&act_set_in_dst_ip;
//
//    if (IPPROTO_TCP == proto_type) {
//    	memset(&act_set_in_dst_port, 0, sizeof(gn_action_set_field_t));
//		act_set_in_dst_port.type = OFPAT13_SET_FIELD;
//		act_set_in_dst_port.oxm_fields.tcp_dst = ntohs(packetin_src_port);
//		act_set_in_dst_port.oxm_fields.mask |= (1 << OFPXMT_OFB_TCP_DST);
//		act_set_in_dst_port.next = instruction.actions;
//		instruction.actions = (gn_action_t *)&act_set_in_dst_port;
//    }
//    else if (IPPROTO_UDP == proto_type) {
//    	memset(&act_set_in_dst_port, 0, sizeof(gn_action_set_field_t));
//		act_set_in_dst_port.type = OFPAT13_SET_FIELD;
//		act_set_in_dst_port.oxm_fields.udp_dst = ntohs(packetin_src_port);
//		act_set_in_dst_port.oxm_fields.mask |= (1 << OFPXMT_OFB_UDP_DST);
//		act_set_in_dst_port.next = instruction.actions;
//		instruction.actions = (gn_action_t *)&act_set_in_dst_port;
//    }
//	else {
//		LOG_PROC("INFO","install_fabric_nat_flow: not TCP/UDP!");
//		return ;
//	}
//
//  	flow_mod_req.xid = 0;
//  	flow_mod_req.buffer_id = 0xffffffff;
//  	flow_mod_req.out_port = 0xffffffff;
//  	flow_mod_req.out_group = 0xffffffff;
//  	flow_mod_req.command = OFPFC_ADD;
//  	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
//  	flow_mod_req.flow = &flow;
//
//  	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
//
//     LOG_PROC("INFO", "NAT: Success install_fabric_nat_flow from external !");
}


void delete_fabric_input_flow_by_ip(gn_switch_t* sw, UINT4 ip)
{
	// printf("******\n*******\n***********\n");
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_INPUT_TABLE;//(0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_INPUT_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}

void delete_fabric_input_flow_by_mac_portno(gn_switch_t* sw, UINT1* dst_mac, UINT2 dst_port, UINT2 proto)
{
	// printf("******\n*******\n***********\n");
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_INPUT_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_dst, dst_mac, 6);
	flow_param->match_param->ip_proto = proto;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto) ? dst_port : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto) ? dst_port : 0;

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}


void delete_fabric_flow_by_ip(gn_switch_t* sw, UINT4 ip, UINT2 table_id)
{
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);

	install_fabric_flows(sw, 0, 0, 0, table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}

void delete_fabric_flow_by_mac(gn_switch_t* sw, UINT1* mac, UINT2 table_id)
{
	flow_param_t* flow_param = init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);

	install_fabric_flows(sw, 0, 0, 0, table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}


void set_flow_match(gn_oxm_t* flow_oxm, gn_oxm_t* oxm)
{
	// printf("%s\n", FN);
	memcpy(flow_oxm, oxm, sizeof(gn_oxm_t));

	if (oxm->in_port) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IN_PORT);
	}
	if (oxm->in_phy_port) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IN_PHY_PORT);
	}
	if (oxm->metadata) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_METADATA);
	}
	if (0xffffff & *oxm->eth_dst) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ETH_DST);
	}
	if (0xffffff & *oxm->eth_src) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ETH_SRC);
	}
	if (oxm->eth_type) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ETH_TYPE);
	}
	if (oxm->vlan_vid) {
		// printf("**********vlan vid:%d\n", oxm->vlan_vid);
		flow_oxm->mask |= (1 << OFPXMT_OFB_VLAN_VID);
	}
	if (oxm->vlan_pcp) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_VLAN_PCP);
	}
	if (oxm->ip_dscp) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IP_DSCP);
	}
	if (oxm->ip_ecn) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IP_ECN);
	}
	if (oxm->ip_proto) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IP_PROTO);
	}
	if (oxm->ipv4_src) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV4_SRC);
	}
	if (oxm->ipv4_dst) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV4_DST);
	}
	if (oxm->tcp_src) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_TCP_SRC);
	}
	if (oxm->tcp_dst) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_TCP_DST);
	}
	if (oxm->udp_src) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_UDP_SRC);
	}
	if (oxm->udp_dst) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_UDP_DST);
	}
	if (oxm->icmpv4_type) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ICMPV4_TYPE);
	}
	if (oxm->icmpv4_code) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ICMPV4_CODE);
	}
	if (oxm->arp_op) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_OP);
	}
	if (oxm->arp_spa) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_SPA);
	}
	if (oxm->arp_tpa) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_TPA);
	}
	if (0xffffff & *oxm->arp_sha) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_SHA);
	}
	if (0xffffff & *oxm->arp_tha) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_THA);
	}
	if (0xffffff & *oxm->ipv6_src) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV6_SRC);
	}
	if (0xffffff & *oxm->ipv6_dst) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV6_DST);
	}
	/*
	if (oxm->mpls_label) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_MPLS_LABEL);
	}
	if (oxm->tunnel_id) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_TUNNEL_ID);
	}
	if (oxm->ipv4_src_prefix) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV4_SRC_PREFIX);
	}
	if (oxm->ipv4_dst_prefix) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV4_DST_PREFIX);
	}
	if (oxm->ipv6_src_prefix) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV6_SRC_PREFIX);
	}
	if (oxm->ipv6_dst_prefix) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV6_DST_PREFIX);
	}
	if (oxm->metadata_mask) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_METADATA_MASK);
	}
	*/
}

void set_security_match( gn_oxm_t* oxm, security_param_t* security_param)
{
	// printf("%s\n", FN);
	if ((NULL != security_param) && (security_param->ip_proto)) {
		oxm->ip_proto = security_param->ip_proto;
		oxm->icmpv4_code = security_param->icmp_code;
		oxm->icmpv4_type = security_param->imcp_type;
		oxm->tcp_src = security_param->tcp_port_num;
		oxm->udp_src = security_param->udp_port_num;
	}
}

gn_instruction_t* set_flow_instruction(gn_instruction_t* flow_instruction, action_param_t* instraction_param)
{
	// printf("%s\n", FN);
	gn_instruction_t* instruction = flow_instruction;

	while (NULL != instraction_param) {
		switch (instraction_param->type) {
		case OFPIT_GOTO_TABLE:
		{
			gn_instruction_goto_table_t* instruction_goto = gn_malloc(sizeof(gn_instruction_goto_table_t));
			memset(instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
			instruction_goto->type = OFPIT_GOTO_TABLE;
			instruction_goto->table_id = FABRIC_SWAPTAG_TABLE;
			instruction_goto->next = instruction;
			instruction = (gn_instruction_t *)instruction_goto;
			break;
		}
		case OFPIT_WRITE_METADATA:
		{
			break;
		}
		case OFPIT_WRITE_ACTIONS:
		{
			break;
		}
		case OFPIT_APPLY_ACTIONS:
		{
			// printf("***%s\n",FN);
			gn_instruction_actions_t* instruction_apply = (gn_instruction_actions_t*)gn_malloc(sizeof(gn_instruction_actions_t));
			memset(instruction_apply, 0, sizeof(gn_instruction_actions_t));
			instruction_apply->type = (UINT2)OFPIT_APPLY_ACTIONS;
			instruction_apply->next = instruction;
			instruction = (gn_instruction_t *)instruction_apply;
			break;
		}
		case OFPIT_CLEAR_ACTIONS:
		{
			gn_instruction_actions_t* instruction_clear = gn_malloc(sizeof(gn_instruction_actions_t));
			memset(instruction_clear, 0, sizeof(gn_instruction_actions_t));
			instruction_clear->type = OFPIT_CLEAR_ACTIONS;
			instruction_clear->next = instruction;
			instruction = (gn_instruction_t *)instruction_clear;
			break;
		}
		case OFPIT_METER:
		{
			break;
		}
		case OFPIT_EXPERIMENTER:
		{
			break;
		}
		default:
			break;
		}
		instraction_param = instraction_param->next;
	}

	return instruction;
}

void set_flow_action(gn_instruction_t* flow_instruction, action_param_t* action_param)
{
	// printf("%s\n", FN);
	gn_instruction_t* instruction_p = flow_instruction;
	if (NULL == instruction_p) {
		// printf("%s instruction is NULL!\n", FN);
	}

	while (NULL != instruction_p)
	{
		if (OFPIT_APPLY_ACTIONS == instruction_p->type) {
			break;
		}
		instruction_p = instruction_p->next;
	}

	if (NULL == instruction_p) {
		return ;
	}

	// printf("%s********************\n", FN);
	gn_instruction_actions_t* instruction = (gn_instruction_actions_t*)instruction_p;

	while (NULL != action_param) {
		// printf("action type is %d,parameter is%d\n", action_param->type, *(UINT4*)action_param->param);
		switch (action_param->type) {
		case OFPAT13_OUTPUT:
		{
			// printf("%s****output********\n", FN);
			gn_action_output_t* action_output = gn_malloc(sizeof(gn_action_output_t));
			memset(action_output, 0, sizeof(gn_action_t));
			action_output->port = *(UINT4*)action_param->param;
			action_output->type = OFPAT13_OUTPUT;
			action_output->next = instruction->actions;
			action_output->max_len = 0xffff;
			instruction->actions = (gn_action_t *)action_output;
			break;
		}
		case OFPAT13_COPY_TTL_OUT:
		{
			break;
		}
		case OFPAT13_COPY_TTL_IN:
		{
			break;
		}
		case OFPAT13_MPLS_TTL:
		{
			break;
		}
		case OFPAT13_DEC_MPLS_TTL:
		{
			break;
		}
		case OFPAT13_PUSH_VLAN:
		{
			//
			gn_action_t* act_pushVlan = gn_malloc(sizeof(gn_action_t));
			memset(act_pushVlan, 0, sizeof(gn_action_t));
			act_pushVlan->type = OFPAT13_PUSH_VLAN;
			act_pushVlan->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_pushVlan;
			break;
		}
		case OFPAT13_POP_VLAN:
		{
			//
			gn_action_t* act_popVlan = gn_malloc(sizeof(gn_action_t));
			memset(act_popVlan, 0, sizeof(gn_action_t));
			act_popVlan->next = instruction->actions;
			act_popVlan->type = OFPAT13_POP_VLAN;
			instruction->actions = (gn_action_t *)act_popVlan;
			break;
		}
		case OFPAT13_PUSH_MPLS:
		{
			//
			break;
		}
		case OFPAT13_POP_MPLS:
		{
			//
			break;
		}
		case OFPAT13_SET_QUEUE:
		{
			break;
		}
		case OFPAT13_GROUP:
		{
			//
			break;
		}
		case OFPAT13_SET_NW_TTL:
		{
			break;
		}
		case OFPAT13_DEC_NW_TTL:
		{
			break;
		}
		case OFPAT13_SET_FIELD:
		{
			//
			gn_action_set_field_t* act_set_field = gn_malloc(sizeof(gn_action_set_field_t));
			memset(act_set_field, 0, sizeof(gn_action_set_field_t));
			act_set_field->type = OFPAT13_SET_FIELD;
			set_flow_match(&act_set_field->oxm_fields, (gn_oxm_t*)action_param->param);
			// printf("action param: %d, %llu", act_set_field->oxm_fields.vlan_vid, act_set_field->oxm_fields.mask);
			act_set_field->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_set_field;
			break;
		}
		case OFPAT13_PUSH_PBB:
		{
			break;
		}
		case OFPAT13_POP_PBB:
		{
			break;
		}
		case OFPAT13_EXPERIMENTER:
		{
			break;
		}
		default:
			break;
		}
		action_param = action_param->next;
	}
}

void clear_flow_temp_data(gn_instruction_t* instruction)
{
	// printf("%s\n", FN);
	gn_instruction_t* temp_instruction = NULL;
	if (NULL != instruction) {
		temp_instruction = instruction->next;
	}

	while (NULL != temp_instruction) {
		if (OFPIT_APPLY_ACTIONS == instruction->type) {
			gn_instruction_actions_t* action_instruction = (gn_instruction_actions_t*)instruction;
			gn_action_t* action_p = action_instruction->actions;
			gn_action_t* temp_p = NULL;
			if (NULL != action_p) {
				temp_p = action_p->next;
			}
			while (NULL != temp_p) {
				action_p = NULL;
				gn_free((void**)&action_p);
				action_p = temp_p;
				temp_p = temp_p->next;
			}
			if (NULL != action_p) {
				action_p = NULL;
				gn_free((void**)&action_p);
			}
		}
		instruction = NULL;
		gn_free((void**)&instruction);
		instruction = temp_instruction;
		temp_instruction = temp_instruction->next;
	}
	if (NULL != instruction) {
		instruction = NULL;
		gn_free((void**)&instruction);
	}
}

void install_fabric_flows(gn_switch_t * sw,
						  UINT2 idle_timeout,
						  UINT2 hard_timeout,
						  UINT2 priority,
						  UINT1 table_id,
						  UINT1 command,
						  flow_param_t* flow_param)
{
	// printf("%s\n", FN);
	flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = idle_timeout;
	flow.hard_timeout = hard_timeout;
	flow.priority = priority;
	flow.table_id = table_id;
	flow.match.type = OFPMT_OXM;

	set_flow_match(&flow.match.oxm_fields, flow_param->match_param);
	// printf("flow match mask is %llu\n", flow.match.oxm_fields.mask);
	flow.instructions = set_flow_instruction(flow.instructions, flow_param->instruction_param);
	set_flow_action(flow.instructions, flow_param->action_param);

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = command;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	clear_flow_temp_data(flow.instructions);
}

void add_action_param(action_param_t** action_param, INT4 type, void* param)
{
	action_param_t* new_param = (action_param_t*)gn_malloc(sizeof(action_param_t));
	memset(new_param, 0, sizeof(action_param_t));
	new_param->type = type;
	new_param->param = param;
	new_param->next = *action_param;
	*action_param = new_param;
}

void clear_action_param(action_param_t* action_param)
{
	action_param_t* temp = NULL;
	if (NULL != action_param) {
		temp = action_param->next;
	}

	while (NULL != temp) {
		temp = action_param->next;
		action_param = NULL;
		gn_free((void**)&action_param);
		action_param = temp;
		temp = temp->next;
	}

	if (NULL != action_param) {
		gn_free((void**)&action_param);
		action_param = temp;
	}
}

flow_param_t* init_flow_param()
{
	flow_param_t* flow_param = (flow_param_t*)gn_malloc(sizeof(flow_param_t));
	memset(flow_param, 0, sizeof(flow_param_t));
	flow_param->match_param = (gn_oxm_t*)gn_malloc(sizeof(gn_oxm_t));
	memset(flow_param->match_param, 0, sizeof(gn_oxm_t));
	flow_param->instruction_param = NULL;
	flow_param->action_param = NULL;

	return flow_param;
}

void clear_flow_param(flow_param_t* flow_param)
{
	flow_param->match_param = NULL;
	gn_free((void**)&flow_param->match_param);
	clear_action_param(flow_param->instruction_param);
	clear_action_param(flow_param->action_param);
	gn_free((void**)&flow_param);
}




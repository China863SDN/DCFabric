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
 * fabric_floating_ip.c
 *
 *  Created on: Sep 8, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: Sep 8, 2015
 */

#include "fabric_floating_ip.h"
#include "common.h"
#include "mod-types.h"
#include "fabric_flows.h"
#include "timer.h"
#include "gn_inet.h"
#include "openstack_host.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../conn-svr/conn-svr.h"
#include "fabric_impl.h"
#include "fabric_openstack_nat.h"

const UINT1 nat_zero_mac[] = {0x0,0x0,0x0,0x0,0x0,0x0};
const UINT1 nat_broadcat_mac[] = {0xff,0xff,0xff,0xff,0xff,0xff};
extern void fabric_openstack_packet_flood(packet_in_info_t *packet_in_info);

extern void fabric_openstack_create_arp_reply_public(UINT1* srcMac, UINT4 srcIP, UINT1* dstMac, UINT4 dstIP, gn_switch_t* sw,
		UINT4 outPort, packet_in_info_t *packet_in_info);

extern void fabric_openstack_show_ip(UINT4 ip);

extern void fabric_openstack_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport);

extern UINT4 of131_fabric_impl_get_tag_sw(gn_switch_t *sw);

extern void fabric_openstack_packet_flood(packet_in_info_t *packet_in_info);

extern openstack_port_p find_openstack_app_gateway_by_host(openstack_port_p host);

void fabric_openstack_floating_ip_install_set_vlan_out_flow(gn_switch_t * sw, UINT4 match_ip, UINT1* match_mac, UINT4 mod_src_ip, UINT1* mod_dst_mac, UINT4 vlan_id)
{
	//printf("%s : sw ip is %s; vlan_id is %d \n", FN, inet_ntoa(*(struct in_addr*)&sw->sw_ip), vlan_id);
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction_act;
	gn_action_set_field_t act_set_field_mac;
	gn_action_set_field_t act_set_field_ip;
	gn_action_set_field_t act_set_field_vlan;
	gn_action_t act_pushVlan;
	gn_instruction_goto_table_t instruction_goto;

	//match rule
	memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_FLOATING_FLOW;
	flow.table_id = FABRIC_PUSHTAG_TABLE;
	flow.match.type = OFPMT_OXM;
	flow.match.oxm_fields.eth_type = ETHER_IP;
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
	flow.match.oxm_fields.ipv4_dst = ntohl(match_ip);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
	memcpy(flow.match.oxm_fields.eth_src, match_mac, 6);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_SRC);

	//set go-to-table
	memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
	instruction_goto.type = OFPIT_GOTO_TABLE;
	instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
	instruction_goto.next = flow.instructions;
	flow.instructions = (gn_instruction_t *)&instruction_goto;

	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
	instruction_act.type = OFPIT_APPLY_ACTIONS;
	instruction_act.next = flow.instructions;
	flow.instructions = (gn_instruction_t *)&instruction_act;

	//set dest mac
	memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
	act_set_field_mac.type = OFPAT13_SET_FIELD;
	memcpy(act_set_field_mac.oxm_fields.eth_dst, mod_dst_mac, 6);
	act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
	act_set_field_mac.next = instruction_act.actions;
	instruction_act.actions = (gn_action_t *)&act_set_field_mac;

	//set src ip
	memset(&act_set_field_ip, 0, sizeof(gn_action_set_field_t));
	act_set_field_ip.type = OFPAT13_SET_FIELD;
	act_set_field_ip.oxm_fields.ipv4_src = ntohl(mod_src_ip);
	act_set_field_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);
	act_set_field_ip.next = instruction_act.actions;
	instruction_act.actions = (gn_action_t *)&act_set_field_ip;

	//set vlan
	memset(&act_set_field_vlan, 0, sizeof(gn_action_set_field_t));
	act_set_field_vlan.type = OFPAT13_SET_FIELD;
	act_set_field_vlan.oxm_fields.vlan_vid = vlan_id;
	act_set_field_vlan.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
	act_set_field_vlan.next = instruction_act.actions;
	instruction_act.actions = (gn_action_t *)&act_set_field_vlan;

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
}
void fabric_openstack_floating_ip_install_set_vlan_in_flow(gn_switch_t * sw, UINT4 match_ip, UINT4 mod_dst_ip, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port)
{
	//printf("%s : sw ip is %s; vlan_id is %d \n", FN, inet_ntoa(*(struct in_addr*)&sw->sw_ip), vlan_id);
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction_act;
	gn_action_set_field_t act_set_field_mac;
	gn_action_set_field_t act_set_field_ip;
	gn_action_set_field_t act_set_field_vlan;
	gn_action_t act_pushVlan;
	gn_instruction_goto_table_t instruction_goto;
	gn_action_output_t output_action;

	memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_ARP_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_FLOATING_FLOW;
	flow.table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_INPUT_TABLE;
	flow.match.type = OFPMT_OXM;
	flow.match.oxm_fields.eth_type = ETHER_IP;
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
	flow.match.oxm_fields.ipv4_dst = ntohl(match_ip);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);

	
	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
	instruction_act.type = OFPIT_APPLY_ACTIONS;
	instruction_act.next = flow.instructions;
	flow.instructions = (gn_instruction_t *)&instruction_act;

	if (0 == get_nat_physical_switch_flag()) {
		memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
		instruction_goto.type = OFPIT_GOTO_TABLE;
		instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
		instruction_goto.next = flow.instructions;
		flow.instructions = (gn_instruction_t *)&instruction_goto;
	}
	else {
		memset(&output_action, 0, sizeof(gn_action_output_t));
		output_action.port = out_port;
		output_action.type = OFPAT13_OUTPUT;
		output_action.max_len = 0xffff;
		output_action.next = instruction_act.actions;
		instruction_act.actions = (gn_action_t *)&output_action;
	}

	memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
	act_set_field_mac.type = OFPAT13_SET_FIELD;
	memcpy(act_set_field_mac.oxm_fields.eth_dst, mod_dst_mac, 6);
	act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
	act_set_field_mac.next = instruction_act.actions;
	instruction_act.actions = (gn_action_t *)&act_set_field_mac;

	memset(&act_set_field_ip, 0, sizeof(gn_action_set_field_t));
	act_set_field_ip.type = OFPAT13_SET_FIELD;
	act_set_field_ip.oxm_fields.ipv4_dst = ntohl(mod_dst_ip);
	act_set_field_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
	act_set_field_ip.next = instruction_act.actions;
	instruction_act.actions = (gn_action_t *)&act_set_field_ip;

	memset(&act_set_field_vlan, 0, sizeof(gn_action_set_field_t));
	act_set_field_vlan.type = OFPAT13_SET_FIELD;
	act_set_field_vlan.oxm_fields.vlan_vid = vlan_id;
	act_set_field_vlan.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
	act_set_field_vlan.next = instruction_act.actions;
	instruction_act.actions = (gn_action_t *)&act_set_field_vlan;

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
}
void fabric_openstack_floating_ip_install_table_3_flow(gn_switch_t * sw, UINT1* match_mac, UINT4 out_port)
{
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_instruction_actions_t instruction_act;
	gn_action_output_t output_action;

	memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = FABRIC_FIND_HOST_IDLE_TIME_OUT;
	flow.hard_timeout = FABRIC_ARP_HARD_TIME_OUT;
	flow.priority = FABRIC_PRIORITY_FLOATING_FLOW;
	flow.table_id = FABRIC_OUTPUT_TABLE;
	flow.match.type = OFPMT_OXM;
	memcpy(flow.match.oxm_fields.eth_dst, match_mac, 6);
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);

	memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
	instruction_act.type = OFPIT_APPLY_ACTIONS;
	instruction_act.next = flow.instructions;
	flow.instructions = (gn_instruction_t *)&instruction_act;

	memset(&output_action, 0, sizeof(gn_action_output_t));
	output_action.port = out_port;
	output_action.type = OFPAT13_OUTPUT;
	output_action.max_len = 0xffff;
	output_action.next = instruction_act.actions;
	instruction_act.actions = (gn_action_t *)&output_action;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
}

void fabric_openstack_floating_ip_packet_out_handle(openstack_port_p src_port, packet_in_info_t *packet_in, external_floating_ip_p fip)
{
	ip_t *ip = (ip_t *)(packet_in->data);
	//printf("%s\n", FN);
	//printf("source ip  ");
	//fabric_openstack_show_ip(ip->src);
	//printf("destination ip  ");
	//fabric_openstack_show_ip(ip->dest);
	//printf("source mac  ");
	//fabric_openstack_show_mac(ip->eth_head.src);
	//printf("destination mac  ");
	//fabric_openstack_show_mac(ip->eth_head.dest);
	//printf("\n");
	external_port_p ext_port = NULL;

	ext_port = get_external_port_by_floatip(fip->floating_ip);

	if(src_port == NULL || ext_port == NULL)
	{
		return;
	}

	//write flow table
	//packet out rule
	gn_switch_t * switch_gw = find_sw_by_dpid(ext_port->external_dpid);
	UINT4 vlan_id = of131_fabric_impl_get_tag_sw(switch_gw);
	//printf("%s packet out rule : sw ip is %s ;vlan id is %d \n", FN, inet_ntoa(*(struct in_addr*)&switch_gw->sw_ip), vlan_id);
	fabric_openstack_floating_ip_install_set_vlan_out_flow(src_port->sw, ip->dest, src_port->mac, fip->floating_ip, ext_port->external_gateway_mac, vlan_id);

	//response rule
	vlan_id = of131_fabric_impl_get_tag_sw(src_port->sw);
	//printf("%s response rule : sw ip is %s ;vlan id is %d \n", FN, inet_ntoa(*(struct in_addr*)&src_port->sw->sw_ip), vlan_id);

	UINT4 out_port = get_out_port_between_switch(ext_port->external_dpid, src_port->sw->dpid);
	if (0 != out_port) {
		fabric_openstack_floating_ip_install_set_vlan_in_flow(switch_gw, fip->floating_ip, ip->src, ip->eth_head.src, vlan_id, out_port);
	}
	//write table 3
	fabric_openstack_floating_ip_install_table_3_flow(src_port->sw, src_port->mac, packet_in->inport);

	//pack back
	fabric_openstack_packet_output(src_port->sw, packet_in, OFPP13_TABLE);

}
void fabric_openstack_floating_ip_packet_in_handle(external_port_p epp, packet_in_info_t *packet_in, external_floating_ip_p fip)
{
	ip_t *ip = (ip_t *)(packet_in->data);
	//printf("%s\n", FN);
	/*
	//printf("source ip  ");
	fabric_openstack_show_ip(ip->src);
	//printf("destination ip  ");
	fabric_openstack_show_ip(ip->dest);
	printf("source mac  ");
	fabric_openstack_show_mac(ip->eth_head.src);
	printf("destination mac  ");
	fabric_openstack_show_mac(ip->eth_head.dest);
	*/
	//printf("\n");
	//printf("sw ip is ");
	// fabric_openstack_show_ip(fip->floating_ip);

	openstack_port_p dst_port = NULL;
	gn_switch_t * sww = find_sw_by_dpid(epp->external_dpid);
	dst_port = find_openstack_host_port_by_mac(ip->eth_head.dest);
	//printf("dest ip is ");
	// fabric_openstack_show_ip(dst_port->ip);
	if(dst_port == NULL || epp == NULL)
	{
		return ;
	}

	if  (dst_port->sw == NULL) {
		//printf("fabric_openstack_floating_ip_packet_in_handle dst_port : %d | dst_port->sw : %d | ext_port : %d\n",(int)dst_port, (int)dst_port->sw, epp->external_dpid);

		openstack_port_p gateway_p = find_openstack_app_gateway_by_host(dst_port);

		if (NULL != gateway_p) {
			fabric_opnestack_floating_flood_inside(gateway_p->ip, dst_port->ip, gateway_p->mac, epp->external_dpid);
		}
		else {
			LOG_PROC("ERROR", "Can't find gateway of dest ip");
		}

		// fabric_openstack_packet_output(sww, packet_in, OFPP13_TABLE);
		return ;
	}

	//packet in rule
	UINT4 vlan_id = of131_fabric_impl_get_tag_sw(dst_port->sw);
	//printf("%s packet in rule : sw ip is %s ;vlan id is %d \n", FN, inet_ntoa(*(struct in_addr*)&dst_port->sw->sw_ip), vlan_id);
	UINT4 out_port = get_out_port_between_switch(epp->external_dpid, dst_port->sw->dpid);
	if (0 != out_port) {
		fabric_openstack_floating_ip_install_set_vlan_in_flow(sww, ip->dest, dst_port->ip, dst_port->mac, vlan_id, out_port);
	}
	else {
		// start flood
		openstack_port_p gateway_p = find_openstack_app_gateway_by_host(dst_port);

		if (NULL != gateway_p) {
			fabric_opnestack_floating_flood_inside(gateway_p->ip, dst_port->ip, gateway_p->mac, epp->external_dpid);
		}
		else {
			LOG_PROC("ERROR", "Can't find gateway of dest ip");
		}
		return;
	}
	//write table 3
	fabric_openstack_floating_ip_install_table_3_flow(dst_port->sw, dst_port->mac, dst_port->port);

	//response rule
	vlan_id = of131_fabric_impl_get_tag_sw(sww);
	//printf("%s response rule : sw ip is %s ; vlan id is %d \n", FN, inet_ntoa(*(struct in_addr*)&sww->sw_ip), vlan_id);
	fabric_openstack_floating_ip_install_set_vlan_out_flow(dst_port->sw, ip->src, dst_port->mac, fip->floating_ip, epp->external_gateway_mac, vlan_id);

	//pack back
	fabric_openstack_packet_output(sww, packet_in, OFPP13_TABLE);
}

void fabric_openstack_floating_ip_arp_request_handle(gn_switch_t *sw, external_floating_ip_p fip, packet_in_info_t *packet_in)
{
	arp_t *arp = (arp_t *)(packet_in->data);
	//printf("%s\n", FN);
	//printf("source ip  ");
	//fabric_openstack_show_ip(arp->sendip);
	//printf("destination ip  ");
	//fabric_openstack_show_ip(arp->targetip);
	//printf("source mac  ");
	//fabric_openstack_show_mac(arp->sendmac);
	//printf("destination mac  ");
	//fabric_openstack_show_mac(arp->targetmac);
	openstack_port_p float_port = find_openstack_host_port_by_port_id(fip->port_id);
	if(float_port != NULL)
	{
		openstack_port_p dst_port = find_openstack_host_port_by_port_id(float_port->port_id);

		// get external port
		external_port_p ext_port = get_external_port_by_floatip(fip->floating_ip);
		if (NULL == ext_port) {
			LOG_PROC("INFO", "Floating IP: external port is NULL!");
			return;
		}

		// get external sw
		gn_switch_t * ext_sw = NULL;
		ext_sw = find_sw_by_dpid(ext_port->external_dpid);
		if (NULL == ext_sw) {
			LOG_PROC("INFO", "Floating IP: gateway sw is NULL!");
			return;
		}

		if(dst_port->sw != NULL)
		{
			//printf("%s packet out \n", FN);
			fabric_openstack_create_arp_reply_public(float_port->mac, arp->targetip, arp->sendmac, \
					arp->sendip, ext_sw, ext_port->external_port, packet_in);
		}else
		{
			//printf("%s : arp target ip is %s ", FN, inet_ntoa(*(struct in_addr*)&arp->targetip));
			//printf(";fip fixed ip is %s \n", inet_ntoa(*(struct in_addr*)&fip->fixed_ip));
			arp->targetip = fip->fixed_ip;
			fabric_openstack_packet_flood_inside(packet_in, ext_port->external_dpid);
			//printf("%s flood \n", FN);
		}
	}
}


void fabric_openstack_floating_ip_arp_reply_handle(gn_switch_t *sw, external_floating_ip_p fip, packet_in_info_t *packet_in)
{
	external_port_p ext_port = get_external_port_by_floatip(fip->floating_ip);
	arp_t *arp = (arp_t *)(packet_in->data);
	//printf("%s\n", FN);
	//printf("source ip  ");
	//fabric_openstack_show_ip(arp->sendip);
	//printf("destination ip  ");
	//fabric_openstack_show_ip(arp->targetip);
	//printf("source mac  ");
	//fabric_openstack_show_mac(arp->sendmac);
	//printf("destination mac  ");
	//fabric_openstack_show_mac(arp->targetmac);
	if(ext_port != NULL)
	{
		gn_switch_t * dest_sw = find_sw_by_dpid(ext_port->external_dpid);
		if(dest_sw != NULL)
		{
			arp->sendip = fip->floating_ip;
			memcpy(arp->targetmac, ext_port->external_gateway_mac, 6);
			fabric_openstack_packet_output(dest_sw, packet_in, ext_port->external_port);
		}
	}
}

void fabric_opnestack_floating_flood_inside(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT8 ext_dpid)
{
	LOG_PROC("INFO", "External: Can't find the dest: start flood!");
	packout_req_info_t packout_req_info;
	arp_t new_arp_pkt;

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.max_len = 0xff;
	packout_req_info.xid = 0;
	packout_req_info.data_len = sizeof(arp_t);
	packout_req_info.data = (UINT1 *)&new_arp_pkt;

	memcpy(new_arp_pkt.eth_head.src, src_mac, 6);
	memcpy(new_arp_pkt.eth_head.dest, nat_broadcat_mac, 6);
	new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
	new_arp_pkt.hardwaretype = htons(1);
	new_arp_pkt.prototype = htons(ETHER_IP);
	new_arp_pkt.hardwaresize = 0x6;
	new_arp_pkt.protocolsize = 0x4;
	new_arp_pkt.opcode = htons(1);
	new_arp_pkt.sendip = src_ip;
	new_arp_pkt.targetip=dst_ip;

	memcpy(new_arp_pkt.sendmac, src_mac, 6);
	memcpy(new_arp_pkt.targetmac, nat_zero_mac, 6);

	gn_switch_t *sw = NULL;
	UINT2 i = 0,j=0;

	for(i = 0; i < g_server.max_switch; i++) {
		if (g_server.switches[i].state) {
			sw = &g_server.switches[i];
			if (sw->dpid == ext_dpid) {
				continue;
			}

			for(j=0; j<sw->n_ports; j++){
				// check port state is ok and also not connect other switch(neighbor)
				if(sw->neighbor[j] == NULL){
					packout_req_info.outport = sw->ports[j].port_no;
					sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
				}
			}
		}
	}
}

void fabric_openstack_packet_flood_inside(packet_in_info_t *packet_in_info, UINT8 ext_dpid)
{
	LOG_PROC("INFO", "External: Can't find the dest: start flood!");
	packout_req_info_t pakout_req;
	gn_switch_t *sw = NULL;
	UINT2 i = 0,j=0;
	pakout_req.buffer_id = 0xffffffff;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;

	// find all switch
	for(i = 0; i < g_server.max_switch; i++){
		if (g_server.switches[i].state){
			sw = &g_server.switches[i];
//			sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
			// find switch's outter ports

			if (sw->dpid == ext_dpid) {
				continue;
			}

			for(j=0; j<sw->n_ports; j++){
				// check port state is ok and also not connect other switch(neighbor)
				if(sw->neighbor[j] == NULL){
					pakout_req.outport = sw->ports[j].port_no;
					sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
				}
			}
		}
	}
	return;
};

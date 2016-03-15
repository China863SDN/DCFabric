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
#include "openstack_app.h"
#include "fabric_openstack_arp.h"


// const UINT1 nat_zero_mac[] = {0x0,0x0,0x0,0x0,0x0,0x0};
// const UINT1 nat_broadcat_mac[] = {0xff,0xff,0xff,0xff,0xff,0xff};

// extern void fabric_openstack_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport);


INT4 fabric_openstack_floating_ip_packet_out_handle(p_fabric_host_node src_port, packet_in_info_t *packet_in, external_floating_ip_p fip, param_set_p param_set)
{
	// printf("%s\n", FN);
	ip_t *ip = (ip_t *)(packet_in->data);

	external_port_p ext_port = NULL;
	ext_port = get_external_port_by_floatip(fip->floating_ip);

	if ((src_port == NULL || ext_port == NULL) || (src_port->sw == NULL))
	{
		return IP_DROP;
	}

	//write flow table
	//packet out rule
	gn_switch_t * switch_gw = find_sw_by_dpid(ext_port->external_dpid);
	if (NULL == switch_gw) {
		LOG_PROC("INFO", "Floating: External switch is NULL!");
		return IP_DROP;
	}

	UINT4 vlan_id = of131_fabric_impl_get_tag_sw(switch_gw);
	param_set->src_sw = src_port->sw;
	param_set->dst_ip = ip->dest;
	memcpy(param_set->src_mac, src_port->mac, 6);
	param_set->mod_src_ip = fip->floating_ip;
	memcpy(param_set->dst_gateway_mac, ext_port->external_gateway_mac, 6);
	param_set->src_vlanid = vlan_id;

	//response rule
	vlan_id = of131_fabric_impl_get_tag_sw(src_port->sw);

	UINT4 out_port = get_out_port_between_switch(ext_port->external_dpid, src_port->sw->dpid);
	if (0 != out_port) {
		//fabric_openstack_floating_ip_install_set_vlan_in_flow(switch_gw, fip->floating_ip, ip->src, ip->eth_head.src, vlan_id, out_port);
		param_set->dst_sw = switch_gw;
		param_set->src_ip = ip->src;
		memcpy(param_set->packet_src_mac, ip->eth_head.src, 6);
		param_set->dst_vlanid = vlan_id;
		param_set->dst_inport = out_port;
		param_set->src_inport = packet_in->inport;

		return Floating_ip_flow;
	}

	return IP_DROP;
}
//
//void fabric_openstack_floating_ip_packet_in_handle(external_port_p epp, packet_in_info_t *packet_in, external_floating_ip_p fip)
//{
//	ip_t *ip = (ip_t *)(packet_in->data);
//	//printf("%s\n", FN);
//	/*
//	//printf("source ip  ");
//	fabric_openstack_show_ip(ip->src);
//	//printf("destination ip  ");
//	fabric_openstack_show_ip(ip->dest);
//	printf("source mac  ");
//	fabric_openstack_show_mac(ip->eth_head.src);
//	printf("destination mac  ");
//	fabric_openstack_show_mac(ip->eth_head.dest);
//	*/
//	//printf("\n");
//	//printf("sw ip is ");
//	// fabric_openstack_show_ip(fip->floating_ip);
//
//	p_fabric_host_node dst_port = NULL;
//	gn_switch_t * sww = find_sw_by_dpid(epp->external_dpid);
//	dst_port = get_fabric_host_from_list_by_mac(ip->eth_head.dest);
//	//printf("dest ip is ");
//	// fabric_openstack_show_ip(dst_port->ip);
//	if(dst_port == NULL || epp == NULL)
//	{
//		return ;
//	}
//
//	if  (dst_port->sw == NULL) {
//		//printf("fabric_openstack_floating_ip_packet_in_handle dst_port : %d | dst_port->sw : %d | ext_port : %d\n",(int)dst_port, (int)dst_port->sw, epp->external_dpid);
//
//		p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(dst_port);
//
//		if (NULL != gateway_p) {
//			fabric_opnestack_floating_flood_inside(gateway_p->ip_list[0], dst_port->ip_list[0], gateway_p->mac, epp->external_dpid);
//		}
//		else {
//			LOG_PROC("ERROR", "Can't find gateway of dest ip");
//		}
//
//		// fabric_openstack_packet_output(sww, packet_in, OFPP13_TABLE);
//		return ;
//	}
//
//	//packet in rule
//	UINT4 vlan_id = of131_fabric_impl_get_tag_sw(dst_port->sw);
//	//printf("%s packet in rule : sw ip is %s ;vlan id is %d \n", FN, inet_ntoa(*(struct in_addr*)&dst_port->sw->sw_ip), vlan_id);
//	UINT4 out_port = get_out_port_between_switch(epp->external_dpid, dst_port->sw->dpid);
//	if (0 != out_port) {
//		fabric_openstack_floating_ip_install_set_vlan_in_flow(sww, ip->dest, dst_port->ip_list[0], dst_port->mac, vlan_id, out_port);
//	}
//	else {
//		// start flood
//		p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(dst_port);
//
//		if (NULL != gateway_p) {
//			fabric_opnestack_floating_flood_inside(gateway_p->ip_list[0], dst_port->ip_list[0], gateway_p->mac, epp->external_dpid);
//		}
//		else {
//			LOG_PROC("ERROR", "Can't find gateway of dest ip");
//		}
//		return;
//	}
//	//write table 3
//	install_fabric_output_flow(dst_port->sw, dst_port->mac, dst_port->port);
//
//	//response rule
//	vlan_id = of131_fabric_impl_get_tag_sw(sww);
//	//printf("%s response rule : sw ip is %s ; vlan id is %d \n", FN, inet_ntoa(*(struct in_addr*)&sww->sw_ip), vlan_id);
//	fabric_openstack_floating_ip_install_set_vlan_out_flow(dst_port->sw, ip->src, dst_port->mac, fip->floating_ip, epp->external_gateway_mac, vlan_id);
//
//	//pack back
//	fabric_openstack_packet_output(sww, packet_in, OFPP13_TABLE);
//}

//void fabric_openstack_floating_ip_arp_request_handle(gn_switch_t *sw, external_floating_ip_p fip, packet_in_info_t *packet_in)
//{
//	arp_t *arp = (arp_t *)(packet_in->data);
//	//printf("%s\n", FN);
//	//printf("source ip  ");
//	//fabric_openstack_show_ip(arp->sendip);
//	//printf("destination ip  ");
//	//fabric_openstack_show_ip(arp->targetip);
//	//printf("source mac  ");
//	//fabric_openstack_show_mac(arp->sendmac);
//	//printf("destination mac  ");
//	//fabric_openstack_show_mac(arp->targetmac);
//	p_fabric_host_node float_port = find_fabric_host_port_by_port_id(fip->port_id);
//	if(float_port != NULL)
//	{
//		p_fabric_host_node dst_port = float_port;
//
//		// get external port
//		external_port_p ext_port = get_external_port_by_floatip(fip->floating_ip);
//		if (NULL == ext_port) {
//			LOG_PROC("INFO", "Floating IP: external port is NULL!");
//			return;
//		}
//
//		// get external sw
//		gn_switch_t * ext_sw = NULL;
//		ext_sw = find_sw_by_dpid(ext_port->external_dpid);
//		if (NULL == ext_sw) {
//			LOG_PROC("INFO", "Floating IP: gateway sw is NULL!");
//			return;
//		}
//
//		if(dst_port->sw != NULL)
//		{
//			//printf("%s packet out \n", FN);
//			fabric_openstack_create_arp_reply_public(float_port->mac, arp->targetip, arp->sendmac,
//					arp->sendip, ext_sw, ext_port->external_port, packet_in);
//		}else
//		{
//			//printf("%s : arp target ip is %s ", FN, inet_ntoa(*(struct in_addr*)&arp->targetip));
//			//printf(";fip fixed ip is %s \n", inet_ntoa(*(struct in_addr*)&fip->fixed_ip));
//			arp->targetip = fip->fixed_ip;
//			fabric_openstack_packet_flood_inside(packet_in, ext_port->external_dpid);
//			//printf("%s flood \n", FN);
//		}
//	}
//}



//void fabric_openstack_floating_ip_arp_reply_handle(gn_switch_t *sw, external_floating_ip_p fip, packet_in_info_t *packet_in)
//{
//	external_port_p ext_port = get_external_port_by_floatip(fip->floating_ip);
//	arp_t *arp = (arp_t *)(packet_in->data);
//	//printf("%s\n", FN);
//	//printf("source ip  ");
//	//fabric_openstack_show_ip(arp->sendip);
//	//printf("destination ip  ");
//	//fabric_openstack_show_ip(arp->targetip);
//	//printf("source mac  ");
//	//fabric_openstack_show_mac(arp->sendmac);
//	//printf("destination mac  ");
//	//fabric_openstack_show_mac(arp->targetmac);
//	if(ext_port != NULL)
//	{
//		gn_switch_t * dest_sw = find_sw_by_dpid(ext_port->external_dpid);
//		if(dest_sw != NULL)
//		{
//			arp->sendip = fip->floating_ip;
//			memcpy(arp->targetmac, ext_port->external_gateway_mac, 6);
//			fabric_openstack_packet_output(dest_sw, packet_in, ext_port->external_port);
//		}
//	}
//}
//
//void fabric_opnestack_floating_flood_inside(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT8 ext_dpid)
//{
//	// LOG_PROC("INFO", "External: Can't find the dest: start flood!");
//	packout_req_info_t packout_req_info;
//	arp_t new_arp_pkt;
//
//	packout_req_info.buffer_id = 0xffffffff;
//	packout_req_info.inport = OFPP13_CONTROLLER;
//	packout_req_info.max_len = 0xff;
//	packout_req_info.xid = 0;
//	packout_req_info.data_len = sizeof(arp_t);
//	packout_req_info.data = (UINT1 *)&new_arp_pkt;
//
//	memcpy(new_arp_pkt.eth_head.src, src_mac, 6);
//	memcpy(new_arp_pkt.eth_head.dest, nat_broadcat_mac, 6);
//	new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
//	new_arp_pkt.hardwaretype = htons(1);
//	new_arp_pkt.prototype = htons(ETHER_IP);
//	new_arp_pkt.hardwaresize = 0x6;
//	new_arp_pkt.protocolsize = 0x4;
//	new_arp_pkt.opcode = htons(1);
//	new_arp_pkt.sendip = src_ip;
//	new_arp_pkt.targetip=dst_ip;
//
//	memcpy(new_arp_pkt.sendmac, src_mac, 6);
//	memcpy(new_arp_pkt.targetmac, nat_zero_mac, 6);
//
//	gn_switch_t *sw = NULL;
//	UINT2 i = 0,j=0;
//
//	for(i = 0; i < g_server.max_switch; i++) {
//		if (g_server.switches[i].state) {
//			sw = &g_server.switches[i];
//			if (sw->dpid == ext_dpid) {
//				continue;
//			}
//
//			for(j=0; j<sw->n_ports; j++){
//				// check port state is ok and also not connect other switch(neighbor)
//				if(sw->neighbor[j] == NULL){
//					packout_req_info.outport = sw->ports[j].port_no;
//					sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
//				}
//			}
//		}
//	}
//}
//
//void fabric_openstack_packet_flood_inside(packet_in_info_t *packet_in_info, UINT8 ext_dpid)
//{
//	LOG_PROC("INFO", "External: Can't find the dest: start flood!");
//	packout_req_info_t pakout_req;
//	gn_switch_t *sw = NULL;
//	UINT2 i = 0,j=0;
//	pakout_req.buffer_id = 0xffffffff;
//	pakout_req.inport = OFPP13_CONTROLLER;
//	pakout_req.max_len = 0xff;
//	pakout_req.xid = packet_in_info->xid;
//	pakout_req.data_len = packet_in_info->data_len;
//	pakout_req.data = packet_in_info->data;
//
//	// find all switch
//	for(i = 0; i < g_server.max_switch; i++){
//		if (g_server.switches[i].state){
//			sw = &g_server.switches[i];
////			sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
//			// find switch's outter ports
//
//			if (sw->dpid == ext_dpid) {
//				continue;
//			}
//
//			for(j=0; j<sw->n_ports; j++){
//				// check port state is ok and also not connect other switch(neighbor)
//				if(sw->neighbor[j] == NULL){
//					pakout_req.outport = sw->ports[j].port_no;
//					sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
//				}
//			}
//		}
//	}
//	return;
//};


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
 * fabric_openstack_arp.c
 *
 *  Created on: Jun 19, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */

#include "fabric_openstack_arp.h"
#include "fabric_flows.h"
#include "fabric_impl.h"
#include "openstack_app.h"
#include "gn_inet.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "common.h"
#include "fabric_floating_ip.h"
#include "fabric_openstack_nat.h"
#include "fabric_openstack_external.h"
#include "../conn-svr/conn-svr.h"
#include "fabric_arp.h"
#include "openstack_security_app.h"
#include "openstack_lbaas_app.h"

UINT4 g_openstack_dns_ip = 0x8080808;
const UINT1 arp_zero_mac[] = {0x0,0x0,0x0,0x0,0x0,0x0};
const UINT1 arp_broadcat_mac[] = {0xff,0xff,0xff,0xff,0xff,0xff};

extern UINT4 g_openstack_on;
extern UINT4 g_proactive_flow_flag;
/*****************************
 * local function
 *****************************/
UINT1 g_broad_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
UINT4 g_broad_ip = -1;
//void fabric_openstack_arp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
//void fabric_openstack_arp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
void fabric_openstack_ip_broadcast_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,p_fabric_host_node src_port);
// void fabric_openstack_dhcp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,udp_t* udp,p_fabric_host_node src_port);
// void fabric_openstack_dhcp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,udp_t* udp,p_fabric_host_node src_port);
//UINT1 fabric_openstack_stand_handle();
void fabric_openstack_dhcp_request_handle(packet_in_info_t *packet_in, p_fabric_host_node src_port);
void fabric_openstack_dhcp_reply_handle(packet_in_info_t *packet_in, p_fabric_host_node dst_port);
void fabric_openstack_install_fabric_flows(p_fabric_host_node src_port,p_fabric_host_node dst_port,
										   security_param_p src_security, security_param_p dst_security);
void fabric_openstack_install_fabric_out_subnet_flows(p_fabric_host_node src_port,p_fabric_host_node src_gateway,
		p_fabric_host_node dst_port,p_fabric_host_node dst_gateway, security_param_p src_security, security_param_p dst_security);
int check_fabric_openstack_subnet_dhcp_gateway(p_fabric_host_node port,openstack_subnet_p subnet);
INT4 create_arp_flood_parameter(UINT4 dst_ip, p_fabric_host_node dst_port, param_set_p param);
/*****************************
 * local function : packet out
 *****************************/

void fabric_openstack_packet_flood(packet_in_info_t *packet_in_info);
void fabric_openstack_packet_flood_in_subnet(packet_in_info_t *packet_in_info,char* subnet_id,UINT4 subnet_port_num);
void fabric_openstack_create_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in_info);
void fabric_openstack_create_arp_reply_public(UINT1* srcMac, UINT4 srcIP, UINT1* dstMac, UINT4 dstIP, gn_switch_t* sw,
											UINT4 outPort, packet_in_info_t *packet_in_info);
void fabric_openstack_external_arp_mac();
gn_switch_t* get_ext_sw_by_dpid(UINT8 dpid);

INT4 external_packet_out_compute_forward(p_fabric_host_node src_port,UINT4 sendip, UINT4 targetip, packet_in_info_t *packet_in, UINT1 proto, param_set_p param_set);
INT4 external_packet_in_compute_forward(p_fabric_host_node src_port, UINT4 src_ip, UINT4 targetip, packet_in_info_t* packet_in, UINT1 proto, param_set_p param_set);
INT4 internal_packet_compute_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip, param_set_p param_set, ip_t *ip);
void remove_host_output_flow_by_ip_mac(gn_switch_t* sw, UINT4 ip, UINT1* mac);
void remove_floating_flow(gn_switch_t* sw, UINT4 floating_ip, UINT1* mac);
void remove_nat_flow(gn_switch_t* sw, UINT4 ip, UINT1* src_mac);
INT4 internal_packet_compute_vip_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip, param_set_p param_set, ip_t *ip);
INT4 openstack_check_src_dst_is_controller(packet_in_info_t *packet_in);

void fabric_openstack_show_port(p_fabric_host_node port){
	struct in_addr addr;
	char temp[16] = {0};
	memcpy(&addr, &port->ip_list[0], 4);
	mac2str(port->mac, temp);
	openstack_port_p port_p = (openstack_port_p)port->data;
	LOG_PROC("INFO","Tenant: %s | Network: %s | Subnet: %s  | Port: %s | IP: %s  | MAC: %s  |\n",port_p->tenant_id,port_p->network_id,port_p->subnet_id,port_p->port_id,inet_ntoa(addr),temp);

	return;
}

void fabric_openstack_show_ip(UINT4 ip){
	struct in_addr addr;
	memcpy(&addr, &ip, 4);
	LOG_PROC("INFO","IP: %s  |",inet_ntoa(addr));
	return;
}
void fabric_openstack_show_mac(UINT1* mac){
	char temp[16] = {0};
	mac2str(mac, temp);
	LOG_PROC("INFO","MAC: %s  |",temp);
	return;
}
extern UINT4 g_openstack_fobidden_ip;
/*****************************
 * global variables
 *****************************/
//void fabric_openstack_arp_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
//	arp_t *arp = (arp_t *)(packet_in->data);
//
//	//printf("%s\n", FN);
//	//printf("arp source ip  ");
//	//fabric_openstack_show_ip(arp->sendip);
//	//printf("arp destination ip  ");
//	//fabric_openstack_show_ip(arp->targetip);
//	//printf("arp source mac  ");
//	//fabric_openstack_show_mac(arp->eth_head.src);
//	//printf("arp destination mac  ");
//	//fabric_openstack_show_mac(arp->eth_head.dest);
//	//printf("\n");
//
//
//
//	if(arp->opcode == htons(1)){
//		//fabric_openstack_arp_request_handle(sw,packet_in);
//	}else{
//		//fabric_openstack_arp_reply_handle(sw,packet_in);
//	}
//	return;
//};
//
//void fabric_openstack_ip_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
//	p_fabric_host_node src_port = NULL;
//	p_fabric_host_node dst_port = NULL;
//	p_fabric_host_node src_gateway = NULL;
//	p_fabric_host_node dst_gateway = NULL;
//	openstack_subnet_p src_subnet = NULL;
//	openstack_subnet_p dst_subnet = NULL;
//	external_floating_ip_p fip = NULL;
//
//	ip_t *ip = (ip_t *)(packet_in->data);
//	//printf("%s\n", FN);
//	//printf("packet in sw ip ");
//	//fabric_openstack_show_ip(sw->sw_ip);
//	//printf("source ip  ");
//	//fabric_openstack_show_ip(ip->src);
//	//printf("destination ip  ");
//	//fabric_openstack_show_ip(ip->dest);
//	//printf("source mac  ");
//	//fabric_openstack_show_mac(ip->eth_head.src);
//	//printf("destination mac  ");
//	//fabric_openstack_show_mac(ip->eth_head.dest);
//	//printf("\n");
//
//	// find the source host
//	src_port = get_fabric_host_from_list_by_mac(ip->eth_head.src);
//	if(src_port == NULL){
//		fip = get_external_floating_ip_by_floating_ip(ip->dest);
//		external_port_p epp = get_external_port_by_floatip(ip->dest);
//		if(fip != NULL && epp!=NULL)
//		{
//			fabric_openstack_floating_ip_packet_in_handle(epp, packet_in, fip);
//		}
//		else /*if (NULL != epp)*/
//		{
//			fabric_openstack_ip_nat_handle(sw, packet_in, FALSE);
//		}
//
//		return;
//	}
//
//	// update
//	src_port->ip_list[0] = ip->src;
//	src_port->port = packet_in->inport;
//	src_port->sw = sw;
//	openstack_port_p src_port_p = (openstack_port_p)src_port->data;
//	// printf("%s update sw(%s) to ", FN, inet_ntoa(*(struct in_addr*)&sw->sw_ip));
//	// printf(" ip(%s); port is %d\n", inet_ntoa(*(struct in_addr*)&ip->src), packet_in->inport);
//	// is broadcast?
//	if(/*0 ==memcmp(g_broad_mac,ip->eth_head.dest,6) || */g_broad_ip == ip->dest){
//		fabric_openstack_ip_broadcast_handle(sw,packet_in,ip,src_port);
//		return;
//	}
//
//
//	// get subnet
//	if (NULL != src_port_p) {
//		src_subnet = find_openstack_app_subnet_by_subnet_id(src_port_p->subnet_id);
//	}
//	// if g_openstack_fobidden_ip
//	if(ip->dest == g_openstack_fobidden_ip){
//		LOG_PROC("INFO","IP Handle :  IP 169.254.169.254! IP_DROP!");
////		LOG_PROC("INFO","IP Handle :  IP 169.254.169.254! TO DCHP SERVER!");
////		dst_port = src_subnet->dhcp_port;
////		//change mac (out gateway)
////		memcpy(ip->eth_head.dest,dst_port->mac,6);
////
////		if(dst_port != NULL && dst_port->sw != NULL && dst_port->port != 0){
////			LOG_PROC("INFO","IP Handle :  IP 169.254.169.254! FOUND: SETUP FLOWS & PACKET OUT!");
////			fabric_openstack_install_fabric_flows(src_port,dst_port);
////			fabric_openstack_install_fabric_out_subnet_flows(dst_port->sw,packet_in,dst_port->port);
////		}else{
////			LOG_PROC("INFO","IP Handle :  IP 169.254.169.254! NOT FOUND:FLOOD!");
////			fabric_openstack_packet_flood(packet_in);
////		}
//		return;
//	}
//
//	// find dst_port ? if not, openstack has not this port
//	dst_port = get_fabric_host_from_list_by_mac(ip->eth_head.dest);
//
//	if(dst_port == NULL){
//		LOG_PROC("INFO","IP Handle : Can't find destination host!");
//		return;
//	}
//	openstack_port_p dst_port_p = (openstack_port_p)dst_port->data;
//	// dst_port is gateway?
//	if ((NULL != src_subnet) && (dst_port->ip_list[0] == src_subnet->gateway_ip)) {
//		src_gateway = dst_port;
//		// find dst_port by ip
//		if (NULL != src_port_p) {
//			dst_port = find_fabric_host_port_by_tenant_id(ip->dest, src_port_p->tenant_id);
//		}
//
//		if ((NULL == dst_port) && (NULL != src_port_p)) {
//			dst_port = find_fabric_host_port_by_network_id(ip->dest,src_port_p->network_id);
//		}
//		if(NULL == dst_port){
//			//check ip is in openstack intranet or not.
//			fip = get_external_floating_ip_by_fixed_ip(ip->src);
//			if(fip != NULL)
//			{
//				fabric_openstack_floating_ip_packet_out_handle(src_port, packet_in, fip);
//			}else
//			{
//				if (ip->dest == g_openstack_dns_ip)
//				{
//					// printf("NAT: dns ip! do nothing!");
//					return ;
//				}
//				fabric_openstack_ip_nat_handle(sw, packet_in, TRUE);
//			}
//			return;
//		}
//
//		// if dst_port is source gateway packet out
//		if(	src_gateway->ip_list[0] == dst_port->ip_list[0]){
//			// packet out
//			LOG_PROC("INFO","IP Handle :  Destination is gate way! IP_DROP!");
//
////			if(src_gateway->sw != NULL && src_gateway->port != 0){
////				fabric_openstack_packet_output(src_gateway->sw,packet_in,src_gateway->port);
////			}else{
////				fabric_openstack_packet_flood(packet_in);
////			}
//			return;
//		}
//
//		if (NULL != dst_port_p) {
//			dst_gateway = find_openstack_app_gateway_by_subnet_id(dst_port_p->subnet_id);
//		}
//
//		// dst gateway is not found?
//		if(dst_gateway == NULL){
//			// IP_DROP flow
//			return;
//		}
//		//change mac
//		memcpy(ip->eth_head.dest,dst_port->mac,6);
//
//		if(dst_port->sw == NULL){
//			//flood
//			fabric_openstack_packet_flood(packet_in);
//		}else{
//			//setup flow
//			// fobidden setup flows if it's gateway & dhcp port
//			dst_subnet = find_openstack_app_subnet_by_subnet_id(dst_port_p->subnet_id);
//			if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,src_subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,dst_subnet)){
//				fabric_openstack_install_fabric_out_subnet_flows(src_port,src_gateway,dst_port,dst_gateway);
//			}
//			//packet out
//			fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
//		}
//	}
//	else if ((NULL != dst_port_p) && (NULL != src_port_p) && (0 == strcmp(dst_port_p->subnet_id,src_port_p->subnet_id))) {
//		if(dst_port->sw == NULL){
//			//flood
//			fabric_openstack_packet_flood(packet_in);
//		}else{
//			LOG_PROC("INFO","IP Handle : SETUP FLOWS & PACKET OUT!");
//
//			//setup flow
//			// fobidden setup flows if it's gateway & dhcp port
//			if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,src_subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,src_subnet)){
//				fabric_openstack_install_fabric_flows(src_port,dst_port);
//			}
//
//			// packet out
//			fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
//		}
//	}
//	return;
//};
//
//
p_fabric_host_node openstack_save_host_info(gn_switch_t *sw,UINT1* sendmac,UINT4 sendip,UINT4 inport){
	p_fabric_host_node p_node =  get_fabric_host_from_list_by_mac(sendmac);
	if(p_node!=NULL){
//		if(!check_IP_in_fabric_host(p_node,sendip))
//		{
//
//			add_fabric_host_ip(p_node,sendip);
//		}
	}else{
//		p_node = create_fabric_host_list_node(sw,inport,sendmac,sendip);
//		insert_fabric_host_into_list(p_node);
		return NULL;
	}

	/*
	 * temporary code write for internal instance transition.
	 */
	if ((NULL != p_node->sw) && (0 != p_node->port))
		return p_node;

	if (sendip) 
		p_node->ip_list[0] = sendip;
	p_node->port = inport;
	p_node->sw=sw;
	//please test if need install flow
	// install_fabric_output_flow(sw,sendmac,inport);
	return p_node;
}

p_fabric_host_node openstack_find_dst_port(p_fabric_host_node src_node,UINT4 targetip){
	p_fabric_host_node dst_port=NULL;
	if(src_node==NULL){
		external_floating_ip_p fip = find_external_floating_ip_by_floating_ip(targetip);
		if(fip != NULL)
		{
			dst_port = find_fabric_host_port_by_port_id(fip->port_id);
		}else{
			dst_port = get_fabric_host_from_list_by_ip(targetip);
		}
	}else{
		openstack_port_p src_port_p = (openstack_port_p)src_node->data;

		// find dst_port by ip (because mac maybe is the 00:00:00:00:00:00)
		if (NULL != src_port_p) {
			dst_port = find_fabric_host_port_by_subnet_id(targetip,src_port_p->subnet_id);
		}
	}
	return dst_port;
}

p_fabric_host_node openstack_find_ip_dst_port(p_fabric_host_node src_node,UINT4 targetip)
{
	p_fabric_host_node dst_port=NULL;
	// external_port_p ext_port=NULL;
	// openstack_port_p src_port_p = NULL;
	// openstack_port_p dst_port_p = NULL;
	// openstack_subnet_p src_subnet = NULL;
	//openstack_subnet_p dst_subnet = NULL;
	//p_fabric_host_node src_gateway = NULL;
	//p_fabric_host_node dst_gateway = NULL;
	//external_floating_ip_p fip = NULL;

	// find dst port
	if (src_node) 
		dst_port = find_openstack_host_by_srcport_ip(src_node, targetip);
	else 
		dst_port = get_fabric_host_from_list_by_ip(targetip);

//	if ((NULL != src_node) && (NULL != dst_port))
//	{
//		src_port_p = (openstack_port_p)src_node->data;
//		dst_port_p = (openstack_port_p)dst_port->data;
//
//		if (NULL != src_port_p) {
//			src_subnet = find_openstack_app_subnet_by_subnet_id(src_port_p->subnet_id);
//		}
//
//		// dst_port is gateway?
//		if ((NULL != src_subnet) && (dst_port->ip_list[0] == src_subnet->gateway_ip)) {
//
//			// find dst_port by ip
//			if (NULL != src_port_p) {
//				dst_port = find_fabric_host_port_by_tenant_id(targetip, src_port_p->tenant_id);
//			}
//
//			if ((NULL == dst_port) && (NULL != src_port_p)) {
//				dst_port = find_fabric_host_port_by_network_id(targetip,src_port_p->network_id);
//			}
//		}
//	}

	return dst_port;
}

INT4 openstack_arp_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in){
	if ((NULL != src_port) && (NULL == find_fabric_host_port_by_subnet_id(targetip,"0"))) {
		fabric_add_into_arp_request(src_port,sendip,targetip);
		// flood to outter ports
		fabric_push_arp_flood_queue(targetip,packet_in);
	}

	return GN_OK;
}
INT4 openstack_arp_remove_ip_from_flood_list(UINT4 sendip){
	p_fabric_arp_request_node temp_node = remove_fabric_arp_request_from_list_by_dstip(sendip);
	if(temp_node!=NULL){
		temp_node = delete_fabric_arp_request_list_node(temp_node);
	}
	return GN_OK;
}
INT4 openstack_ip_p_install_flow(param_set_p param_set, INT4 foward_type)
{
	// fabric_openstack_install_fabric_flows(src_port,dst_port);
	if (NULL == param_set) {
		return GN_OK;
	}

	// printf("*************foward type is%d\n", foward_type);

	if (Internal_port_flow == foward_type) {
		fabric_openstack_install_fabric_flows(param_set->src_port, param_set->dst_port,
				param_set->src_security, param_set->dst_security);
	}
	else if (Internal_out_subnet_flow == foward_type) {
		fabric_openstack_install_fabric_out_subnet_flows(param_set->src_port, param_set->src_gateway,
		        param_set->dst_port, param_set->dst_gateway, param_set->src_security, param_set->dst_security);
	}
	else if (Floating_ip_flow == foward_type) {
        if (0 == g_proactive_flow_flag) {
            fabric_openstack_floating_ip_install_set_vlan_out_flow(param_set->src_sw, param_set->dst_ip, param_set->src_mac,
                            param_set->mod_src_ip, param_set->dst_gateway_mac, param_set->src_vlanid, param_set->src_security);
            fabric_openstack_floating_ip_install_set_vlan_in_flow(param_set->dst_sw, param_set->mod_src_ip, param_set->src_ip,
                    param_set->packet_src_mac, param_set->dst_vlanid, param_set->dst_inport);
            install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);

        }
	}
	else if (Nat_ip_flow == foward_type) {
		install_fabric_nat_from_inside_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
				param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->dst_inport,param_set->src_sw, param_set->dst_sw, param_set->src_security);

		if (0 == get_nat_physical_switch_flag()) {
			install_fabric_nat_from_external_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
					param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->dst_inport, param_set->src_sw, param_set->dst_sw);

			install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);
		}
		else {
			install_fabric_nat_from_external_fabric_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
					param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->dst_inport, param_set->src_sw, param_set->dst_sw, param_set->dst_gateway_output);

			install_fabric_nat_from_external_fabric_host_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
					param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->src_inport, param_set->src_sw, param_set->dst_sw);
		}
	}
	else if (Internal_vip_flow == foward_type) {
		fabric_openstack_install_fabric_vip_flows(param_set->src_port, param_set->dst_port, param_set->proto, param_set->vip, param_set->vip_mac,
				param_set->src_gateway, param_set->dst_gateway, param_set->src_port_no, param_set->vip_tcp_port_no, param_set->src_security, param_set->dst_security);
	}
	else if (External_vip_flow == foward_type) {
         if (0 == g_proactive_flow_flag) {
    	    fabric_openstack_install_fabric_vip_out_flows(param_set->src_sw, param_set->src_ip, param_set->src_mac, param_set->dst_ip,
    				param_set->dst_mac, param_set->dst_port, param_set->proto, param_set->vip, param_set->vip_mac, param_set->src_gateway_mac,
    				param_set->dst_gateway_mac, param_set->src_port_no, param_set->vip_tcp_port_no, param_set->outer_gateway_mac,
    				param_set->src_security, param_set->dst_security);
            }
	}
	else if (Internal_floating_vip_flow == foward_type) {
		fabric_openstack_install_fabric_floaing_vip_flows(param_set->src_port, param_set->dst_port, param_set->proto, param_set->vip, param_set->vip_mac, param_set->mod_dst_ip, param_set->packet_dst_mac,
				param_set->src_gateway, param_set->dst_gateway, param_set->src_port_no, param_set->vip_tcp_port_no, param_set->src_security, param_set->dst_security);
	}
	else {

	}

	return GN_OK;
}

INT4 openstack_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in){
	if(src_port==NULL){
		external_port_p ext_port=NULL;
		arp_t *arp = (arp_t *)(packet_in->data);
		if (OPENSTACK_PORT_TYPE_GATEWAY == dst_port->type)
		{
			//NAT
			ext_port = get_external_port_by_out_interface_ip(targetip);
			gn_switch_t * ext_sw = NULL;
			ext_sw = get_ext_sw_by_dpid(ext_port->external_dpid);
			fabric_openstack_create_arp_reply_public( ext_port->external_outer_interface_mac, ext_port->external_outer_interface_ip,
					arp->sendmac, arp->sendip, ext_sw, ext_port->external_port, packet_in);
		}else{
			//floating_ip
			external_floating_ip_p fip = find_external_floating_ip_by_floating_ip(targetip);
			if (NULL != fip) {
				external_port_p ext_port = get_external_port_by_floatip(fip->floating_ip);
				p_fabric_host_node float_port = find_fabric_host_port_by_port_id(fip->port_id);
				gn_switch_t * ext_sw = NULL;
				if (NULL != ext_port) {
					ext_sw = get_ext_sw_by_dpid(ext_port->external_dpid);
					arp_t *arp = (arp_t *)(packet_in->data);
					// printf("%s:create floating reply\n",FN);
					fabric_openstack_create_arp_reply_public(float_port->mac, targetip,arp->sendmac,
							arp->sendip, ext_sw, ext_port->external_port, packet_in);
				}
			}
		}
	}else{
		if(dst_port!=NULL){
			fabric_openstack_create_arp_reply(src_port,dst_port,packet_in);
		}
	}
	return GN_OK;
}

INT4 openstack_arp_reply_output(p_fabric_host_node src,p_fabric_host_node dst,UINT4 targetIP, packet_in_info_t *packet_in){
	arp_t *arp = (arp_t *)(packet_in->data);
	UINT1 arp_dst_mac[6] = {0};
	memcpy(arp_dst_mac, arp->eth_head.dest, 6);
	//fabric_push_flow_queue(src,arp->sendip, dst, targetIP);
	memcpy(arp->eth_head.dest,dst->mac, 6);
	arp->targetip = targetIP;
	memcpy(arp->targetmac,dst->mac, 6);
	if (NULL != dst->sw) {
		fabric_packet_output(dst->sw,packet_in,dst->port);
	}

	if (OPENSTACK_PORT_TYPE_GATEWAY == dst->type) {
		update_openstack_external_gateway_mac(arp->sendip, arp->sendmac, arp->targetip, arp_dst_mac);
	}
	return GN_OK;
}

INT4 openstack_ip_p_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,UINT1* srcmac,packet_in_info_t *packet_in)
{
//	if (NULL != src_port) {
//		printf("start ip flood!");
//		//nat_show_ip(targetip);
//		p_fabric_arp_request_node arp_node = create_fabric_arp_request_list_node(src_port,sendip,targetip);
//		insert_fabric_arp_request_into_list(arp_node);
//		// flood to outter ports
//		fabric_push_arp_flood_queue(targetip,packet_in);
//	}

//	fabric_opnestack_create_arp_flood(sendip, targetip, sendmac);
//	printf("start flood!\n");
//	fabric_openstack_packet_flood(packet_in);
	fabric_opnestack_create_arp_flood(sendip, targetip, srcmac);
	return GN_OK;
}

INT4 openstack_ip_p_broadcast(packet_in_info_t *packet_in)
{
	p_fabric_host_node src_port = NULL;
	p_fabric_host_node dst_port = NULL;

	ip_t *p_ip = (ip_t *)(packet_in->data);
	if (NULL == p_ip) {
		return GN_ERR;
	}

	src_port = get_fabric_host_from_list_by_mac(p_ip->eth_head.src);
	dst_port = get_fabric_host_from_list_by_mac(p_ip->eth_head.dest);
	
	if (IPPROTO_UDP == p_ip->proto)
	{
		udp_t* udp = (udp_t*)p_ip->data;
		
		// if dhcp request handler
		if (67 == ntohs(udp->dport)) {
			fabric_openstack_dhcp_request_handle(packet_in, src_port);
		}
		// if dhcp reply handler
		else if (68 == ntohs(udp->dport)) {
			fabric_openstack_dhcp_reply_handle(packet_in, dst_port);
		}
		// ohter case
		else {
			fabric_openstack_packet_flood(packet_in);
		}
	}
	else {
		fabric_openstack_packet_flood(packet_in);
	}
	return GN_OK;
}
INT4 openstack_ip_packet_output(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in){

	return GN_OK;
}


gn_switch_t* get_ext_sw_by_dpid(UINT8 dpid){
	gn_switch_t * ext_sw = NULL;
	ext_sw = find_sw_by_dpid(dpid);
	if (NULL == ext_sw) {
		LOG_PROC("INFO", "Floating IP: gateway sw is NULL!");
		return NULL;
	}
	return ext_sw;
}


/*****************************
 * intern function
 *****************************/
//void fabric_openstack_arp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
//	p_fabric_host_node src_port = NULL;
//	p_fabric_host_node dst_port = NULL;
//	openstack_subnet_p subnet = NULL;
//	arp_t *arp = (arp_t *)(packet_in->data);
//
//	//printf("%s\n", FN);
//	//printf("source ip  ");
//	//fabric_openstack_show_ip(arp->sendip);
//	//printf("destination ip  ");
//	//fabric_openstack_show_ip(arp->targetip);
//	//printf("source mac  ");
//	////fabric_openstack_show_mac(arp->sendmac);
//	//printf("destination mac  ");
//	//fabric_openstack_show_mac(arp->targetmac);
//	//LOG_PROC("TEST", "%d| %d| ", sw->dpid, packet_in->inport);
//
//	//printf("\n");
//
//
//	src_port = get_fabric_host_from_list_by_mac(arp->sendmac);
//	if(src_port == NULL){
//
//		external_floating_ip_p fip = find_external_floating_ip_by_floating_ip(arp->targetip);
//		if(fip != NULL)
//		{
//			fabric_openstack_floating_ip_arp_request_handle(sw, fip, packet_in);
//		}else{
//			external_port_p epp = get_external_port_by_out_interface_ip(arp->targetip);
//			//ques
//			if(epp==NULL){
////				LOG_PROC("INFO","ARP Request Handle : Can't find destination host!");
////				fabric_openstack_show_ip(arp->targetip);
//				return;
//			}
//			// printf("Nat IP Arp Request Handle Start \n");
//
//			gn_switch_t * ext_sw = NULL;
//			ext_sw = find_sw_by_dpid(epp->external_dpid);
//			if (NULL == ext_sw) {
//				LOG_PROC("INFO", "NAT IP: gateway sw is NULL!");
//				return;
//			}
//
//			fabric_openstack_create_arp_reply_public( epp->external_outer_interface_mac, epp->external_outer_interface_ip,
//								arp->sendmac, arp->sendip, ext_sw, epp->external_port, packet_in);
////			packout_req_info_t packout_req_info;
////			arp_t new_arp_pkt;
////
////			packout_req_info.buffer_id = 0xffffffff;
////			packout_req_info.inport = OFPP13_CONTROLLER;
////			packout_req_info.outport = packet_in->inport;
////			packout_req_info.max_len = 0xff;
////			packout_req_info.xid = packet_in->xid;
////			packout_req_info.data_len = sizeof(arp_t);
////			packout_req_info.data = (UINT1 *)&new_arp_pkt;
////
////			memcpy(&new_arp_pkt, arp, sizeof(arp_t));
////			memcpy(new_arp_pkt.eth_head.src, epp->external_outer_interface_mac, 6);
////			memcpy(new_arp_pkt.eth_head.dest, arp->sendmac, 6);
////			new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
////			new_arp_pkt.opcode = htons(2);
////			new_arp_pkt.sendip = arp->targetip;
////			new_arp_pkt.targetip = arp->sendip;
////			memcpy(new_arp_pkt.sendmac, epp->external_outer_interface_mac, 6);
////			memcpy(new_arp_pkt.targetmac, arp->sendmac, 6);
////
////			sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
//		}
//		return;
//	}
//	// update
//	src_port->ip_list[0] = arp->sendip;
//	src_port->port = packet_in->inport;
//	src_port->sw = sw;
//	openstack_port_p src_port_p = (openstack_port_p)src_port->data;
//
//	// find dst_port by ip (because mac maybe is the 00:00:00:00:00:00)
//	dst_port = find_fabric_host_port_by_subnet_id(arp->targetip,src_port_p->subnet_id);
//	openstack_port_p dst_port_p = (openstack_port_p)dst_port->data;
//	subnet = find_openstack_app_subnet_by_subnet_id(src_port_p->subnet_id);
//	if(NULL == dst_port){
//		// flood in subnet
//		// maybe dest_port is new and dhcp,
//		// but not update the ip after dhcp
//		LOG_PROC("INFO","ARP Request Handle : Can't find destination host!");
//		//fabric_openstack_packet_flood_in_subnet(packet_in,subnet->subnet_id,subnet->port_num);
//		// if gateway no flood
//		if(arp->targetip == subnet->gateway_ip){
//			LOG_PROC("INFO","NO gateway,IP_DROP!");
//			return;
//		}else{
//			LOG_PROC("INFO","Flood in subnet!");
//			fabric_openstack_packet_flood(packet_in);
//		}
//		return;
//	}
//
//	// create reply
//	fabric_openstack_create_arp_reply(src_port,dst_port,packet_in);
//	// if gateway no flood
//	if(dst_port->ip_list[0] == subnet->gateway_ip){
//		return;
//	}
//
//	if(dst_port->sw == NULL){
//		// flood
//		fabric_openstack_packet_flood(packet_in);
//	}else{
//		LOG_PROC("INFO","ARP REQUEST Handle : SETUP FLOWS & PACKET OUT!");
//
//		//setup flow
//		// fobidden setup flows if it's gateway & dhcp port
//		if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,subnet)){
//			fabric_openstack_install_fabric_flows(src_port,dst_port);
//		}
//		// packet out
//		fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
//	}
//
//	return;
//};
//void fabric_openstack_arp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
//	p_fabric_host_node src_port = NULL;
//	p_fabric_host_node dst_port = NULL;
//	openstack_subnet_p subnet = NULL;
//	arp_t *arp = (arp_t *)(packet_in->data);
////	printf("%s\n", FN);
////	printf("source ip  ");
////	fabric_openstack_show_ip(arp->sendip);
////	printf("destination ip  ");
////	fabric_openstack_show_ip(arp->targetip);
////	printf("source mac  ");
////	fabric_openstack_show_mac(arp->sendmac);
////	printf("destination mac  ");
////	fabric_openstack_show_mac(arp->targetmac);
////	printf("\n");
//
//	src_port = get_fabric_host_from_list_by_mac(arp->sendmac);
//	if(src_port == NULL){
//		LOG_PROC("INFO","ARP Rply Handle : Can't find source host!");
//		return;
//	}
//	// update
//	src_port->ip_list[0] = arp->sendip;
//	src_port->port = packet_in->inport;
//	src_port->port = packet_in->inport;
//
//	if (NULL == src_port->sw) {
//		install_fabric_output_flow(sw, src_port->mac, packet_in->inport);
//	}
//
//	src_port->sw = sw;
//	openstack_port_p src_port_p = (openstack_port_p)src_port->data;
//	// printf("%s update sw(%s) to ", FN, inet_ntoa(*(struct in_addr*)&sw->sw_ip));
//	// printf(" ip(%s); port is %d\n", inet_ntoa(*(struct in_addr*)&arp->sendip), packet_in->inport);
//
//	// find dst_port (because mac maybe is the gateway)
//	dst_port = find_fabric_host_port_by_subnet_id(arp->targetip,src_port_p->subnet_id);
//	if(NULL == dst_port){
//		external_floating_ip_p fip = find_external_floating_ip_by_fixed_ip(arp->sendip);
//		if(fip != NULL)
//		{
//			fabric_openstack_floating_ip_arp_reply_handle(sw, fip, packet_in);
//		}
//		// IP_DROP
//		LOG_PROC("INFO","ARP Rply Handle : Can't find destination host!");
//		return;
//	}
//
//
//	if(dst_port->sw == NULL){
//		// flood
//		fabric_openstack_packet_flood(packet_in);
//	}else{
//		LOG_PROC("INFO","ARP RPLAY Handle : SETUP FLOWS & PACKET OUT!");
//
//		subnet =find_openstack_app_subnet_by_subnet_id(src_port_p->subnet_id);
//		// download flows
//		// fobidden setup flows if it's gateway & dhcp port
//		if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,subnet)){
//			fabric_openstack_install_fabric_flows(src_port,dst_port);
//		}
//		// packet out
//		fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
//	}
//
//	return;
//};
/*
 * ip broad cast
 */
 /*
void fabric_openstack_ip_broadcast_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,p_fabric_host_node src_port){
	udp_t* udp = NULL;
//	printf("%s\n", FN);
	if(ip->proto == IPPROTO_UDP){
		udp = (udp_t*)(ip->data);
		LOG_PROC("INFO","UDP! udp->sport : %u | udp->dport : %u \n",udp->sport,udp->dport);
		if(udp->sport == 68 && udp->dport == 67){
			fabric_openstack_dhcp_request_handle(sw,packet_in,ip,udp,src_port);
			return;
		}else if(udp->sport == 67 && udp->dport == 68){
			fabric_openstack_dhcp_reply_handle(sw,packet_in,ip,udp,src_port);
			return;
		}
	}

	// flood in subnet
	fabric_openstack_packet_flood(packet_in);
	return;
};
*/
/*
 * dhcp request
 */
void fabric_openstack_dhcp_request_handle(packet_in_info_t *packet_in, p_fabric_host_node src_port)
{
	p_fabric_host_node dhcp_port = NULL;
	openstack_port_p src_port_p = NULL;

	if ((NULL == src_port) || (NULL == src_port->data)) {
		return ;
	}

	src_port_p = (openstack_port_p)src_port->data;
	dhcp_port = find_openstack_app_dhcp_by_subnet_id(src_port_p->subnet_id);

	if ((NULL == dhcp_port) || (NULL == dhcp_port->sw) || (0 == dhcp_port->port)) {
		// if dhcp not exist, flood
		// printf("%s-flood\n", FN);
		fabric_openstack_packet_flood(packet_in);
	}
	else {
		// packet out
		// printf("%s-packetout\n", FN);
		fabric_openstack_packet_output(dhcp_port->sw, packet_in, dhcp_port->port);
	}
	return;
};
/*
 * dhcp reply
 */
void fabric_openstack_dhcp_reply_handle(packet_in_info_t *packet_in, p_fabric_host_node dst_port)
{
	if ((NULL == dst_port) || (NULL == dst_port->sw) || (0 == dst_port->port)) {
		// if dhcp not exist, flood
		fabric_openstack_packet_flood(packet_in);
	}
	else {
		// packet out
		fabric_openstack_packet_output(dst_port->sw, packet_in, dst_port->port);
	}
	return;
}

/*
void fabric_openstack_dhcp_reply_handler(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,udp_t* udp,p_fabric_host_node src_port){
	p_fabric_host_node dst_port = NULL;
	dhcp_t* dhcp = NULL;
	UINT1 mac[6];
//	printf("%s\n", FN);
	// find dst
	dhcp = (dhcp_t*)(udp->data);
	memcpy(mac,dhcp->cmcaddr,6);

	dst_port = get_fabric_host_from_list_by_mac(mac);
	if(dst_port == NULL || dst_port->port == 0){
		// flood
		fabric_openstack_packet_flood(packet_in);
	}else{
		// packet out
		fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
	}
	return;
};
*/

void fabric_openstack_install_fabric_flows(p_fabric_host_node src_port,p_fabric_host_node dst_port,
										   security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	// display port info
//	LOG_PROC("INFO","Sourt Port Info:");
//	fabric_openstack_show_port(src_port);
//	LOG_PROC("INFO","Destination Port Info:");
//	fabric_openstack_show_port(dst_port);
	if ((NULL== src_port) || (NULL == dst_port) || (NULL == src_port->sw) || (NULL == dst_port->sw) 
						  || (0 == src_port->ip_list[0]) || (0 == dst_port->ip_list[0])) {
		return ;
	}

	src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);
	if(src_port->sw == dst_port->sw){
		// printf("same switch\n");
		install_fabric_same_switch_security_flow(src_port->sw,src_port->mac,src_port->port, src_security);
		install_fabric_same_switch_security_flow(dst_port->sw,dst_port->mac,dst_port->port, dst_security);
	}else{
		// printf("different switch\n");
		install_fabric_push_tag_security_flow(src_port->sw,dst_port->ip_list[0], dst_port->mac,dst_tag, src_security);
		install_fabric_push_tag_security_flow(dst_port->sw,src_port->ip_list[0], src_port->mac,src_tag, dst_security);
		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}

	return;
};
void fabric_openstack_install_fabric_out_subnet_flows(p_fabric_host_node src_port,p_fabric_host_node src_gateway,
		p_fabric_host_node dst_port,p_fabric_host_node dst_gateway, security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	// display port info
//	LOG_PROC("INFO","Sourt Port Info:");
//	fabric_openstack_show_port(src_port);
//	LOG_PROC("INFO","Destination Port Info:");
//	fabric_openstack_show_port(dst_port);
	if ((NULL== src_port) || (NULL == dst_port) || (NULL == src_port->sw) || (NULL == dst_port->sw) 
						  || (0 == src_port->ip_list[0]) || (0 == dst_port->ip_list[0])) {
		return ;
	}

	src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);
	if(src_port->sw == dst_port->sw){
		install_fabric_same_switch_out_subnet_flow(src_port->sw,src_gateway->mac,dst_port->mac,dst_port->ip_list[0],dst_port->port, src_security);
		install_fabric_same_switch_out_subnet_flow(dst_port->sw,dst_gateway->mac,src_port->mac,src_port->ip_list[0],src_port->port, dst_security);
	}else{
		install_fabric_push_tag_out_subnet_flow(src_port->sw,src_gateway->mac,dst_port->mac,dst_port->ip_list[0],dst_tag, src_security);
		install_fabric_push_tag_out_subnet_flow(dst_port->sw,dst_gateway->mac,src_port->mac,src_port->ip_list[0],src_tag, dst_security);
		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}
	return;
};

int check_fabric_openstack_subnet_dhcp_gateway(p_fabric_host_node port,openstack_subnet_p subnet){
	return ((port == subnet->dhcp_port) || (port == subnet->gateway_port))?1:0;
};
/*****************************
 * intern function: packet out
 *****************************/
/*
 * out put the packet
 */
void fabric_openstack_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport){
	if (NULL != sw) {
		packout_req_info_t pakout_req;
		pakout_req.buffer_id = packet_in_info->buffer_id;
		pakout_req.inport = OFPP13_CONTROLLER;
		pakout_req.outport = outport;
		pakout_req.max_len = 0xff;
		pakout_req.xid = packet_in_info->xid;
		pakout_req.data_len = packet_in_info->data_len;
		pakout_req.data = packet_in_info->data;
		sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
	}
};
/*
 * flood
 */
void fabric_openstack_packet_flood(packet_in_info_t *packet_in_info){
	packout_req_info_t pakout_req;
	gn_switch_t *sw = NULL;
	UINT2 i = 0,j=0;
	pakout_req.buffer_id = 0xffffffff;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;
//	pakout_req.outport = OFPP13_FLOOD;

	// find all switch
	for(i = 0; i < g_server.max_switch; i++){
		if (g_server.switches[i].state){
			sw = &g_server.switches[i];
//			sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
			// find switch's outter ports

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
/*
 * flood in subnet
 */
void fabric_openstack_packet_flood_in_subnet(packet_in_info_t *packet_in_info,char* subnet_id,UINT4 subnet_port_num){
	p_fabric_host_node port_list[subnet_port_num+3];
	p_fabric_host_node temp = NULL;
	UINT4 i = 0,port_num = 0;
	packout_req_info_t pakout_req;
	gn_switch_t *sw = NULL;

	pakout_req.buffer_id = 0xffffffff;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;


	port_num = find_fabric_host_ports_by_subnet_id(subnet_id,port_list);
	for(i = 0 ; i < port_num ; i++){
		temp = port_list[i];
		if(temp != NULL && temp->port != 0 && temp->sw != NULL){
			sw = temp->sw;
			sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
		}
	}
	return;
};

void fabric_openstack_create_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in_info){
    packout_req_info_t packout_req_info;
    arp_t new_arp_pkt;
    arp_t *arp = (arp_t *)(packet_in_info->data);

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = OFPP13_CONTROLLER;
    packout_req_info.outport = src_port->port;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = packet_in_info->xid;
    packout_req_info.data_len = sizeof(arp_t);
    packout_req_info.data = (UINT1 *)&new_arp_pkt;

    memcpy(&new_arp_pkt, arp, sizeof(arp_t));
    memcpy(new_arp_pkt.eth_head.src, dst_port->mac, 6);
    memcpy(new_arp_pkt.eth_head.dest, src_port->mac, 6);
    new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
    new_arp_pkt.opcode = htons(2);
    new_arp_pkt.sendip = dst_port->ip_list[0];
    new_arp_pkt.targetip = src_port->ip_list[0];
    memcpy(new_arp_pkt.sendmac, dst_port->mac, 6);
    memcpy(new_arp_pkt.targetmac, src_port->mac, 6);

    src_port->sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](src_port->sw, (UINT1 *)&packout_req_info);
};

void fabric_openstack_create_arp_reply_public(UINT1* srcMac,UINT4 srcIP, UINT1* dstMac,UINT4 dstIP,
		gn_switch_t* sw, UINT4 outPort,packet_in_info_t *packet_in_info){
//	LOG_PROC("TEST", "fabric_openstack_create_arp_reply_public srcMac| srcIP| dstMac| dstIP| sw dpid| outPort|");
//	fabric_openstack_show_mac(srcMac);
//	fabric_openstack_show_ip(srcIP);
//	fabric_openstack_show_mac(dstMac);
//	fabric_openstack_show_ip(dstIP);
//	LOG_PROC("TEST", "%d| %d| ", sw->dpid, outPort);


    packout_req_info_t packout_req_info;
    arp_t new_arp_pkt;
    arp_t *arp = (arp_t *)(packet_in_info->data);

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = OFPP13_CONTROLLER;
    packout_req_info.outport = outPort;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = packet_in_info->xid;
    packout_req_info.data_len = sizeof(arp_t);
    packout_req_info.data = (UINT1 *)&new_arp_pkt;

    memcpy(&new_arp_pkt, arp, sizeof(arp_t));
    memcpy(new_arp_pkt.eth_head.src,srcMac , 6);
    memcpy(new_arp_pkt.eth_head.dest,dstMac , 6);
    new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
    new_arp_pkt.opcode = htons(2);
    new_arp_pkt.sendip = srcIP;
    new_arp_pkt.targetip = dstIP;
    memcpy(new_arp_pkt.sendmac, srcMac, 6);
    memcpy(new_arp_pkt.targetmac,dstMac , 6);

    sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
};

void fabric_openstack_external_arp_mac(){
//	packout_req_info_t packout_req_info;
//    arp_t new_arp_pkt;
//    packout_req_info.buffer_id = 0xffffffff;
//	packout_req_info.inport = 0xfffffffd;
//	packout_req_info.outport = 0;
//	packout_req_info.max_len = 0xff;
//	packout_req_info.xid = 0;
//	packout_req_info.data_len = sizeof(arp_t);
//	packout_req_info.data = (UINT1 *)&new_arp_pkt;
//
//	memcpy(&new_arp_pkt, arp, sizeof(arp_t));
//	memcpy(new_arp_pkt.eth_head.src, g_controller_mac, 6);
//	memcpy(new_arp_pkt.eth_head.dest, arp->eth_head.src, 6);
//	new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
//	new_arp_pkt.opcode = htons(2);
//	new_arp_pkt.sendip = arp->targetip;
//	new_arp_pkt.targetip = arp->sendip;
//	memcpy(new_arp_pkt.sendmac, g_controller_mac, 6);
//	memcpy(new_arp_pkt.targetmac, arp->sendmac, 6);
//
//	if(sw->ofp_version == OFP10_VERSION)
//	{
//		sw->msg_driver.msg_handler[OFPT_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
//	}
//	else if(sw->ofp_version == OFP13_VERSION)
//	{
//		sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
//	}
};


void fabric_opnestack_create_arp_flood(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac)
{
	// LOG_PROC("INFO", "External: Can't find the dest: start flood!");
	packet_in_info_t packout_req_info;
	arp_t* new_arp_pkt = (arp_t*)malloc(sizeof(arp_t));

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.xid = 0;
	packout_req_info.data_len = sizeof(arp_t);
	packout_req_info.data = (UINT1 *)new_arp_pkt;

	memcpy(new_arp_pkt->eth_head.src, src_mac, 6);
	memcpy(new_arp_pkt->eth_head.dest, arp_broadcat_mac, 6);
	new_arp_pkt->eth_head.proto = htons(ETHER_ARP);
	new_arp_pkt->hardwaretype = htons(1);
	new_arp_pkt->prototype = htons(ETHER_IP);
	new_arp_pkt->hardwaresize = 0x6;
	new_arp_pkt->protocolsize = 0x4;
	new_arp_pkt->opcode = htons(1);
	new_arp_pkt->sendip = src_ip;
	new_arp_pkt->targetip=dst_ip;

	memcpy(new_arp_pkt->sendmac, src_mac, 6);
	memcpy(new_arp_pkt->targetmac, arp_zero_mac, 6);

	p_fabric_host_node src_node = get_fabric_host_from_list_by_ip(src_ip);

	fabric_add_into_arp_request(src_node,src_ip,dst_ip);
	// flood to outter ports
	fabric_push_arp_flood_queue(dst_ip, &packout_req_info);
}

INT4 openstack_ip_packet_compute_src_dst_forward(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in, param_set_p param_set)
{
	// return value is foward type
	INT4 foward_type = IP_DROP;
	ip_t *ip = (ip_t *)(packet_in->data);

	if ((NULL != dst_port) && (OPENSTACK_PORT_TYPE_GATEWAY == dst_port->type))
	{
		dst_port = NULL;
	}

	if (NULL != get_external_floating_ip_by_floating_ip(ip->dest))
	{
		dst_port = NULL;
	}

	if ((ip->dest == ntohl(g_reserve_ip)) && (src_port)) {
		if (IPPROTO_ICMP == ip->proto) {

			icmp_t* icmp = (icmp_t*)ip->data;
			// printf("receive controller port:%d\n", icmp->id);
			update_openstack_lbaas_listener_member_status(LBAAS_LISTENER_PING, src_port, ntohs(icmp->id), 0, 0);
		}
        else if (IPPROTO_TCP == ip->proto) {
            tcp_t* tcp = (tcp_t*)ip->data;

            // printf("receive controller ack:%d, sport: %d, code:%d\n", ntohl(tcp->ack), ntohs(tcp->sport), tcp->code);
            // receive "SYN, ACK" packet
            update_openstack_lbaas_listener_member_status(LBAAS_LISTENER_TCP, src_port, ntohl(tcp->ack), ntohs(tcp->sport), tcp->code);
        }
		return IP_DROP;
	}


	// internal network
	if ((NULL != src_port) && (NULL != dst_port))
	{
//		openstack_port_p src_port_p = (openstack_port_p)src_port->data;
//		openstack_port_p dst_port_p = (openstack_port_p)dst_port->data;
//
//		if ((src_port_p) && (dst_port_p)) {
//			if (0 != strcmp(src_port_p->tenant_id, src_port_p->tenant_id)) {
//				return IP_DROP;
//			}
//		}
		
		if (NULL != find_openstack_lbaas_pool_by_ip(ip->src) || (NULL != find_openstack_lbaas_pool_by_ip(ip->dest))) {
			foward_type = internal_packet_compute_vip_forward(src_port, dst_port, ip->dest, param_set, ip);
		}
		else {
			foward_type = internal_packet_compute_forward(src_port, dst_port, ip->dest, param_set, ip);
		}
	}
	// from external to internal
	else if ((NULL == src_port) && (g_broad_ip != ip->dest))
	{
		foward_type = external_packet_in_compute_forward(src_port, ip->src, ip->dest, packet_in, ip->proto, param_set);
	}
	// from internal to external
	else if (NULL == dst_port)
	{
		// if dest ip is dns or fobidden ip
		if (/*(ip->dest == g_openstack_dns_ip) || */((ip->dest == g_openstack_fobidden_ip)))
		{
			// LOG_PROC("INFO", "dst is forbidden ip or dns ip!");
			return IP_DROP;
		}
		// is broadcast?
		else if(/*0 ==memcmp(g_broad_mac,ip->eth_head.dest,6) || */g_broad_ip == ip->dest){
			// fabric_openstack_ip_broadcast_handle(src_port->sw,packet_in,ip,src_port);
			return BROADCAST_DHCP;
		}
		else if(ip->proto == IPPROTO_UDP) {
			udp_t* udp = (udp_t*)(ip->data);
			// printf("%d %d\n", udp->sport, udp->dport);
			if ((udp->sport == htons(68) && udp->dport == htons(67)) || (udp->sport == htons(67) && udp->dport == htons(68)))
			{
				// fabric_openstack_ip_broadcast_handle(src_port->sw,packet_in,ip,src_port);
				// fabric_openstack_packet_flood(packet_in);
				// fabric_ip_packet_flood(packet_in);
				return BROADCAST_DHCP;
			}
			else {
				foward_type = external_packet_out_compute_forward(src_port, ip->src, ip->dest, packet_in, ip->proto, param_set);
			}
		}
		else {
			foward_type = external_packet_out_compute_forward(src_port, ip->src, ip->dest, packet_in, ip->proto, param_set);
		}
	}
	else
	{
		// do nothing
	}

	return foward_type;
}

INT4 internal_packet_compute_vip_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip, param_set_p param_set, ip_t *ip)
{
	// define the value
	UINT4 vip_tcp_port_no = 0;
	// p_fabric_host_node vip_dst_port = NULL;
	p_fabric_host_node vip_port = NULL;
	p_fabric_host_node src_gw = NULL;
	p_fabric_host_node dst_gw = NULL;
	p_fabric_host_node vip_gw = NULL;

	if (src_port) {
		// if source is virtual ip
		if (find_openstack_lbaas_pool_by_ip(src_port->ip_list[0])) {
			// printf("src is virtual ip\n", FN);
			return IP_DROP;
		}

		// if the src ip in the same virtual ip pool, drop
		if (find_openstack_lbaas_member_by_ip(dst_port->ip_list[0], src_port->ip_list[0])) {
			// printf("src is inside ip\n", FN);
			return IP_DROP;
		}
	}


	// get fixed ip
	UINT4 lb_ip = get_openstack_lbaas_ip_by_ip_proto(targetip, ip->proto);

	// if exist, save the dest
	if (lb_ip) {
		// printf("get member ip\n", FN);
		// nat_show_ip(lb_ip);

		vip_port = dst_port;
		param_set->vip = dst_port->ip_list[0];
		memcpy(param_set->vip_mac, dst_port->mac, 6);
	}
	else {
		return IP_DROP;
	}

	// get the fixed port
	dst_port = get_fabric_host_from_list_by_ip(lb_ip);

	if (NULL == dst_port) {
		LOG_PROC("INFO", "load balance dst port is NULL");
		return IP_DROP;
	}

// if src port and vip not in same pool
	if ((src_port) && (dst_port))
	{
		openstack_port_p src_port_p = (openstack_port_p)src_port->data;
		openstack_port_p dst_port_p = (openstack_port_p)dst_port->data;

		if (((src_port_p) && (dst_port_p)) &&  (0 == strcmp(src_port_p->tenant_id, dst_port_p->tenant_id))) {
			// do nothing
		}
		else {
			return IP_DROP;
		}
	}

	// if dst port sw not exist, save and flood
	if ((NULL == dst_port->sw) || (0 == dst_port->port)) {
		// printf("Can't find ip start flood %s\n", FN);
		if (src_port) {
			param_set->src_ip = src_port->ip_list[0];
			memcpy(param_set->src_mac, src_port->mac, 6);
			param_set->dst_ip = dst_port->ip_list[0];
			return IP_FLOOD;
		}
		else {
			return create_arp_flood_parameter(dst_port->ip_list[0], dst_port, param_set);
		}
	}
	else {
		if (src_port)
			src_gw = find_openstack_app_gateway_by_host(src_port);

		if (dst_port)
			dst_gw = find_openstack_app_gateway_by_host(dst_port);

		if (vip_port)
			vip_gw = find_openstack_app_gateway_by_host(vip_port);

		param_set->src_gateway = (src_gw == vip_gw) ? NULL : src_gw;
		param_set->dst_gateway = (dst_gw == vip_gw) ? NULL : dst_gw;

		param_set->src_port = src_port;
		param_set->dst_port = dst_port;
		param_set->proto = ip->proto;

		if (ip->proto == IPPROTO_TCP) {
			tcp_t* tcp = (tcp_t*)ip->data;

			param_set->src_port_no = ntohs(tcp->sport);

			vip_tcp_port_no = create_openstack_lbaas_connect(ip->src, dst_port->ip_list[0], vip_port->ip_list[0], param_set->src_port_no);
			// printf("create vip port:%d", vip_tcp_port_no);

			param_set->vip_tcp_port_no = vip_tcp_port_no;

			return Internal_vip_flow;
		}
	}

	return IP_DROP;
}


INT4 external_packet_in_compute_vip_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip,
		param_set_p param_set, ip_t *ip, external_port_p epp, external_floating_ip_p fip)
{
	// if src port is external outer ip
		// get the inside ip
		// judge the ip is in the pool
		// port_p = find_nat_connect(packetin_src_ip, ntohs(packetin_dst_port), packetin_proto_type);

	INT4 return_value = internal_packet_compute_vip_forward(src_port, dst_port, targetip, param_set, ip);

	if (Internal_vip_flow == return_value) {
		param_set->src_sw = find_sw_by_dpid(epp->external_dpid);
		param_set->src_ip = ip->src;
		memcpy(param_set->src_mac, ip->eth_head.src, 6);
		param_set->dst_ip = ip->dest;
		memcpy(param_set->dst_mac, ip->eth_head.dest, 6);
		memcpy(param_set->src_gateway_mac, epp->external_gateway_mac, 6);
		memcpy(param_set->outer_gateway_mac, epp->external_gateway_mac, 6);
		return External_vip_flow;
	}
	else {
		return return_value;
	}
}

INT4 internal_packet_compute_floating_vip_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip,
		param_set_p param_set, ip_t *ip, external_port_p epp, external_floating_ip_p fip)
{
	p_fabric_host_node src_gw = find_openstack_app_gateway_by_host(src_port);
	if (NULL == src_gw) {
		return IP_DROP;
	}

	INT4 return_value = internal_packet_compute_vip_forward(src_port, dst_port, targetip, param_set, ip);

	if (Internal_vip_flow == return_value) {
		param_set->src_gateway = src_gw;
		param_set->mod_dst_ip = fip->floating_ip;
		return Internal_floating_vip_flow;
	}
	else {
		return return_value;
	}
}


INT4 internal_packet_compute_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip, param_set_p param_set, ip_t *ip)
{
	openstack_subnet_p src_subnet = NULL;
	openstack_subnet_p dst_subnet = NULL;
	p_fabric_host_node src_gateway = NULL;
	p_fabric_host_node dst_gateway = NULL;

	openstack_port_p src_port_p = (openstack_port_p)src_port->data;
	openstack_port_p dst_port_p = (openstack_port_p)dst_port->data;

	if ((NULL == src_port_p) || (NULL == dst_port_p)) {
		return IP_DROP;
	}

	src_subnet = find_openstack_app_subnet_by_subnet_id(src_port_p->subnet_id);

	if (NULL == src_subnet) {
		LOG_PROC("INFO", "Can't get src port subnet!");
		return IP_DROP;
	}

	// save the parameter
	param_set->src_port = src_port;
	param_set->dst_port = dst_port;
	param_set->dst_sw = dst_port->sw;
	param_set->dst_inport = dst_port->port;

	// if in the same subnet
	if (0 == strcmp(dst_port_p->subnet_id, src_port_p->subnet_id)) {
		if(NULL == dst_port->sw) {
			//flood
			return BROADCAST_DHCP;
		}
		else {
			//setup flow
			// fobidden setup flows if it's gateway & dhcp port
			if( (0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,src_subnet))
					&& (0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,src_subnet))) {
				// install flows
				return Internal_port_flow;
			}
			// packet out
			return CONTROLLER_FORWARD;
		}
	}
	// in the various subnet
	else {
		src_gateway = find_openstack_app_gateway_by_subnet_id(src_port_p->subnet_id);
		dst_gateway = find_openstack_app_gateway_by_subnet_id(dst_port_p->subnet_id);

		if (OPENSTACK_PORT_TYPE_DHCP != src_port->type)
		{
			if ((NULL == src_gateway) || (NULL == dst_gateway)) {
			// LOG_PROC("INFO","Src or Dst gateway is NULL!");
			return IP_DROP;
			}
			
			// modify the dst mac
			memcpy(ip->eth_head.dest,dst_port->mac,6);
		}

		if (dst_port->sw == NULL) {
			//flood
			return BROADCAST_DHCP;
		}
		else {
			dst_subnet = find_openstack_app_subnet_by_subnet_id(dst_port_p->subnet_id);
			// fobidden setup flows if it's gateway & dhcp port
			if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,src_subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,dst_subnet)){
				// save gateway info
				param_set->src_gateway = src_gateway;
				param_set->dst_gateway = dst_gateway;
				return Internal_out_subnet_flow;
			}
			//packet out
			return CONTROLLER_FORWARD;
		}
	}

	return IP_DROP;
}

INT4 external_packet_out_compute_forward(p_fabric_host_node src_port, UINT4 sendip, UINT4 targetip, packet_in_info_t *packet_in, UINT1 proto, param_set_p param_set)
{
	INT4 foward_type = IP_DROP;
	external_floating_ip_p fip_src = NULL;
	external_floating_ip_p fip_dst = NULL;
	p_fabric_host_node fixed_dst_port = NULL;

	// get floating ip
	fip_src = get_external_floating_ip_by_fixed_ip(sendip);
	fip_dst = get_external_floating_ip_by_floating_ip(targetip);

	// if source port is floating ip
	if(NULL != fip_src)
	{
		foward_type = fabric_openstack_floating_ip_packet_out_handle(src_port, packet_in, fip_src, param_set);
	}
	else if ((fip_dst) && (find_openstack_lbaas_pool_by_ip(fip_dst->fixed_ip))) {
		ip_t* ip = (ip_t*)packet_in->data;
		fixed_dst_port = get_fabric_host_from_list_by_ip(fip_dst->fixed_ip);
		if (fixed_dst_port)
			foward_type = internal_packet_compute_floating_vip_forward(src_port, fixed_dst_port, fip_dst->fixed_ip,
							param_set, ip, NULL, fip_dst);
	}
	else
	{
		if (IPPROTO_ICMP == proto) {
			foward_type = fabric_openstack_nat_icmp_comute_foward(src_port->sw, packet_in,TRUE,param_set);
		}
		else {
			foward_type = fabric_openstack_ip_nat_comute_foward(src_port->sw, packet_in, TRUE, param_set);
		}
	}
	return foward_type;
}

INT4 external_packet_in_compute_forward(p_fabric_host_node src_port, UINT4 src_ip, UINT4 targetip, packet_in_info_t* packet_in, UINT1 proto, param_set_p param_set)
{
	//outer -> inner
	external_floating_ip_p fip = NULL;
	p_fabric_host_node dst_port = NULL;
	INT4 foward_type = IP_DROP;

	fip = get_external_floating_ip_by_floating_ip(targetip);
	external_port_p epp = get_external_port_by_floatip(targetip);

	if ((NULL != fip) && (NULL != epp))
	{        
		if (g_proactive_flow_flag)
	        return IP_DROP;

        gn_switch_t * external_sw = get_ext_sw_by_dpid(epp->external_dpid);
		if (NULL == external_sw) {
			LOG_PROC("INFO", "Floating IP: Can't get external switch");
			return IP_DROP;
		}

		dst_port = get_fabric_host_from_list_by_ip(fip->fixed_ip);
		if (NULL == dst_port) {
			LOG_PROC("INFO", "Floating IP: Fixed ip is not exist");
			return IP_DROP;
		}

		if (NULL != find_openstack_lbaas_pool_by_ip(fip->fixed_ip)) {
			ip_t* ip = (ip_t*)packet_in->data;
			return external_packet_in_compute_vip_forward(src_port, dst_port, fip->fixed_ip, param_set, ip, epp, fip);
		}

		if (NULL == dst_port->sw) {
			LOG_PROC("INFO", "Floating IP: Fixed ip sw is NULL");
			return create_arp_flood_parameter(fip->fixed_ip, dst_port, param_set);
		}

		UINT4 out_port = get_out_port_between_switch(epp->external_dpid, dst_port->sw->dpid);
		if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) {
			param_set->dst_sw = external_sw;
			param_set->src_ip = dst_port->ip_list[0];
			memcpy(param_set->packet_src_mac, dst_port->mac, 6);
			param_set->dst_vlanid = of131_fabric_impl_get_tag_sw(dst_port->sw);
			param_set->dst_inport = out_port;

			param_set->src_sw = dst_port->sw;
			param_set->dst_ip = src_ip;
			memcpy(param_set->src_mac, dst_port->mac, 6);
			param_set->mod_src_ip = fip->floating_ip;
			memcpy(param_set->dst_gateway_mac, epp->external_gateway_mac, 6);
			param_set->src_vlanid = of131_fabric_impl_get_tag_sw(external_sw);
			param_set->src_inport = dst_port->port;

			return Floating_ip_flow;
		}
	}
	else
	{
		if (IPPROTO_ICMP == proto) {
			foward_type = fabric_openstack_nat_icmp_comute_foward(NULL, packet_in, FALSE, param_set);
		}
		else {
			foward_type = fabric_openstack_ip_nat_comute_foward(NULL, packet_in, FALSE, param_set);
		}
	}

	return foward_type;
}

INT4 openstack_check_src_dst_is_controller(packet_in_info_t *packet_in)
{
	INT4 check_result = GN_ERR;

	if (packet_in) {
		ip_t* ip = (ip_t*)packet_in->data;

		if ((ip) && ((ip->src == ntohl(g_reserve_ip)) || (ip->dest == ntohl(g_reserve_ip)))) {
			check_result = GN_OK;
		}
	}
	return check_result;
}


INT4 openstack_ip_packet_check_access(p_fabric_host_node src_port, p_fabric_host_node dst_port, packet_in_info_t *packet_in, param_set_p param)
{

	INT4 check_result = GN_ERR;

	check_result = openstack_check_src_dst_is_controller(packet_in);

	if (GN_OK == check_result) {
		return check_result;
	}

	check_result = openstack_security_group_main_check(src_port, dst_port, packet_in, param->src_security, param->dst_security);
	return check_result;
}


void remove_flows_by_sw_port(UINT8 sw_dpid, UINT4 port)
{
	if (0 == g_openstack_on) {
		return ;
	}

	p_fabric_host_node host = get_fabric_host_from_list_by_sw_port(sw_dpid, port);

	if (NULL != host) {
		gn_switch_t* sw = host->sw;


		host->sw = NULL;
		host->port = 0;

		external_floating_ip_p fip = NULL;
		fip = get_external_floating_ip_by_fixed_ip(host->ip_list[0]);
		if (NULL != fip) {
			remove_floating_flow(sw, fip->floating_ip, host->mac);
		}
		else {
			remove_nat_flow(sw, host->ip_list[0], host->mac);
		}

		if (NULL != sw) {
			remove_host_output_flow_by_ip_mac(sw, host->ip_list[0], host->mac);
		}

		// create arp flood
		p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(host);
		if (NULL != gateway_p) {
			fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], host->ip_list[0], gateway_p->mac);
		}
	}
}

void remove_floating_flow(gn_switch_t* sw, UINT4 floating_ip, UINT1* mac)
{
	external_port_p epp = get_external_port_by_floatip(floating_ip);
	if (NULL != epp) {
	  gn_switch_t* sww = get_ext_sw_by_dpid(epp->external_dpid);
	  if (NULL != sww) {
		  delete_fabric_input_flow_by_ip(sww, floating_ip);
		  // delete_fabric_flow_by_mac(sw, mac, FABRIC_OUTPUT_TABLE);
	  }
	}
}

void remove_nat_flow(gn_switch_t* sw, UINT4 ip, UINT1* src_mac)
{
	UINT2 port_list[100];
	UINT4 externalip_list[100];
	UINT2 proto_list[100];
	UINT4 port_number;
	port_number = get_nat_connect_count_by_ip(ip, port_list, externalip_list, proto_list);
	external_port_p epp = get_external_port_by_host_mac(src_mac);
	if (NULL != epp) {
		gn_switch_t* sww = get_ext_sw_by_dpid(epp->external_dpid);
		while (port_number)
		{
//			printf("port number:%d\n", port_list[port_number]);
//			printf("proto is %d", proto_list[port_number]);
//			nat_show_mac(epp->external_outer_interface_mac);
			delete_fabric_input_flow_by_mac_portno(sww, epp->external_outer_interface_mac, port_list[port_number], proto_list[port_number]);
//			delete_fabric_output_flow_by_ip_portno(sw, externalip_list[port_number], port_list[port_number], proto_list[port_number]);
//			delete_fabric_flow_by_ip(sw, externalip_list[port_number], FABRIC_PUSHTAG_TABLE);
			port_number--;
		}
	}
}

void remove_host_output_flow_by_ip_mac(gn_switch_t* sw, UINT4 ip, UINT1* mac)
{
	// delete_fabric_flow_by_ip()
	if (0 != ip) {
		delete_fabric_flow_by_ip(sw, ip, FABRIC_OUTPUT_TABLE);
	}

	if (NULL != mac) {
		delete_fabric_flow_by_mac(sw, mac, FABRIC_OUTPUT_TABLE);
	}
}

INT4 create_arp_flood_parameter(UINT4 dst_ip, p_fabric_host_node dst_port, param_set_p param)
{
	if (NULL != dst_port) {
		p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(dst_port);

		if (NULL != gateway_p) {
			// fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], fip->fixed_ip, gateway_p->mac, epp->external_dpid);
			param->src_ip = gateway_p->ip_list[0];
			param->dst_ip = dst_ip;
			memcpy(param->src_mac, gateway_p->mac, 6);
			return IP_FLOOD;
		}
	}

	return IP_DROP;
}

void fabric_opnestack_create_arp_request(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, gn_switch_t* sw, UINT4 outPort)
{

	packet_in_info_t packout_req_info;
	arp_t* new_arp_pkt = (arp_t*)malloc(sizeof(arp_t));

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.xid = 0;
	packout_req_info.data_len = sizeof(arp_t);
	packout_req_info.data = (UINT1 *)new_arp_pkt;

	memcpy(new_arp_pkt->eth_head.src, src_mac, 6);
	memcpy(new_arp_pkt->eth_head.dest, arp_broadcat_mac, 6);
	new_arp_pkt->eth_head.proto = htons(ETHER_ARP);
	new_arp_pkt->hardwaretype = htons(1);
	new_arp_pkt->prototype = htons(ETHER_IP);
	new_arp_pkt->hardwaresize = 0x6;
	new_arp_pkt->protocolsize = 0x4;
	new_arp_pkt->opcode = htons(1);
	new_arp_pkt->sendip = src_ip;
	new_arp_pkt->targetip=dst_ip;

	memcpy(new_arp_pkt->sendmac, src_mac, 6);
	memcpy(new_arp_pkt->targetmac, arp_broadcat_mac, 6);

    fabric_openstack_packet_output(sw, &packout_req_info, outPort);
};


INT4 openstack_ip_install_deny_flow(gn_switch_t* sw, ip_t* ip)
{
	INT4 return_value = GN_OK;
	INT1* value = get_value(g_controller_configure, "[openvstack_conf]", "security_drop_on");
	INT4 flag_security_drop_on = (NULL == value) ? 0: atoi(value);

	if (flag_security_drop_on) {
		return_value = fabric_ip_install_deny_flow(sw, ip);
	}
	return return_value;
}

INT4 openstack_ip_remove_deny_flow(UINT1* src_mac)
{
	INT4 return_value = GN_OK;
	INT1* value = get_value(g_controller_configure, "[openvstack_conf]", "security_drop_on");
	INT4 flag_security_drop_on = (NULL == value) ? 0: atoi(value);
	
	if (flag_security_drop_on) {
		return_value = fabric_ip_remove_deny_flow(src_mac);
	}
	return return_value;
}

p_fabric_host_node openstack_save_host_info_ipv6(gn_switch_t *sw,UINT1* sendmac,UINT1* sendip,UINT4 inport)
{
	p_fabric_host_node p_node =  get_fabric_host_from_list_by_mac(sendmac);
	if(p_node!=NULL){
	}else{
		return NULL;
	}

	if ((NULL != p_node->sw) && (0 != p_node->port))
		return p_node;

	if (sendip)
		memcpy(p_node->ipv6[0], sendip, 16);
	p_node->port = inport;
	p_node->sw=sw;
	return p_node;
}

/*
 * temp added for ipv6
 * by lxf@2016.1.11
 */
#if 1
p_fabric_host_node openstack_find_ip_dst_port_ipv6(p_fabric_host_node src_node,UINT1* targetip)
{
	p_fabric_host_node dst_port=NULL;
	// find dst port
	dst_port = get_fabric_host_from_list_by_ipv6(targetip);

	return dst_port;
}

void fabric_openstack_install_fabric_flows_ipv6(p_fabric_host_node src_port,p_fabric_host_node dst_port,
										   security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	// display port info
//	LOG_PROC("INFO","Sourt Port Info:");
//	fabric_openstack_show_port(src_port);
//	LOG_PROC("INFO","Destination Port Info:");
//	fabric_openstack_show_port(dst_port);
	if ((NULL== src_port) || (NULL == dst_port) || (NULL == src_port->sw) || (NULL == dst_port->sw))
		return ;

	src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);
	if(src_port->sw == dst_port->sw){
		// printf("same switch\n");
		install_fabric_same_switch_security_flow(src_port->sw,src_port->mac,src_port->port, src_security);
		install_fabric_same_switch_security_flow(dst_port->sw,dst_port->mac,dst_port->port, dst_security);
	}else{
		// printf("different switch\n");
		install_fabric_push_tag_security_flow_ipv6(src_port->sw,dst_port->ipv6[0], dst_port->mac,dst_tag, src_security);
		install_fabric_push_tag_security_flow_ipv6(dst_port->sw,src_port->ipv6[0], src_port->mac,src_tag, src_security);
		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}

	return;
};
#endif

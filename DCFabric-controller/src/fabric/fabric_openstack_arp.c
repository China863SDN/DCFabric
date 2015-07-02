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
/*****************************
 * local function
 *****************************/
UINT1 g_broad_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
UINT4 g_broad_ip = -1;
void fabric_openstack_arp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
void fabric_openstack_arp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
void fabric_openstack_ip_broadcast_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,openstack_port_p src_port);
void fabric_openstack_dhcp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,udp_t* udp,openstack_port_p src_port);
void fabric_openstack_dhcp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,udp_t* udp,openstack_port_p src_port);
//UINT1 fabric_openstack_stand_handle();
void fabric_openstack_install_fabric_flows(openstack_port_p src_port,openstack_port_p dst_port);
void fabric_openstack_install_fabric_out_subnet_flows(openstack_port_p src_port,openstack_port_p src_gateway,openstack_port_p dst_port,openstack_port_p dst_gateway);
int check_fabric_openstack_subnet_dhcp_gateway(openstack_port_p port,openstack_subnet_p subnet);
/*****************************
 * local function : packet out
 *****************************/

void fabric_openstack_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport);
void fabric_openstack_packet_flood(packet_in_info_t *packet_in_info);
void fabric_openstack_packet_flood_in_subnet(packet_in_info_t *packet_in_info,char* subnet_id,UINT4 subnet_port_num);
void fabric_openstack_create_arp_reply(openstack_port_p src_port,openstack_port_p dst_port,packet_in_info_t *packet_in_info);

void fabric_openstack_show_port(openstack_port_p port){
	struct in_addr addr;
	char temp[16] = {0};
	memcpy(&addr, &port->ip, 4);
	mac2str(port->mac, temp);
	LOG_PROC("INFO","Tenant: %s | Network: %s | Subnet: %s  | Port: %s | IP: %s  | MAC: %s  |\n",port->tenant_id,port->network_id,port->subnet_id,port->port_id,inet_ntoa(addr),temp);

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
void fabric_openstack_arp_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	arp_t *arp = (arp_t *)(packet_in->data);
	if(arp->opcode == htons(1)){
		fabric_openstack_arp_request_handle(sw,packet_in);
	}else{
		fabric_openstack_arp_reply_handle(sw,packet_in);
	}
	return;
};
void fabric_openstack_ip_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	openstack_port_p src_port = NULL;
	openstack_port_p dst_port = NULL;
	openstack_port_p src_gateway = NULL;
	openstack_port_p dst_gateway = NULL;
	openstack_subnet_p src_subnet = NULL;
	openstack_subnet_p dst_subnet = NULL;

	ip_t *ip = (ip_t *)(packet_in->data);
//	printf("%s\n", FN);
//	printf("source ip  ");
//	fabric_openstack_show_ip(ip->src);
//	printf("destination ip  ");
//	fabric_openstack_show_ip(ip->dest);
//	printf("source mac  ");
//	fabric_openstack_show_mac(ip->eth_head.src);
//	printf("destination mac  ");
//	fabric_openstack_show_mac(ip->eth_head.dest);
//	printf("\n");

	// find the source host
	src_port = find_openstack_app_host_by_mac(ip->eth_head.src);
	if(src_port == NULL){
		LOG_PROC("INFO","IP Handle : Can't find source host!");
		return;
	}

	// update
	src_port->ip = ip->src;
	src_port->port = packet_in->inport;
	src_port->sw = sw;

	// is broadcast?
	if(/*0 ==memcmp(g_broad_mac,ip->eth_head.dest,6) || */g_broad_ip == ip->dest){
		fabric_openstack_ip_broadcast_handle(sw,packet_in,ip,src_port);
		return;
	}

	// if g_openstack_fobidden_ip
	if(ip->dest == g_openstack_fobidden_ip){
		LOG_PROC("INFO","IP Handle : Fobidden IP 169.254.169.254!");
		return;
	}

	// find dst_port ? if not, openstack has not this port
	dst_port = find_openstack_app_host_by_mac(ip->eth_head.dest);
	if(dst_port == NULL){
		LOG_PROC("INFO","IP Handle : Can't find destination host!");
		return;
	}
	// dst_port is gateway?
	src_subnet = find_openstack_app_subnet_by_subnet_id(src_port->subnet_id);
	if(dst_port->ip == src_subnet->gateway_ip){
		src_gateway = dst_port;
		// find dst_port by ip
		dst_port = find_openstack_app_host_by_ip_tenant(ip->dest,src_port->tenant_id);
		if(NULL == dst_port){
			dst_port = find_openstack_app_host_by_ip_network(ip->dest,src_port->network_id);
		}
		if(NULL == dst_port){
			//flood
			//fabric_openstack_packet_flood(packet_in);
			return;
		}

		dst_gateway = find_openstack_app_gateway_by_subnet_id(dst_port->subnet_id);
		// dst gateway is not found?
		if(dst_gateway == NULL){
			// drop flow
			return;
		}
		//change mac
		memcpy(ip->eth_head.dest,dst_port->mac,6);

		if(dst_port->sw == NULL){
			//flood
			fabric_openstack_packet_flood(packet_in);
		}else{
			//setup flow
			fabric_openstack_install_fabric_out_subnet_flows(src_port,src_gateway,dst_port,dst_gateway);
			//packet out
			fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
		}
	}else if(0 == strcmp(dst_port->subnet_id,src_port->subnet_id)){
		if(dst_port->sw == NULL){
			//flood
			fabric_openstack_packet_flood(packet_in);
		}else{
			LOG_PROC("INFO","IP Handle : SETUP FLOWS & PACKET OUT!");

			//setup flow
			if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,src_subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,src_subnet)){
				fabric_openstack_show_port(src_port);
				fabric_openstack_show_port(dst_port);
				fabric_openstack_install_fabric_flows(src_port,dst_port);
			}

			// packet out
			fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
		}
	}
	return;
};
/*****************************
 * intern function
 *****************************/
void fabric_openstack_arp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	openstack_port_p src_port = NULL;
	openstack_port_p dst_port = NULL;
	openstack_subnet_p subnet = NULL;
	arp_t *arp = (arp_t *)(packet_in->data);
//	printf("%s\n", FN);
//	printf("source ip  ");
//	fabric_openstack_show_ip(arp->sendip);
//	printf("destination ip  ");
//	fabric_openstack_show_ip(arp->targetip);
//	printf("source mac  ");
//	fabric_openstack_show_mac(arp->sendmac);
//	printf("destination mac  ");
//	fabric_openstack_show_mac(arp->targetmac);
//	printf("\n");

	src_port = find_openstack_app_host_by_mac(arp->sendmac);
	if(src_port == NULL){
		LOG_PROC("INFO","ARP Request Handle : Can't find source host!");
		return;
	}
	// update
	src_port->ip = arp->sendip;
	src_port->port = packet_in->inport;
	src_port->sw = sw;

	// find dst_port by ip (because mac maybe is the 00:00:00:00:00:00)
	dst_port = find_openstack_app_host_by_ip_subnet(arp->targetip,src_port->subnet_id);
	subnet = find_openstack_app_subnet_by_subnet_id(src_port->subnet_id);
	if(NULL == dst_port){
		// flood in subnet
		// maybe dest_port is new and dhcp,
		// but not update the ip after dhcp
		LOG_PROC("INFO","ARP Request Handle : Can't find destination host!");
		//fabric_openstack_packet_flood_in_subnet(packet_in,subnet->subnet_id,subnet->port_num);
		// if gateway no flood
		if(arp->targetip == subnet->gateway_ip){
			LOG_PROC("INFO","NO gateway,dorp!");
			return;
		}else{
			LOG_PROC("INFO","Flood in subnet!");
			fabric_openstack_packet_flood(packet_in);
		}
		return;
	}

	// create reply
	fabric_openstack_create_arp_reply(src_port,dst_port,packet_in);
	// if gateway no flood
	if(dst_port->ip == subnet->gateway_ip){
		return;
	}

	if(dst_port->sw == NULL){
		// flood
		fabric_openstack_packet_flood(packet_in);
	}else{
		LOG_PROC("INFO","ARP REQUEST Handle : SETUP FLOWS & PACKET OUT!");

		//setup flow
		// to gateway & dhcp flows is fobidden if necessary
		if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,subnet)){
			// show & check
			fabric_openstack_show_port(src_port);
			fabric_openstack_show_port(dst_port);
			fabric_openstack_install_fabric_flows(src_port,dst_port);
		}
		// packet out
		fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
	}

	return;
};
void fabric_openstack_arp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	openstack_port_p src_port = NULL;
	openstack_port_p dst_port = NULL;
	openstack_subnet_p subnet = NULL;
	arp_t *arp = (arp_t *)(packet_in->data);
//	printf("%s\n", FN);
//	printf("source ip  ");
//	fabric_openstack_show_ip(arp->sendip);
//	printf("destination ip  ");
//	fabric_openstack_show_ip(arp->targetip);
//	printf("source mac  ");
//	fabric_openstack_show_mac(arp->sendmac);
//	printf("destination mac  ");
//	fabric_openstack_show_mac(arp->targetmac);
//	printf("\n");

	src_port = find_openstack_app_host_by_mac(arp->sendmac);
	if(src_port == NULL){
		LOG_PROC("INFO","ARP Rply Handle : Can't find source host!");
		return;
	}
	// update
	src_port->ip = arp->sendip;
	src_port->port = packet_in->inport;
	src_port->sw = sw;

	// find dst_port (because mac maybe is the gateway)
	dst_port = find_openstack_app_host_by_ip_subnet(arp->targetip,src_port->subnet_id);
	if(NULL == dst_port){
		// drop
		LOG_PROC("INFO","ARP Rply Handle : Can't find destination host!");
		return;
	}

	if(dst_port->sw == NULL){
		// flood
		fabric_openstack_packet_flood(packet_in);
	}else{
		LOG_PROC("INFO","ARP RPLAY Handle : SETUP FLOWS & PACKET OUT!");

		subnet =find_openstack_app_subnet_by_subnet_id(src_port->subnet_id);
		// download flows
		if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,subnet)){
			fabric_openstack_show_port(src_port);
			fabric_openstack_show_port(dst_port);
			fabric_openstack_install_fabric_flows(src_port,dst_port);
		}
		// packet out
		fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
	}

	return;
};
/*
 * ip broad cast
 */
void fabric_openstack_ip_broadcast_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,openstack_port_p src_port){
	udp_t* udp = NULL;
//	printf("%s\n", FN);
	if(ip->proto == IPPROTO_UDP){
		udp = (udp_t*)(ip->data);
//		printf("UDP! udp->sport : %u | udp->dport : %u \n",udp->sport,udp->dport);
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
/*
 * dhcp request
 */
void fabric_openstack_dhcp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,udp_t* udp,openstack_port_p src_port){
	openstack_port_p dhcp_port = NULL;
//	printf("%s\n", FN);
	// find dhcp
	dhcp_port = find_openstack_app_dhcp_by_subnet_id(src_port->subnet_id);

	if(dhcp_port == NULL || dhcp_port->port == 0){
		// flood
		fabric_openstack_packet_flood(packet_in);
	}else{
		// packet out
		fabric_openstack_packet_output(dhcp_port->sw,packet_in,dhcp_port->port);
	}
	return;
};
/*
 * dhcp reply
 */
void fabric_openstack_dhcp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,udp_t* udp,openstack_port_p src_port){
	openstack_port_p dst_port = NULL;
	dhcp_t* dhcp = NULL;
	UINT1 mac[6];
//	printf("%s\n", FN);
	// find dst
	dhcp = (dhcp_t*)(udp->data);
	memcpy(mac,dhcp->cmcaddr,6);

	dst_port = find_openstack_app_host_by_mac(mac);
	if(dst_port == NULL || dst_port->port == 0){
		// flood
		fabric_openstack_packet_flood(packet_in);
	}else{
		// packet out
		fabric_openstack_packet_output(dst_port->sw,packet_in,dst_port->port);
	}
	return;
};

void fabric_openstack_install_fabric_flows(openstack_port_p src_port,openstack_port_p dst_port){
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);
	if(src_port->sw == dst_port->sw){
		install_fabric_same_switch_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_same_switch_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}else{
		install_fabric_push_tag_flow(src_port->sw,dst_port->mac,dst_tag);
		install_fabric_push_tag_flow(dst_port->sw,src_port->mac,src_tag);
		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}

	return;
};
void fabric_openstack_install_fabric_out_subnet_flows(openstack_port_p src_port,openstack_port_p src_gateway,openstack_port_p dst_port,openstack_port_p dst_gateway){
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);
	if(src_port->sw == dst_port->sw){
		install_fabric_same_switch_out_subnet_flow(src_port->sw,src_gateway->mac,dst_port->mac,dst_port->ip,dst_port->port);
		install_fabric_same_switch_out_subnet_flow(dst_port->sw,dst_gateway->mac,src_port->mac,src_port->ip,src_port->port);
	}else{
		install_fabric_push_tag_out_subnet_flow(src_port->sw,src_gateway->mac,dst_port->mac,dst_port->ip,dst_tag);
		install_fabric_push_tag_out_subnet_flow(dst_port->sw,dst_gateway->mac,src_port->mac,src_port->ip,src_tag);
		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}
	return;
};
int check_fabric_openstack_subnet_dhcp_gateway(openstack_port_p port,openstack_subnet_p subnet){
	return ((port == subnet->dhcp_port) || (port == subnet->gateway_port))?1:0;
};
/*****************************
 * intern function: packet out
 *****************************/
/*
 * out put the packet
 */
void fabric_openstack_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport){
	packout_req_info_t pakout_req;
	pakout_req.buffer_id = packet_in_info->buffer_id;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.outport = outport;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;
	sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
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
	openstack_port_p port_list[subnet_port_num+3];
	openstack_port_p temp = NULL;
	UINT4 i = 0,port_num = 0;
	packout_req_info_t pakout_req;
	gn_switch_t *sw = NULL;

	pakout_req.buffer_id = 0xffffffff;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;


	port_num = find_openstack_app_hosts_by_subnet_id(subnet_id,port_list);
	for(i = 0 ; i < port_num ; i++){
		temp = port_list[i];
		if(temp != NULL && temp->port != 0 && temp->sw != NULL){
			sw = temp->sw;
			sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
		}
	}
	return;
};

void fabric_openstack_create_arp_reply(openstack_port_p src_port,openstack_port_p dst_port,packet_in_info_t *packet_in_info){
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
    new_arp_pkt.sendip = dst_port->ip;
    new_arp_pkt.targetip = src_port->ip;
    memcpy(new_arp_pkt.sendmac, dst_port->mac, 6);
    memcpy(new_arp_pkt.targetmac, src_port->mac, 6);

    src_port->sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](src_port->sw, (UINT1 *)&packout_req_info);
};

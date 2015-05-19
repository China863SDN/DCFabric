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
 * fabric_arp.c
 *
 *  Created on: Apr 2, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#include "gnflush-types.h"
#include "fabric_arp.h"
#include "fabric_host.h"
#include "fabric_impl.h"
#include "fabric_flows.h"
#include "gn_inet.h"
#include "openflow-10.h"
#include "openflow-13.h"

/*****************************
 * local function
 *****************************/
void fabric_arp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
void fabric_arp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in);

void fabric_push_arp_flood_queue(UINT4 ip,arp_t* arp_info,packet_in_info_t *packet_in);
void fabric_push_ip_flood_queue(UINT4 ip,ip_t* arp_info,packet_in_info_t *packet_in);
void fabric_push_flow_queue(p_fabric_host_node src,p_fabric_host_node dst);

void fabric_create_arp_reply(p_fabric_host_node src,p_fabric_host_node dst,packet_in_info_t *packet_in_info);

void fabric_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport);

void fabric_ip_packet_flood(packet_in_info_t *packet_in_info);

//void fabric_push_host_queue(gn_switch_t* sw,UINT4 port,UINT1* mac,UINT4 ip);

/*****************************
 * global variables
 *****************************/
// switch server
//extern gn_server_t g_server;
extern sem_t fabric_arp_flood_sem;
extern sem_t fabric_flow_sem;
extern sem_t fabric_ip_flood_sem;

void fabric_arp_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	arp_t *arp = (arp_t *)(packet_in->data);
	if(arp->opcode == htons(1)){
		fabric_arp_request_handle(sw,packet_in);
	}else{
		fabric_arp_reply_handle(sw,packet_in);
	}
	return;
};
void fabric_ip_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	p_fabric_host_node src = NULL,dst=NULL;
	ip_t *ip = (ip_t *)(packet_in->data);
	src = get_fabric_host_from_list_by_ip(ip->src);
	if(src == NULL){
//		printf("%s : add ip:%s\n",FN,inet_htoa(ntohl(ip->src)));
		src = create_fabric_host_list_node(sw,packet_in->inport,ip->eth_head.src,ip->src);
		insert_fabric_host_into_list(src);
		// install output flow table 3

		install_fabric_output_flow(sw,ip->eth_head.src,packet_in->inport);
	}else if(src->port != packet_in->inport){
		// update host inport
		src->port = packet_in->inport;
		// install output flow table 3
		install_fabric_output_flow(sw,ip->eth_head.src,packet_in->inport);
	}

	dst = get_fabric_host_from_list_by_ip(ip->dest);
	if(dst != NULL){
//		char *ip_src = strdup(inet_htoa(ntohl(ip->src)));
//		char *ip_dst = strdup(inet_htoa(ntohl(ip->dest)));
//
//		printf("ip->proto == %d src: %s | dst:%s\n",ip->proto, ip_src,ip_dst);
//		free(ip_src);
//		free(ip_dst);

		// download flows
		fabric_push_flow_queue(src,dst);

		// pecket out
		fabric_packet_output(dst->sw,packet_in,dst->port);
	}
	else if(ip->proto == 1){
//		printf("ip->proto == 1 %s\n",FN);
		fabric_ip_packet_flood(packet_in);
		//fabric_push_ip_flood_queue(ip->dest,ip,packet_in);
	}
	return;
};
/*****************************
 * intern function : handle packet
 *****************************/
void fabric_arp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	p_fabric_host_node src = NULL,dst=NULL;
	p_fabric_arp_request_node arp_node= NULL;
	//packout_req_info_t packout_req = NULL;
	arp_t *arp = (arp_t *)(packet_in->data);
	src = get_fabric_host_from_list_by_ip(arp->sendip);
	if(src == NULL){
//		printf("%s : add ip:%s\n",FN,inet_htoa(ntohl(arp->sendip)));
		src = create_fabric_host_list_node(sw,packet_in->inport,arp->sendmac,arp->sendip);
		insert_fabric_host_into_list(src);
		// install output flow table 3
		install_fabric_output_flow(sw,arp->eth_head.src,packet_in->inport);
	}else if(src->port != packet_in->inport){
		// update host inport
		src->port = packet_in->inport;
		// install output flow table 3
		install_fabric_output_flow(sw,arp->eth_head.src,packet_in->inport);
	}

	dst = get_fabric_host_from_list_by_ip(arp->targetip);
	if(dst == NULL){
		// add to arp queue
		arp_node = create_fabric_arp_request_list_node(src,arp->targetip);
		insert_fabric_arp_request_into_list(arp_node);

		// flood to outter ports
		fabric_push_arp_flood_queue(arp->targetip,arp,packet_in);
	}else{
		// download flows
		fabric_push_flow_queue(src,dst);
		// create reply
		fabric_create_arp_reply(src,dst,packet_in);
	}
	return;
};
void fabric_arp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	p_fabric_host_node src = NULL,dst=NULL;
	p_fabric_arp_request_node temp_node = NULL;

	arp_t *arp = (arp_t *)(packet_in->data);
	src = get_fabric_host_from_list_by_ip(arp->sendip);
	if(src == NULL){
//		printf("%s : add ip:%s\n",FN,inet_htoa(ntohl(arp->sendip)));
		src = create_fabric_host_list_node(sw,packet_in->inport,arp->sendmac,arp->sendip);
		insert_fabric_host_into_list(src);
		// install output flow table 3
		install_fabric_output_flow(sw,arp->eth_head.src,packet_in->inport);
	}else if(src->port != packet_in->inport){
		// update host inport
		src->port = packet_in->inport;
		// install output flow table 3
		install_fabric_output_flow(sw,arp->eth_head.src,packet_in->inport);
	}

	temp_node = remove_fabric_arp_request_from_list_by_dstip(src->ip);
	while(temp_node != NULL){
//		printf("%s : src ip:%d| dst ip:%d\n",FN,arp->sendip,arp->targetip);
		dst = temp_node->src_req;
		// add fabric flow
		fabric_push_flow_queue(src,dst);
		// create fabric arp reply
		memcpy(arp->eth_head.dest,dst->mac, 6);
		arp->targetip = dst->ip;
		memcpy(arp->targetmac,dst->mac, 6);
		fabric_packet_output(dst->sw,packet_in,dst->port);

		// delete arp request node
		temp_node = delete_fabric_arp_request_list_node(temp_node);
	}

	return;
};
/*****************************
 * intern function
 *****************************/
/*
 * push arp flood queue
 */
void fabric_push_arp_flood_queue(UINT4 ip,arp_t* arp_info,packet_in_info_t *packet_in_info){
	p_fabric_arp_flood_node node = NULL;
	node = get_fabric_arp_flood_from_queue_by_ip(ip);
	if(node == NULL){
		node = create_fabric_arp_flood_node(packet_in_info,ip);
		push_fabric_arp_flood_into_queue(node);
		sem_post(&fabric_arp_flood_sem);
	}
	return;
};

/*
 * push ip flood queue
 */
void fabric_push_ip_flood_queue(UINT4 ip,ip_t* ip_info,packet_in_info_t *packet_in){
	p_fabric_ip_flood_node node = NULL;
	node = get_fabric_ip_flood_from_queue_by_ip(ip);
	if(node == NULL){
		node = create_fabric_ip_flood_node(packet_in,ip);
		push_fabric_ip_flood_into_queue(node);
		sem_post(&fabric_ip_flood_sem);
	}
	return;
};
/*
 * push fabric flow queue
 */
void fabric_push_flow_queue(p_fabric_host_node src,p_fabric_host_node dst){
	UINT4 src_tag = 0,dst_tag = 0;
	p_fabric_flow_node node = NULL;

	node = get_fabric_flow_from_queue_by_ip(src,dst);
	if(node == NULL){
		src_tag = of131_fabric_impl_get_tag_sw(src->sw);
		dst_tag = of131_fabric_impl_get_tag_sw(dst->sw);
		node = create_fabric_flow_node(src,src_tag,dst,dst_tag);
		push_fabric_flow_into_queue(node);
		sem_post(&fabric_flow_sem);
	}
	return;
};
/*
 * create a reply to arp request host
 */
void fabric_create_arp_reply(p_fabric_host_node src,p_fabric_host_node dst,packet_in_info_t *packet_in_info){
    packout_req_info_t packout_req_info;
    arp_t new_arp_pkt;
    arp_t *arp = (arp_t *)(packet_in_info->data);

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = OFPP13_CONTROLLER;
    packout_req_info.outport = src->port;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = packet_in_info->xid;
    packout_req_info.data_len = sizeof(arp_t);
    packout_req_info.data = (UINT1 *)&new_arp_pkt;

    memcpy(&new_arp_pkt, arp, sizeof(arp_t));
    memcpy(new_arp_pkt.eth_head.src, dst->mac, 6);
    memcpy(new_arp_pkt.eth_head.dest, src->mac, 6);
    new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
    new_arp_pkt.opcode = htons(2);
    new_arp_pkt.sendip = dst->ip;
    new_arp_pkt.targetip = src->ip;
    memcpy(new_arp_pkt.sendmac, dst->mac, 6);
    memcpy(new_arp_pkt.targetmac, src->mac, 6);

    src->sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](src->sw, (UINT1 *)&packout_req_info);
};
/*
 * out put the packet
 */
void fabric_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport){
//    printf("%s\n", FN);
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
 * flood the packet to each port without switch intern ports
 */
void fabric_ip_packet_flood( packet_in_info_t *packet_in_info){
	packout_req_info_t pakout_req;
	gn_switch_t *sw = NULL;
	UINT2 i = 0,j=0;
//	ip_t *ip = (ip_t *)(packet_in_info->data);
//printf("%s\n", FN);
	pakout_req.buffer_id = 0xffffffff;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;

//	pakout_req.outport = OFPP13_FLOOD;
//	memset(ip->eth_head.src,0,6);

	// find all switch
	for(i = 0; i < g_server.max_switch; i++){
		if (g_server.switches[i].state){
			sw = &g_server.switches[i];
			// find switch's outter ports
			for(j=0; j<sw->n_ports; j++){
				// check port state is ok and also not connect other switch(neighbor)
				if(sw->ports[j].state == 0 && sw->neighbor[j] == NULL){
					pakout_req.outport = sw->ports[j].port_no;
					sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
				}
			}
			//sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
		}
	}

	return;
};

/////////////////////////////////////////////////////////////////////////////////
///*
// * flood the packet to each port without switch intern ports
// */
//void fabric_paket_flood( packet_in_info_t *packet_in_info){
//	packout_req_info_t pakout_req;
//	gn_switch_t *sw = NULL;
//	UINT2 i = 0,j=0;
////	arp_t new_arp_pkt;
//
//	pakout_req.buffer_id = packet_in_info->buffer_id;
//	//pakout_req.buffer_id = 0xffffffff;
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
//			// find switch's outter ports
//			for(j=0; j<sw->n_ports; j++){
//				// check port state is ok and also not connect other switch(neighbor)
////				if(sw->ports[j].neighbour == NULL){
//				if( 0 == check_is_neighbor_port(sw,sw->ports[j].port_no)){
////					printf("Port no:%d\n",sw->ports[j].port_no);
////					temp_fabric_paket_output(sw,packet_in_info,sw->ports[j].port_no);
//					pakout_req.outport = sw->ports[j].port_no;
//					//pakout_req.buffer_id++;
//					sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
//				}
//			}
//		}
//	}
//
//	return;
//};
//UINT1 check_is_neighbor_port(gn_switch_t* sw,UINT4 port){
//	UINT2 i = 0,j=0;
//	gn_switch_t* neighbor = NULL;
//	for(i=0;i < sw->n_ports;i++){
//		// get neighbor switch
//		if(sw->neighbor[i] != NULL && sw->neighbor[i]->sw != NULL){
//			neighbor = sw->neighbor[i]->sw;
//			for(j=0;j<neighbor->n_ports;j++){
//				if(neighbor->neighbor[j] != NULL && neighbor->neighbor[j]->sw == sw){
//					if(neighbor->neighbor[j]->port->port_no == port){
//						return 1;
//					}
//				}
//			}
//		}
//	}
//	return 0;
//}
//

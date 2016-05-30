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
#include "fabric_impl.h"
#include "fabric_flows.h"
#include "gn_inet.h"
#include "openflow-10.h"
#include "openflow-13.h"

/*****************************
 * local function
 *****************************/
//void fabric_arp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
//void fabric_arp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in);

void fabric_push_ip_flood_queue(UINT4 ip,ip_t* arp_info,packet_in_info_t *packet_in);
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

//void fabric_arp_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
//	arp_t *arp = (arp_t *)(packet_in->data);
//	if(arp->opcode == htons(1)){
//		fabric_arp_request_handle(sw,packet_in);
//	}else{
//		fabric_arp_reply_handle(sw,packet_in);
//	}
//	return;
//};
//added by xuyanwei at 2015-08-13
void fabric_vlan_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	printf("%s need to be evaluated in the future !!!!\n",FN);
	return ;
}
void fabric_ip_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
	//	printf("%s start \n",FN);
	p_fabric_host_node src = NULL,dst=NULL;
	ip_t *ip = (ip_t *)(packet_in->data);
	src = get_fabric_host_from_list_by_ip(ip->src);
	if(src == NULL){
//		printf("%s : src=null, add ip:%s\n",FN,inet_htoa(ntohl(ip->src)));
		src = create_fabric_host_list_node(sw,packet_in->inport,ip->eth_head.src,ip->src,NULL);
		insert_fabric_host_into_list(src);
		// install output flow table 3

		install_fabric_output_flow(sw,ip->eth_head.src,packet_in->inport);
	}else
	{
//		printf("%s : src!=null, ip:%s\n",FN,inet_htoa(ntohl(ip->src)));
		if(src->port != packet_in->inport)
		{
			// update host inport
			src->port = packet_in->inport;
			// install output flow table 3
			install_fabric_output_flow(sw,ip->eth_head.src,packet_in->inport);
		}
	}

	dst = get_fabric_host_from_list_by_ip(ip->dest);
	if(dst != NULL){
//		char *ip_src = strdup(inet_htoa(ntohl(ip->src)));
//		char *ip_dst = strdup(inet_htoa(ntohl(ip->dest)));
//
		//		printf("%s: ip->proto == %d src: %s | dst:%s\n",FN,ip->proto, ip_src,ip_dst);
//		free(ip_src);
//		free(ip_dst);

		// download flows
		fabric_push_flow_queue(src,ip->src,dst,ip->dest);

		// pecket out
		fabric_packet_output(dst->sw,packet_in,dst->port);
	}
	// icmp packet that not found destination flood
	else if(ip->proto == 1){
		printf("ip->proto == 1 %s\n",FN);
		fabric_ip_packet_flood(packet_in);
		//fabric_push_ip_flood_queue(ip->dest,ip,packet_in);
	}
	//	printf("%s end \n",FN);
	return;
};


p_fabric_host_node fabric_save_host_info(gn_switch_t *sw,UINT1* sendmac,UINT4 sendip,UINT4 inport){
	p_fabric_host_node p_node =  get_fabric_host_from_list_by_mac(sendmac);
	if(p_node!=NULL){
		if(!check_IP_in_fabric_host(p_node,sendip))
		{
			add_fabric_host_ip(p_node,sendip);
		}
	}else{
		p_node = create_fabric_host_list_node(sw,inport,sendmac,sendip,NULL);
		if (NULL == p_node) {
		    return NULL;	
		}
		insert_fabric_host_into_list(p_node);
        install_fabric_output_flow(sw,sendmac,inport);
	}
    
	if ((NULL != p_node->sw) && (0 != p_node->port))
    {   
	    return p_node;
    }
        
	p_node->port = inport;
	p_node->sw=sw;
    if (sendip)
    {
        p_node->ip_list[0] = sendip;
    }
	install_fabric_output_flow(sw,sendmac,inport);
	return p_node;
}

p_fabric_host_node fabric_find_dst_port(p_fabric_host_node src_node,UINT4 targetip){
	p_fabric_host_node p_node = get_fabric_host_from_list_by_ip(targetip);
	return p_node;
}

p_fabric_host_node fabric_find_dst_port_ip(p_fabric_host_node src_node,UINT4 targetip)
{
	p_fabric_host_node p_node = get_fabric_host_from_list_by_ip(targetip);
	return p_node;
}

INT4 fabric_arp_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in)
{
	fabric_add_into_arp_request(src_port, sendip, targetip);
	
	// flood to outter ports
	fabric_push_arp_flood_queue(targetip,packet_in);
	return GN_OK;
}
INT4 fabric_ip_p_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,UINT1* srcmac,packet_in_info_t *packet_in){
	ip_t *p_ip = (ip_t *)(packet_in->data);
	if(p_ip->proto == 1){
		printf("ip->proto == 1 %s\n",FN);
		fabric_ip_packet_flood(packet_in);
		//fabric_push_ip_flood_queue(ip->dest,ip,packet_in);
	}
	return GN_OK;
}
INT4 fabric_ip_p_install_flow(param_set_p param, INT4 forward_type)
{
	if (NULL != param)  {
		if ((NULL != param->src_port) && (NULL != param->dst_port)) {
			// install_fabric_output_flow(param->src_port->sw, param->src_port->mac, param->src_port->port);
			fabric_push_flow_queue(param->src_port, param->src_port->ip_list[0], param->dst_port, param->dst_port->ip_list[0]);
		}

//		if (NULL != param->dst_port)
//			install_fabric_output_flow(param->dst_port->sw, param->dst_port->mac, param->dst_port->port);
	}
	return GN_OK;
}
INT4 fabric_arp_remove_ip_from_flood_list(UINT4 sendip){
	p_fabric_arp_request_node temp_node = remove_fabric_arp_request_from_list_by_dstip(sendip);
	if(temp_node!=NULL){
		temp_node = delete_fabric_arp_request_list_node(temp_node);
	}
	return GN_OK;
}

INT4 fabric_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in){
	// download flows
	fabric_push_flow_queue(src_port,sendip,dst_port,targetip);
	// create reply
	fabric_create_arp_reply(src_port,dst_port,packet_in);
	return GN_OK;
}
INT4 fabric_ip_packet_output(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in){
	// download flows
	fabric_push_flow_queue(src_port,sendip,dst_port,targetip);

	// pecket out
	fabric_packet_output(dst_port->sw,packet_in,dst_port->port);
	return GN_OK;
}
INT4 fabric_ip_packet_check_access(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in, param_set_p param_set){
	return GN_OK;
}

INT4 fabric_compute_src_dst_forward(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in, param_set_p param)
{
	if (NULL != dst_port)
	{
		// pecket out
		return Internal_port_flow;
	}
	else {
		return IP_FLOOD;
	}

    return GN_OK;
}
INT4 fabric_arp_reply_output(p_fabric_host_node src,p_fabric_host_node dst,UINT4 targetIP, packet_in_info_t *packet_in){
	arp_t *arp = (arp_t *)(packet_in->data);
	fabric_push_flow_queue(src,arp->sendip, dst, targetIP);
	memcpy(arp->eth_head.dest,dst->mac, 6);
	arp->targetip = targetIP;
	memcpy(arp->targetmac,dst->mac, 6);
	fabric_packet_output(dst->sw,packet_in,dst->port);
	return GN_OK;
}

/*****************************
 * intern function : handle packet
 *****************************/
//void fabric_arp_request_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
//	//	printf("%s start \n",FN);
//	p_fabric_host_node src = NULL,dst=NULL;
//	p_fabric_arp_request_node arp_node= NULL;
//	//packout_req_info_t packout_req = NULL;
//	arp_t *arp = (arp_t *)(packet_in->data);
//	//src = get_fabric_host_from_list_by_ip(arp->sendip);
//	src = get_fabric_host_from_list_by_mac(arp->sendmac);
//	if(src == NULL){
////		printf("%s : src=null,add ip:%s\n",FN,inet_htoa(ntohl(arp->sendip)));
//		src = create_fabric_host_list_node(sw,packet_in->inport,arp->sendmac,arp->sendip);
//		insert_fabric_host_into_list(src);
//		// install output flow table 3
//		install_fabric_output_flow(sw,arp->eth_head.src,packet_in->inport);
//	}else
//		{
//			//printf("%s : src!=null sender ip:%s\n",FN,inet_htoa(ntohl(arp->sendip)));
//			if(!check_IP_in_fabric_host(src,arp->sendip))
//			{
//				add_fabric_host_ip(src,arp->sendip);
//			//	printf("%s : new ip:%s \n",FN ,inet_htoa(ntohl(arp->sendip)));
//			}
//			 if(src->port != packet_in->inport)
//			 {
//				 src->port = packet_in->inport;
//				 src->sw=sw;
//				 // install output flow table 3
//				 install_fabric_output_flow(sw,arp->eth_head.src,packet_in->inport);
//			 }
//		}
//	dst = get_fabric_host_from_list_by_ip(arp->targetip);
//	if(dst == NULL){
//		//printf("%s : dst=null target ip:%s\n",FN,inet_htoa(ntohl(arp->targetip)));
//
//		// add to arp queue
//		arp_node = create_fabric_arp_request_list_node(src,arp->sendip,arp->targetip);
//		insert_fabric_arp_request_into_list(arp_node);
//		///printf("%s : arp_node.targetip:%s\n",FN,inet_htoa(ntohl(arp_node->dst_IP)));
//
//		// flood to outter ports
//		fabric_push_arp_flood_queue(arp->targetip,packet_in);
//	}else
//	{
//		//printf("%s : dst!=null target ip:%s\n",FN,inet_htoa(ntohl(arp->targetip)));
//		// download flows
//		fabric_push_flow_queue(src,arp->sendip,dst,arp->targetip);
//		// create reply
//		fabric_create_arp_reply(src,dst,packet_in);
//	}
//	//printf("%s end \n",FN);
//	return;
//};

//void fabric_arp_reply_handle(gn_switch_t *sw, packet_in_info_t *packet_in){
//	//printf("%s start \n",FN);
//	p_fabric_host_node src = NULL,dst=NULL;
//	p_fabric_arp_request_node temp_node = NULL;
//
//	arp_t *arp = (arp_t *)(packet_in->data);
//	//src = get_fabric_host_from_list_by_ip(arp->sendip);
//	src = get_fabric_host_from_list_by_mac(arp->sendmac);
//	if(src == NULL){
////		printf("%s :  src=null add sender ip:%s\n",FN,inet_htoa(ntohl(arp->sendip)));
//		src = create_fabric_host_list_node(sw,packet_in->inport,arp->sendmac,arp->sendip);
//		insert_fabric_host_into_list(src);
//		// install output flow table 3
//		install_fabric_output_flow(sw,arp->eth_head.src,packet_in->inport);
//
//	}
//
//	else
//	{
//		//printf("%s : src!=null sender ip:%s\n",FN,inet_htoa(ntohl(arp->sendip)));
//		if(src->port != packet_in->inport)
//		{
//			// update host inport
//			src->port = packet_in->inport;
//			src->sw=sw;
//			// install output flow table 3
//			install_fabric_output_flow(sw,arp->eth_head.src,packet_in->inport);
//		}
//		if(!check_IP_in_fabric_host(src,arp->sendip))
//		{
//			add_fabric_host_ip(src,arp->sendip);
//			//printf("%s :  new ip:%s \n",FN,inet_htoa(ntohl(arp->sendip)));
//		}
//	}
//	//  temp_node = remove_fabric_arp_request_from_list_by_dstip(src->ip);
//	  temp_node = remove_fabric_arp_request_from_list_by_dstip(arp->sendip);
//      while(temp_node != NULL)
//	  {
//			//printf("%s : arp requested ip: %s\n",FN,inet_htoa(ntohl(temp_node->src_IP)));
//			//printf("%s : arpreply.targetip:%s\n",FN,inet_htoa(ntohl( arp->targetip)));
//		dst = temp_node->src_req;
//		// add fabric flow
//		fabric_push_flow_queue(src,arp->sendip, dst, temp_node->src_IP);
//		// create fabric arp reply
//		memcpy(arp->eth_head.dest,dst->mac, 6);
//		// arp->targetip =src->ip;
//	    arp->targetip = temp_node->src_IP;
//		memcpy(arp->targetmac,dst->mac, 6);
//		fabric_packet_output(dst->sw,packet_in,dst->port);
//
//		// delete arp request node
//		temp_node = delete_fabric_arp_request_list_node(temp_node);
//	}
//  	//printf("%s end \n",FN);
//	return;
//};
/*****************************
 * intern function
 *****************************/
/*
 * push arp flood queue
 */
void fabric_push_arp_flood_queue(UINT4 targetIP,packet_in_info_t *packet_in_info){
	p_fabric_arp_flood_node node = NULL;
	node = get_fabric_arp_flood_from_queue_by_ip(targetIP);
	if(node == NULL){
		node = create_fabric_arp_flood_node(packet_in_info,targetIP);
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
void fabric_push_flow_queue(p_fabric_host_node src,UINT4 src_IP,p_fabric_host_node dst,UINT4 dst_IP){
	UINT4 src_tag = 0,dst_tag = 0;
	p_fabric_flow_node node = NULL;

	 node = get_fabric_flow_from_queue(src,src_IP,dst,dst_IP);
	if(node == NULL){
		if ((NULL != src->sw) && (NULL != dst->sw)){
			src_tag = of131_fabric_impl_get_tag_sw(src->sw);
			dst_tag = of131_fabric_impl_get_tag_sw(dst->sw);
			node = create_fabric_flow_node(src,src_IP, src_tag,dst,dst_IP, dst_tag);
			push_fabric_flow_into_queue(node);
			sem_post(&fabric_flow_sem);
		}
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
    //new_arp_pkt.sendip = dst->ip;
    new_arp_pkt.sendip = arp->targetip ;

    //new_arp_pkt.targetip = src->ip;
    new_arp_pkt.targetip = arp->sendip;
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
void fabric_ip_packet_flood( packet_in_info_t *packet_in_info)
{
	packout_req_info_t pakout_req;
	gn_switch_t *sw = NULL;
	UINT2 i = 0,j=0;
//	ip_t *ip = (ip_t *)(packet_in_info->data);
//	printf("%s\n", FN);
	pakout_req.buffer_id = 0xffffffff;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;

//	pakout_req.outport = OFPP13_FLOOD;
//	memset(ip->eth_head.src,0,6);

	// find all switch
	for(i = 0; i < g_server.max_switch; i++)
	{
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
}

INT4 fabric_ip_install_deny_flow(gn_switch_t *sw, ip_t* p_ip)
{
	UINT4 src_ip = 0;
	UINT4 dst_ip = 0;
	UINT1 src_mac[6] = {0};
	UINT1 dst_mac[6] = {0};
	UINT1 proto = 0;
	icmp_t* p_icmp = NULL;
	tcp_t* p_tcp = NULL;
	udp_t* p_udp = NULL;
	UINT1 icmp_type = 0;
	UINT1 icmp_code = 0;
	UINT2 sport = 0;
	UINT2 dport = 0;
	
	if (NULL == p_ip) {
		return GN_ERR;
	}	

	src_ip = p_ip->src;
	dst_ip = p_ip->dest;
	memcpy(src_mac, p_ip->eth_head.src, 6);
	memcpy(dst_mac, p_ip->eth_head.dest, 6);
	proto = p_ip->proto;

	switch(proto)
	{
	case IPPROTO_ICMP:
		{
			p_icmp = (icmp_t*)p_ip->data;
			if (p_icmp) {
				icmp_type = p_icmp->type;
				icmp_code = p_icmp->code;
			}
		}
		break;
	case IPPROTO_TCP:
		{
			p_tcp = (tcp_t*)p_ip->data;
			if (p_tcp) {
				sport = p_tcp->sport;
				dport = p_tcp->dport;
			}
		}
		break;
	case IPPROTO_UDP:
		{
			p_udp = (udp_t*)p_ip->data;
			if (p_udp) {
				sport = p_udp->sport;
				dport = p_udp->dport;
			}
		}
		break;
	default:
		break;
	}
	
	install_deny_flow(sw, src_ip, dst_ip, src_mac, dst_mac, proto, icmp_type, icmp_code, sport, dport);

	return GN_OK;

}

INT4 fabric_ip_remove_deny_flow(UINT1* src_mac)
{
	p_fabric_host_node host = get_fabric_host_from_list_by_mac(src_mac);
	if (host) {
		gn_switch_t* sw = host->sw;
		if (sw) {
			remove_deny_flow(sw, src_mac);
		}
	}
	
	return GN_OK;
}



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

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
 * fabric_thread.c
 *
 *  Created on: Apr 5, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#include "fabric_thread.h"
#include "fabric_flows.h"
#include "gnflush-types.h"
#include "fabric_host.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "gn_inet.h"
#include <pthread.h>
#include <semaphore.h>
/*****************************
 * intern functions
 *****************************/
/*
 * flood thread
 */
void* fabric_arp_flood_thread();
void* fabric_ip_flood_thread();
void fabric_packet_flood( packet_in_info_t *packet_in_info);

/*
 * flow thread
 */
void* fabric_flow_thread();
/*
 * host thread
 */
void* fabric_host_thread();
//void* fabric_arp_request_thread();
//void* fabric_arp_reply_thread();
//void* fabric_ip_thread();
//UINT1 check_is_neighbor_port(gn_switch_t* sw,UINT4 port);
//void temp_fabric_paket_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport);
/*****************************
 * global variables
 *****************************/
static UINT1 g_fabric_arp_flood_flag;
pthread_t p_fabric_arp_flood_thread_id;
sem_t fabric_arp_flood_sem;

static UINT1 g_fabric_ip_flood_flag;
pthread_t p_fabric_ip_flood_thread_id;
sem_t fabric_ip_flood_sem;

static UINT1 g_fabric_flow_flag;
pthread_t p_fabric_flow_thread_id;
sem_t fabric_flow_sem;

// static UINT1 g_fabric_host_flag;
pthread_t p_fabric_host_thread_id;
sem_t fabric_host_sem;

/*****************************
 * interface functions
 *****************************/
void start_fabric_thread(){

	init_fabric_arp_request_list();
	init_fabric_host_list();
	// start arp flood thread
	g_fabric_arp_flood_flag = 1;
	sem_init(&fabric_arp_flood_sem,0,0);
	pthread_create(&p_fabric_arp_flood_thread_id,NULL,fabric_arp_flood_thread,NULL);

//	// start ip flood thread
//	g_fabric_ip_flood_flag = 1;
//	sem_init(&fabric_ip_flood_sem,0,0);
//	pthread_create(&p_fabric_ip_flood_thread_id,NULL,fabric_ip_flood_thread,NULL);

	// start flow thread
	g_fabric_flow_flag = 1;
	sem_init(&fabric_flow_sem,0,0);
	pthread_create(&p_fabric_flow_thread_id,NULL,fabric_flow_thread,NULL);

//	g_fabric_host_flag = 1;
//	sem_init(&fabric_host_sem,0,0);
//	pthread_create(&p_fabric_host_thread_id,NULL,fabric_host_thread,NULL);

	return;
};
void stop_fabric_thread(){

	// clear fabric_arp_request_list;
	destroy_fabric_arp_request_list();
	// clear fabric_host_list;
	destroy_fabric_host_list();

	// stop arp flood thread
	g_fabric_arp_flood_flag = 0;
	sem_post(&fabric_arp_flood_sem);
	if(p_fabric_arp_flood_thread_id)
	    pthread_detach(p_fabric_arp_flood_thread_id);

//	// stop ip flood thread
//	g_fabric_ip_flood_flag = 0;
//	sem_post(&fabric_ip_flood_sem);
//	if(p_fabric_ip_flood_thread_id)
//	    pthread_detach(p_fabric_ip_flood_thread_id);


	// stop flow thread
	g_fabric_flow_flag = 0;
	sem_post(&fabric_flow_sem);
    if(p_fabric_flow_thread_id)
        pthread_detach(p_fabric_flow_thread_id);

//	g_fabric_host_flag = 0;
//	sem_post(&fabric_host_sem);
//	if(p_fabric_host_thread_id)
//		pthread_detach(p_fabric_host_thread_id);
	return;
};
/*****************************
 * intern functions: thread functions
 *****************************/
/*
 * fabric arp flood thread
 */
void* fabric_arp_flood_thread(){
	p_fabric_arp_flood_node flood_node = NULL;
	packet_in_info_t *packet_in = NULL;

	// initialize the flood queue
	init_fabric_arp_flood_queue();

	LOG_PROC("INFO","fabric_flood_thread init!");
	while(g_fabric_arp_flood_flag){
		sem_wait(&fabric_arp_flood_sem);
		while(is_fabric_arp_flood_queue_empty()){
			flood_node = get_head_fabric_arp_flood_from_queue();
			if(flood_node != NULL){
				packet_in = &flood_node->packet_in_info;
				fabric_packet_flood(packet_in);
			}
			flood_node = pop_fabric_arp_flood_from_queue();
			delete_fabric_arp_flood_node(flood_node);
		}
	}
	// Destroy the flood queue
	destory_fabric_arp_flood_queue();
	LOG_PROC("INFO","fabric_flood_thread end!");
	return NULL;
};

/*
 * fabric ip flood thread
 */
void* fabric_ip_flood_thread(){
	p_fabric_ip_flood_node flood_node = NULL;
	packet_in_info_t *packet_in = NULL;

	// initialize the ip flood queue
	init_fabric_ip_flood_queue();

	LOG_PROC("INFO","fabric_ip_flood_thread init!");
	while(g_fabric_ip_flood_flag){
		sem_wait(&fabric_ip_flood_sem);
		while(is_fabric_ip_flood_queue_empty()){
			flood_node = get_head_fabric_ip_flood_from_queue();
			if(flood_node != NULL){
				packet_in = &flood_node->packet_in_info;
				fabric_packet_flood(packet_in);
			}
			flood_node = pop_fabric_ip_flood_from_queue();
			delete_fabric_ip_flood_node(flood_node);
		}
	}
	// Destroy the ip flood queue
	destory_fabric_ip_flood_queue();
	LOG_PROC("INFO","fabric_ip_flood_thread end!");
	return NULL;
};
/*
 * fabric flows thread
 */
void* fabric_flow_thread(){
	p_fabric_flow_node flow_node = NULL;

	// initialize the flood queue
	init_fabric_flow_queue();

	LOG_PROC("INFO","fabric_flow_thread init!");
	while(g_fabric_flow_flag){
		sem_wait(&fabric_flow_sem);
		while(is_fabric_flow_queue_empty()){
			flow_node = get_head_fabric_flow_from_queue();
			if(flow_node != NULL){
				if(flow_node->dst_host->sw == flow_node->src_host->sw){
					install_fabric_same_switch_flow(flow_node->dst_host->sw,flow_node->dst_host->mac,flow_node->dst_host->port);
					install_fabric_same_switch_flow(flow_node->src_host->sw,flow_node->src_host->mac,flow_node->src_host->port);
				}else{
					install_fabric_push_tag_flow(flow_node->src_host->sw,flow_node->dst_host->mac,flow_node->dst_tag);
					install_fabric_push_tag_flow(flow_node->dst_host->sw,flow_node->src_host->mac,flow_node->src_tag);
				}
			}
			flow_node = pop_fabric_flow_from_queue();
			delete_fabric_flow_node(flow_node);
		}
	}
	// Destroy the flood queue
	destroy_fabric_flow_queue();
	LOG_PROC("INFO","fabric_flow_thread end! ");
	return NULL;
};

//void* fabric_host_thread(){
//	p_fabric_host_node host_queue_node = NULL;
//	p_fabric_host_node host_list_node = NULL;
//
//	// initialize the host queue
//	init_fabric_host_queue();
//
//	LOG_PROC("INFO","fabric_host_thread init !");
//	while(g_fabric_host_flag){
//		sem_wait(&fabric_host_sem);
//
//		while(is_fabric_host_queue_empty()){
////			printf("%s : queue is :%d \n",FN,is_fabric_host_queue_empty());
//			host_queue_node = get_head_fabric_host_from_queue();
//			// download host flow
//			install_fabric_output_flow(host_queue_node->sw,host_queue_node->mac,host_queue_node->port);
//
//			// find host in list
//			host_list_node = get_fabric_host_from_list_by_ip(host_queue_node->ip);
//			if(host_list_node == NULL){
//				host_list_node = create_fabric_host_list_node(
//						host_list_node->sw,
//						host_list_node->port,
//						host_list_node->mac,
//						host_list_node->ip
//						);
//				insert_fabric_host_into_list(host_list_node);
//			}else{
//				// clear old host flow
//				// wait...
//				// update
//				host_list_node->port = host_queue_node->port;
//				host_list_node->sw = host_queue_node->sw;
//				memcpy(host_list_node->mac,host_queue_node->mac, 6);
//			}
//			host_queue_node = pop_fabric_host_from_queue();
//			delete_fabric_host_queue_node(host_queue_node);
////			printf("%s end\n",FN);
//		}
//	}
//	// Destroy the host queue
//	destroy_fabric_host_queue();
//	LOG_PROC("INFO","fabric_host_thread end!");
//	return NULL;
//};
/*****************************
 * intern functions
 *****************************/
/*
 * flood the packet to each port without switch intern ports
 */
void fabric_packet_flood( packet_in_info_t *packet_in_info){
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
			// find switch's outter ports
			for(j=0; j<sw->n_ports; j++){
				// check port state is ok and also not connect other switch(neighbor)
				if(sw->ports[j].state == 0 && sw->neighbor[j] == NULL){
					pakout_req.outport = sw->ports[j].port_no;
					sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
				}
			}
		}
	}

	return;
};

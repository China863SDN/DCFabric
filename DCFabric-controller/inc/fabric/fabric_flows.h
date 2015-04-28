/*
 * fabric_flows.h
 *
 *  Created on: Mar 27, 2015
 *      Author: joe
 */

#ifndef INC_FABRIC_FABRIC_FLOWS_H_
#define INC_FABRIC_FABRIC_FLOWS_H_
#include "gnflush-types.h"

#define FABRIC_IMPL_HARD_TIME_OUT 0
#define FABRIC_IMPL_IDLE_TIME_OUT 0
#define FABRIC_ARP_HARD_TIME_OUT 0
#define FABRIC_ARP_IDLE_TIME_OUT 100
#define FABRIC_FIND_HOST_IDLE_TIME_OUT 0

#define FABRIC_PRIORITY_HOST_INPUT_FLOW 0
#define FABRIC_PRIORITY_SWITCH_INPUT_FLOW 10
#define FABRIC_PRIORITY_ARP_MISSMATCH_INPUT_FLOW 5
#define FABRIC_PRIORITY_MISSMATCH_PUSHTAG_FLOW 0
#define FABRIC_PRIORITY_SWAPTAG_FLOW 20
#define FABRIC_PRIORITY_ARP_FLOW 15

#define FABRIC_INPUT_TABLE 0
#define FABRIC_PUSHTAG_TABLE 1
#define FABRIC_SWAPTAG_TABLE 2
#define FABRIC_OUTPUT_TABLE 3

#define FABRIC_SWITCH_INPUT_MAX_FLOW_NUM 48

typedef struct fabric_flow{
	gn_switch_t* sw;
	gn_flow_t* host_input_flow;
	gn_flow_t* miss_match_flow;
	gn_flow_t* arp_miss_match_flow;
	//gn_flow_t* switch_inport_flow[FABRIC_SWITCH_INPUT_MAX_FLOW_NUM];

	gn_flow_t* last_flow;
	gn_flow_t* first_flows;
	gn_flow_t* middle_flows;
	//UINT2 switch_inport_flow_num;
	struct fabric_flow* next;
}t_fabric_flow,* p_fabric_flow;

void install_fabric_base_flows(gn_switch_t * sw);
void install_fabric_last_flow(gn_switch_t * sw,UINT4 tag);
void install_fabric_first_flow(gn_switch_t * sw,UINT4 port,UINT4 tag);
void install_fabric_middle_flow(gn_switch_t * sw,UINT4 port,UINT4 tag);

void install_fabric_same_switch_flow(gn_switch_t * sw,UINT1* mac, UINT4 port);
void install_fabric_output_flow(gn_switch_t * sw,UINT1* mac, UINT4 port);
void install_fabric_push_tag_flow(gn_switch_t * sw,UINT1* mac,UINT4 tag);

void init_fabric_flows();
void delete_fabric_flow(gn_switch_t *sw);
void delete_fabric_flow_by_sw(gn_switch_t *sw);



#endif /* INC_FABRIC_FABRIC_FLOWS_H_ */

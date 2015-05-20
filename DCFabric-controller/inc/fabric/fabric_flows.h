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
 * fabric_flows.h
 *
 *  Created on: Mar 27, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
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


////////////////////////////////////////////////////////////////////////
/*
 * interfaces
 */
////////////////////////////////////////////////////////////////////////
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
void delete_fabric_impl_flow(gn_switch_t *sw,UINT4 port_no,UINT4 tag,UINT1 table_id);

UINT1 get_fabric_last_flow_table();
UINT1 get_fabric_first_flow_table();
UINT1 get_fabric_middle_flow_table();

#endif /* INC_FABRIC_FABRIC_FLOWS_H_ */

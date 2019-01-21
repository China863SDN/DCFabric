/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2016, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the BNC SDN Controller. BNC SDN
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

/******************************************************************************
*                                                                             *
*   File Name   : CFlowMod.h                 *
*   Author      : bnc cyyang                *
*   Create Date : 2017-12-21               *
*   Version     : 1.0                     *
*   Function    : .                       *
*                                                                             *
******************************************************************************/
#ifndef _CFLOWMOD_H
#define _CFLOWMOD_H
#include "bnc-param.h"
#include "CMutex.h"


#define MSG_MAX_LEN  10240

#define Msg_Buf(msgtype) \
    char msgbuf[MSG_MAX_LEN];\
    memset(msgbuf, 0, sizeof(msgbuf));\
    msgtype* pMsg = NULL;\
    pMsg = (msgtype*)msgbuf;

class CFlowMod
{
public:
    CFlowMod();
    ~CFlowMod();
	
	static UINT2 of13_add_oxm_field(UINT1 *buf, gn_oxm_t *oxm_fields);
	static UINT2 of13_add_set_field(UINT1 *buf, gn_oxm_t *oxm_fields);
	static UINT2 of13_add_action(UINT1 *buf, gn_action_t *action);
	static UINT2 of13_add_instruction(UINT1 *buf, gn_flow_t *flow);
	static UINT2 of13_add_match(struct ofpx_match *match, gn_match_t *gn_match);



	
	static flow_param_t* init_flow_param();
	static void clear_flow_param(flow_param_t* flow_param);	
	static void set_flow_match(gn_oxm_t* flow_oxm, gn_oxm_t* oxm);
	static void set_security_match( gn_oxm_t* oxm, security_param_t* security_param);
	static gn_instruction_t* set_flow_instruction(gn_instruction_t* flow_instruction, action_param_t* instraction_param);
	static void set_flow_action(gn_instruction_t* flow_instruction, action_param_t* action_param, enum ofp_instruction_type instruction_type);
	static void clear_instruction_data(gn_instruction_t* instruction);
	static void add_action_param(action_param_t** action_param, INT4 type, void* param);
	static void add_action_param_rear(action_param_t** action_param, INT4 type, void* param);
	static void clear_action_param(action_param_t* action_param);

	//static INT4 install_fabric_flows(CSmartPtr<CSwitch> sw, UINT2 idle_timeout,UINT2 hard_timeout,UINT2 priority,UINT1 table_id,UINT1 command,flow_param_t* flow_param);

   // static BOOL sendOfp13FlowMod(CSmartPtr<CSwitch> sw, UINT1 *flowmod_req);

private:

	 //static CMutex   g_sendPktMutex;
};

#endif

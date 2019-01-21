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
*   File Name   : CFlowMgr.cpp		*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-29         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "CSmartPtr.h"
#include "CFlowMgr.h"
#include "bnc-type.h"
#include "COfMsgUtil.h"
#include "CHost.h"
#include "CServer.h"
#include "CFlowDefine.h"
#include "CControl.h"
#include "CHostMgr.h"
#include "CFlowMod.h"
#include "BaseExternalManager.h"
#include "COpenstackExternal.h"
#include "COpenstackExternalMgr.h"
#include "SwitchManageMode.h"
#include "SwitchManagePresistence.h"
#include "CFlowTableEventReportor.h"
#include "CClusterService.h"

#include "CConf.h"
#include "log.h"

CMutex   CFlowMgr::g_sendPktMutex;

CFlowMgr* CFlowMgr::m_pInstance = 0;

using namespace bnc::flow;

CFlowMgr::CFlowMgr()
{
}

CFlowMgr::~CFlowMgr()
{
    m_flowCache.deregister();
}

CFlowMgr* CFlowMgr::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CFlowMgr();
		if (NULL == m_pInstance)
		{
			exit(-1);
		}
    }

    return (m_pInstance);
}

INT4 CFlowMgr::init()
{
	INT4 ret = BNC_OK;
    ret = m_flowCache.onregister();
	ret += m_tagflow_consume.onregister();
	return  (BNC_OK == ret) ? BNC_OK: BNC_ERR;
	
}

void CFlowMgr::clear_flow(const CSmartPtr<CSwitch> & sw)
{
#if 0
    gn_flow_t flow;
    flow_mod_req_info_t flow_mod_req;

    memset(&flow, 0, sizeof(gn_flow_t));
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 0;
    flow.match.type = OFPMT_OXM;
    flow.table_id = 0xff;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_DELETE;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sendOfp13FlowMod(sw, (UINT1*)&flow_mod_req);
#endif

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE0, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE1, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE2, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE3, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE4, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE5, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE6, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE7, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE8, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE9, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE10, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE11, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE12, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE13, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE14, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE15, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE16, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE17, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE18, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE19, OFPFC_DELETE, flow_param);
	install_fabric_flows(sw, 0, 0, 0, FABRIC_TABLE20, OFPFC_DELETE, flow_param);

	CFlowMod::clear_flow_param(flow_param);
	
}

void CFlowMgr::install_base_flows(const CSmartPtr<CSwitch> & sw)
{
	
    install_base_flow(sw, TABLE_PROTOCAL);
    install_base_flow(sw, TABLE_SECURITY);
    install_base_flow(sw, TABLE_GROUP);
    install_base_flow(sw, TABLE_INTERNAL);
    install_base_flow(sw, TABLE_EXTERNAL);
    install_base_flow(sw, TABLE_VLAN);
    install_base_flow(sw, TABLE_OUTPUT);

    //install_ip_base_flow(sw);
    install_vlan_base_flow(sw);
}



void CFlowMgr::install_base_flow(const CSmartPtr<CSwitch> & sw, UINT1 table_id)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction;
    gn_action_output_t action;
	
	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == sw->getManageMode()))
	{
		//LOG_ERROR("switch manage mode is on!");
		return ;
	}
	
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 0;
    flow.table_id = table_id;
    flow.match.type = OFPMT_OXM;

    memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    memset(&action, 0, sizeof(gn_action_output_t));
    action.port = OFPP13_CONTROLLER;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sendOfp13FlowMod(sw, (UINT1*)&flow_mod_req);
}

void CFlowMgr::install_base_flow(const CSmartPtr<CSwitch> & sw, UINT2 match_proto, UINT1 table_id)
{
	flow_mod_req_info_t flow_mod_req;
	gn_flow_t flow;
	gn_action_output_t action;
	gn_instruction_actions_t instruction;
	//gn_instruction_goto_table_t instruction_goto;

	
	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == sw->getManageMode()))
	{
		//LOG_ERROR("switch manage mode is on!");
		return ;
	}
	memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = 0;
	flow.idle_timeout = 0;
	flow.hard_timeout = 0;
	flow.priority = 5;
	flow.table_id = table_id;
	flow.match.type = OFPMT_OXM;

	flow.match.oxm_fields.eth_type = match_proto;
	flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);


	memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;
	
	memset(&action, 0, sizeof(gn_action_output_t));
    action.port = OFPP13_CONTROLLER;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;

	//memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
	//instruction_goto.type = OFPIT_GOTO_TABLE;
	//instruction_goto.table_id = TABLE_INTERNAL;
	//instruction_goto.next = flow.instructions;
	//flow.instructions = (gn_instruction_t *)&instruction_goto;

	flow_mod_req.xid = 0;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = OFPFC_ADD;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

	sendOfp13FlowMod(sw, (UINT1*)&flow_mod_req);
}

void CFlowMgr::install_base_dcfabric_flows(const CSmartPtr<CSwitch> & sw)
{
    install_base_flow(sw, FABRIC_TABLE_INPUT);
	//install_base_flow(sw, ETHER_IP,  FABRIC_TABLE_INPUT);
	install_base_flow(sw, ETHER_ARP, FABRIC_TABLE_INPUT);
	install_base_flow(sw, ETHER_IP,  FABRIC_TABLE10);
	install_base_flow(sw, ETHER_IP,  FABRIC_TABLE19);
    install_add_fabric_controller_flow(sw);

    install_vlan_base_flow(sw);
}
void CFlowMgr::install_base_gotonext_flow(const CSmartPtr<CSwitch> & sw, UINT1 table_id)
{
    install_base_gototable_flow(sw, FABRIC_TABLE_QOS_INEXT, FABRIC_TABLE_PORTFORWARD_INEXT);
    install_base_gototable_flow(sw, FABRIC_TABLE_PORTFORWARD_INEXT, FABRIC_TABLE_NAT_INEXT);
    install_base_gototable_flow(sw, FABRIC_TABLE_NAT_INEXT,FABRIC_TABLE_FLOATINGIP_INEXT );
	install_base_gototable_flow(sw, FABRIC_TABLE_FLOATINGIP_INEXT,FABRIC_TABLE_TAGFORWARD_EXT );
	install_base_gototable_flow(sw, FABRIC_TABLE_FIREWALL_OUTEXT,FABRIC_TABLE_QOS_OUTEXT );
	install_base_gototable_flow(sw, FABRIC_TABLE_QOS_OUTEXT,FABRIC_TABLE13_REV );
	install_base_gototable_flow(sw, FABRIC_TABLE13_REV,FABRIC_TABLE17_REV );
	install_base_gototable_flow(sw, FABRIC_TABLE17_REV,FABRIC_TABLE18_REV );
	install_base_gototable_flow(sw, FABRIC_TABLE18_REV,FABRIC_TABLE_MAC_FORWARD );
	
}

void CFlowMgr::install_base_default_gotonext_flow(const CSmartPtr<CSwitch> & sw)
{
	install_base_gototable_flow(sw, FABRIC_TABLE0, FABRIC_TABLE1);
    if (!CConf::getInstance()->isSecurityGroupOn())
	    install_base_gototable_flow(sw, FABRIC_TABLE1, FABRIC_TABLE2);
    else        
        install_base_flow(sw, FABRIC_TABLE1);
    install_base_gototable_flow(sw, FABRIC_TABLE2, FABRIC_TABLE3);
    install_base_gototable_flow(sw, FABRIC_TABLE3, FABRIC_TABLE4);
    install_base_gototable_flow(sw, FABRIC_TABLE4,FABRIC_TABLE5 );
	install_base_gototable_flow(sw, FABRIC_TABLE5,FABRIC_TABLE9 );
	install_base_gototable_flow(sw, FABRIC_TABLE9,FABRIC_TABLE10 );

	install_base_gototable_flow(sw, FABRIC_TABLE10,FABRIC_TABLE11 );
	
	install_base_gototable_flow(sw, FABRIC_TABLE11,FABRIC_TABLE12 );
	install_base_gototable_flow(sw, FABRIC_TABLE12,FABRIC_TABLE13 );
	
	install_base_gototable_flow(sw, FABRIC_TABLE13,FABRIC_TABLE17 );
    if (!CConf::getInstance()->isSecurityGroupOn())
    	install_base_gototable_flow(sw, FABRIC_TABLE17,FABRIC_TABLE18 );
    else        
        install_base_flow(sw, FABRIC_TABLE17);
	install_base_gototable_flow(sw, FABRIC_TABLE18,FABRIC_TABLE19 );
		
}

void CFlowMgr::install_base_gototable_flow(const CSmartPtr<CSwitch> & sw, UINT1 table_id, UINT1 gototable_id)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->instruction_param,  OFPIT_GOTO_TABLE, (void*)&gototable_id);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, 2,
						 table_id, OFPFC_ADD, flow_param);
	

	CFlowMod::clear_flow_param(flow_param);
}


void CFlowMgr::install_ip_base_flow(const CSmartPtr<CSwitch> & sw)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_goto_table_t instruction_goto;

	
	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == sw->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return ;
	}
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 5;
    flow.table_id = TABLE_PROTOCAL;
    flow.match.type = OFPMT_OXM;

    flow.match.oxm_fields.eth_type = ETHER_IP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

    memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
    instruction_goto.type = OFPIT_GOTO_TABLE;
    instruction_goto.table_id = TABLE_INTERNAL;
    instruction_goto.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_goto;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sendOfp13FlowMod(sw, (UINT1*)&flow_mod_req);
}



void CFlowMgr::install_vlan_base_flow(const CSmartPtr<CSwitch> & sw)
{
	if(sw.isNull())
	{
		LOG_ERROR("sw is null!");
		return;
	}
	UINT4 JumpToTable = FABRIC_TABLE_FIREWALL_INVM;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->vlan_vid = sw->getTag();
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->instruction_param,  OFPIT_GOTO_TABLE, (void*)&JumpToTable);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_POP_VLAN, NULL);
	install_fabric_flows(sw, 0, 0, 30,
						 FABRIC_TABLE_INPUT, OFPFC_ADD, flow_param); //FABRIC_TABLE_vm
	CFlowMod::clear_flow_param(flow_param);
}

void CFlowMgr::install_swap_input_flow(const CSmartPtr<CSwitch> & srcSw, const CSmartPtr<CSwitch> & dstSw, UINT4 port_no)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction_act;
    gn_action_output_t act_output;

	
	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == srcSw->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return ;
	}
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 30;
    flow.table_id = TABLE_PROTOCAL;
    flow.match.type = OFPMT_OXM;

    flow.match.oxm_fields.vlan_vid = (UINT2)dstSw->getTag();
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);

    memset(&act_output, 0, sizeof(gn_action_output_t));
    act_output.next = NULL;
    act_output.type = OFPAT13_OUTPUT;
    act_output.port = port_no;

    memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.actions = (gn_action_t*)&act_output;
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sendOfp13FlowMod(srcSw, (UINT1*)&flow_mod_req);
}


void CFlowMgr::install_swap_flow(const CSmartPtr<CSwitch> & srcSw, const CSmartPtr<CSwitch> & dstSw, UINT4 port_no)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction_act;
    gn_action_output_t act_output;

	
	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == srcSw->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return ;
	}
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 30;
    flow.table_id = 10;//TABLE_VLAN;
    flow.match.type = OFPMT_OXM;

    flow.match.oxm_fields.vlan_vid = (UINT2)dstSw->getTag();
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);

    memset(&act_output, 0, sizeof(gn_action_output_t));
    act_output.next = NULL;
    act_output.type = OFPAT13_OUTPUT;
    act_output.port = port_no;

    memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.actions = (gn_action_t*)&act_output;
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sendOfp13FlowMod(srcSw, (UINT1*)&flow_mod_req);
}

void CFlowMgr::install_local_host_flows(CSmartPtr<CHost>& host)
{
	if((host.isNull())||(TRUE == host->getSw().isNull()))
	{
		return ;
	}
	if(TRUE ==G_ExternalMgr.checkSwitch_isExternal(host->getDpid()))
	{
		install_local_host_output_flow(host, FABRIC_TABLE_NAT_INEXT);
	}
	else
	{
    	install_local_host_output_flow(host, FABRIC_TABLE_FORWARD_INVM);
	}
    //install_local_host_find_flow(host);
}

void CFlowMgr::install_local_host_output_flow(CSmartPtr<CHost>& host, UINT1 tableid)
{
	if((host.isNull())||(TRUE == host->getSw().isNull()))
	{
		return ;
	}
    LOG_INFO_FMT("install local host out flow: %x", host->getIp());

	UINT4 output = host->getPortNo();
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	memcpy(flow_param->match_param->eth_dst,  host->getMac(), 6);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param,  OFPAT13_OUTPUT, (void*)&output);

	install_fabric_flows(host->getSw(), 0, 0, 20,
						tableid , OFPFC_ADD, flow_param);
	

	CFlowMod::clear_flow_param(flow_param);
	
}


void CFlowMgr::install_local_host_find_flow(CSmartPtr<CHost>& host)
{
    if (host.isNotNull())
        LOG_INFO_FMT("install local host find flow: %x", host->getIp());
}

void CFlowMgr::install_same_switch_flow(CSmartPtr<CHost>& srchost, CSmartPtr<CHost>& dstHost)
{
	if((dstHost.isNull())||(srchost.isNull())||(dstHost->getSw().isNull()))
	{
		return ;
	}

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, dstHost->getMac(), 6);

	
	UINT4 output = dstHost->getPortNo();


	flow_param_t* flow_param = CFlowMod::init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst=ntohl(dstHost->getfixIp());
	memcpy(flow_param->match_param->eth_src,  srchost->getMac(), 6);
	//memcpy(flow_param->match_param->eth_dst,  dstHost->getMac(), 6);
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param,  OFPAT13_OUTPUT, (void*)&output);

	install_fabric_flows(dstHost->getSw(), 0, 0, 21,
						 0, OFPFC_ADD, flow_param);
	
	CFlowMod::clear_flow_param(flow_param);

}

void CFlowMgr::install_same_switch_gateway_flow(CSmartPtr<CHost>& dstHost, CSmartPtr<CHost>& srcGateway)
{

    LOG_INFO("install local host flow");

	if((dstHost.isNull())||(srcGateway.isNull())||(dstHost->getSw().isNull()))
	{
		return ;
	}
	UINT4 output = dstHost->getPortNo();
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, dstHost->getMac(), 6);
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(dstHost->getIp());
	memcpy(flow_param->match_param->eth_dst,  srcGateway->getMac(), 6);
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param,  OFPAT13_OUTPUT, (void*)&output);

	install_fabric_flows(dstHost->getSw(), FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, 12,
						 0, OFPFC_ADD, flow_param);
	

	CFlowMod::clear_flow_param(flow_param);
}

void CFlowMgr::install_different_switch_flow(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost)
{
    LOG_INFO("install different switch flow");

	if((srcHost.isNull())||(dstHost.isNull()) ||( dstHost->getSw().isNull())||( srcHost->getSw().isNull()) )
	{
		return ;
	}

	UINT2 outport = CControl::getInstance()->getTopoMgr().get_out_port_between_switch( dstHost->getSw()->getDpid(), srcHost->getSw()->getDpid());
	if (0 == outport)
	{
        return ;
    }
	
	UINT2 JumpToTable = FABRIC_TABLE_TAGFORWARD_TOR;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)dstHost->getSw()->getTag();
	memcpy(oxm.eth_dst, dstHost->getMac(), 6);
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	memcpy(flow_param->match_param->eth_src,  srcHost->getMac(), 6);
	
	
	flow_param->match_param->ipv4_dst = ntohl(dstHost->getfixIp());
	flow_param->match_param->eth_type = ETHER_IP;
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	//CFlowMod::add_action_param(&flow_param->instruction_param,  OFPIT_GOTO_TABLE, (void*)&JumpToTable);


	
	if(srcHost->getSw().isNotNull()
        &&(BNC_OK == srcHost->getSw()->flowInstall(dstHost->getSw(),flow_param,  JumpToTable)))
	{
		
		LOG_DEBUG_FMT("param=0x%p flow_param->action_param->param=0x%x", flow_param->action_param->param, *(UINT4*)flow_param->action_param->param);
		install_fabric_flows(srcHost->getSw(), 0, 0, 21,
						 FABRIC_TABLE_INPUT, OFPFC_ADD, flow_param); //FABRIC_TABLE_vm
		
		srcHost->getSw()->flowAllocMemFree((void *)flow_param->action_param->param);
	}

	
	CFlowMod::clear_flow_param(flow_param);

	
	LOG_DEBUG_FMT("%s %d oxm.vlan_vid=%d srcHost->getSw()->getSwIp()=0x%x",FN,LN,oxm.vlan_vid ,srcHost->getSw()->getSwIp());

	flow_param = CFlowMod::init_flow_param();
	oxm.vlan_vid = (UINT2)srcHost->getSw()->getTag();
	memcpy(oxm.eth_dst, srcHost->getMac(), 6);

	memcpy(flow_param->match_param->eth_src,  dstHost->getMac(), 6);
	flow_param->match_param->ipv4_dst = ntohl(srcHost->getfixIp());
	flow_param->match_param->eth_type = ETHER_IP;
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	//CFlowMod::add_action_param(&flow_param->instruction_param,  OFPIT_GOTO_TABLE, (void*)&JumpToTable);
	
	if(dstHost->getSw().isNotNull()
        &&(BNC_OK == dstHost->getSw()->flowInstall(srcHost->getSw(),flow_param,  JumpToTable)))
	{
		install_fabric_flows(dstHost->getSw(), 0, 0, 21,
						 FABRIC_TABLE_INPUT, OFPFC_ADD, flow_param);//FABRIC_TABLE_vm
		
		dstHost->getSw()->flowAllocMemFree((void *)flow_param->action_param->param);
	}
	
	LOG_DEBUG_FMT("%s %d oxm.vlan_vid=%d dstHost->getSw()->getSwIp()=0x%x",FN,LN,oxm.vlan_vid ,dstHost->getSw()->getSwIp());
	
	CFlowMod::clear_flow_param(flow_param);
}


void CFlowMgr::install_different_switch_flow(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, CSmartPtr<CHost>& srcGateway)
{
    LOG_INFO("install different switch flow");

	LOG_ERROR("install different switch flow");
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction_act;
    gn_instruction_goto_table_t instruction_goto;
    gn_action_set_field_t act_set_field;
    gn_action_t act_pushVlan;

	if((srcHost.isNull())||(dstHost.isNull())||(srcGateway.isNull()) ||(srcHost->getSw().isNull()) || (dstHost->getSw().isNull()))
	{
		return ;
	}
	
	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == srcHost->getSw()->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return ;
	}
	
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 20;
    flow.table_id = TABLE_INTERNAL;
    // flow.table_id = 0;
    flow.match.type = OFPMT_OXM;

    //memcpy(flow.match.oxm_fields.eth_dst, srcGateway->getMac(), 6);
    //flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);

	flow.match.oxm_fields.ipv4_src=ntohl(srcHost->getIp());
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);
	
    flow.match.oxm_fields.eth_type = ETHER_IP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);

    flow.match.oxm_fields.ipv4_dst=ntohl(dstHost->getIp());
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);

    memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
    instruction_goto.type = OFPIT_GOTO_TABLE;
    instruction_goto.table_id = TABLE_VLAN;
    instruction_goto.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_goto;

    memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;


//    memset(&action, 0, sizeof(gn_action_t));
//    action.port = CServer::getInstance()->getTopoMgr()->getPortNoBetweenSW(srcHost->getSw(), dstHost->getSw());
//    action.type = OFPAT13_OUTPUT;
//    action.next = instruction_act.actions;
//    action.max_len = 0xffff;
//    instruction_act.actions = (gn_action_t *)&action;

    memset(&act_set_field, 0, sizeof(gn_action_set_field_t));
    act_set_field.type = OFPAT13_SET_FIELD;
    act_set_field.oxm_fields.vlan_vid = (UINT2)dstHost->getSw()->getTag();
    act_set_field.oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
    act_set_field.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field;

    memcpy(act_set_field.oxm_fields.eth_dst, dstHost->getMac(), 6);
    act_set_field.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);


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


    sendOfp13FlowMod(srcHost->getSw(), (UINT1*)&flow_mod_req);
}
void CFlowMgr::install_different_switch_flow(CSmartPtr<CSwitch> dstSw, CSmartPtr<CSwitch> nodeSw, UINT4 output)
{
	if(dstSw.isNull()||nodeSw.isNull())
	{
		return;
	}
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->vlan_vid = dstSw->getTag();
		
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	
	CFlowMod::add_action_param(&flow_param->action_param,  OFPAT13_OUTPUT, (void*)&output);

	install_fabric_flows(nodeSw, 0, 0, 30,
						 FABRIC_TABLE_TAGFORWARD_TOR, OFPFC_ADD, flow_param);
	
	//install_fabric_flows(nodeSw, 0, 0, 30,
	//					 FABRIC_TABLE_INPUT, OFPFC_ADD, flow_param);
	
	CFlowMod::clear_flow_param(flow_param);
}

void CFlowMgr::install_proxy_flows(CProxyConnect* proxyConnect)
{
    LOG_INFO("install proxy flow");
	#if 0
    COpenstackExternal* external = COpenstackExternalMgr::getInstance()->findOpenstackExternalAny();

    if (NULL == external)
    {
       LOG_INFO("External is not exist.");
       return ;
    }

    CSmartPtr<CSwitch> extSw = CControl::getInstance()->getSwitchMgr().findSwByDpid(external->getExternalDpid());

    if (extSw.isNull())
    {
       LOG_INFO("External switch is not exist.");
       return ;
    }
	
    CHost* srcHost = CHostMgr::getInstance()->findHostByMac(proxyConnect->getFixedMac());

    if (NULL == srcHost)
    {
       LOG_INFO("Fixed host is not exist.");
       
       return;
    }

    if ((NULL == srcHost->getSw()) || (0 == srcHost->getPortNo()))
    {
       LOG_INFO("Fixed host is not exist.");
       return;
    }

    UINT4 outPort = CControl::getInstance()->getTopoMgr().getPortNoBetweenSW(extSw, srcHost->getSw());

    if (0 == outPort)
    {
        LOG_INFO("Can't find the port from ext to host");
        return;
    }

    install_proxy_host_flow(proxyConnect, extSw, srcHost->getSw());
    install_proxy_external_flow(proxyConnect, extSw, srcHost->getSw(), outPort);
    install_local_host_output_flow(srcHost);
    install_proxy_external_output_flow(extSw, external->getExternalGatewayMac(), external->getExternalPort());
	#endif
}

void CFlowMgr::install_proxy_host_flow(CProxyConnect* proxyConnect, CSmartPtr<CSwitch> extSw, CSmartPtr<CSwitch> hostSw)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction_act;
    gn_action_set_field_t act_set_field_mac;
    gn_action_set_field_t act_set_field_srcmac;
    gn_action_set_field_t act_set_field_ip;
    gn_action_set_field_t act_set_field_vlan;
    gn_action_t act_pushVlan;
    gn_instruction_goto_table_t instruction_goto;

	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == hostSw->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return ;
	}
    //match rule
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.idle_timeout = 100;
    flow.hard_timeout = 100;
    flow.priority = 16;
    flow.table_id = TABLE_INTERNAL;
    flow.match.type = OFPMT_OXM;
    flow.match.oxm_fields.eth_type = ETHER_IP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
    flow.match.oxm_fields.ipv4_dst = ntohl(proxyConnect->getExtIp());
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
    memcpy(flow.match.oxm_fields.eth_src, proxyConnect->getFixedMac(), 6);
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_SRC);

    //set go-to-table
    memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
    instruction_goto.type = OFPIT_GOTO_TABLE;
    instruction_goto.table_id = TABLE_VLAN;
    instruction_goto.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_goto;

    memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

    //set src mac
    memset(&act_set_field_srcmac, 0, sizeof(gn_action_set_field_t));
    act_set_field_srcmac.type = OFPAT13_SET_FIELD;
    memcpy(act_set_field_srcmac.oxm_fields.eth_src, proxyConnect->getProxyMac(), 6);
    act_set_field_srcmac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_SRC);
    act_set_field_srcmac.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field_srcmac;

    //set dest mac
    memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
    act_set_field_mac.type = OFPAT13_SET_FIELD;
    memcpy(act_set_field_mac.oxm_fields.eth_dst, proxyConnect->getExtMac(), 6);
    act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
    act_set_field_mac.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field_mac;

    //set src ip
    memset(&act_set_field_ip, 0, sizeof(gn_action_set_field_t));
    act_set_field_ip.type = OFPAT13_SET_FIELD;
    act_set_field_ip.oxm_fields.ipv4_src = ntohl(proxyConnect->getProxyIp());
    act_set_field_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);
    act_set_field_ip.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field_ip;

    //set vlan
    memset(&act_set_field_vlan, 0, sizeof(gn_action_set_field_t));
    act_set_field_vlan.type = OFPAT13_SET_FIELD;
    act_set_field_vlan.oxm_fields.vlan_vid = extSw->getTag();
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

    sendOfp13FlowMod(hostSw, (UINT1*)&flow_mod_req);
}


void CFlowMgr::install_proxy_external_flow(CProxyConnect* proxyConnect, CSmartPtr<CSwitch> extSw, CSmartPtr<CSwitch> hostSw, UINT4 outPort)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction_act;
    gn_action_set_field_t act_set_field_mac;
    gn_action_set_field_t act_set_field_ip;
    gn_action_set_field_t act_set_field_vlan;
    gn_action_t act_pushVlan;
    //gn_instruction_goto_table_t instruction_goto;
    gn_action_output_t output_action;

	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == extSw->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return ;
	}
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.idle_timeout = 100;
    flow.hard_timeout = 100;
    flow.priority = 20;
    flow.table_id = TABLE_PROTOCAL;  //
    flow.match.type = OFPMT_OXM;
    flow.match.oxm_fields.eth_type = ETHER_IP;
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_TYPE);
    flow.match.oxm_fields.ipv4_dst = ntohl(proxyConnect->getProxyIp());
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);


    memset(&instruction_act, 0, sizeof(gn_instruction_actions_t));
    instruction_act.type = OFPIT_APPLY_ACTIONS;
    instruction_act.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction_act;

//    if (0 == get_nat_physical_switch_flag()) {
//      memset(&instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
//      instruction_goto.type = OFPIT_GOTO_TABLE;
//      instruction_goto.table_id = FABRIC_SWAPTAG_TABLE;
//      instruction_goto.next = flow.instructions;
//      flow.instructions = (gn_instruction_t *)&instruction_goto;
//    }
//    else {
//
//    }

    ///< pica8交换机必须立刻Output
    memset(&output_action, 0, sizeof(gn_action_output_t));
    output_action.port = outPort;
    output_action.type = OFPAT13_OUTPUT;
    output_action.max_len = 0xffff;
    output_action.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&output_action;

    memset(&act_set_field_mac, 0, sizeof(gn_action_set_field_t));
    act_set_field_mac.type = OFPAT13_SET_FIELD;
    memcpy(act_set_field_mac.oxm_fields.eth_dst, proxyConnect->getFixedMac(), 6);
    act_set_field_mac.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
    act_set_field_mac.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field_mac;

    memset(&act_set_field_ip, 0, sizeof(gn_action_set_field_t));
    act_set_field_ip.type = OFPAT13_SET_FIELD;
    act_set_field_ip.oxm_fields.ipv4_dst = ntohl(proxyConnect->getFixedIp());
    act_set_field_ip.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
    act_set_field_ip.next = instruction_act.actions;
    instruction_act.actions = (gn_action_t *)&act_set_field_ip;

    memset(&act_set_field_vlan, 0, sizeof(gn_action_set_field_t));
    act_set_field_vlan.type = OFPAT13_SET_FIELD;
    act_set_field_vlan.oxm_fields.vlan_vid = hostSw->getTag();
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

    sendOfp13FlowMod(extSw, (UINT1*)&flow_mod_req);
}

void CFlowMgr::install_proxy_external_output_flow(CSmartPtr<CSwitch> extSw, const UINT1* gatewayMac, UINT2 extPortNo)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    gn_instruction_actions_t instruction;
    gn_action_output_t action;

	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == extSw->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return ;
	}
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 10;
    flow.table_id = TABLE_OUTPUT;
    flow.match.type = OFPMT_OXM;

    memcpy(flow.match.oxm_fields.eth_dst, gatewayMac, 6);
    flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);

    memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    memset(&action, 0, sizeof(gn_action_t));
    action.port = extPortNo;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sendOfp13FlowMod(extSw, (UINT1*)&flow_mod_req);
}

void CFlowMgr::install_fabric_output_flow(CSmartPtr<CSwitch> sw,UINT1* mac, UINT4 port)
{
	if(0 == port)
	{
		return ;
	}
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, 
						 0, 
						 0, 
						 20,
						 FABRIC_TABLE_MAC_FORWARD, 
						 OFPFC_ADD, 
						 flow_param);

	CFlowMod::clear_flow_param(flow_param);
};

void  CFlowMgr::install_modifine_ExternalSwitch_Table_flow(CSmartPtr<CSwitch> sw)
{
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT1 table_id = FABRIC_TABLE_FIREWALL_OUTTOR;

	flow_param->match_param->vlan_vid = sw->getTag();
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_POP_VLAN, NULL);

	
	install_fabric_flows(sw, 
							 FABRIC_IMPL_IDLE_TIME_OUT, 
							 FABRIC_IMPL_HARD_TIME_OUT, 
							 30,
							 FABRIC_TABLE_INPUT, 
							 OFPFC_ADD, 
							 flow_param);
	CFlowMod::clear_flow_param(flow_param);
}

void CFlowMgr::install_modifine_ExternalSwitch_ExPort_flow(CSmartPtr<CSwitch> sw, UINT4 inport)
{

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT1 table_id = FABRIC_TABLE_FIREWALL_INVM;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->in_port = inport;
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, 20,
						 FABRIC_TABLE_INPUT, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);
}

//by: 装载openstack对外输出流表
void CFlowMgr::install_fabric_external_output_flow(CSmartPtr<CSwitch> sw,UINT4 port,UINT1* gateway_mac,UINT4 outer_interface_ip,UINT1 type){

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT2 flow_table_id = FABRIC_TABLE_NAT_OUTTOR;

	
	if (1 == type) 
	{
	    memcpy(flow_param->match_param->eth_dst, gateway_mac, 6);
		flow_param->match_param->eth_type = ETHER_IP;
	}
	else if(2 == type)
	{
		flow_param->match_param->ipv4_src = ntohl(outer_interface_ip);
		flow_param->match_param->eth_type = ETHER_IP;
	}
	else 
	{
	}



	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);


	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, 18,
						 flow_table_id, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);
	return;
}

void CFlowMgr::install_fabric_nat_throughfirewall_from_inside_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, CSmartPtr<CSwitch> sw)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	UINT2 table_id = FABRIC_TABLE_NAT_INVM;


	memcpy(flow_param->match_param->eth_src, packetin_src_mac, 6);
	flow_param->match_param->ipv4_src = ntohl(packetin_src_ip);
	flow_param->match_param->ipv4_dst = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);



	install_fabric_flows(sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 20,
						 FABRIC_TABLE_NAT_OUTTOR, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);
}
void CFlowMgr::install_fabric_nat_from_inside_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
								UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
								UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	CSmartPtr<CSwitch> sw, CSmartPtr<CSwitch> gateway_sw)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	UINT2 table_id = FABRIC_TABLE_TAGFORWARD_TOR;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_src, external_mac, 6);
	oxm.ipv4_src = ntohl(external_ip);
	oxm.tcp_src = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	oxm.udp_src = (IPPROTO_UDP == proto_type) ? external_port_no : 0;
	oxm.vlan_vid = (sw->getDpid()!= gateway_sw->getDpid()) ? (UINT2)gateway_vlan_vid : 0;
	memcpy(oxm.eth_dst, gateway_mac, 6);

	memcpy(flow_param->match_param->eth_src, packetin_src_mac, 6);
	flow_param->match_param->ipv4_src = ntohl(packetin_src_ip);
	flow_param->match_param->ipv4_dst = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) {
		CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	}
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	if((!sw.isNull())&&(BNC_OK == sw->flowInstall(gateway_sw,flow_param,  table_id)))
	{

		LOG_ERROR_FMT("param=0x%x flow_param->action_param->param=%d", flow_param->action_param->param, *(UINT4*)flow_param->action_param->param);
		install_fabric_flows(sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 20,
						 FABRIC_TABLE_NAT_OUTTOR, OFPFC_ADD, flow_param);
		
		sw->flowAllocMemFree((void *)flow_param->action_param->param);
	}
	#if 0
	if (sw->getDpid()!= gateway_sw->getDpid()) {
		CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&gateway_out_port);
	}

	install_fabric_flows(sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 20,
						 FABRIC_TABLE_NAT_OUTTOR, OFPFC_ADD, flow_param);
	#endif
	CFlowMod::clear_flow_param(flow_param);
}
/* 
* 使用物理出口交换机的情况出口交换机上下发的nat返回流表
 */
void CFlowMgr::install_fabric_nat_from_external_fabric_flow(
    UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
	UINT1* packetin_src_mac, UINT4 external_ip, UINT1* external_mac, UINT2 external_port_no,
	UINT4 src_vlan_vid, CSmartPtr<CSwitch> gateway_sw, UINT4 out_port)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.vlan_vid = (UINT2)src_vlan_vid;

	memcpy(flow_param->match_param->eth_dst, external_mac, 6);
	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->ipv4_dst = ntohl(external_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto_type) ? external_port_no : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);


	install_fabric_flows(gateway_sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 20,
					     FABRIC_TABLE_QOS_INVM, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);
}

void  CFlowMgr::install_fabric_nat_from_external_fabric_host_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	CSmartPtr<CSwitch> sw, CSmartPtr<CSwitch> gateway_sw)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	UINT1 GotoTable  = FABRIC_TABLE_FORWARD_INVM;
	
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	//memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.ipv4_dst = ntohl(packetin_src_ip);
	oxm.tcp_dst = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	oxm.udp_dst = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	flow_param->match_param->vlan_vid = (UINT2)sw->getTag();
	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->ipv4_dst = ntohl(external_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto_type) ? external_port_no : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&GotoTable);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_POP_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	
	
	

	install_fabric_flows(sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 31,
						 FABRIC_TABLE_INPUT, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);

}

#if 0
void  CFlowMgr::install_fabric_nat_sameswitch_external2host_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip,UINT1* external_mac, UINT2 external_port_no, UINT2 host_port, CSmartPtr<CSwitch> sw)
{
	UINT4 output = host_port;
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	LOG_ERROR_FMT("NAT: install fabric flow table host_port=%d sw ip =0x%x",host_port,sw->getSwIp());
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.ipv4_dst = ntohl(packetin_src_ip);
	oxm.tcp_dst = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	oxm.udp_dst = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;
		
	memcpy(flow_param->match_param->eth_dst, external_mac, 6);
	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto_type) ? external_port_no : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&output);

 
	install_fabric_flows(sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 30,
						 FABRIC_TABLE_QOS_INVM, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);
}


void  CFlowMgr::install_fabric_nat_sameswitch_host2external_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no, UINT2 gateway_out_port, CSmartPtr<CSwitch> sw)
{
    UINT4 output = gateway_out_port;
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	LOG_ERROR_FMT("NAT: install fabric flow table gateway_out_port=%d sw ip =0x%x",gateway_out_port,sw->getSwIp());
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_src, external_mac, 6);
	oxm.ipv4_src = ntohl(external_ip);
	oxm.tcp_src = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	oxm.udp_src = (IPPROTO_UDP == proto_type) ? external_port_no : 0;
	memcpy(oxm.eth_dst, gateway_mac, 6);

	memcpy(flow_param->match_param->eth_src, packetin_src_mac, 6);
	flow_param->match_param->ipv4_dst = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&output);

	install_fabric_flows(sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 30,
						 FABRIC_TABLE_NAT_OUTTOR, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);
}
#else


void  CFlowMgr::install_fabric_nat_sameswitch_external2host_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip,UINT1* external_mac, UINT2 external_port_no, UINT2 host_port, CSmartPtr<CSwitch> sw)
{
	UINT4 output = host_port;
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	LOG_ERROR_FMT("NAT: install fabric flow table host_port=%d sw ip =0x%x",host_port,sw->getSwIp());
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.ipv4_dst = ntohl(packetin_src_ip);
	oxm.tcp_dst = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	oxm.udp_dst = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	
	memcpy(flow_param->match_param->eth_dst, external_mac, 6);
	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->ipv4_dst = ntohl(external_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto_type) ? external_port_no : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	//CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	//CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&output);

	if(sw.isNotNull()&&(BNC_OK == sw->flow_fabric_nat_sameswitch_external2host(flow_param, oxm)))
	{
		LOG_ERROR_FMT("oxm.vlan_vid=%d",oxm.vlan_vid);
		CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
		CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&output);
		install_fabric_flows(sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 30,
						 FABRIC_TABLE_QOS_INVM, OFPFC_ADD, flow_param);
	}
 

	CFlowMod::clear_flow_param(flow_param);
}


void  CFlowMgr::install_fabric_nat_sameswitch_host2external_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no, UINT2 gateway_out_port, CSmartPtr<CSwitch> sw)
{
    UINT4 output = gateway_out_port;
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	LOG_ERROR_FMT("NAT: install fabric flow table gateway_out_port=%d sw ip =0x%x",gateway_out_port,sw->getSwIp());
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_src, external_mac, 6);
	oxm.ipv4_src = ntohl(external_ip);
	oxm.tcp_src = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	oxm.udp_src = (IPPROTO_UDP == proto_type) ? external_port_no : 0;
	memcpy(oxm.eth_dst, gateway_mac, 6);
	//oxm.vlan_vid = 1;   // only pica8
	
	memcpy(flow_param->match_param->eth_src, packetin_src_mac, 6);
	flow_param->match_param->ipv4_src = ntohl(packetin_src_ip);
	flow_param->match_param->ipv4_dst = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	//CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL); //only pica8
	//CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	//CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&output);

	if(sw.isNotNull()&&(BNC_OK == sw->flow_fabric_nat_sameswitch_host2external(flow_param, oxm)))
	{
		LOG_ERROR_FMT("oxm.vlan_vid=%d",oxm.vlan_vid);
		CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
		CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&output);
		install_fabric_flows(sw, FABRIC_NAT_IDLE_TIME_OUT, FABRIC_NAT_HARD_TIME_OUT, 30,
						 FABRIC_TABLE_NAT_OUTTOR, OFPFC_ADD, flow_param);
	}

	CFlowMod::clear_flow_param(flow_param);
}

#endif

//***********************************************floating ip flow *****************************************************************************//
INT4 CFlowMgr::install_add_fabric_controller_flow(const CSmartPtr<CSwitch>& sw)
{
	INT4 ret = BNC_ERR;
	if (0 == CServer::getInstance()->getControllerIp()) 
	{
		return BNC_ERR;
	}

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst= CServer::getInstance()->getControllerIp();
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	ret = install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, 20,
						 FABRIC_TABLE_INPUT, OFPFC_ADD, flow_param);
	

	CFlowMod::clear_flow_param(flow_param);
	return ret;
}

INT4 CFlowMgr::install_fabric_floating_internal_subnet_flow(const CSmartPtr<CSwitch>& sw, INT4 type, UINT4 dst_ip, UINT4 dst_mask)
{
	flow_param_t* flow_param =CFlowMod::init_flow_param();

	UINT2 table_id = FABRIC_TABLE_TAGFORWARD_TOR; 
	UINT1 action = 0;
	if (FLOATING_ADD == type) 
	{
		action = OFPFC_ADD;
	}
	else if (FLOATING_DEL == type) 
	{
		action = OFPFC_DELETE;
	}
	else 
	{
		CFlowMod::clear_flow_param(flow_param);
		return BNC_ERR;
	}
	LOG_DEBUG_FMT(" install_fabric_floating_internal_subnet_flow action= %d  dst_ip=0x%x dst_mask=%d !!!", action, dst_ip , dst_mask);

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst= ntohl(dst_ip);
	flow_param->match_param->ipv4_dst_prefix = (dst_mask);

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);


	install_fabric_flows(sw, 
						 FABRIC_IMPL_HARD_TIME_OUT, 
						 FABRIC_IMPL_IDLE_TIME_OUT, 
						 FABRIC_PRIORITY_FLOATING_INTERNAL_SUBNET_FLOW,
						 FABRIC_TABLE_INPUT, 
						 action, flow_param);

	CFlowMod::clear_flow_param(flow_param);
	
	return BNC_OK;

}

INT4 CFlowMgr::install_proactive_floating_host_to_external_flow(const CSmartPtr<CSwitch>& sw, INT4 type, UINT4 match_ip, UINT1* match_mac, UINT4 mod_src_ip, UINT1* mod_dst_mac, UINT4 vlan_id)
{
	INT4 ret = BNC_ERR;
	UINT1 action = 0;
	switch(type)
	{
		case FLOATING_ADD:
			action = OFPFC_ADD;
			break;
		case FLOATING_DEL:
			action = OFPFC_DELETE;
			break;
		default:
			return BNC_ERR;
	}


	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT2 table_id = FABRIC_TABLE_TAGFORWARD_TOR;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.ipv4_src = ntohl(mod_src_ip);
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.vlan_vid = vlan_id;

	memcpy(flow_param->match_param->eth_src, match_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(match_ip);

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	LOG_DEBUG_FMT(" install_proactive_floating_host_to_external_flow sw->ipv4= 0x%x oxm.ipv4_src=0x%x oxm.vlan_vid=%d !!!", sw->getSwIp(),oxm.ipv4_src , oxm.vlan_vid);

	ret = install_fabric_flows(sw, 
						 FABRIC_IMPL_HARD_TIME_OUT, 
						 FABRIC_IMPL_IDLE_TIME_OUT, 
						 FABRIC_PRIORITY_FLOATING_EXTERNAL_HOST_SUBNET_FLOW,
						 FABRIC_TABLE_FLOATINGIP_OUTTOR, 
						 action, 
						 flow_param);

	CFlowMod::clear_flow_param(flow_param);


	return ret;
}


INT4 CFlowMgr::install_floatingip_set_vlan_in_flow(const CSmartPtr<CSwitch>& sw, UINT4 match_ip, UINT4 mod_dst_ip, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port)
{

	INT4 ret = BNC_ERR;
	if(sw.isNull())
	{
		return BNC_ERR;
	}


	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT2 flow_table_id =  FABRIC_TABLE_QOS_INVM;
	UINT2 priority_level = FABRIC_PRIORITY_OUTPUT_TABLE_FLOATING_INVM_FLOW;
	
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	//oxm.ipv4_dst = ntohl(mod_dst_ip);
	oxm.vlan_vid = vlan_id;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);

	ret = install_fabric_flows(sw, 
						 FABRIC_IMPL_HARD_TIME_OUT, 
						 FABRIC_IMPL_IDLE_TIME_OUT, 
						 priority_level,
						 flow_table_id, 
						 OFPFC_ADD, 
						 flow_param);

	CFlowMod::clear_flow_param(flow_param);

	return ret;
}


INT4 CFlowMgr::install_add_FloatingIP_ToFixIP_OutputToHost_flow(CSmartPtr<CHost>& fixed_port, UINT4 floatingip)
{
	INT4 ret = BNC_ERR;
	UINT4 out_port = 0;
	gn_oxm_t oxm;
	
	if((fixed_port.isNull())||(fixed_port->getSw().isNull())||(0 == fixed_port->getPortNo()))
	{
		return BNC_ERR;
	}

	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	out_port = fixed_port->getPortNo();
	oxm.ipv4_dst = ntohl(fixed_port->getIp());

	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(floatingip);

	
	LOG_DEBUG_FMT(" install_add_FloatingIP_ToFixIP_OutputToHost_flow match_param->ipv4_dst= 0x%x  oxm.ipv4_dst=0x%x fixed_port->getPortNo()=%d !!!", flow_param->match_param->ipv4_dst, oxm.ipv4_dst , fixed_port->getPortNo());
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);

	CSmartPtr<CSwitch> sw = fixed_port->getSw();
	ret = install_fabric_flows(sw, 
		FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_IDLE_TIME_OUT, 
		FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_HARD_TIME_OUT, 
		FABRIC_PRIORITY_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW+1,
		FABRIC_TABLE_FORWARD_INVM, 
		OFPFC_ADD, 
		flow_param); 
	LOG_DEBUG_FMT(" install_add_FloatingIP_ToFixIP_OutputToHost_flow 0x%x  state=%d floatingip=0x%x fixed_port->getPortNo()=%d fixed_port->getIp()=0x%x!!!", fixed_port->getSw()->getSwIp(), fixed_port->getSw()->getState(),floatingip , fixed_port->getPortNo(), fixed_port->getIp());

	CFlowMod::clear_flow_param(flow_param);

	return ret;
}
INT4 CFlowMgr::install_remove_FloatingIP_ToFixIP_OutputToHost_flow(CSmartPtr<CHost>& fixed_port, UINT4 floatingip)
{
	UINT2 out_port = 0;
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	
    if (fixed_port.isNull())
        return BNC_ERR;

	out_port = fixed_port->getPortNo();
	CSmartPtr<CSwitch> sw = fixed_port->getSw();
	if(sw.isNull()||(0 == out_port))
	{
		LOG_ERROR("sw is Null or Port no is 0!!!");
		CFlowMod::clear_flow_param(flow_param);
		return BNC_ERR;
	}
	oxm.ipv4_dst = ntohl(fixed_port->getfixIp());

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(floatingip);

	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);

	
	LOG_INFO_FMT(" %s_%d floatingip=0x%x\n",FN,LN,floatingip);
	install_fabric_flows(sw, 
		FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_IDLE_TIME_OUT, 
		FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_HARD_TIME_OUT, 
		FABRIC_PRIORITY_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW,
		FABRIC_TABLE_FORWARD_INVM, 
		OFPFC_DELETE, 
		flow_param);


	CFlowMod::clear_flow_param(flow_param);

	return BNC_OK;
}


INT4 CFlowMgr::delete_fabric_input_flow_by_ip(const CSmartPtr<CSwitch>& sw, UINT4 ip)
{
	if(sw.isNull())
	{
		return BNC_ERR;
	}
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT2 flow_table_id = FABRIC_TABLE_QOS_INVM; 

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	CFlowMod::clear_flow_param(flow_param);
	return BNC_OK;
}


INT4 CFlowMgr::delete_fabric_flow_by_ip(const CSmartPtr<CSwitch>& sw, UINT4 ip, UINT2 table_id)
{
	if(sw.isNull())
		return BNC_ERR;
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);

	install_fabric_flows(sw, 0, 0, 0, table_id, OFPFC_DELETE, flow_param);

	CFlowMod::clear_flow_param(flow_param);
	return BNC_OK;
}

INT4 CFlowMgr::delete_fabric_flow_by_mac(const CSmartPtr<CSwitch>& sw, UINT1* mac, UINT2 table_id)
{
	if(sw.isNull())
		return BNC_ERR;
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);

	install_fabric_flows(sw, 0, 0, 0, table_id, OFPFC_DELETE, flow_param);

	CFlowMod::clear_flow_param(flow_param);
	return BNC_OK;
}

//匹配:eth_dst,ip_proto,tcp_dst/udp_dst端口
INT4 CFlowMgr::delete_fabric_input_flow_by_dstmac_proto(const CSmartPtr<CSwitch>& sw, UINT1* dst_mac, UINT4 src_ip, UINT2 dst_port, UINT2 proto)
{
	if(sw.isNull())
		return BNC_ERR;
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT2 flow_table_id = FABRIC_TABLE_QOS_INVM;

	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_dst, dst_mac, 6);
	flow_param->match_param->ip_proto = proto;
	flow_param->match_param->ipv4_src = ntohl(src_ip);
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto) ? dst_port : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto) ? dst_port : 0;

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	CFlowMod::clear_flow_param(flow_param);
	return BNC_OK;
}

INT4 CFlowMgr::delete_fabric_input_flow_by_srcmac_proto(const CSmartPtr<CSwitch>& sw, UINT1* src_mac, UINT4 dst_ip, UINT2 src_port, UINT2 proto)
{
	if(sw.isNull())
		return BNC_ERR;
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT2 flow_table_id = FABRIC_TABLE_NAT_OUTTOR;

	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_src, src_mac, 6);
	flow_param->match_param->ip_proto = proto;
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto) ? src_port : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto) ? src_port : 0;

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	CFlowMod::clear_flow_param(flow_param);
	return BNC_OK;
}

INT4 CFlowMgr::install_ip_controller_flow(const CSmartPtr<CSwitch>& sw, UINT4 ip, UINT2 table_id)
{
	if(sw.isNull())
		return BNC_ERR;
	
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;
	
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);
	install_fabric_flows(sw, 5,FABRIC_ARP_HARD_TIME_OUT, 10, table_id, OFPFC_ADD, flow_param);
	CFlowMod::clear_flow_param(flow_param);
	return BNC_OK;
}

INT4 CFlowMgr::install_firewallIn_withPort_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT4 dstIP, 
    UINT4 srcIPMask, UINT1 protocol, UINT2 srcPort, UINT2 dstPort, UINT2 priority, UINT1 command, BOOL accept)
{
	if (sw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;

	if (srcIP > 0)
	{
		flow_param->match_param->ipv4_src = srcIP;
		flow_param->match_param->ipv4_src_prefix = srcIPMask;
	}
	if (dstIP > 0)
	{
		flow_param->match_param->ipv4_dst = dstIP;
	}
	if (protocol > 0)
	{
		flow_param->match_param->ip_proto = protocol;
		if (IPPROTO_TCP == protocol)
		{
			flow_param->match_param->tcp_dst = dstPort;
		}
		else if (IPPROTO_UDP == protocol)
		{
			flow_param->match_param->udp_dst = dstPort;
		}
		else if (IPPROTO_ICMP == protocol)
		{
			flow_param->match_param->icmpv4_type = srcPort;
			flow_param->match_param->icmpv4_code = dstPort;
		}
	}
	
    CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	UINT1 JumpToTable = FABRIC_TABLE_QOS_INVM;
    UINT4 port = OFPP13_TABLE;
    if (accept)
    	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
    else
    	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_IDLE_TIME_OUT, FABRIC_FIREWALL_HARD_TIME_OUT,
        (FABRIC_PRIORITY_FIREWALL_IN_FLOW+priority), FABRIC_TABLE_FIREWALL_INVM, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(dstIP), dstIpStr);

    if (IPPROTO_ICMP != protocol)
    {
        LOG_WARN_FMT("%s FIREWALL-IN FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s/%u]dst[%s]port[%u]action[%s]",
            (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
            sw->getDpid(), swIpStr, FIREWALL_DIRECTION_IN, 
            (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
            priority, srcIpStr, srcIPMask, dstIpStr, dstPort, accept?"accept":"drop");
    }
    else
    {
        LOG_WARN_FMT("%s FIREWALL-IN FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s/%u]dst[%s]type[%u]code[%u]action[%s]",
            (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
            sw->getDpid(), swIpStr, FIREWALL_DIRECTION_IN, 
            (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
            priority, srcIpStr, srcIPMask, dstIpStr, srcPort, dstPort, accept?"accept":"drop");
    }

	return BNC_OK;
}

INT4 CFlowMgr::install_firewallOut_withPort_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT4 dstIP, 
    UINT4 dstIPMask, UINT1 protocol, UINT4 srcPort, UINT4 dstPort, UINT2 priority, UINT1 command, BOOL accept)
{
	if (sw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;

	if (srcIP > 0)
	{
		flow_param->match_param->ipv4_src = srcIP;
	}
	if (dstIP > 0)
	{
		flow_param->match_param->ipv4_dst = dstIP;
        flow_param->match_param->ipv4_dst_prefix = dstIPMask;
	}
	if (protocol > 0)
	{
		flow_param->match_param->ip_proto = protocol;
		if (IPPROTO_TCP == protocol)
		{
			flow_param->match_param->tcp_src = srcPort;
		}
		else if (IPPROTO_UDP == protocol)
		{
			flow_param->match_param->udp_src = srcPort;
		}
		else if (IPPROTO_ICMP == protocol)
		{
			flow_param->match_param->icmpv4_type = srcPort;
			flow_param->match_param->icmpv4_code = dstPort;
		}
	}
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	UINT1 JumpToTable = FABRIC_TABLE_QOS_OUTTOR;
    UINT4 port = OFPP13_TABLE;
    if (accept)
    	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
    else
    	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_IDLE_TIME_OUT, FABRIC_FIREWALL_HARD_TIME_OUT,
        (FABRIC_PRIORITY_FIREWALL_OUT_FLOW+priority), FABRIC_TABLE_FIREWALL_OUTTOR, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    if (IPPROTO_ICMP != protocol)
    {
        LOG_WARN_FMT("%s FIREWALL-OUT FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s]port[%u]dst[%s/%u]action[%s]",
            (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
            sw->getDpid(), swIpStr, FIREWALL_DIRECTION_OUT, 
            (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
            priority, srcIpStr, srcPort, dstIpStr, dstIPMask, accept?"accept":"drop");
    }
    else
    {
        LOG_WARN_FMT("%s FIREWALL-OUT FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s]type[%u]code[%u]dst[%s/%u]action[%s]",
            (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
            sw->getDpid(), swIpStr, FIREWALL_DIRECTION_OUT, 
            (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
            priority, srcIpStr, srcPort, dstPort, dstIpStr, dstIPMask, accept?"accept":"drop");
    }
    
	return BNC_OK;
}

INT4 CFlowMgr::install_firewallIn_withoutPort_flow(const CSmartPtr<CSwitch>& sw, 
    UINT4 srcIP, UINT4 dstIP, UINT4 srcIPMask, UINT1 protocol, UINT2 priority, UINT1 command, BOOL accept)
{
	if (sw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;

	if (srcIP > 0)
	{
		flow_param->match_param->ipv4_src = srcIP;
		flow_param->match_param->ipv4_src_prefix = srcIPMask;
	}
	if (dstIP > 0)
	{
		flow_param->match_param->ipv4_dst = dstIP;
	}
	if (protocol > 0)
	{
		flow_param->match_param->ip_proto = protocol;
	}
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	UINT1 JumpToTable = FABRIC_TABLE_QOS_INVM;
    UINT4 port = OFPP13_TABLE;
    if (accept)
    	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
    else
    	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_IDLE_TIME_OUT, FABRIC_FIREWALL_HARD_TIME_OUT,
        (FABRIC_PRIORITY_FIREWALL_IN_FLOW+priority), FABRIC_TABLE_FIREWALL_INVM, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    LOG_WARN_FMT("%s FIREWALL-IN FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s/%u]dst[%s]action[%s]",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        sw->getDpid(), swIpStr, FIREWALL_DIRECTION_IN, 
        (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
        priority, srcIpStr, srcIPMask, dstIpStr, accept?"accept":"drop");

	return BNC_OK;
}

INT4 CFlowMgr::install_firewallOut_withoutPort_flow(const CSmartPtr<CSwitch>& sw, 
    UINT4 srcIP, UINT4 dstIP, UINT4 dstIPMask, UINT1 protocol, UINT2 priority, UINT1 command, BOOL accept)
{
	if (sw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;

	if (srcIP > 0)
	{
		flow_param->match_param->ipv4_src = srcIP;
	}
	if (dstIP > 0)
	{
		flow_param->match_param->ipv4_dst = dstIP;
        flow_param->match_param->ipv4_dst_prefix = dstIPMask;
	}
	if (protocol > 0)
	{
		flow_param->match_param->ip_proto = protocol;
	}
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	UINT1 JumpToTable = FABRIC_TABLE_QOS_OUTTOR;
    UINT4 port = OFPP13_TABLE;
    if (accept)
    	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
    else
    	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_IDLE_TIME_OUT, FABRIC_FIREWALL_HARD_TIME_OUT,
        (FABRIC_PRIORITY_FIREWALL_OUT_FLOW+priority), FABRIC_TABLE_FIREWALL_OUTTOR, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    LOG_WARN_FMT("%s FIREWALL-OUT FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s]dst[%s/%u]action[%s]",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        sw->getDpid(), swIpStr, FIREWALL_DIRECTION_OUT, 
        (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
        priority, srcIpStr, dstIpStr, dstIPMask, accept?"accept":"drop");

	return BNC_OK;
}

INT4 CFlowMgr::install_firewallIn_gotoController_flow(const CSmartPtr<CSwitch>& sw, 
    UINT4 srcIP, UINT4 dstIP, UINT4 srcIPMask, UINT1 protocol, UINT2 priority, UINT1 command, BOOL accept)
{
	if (sw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;

	if (srcIP > 0)
	{
		flow_param->match_param->ipv4_src = srcIP;
        flow_param->match_param->ipv4_src_prefix = srcIPMask;
	}
	if (dstIP > 0)
	{
		flow_param->match_param->ipv4_dst = dstIP;
	}
	if (protocol > 0)
	{
		flow_param->match_param->ip_proto = protocol;
	}
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	UINT4 port = accept ? OFPP13_CONTROLLER : OFPP13_TABLE;
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_IDLE_TIME_OUT, FABRIC_FIREWALL_HARD_TIME_OUT,
        FABRIC_PRIORITY_FIREWALL_IN_FLOW+priority, FABRIC_TABLE_FIREWALL_INVM, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    LOG_WARN_FMT("%s gotoController FIREWALL FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s/%u]dst[%s]action[%s]",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        sw->getDpid(), swIpStr, FIREWALL_DIRECTION_IN, 
        (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
        priority, srcIpStr, srcIPMask, dstIpStr, accept?"accept":"drop");

	return BNC_OK;
}

INT4 CFlowMgr::install_firewallOut_gotoController_flow(const CSmartPtr<CSwitch>& sw, 
    UINT4 srcIP, UINT4 dstIP, UINT4 dstIPMask, UINT1 protocol, UINT2 priority, UINT1 command, BOOL accept)
{
	if (sw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;

	if (srcIP > 0)
	{
		flow_param->match_param->ipv4_src = srcIP;
	}
	if (dstIP > 0)
	{
		flow_param->match_param->ipv4_dst = dstIP;
        flow_param->match_param->ipv4_dst_prefix = dstIPMask;
	}
	if (protocol > 0)
	{
		flow_param->match_param->ip_proto = protocol;
	}
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	UINT4 port = accept ? OFPP13_CONTROLLER : OFPP13_TABLE;
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_IDLE_TIME_OUT, FABRIC_FIREWALL_HARD_TIME_OUT,
        FABRIC_PRIORITY_FIREWALL_OUT_FLOW+priority, FABRIC_TABLE_FIREWALL_OUTTOR, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    LOG_WARN_FMT("%s gotoController FIREWALL FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s]dst[%s/%u]action[%s]",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        sw->getDpid(), swIpStr, FIREWALL_DIRECTION_OUT, 
        (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
        priority, srcIpStr, dstIpStr, dstIPMask, accept?"accept":"drop");

	return BNC_OK;
}

INT4 CFlowMgr::install_firewallIn_ephemeral_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT4 dstIP, 
    UINT1 protocol, UINT4 srcPort, UINT4 dstPort, UINT2 priority, BOOL accept)
{
	if (sw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;

	if (srcIP > 0)
	{
		flow_param->match_param->ipv4_src = srcIP;
	}
	if (dstIP > 0)
	{
		flow_param->match_param->ipv4_dst = dstIP;
	}
	if (protocol > 0)
	{
		flow_param->match_param->ip_proto = protocol;
		if (IPPROTO_TCP == protocol)
		{
			flow_param->match_param->tcp_src = srcPort;
            flow_param->match_param->tcp_dst = dstPort;
		}
		else if (IPPROTO_UDP == protocol)
		{
			flow_param->match_param->udp_src = srcPort;
            flow_param->match_param->udp_dst = dstPort;
		}
		else
		{
            CFlowMod::clear_flow_param(flow_param);
            return BNC_OK;
		}
	}
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	UINT1 JumpToTable = FABRIC_TABLE_QOS_INVM;
    UINT4 port = OFPP13_TABLE;
    if (accept)
    	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
    else
    	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_EPHEMERAL_FLOW_IDLE_TIME_OUT, FABRIC_FIREWALL_EPHEMERAL_FLOW_HARD_TIME_OUT,
        (FABRIC_PRIORITY_FIREWALL_OUT_FLOW+priority), FABRIC_TABLE_FIREWALL_INVM, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    LOG_WARN_FMT("Installed ephemeral FIREWALL-IN FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s/%u]dst[%s/%u]action[%s]",
        sw->getDpid(), swIpStr, FIREWALL_DIRECTION_OUT, 
        (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
        priority, srcIpStr, srcPort, dstIpStr, dstPort, accept?"accept":"drop");
    
	return BNC_OK;
}

INT4 CFlowMgr::install_firewallOut_ephemeral_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT4 dstIP, 
    UINT1 protocol, UINT4 srcPort, UINT4 dstPort, UINT2 priority, BOOL accept)
{
	if (sw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;

	if (srcIP > 0)
	{
		flow_param->match_param->ipv4_src = srcIP;
	}
	if (dstIP > 0)
	{
		flow_param->match_param->ipv4_dst = dstIP;
	}
	if (protocol > 0)
	{
		flow_param->match_param->ip_proto = protocol;
		if (IPPROTO_TCP == protocol)
		{
			flow_param->match_param->tcp_src = srcPort;
            flow_param->match_param->tcp_dst = dstPort;
		}
		else if (IPPROTO_UDP == protocol)
		{
			flow_param->match_param->udp_src = srcPort;
            flow_param->match_param->udp_dst = dstPort;
		}
		else
		{
            CFlowMod::clear_flow_param(flow_param);
            return BNC_OK;
		}
	}
	
	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	UINT1 JumpToTable = FABRIC_TABLE_QOS_OUTTOR;
    UINT4 port = OFPP13_TABLE;
    if (accept)
    	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
    else
    	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_EPHEMERAL_FLOW_IDLE_TIME_OUT, FABRIC_FIREWALL_EPHEMERAL_FLOW_HARD_TIME_OUT,
        (FABRIC_PRIORITY_FIREWALL_OUT_FLOW+priority), FABRIC_TABLE_FIREWALL_OUTTOR, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    LOG_WARN_FMT("Installed ephemeral FIREWALL-OUT FLOW on switch[0x%llx][%s]: direction[%s]protocol[%s]priority[%u]src[%s/%u]dst[%s/%u]action[%s]",
        sw->getDpid(), swIpStr, FIREWALL_DIRECTION_OUT, 
        (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
        priority, srcIpStr, srcPort, dstIpStr, dstPort, accept?"accept":"drop");
    
	return BNC_OK;
}

INT4 CFlowMgr::install_portforwardIn_output_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT2 srcPort, UINT4 dstIP, 
    UINT2 dstPort, UINT1 protocol, UINT1* inMac, UINT4 inIP, UINT2 inPort, UINT2 tag, UINT4 outputPort, UINT1 command)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
    if (NULL != inMac)
    	memcpy(oxm.eth_dst, inMac, 6);
    if (0 != inIP)
        oxm.ipv4_dst = inIP;
    if (0 != inPort)
    {
        oxm.tcp_dst = (IPPROTO_TCP == protocol) ? inPort : 0;
        oxm.udp_dst = (IPPROTO_UDP == protocol) ? inPort : 0;
    }
    if (0 != tag)
    	oxm.vlan_vid = tag;

	flow_param->match_param->ipv4_dst = dstIP;
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = protocol;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == protocol) ? dstPort : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == protocol) ? dstPort : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) 
    	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&outputPort);

	install_fabric_flows(sw, FABRIC_PORTFORWARD_IDLE_TIME_OUT, FABRIC_PORTFORWARD_HARD_TIME_OUT, 
        FABRIC_PRIORITY_PORTFORWARD_IN_FLOW+1, FABRIC_TABLE_FORWARD_INVM, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 inMacStr[20] = {0};
    if (NULL != inMac)
        mac2str(inMac, inMacStr);
    INT1 swIpStr[20] = {0}, dstIpStr[20] = {0}, inIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    number2ip(htonl(inIP), inIpStr);

    LOG_WARN_FMT("%s PORTFORWARD-IN FLOW on switch[0x%llx][%s]: dst ip[%s]port[%u] ==> in mac[%s]ip[%s]port[%u], protocol[%d]",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        sw->getDpid(), swIpStr, dstIpStr, dstPort, inMacStr, inIpStr, inPort, protocol);

    return BNC_OK;
}

INT4 CFlowMgr::install_portforwardOut_output_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT2 srcPort, UINT4 dstIP, 
    UINT2 dstPort, UINT1 protocol, UINT1* gwMac, UINT4 outIP, UINT2 outPort, UINT2 tag, UINT4 outputPort, UINT1 command)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	gn_oxm_t oxm = {0};
    if (NULL != gwMac)
    	memcpy(oxm.eth_dst, gwMac, 6);
    if (0 != outIP)
    	oxm.ipv4_src = outIP;
    if (0 != outPort)
    {
        oxm.tcp_src = (IPPROTO_TCP == protocol) ? outPort : 0;
        oxm.udp_src = (IPPROTO_UDP == protocol) ? outPort : 0;
    }
    if (0 != tag)
    	oxm.vlan_vid = tag;

	flow_param->match_param->ipv4_src = srcIP;
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = protocol;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == protocol) ? srcPort : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == protocol) ? srcPort : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) 
		CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&outputPort);

	install_fabric_flows(sw, FABRIC_PORTFORWARD_IDLE_TIME_OUT, FABRIC_PORTFORWARD_HARD_TIME_OUT, 
        FABRIC_PRIORITY_PORTFORWARD_OUT_FLOW+1, FABRIC_TABLE_PORTFORWARD_OUTTOR, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0}, outIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    number2ip(htonl(outIP), outIpStr);

    LOG_WARN_FMT("%s PORTFORWARD-OUT FLOW on switch[0x%llx][%s]: src ip[%s]port[%u] ==> out ip[%s]port[%u], protocol[%d]",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        sw->getDpid(), swIpStr, srcIpStr, srcPort, outIpStr, outPort, protocol);

    return BNC_OK;
}

INT4 CFlowMgr::install_portforwardIn_gotoController_flow(
    const CSmartPtr<CSwitch>& gwSw, UINT4 dstIP, UINT1 protocol, UINT1 command)
{
	if (gwSw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = dstIP;
	flow_param->match_param->ip_proto = protocol;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	UINT4 port = OFPP13_CONTROLLER;
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(gwSw, FABRIC_PORTFORWARD_IDLE_TIME_OUT, FABRIC_PORTFORWARD_HARD_TIME_OUT,
        FABRIC_PRIORITY_PORTFORWARD_IN_FLOW, FABRIC_TABLE_FORWARD_INVM, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(gwSw->getSwIp()), swIpStr);
    number2ip(htonl(dstIP), dstIpStr);
    LOG_WARN_FMT("%s gotoController PORTFORWARD FLOW on switch[0x%llx][%s]: dstIP[%s]protocol[%d] ==> controller",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        gwSw->getDpid(), swIpStr, dstIpStr, protocol);

	return BNC_OK;
}

INT4 CFlowMgr::install_portforwardOut_gotoController_flow(
    const CSmartPtr<CSwitch>& hostSw, UINT4 srcIP, UINT1 protocol, UINT1 command)
{
	if (hostSw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = srcIP;
	flow_param->match_param->ip_proto = protocol;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	UINT4 port = OFPP13_CONTROLLER;
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(hostSw, FABRIC_PORTFORWARD_IDLE_TIME_OUT, FABRIC_PORTFORWARD_HARD_TIME_OUT,
        FABRIC_PRIORITY_PORTFORWARD_IN_FLOW, FABRIC_TABLE_PORTFORWARD_OUTTOR, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0};
    number2ip(htonl(hostSw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    LOG_WARN_FMT("%s gotoController PORTFORWARD FLOW on switch[0x%llx][%s]: srcIP[%s]protocol[%d] ==> controller",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        hostSw->getDpid(), swIpStr, srcIpStr, protocol);

	return BNC_OK;
}

INT4 CFlowMgr::install_portforwardOut_external_flow(const CSmartPtr<CSwitch>& gwSw, 
    UINT4 srcIP, UINT1 protocol, UINT2 srcPort, UINT4 outputPort, UINT1 command, BOOL ephemeral)
{
	if (gwSw.isNull())
		return BNC_ERR;

	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = srcIP;
	flow_param->match_param->ip_proto = protocol;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == protocol) ? srcPort : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == protocol) ? srcPort : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&outputPort);

	install_fabric_flows(gwSw, 
        ephemeral?FABRIC_PORTFORWARD_EPHEMERAL_FLOW_IDLE_TIME_OUT:FABRIC_PORTFORWARD_IDLE_TIME_OUT, 
        ephemeral?FABRIC_PORTFORWARD_EPHEMERAL_FLOW_HARD_TIME_OUT:FABRIC_PORTFORWARD_HARD_TIME_OUT,
        FABRIC_PRIORITY_PORTFORWARD_OUT_FLOW+(ephemeral?1:0), 
        FABRIC_TABLE_PORTFORWARD_OUTTOR, command, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, srcIpStr[20] = {0};
    number2ip(htonl(gwSw->getSwIp()), swIpStr);
    number2ip(htonl(srcIP), srcIpStr);
    LOG_WARN_FMT("%s PORTFORWARD-OUT FLOW on switch[0x%llx][%s]: src IP[%s]port[%u]protocol[%d] ==> output:%u",
        (OFPFC_DELETE_STRICT==command)?"Uninstalled":(OFPFC_DELETE==command)?"Uninstalled":(OFPFC_ADD==command)?"Installed":"Unknown",
        gwSw->getDpid(), swIpStr, srcIpStr, srcPort, protocol, outputPort);

	return BNC_OK;
}

INT4 CFlowMgr::install_portforwardIn_ephemeral_flow(const CSmartPtr<CSwitch>& sw, UINT4 outIP, 
    UINT2 outPort, UINT1 protocol, UINT1* inMac, UINT4 inIP, UINT2 inPort, UINT2 tag, UINT4 outputPort)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
    if (NULL != inMac)
    	memcpy(oxm.eth_dst, inMac, 6);
    if (0 != inIP)
        oxm.ipv4_dst = inIP;
    if (0 != inPort)
    {
        oxm.tcp_dst = (IPPROTO_TCP == protocol) ? inPort : 0;
        oxm.udp_dst = (IPPROTO_UDP == protocol) ? inPort : 0;
    }
    if (0 != tag)
    	oxm.vlan_vid = tag;

	flow_param->match_param->ipv4_dst = outIP;
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = protocol;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == protocol) ? outPort : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == protocol) ? outPort : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) 
    	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&outputPort);

	install_fabric_flows(sw, FABRIC_PORTFORWARD_EPHEMERAL_FLOW_IDLE_TIME_OUT, FABRIC_PORTFORWARD_EPHEMERAL_FLOW_HARD_TIME_OUT, 
        FABRIC_PRIORITY_PORTFORWARD_IN_FLOW+1, FABRIC_TABLE_FORWARD_INVM, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 inMacStr[20] = {0};
    if (NULL != inMac)
        mac2str(inMac, inMacStr);
    INT1 swIpStr[20] = {0}, outIpStr[20] = {0}, inIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(outIP), outIpStr);
    number2ip(htonl(inIP), inIpStr);

    LOG_WARN_FMT("Installed ephemeral PORTFORWARD-IN FLOW on switch[0x%llx][%s]: out ip[%s]port[%u] ==> in mac[%s]ip[%s]port[%u], protocol[%d]",
        sw->getDpid(), swIpStr, outIpStr, outPort, inMacStr, inIpStr, inPort, protocol);

    return BNC_OK;
}

INT4 CFlowMgr::install_portforwardOut_ephemeral_flow(const CSmartPtr<CSwitch>& sw, UINT4 inIP, UINT2 inPort, 
    UINT1 protocol, UINT1* gwMac, UINT4 outIP, UINT2 outPort, UINT2 tag, UINT4 outputPort)
{
	flow_param_t* flow_param = CFlowMod::init_flow_param();

	gn_oxm_t oxm = {0};
    if (NULL != gwMac)
    	memcpy(oxm.eth_dst, gwMac, 6);
    if (0 != outIP)
    	oxm.ipv4_src = outIP;
    if (0 != outPort)
    {
        oxm.tcp_src = (IPPROTO_TCP == protocol) ? outPort : 0;
        oxm.udp_src = (IPPROTO_UDP == protocol) ? outPort : 0;
    }
    if (0 != tag)
    	oxm.vlan_vid = tag;

	flow_param->match_param->ipv4_src = inIP;
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = protocol;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == protocol) ? inPort : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == protocol) ? inPort : 0;

	CFlowMod::add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) 
		CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&outputPort);

	install_fabric_flows(sw, FABRIC_PORTFORWARD_EPHEMERAL_FLOW_IDLE_TIME_OUT, FABRIC_PORTFORWARD_EPHEMERAL_FLOW_HARD_TIME_OUT, 
        FABRIC_PRIORITY_PORTFORWARD_OUT_FLOW+1, FABRIC_TABLE_PORTFORWARD_OUTTOR, OFPFC_ADD, flow_param);

	CFlowMod::clear_flow_param(flow_param);

    INT1 swIpStr[20] = {0}, inIpStr[20] = {0}, outIpStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), swIpStr);
    number2ip(htonl(inIP), inIpStr);
    number2ip(htonl(outIP), outIpStr);

    LOG_WARN_FMT("Installed ephemeral PORTFORWARD-OUT FLOW on switch[0x%llx][%s]: in ip[%s]port[%u] ==> out ip[%s]port[%u], protocol[%d]",
        sw->getDpid(), swIpStr, inIpStr, inPort, outIpStr, outPort, protocol);

    return BNC_OK;
}

INT4 CFlowMgr::install_delete_mediaflow(INT4 svr_ip, INT4 client_ip)
{
	CSmartPtr<CHost> svrHost = CHostMgr::getInstance()->findHostByIp(svr_ip);
	CSmartPtr<CHost> clientHost = CHostMgr::getInstance()->findHostByIp(client_ip);

	if((svrHost.isNull())|| (clientHost.isNull()))
	{
		return BNC_ERR;
	}
	flow_param_t* flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->ipv4_dst = ntohl(client_ip);
	flow_param->match_param->eth_type = ETHER_IP;

	install_fabric_flows(svrHost->getSw(), 0, 0, 0, FABRIC_TABLE0, OFPFC_DELETE, flow_param);
	CFlowMod::clear_flow_param(flow_param);

	flow_param = CFlowMod::init_flow_param();
	flow_param->match_param->ipv4_dst = ntohl(svr_ip);
	flow_param->match_param->eth_type = ETHER_IP;

	install_fabric_flows(clientHost->getSw(), 0, 0, 0, FABRIC_TABLE0, OFPFC_DELETE, flow_param);
	CFlowMod::clear_flow_param(flow_param);

	return BNC_OK;

}

INT4 CFlowMgr::add_flow_entry(const CSmartPtr<CSwitch>& sw, gn_flow_t& flow)
{
    flow_mod_req_info_t flow_mod_req;
    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sendOfp13FlowMod(sw, (UINT1*)&flow_mod_req);

	CFlowMod::clear_instruction_data(flow.instructions);

    return BNC_OK;
}

INT4 CFlowMgr::del_flow_entry(const CSmartPtr<CSwitch>& sw, gn_flow_t& flow)
{
    flow_mod_req_info_t flow_mod_req;
    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_DELETE_STRICT;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = &flow;

    sendOfp13FlowMod(sw, (UINT1*)&flow_mod_req);

	CFlowMod::clear_instruction_data(flow.instructions);

    return BNC_OK;
}

BOOL CFlowMgr::sendOfp13FlowMod(CSmartPtr<CSwitch> sw, UINT1 *flowmod_req)
{
	UINT2 match_len = 0;
	UINT2 instruction_len = 0;
	UINT2 total_len = sizeof(struct ofp13_flow_mod);
	flow_mod_req_info_t *mod_info = (flow_mod_req_info_t *)flowmod_req;

//LOG_WARN_FMT("mod_info->xid=0x%x mod_info->flags=0x%x",mod_info->xid,mod_info->flags);
	Msg_Buf(struct ofp13_flow_mod);
	//struct ofp13_flow_mod *ofm = (struct ofp13_flow_mod *)buffer->getData();
	pMsg->header.version = OFP13_VERSION;
	pMsg->header.type = OFPT13_FLOW_MOD;
	pMsg->header.length = htons(total_len);
	pMsg->header.xid = (mod_info->xid != OF13_DEFAULT_XID) ? mod_info->xid : randUint32();

	pMsg->cookie = htonll(0x0);
	pMsg->cookie_mask = htonll(0x0);
	pMsg->table_id = mod_info->flow->table_id;
	pMsg->command = mod_info->command;
	pMsg->idle_timeout = htons(mod_info->flow->idle_timeout);
	pMsg->hard_timeout = htons(mod_info->flow->hard_timeout);
	pMsg->priority = htons(mod_info->flow->priority);
	pMsg->buffer_id = htonl(mod_info->buffer_id);                //Default 0xffffffff
	pMsg->out_port = htonl(mod_info->out_port);                  //Default 0xffffffff
	pMsg->out_group = htonl(mod_info->out_group);                //Default 0xffffffff
	pMsg->flags = htons(mod_info->flags);                        //Default OFPFF13_SEND_FLOW_REM
	pMsg->pad[0] = 0x0;
	pMsg->pad[1] = 0x0;

	pMsg->match.type = htons(mod_info->flow->match.type);        //Default OFPMT_OXM
	match_len = CFlowMod::of13_add_match(&pMsg->match, &(mod_info->flow->match));
	pMsg->match.length = htons(match_len);
	total_len = total_len - sizeof(struct ofpx_match) + ALIGN_8(match_len);

	instruction_len = CFlowMod::of13_add_instruction((UINT1*)(pMsg)+ total_len, mod_info->flow);
	total_len += ALIGN_8(instruction_len);
	pMsg->header.length = htons(total_len);

	g_sendPktMutex.lock();
	 //������Ϣ
	INT4 ret = sendMsgOut(sw->getSockfd(), (INT1*)pMsg, total_len);
	g_sendPktMutex.unlock();
	if (!ret)
		LOG_ERROR("send of13 flow mod msg failure");

    INT4 event = (OFPFC_ADD == mod_info->command) ? EVENT_TYPE_FLOW_TABLE_ADD : 
                 (OFPFC_MODIFY == mod_info->command) ? EVENT_TYPE_FLOW_TABLE_MOD : 
                 (OFPFC_MODIFY_STRICT == mod_info->command) ? EVENT_TYPE_FLOW_TABLE_MOD_STRICT : 
                 (OFPFC_DELETE == mod_info->command) ? EVENT_TYPE_FLOW_TABLE_DEL : 
                 (OFPFC_DELETE_STRICT == mod_info->command) ? EVENT_TYPE_FLOW_TABLE_DEL_STRICT : 
                 EVENT_TYPE_MAX;
    INT4 reason = (OFPFC_ADD == mod_info->command) ? EVENT_REASON_FLOW_TABLE_ADD : 
                  (OFPFC_MODIFY == mod_info->command) ? EVENT_REASON_FLOW_TABLE_MOD : 
                  (OFPFC_MODIFY_STRICT == mod_info->command) ? EVENT_REASON_FLOW_TABLE_MOD_STRICT : 
                  (OFPFC_DELETE == mod_info->command) ? EVENT_REASON_FLOW_TABLE_DEL : 
                  (OFPFC_DELETE_STRICT == mod_info->command) ? EVENT_REASON_FLOW_TABLE_DEL_STRICT : 
                  EVENT_REASON_MAX;
    CFlowTableEventReportor::getInstance()->report(event, reason, sw, *(mod_info->flow));

	return TRUE;
}

//by:yhy ��sw���ղ���װ��һ��������
INT4 CFlowMgr::install_fabric_flows(CSmartPtr<CSwitch> sw,
						  UINT2 idle_timeout,
						  UINT2 hard_timeout,
						  UINT2 priority,
						  UINT1 table_id,
						  UINT1 command,
						  flow_param_t* flow_param)
{
	if(sw.isNull() || (SW_STATE_STABLE != sw->getState()))
		return BNC_ERR;

	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == sw->getManageMode()))
	{
		//LOG_ERROR("switch manage mode is on!");
		return BNC_ERR;
	}

    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return BNC_ERR;
	
    gn_flow_t flow;
    memset(&flow, 0, sizeof(gn_flow_t));
	flow.idle_timeout = idle_timeout;
	flow.hard_timeout = hard_timeout;
	flow.priority = priority;
	flow.table_id = table_id;
	flow.match.type = OFPMT_OXM;
	CFlowMod::set_flow_match(&flow.match.oxm_fields, flow_param->match_param);
	flow.instructions = CFlowMod::set_flow_instruction(flow.instructions, flow_param->instruction_param);
	if(flow.instructions)
		CFlowMod::set_flow_action(flow.instructions, flow_param->action_param, OFPIT_APPLY_ACTIONS);
	if(flow.instructions)
		CFlowMod::set_flow_action(flow.instructions, flow_param->write_action_param, OFPIT_WRITE_ACTIONS);

	flow_mod_req_info_t flow_mod_req;
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = command;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;

    sendOfp13FlowMod(sw, (UINT1*)&flow_mod_req);

	CFlowMod::clear_instruction_data(flow.instructions);

	return BNC_OK;
}


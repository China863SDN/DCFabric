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
*   File Name   : CPhysicalSwitch.cpp                                         *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "CFlowMod.h"
#include "CControl.h"
#include "CPhysicalSwitch.h"

CPhysicalSwitch::CPhysicalSwitch()
{
}

CPhysicalSwitch::CPhysicalSwitch(CSmartPtr<CSwitch>& sw):
    CSwitch(sw)
{
}

CPhysicalSwitch::~CPhysicalSwitch()
{
}

INT4 CPhysicalSwitch::toBeOverrided()
{
    return BNC_OK;
}

INT4 CPhysicalSwitch::flowInstall(CSmartPtr<CSwitch > dst_sw, flow_param_t* &flow_param,   UINT2& goto_Table)
{
	if(dst_sw.isNull()||(NULL == flow_param))
	{
		return BNC_ERR;
	}
	UINT4 outport	= CControl::getInstance()->getTopoMgr().get_out_port_between_switch(this->getDpid(), dst_sw->getDpid());
	if (0 == outport)
	{
		LOG_ERROR_FMT("output=%d this->getDpid()=0x%llx dst_sw->getDpid()=0x%llx",outport, this->getDpid(),dst_sw->getDpid() );
        return BNC_ERR;
    }
	
	UINT4* outport_param  = (UINT4 *)malloc(sizeof(UINT4));
	*outport_param = outport;
	//LOG_ERROR_FMT("this->getDpid()=0x%x dst_sw->getDpid()=0x%x outport=%d ",this->getDpid(), dst_sw->getDpid(), outport);
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)outport_param);
	
	//LOG_ERROR_FMT("param=0x%x outport=%d flow_param->action_param->param=%d",flow_param->action_param->param, *(UINT4*)outport_param, *(UINT4*)flow_param->action_param->param);
	return BNC_OK;
	
}
INT4 CPhysicalSwitch::flowAllocMemFree(void * mem)
{
	if(NULL != mem)
	{
		free(mem);
	}
	return BNC_OK;
}

INT4  CPhysicalSwitch::flow_fabric_nat_sameswitch_host2external(flow_param_t* &flow_param, gn_oxm_t &oxm)
{	
	if(NULL == flow_param)
	{
		return BNC_ERR;
	}
	return BNC_OK;
}
INT4  CPhysicalSwitch::flow_fabric_nat_sameswitch_external2host(flow_param_t* &flow_param, gn_oxm_t &oxm)
{
	if(NULL == flow_param)
	{
		return BNC_ERR;
	}
	return BNC_OK;
}



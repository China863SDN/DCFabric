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
*   File Name   : CPica8PhySwitch.cpp                                         *
*   Author      : bnc ycy                                                 *
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
#include "CPica8PhySwitch.h"

CPica8PhySwitch::CPica8PhySwitch()
{
}

CPica8PhySwitch::CPica8PhySwitch(CSmartPtr<CSwitch>& sw):
    CPhysicalSwitch(sw)
{
}

CPica8PhySwitch::~CPica8PhySwitch()
{
}



INT4  CPica8PhySwitch::flow_fabric_nat_sameswitch_host2external(flow_param_t* &flow_param, gn_oxm_t &oxm)
{	
	if(NULL == flow_param)
	{
		return BNC_ERR;
	}
	oxm.vlan_vid = 4095;
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	return BNC_OK;
}
INT4  CPica8PhySwitch::flow_fabric_nat_sameswitch_external2host(flow_param_t* &flow_param, gn_oxm_t &oxm)
{
	if(NULL == flow_param)
	{
		return BNC_ERR;
	}
	oxm.vlan_vid = 4095;
	CFlowMod::add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	return BNC_OK;
}




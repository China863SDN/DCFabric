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
*   File Name   : CPhysicalSwitch.h                                           *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CPHYSICALSWITCH_H
#define __CPHYSICALSWITCH_H

#include "CSwitch.h"

/*
 * 交换机类
 */
class CPhysicalSwitch : public CSwitch
{
public:
    CPhysicalSwitch();
    CPhysicalSwitch(CSmartPtr<CSwitch>& sw);
    virtual ~CPhysicalSwitch();

	virtual INT4 flowInstall(CSmartPtr<CSwitch > dst_sw, flow_param_t* &flow_param,  UINT2& goto_Table);
	virtual INT4 flowAllocMemFree(void * mem);
	virtual INT4 toBeOverrided();
	
	virtual INT4  flow_fabric_nat_sameswitch_host2external(flow_param_t* &flow_param, gn_oxm_t &oxm);
	virtual INT4  flow_fabric_nat_sameswitch_external2host(flow_param_t* &flow_param, gn_oxm_t &oxm);
	
};

#endif

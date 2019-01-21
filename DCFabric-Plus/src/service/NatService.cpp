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
*   File Name   : NatService.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "bnc-error.h"
#include "NatService.h"
#include "BaseNetwork.h"
#include "BaseNetworkManager.h"

NatService::NatService()
{
	
}
NatService::NatService(bnc::l3service::service_type type):L3Service(type)
{
	
}

NatService::~NatService()
{
	
}
INT4 NatService::IpHandler(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
        		 ip_t* pkt, packet_in_info_t* packetIn)
{
	return BNC_OK;
}

INT4 NatService::GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, UINT4 srcIp, UINT4 dstIp,  UINT2 srcPort, UINT2 dstPort, UINT2 proto) 
{
	if(srcHost.isNotNull())
	{
		Base_Network* networknode =  G_NetworkMgr.targetNetwork_Bysubnet_ID(srcHost->getSubnetId());
		if((NULL != networknode)&&(TRUE == networknode->get_router_external()))
		{
			return BNC_ERR;
		}
	}
	if((srcHost.isNotNull()&&(bnc::host::HOST_NORMAL == srcHost->getHostType()))
		&&(dstHost.isNull()||(bnc::host::HOST_EXTERNAL == dstHost->getHostType()))
		&&(dstIp != 0xffffffff))
	{
		return BNC_OK;
	}
	return BNC_ERR;
}



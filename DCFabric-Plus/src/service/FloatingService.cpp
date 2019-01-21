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
*   File Name   : FloatingService.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "bnc-error.h"
#include "CHost.h"
#include "FloatingService.h"
#include "CFloatingIp.h"
#include "CFloatingIpMgr.h"

FloatingService::FloatingService()
{
	
}
FloatingService::FloatingService(bnc::l3service::service_type type):L3Service(type)
{

}

FloatingService::~FloatingService()
{
	
}

INT4 FloatingService::IpHandler(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
        		 ip_t* pkt, packet_in_info_t* packetIn)
{
	if(srcHost.isNotNull()&&(bnc::host::HOST_FLOATINGIP == srcHost->getHostType()))
	{
		
	}

	if(dstHost.isNotNull()&&(bnc::host::HOST_FLOATINGIP == dstHost->getHostType()))
	{
		
	}
	
	return BNC_OK;
}

INT4 FloatingService::GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, UINT4 srcIp, UINT4 dstIp,  UINT2 srcPort, UINT2 dstPort, UINT2 proto) 
{
    //out
	if ((srcHost.isNotNull() && (bnc::host::HOST_NORMAL == srcHost->getHostType())) &&
        (dstHost.isNull() || (bnc::host::HOST_EXTERNAL == dstHost->getHostType())))
	{
        if (NULL != CFloatingIpMgr::getInstance()->findFloatingIpNodeByfixedIp(srcIp))
    		return BNC_OK;
	}

    //in
	if ((srcHost.isNull() || (bnc::host::HOST_EXTERNAL == srcHost->getHostType())) && 
        (dstHost.isNotNull() && (bnc::host::HOST_FLOATINGIP == dstHost->getHostType())))
	{
		return BNC_OK;
	}

	return BNC_ERR;
}


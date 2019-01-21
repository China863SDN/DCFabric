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
*   File Name   : UnknownHostService.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "CFlowMgr.h"
#include "COfMsgUtil.h"
#include "CArpFloodMgr.h"
#include "UnknownHostService.h"

UnknownHostService::UnknownHostService()
{
	
}
UnknownHostService::UnknownHostService(bnc::l3service::service_type type):L3Service(type)
{
	
}

UnknownHostService::~UnknownHostService()
{
	
}

INT4 UnknownHostService::IpHandler(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
        		 ip_t* pkt, packet_in_info_t* packetIn)
{

	LOG_INFO("Process dstHost Unknown Packet");
	LOG_ERROR_FMT("%s %d",FN,LN);
	CArpFloodMgr::getInstance()->AddArpRequestNode(pkt->dest,pkt->src,packetIn);
	return BNC_OK;
}

INT4 UnknownHostService::GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,UINT4 srcIp, UINT4 dstIp,  UINT2 srcPort, UINT2 dstPort, UINT2 proto ) 
{
	//LOG_ERROR_FMT("%s %d",FN,LN);
	if(srcHost.isNotNull()&&dstHost.isNull())
	{
		LOG_ERROR_FMT("%s %d",FN,LN);
		return BNC_OK;
	}
	return BNC_ERR;
}



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
*   File Name   : DhcpService.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "DhcpService.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "COfMsgUtil.h"

DhcpService::DhcpService()
{
	
}
DhcpService::DhcpService(bnc::l3service::service_type type):L3Service(type)
{

}

DhcpService::~DhcpService()
{
	
}

INT4 DhcpService::Dhcp_request_handle(packet_in_info_t* packetIn, CSmartPtr<CHost>& srcHost)
{
	if(NULL == packetIn)
	{
		return BNC_ERR;
	}

    CSmartPtr<CHost> host(NULL);
    if (srcHost.isNotNull())
    {
        host = srcHost;
    }
    else
    {
    	ip_t* pkt = (ip_t*)packetIn->data;
    	host = CHostMgr::getInstance()->findHostByMac(pkt->eth_head.src);
    	if (host.isNull())
    	{
    		return BNC_ERR;
    	}
    }

	CSmartPtr<CHost> dhcpport = CHostMgr::getInstance()->getHostDhcp(srcHost);
	if((dhcpport.isNull())||dhcpport->getSw().isNull()||(0 == dhcpport->getPortNo()))
	{
		
		COfMsgUtil::ofp13floodInternal(packetIn);
	}
	else
	{
		COfMsgUtil::forward(dhcpport->getSw(), dhcpport->getPortNo(), packetIn);
	}
	return BNC_OK;
}

INT4 DhcpService::Dhcp_reply_handle(packet_in_info_t* packetIn, CSmartPtr<CHost>& dstHost)
{
	if(NULL == packetIn)
	{
		return BNC_ERR;
	}
	if((dstHost.isNull())||dstHost->getSw().isNull()||(0 == dstHost->getPortNo()))
	{
		COfMsgUtil::ofp13floodInternal(packetIn);
	}
	else
	{
		COfMsgUtil::forward(dstHost->getSw(), dstHost->getPortNo(), packetIn);
	}
	return BNC_OK;
}

INT4 DhcpService::IpHandler(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
        		 ip_t* pkt, packet_in_info_t* packetIn)
{
	INT4 ret = BNC_ERR;
	if((NULL == pkt)||(NULL == pkt->data)||(NULL == packetIn))
	{
		return BNC_ERR;
	}
	udp_t* udppkt = (udp_t*)pkt->data;
	if(NULL == udppkt)
	{
		return BNC_ERR;
	}
	UINT2 dstPort = ntohs(udppkt->dport);
	LOG_ERROR_FMT("dstPort=%d srcport=%d srcHost=0x%p dstHost=0x%p", dstPort,ntohs(udppkt->sport),srcHost.getPtr(),dstHost.getPtr());
	switch(dstPort)
	{
		case 67:
			ret = Dhcp_request_handle(packetIn, srcHost);
			break;
		case 68:
			ret = Dhcp_reply_handle(packetIn, dstHost);
			break;
		default:
			ret = BNC_ERR;
			break;
	}
	return ret;
}
INT4 DhcpService::GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, UINT4 srcIp, UINT4 dstIp,  UINT2 srcPort, UINT2 dstPort, UINT2 proto ) 
{
	//LOG_ERROR_FMT("srcPort=%d dstPort=%d srcIp = 0x%x dstIp=0x%x",srcPort, dstPort,srcIp, dstIp);
	if(((((htons(68) == srcPort)&&(htons(67) == dstPort))||
		((htons(67) == srcPort)&&(htons(68) == dstPort)))&&(IPPROTO_UDP == proto))||
		(0xFFFFFFFF == dstIp))
	{
        LOG_ERROR_FMT("srcPort=%d dstPort=%d srcIp = 0x%x dstIp=0x%x",srcPort, dstPort,srcIp, dstIp);
		return BNC_OK;
	}
	return BNC_ERR;
}


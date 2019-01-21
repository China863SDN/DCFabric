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
*   File Name   : CPortforwardService.cpp                                     *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "CPortforwardService.h"
#include "CPortforwardPolicy.h"
#include "CHostMgr.h"
#include "CControl.h"
#include "CFlowMgr.h"

CPortforwardService::CPortforwardService():L3Service(bnc::l3service::SERVICE_PORTFORWARD)
{	
}

CPortforwardService::~CPortforwardService()
{
}

INT4 CPortforwardService::IpHandler(const CSmartPtr<CSwitch>& srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, ip_t* pkt, packet_in_info_t* packetIn)
{
    if (srcSw.isNull())
        return BNC_ERR;

	UINT4 srcIp = ntohl(pkt->src);
	UINT4 dstIp = ntohl(pkt->dest);
	UINT2 srcPort = 0;
	UINT2 dstPort = 0;
	INT4 protocol = pkt->proto;
	
	if (IPPROTO_TCP == protocol)
	{
		tcp_t* tcp = (tcp_t*)pkt->data;
		srcPort = ntohs(tcp->sport);
		dstPort = ntohs(tcp->dport);
	}
	else if (IPPROTO_UDP == protocol) 
	{
		udp_t* udp = (udp_t*)pkt->data;
		srcPort = ntohs(udp->sport);
		dstPort = ntohs(udp->dport);
	}
    else
    {
        return BNC_OK;
    }

    //in
	if ((srcHost.isNull() || (bnc::host::HOST_EXTERNAL == srcHost->getHostType())) && 
        (dstHost.isNotNull() && (bnc::host::HOST_GATEWAY == dstHost->getHostType())))
	{
        return CPortforwardPolicy::getInstance()->applyIn(dstIp, dstPort, protocol);
	}

    //out
	if ((srcHost.isNotNull() && (bnc::host::HOST_NORMAL == srcHost->getHostType())) && 
        (dstHost.isNull() || (bnc::host::HOST_EXTERNAL == dstHost->getHostType())))
	{
        return CPortforwardPolicy::getInstance()->applyOut(srcIp, srcPort, protocol);
	}

	return BNC_ERR;
}

INT4 CPortforwardService::GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, UINT4 srcIp, UINT4 dstIp, UINT2 srcPort, UINT2 dstPort, UINT2 proto) 
{
    INT1 srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(srcIp, srcIpStr);
    number2ip(dstIp, dstIpStr);

    //in
    UINT4 inIp = 0;
    UINT2 inPort = 0;
	if ((srcHost.isNull() || (bnc::host::HOST_EXTERNAL == srcHost->getHostType())) && 
        (dstHost.isNotNull() && (bnc::host::HOST_GATEWAY == dstHost->getHostType())) &&
        CPortforwardPolicy::getInstance()->checkIn(ntohl(dstIp), ntohs(dstPort), proto, inIp, inPort))
	{
		LOG_WARN_FMT("traffic in: srcHost[%s]srcIp[%s:%u]dstHost[router]dstIp[%s:%u]protocol[%u]",
            (srcHost.isNull())?"null":"external", srcIpStr, ntohs(srcPort), dstIpStr, ntohs(dstPort), proto);
		return BNC_OK;
	}

    //out
    UINT4 outIp = 0;
    UINT2 outPort = 0;
	if ((srcHost.isNotNull() && (bnc::host::HOST_NORMAL == srcHost->getHostType())) &&
        (dstHost.isNull() || (bnc::host::HOST_EXTERNAL == dstHost->getHostType())) && 
        CPortforwardPolicy::getInstance()->checkOut(ntohl(srcIp), ntohs(srcPort), proto, outIp, outPort))
	{
		LOG_WARN_FMT("traffic out: srcHost[normal]srcIp[%s:%u]dstHost[%s]dstIp[%s:%u]protocol[%u]",
            srcIpStr, ntohs(srcPort), dstHost.isNull()?"null":"external", dstIpStr, ntohs(dstPort), proto);
		return BNC_OK;
	}

	return BNC_ERR;
}



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
*   File Name   : L3Service.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "bnc-inet.h"
#include "openflow-common.h"
#include "CConf.h"
#include "COfMsgUtil.h"
#include "CHost.h"
#include "CHostNormal.h"
#include "CHostMgr.h"
#include "CArpFloodMgr.h"
#include "COpenstackExternalMgr.h"
#include "L3Service.h"

L3Service::L3Service()
{
	
}

L3Service::L3Service(bnc::l3service::service_type type):m_type(type)
{
	
}

L3Service::~L3Service()
{
	
}


INT4  L3Service::ArpRequestHandler(const CSmartPtr<CSwitch>& srcSw, 
                                         CSmartPtr<CHost>& srcHost, 
                                         CSmartPtr<CHost>& dstHost, 
                                         packet_in_info_t* data, 
                                         arp_t* arpPkt)
{
	arp_t arp_pkt;

	// 如果源主机和目的主机都不存在
	if (srcHost.isNull() && dstHost.isNull())
	{
		// 是一个外部网络之间的Arp请求�?不处�?
	}
	
	// 如果目的主机存在
	else if (dstHost.isNotNull())
	{
		// 源主机存�?
		if (srcHost.isNotNull())
		{
			// 创建Arp Reply回应报文并且转发
			LOG_WARN_FMT("Create Arp Reply: send fixedip=0x%x, target fixedip=0x%x, srcIp=0x%x, dstIp=0x%x", 
                srcHost->getfixIp(), dstHost->getfixIp(), srcHost->getIp(), dstHost->getIp());
			COfMsgUtil::createArpReplyPacket(&arp_pkt, srcHost, dstHost);
			COfMsgUtil::forward(srcSw, srcHost->getPortNo(), sizeof(arp_pkt), &arp_pkt);
			LOG_DEBUG_FMT("Create Arp Reply: srcIp= 0x%x, dstIp=0x%x", arp_pkt.sendip, arp_pkt.targetip);
		}
		// 源主机不存在
		else
		{
			// 判断是不是代理主�?
			if (dstHost->isProxyHost())
			{
				LOG_INFO("create Arp Proxy Reply");
				// 创建Arp Reply�?
				COfMsgUtil::createArpReplyPacket(&arp_pkt,dstHost,arpPkt);
				const CSmartPtr<CSwitch>& sw = CHostMgr::getInstance()->getExternalHostSw(dstHost);
				UINT4 port = CHostMgr::getInstance()->getExternalHostPort(dstHost);
				if (port && sw.isNotNull())
				{
					//通过External的交换机指定端口转发
					LOG_INFO("transmit the Floating/NAT reply");
					COfMsgUtil::forward(sw, port, sizeof(arp_pkt), &arp_pkt);
				}
			}
			CHost* TmpsrcHost= new CHostNormal(srcSw, srcSw->getDpid(), data->inport, arpPkt->sendmac, arpPkt->sendip); 
            CSmartPtr<CHost> tempSrcHost(TmpsrcHost);

			COfMsgUtil::createArpReplyPacket(&arp_pkt, tempSrcHost, dstHost);
			COfMsgUtil::forward(srcSw, tempSrcHost->getPortNo(), sizeof(arp_pkt), &arp_pkt);
		}
	}
	// 如果目的主机不存�?
	else if (dstHost.isNull() || (dstHost->getSw().isNull()))
	{
		INT1 str_ip[32] = {0};
		number2ip(arpPkt->targetip, str_ip);
		LOG_INFO_FMT("flood Arp request: who has %s", str_ip);

		LOG_WARN_FMT("flood Arp request: who has %s arpPkt->sendip=0x%x ", str_ip,arpPkt->sendip);
		
		// 将这个包广播
		//COfMsgUtil::ofp13floodInternal( data);  //ycy
		CArpFloodMgr::getInstance()->AddArpRequestNode(arpPkt->targetip,arpPkt->sendip,data);
		
	}
	else
	{
		// do nothing
	}

	return BNC_OK;
}

INT4  L3Service::ArpResponseHandler(const CSmartPtr<CSwitch>& srcSw, 
                                          CSmartPtr<CHost>& srcHost, 
                                          CSmartPtr<CHost>& dstHost, 
                                          packet_in_info_t* data, 
                                          arp_t* arpPkt)
{
	Service_ExternalDetecting::Get_Instance()->CheckARPReply_UpdateExternal(arpPkt->targetip,arpPkt->targetmac,arpPkt->sendip,arpPkt->sendmac,srcSw->getDpid(),data->inport);
	// 如果目的主机存在
	if (dstHost.isNotNull())
	{
		// 将Reply发送给目标主机
		if (dstHost->isProxyHost())
		{
			//如果收到的是external request的reply包更新External的gateway mac
			if (COpenstackExternalMgr::getInstance())
			{
				COpenstackExternalMgr::getInstance()->updateOpenstackExternalAll(arpPkt->sendip,arpPkt->sendmac);
			}
		}
		else
		{
			// 将这个包转发给目标主�?
			if (dstHost->getSw().isNotNull() && (0 != dstHost->getPortNo()))
			{
			    COfMsgUtil::forward(dstHost->getSw(), dstHost->getPortNo(), data);
			}
			else
			{
				// 广播
			}
			CArpFloodMgr::getInstance()->DelArpRequestNode(arpPkt->sendip,arpPkt->targetip);
		}

	}
	// 如果目的主机不存�?
	else
	{
		// 取消广播
	}

	return BNC_OK;
}




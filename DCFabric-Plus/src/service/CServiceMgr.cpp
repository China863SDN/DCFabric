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
*   File Name   : CServiceMgr.cpp           *
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
#include "CControl.h"
#include "CServer.h"
#include "CRecvWorker.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "CHostDefine.h"
#include "CSubnet.h"
#include "CSubnetMgr.h"
#include "L3Service.h"
#include "L3ServiceDefine.h"
#include "InsideCommService.h"
#include "UnknownHostService.h"
#include "CNatIcmpService.h"
#include "CNatCommService.h"
#include "FloatingService.h"
#include "DhcpService.h"
#include "CServiceMgr.h"
#include "COfConnectMgr.h"
#include "CPortforwardService.h"
#include "SwitchManageMode.h"
#include "SwitchManagePresistence.h"
#include "CFirewallService.h"

CServiceMgr* CServiceMgr::m_pInstance = NULL;

CServiceMgr* CServiceMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CServiceMgr();
		if (NULL == m_pInstance)
		{
			exit(-1);
		}
		m_pInstance->init();
	}
	return m_pInstance; 
}

CServiceMgr::CServiceMgr()
{
}
CServiceMgr::~CServiceMgr()
{
}

void CServiceMgr::init()
{
	L3Service* firewallService = new CFirewallService();
	if (NULL != firewallService)
	{
		m_L3ServiceList.push_back(firewallService);
	}
	L3Service* inCommService = new InsideCommService(bnc::l3service::SERVICE_INSIDECOMM);
	if(NULL != inCommService)
	{
		m_L3ServiceMap.insert(std::make_pair(inCommService->GetServiceType(), inCommService));
	}

#if 0
	L3Service* natService= new NatService(bnc::l3service::SERVICE_NATCOMM);
	if(NULL != natService)
	{
		m_L3ServiceMap.insert(std::make_pair(natService->GetServiceType(), natService));
	}
#endif
	L3Service* natIcmpService= new CNatIcmpService(bnc::l3service::SERVICE_NATICMPCOMM);
	if(NULL != natIcmpService)
	{
		m_L3ServiceMap.insert(std::make_pair(natIcmpService->GetServiceType(), natIcmpService));
	}

	L3Service* natcommService= new CNatCommService(bnc::l3service::SERVICE_NATCOMM);
	if(NULL != natcommService)
	{
		m_L3ServiceMap.insert(std::make_pair(natcommService->GetServiceType(), natcommService));
	}
	L3Service* floatingService = new FloatingService(bnc::l3service::SERVICE_FLOATING);
	if(NULL != floatingService)
	{
		m_L3ServiceMap.insert(std::make_pair(floatingService->GetServiceType(), floatingService));
	}
	L3Service* dhcpService  = new DhcpService(bnc::l3service::SERVICE_DHCP);
	if(NULL != dhcpService)
	{
		m_L3ServiceMap.insert(std::make_pair(dhcpService->GetServiceType(), dhcpService));
	}
	
	L3Service* dstHostUnknownService = new UnknownHostService(bnc::l3service::SERVICE_DSTUNKNOWN);
	if(NULL != dstHostUnknownService)
	{
		m_L3ServiceMap.insert(std::make_pair(dstHostUnknownService->GetServiceType(), dstHostUnknownService));
	}
	
	L3Service* portforwardService = new CPortforwardService();
	if (NULL != portforwardService)
	{
		m_L3ServiceMap.insert(std::make_pair(portforwardService->GetServiceType(), portforwardService));
	}
}

INT4 CServiceMgr::AddService(bnc::l3service::service_type servicetype, L3Service* service )
{
	if(NULL != service)
	{
		m_L3ServiceMap.insert(std::make_pair(servicetype , service));
		return BNC_OK;
	}
	return BNC_ERR;
}
INT4 CServiceMgr::DelService(bnc::l3service::service_type servicetype)
{
	CServiceMap::iterator    service = m_L3ServiceMap.find(servicetype);
	if(m_L3ServiceMap.end() == service)
	{
		return BNC_ERR;
	}
	m_L3ServiceMap.erase(servicetype);
	return BNC_OK;
}
INT4 CServiceMgr::ArpProcess(CMsg* msg)
{
	INT4 sockfd = msg->getSockfd();
    CSmartPtr<CSwitch> srcSw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (srcSw.isNull())
    {
        LOG_WARN_FMT("CSwitch not created for sockfd[%d] !", sockfd);
        COfConnectMgr::processSystemFailure(sockfd);
        return BNC_ERR;
    }
	if((0 == srcSw->getTag())||(SW_STATE_STABLE != srcSw->getState()))
	{
		return BNC_ERR;
	}
    INT1* data = msg->getData();
    if (NULL == data)
    {
        LOG_WARN_FMT("[%p] received ARP msg with no data from sockfd[%d] !", 
             this, sockfd);
        return BNC_ERR;
    }

    packet_in_info_t packetIn = {0};
    COfMsgUtil::ofpPacketInConvertter(data, packetIn);

	arp_t* pkt = (arp_t*)packetIn.data;
	printArp(pkt);
	
	if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()&&(0 == srcSw->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return BNC_ERR;
	}
	CSmartPtr<CHost> srcHost = CHostMgr::getInstance()->findHostByIp(pkt->sendip);
	CSmartPtr<CHost> dstHost = CHostMgr::getInstance()->findHostByIp(pkt->targetip);
	CSubnet* srcSubnet = CSubnetMgr::getInstance()->findSubnetByIp(pkt->sendip);
	CSubnet* dstSubnet = CSubnetMgr::getInstance()->findSubnetByIp(pkt->targetip);

    INT1 sendMacStr[24] = {0}, targetMacStr[24] = {0};
    INT1 sendIpStr[24] = {0}, targetIpStr[24] = {0};
	//LOG_WARN_FMT("[%s-%s]==>[%s-%s] ARP opcode[%u],srcHost[%p],dstHost[%p]", 
    //    mac2str(pkt->sendmac, sendMacStr), number2ip(pkt->sendip, sendIpStr), 
    //    mac2str(pkt->targetmac, targetMacStr), number2ip(pkt->targetip, targetIpStr), 
    //    ntohs(pkt->opcode), srcHost.getPtr(), dstHost.getPtr());

	if(CControl::getInstance()->isL3ModeOn()
        && srcHost.isNull() 
        && (dstHost.isNull() || (bnc::host::HOST_EXTERNAL == dstHost->getHostType()))
		&& (NULL == srcSubnet)
		&& (NULL == dstSubnet)) //mininet
	{
		LOG_DEBUG_FMT("srcHost=0x%p srcSubnet=0x%p dstSubnet=0x%p", srcHost.getPtr(), srcSubnet, dstSubnet);
		return BNC_ERR;
	}
		
	//LOG_ERROR_FMT("%s:%d pkt->sendip=0x%x pkt->targetip=0x%x srcSwIp=0x%x dpid=0x%llx",
	//    FN, LN, pkt->sendip, pkt->targetip, srcSw->getSwIp(), srcSw->getDpid());
	srcHost = CHostMgr::getInstance()->addHost(srcSw, packetIn.inport, pkt->sendmac, pkt->sendip, srcSw->getDpid());

	LOG_WARN_FMT("[%s-%s]==>[%s-%s] ARP opcode[%u],srcHost[%p],dstHost[%p]", 
        mac2str(pkt->sendmac, sendMacStr), number2ip(pkt->sendip, sendIpStr), 
        mac2str(pkt->targetmac, targetMacStr), number2ip(pkt->targetip, targetIpStr), 
        ntohs(pkt->opcode), srcHost.getPtr(), dstHost.getPtr());
	
	L3Service* L3_ArpService = m_L3ServiceMap[bnc::l3service::SERVICE_INSIDECOMM];
	if(pkt->opcode == htons(1))
	{
		L3_ArpService->ArpRequestHandler(srcSw, srcHost, dstHost, &packetIn, pkt);
	}
	else
	{
		L3_ArpService->ArpResponseHandler(srcSw, srcHost, dstHost, &packetIn, pkt);
	}


	return BNC_OK;
	
}
INT4 CServiceMgr::IpProcess(CMsg* msg)
{
	INT4 sockfd = msg->getSockfd();
    CSmartPtr<CSwitch> srcSw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (srcSw.isNull())
    {
        LOG_ERROR_FMT("CSwitch not created for sockfd[%d] !", sockfd);
        COfConnectMgr::processSystemFailure(sockfd);
        return BNC_ERR;
    }
	if ((0 == srcSw->getTag()) || (SW_STATE_STABLE != srcSw->getState()))
	{
        LOG_WARN_FMT("CSwitch tag[%u] state[%d] !", srcSw->getTag(), srcSw->getState());
		return BNC_ERR;
	}

	if (SwitchManagePresistence::GetInstance()->isGlobalManageModeOn() && (0 == srcSw->getManageMode()))
	{
		LOG_ERROR("switch manage mode is on!");
		return BNC_ERR;
	}

    INT1* data = msg->getData();
    if (NULL == data)
    {
        LOG_WARN_FMT("[%p] received IP msg with no data from sockfd[%d] !", this, msg->getSockfd());
        return BNC_ERR;
    }

    packet_in_info_t packetIn = {0};
    COfMsgUtil::ofpPacketInConvertter(data, packetIn);
	
	ip_t* pkt = (ip_t*)packetIn.data;
	if (NULL == pkt)
	{
        return BNC_ERR;
	}

	printPacketInfo(pkt);

	CSmartPtr<CHost> dstHost = CHostMgr::getInstance()->findHostByIp(pkt->dest);
	CSmartPtr<CHost> srcHost = CHostMgr::getInstance()->findHostByIp(pkt->src);
	CSubnet* srcSubnet = CSubnetMgr::getInstance()->findSubnetByIp(pkt->src);
	CSubnet* dstSubnet = CSubnetMgr::getInstance()->findSubnetByIp(pkt->dest);

    INT1 srcMacStr[24] = {0}, dstMacStr[24] = {0};
    INT1 srcIpStr[24] = {0}, dstIpStr[24] = {0};
    UINT2 sport = (IP_ICMP!=pkt->proto)?ntohs(*(UINT2*)pkt->data):*(UINT1*)pkt->data;
    UINT2 dport = (IP_ICMP!=pkt->proto)?ntohs(*(UINT2*)(pkt->data+2)):*(UINT1*)(pkt->data+1);
	//LOG_WARN_FMT("[%s-%s:%u]==>[%s-%s:%u] IP proto[%d],srcHost[%p],dstHost[%p]", 
    //    mac2str(pkt->eth_head.src, srcMacStr), number2ip(pkt->src, srcIpStr), sport, 
    //    mac2str(pkt->eth_head.dest, dstMacStr), number2ip(pkt->dest, dstIpStr), dport, 
    //    pkt->proto, srcHost.getPtr(), dstHost.getPtr());

	//LOG_WARN_FMT("srcHost=0x%x, dstHost=0x%x, pkt->src=0x%x, pkt->dest=0x%x", srcHost, dstHost, pkt->src, pkt->dest);
	if (CControl::getInstance()->isL3ModeOn()
        && srcHost.isNull()
		&& (dstHost.isNull() || (bnc::host::HOST_EXTERNAL == dstHost->getHostType()))
		&& (NULL == srcSubnet)
		&& (NULL == dstSubnet) //mininet
		&& (0xffffffff != pkt->dest)
		&& (0x0 != pkt->src))  //DHCP
	{
		return BNC_ERR;
	}
    if (CControl::getInstance()->isL3ModeOn() && 
        (NULL == srcSubnet) && 
        (0x0 != pkt->src) && 
        (0xffffffff == pkt->dest))
    {
        return BNC_ERR;
    }

	srcHost = CHostMgr::getInstance()->addHost(srcSw, packetIn.inport, pkt->eth_head.src, pkt->src, srcSw->getDpid());

	LOG_WARN_FMT("[%s-%s:%u]==>[%s-%s:%u] IP proto[%d],srcHost[%p],dstHost[%p]", 
        mac2str(pkt->eth_head.src, srcMacStr), number2ip(pkt->src, srcIpStr), sport, 
        mac2str(pkt->eth_head.dest, dstMacStr), number2ip(pkt->dest, dstIpStr), dport, 
        pkt->proto, srcHost.getPtr(), dstHost.getPtr());

	UINT2 srcPort = *(UINT2*)pkt->data;
	UINT2 dstPort = *(UINT2*)(pkt->data+2);
    STL_FOR_LOOP(m_L3ServiceList, it)
    {
    	if ((NULL != *it) && (BNC_OK == (*it)->GetServiceType(srcHost, dstHost, pkt->src, pkt->dest, srcPort, dstPort, pkt->proto)))
    	{
    		if ((*it)->IpHandler(srcSw, srcHost, dstHost, pkt, &packetIn) != BNC_OK)
                return BNC_ERR;
    	}
    }

	bnc::l3service::service_type serviceType = getServiceType(pkt);
	LOG_DEBUG_FMT("*** serviceType[%d] ***", serviceType);
	CServiceMap::iterator IpServiceMap = m_L3ServiceMap.find(serviceType);
	if ((m_L3ServiceMap.end() != IpServiceMap) && (NULL != IpServiceMap->second))
	{
		return IpServiceMap->second->IpHandler(srcSw, srcHost, dstHost, pkt, &packetIn);
	}

	return BNC_ERR;
}
void CServiceMgr::printArp(arp_t* pkt)
{
    INT1 str_ip[32] = {0};
    number2ip(pkt->targetip, str_ip);
    LOG_INFO_FMT("Arp target ip: target ip is %s, type is %d", str_ip, ntohs(pkt->opcode));
}

void CServiceMgr::printPacketInfo(ip_t* pkt)
{
    INT1 str_src[24] = {0};
    INT1 str_dst[24] = {0};
    number2ip(pkt->src, str_src);
    number2ip(pkt->dest, str_dst);

    //LOG_INFO_FMT("src ip is %s, dst ip is %s", str_src, str_dst);
	//LOG_ERROR_FMT("src ip is %s, dst ip is %s", str_src, str_dst);
}

bnc::l3service::service_type CServiceMgr::getServiceType(ip_t* ipPktInfo)
{
	UINT2 srcPort=0,dstPort = 0;
	UINT4 srcIp = 0,dstIp = 0;
	UINT2 proto = 0;
	
	srcIp = ipPktInfo->src;
	dstIp = ipPktInfo->dest;
	proto = ipPktInfo->proto;

	if (IPPROTO_ICMP ==  proto) 
	{
		//icmp_t* icmp = (icmp_t*)ipPktInfo->data;
	}
	else if (IPPROTO_TCP == proto)
	{
		tcp_t* tcp = (tcp_t*)ipPktInfo->data;
		dstPort = tcp->dport;
		srcPort = tcp->sport;
	}
	else if (IPPROTO_UDP == proto) 
	{
		udp_t* udp = (udp_t*)ipPktInfo->data;
		dstPort = udp->dport;
		srcPort = udp->sport;
	}
	CSmartPtr<CHost> srcHost = CHostMgr::getInstance()->findHostByIp(srcIp);
	CSmartPtr<CHost> dstHost = CHostMgr::getInstance()->findHostByIp(dstIp);

    INT1 srcIpStr[24] = {0}, dstIpStr[24] = {0};
	LOG_INFO_FMT("srcHost=%p, dstHost=%p, srcIp=%s, dstIp=%s",
        srcHost.getPtr(), dstHost.getPtr(), number2ip(srcIp, srcIpStr), number2ip(dstIp, dstIpStr));

	STL_FOR_LOOP(m_L3ServiceMap,iter)
	{
		if((iter->second)&&(BNC_OK == iter->second->GetServiceType(srcHost, dstHost, srcIp, dstIp, srcPort, dstPort, proto)))
		{
			return iter->second->GetServiceType();
		}
	}
	return bnc::l3service::SERVICE_UNKNOWN;
}


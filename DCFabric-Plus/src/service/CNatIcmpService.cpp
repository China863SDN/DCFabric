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
*   File Name   : CNatIcmpService.h           *
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
#include "CSwitch.h"
#include "CControl.h"
#include "CRouter.h"
#include "CRouterGateMgr.h"
#include "CArpFloodMgr.h"
#include "CNatIcmpEntry.h"
#include "CNatIcmpEntryMgr.h"
#include "CNatIcmpService.h"


INT4 CNatIcmpService::IpHandler(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
		 ip_t* pkt, packet_in_info_t* packetIn)
{

	BOOL from_inside = TRUE;
	Base_External *baseExternalPort(NULL);
	CSmartPtr<CSwitch> gatewaysw(NULL);
	
	if ((dstHost.isNotNull()&&(bnc::host::HOST_GATEWAY == dstHost->getHostType()))
		 &&(srcHost.isNull()||(bnc::host::HOST_EXTERNAL== srcHost->getHostType())))
	{
		from_inside = FALSE;
	}
	


	// get icmp data
	icmp_t* packetin_icmp = (icmp_t*)(pkt->data);

	
	LOG_ERROR_FMT("NAT: icmp: CNatIcmpService::IpHandler from_inside=%d packetin_icmp->id = %d",from_inside, packetin_icmp->id);
	CNatIcmpEntry*  nat_icmp_p = CNatIcmpEntryMgr::getInstance()->findNatIcmpEntryByIdentifier(packetin_icmp->id);
	if (TRUE == from_inside) 
	{
		// judge nat_icmp create
		if (NULL == nat_icmp_p) {
			// if not created, create nat_icmp
			nat_icmp_p = CNatIcmpEntryMgr::getInstance()->AddNatIcmpEntry(packetin_icmp->id,pkt->src, pkt->eth_head.src, srcSw->getDpid(), packetIn->inport);
		}
		if (NULL == nat_icmp_p) {
			LOG_ERROR("NAT: icmp: nat_icmp_p is NULL or srchostNode is NULL!");
			return BNC_ERR;
		}
		CRouter* router = CRouterGateMgr::getInstance()->FindRouterNodeByHostMac(pkt->eth_head.src);
		if(NULL == router)
		{
            INT1 macStr[20] = {0};
            mac2str(pkt->eth_head.src, macStr);
			LOG_ERROR_FMT("NAT: external_port_no is NULL, Can't get router by mac[%s] !!!", macStr);
			return BNC_ERR;
		}
		
		baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(pkt->src);
		if(NULL == baseExternalPort)
		{
			LOG_ERROR("NAT: icmp: baseExternalPort is NULL!");
			return BNC_ERR;
		}
		gatewaysw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
		if(gatewaysw.isNull())
		{
			LOG_ERROR_FMT("NAT: icmp: gatewaysw is NULL!, dpid=0x%llx",baseExternalPort->get_switch_DPID());
			return BNC_ERR;
		}
	
		//UINT4 external_outer_interface_ip= 0xa334a8c0;
		//UINT1 external_outer_interface_mac[6] = {0xfa,0x16,0x3e, 0x32,0xcf, 0xc3};
		UINT4 external_outer_interface_ip= 0;
		UINT1 external_outer_interface_mac[6] = {0};
	
		external_outer_interface_ip = router->getRouterIp();
		CSmartPtr<CHost> routerNode = CHostMgr::getInstance()->findHostByIp(external_outer_interface_ip);
		if(routerNode.isNull())
		{
			LOG_ERROR_FMT("NAT: external_port_no is NULL, Can't get router Node!!! external_outer_interface_ip=0x%x",external_outer_interface_ip);
			return BNC_ERR;
		}
		memcpy(external_outer_interface_mac, routerNode->getMac(), 6);
		pkt->src = external_outer_interface_ip; 
		memcpy(pkt->eth_head.src, external_outer_interface_mac, 6);
		memcpy(pkt->eth_head.dest,baseExternalPort->get_gateway_MAC(), 6);

		/*
		pkt->eth_head.dest[0]=0x4e;
		pkt->eth_head.dest[1]=0xab;
		pkt->eth_head.dest[2]=0x3d;
		pkt->eth_head.dest[3]=0xd7;
		pkt->eth_head.dest[4]=0xa0;
		pkt->eth_head.dest[5]=0xae;
		*/

		// calculate checksum
		pkt->cksum = 0;
		pkt->cksum = calc_ip_checksum((UINT2*)&pkt->hlen, 20);
		COfMsgUtil::forward(gatewaysw,  baseExternalPort->get_switch_port(), packetIn);

		LOG_ERROR_FMT("ip = 0x%x dpid=0x%llx port=%d", gatewaysw->getSwIp(), gatewaysw->getDpid(), baseExternalPort->get_switch_port());
		
	}
	else
	{
		if (NULL == nat_icmp_p) {
			LOG_ERROR("NAT ICMP not created!");
			return BNC_ERR;
		}
		pkt->dest = nat_icmp_p->getIp();
		memcpy(pkt->eth_head.dest, nat_icmp_p->getMac(), MAC_LEN);

		// calculate checksum
		pkt->cksum = 0;
		pkt->cksum = calc_ip_checksum((UINT2*)&pkt->hlen,20);

		CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(nat_icmp_p->getMac());
		if (host.isNull()) {
			LOG_ERROR("NAT: icmp: host is NULL!");
			return BNC_ERR;
		}
		if(host->getSw().isNull()||(0 == host->getPortNo()))
		{
			CSmartPtr<CHost> gateway_p =  CHostMgr::getInstance()->getHostGateway( host);
			if (gateway_p.isNotNull())
			{
				CArpFloodMgr::getInstance()->AddArpRequestNode(gateway_p->getIp(), host->getIp(), const_cast<UINT1 *>(gateway_p->getMac()));
			}
			
			return BNC_ERR;
		}
		LOG_ERROR_FMT("NAT: icmp: ok! nat_icmp_p->getSwDpid()=0x%llx",nat_icmp_p->getSwDpid());
		COfMsgUtil::forward(host->getSw(),  host->getPortNo(), packetIn);
		
	}
	return BNC_OK;
}


INT4 CNatIcmpService::GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, UINT4 srcIp, UINT4 dstIp,  UINT2 srcPort, UINT2 dstPort, UINT2 proto )
{
	if ((((srcHost.isNotNull()&&(bnc::host::HOST_NORMAL == srcHost->getHostType()))
            &&(dstHost.isNull()||(bnc::host::HOST_EXTERNAL == dstHost->getHostType())))
          ||((srcHost.isNull()||(bnc::host::HOST_EXTERNAL == srcHost->getHostType()))
              &&(dstHost.isNotNull()&&(bnc::host::HOST_GATEWAY == dstHost->getHostType()))))
         &&(IPPROTO_ICMP == proto)&&(dstIp != 0xffffffff)&&(srcIp != 0xffffffff))
	{
		return BNC_OK;
	}
	return BNC_ERR;
}


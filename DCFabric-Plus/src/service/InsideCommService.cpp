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
*   File Name   : InsideCommService.cpp           *
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
#include "CControl.h"
#include "CGateway.h"
#include "CRouter.h"
#include "CSubnet.h"
#include "CSubnetMgr.h"
#include "CNetwork.h"
#include "CNetworkMgr.h"
#include "CArpFloodMgr.h"
#include "CRouterGateMgr.h"
#include "InsideCommService.h"

InsideCommService::InsideCommService()
{
	
}
InsideCommService::InsideCommService(bnc::l3service::service_type type):L3Service(type)
{
	
}

InsideCommService::~InsideCommService()
{
	
}

INT4 InsideCommService::IpHandler(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
        		 ip_t* pkt, packet_in_info_t* packetIn)
{
	LOG_INFO("Process Internal Packet");

	
	// 取得网关信息
	CSmartPtr<CHost> srcGateway =srcHost.isNotNull() ? CHostMgr::getInstance()->getHostGateway(srcHost): CHostMgr::getInstance()->getHostGatewayByHostIp(pkt->src);
	CSmartPtr<CHost> dstGateway =dstHost.isNotNull() ? CHostMgr::getInstance()->getHostGateway(dstHost): CHostMgr::getInstance()->getHostGatewayByHostIp(pkt->dest);

	// 如果目的主机交换机不存在, 广播
	if(dstHost.isNull() || (dstHost->getSw().isNull() || (0 == dstHost->getPortNo())))
	{
        LOG_WARN_FMT("%s %d",FN,LN);
		/* 临时代码, 后续添加到专门处理广播的线程中 */
		LOG_INFO("Fail to find dest switch. start flood");
		
		LOG_INFO_FMT("%s %d  srcGateway=0x%p dstGateway=0x%p",FN,LN,srcGateway.getPtr(),dstGateway.getPtr());
		if(dstGateway.isNotNull())
			LOG_INFO_FMT("%s %d pkt->src=0x%x pkt->dest=0x%x dstGateway->getfixIp()=0x%x",FN,LN, pkt->src, pkt->dest, dstGateway->getfixIp());
	    // flood(srcSw, packetIn);
	   // floodInternal(srcSw, packetIn);
	   
	   if(srcGateway == dstGateway)
	   {
	   		CArpFloodMgr::getInstance()->AddArpRequestNode(pkt->dest,pkt->src,packetIn);
	   }
	   else if(dstGateway.isNotNull())
	   {
	  		CArpFloodMgr::getInstance()->AddArpRequestNode(pkt->dest,dstGateway->getfixIp(),packetIn);
	   }
	}
	// 如果目的主机交换机存在
	else
	{
		LOG_WARN_FMT("%s %d",FN,LN);
		// 如果源和目标都是主机
		if (srcHost.isNotNull() && srcHost->isHost() && dstHost.isNotNull() && dstHost->isHost())
		{
			if(srcHost->getTenantId() != dstHost->getTenantId())
			{
				LOG_ERROR_FMT("%s %d srcHost->getTenantId()=%s dstHost->getTenantId()=%s",
                    FN,LN,srcHost->getTenantId().c_str(), dstHost->getTenantId().c_str());
				return BNC_ERR;
			}
			
			//get switch dpid
			UINT8 srcDpid = srcHost->getSw()->getDpid();
			UINT8 dstDpid = dstHost->getSw()->getDpid();
			LOG_INFO_FMT("%s %d srcDpid=0x%llx dstDpid=0x%llx srcGateway=0x%p dstGateway=0x%p",
                FN,LN,srcDpid ,dstDpid ,srcGateway.getPtr(),dstGateway.getPtr());
			// same subnet
			if (srcGateway == dstGateway)
			{
				//same switch
				if (srcDpid != dstDpid)
				{
					CFlowMgr::getInstance()->install_different_switch_flow(srcHost, dstHost);
					CFlowMgr::getInstance()->install_different_switch_flow(dstHost, srcHost);
				}
				else
				{
					CFlowMgr::getInstance()->install_same_switch_flow(srcHost, dstHost);
					CFlowMgr::getInstance()->install_same_switch_flow(dstHost, srcHost);
				}
			}
			// 如果不同网段
			else
			{
				CGateway* srcHostGateway = CRouterGateMgr::getInstance()->FindGatewayNodeBySubnetId(srcHost->getSubnetId());
				CGateway* dstHostGateway = CRouterGateMgr::getInstance()->FindGatewayNodeBySubnetId(dstHost->getSubnetId());
				if((NULL == srcHostGateway)||(NULL == dstHostGateway))
				{
					LOG_ERROR_FMT("%s %d srcGateway=0x%p dstGateway=0x%p",FN,LN, srcHostGateway,dstHostGateway );
					return BNC_ERR;
				}
				CRouter* srcRouter = CRouterGateMgr::getInstance()->FindRouterNodeByDeviceId(srcHostGateway->getDeviceid());
				CRouter* dstRouter = CRouterGateMgr::getInstance()->FindRouterNodeByDeviceId(dstHostGateway->getDeviceid());
				if(srcRouter != dstRouter)
				{
					LOG_ERROR_FMT("%s %d srcRouter=0x%p dstRouter=0x%p",FN,LN, srcRouter,dstRouter );
					return BNC_ERR;
				}
				// 修改包中的目标主机Mac地址(修改前是源主机所在网关Mac)
				//memcpy(pkt->eth_head.dest, dstHost->getMac(), 6);

				// 如果同交换机
				if (srcDpid == dstDpid)
				{
					CFlowMgr::getInstance()->install_same_switch_gateway_flow(dstHost, srcGateway);
					CFlowMgr::getInstance()->install_same_switch_gateway_flow(srcHost, dstGateway);
				}
				// 如果不同交换机
				else
				{
					CFlowMgr::getInstance()->install_different_switch_flow(srcHost, dstHost, srcGateway);
					CFlowMgr::getInstance()->install_different_switch_flow(dstHost, srcHost, dstGateway);
				}
			}

			// install flow to host
			CFlowMgr::getInstance()->install_local_host_flows(srcHost);
			CFlowMgr::getInstance()->install_local_host_flows(dstHost);

			
			// send packet to src
			LOG_INFO("forward packet");
		    //COfMsgUtil::forward(dstHost->getSw(), dstHost->getPortNo(), packetIn);
		    COfMsgUtil::forward(srcHost->getSw(), OFPP13_TABLE, packetIn);
			//LOG_INFO_FMT("%s %d dstHost->getSw()->ip=0x%x dstHost->getPortNo()=%d",FN,LN, dstHost->getSw()->getSwIp(), dstHost->getPortNo());
		}
	}
	return BNC_OK;
}

INT4 InsideCommService::GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, UINT4 srcIp, UINT4 dstIp,  UINT2 srcPort, UINT2 dstPort, UINT2 proto ) 
{
	BOOL bInsideFlag = (CControl::getInstance()->isL3ModeOn())?FALSE:TRUE;
	CSubnet* dstSubnet = CSubnetMgr::getInstance()->findSubnetByIp(dstIp);
	if(NULL != dstSubnet)
	{
		CNetwork* networknode =  CNetworkMgr::getInstance()->findNetworkById(dstSubnet->get_networkid());
		
		//LOG_ERROR_FMT(" dstSubnet->get_networkid()=%s networknode=0x%x ",dstSubnet->get_networkid().c_str(),networknode);
		if((NULL != networknode)&&(FALSE == networknode->get_external()))
		{
			bInsideFlag = TRUE;
		}
	}
	
	//if(srcHost&&dstHost)
	//	LOG_ERROR_FMT("srcHost->getHostType()=0x%x dstHost->getHostType())=0x%x srcIp=0x%x dstIp=0x%x",srcHost->getHostType(), dstHost->getHostType(),srcIp, dstIp);
	if((srcHost.isNotNull()&&((bnc::host::HOST_NORMAL == srcHost->getHostType())||(bnc::host::HOST_ROUTER == srcHost->getHostType())))
		&&(bInsideFlag&&(dstHost.isNull()||(bnc::host::HOST_NORMAL == dstHost->getHostType())||(bnc::host::HOST_ROUTER == dstHost->getHostType()))))
	{
		
		//LOG_ERROR_FMT(" srcIp=0x%x dstIp=0x%x bInsideFlag=%d dstSubnet=0x%p dstHost=0x%p",srcIp, dstIp,bInsideFlag,dstSubnet,dstHost.getPtr());
		//LOG_ERROR_FMT("srcHost->getHostType()=0x%x dstHost->getHostType())=0x%x srcIp=0x%x dstIp=0x%x",srcHost->getHostType(), dstHost->getHostType(),srcIp, dstIp);
		return BNC_OK;
	}
	return BNC_ERR;
}


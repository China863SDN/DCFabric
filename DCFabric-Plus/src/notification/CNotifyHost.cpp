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
*   File Name   : CNotifyHost.cpp           *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "log.h"
#include "bnc-type.h"
#include "CHostMgr.h"
#include "CRouter.h"
#include "CGateway.h"
#include "CRouterGateMgr.h"
#include "CNotificationCenter.h"
#include "CNotifyHost.h"
#include "COpenstackPort.h"
#include "COpenstackMgr.h"
#include "BasePort.h"
#include "BasePortManager.h"


CNotifyHost::CNotifyHost()
{

}

CNotifyHost::~CNotifyHost()
{

}

void CNotifyHost::notifyAddPort(COpenstackPort* port)
{
    /*
     * 收到新增OpenstackPort的通知�?
     * 调用CHostMgr的AddHost接口, 增加主机
     * 此时Host所对应的交换机和端口号都是未知�?
     * 所以对应的传入参数�?
     */
    UINT1 mac[6] = {0};
    port->getMac(mac);
    
    CHostMgr::getInstance()->addHost(port->getDeviceType(),
                                     mac,
                                     port->getFixedFirstIp(),
                                     port->getTenantId(),
                                     port->getNetworkId(),
                                     port->getFixedFristSubnetId(),
                                     port->getId());
	LOG_DEBUG_FMT("port->getFixedFirstIp=0x%x port->getFixedFristSubnetId().c_str=%s",port->getFixedFirstIp(), port->getFixedFristSubnetId().c_str());
	
	CRouterGateMgr::getInstance()->AddNode( port->getDeviceId(),port->getNetworkId(),port->getFixedFristSubnetId(),port->getFixedFirstIp(), UINT1(port->getDeviceType()));

	if((bnc::openstack::device_gateway == port->getDeviceType())||(bnc::openstack::device_interface== port->getDeviceType()))
	{
		
		Base_Port* baseport = new Base_Port( port->getId(), port->getName(),port->getTenantId(),port->getNetworkId(), port->getDeviceId(), port->getDeviceOwner(), port->getCheckStatus(),mac );
		baseport->add_fixed_IP(port->getFixedFirstIp(),port->getFixedFristSubnetId());
		 G_PortMgr.insertNode_ByPort(baseport);
		 LOG_DEBUG_FMT("baseport->getFirstFixedIp()=0x%x baseport->getFirstSubnet().c_str()=%s port->getDeviceOwner()=%s", baseport->getFirstFixedIp(), baseport->getFirstSubnet().c_str(),port->getDeviceOwner().c_str());
	}

}

void CNotifyHost::notifyDelPort(const std::string & port_id)
{
	UINT1 mac[6] = {0};

   	if(NULL != COpenstackMgr::getInstance()->getOpenstack())
   	{
		CSmartPtr<COpenstackPort> port = COpenstackMgr::getInstance()->getOpenstack()->getResource()->findOpenstackPort(port_id);
		if(port.isNotNull())
		{
			port->getMac(mac);
			CHostMgr::getInstance()->delHostByMac(mac);
			CRouterGateMgr::getInstance()->DelNode(port->getDeviceId(),port->getNetworkId(),port->getSubnetId(),port->getFixedFirstIp(), UINT1(port->getDeviceType()));
		}
   	}
	Base_Port* baseport = G_PortMgr.targetPort_ByID(port_id);
	LOG_DEBUG_FMT("port_id=%s baseport=0x%p", port_id.c_str(), baseport);
	if(NULL != baseport)
	{
		CHostMgr::getInstance()->delHostByMac(baseport->get_MAC());
	}
	
}

void CNotifyHost::notifyUpdatePort(COpenstackPort * port)
{
	UINT1 mac[6] = {0};
    port->getMac(mac);
	
	CSmartPtr<CHost> ChostNode = CHostMgr::getInstance()->findHostByMac(mac);
	if(ChostNode.isNotNull())
	{
		ChostNode->setSubnetId(port->getSubnetId());
		ChostNode->setIp(port->getFixedFirstIp());
		
		LOG_DEBUG_FMT("ChostNode->getIp=0x%x",ChostNode->getIp());
	}
	
	CRouterGateMgr::getInstance()->DelNode( port->getDeviceId(),port->getNetworkId(),port->getSubnetId(),port->getFixedFirstIp(), UINT1(port->getDeviceType()));
}

void CNotifyHost::notifyAddPort(Base_Port* port)
{
	/*
	 * 收到新增OpenstackPort的通知�?
	 * 调用CHostMgr的AddHost接口, 增加主机
	 * 此时Host所对应的交换机和端口号都是未知�?
	 * 所以对应的传入参数�?
	 */
	if(NULL == port)
	{
		return;
	}

	string subnetId = port->getFirstSubnet();

	CHostMgr::getInstance()->addHost(port->getDeviceType(),
									 port->get_MAC(),
									 port->getFirstFixedIp(),
									 port->get_tenant_ID(),
									 port->get_network_ID(),
									 subnetId,
									 port->get_ID());
	LOG_DEBUG_FMT("port->getFirstFixedIp=0x%x port->getFirstSubnet()=%s",port->getFirstFixedIp(), port->getFirstSubnet().c_str());
	
	CRouterGateMgr::getInstance()->AddNode( port->get_device_ID(),port->get_network_ID(),subnetId,port->getFirstFixedIp(), UINT1(port->getDeviceType()));

}

void CNotifyHost::notifyUpdatePort(Base_Port* port)
{
	if(NULL == port)
	{
		return;
	}
	string subnetId = port->getFirstSubnet();
	CSmartPtr<CHost> ChostNode = CHostMgr::getInstance()->findHostByMac(port->get_MAC());
	if(ChostNode.isNotNull())
	{
		ChostNode->setSubnetId(subnetId);
		ChostNode->setIp(port->getFirstFixedIp());
		
		LOG_DEBUG_FMT("ChostNode->getIp=0x%x",ChostNode->getIp());
	}
	CRouterGateMgr::getInstance()->DelNode( port->get_device_ID(),port->get_network_ID(),subnetId,port->getFirstFixedIp(), UINT1(port->getDeviceType()));
}




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
*   File Name   : CNotifyExternal.cpp       *
*   Author      : bnc mmzhang               *
*   Create Date : 2016-9-26                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/

#include "CNotifyExternal.h"
#include "COpenstackExternalMgr.h"
#include "COpenstackMgr.h"
#include "comm-util.h"

CNotifyExternal::CNotifyExternal()
{

}

CNotifyExternal::~CNotifyExternal()
{

}

void CNotifyExternal::notifyAddPort(COpenstackPort* port)
{
    if ((port) && (bnc::openstack::device_gateway == port->getDeviceType()))
    {
        UINT4 gateway_ip = 0;

        // 判断subnet不为�?
        if (!port->getFixedFristSubnetId().empty())
        {
            COpenstackSubnet* subnet = COpenstackMgr::getInstance()->getOpenstack()->getResource()->findOpenstackSubnet(port->getFixedFristSubnetId());
            gateway_ip = ip2number(subnet->getGatewayIp().c_str());
        }

        UINT1 mac[6] = {0};
        if (COpenstackExternalMgr * ret = COpenstackExternalMgr::getInstance())
            ret->addOpenstackExternal(port->getFixedFristSubnetId(),gateway_ip,port->getFixedFirstIp(),0,port->getMac(mac),0,0);
        return;
    }
}

void CNotifyExternal::notifyDelPort(const std::string & port_id)
{
    if (!port_id.empty())
    {
        CSmartPtr<COpenstackPort> port = COpenstackMgr::getInstance()->getOpenstack()->getResource()->findOpenstackPort(port_id);
        if (port.isNotNull())
            COpenstackExternalMgr::getInstance()->removeOpenstackExternalByOuterIp(port->getFixedFirstIp());
    }
}

void CNotifyExternal::notifyAddSubnet(COpenstackSubnet* subnet)
{
    if (subnet)
    {
        COpenstackExternalMgr::getInstance()->updateOpenstackExternalBySubnetId(subnet->getId(),ip2number(subnet->getGatewayIp().c_str()));
    }
}

void CNotifyExternal::notifyDelSubent(const std::string & subnet_id)
{
    if (!subnet_id.empty())
    {
            COpenstackExternalMgr::getInstance()->removeOpenstackExternalBySubnetId(subnet_id);
    }
}




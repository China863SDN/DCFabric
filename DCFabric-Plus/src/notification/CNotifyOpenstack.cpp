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
*   File Name   : CNotifyOpenstack.cpp  *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-13                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CNotifyOpenstack.h"

CNotifyOpenstack::CNotifyOpenstack()
{

}

CNotifyOpenstack::~CNotifyOpenstack()
{

}

void CNotifyOpenstack::notifyAddPort(COpenstackPort* port)
{
    m_pNotificationCenter->notifyAddPortHandler(this, port);
}

void CNotifyOpenstack::notifyDelPort(const std::string & port_id)
{
    m_pNotificationCenter->notifyDeletePortHandler(this, port_id);
}

void CNotifyOpenstack::notifyUpdatePort(COpenstackPort* port)
{
    m_pNotificationCenter->notifyUpdatePortHandler(this, port);
}


void CNotifyOpenstack::notifyAddNetwork(COpenstackNetwork* network)
{
    m_pNotificationCenter->notifyAddNetworkHandler(this, network);
}

void CNotifyOpenstack::notifyDelNetwork(const std::string & network_id)
{
    m_pNotificationCenter->notifyDeleteNetworkHandler(this, network_id);
}

void CNotifyOpenstack::notifyUpdateNetwork(COpenstackNetwork* network)
{
    m_pNotificationCenter->notifyUpdateNetworkHandler(this, network);
}

void CNotifyOpenstack::notifyAddSubnet(COpenstackSubnet* subnet)
{
    m_pNotificationCenter->notifyAddSubnetHandler(this, subnet);
}

void CNotifyOpenstack::notifyDelSubent(const std::string & subnet_id)
{
    m_pNotificationCenter->notifyDeleteSubnetHandler(this, subnet_id);
}

void CNotifyOpenstack::notifyUpdateSubnet(COpenstackSubnet* subnet)
{
    m_pNotificationCenter->notifyUpdateSubnetHandler(this, subnet);
}

void CNotifyOpenstack::notifyAddFloatingIp(COpenstackFloatingip * floatingip)
{
	 m_pNotificationCenter->notifyAddFloatingIpHandler(this, floatingip);
}

void CNotifyOpenstack::notifyDelFloatingIp(const std::string & floatingip_id)
{
	m_pNotificationCenter->notifyDelFloatingIpHandler(this, floatingip_id);
}

void CNotifyOpenstack::notifyUpdateFloatingIp(COpenstackFloatingip * floatingip)
{
	 m_pNotificationCenter->notifyUpdateFloatingIpHandler(this, floatingip);
}


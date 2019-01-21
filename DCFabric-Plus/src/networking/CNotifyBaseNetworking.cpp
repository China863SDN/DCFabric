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
*   File Name   : CNotifyBaseNetworking.cpp  *
*   Author      : bnc ycy                  *
*   Create Date : 2018-1-31                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CNotifyBaseNetworking.h"

CNotifyBaseNetworking::CNotifyBaseNetworking()
{

}

CNotifyBaseNetworking::~CNotifyBaseNetworking()
{

}

void CNotifyBaseNetworking::notifyAddPort(Base_Port* port)
{
    m_pNotificationCenter->notifyAddPortHandler(this, port);
}

void CNotifyBaseNetworking::notifyDelPort(const std::string & port_id)
{
    m_pNotificationCenter->notifyDeletePortHandler(this, port_id);
}

void CNotifyBaseNetworking::notifyUpdatePort(Base_Port* port)
{
    m_pNotificationCenter->notifyUpdatePortHandler(this, port);
}


void CNotifyBaseNetworking::notifyAddNetwork(Base_Network* network)
{
    m_pNotificationCenter->notifyAddNetworkHandler(this, network);
}

void CNotifyBaseNetworking::notifyDelNetwork(const std::string & network_id)
{
    m_pNotificationCenter->notifyDeleteNetworkHandler(this, network_id);
}

void CNotifyBaseNetworking::notifyUpdateNetwork(Base_Network* network)
{
    m_pNotificationCenter->notifyUpdateNetworkHandler(this, network);
}

void CNotifyBaseNetworking::notifyAddSubnet(Base_Subnet* subnet)
{
    m_pNotificationCenter->notifyAddSubnetHandler(this, subnet);
}

void CNotifyBaseNetworking::notifyDelSubent(const std::string & subnet_id)
{
    m_pNotificationCenter->notifyDeleteSubnetHandler(this, subnet_id);
}

void CNotifyBaseNetworking::notifyUpdateSubnet(Base_Subnet* subnet)
{
    m_pNotificationCenter->notifyUpdateSubnetHandler(this, subnet);
}

void CNotifyBaseNetworking::notifyAddFloatingIp(Base_Floating* floatingip)
{
	 m_pNotificationCenter->notifyAddFloatingIpHandler(this, floatingip);
}

void CNotifyBaseNetworking::notifyDelFloatingIp(const std::string & floatingip_id)
{
	m_pNotificationCenter->notifyDelFloatingIpHandler(this, floatingip_id);
}

void CNotifyBaseNetworking::notifyUpdateFloatingIp(Base_Floating* floatingip)
{
	 m_pNotificationCenter->notifyUpdateFloatingIpHandler(this, floatingip);
}



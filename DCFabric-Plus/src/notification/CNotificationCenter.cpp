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
*   File Name   : CNotificationCenter.cpp   *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "comm-util.h"
#include "CNotificationCenter.h"
#include "CNotifyHandler.h"


CNotificationCenter::CNotificationCenter()
{

}

CNotificationCenter::~CNotificationCenter()
{

}

void CNotificationCenter::notifyAddPortHandler(CNotifyHandler* handler, COpenstackPort* port)
{
    if (bnc::notify::OPENSTACK == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::OPENSTACK == iter->first->getType())&&(bnc::notify::HOST == iter->second->getType()))
            {
                iter->second->notifyAddPort(port);
            }
        }
    }
}

void CNotificationCenter::notifyAddPortHandler(CNotifyHandler* handler, Base_Port* port)
{
    if (bnc::notify::BASENETWORKING == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::BASENETWORKING == iter->first->getType())&&(bnc::notify::HOST == iter->second->getType()))
            {
                iter->second->notifyAddPort(port);
            }
        }
    }
}

void CNotificationCenter::notifyDeletePortHandler(CNotifyHandler* handler, const string & port_id)
{
    if ((bnc::notify::OPENSTACK == handler->getType())||(bnc::notify::BASENETWORKING == handler->getType()))
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((handler->getType() == iter->first->getType())&&(bnc::notify::HOST == iter->second->getType()))
            {
                iter->second->notifyDelPort(port_id);
            }
        }
    }
}

void CNotificationCenter::notifyUpdatePortHandler(CNotifyHandler* handler, COpenstackPort* port)
{
	if (bnc::notify::OPENSTACK == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::OPENSTACK == iter->first->getType())&&(bnc::notify::HOST == iter->second->getType()))
            {
                iter->second->notifyUpdatePort(port);
            }
        }
    }
}

void CNotificationCenter::notifyUpdatePortHandler(CNotifyHandler* handler, Base_Port* port)
{
	if (bnc::notify::BASENETWORKING == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::BASENETWORKING == iter->first->getType())&&(bnc::notify::HOST == iter->second->getType()))
            {
                iter->second->notifyUpdatePort(port);
            }
        }
    }
}

void CNotificationCenter::notifyAddNetworkHandler(CNotifyHandler * handler,COpenstackNetwork * network)
{
	if (bnc::notify::OPENSTACK == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::OPENSTACK == iter->first->getType())&&(bnc::notify::NETWORK == iter->second->getType()))
            {
                iter->second->notifyAddNetwork(network);
            }
        }
    }
}

void CNotificationCenter::notifyAddNetworkHandler(CNotifyHandler * handler,Base_Network* network)
{
	if (bnc::notify::BASENETWORKING == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::BASENETWORKING == iter->first->getType())&&(bnc::notify::NETWORK == iter->second->getType()))
            {
                iter->second->notifyAddNetwork(network);
            }
        }
    }
}

void CNotificationCenter::notifyDeleteNetworkHandler(CNotifyHandler * handler,const string & network_id)
{
	if ((bnc::notify::OPENSTACK == handler->getType())||(bnc::notify::BASENETWORKING == handler->getType()))
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((handler->getType() == iter->first->getType())&&(bnc::notify::NETWORK == iter->second->getType()))
            {
                iter->second->notifyDelNetwork(network_id);
            }
        }
    }
}

void CNotificationCenter::notifyUpdateNetworkHandler(CNotifyHandler * handler,COpenstackNetwork * network)
{
	if (bnc::notify::OPENSTACK == handler->getType())
	{
		STL_FOR_LOOP(m_pNotifyList, iter)
		{
			if((bnc::notify::OPENSTACK == iter->first->getType())&& (bnc::notify::NETWORK == iter->second->getType()))
			{
				iter->second->notifyUpdateNetwork(network);
			}
		}
	}
}

void CNotificationCenter::notifyUpdateNetworkHandler(CNotifyHandler * handler,Base_Network* network)
{
	if (bnc::notify::BASENETWORKING == handler->getType())
	{
		STL_FOR_LOOP(m_pNotifyList, iter)
		{
			if((bnc::notify::BASENETWORKING == iter->first->getType())&& (bnc::notify::NETWORK == iter->second->getType()))
			{
				iter->second->notifyUpdateNetwork(network);
			}
		}
	}
}

void CNotificationCenter::notifyAddSubnetHandler(CNotifyHandler* handler, COpenstackSubnet* subnet)
{
    if (bnc::notify::OPENSTACK == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::OPENSTACK == iter->first->getType())&&(bnc::notify::SUBNET == iter->second->getType()))
            {
                iter->second->notifyAddSubnet(subnet);
            }
        }
    }
}

void CNotificationCenter::notifyAddSubnetHandler(CNotifyHandler* handler, Base_Subnet* subnet)
{
    if (bnc::notify::BASENETWORKING == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if((bnc::notify::BASENETWORKING == iter->first->getType())&& (bnc::notify::SUBNET == iter->second->getType()))
            {
                iter->second->notifyAddSubnet(subnet);
            }
        }
    }
}

void CNotificationCenter::notifyDeleteSubnetHandler(CNotifyHandler* handler, const string & subnet_id)
{
    if ((bnc::notify::OPENSTACK == handler->getType())||(bnc::notify::BASENETWORKING == handler->getType()))
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((handler->getType() == iter->first->getType())&&(bnc::notify::SUBNET == iter->second->getType()))
            {
                iter->second->notifyDelSubnet(subnet_id);
            }
        }
    }
}
void CNotificationCenter::notifyUpdateSubnetHandler(CNotifyHandler* handler, COpenstackSubnet* subnet)
{
    if (bnc::notify::OPENSTACK == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::OPENSTACK == iter->first->getType())&&(bnc::notify::SUBNET == iter->second->getType()))
            {
                iter->second->notifyUpdateSubnet(subnet);
            }
        }
    }
}

void CNotificationCenter::notifyUpdateSubnetHandler(CNotifyHandler* handler, Base_Subnet* subnet)
{
    if (bnc::notify::BASENETWORKING == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::BASENETWORKING == iter->first->getType()) && (bnc::notify::SUBNET == iter->second->getType()))
            {
                iter->second->notifyUpdateSubnet(subnet);
            }
        }
    }
}


void CNotificationCenter::notifyAddFloatingIpHandler(CNotifyHandler * handler,COpenstackFloatingip * floatingip)
{
	if (bnc::notify::OPENSTACK == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if((bnc::notify::OPENSTACK == iter->first->getType())&& (bnc::notify::FLOATINGIP == iter->second->getType()))
            {
                iter->second->notifyAddFloatingIp(floatingip);
            }
        }
    }
}

void CNotificationCenter::notifyAddFloatingIpHandler(CNotifyHandler * handler,Base_Floating* floatingip)
{
	if (bnc::notify::BASENETWORKING == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if((bnc::notify::BASENETWORKING == iter->first->getType())&& (bnc::notify::FLOATINGIP == iter->second->getType()))
            {
                iter->second->notifyAddFloatingIp(floatingip);
            }
        }
    }
}

void CNotificationCenter::notifyDelFloatingIpHandler(CNotifyHandler * handler,const string & floatingip_id)
{
	if ((bnc::notify::OPENSTACK == handler->getType())||(bnc::notify::BASENETWORKING == handler->getType()))
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((handler->getType() == iter->first->getType())&&(bnc::notify::FLOATINGIP == iter->second->getType()))
            {
                iter->second->notifyDelFloatingIp(floatingip_id);
            }
        }
    }
}

void CNotificationCenter::notifyUpdateFloatingIpHandler(CNotifyHandler * handler,COpenstackFloatingip * floatingip)
{
	if (bnc::notify::OPENSTACK == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if((bnc::notify::OPENSTACK == iter->first->getType())&& (bnc::notify::FLOATINGIP == iter->second->getType()))
            {
                iter->second->notifyUpdateFloatingIp(floatingip);
            }
        }
    }
}

void CNotificationCenter::notifyUpdateFloatingIpHandler(CNotifyHandler * handler,Base_Floating* floatingip)
{
	if (bnc::notify::BASENETWORKING == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if ((bnc::notify::BASENETWORKING == iter->first->getType())&&(bnc::notify::FLOATINGIP == iter->second->getType()))
            {
                iter->second->notifyUpdateFloatingIp(floatingip);
            }
        }
    }
}


void CNotificationCenter::notifyAddPathHandler(CNotifyHandler* handler,
                                                const CSmartPtr<CSwitch> & src_sw,
                                                const CSmartPtr<CSwitch> & dst_sw,
                                                UINT4 port_no)
{
    if (bnc::notify::PATH == handler->getType())
    {
        STL_FOR_LOOP(m_pNotifyList, iter)
        {
            if (bnc::notify::PATH == iter->first->getType())
            {
                iter->second->notifyAddPath(src_sw, dst_sw, port_no);
            }
        }
    }
}

void CNotificationCenter::addNotifyHandler(CNotifyHandler* res, CNotifyHandler* handler)
{
    m_pNotifyList.push_back(std::make_pair(res, handler));
}

void CNotificationCenter::delNotifyHandler(CNotifyHandler* res, CNotifyHandler* handler)
{
    m_pNotifyList.remove(std::make_pair(res, handler));
}



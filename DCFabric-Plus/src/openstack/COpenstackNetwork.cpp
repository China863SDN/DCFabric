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
*   File Name   : COpenstackNetwork.cpp            *
*   Author      : bnc xflu                      *
*   Create Date : 2016-9-7                      *
*   Version     : 1.0                           *
*   Function    : .                             *
*                                                                             *
******************************************************************************/
#include "COpenstackNetwork.h"


COpenstackNetwork::COpenstackNetwork():
	m_bRouterExternal(FALSE),
	m_bAdminStateUp(FALSE),
	m_uiMtu(0),
	m_bShared(FALSE),
	m_uiProviderSegmentationId(0),
	m_bPortSecurityEnabled(0)
{

}
COpenstackNetwork:: ~COpenstackNetwork()
{

}

BOOL COpenstackNetwork::Compare (COpenstackNetwork* network)
{
	if(NULL == network)
	{
		return FALSE;
	}
	if((0 != network->m_strId.compare(this->m_strId))||(0 != network->m_strTenantId.compare(this->m_strTenantId))||
		(network->m_bRouterExternal != this->m_bRouterExternal)||(network->m_bShared != this->m_bShared))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL COpenstackNetwork::SetObjectValue(COpenstackNetwork* network)
{
	if(NULL == network)
	{
		return FALSE;
	}
	this->m_strStatus = network->m_strStatus;
    this->m_bRouterExternal = network->m_bRouterExternal;
    this->m_strName = network->m_strName;
    this->m_bAdminStateUp = network->m_bAdminStateUp;
    this->m_strTenantId = network->m_strTenantId;
	this->m_uiMtu = network->m_uiMtu;
    this->m_bRouterExternal = network->m_bRouterExternal;
    this->m_bShared = network->m_bShared;
    this->m_strId = network->m_strId;
    this->m_strQosPolicyId = network->m_strQosPolicyId;

	
	this->m_strProviderNetworkType = network->m_strProviderNetworkType;
	this->m_strProviderPhysicalNetwork = network->m_strProviderPhysicalNetwork;
	this->m_uiProviderSegmentationId = network->m_uiProviderSegmentationId;
	this->m_bPortSecurityEnabled = network->m_bPortSecurityEnabled;
    //list<string> m_liSubnets;
	return TRUE;
}


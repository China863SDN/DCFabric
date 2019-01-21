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
*   File Name   : COpenstackSubnet.cpp      *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "COpenstackSubnet.h"

COpenstackSubnet::COpenstackSubnet():m_bEnableDhcp(FALSE),m_uiIpVersion(0)
{

}
COpenstackSubnet::~COpenstackSubnet()
{

}


BOOL COpenstackSubnet::Compare(COpenstackSubnet* subnet)
{
	if(NULL == subnet)
	{
		return FALSE;
	}
	if((0 != subnet->m_strId.compare(this->m_strId))||(0 != subnet->m_strNetworkId.compare(this->m_strNetworkId))||
		(0 != subnet->m_strTenantId.compare(this->m_strTenantId))||(0 != subnet->m_strGatewayIp.compare(this->m_strGatewayIp))||
		(0 != subnet->m_strStart.compare(this->m_strStart))||(0 != subnet->m_strEnd.compare(this->m_strEnd)))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL COpenstackSubnet::SetObjectValue(COpenstackSubnet* subnet)
{
	if(NULL == subnet)
	{
		return FALSE;
	}

	this->m_strName = subnet->m_strName;
    this->m_bEnableDhcp = subnet->m_bEnableDhcp;
    this->m_strNetworkId = subnet->m_strNetworkId;
    this->m_strTenantId = subnet->m_strTenantId;
    //list<string> m_liDnsNameservers;
    //map<string, string> m_mapAlloctionPools;
    this->m_strStart = subnet->m_strStart;
    this->m_strEnd = subnet->m_strEnd;
    //list<string> m_liHostRoutes;
    this->m_uiIpVersion = subnet->m_uiIpVersion;
    this->m_strGatewayIp = subnet->m_strGatewayIp;
    this->m_strCidr = subnet->m_strCidr;
    this->m_strId = subnet->m_strId;

	return TRUE;
}


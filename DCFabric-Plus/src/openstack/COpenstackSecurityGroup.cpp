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
*   File Name   : COpenstackSecurityGroup.cpp   *
*   Author      : bnc xflu                      *
*   Create Date : 2016-9-7                      *
*   Version     : 1.0                           *
*   Function    : .                             *
*                                                                             *
******************************************************************************/
#include "COpenstackSecurityGroup.h"
#include "bnc-param.h"

COpenstackSecurityGroup::COpenstackSecurityGroup():
    m_uiPortRangeMax(0),
    m_uiPortRangeMin(0),
    m_priority(0),
    m_enabled(TRUE),
    m_checkStatus(CHECK_CREATE)
{

}

COpenstackSecurityGroup::~COpenstackSecurityGroup()
{

}

BOOL COpenstackSecurityGroup::Compare(const COpenstackSecurityGroup& securityGroup)
{
	if ((0 == securityGroup.m_strId.compare(m_strId)) &&
        //(0 == securityGroup.m_strRemoteGroupId.compare(m_strRemoteGroupId)) &&
		(0 == securityGroup.m_strDirection.compare(m_strDirection)) &&
		(0 == securityGroup.m_strProtocol.compare(m_strProtocol)) &&
		(0 == securityGroup.m_strEthertype.compare(m_strEthertype)) &&
		(securityGroup.m_uiPortRangeMax == m_uiPortRangeMax) &&
		(securityGroup.m_uiPortRangeMin == m_uiPortRangeMin) &&
		(0 == securityGroup.m_strSecurityGroupId.compare(m_strSecurityGroupId)) &&
		//(0 == securityGroup.m_strTenantId.compare(m_strTenantId)) &&
		(0 == securityGroup.m_strRemoteIpPrefix.compare(m_strRemoteIpPrefix)) &&
		(securityGroup.m_priority == m_priority) &&
		(securityGroup.m_enabled == m_enabled) &&
		(0 == securityGroup.m_strAction.compare(m_strAction)))
		return TRUE;

	return FALSE;
}


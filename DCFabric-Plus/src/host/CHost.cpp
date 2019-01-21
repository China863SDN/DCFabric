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
*   File Name   : CHost.cpp			*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-21         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#include "CHost.h"
#include <algorithm>
#include "comm-util.h"

CHost::CHost():
    m_enType(bnc::host::HOST_NORMAL),
    m_ptrSw(NULL),
    m_iDpid(0),
    m_iPortNo(0)
{
    memset(m_oMac, 0, 6);
}

CHost::CHost(CSmartPtr<CSwitch> sw, UINT8 dpid, UINT4 portNo, const UINT1* mac, UINT4 ip):
    m_enType(bnc::host::HOST_NORMAL),
    m_ptrSw(sw),
    m_iDpid(dpid),
    m_iPortNo(portNo)
{
	memcpy(m_oMac, mac, 6);
	m_oIpList.push_back(ip);
}

CHost::CHost(CSmartPtr<CSwitch> sw, UINT8 dpid, UINT4 portNo, const UINT1* mac, UINT4 ip,
               bnc::host::host_type type, const std::string& subnetid, const std::string& tenantid):
    m_enType(type),
    m_ptrSw(sw),
    m_iDpid(dpid),
    m_iPortNo(portNo),
    m_strSubnetId(subnetid),
    m_strTenantId(tenantid)
{
	memcpy(m_oMac, mac, 6);
	m_oIpList.push_back(ip);
}

CHost::CHost(const UINT1* mac, UINT4 ip, bnc::host::host_type type, const std::string& subnetid, const std::string& tenantid):
    m_enType(type),
    m_ptrSw(NULL),
    m_iDpid(0),
    m_iPortNo(0),
    m_strSubnetId(subnetid),
    m_strTenantId(tenantid)
{
	memcpy(m_oMac, mac, 6);
	m_oIpList.push_back(ip);
}

CHost::~CHost()
{
}

BOOL CHost::isIpExist(INT4 ip) const
{
	if (m_oIpList.end() != find(m_oIpList.begin(), m_oIpList.end(), ip))
	{
		return TRUE;
	}

	return FALSE;
}


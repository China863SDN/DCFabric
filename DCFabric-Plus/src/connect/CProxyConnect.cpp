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
*   File Name   : CProxyConnect.cpp         *
*   Author      : bnc xflu                  *
*   Create Date : 2016-10-17                *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CProxyConnect.h"
#include "log.h"

BOOL CProxyConnect::isProxyConnect(UINT4 extIp, UINT2 proto, UINT2 extPortNo)
{
    if ((extIp == m_extIp) && (proto == m_proto) && (extPortNo == m_extPortNo))
    {
        return TRUE;
    }

    return FALSE;
}


void CProxyConnect::printAll()
{
    LOG_INFO_FMT("extIP: %d, proxyIp: %d, fixedIp: %d, "
                 "extPort: %d, proxyPort: %d, fixedPort: %d, proto: %d",
                  m_extIp, m_proxyIp, m_fixedIp, m_extPortNo, m_proxyPortNo, m_fixedPortNo, m_proto);
}

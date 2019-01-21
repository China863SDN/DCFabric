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
*   File Name   : CProxyConnectHost.cpp     *
*   Author      : bnc xflu                  *
*   Create Date : 2016-10-17                *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CProxyConnectHost.h"
#include "comm-util.h"
#include "log.h"

CProxyConnectHost::CProxyConnectHost()
{

}

CProxyConnectHost::~CProxyConnectHost()
{

}

CProxyConnect* CProxyConnectHost::addProxyConnect(UINT4 extIp, UINT4 proxyIp, UINT4 fixedIp,
                                                  const UINT1* extMac, const UINT1* proxyMac, const UINT1* fixedMac,
                                                  UINT2 extPortNo, UINT2 fixedPortNo, UINT1 proto)
{
    CProxyConnect* proxyConnect = findProxyConnect(extIp, proto, extPortNo);

    if (NULL == proxyConnect)
    {
        // 取得当前最小的Id
        UINT2 id = 100;

        proxyConnect = new CProxyConnect(extIp, proxyIp, fixedIp, extMac, proxyMac, fixedMac, extPortNo, id, fixedPortNo, proto);

        if (NULL == proxyConnect)
        {
            LOG_INFO("Fail to add proxy connect.");
        }
        else
        {
            m_mutex.lock();

            m_pIpList.push_back(proxyConnect);

            m_mutex.unlock();
        }
    }

    return proxyConnect;
}


CProxyConnect* CProxyConnectHost::findProxyConnect(UINT4 extIp, UINT1 proto, UINT2 proxyPortNo)
{
    CProxyConnect* proxyConnect = NULL;

    if (0 == extIp)
    {
        return NULL;
    }

    m_mutex.lock();

    STL_FOR_LOOP(m_pIpList, iter)
    {
        if ((*iter)->isProxyConnect(extIp, proto, proxyPortNo))
        {
            proxyConnect = *iter;
        }
    }

    m_mutex.unlock();

    return proxyConnect;
}

void CProxyConnectHost::printAll()
{
    STL_FOR_LOOP(m_pIpList, iter)
    {
        (*iter)->printAll();
    }
}



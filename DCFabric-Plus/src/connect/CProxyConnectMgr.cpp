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
*   File Name   : CProxyConnectMgr.cpp      *
*   Author      : bnc xflu                  *
*   Create Date : 2016-10-17                *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CProxyConnectMgr.h"
#include "comm-util.h"
#include "log.h"

CProxyConnectMgr* CProxyConnectMgr::m_pInstance = 0;

CProxyConnectMgr::CProxyConnectMgr()
{

}

CProxyConnectMgr::~CProxyConnectMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}


CProxyConnectMgr* CProxyConnectMgr::getInstance()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new CProxyConnectMgr();
        m_pInstance->init();
    }

    return m_pInstance;
}

void CProxyConnectMgr::init()
{

}

void CProxyConnectMgr::printAll()
{
    STL_FOR_LOOP(m_pProxyList, iter)
    {
        LOG_INFO_FMT("ip is %d", iter->first);
        iter->second->printAll();
    }
}



CProxyConnect* CProxyConnectMgr::addProxyConnect(UINT4 extIp, UINT4 proxyIp, UINT4 fixedIp,
                                                 const UINT1* extMac, const UINT1* proxyMac, const UINT1* fixedMac,
                                                 UINT2 extPortNo, UINT2 fixedPortNo, UINT1 proto)
{
    // 定义空的ProxyConnect
    CProxyConnect* proxyConnect;

    // 在Host List中查找对应的proxyIp是否存在
    CProxyConnectHost* proxyHost = findProxyConnectHost(proxyIp);

    // 如果存在
    if (NULL != proxyHost)
    {
        // 调用CProxyConnectHost提供的查找函数判断是否存在
        if (proxyHost->findProxyConnect(extIp, proto, extPortNo))
        {
            // 如果存在, 打印Log
            LOG_INFO_FMT("ProxyConnect extIp: %d, proto: %d, portNo: %d exist.", extIp, proto, extPortNo);
        }
        else
        {
            // 如果不存在, 调用ProxyConnectHost提供的Add函数添加新的连接
            proxyConnect = proxyHost->addProxyConnect(extIp, proxyIp, fixedIp, extMac, proxyMac, fixedMac, extPortNo, fixedPortNo, proto);
        }
    }
    // 如果不存在
    else
    {
        // 调用ProxyConnectHost提供的Add函数添加新的连接
        // 将新建的ConnectHost添加到list
        proxyHost = addProxyConnectHost(proxyIp);

        if (proxyHost)
        {
            proxyConnect = proxyHost->addProxyConnect(extIp, proxyIp, fixedIp, extMac, proxyMac, fixedMac, extPortNo, fixedPortNo, proto);
        }
    }

    return proxyConnect;
}

CProxyConnectHost* CProxyConnectMgr::findProxyConnectHost(UINT4 proxyIp)
{
    if (0 == proxyIp)
    {
        return NULL;
    }

    m_mutex.lock();

    STL_FOR_LOOP(m_pProxyList, iter)
    {
        if (iter->first == proxyIp)
        {
            m_mutex.unlock();
            return iter->second;
        }
    }

    m_mutex.unlock();

    return NULL;
}


CProxyConnectHost* CProxyConnectMgr::addProxyConnectHost(UINT4 proxyIp)
{
    CProxyConnectHost* proxyHost = NULL;

    if (0 == proxyIp)
    {
        LOG_INFO("Fail to add Proxy Connect Host. ip is empty.");
        return NULL;
    }

    if (findProxyConnectHost(proxyIp))
    {
        LOG_INFO("Fail to add ProxyConnect Host. ip exist.");
        return NULL;
    }

    m_mutex.lock();

    proxyHost = new CProxyConnectHost();

    if (NULL != proxyHost)
    {
        m_pProxyList.insert(std::make_pair(proxyIp, proxyHost));
    }
    m_mutex.unlock();

    return proxyHost;
}

CProxyConnect* CProxyConnectMgr::findProxyConnect(UINT4 extIp, UINT4 proxyIp, UINT2 proxyPortNo, UINT2 proto)
{
    CProxyConnect* proxyConnect = NULL;
    CProxyConnectHost* proxyHost = findProxyConnectHost(proxyIp);

    // 如果存在
    if (NULL != proxyHost)
    {
        proxyConnect = proxyHost->findProxyConnect(extIp, proto, proxyPortNo);
    }

    return proxyConnect;
}


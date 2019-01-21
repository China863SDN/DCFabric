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
*   File Name   : CProxyConnectHost.h       *
*   Author      : bnc xflu                  *
*   Create Date : 2016-10-17                *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CPROXYCONNECT_HOST_H
#define _CPROXYCONNECT_HOST_H
#include "CProxyConnect.h"
#include <list>
#include "CMutex.h"

/*
 * 代理主机的类
 * 存储了代理主机的Ip和其相关的所有连接
 */
class CProxyConnectHost
{
public:
    CProxyConnectHost();
    ~CProxyConnectHost();

    /*
     * 增加新的ProxyConnect
     */
    CProxyConnect* addProxyConnect(UINT4 extIp, UINT4 proxyIp, UINT4 fixedIp,
                                   const UINT1* extMac, const UINT1* proxyMac, const UINT1* fixedMac,
                                   UINT2 extPortNo, UINT2 fixedPortNo, UINT1 proto);

    /*
     * 查找ProxyConnect
     */
    CProxyConnect* findProxyConnect(UINT4 extIp, UINT1 proto, UINT2 proxyPortNo);

    /*
     * 测试打印所有
     */
    void printAll();

private:

    UINT4 proxyIp;                              ///< 代理主机Ip
//    CMutex m_tcpmutex;                          ///< 互斥锁,用于tcp加锁
//    CMutex m_udpmutex;                          ///< 互斥锁,用于udp加锁
//    CMutex m_icmpmutex;                         ///< 互斥锁,用于icmp加锁
//    std::list<CProxyConnect*> m_pTcpList;       ///< 用于存储TCP连接
//    std::list<CProxyConnect*> m_pUdpList;       ///< 用于存储UDP连接
//    std::list<CProxyConnect*> m_pIcmpList;      ///< 用于存储ICMP连接
    CMutex m_mutex;
    std::list<CProxyConnect*> m_pIpList;
};



#endif

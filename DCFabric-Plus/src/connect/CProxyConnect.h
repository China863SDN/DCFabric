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
*   File Name   : CProxyConnect.h           *
*   Author      : bnc xflu                  *
*   Create Date : 2016-10-17                *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CPROXY_CONNECT_H
#define _CPROXY_CONNECT_H

#include "bnc-type.h"

/*
 * 每一个和代理主机之间的连接
 */
class CProxyConnect
{
public:
    CProxyConnect();
    ~CProxyConnect();

    /*
     * 带所有参数的构造函数
     */
    CProxyConnect(UINT4 extIp, UINT4 proxyIp, UINT4 fixedIp,
                  const UINT1* extMac, const UINT1* proxyMac, const UINT1* fixedMac,
                  UINT2 extPortNo, UINT2 proxyPortNo, UINT2 fixedPortNo, UINT2 proto)
                : m_extIp(extIp), m_proxyIp(proxyIp), m_fixedIp(fixedIp),
                  m_extPortNo(extPortNo), m_proxyPortNo(proxyPortNo), m_fixedPortNo(fixedPortNo),
                  m_proto(proto)
    {
        if (extMac)
        {
            memcpy(m_extMac, extMac, 6);
            memcpy(m_proxyMac, proxyMac, 6);
            memcpy(m_fixedMac, fixedMac, 6);
        }
    }

    /*
     * 只有IP信息
     */
    CProxyConnect(UINT4 extIp, UINT4 proxyIp, UINT4 fixedIp, const UINT1* proxyMac, const UINT1* fixedMac)
                : m_extIp(extIp), m_proxyIp(proxyIp), m_fixedIp(fixedIp),
                  m_extPortNo(0), m_proxyPortNo(0), m_fixedPortNo(0),
                  m_proto(0)
    {
    }


    /*
     * 判断是不是这个主机
     */
    BOOL isProxyConnect(UINT4 extIp, UINT2 proto, UINT2 extPortNo);

    /*
     * 测试打印所有
     */
    void printAll();

    UINT4 getExtIp() { return m_extIp; }

    const UINT1* getExtMac() const { return m_extMac; }

    UINT4 getProxyIp() { return m_proxyIp; }

    const UINT1* getProxyMac() const { return m_proxyMac; }

    UINT4 getFixedIp() { return m_fixedIp; }

    const UINT1* getFixedMac() const { return m_fixedMac; }

    UINT2 getExtPortNo() { return m_extPortNo; }

    UINT2 getProxyPortNo() { return m_proxyPortNo; }

    UINT2 getFixedPortNo() { return m_fixedPortNo; }

    UINT1 getProto() { return m_proto; }

private:
    UINT4 m_extIp;
    UINT1 m_extMac[6];
    UINT4 m_proxyIp;
    UINT1 m_proxyMac[6];
    UINT4 m_fixedIp;
    UINT1 m_fixedMac[6];

    UINT2 m_extPortNo;
    UINT2 m_proxyPortNo;
    UINT2 m_fixedPortNo;

    UINT1 m_proto;
};

#endif

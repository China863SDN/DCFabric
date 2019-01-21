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
*   File Name   : CProxyConnectMgr.h        *
*   Author      : bnc xflu                  *
*   Create Date : 2016-10-17                *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CPROXY_CONNECT_MGR_H
#define _CPROXY_CONNECT_MGR_H

#include <map>
#include "bnc-type.h"
#include "CProxyConnectHost.h"
#include "CMutex.h"


/*
 * 用来管理所有的代理连接
 * 代理连接指的是外部主机访问一个虚拟的IP进行通信,
 * 实际上通信的是内部网络主机中的一个主机
 * 目前所使用的包括:
 * Openstack的FloatingIP (一个FloatingIp对应一台真实主机)
 * Openstack的NAT (一个虚拟Ip+协议端口对应一台真实主机)
 *
 * 现在的结构是:
 * 每一个Ip对应了一个ConnectHost,
 * 每个ConnectHost下记录了所有的Connect
 */
class CProxyConnectMgr
{
public:
    ~CProxyConnectMgr();

    /*
     * 获取实例
     */
    static CProxyConnectMgr* getInstance();

    /*
     * 增加连接
     */
    CProxyConnect* addProxyConnect(UINT4 extIp, UINT4 proxyIp, UINT4 fixedIp,
                                   const UINT1* extMac, const UINT1* proxyMac, const UINT1* fixedMac,
                                   UINT2 extPortNo, UINT2 fixedPortNo, UINT1 proto);

    /*
     * 删除连接
     */


    /*
     * 查询连接
     */
    CProxyConnect* findProxyConnect(UINT4 extIp, UINT4 proxyIp, UINT2 proxyPortNo, UINT2 proto);


    /*
     * 测试打印所有连接信息
     */
    void printAll();

private:
	
    CProxyConnectMgr();
    /*
     * 初始化
     */
    void init();

    /*
     * 在List中查找对应的ProxyIp是否存在
     */
    CProxyConnectHost* findProxyConnectHost(UINT4 proxyIp);

    /*
     * 创建ProxyIp对应的ProxyHostConnect对象,并且添加到List中
     */
    CProxyConnectHost* addProxyConnectHost(UINT4 proxyIp);

    CMutex m_mutex;                                         ///< 互斥锁,保护list
    std::map<UINT4, CProxyConnectHost*> m_pProxyList;       ///< 所有ConnectHost的List

    static CProxyConnectMgr* m_pInstance;
};






#endif

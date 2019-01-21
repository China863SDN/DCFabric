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
*   File Name   : CConnect.h        *
*   Author      : bnc xflu          *
*   Create Date : 2016-9-6          *
*   Version     : 1.0               *
*   Function    : .                 *
*                                                                             *
******************************************************************************/
#ifndef _CCONNECT_H
#define _CCONNECT_H
#include "CHost.h"
#include "bnc-type.h"

/*
 * 定义了主机和主机之间的连接
 */
class CConnect
{
public:
    /*
     *
     * 创建内网之间的主机通信
     *
     * 带参数的构造函数
     *
     * @param: CHost* src       源主机
     * @param: CHost* dst       目标主机
     */
    CConnect(CHost* src, CHost* dst);

    /*
     * 默认析构函数
     */
    ~CConnect();

    /*
     * 处理
     */
    void process();

private:
    /*
     * 默认构造函数
     */
    CConnect();

    CHost* m_srcHost;           ///< 源主机
    CHost* m_dstHost;           ///< 目标主机
    CHost* m_srcProxy;          ///< 源主机的虚拟信息
    CHost* m_dstProxy;          ///< 目标主机的虚拟信息

    // match field
    UINT4 m_iMatchSrcIp;        ///< 匹配字段: 源主机Ip
    UINT4 m_iMatchDstIp;        ///< 匹配字段: 目标主机Ip
    UINT2 m_iMatchSrcPortNo;    ///< 匹配字段: 源主机TCP/UDP端口号
    UINT2 m_iMatchDstPortNo;    ///< 匹配字段: 目标主机TCP/UDP端口号
    UINT1 m_iMatchProto;        ///< 匹配字段: 协议
    UINT1 m_srcMac[6];          ///< 匹配字段: 源主机Mac地址
    UINT1 m_dstMac[6];          ///< 匹配字段: 目标主机Mac地址
};




#endif

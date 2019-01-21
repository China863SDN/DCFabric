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
*   File Name   : CEtherIPHandler.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-7-6           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef _CETHERIPHANDLER_H
#define _CETHERIPHANDLER_H
#include "CHost.h"
#include "CMsgHandler.h"

/*
 * ip消息处理handler�?
 *
 * 负责ip消息处理
 */
class CEtherIPHandler: public CMsgHandler
{
public:
    /*
     * 默认构造函�?
     */
    CEtherIPHandler();

    /*
     * 默认析构函数
     */
    ~CEtherIPHandler();

	
    /*
     * 获取handler名称
     * ret: 返回handler名称，类型为std::string
     */
    const char* toString();

	  /*
     * 完整的消息路径注�?
     *
     * ret: 成功 or 失败
     */
    INT4 onregister();

    /*
     * 完整的消息路径注册撤销
     *
     * ret: None
     */
    void deregister();

private:
    /*
     * 消息处理接口
     * @param: queue    待处理消�?
     *
     * @return: None
     */
    INT4 handle(CSmartPtr<CMsgCommon> msg);


    /*
     * 克隆方法
     * ret: 返回克隆实例指针
     */
    CMsgHandler* clone();

    /*
     * 处理内部网络之间通信
     */
    void processInternalPacket(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
            const CSmartPtr<CSwitch> & srcSw, ip_t* pkt, packet_in_info_t* packetIn);

    /*
     * 处理内部网络访问外部的通信
     */
    void processOutPacket(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, ip_t* pkt, packet_in_info_t* packetIn);

    /*
     * 处理外部网络访问内部的通信
     */
    void processInPacket(CSmartPtr<CHost>& dstHost, ip_t* pkt, packet_in_info_t* packetIn);

    /*
     * 获取SrcPortNo
     */
    virtual UINT4 getSrcPortNo(ip_t* pkt);
    /*
     * 获取DstPortNo
     */
    virtual UINT4 getDstPortNo(ip_t* pkt);

    /*
     * 打印收到的包的信�?
     */
    void printPacketInfo(ip_t* pkt);

private:
    /*
     * 判断是否为广播包
     *
     * @param: src_ip           源IP
     * @param: dst_ip           目的IP
     *
     * @return: BOOL            TRUE:是广播包; FALSE:不是广播�?
     */
    BOOL isFloodPacket(UINT4 srcIp, UINT4 dstIp);
};


#endif

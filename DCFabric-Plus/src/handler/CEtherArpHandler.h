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
*   File Name   : CEtherArpHandler.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-7-6           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef _CETHERARPHANDLER_H
#define _CETHERARPHANDLER_H
#include "CHost.h"
#include "CMsgHandler.h"

/*
 * arp消息处理handler类
 *         负责arp消息处理
 */
class CEtherArpHandler: public CMsgHandler
{
public:
    CEtherArpHandler();
    ~CEtherArpHandler();

	 /*
     * 获取handler名称
     * @return: string 返回handler名称
     */
    const char* toString();

	  /*
     * 完整的消息路径注册
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
     *
     * @param: queue 待处理消息
     */
    INT4 handle(CSmartPtr<CMsgQueue> queue);

	 /*
     * 消息处理接口
     *  子类选择实现
     *
     * @param: msg    消息
     *
     * ret: 成功 or 失败
     */
    INT4 handle(CSmartPtr<CMsgCommon> msg);


    /*
     * 克隆方法
     *
     * @param: 无
     *
     * @return: 返回克隆实例指针
     */
    CMsgHandler* clone();

    /*
     * 判断是不是request包
     *
     * @param: arp_pkt   ARP包指针
     *
     * @return: BOOL     TRUE: 是ARP Request包; FALSE: 不是ARP Request包
     */
    static BOOL isRequest(arp_t* arp_pkt);

    /*
     * 判断是不是reply包
     *
     * @param: arp_pkt  ARP包指针
     *
     * @return: BOOL    TRUE: 是ARP Reply包; FALSE: 不是ARP Reply包
     */
    static BOOL isReply(arp_t* arp_pkt);

private:
    /*
     * 打印ARP包信息
     */
    void printArp(arp_t* pkt);
};


#endif

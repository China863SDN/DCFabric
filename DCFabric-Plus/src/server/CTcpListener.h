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
*   File Name   : CTcpListener.h                                              *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CTCPLISTENER__H
#define __CTCPLISTENER__H

#include "bnc-type.h"
#include "CThread.h"
#include "CRefObj.h"

/**
 * TCP连接监听类
 *     负责处理交换机的连接请求
 */
class CTcpListener : public CRefObj
{
public:
    CTcpListener();
    CTcpListener(UINT4 ip, UINT2 port, INT4 type=THREAD_TYPE_NORMAL);
    virtual ~CTcpListener();

    /*
     * 初始化
     */
    INT4 init();

    /*
     * 监听初始化
     */
    INT4 listen();

    /*
     * 接收新连接请求
     */
    INT4 accept();

    /*
     * 处理新连接
     */
    virtual INT4 process(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port) = 0;

    /*
     * 获取TCP listener名称
     * ret: 返回TCP listener名称
     */
    virtual const char* toString() {return "CTcpListener";}

private:
    //连接ip
    UINT4 m_uiIp;

    //监听端口
    UINT2 m_uiPort;

    //监听socket文件描述符
    INT4 m_iSockfd;

    //侦听线程
    CThread m_thread;
};

#endif

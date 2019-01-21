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
*   File Name   : CRecvWorker.h                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CRECVWORKER__H
#define __CRECVWORKER__H

#include "bnc-type.h"
#include "CThread.h"
#include "CRefObj.h"

/**
 * worker�?
 * 负责接收并封装消息入队列
 * 是所有RecvWork处理类的基类
 */
class CRecvWorker : public CRefObj
{
public:
    const static UINT4 max_epoll_files = 3000;
    const static UINT4 max_recv_buffer_size = 204800;

public:
    /*
     * 默认构造函�?
     */
    CRecvWorker(INT4 type=THREAD_TYPE_NORMAL);

    /*
     * 默认析构函数
     */
    virtual ~CRecvWorker();

    /*
     * 初始�?
     *
     * @return:  成功 or 失败
     */
    virtual INT4 init();

    /*
     * 增加连接
     *
     * @param: fd                   socket描述�?
     *
     * @return: INT4                成功 or 失败
     */
    virtual INT4 addConnFd(INT4 fd);

    void delConnFd(INT4 fd);

    /*
     * 处理消息接收
     * 处理从Epoll_wait得到的消�?
     * 循环读取(因为使用了et模式)直到无数�?
     * 并且将读到的数据保存到缓冲区
     * 然后调用processMsgIn函数进行消息处理
     * processMsgIn为虚函数, 子类必须进行覆写
     *
     * @param: sockfd           socket描述�?
     *
     */
    void receive(INT4 sockfd);

    /*
     * 处理收到消息
     * 子类需要覆�?
     *
     * @param: sockfd           socket描述�?
     * @param: buffer           消息指针
     * @param: len              消息长度
     *
     * @return: None
     */
    virtual INT4 process(INT4 sockfd, INT1* buffer, UINT4 len) = 0;

    /*
     * 获取所创建的Epoll文件描述�?
     *
     * @return: INT4        epoll文件描述�?
     */
    INT4 getEpollFd() {return m_epollFd;}

    /*
     * 处理对端断开连接
     * 子类需要覆�?
     *
     * @param: sockfd           socket描述�?
     *
     * @return: None
     */
    virtual void processPeerDisconn(INT4 sockfd);

    /*
     * 获取RecvWorker名称
     * ret: 返回RecvWorker名称
     */
    virtual const char* toString() {return "CRecvWorker";}

private:
    INT4 m_epollFd;
    INT1 m_buffer[max_recv_buffer_size];
    CThread m_thread;
};

#endif

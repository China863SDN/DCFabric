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
*   File Name   : CRedisSubscriber.h                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CREDISSUB_H
#define __CREDISSUB_H

#include "bnc-type.h"
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"
#include "CThread.h"
#include "CSemaphore.h"

#define REDIS_SUBSCRIBE_CHANNEL "dynamic_sync"

/*
 * redis订阅类
 */
class CRedisSubscriber
{
public:
    /*
     * 获取CRedisSubscriber实例
     *
     * @return: CRedisSubscriber*   CRedisSubscriber实例
     */
    static CRedisSubscriber* getInstance();

    /*
     * 初始化
     *
     * @return: 成功 or 失败
     */
    INT4 init();

    /*
     * 启动订制
     *
     * @return: 成功 or 失败
     */
    INT4 subscribe();

    /*
     * 通知连接成功
     *
     * @return: None
     */
    void notify();

private:
    /*
     * 默认构造函数
     */
    CRedisSubscriber();

    /*
     * 默认析构函数
     */
    ~CRedisSubscriber();

private:
    static CRedisSubscriber* m_instance;       

    std::string              m_serverIp;
    UINT2                    m_serverPort;
    redisAsyncContext*       m_redisAsyncCtx;
    struct event_base*       m_eventBase;
    CThread                  m_thread;
    CSemaphore               m_semaphore;
};


#endif

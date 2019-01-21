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
*   File Name   : CMsgTreeNode.h                                              *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMSGTREENODE_H
#define __CMSGTREENODE_H

#include "bnc-type.h"
#include "CMsgQueue.h"
#include "CRWLock.h"

class CMsgOperator;

typedef std::map<CMsgOperator*, CMsgQueues> CMsgQueuesMap;

/*
 * 消息存储树节点类
 */
class CMsgTreeNode
{
public:
    /*
     * 构造函数
     */
    CMsgTreeNode(CMsgPath& path);

    /*
     * 析构函数
     */
    ~CMsgTreeNode();

    /*
     * 注册某生产者/消费者到消息路径
     * ret: 成功 or 失败
     */
    INT4 onregister(CMsgOperator* oper);

    /*
     * 为某生产者/消费者撤销注册该消息路径
     */
    void deregister(CMsgOperator* oper);

    /*
     * push消息进树节点
     * ret: 成功 or 失败
     */
    INT4 pushMsg(CSmartPtr<CMsgCommon>& msg);

    /*
     * 从树节点中pop出一条消息
     * ret: CSmartPtr<CMsgCommon>
     */
    CSmartPtr<CMsgCommon> popMsg(CMsgOperator* oper);

    /*
     * 从树节点中pop出一系列消息
     * ret: CSmartPtr<CMsgQueue>
     */
    CSmartPtr<CMsgQueue> popMsgQueue(CMsgOperator* oper);

private:
    /*
     * 注册某生产者到消息路径
     * ret: 成功 or 失败
     */
    INT4 registerProducer(CMsgOperator* oper);

    /*
     * 注册某消费者到消息路径
     * ret: 成功 or 失败
     */
    INT4 registerConsumer(CMsgOperator* oper);

    /*
     * 获取某消费者的队列集
     * ret: CMsgQueues* or NULL
     */
    CMsgQueues* getQueues(CMsgOperator* oper);

    /*
     * 为某消费者创建队列集
     * ret: CMsgQueues* or NULL
     */
    CMsgQueues* createQueues(CMsgOperator* oper);

    INT4 getQueuesLevel(CMsgKey& path);

private:
    CMsgPath m_path;
    CMsgQueues m_prodQueues;
    std::map<CMsgOperator*, BOOL> m_producers;
    CMsgQueuesMap m_consumers;
    CRWLock m_rwlock;
};

#endif

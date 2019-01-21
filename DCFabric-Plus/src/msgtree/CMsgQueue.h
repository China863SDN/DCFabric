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
*   File Name   : CMsgQueue.h                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMSGQUEUE_H
#define __CMSGQUEUE_H

#include "bnc-type.h"
#include "CSmartPtr.h"
#include "CMsgCommon.h"
#include "CMutex.h"
#include "CLFQueue.h"
#include "CHashMap.h"
#include "CLKHashMap.h"
#include "CLFHashMap.h"
#include "CRWLock.h"

//#define USING_STL_MAP 1
//#define USING_HASH_MAP 1
#define USING_LF_HASH_MAP 1
//#define USING_LK_HASH_MAP 1

typedef enum 
{
    MSG_QUEUES_LEVEL_1    = 1,
    MSG_QUEUES_LEVEL_10K  = 10240,
    MSG_QUEUES_LEVEL_50K  = 51200,
    MSG_QUEUES_LEVEL_100K = 102400,
}msg_queues_level_e;

typedef enum 
{
    MSG_PUSH_SPEED_UNLIMITED = 0,
    MSG_PUSH_SPEED_ONCE_10MS = 10,
    MSG_PUSH_SPEED_ONCE_20MS = 20,
}msg_push_speed_e;

typedef enum 
{
    MSG_POP_SPEED_UNLIMITED = 0,
    MSG_POP_SPEED_ONCE_10MS = 10,
    MSG_POP_SPEED_ONCE_20MS = 20,
}msg_pop_speed_e;

typedef CLFQueue<CSmartPtr<CMsgCommon> > CMsgQueueBase;
class CMsgQueue : public CMsgQueueBase 
{
public:
    CMsgQueue() {}
    CMsgQueue(const CMsgQueue& rhs): CMsgQueueBase(rhs) {}
    virtual ~CMsgQueue() {}

    virtual INT4 push(CSmartPtr<CMsgCommon>& msg);
    virtual void pop(CSmartPtr<CMsgCommon>& msg);
};
class CPushLimitedMsgQueue : public CMsgQueue 
{
public:
    CPushLimitedMsgQueue(INT4 speed=MSG_PUSH_SPEED_UNLIMITED) 
    {
        m_speed = speed*1000;
        m_tv.tv_sec  = 0;
        m_tv.tv_usec = 0;
    }
    CPushLimitedMsgQueue(const CPushLimitedMsgQueue& rhs): CMsgQueue(rhs)
    {
        m_speed = rhs.m_speed;
        m_tv.tv_sec  = 0;
        m_tv.tv_usec = 0;
    }
    virtual ~CPushLimitedMsgQueue() {}

    virtual INT4 push(CSmartPtr<CMsgCommon>& msg);

private:
    INT4 m_speed;
    struct timeval m_tv;
};
class CPopLimitedMsgQueue : public CMsgQueue 
{
public:
    CPopLimitedMsgQueue(INT4 speed=MSG_POP_SPEED_UNLIMITED) 
    {
        m_speed = speed*1000;
        m_tv.tv_sec  = 0;
        m_tv.tv_usec = 0;
    }
    CPopLimitedMsgQueue(const CPopLimitedMsgQueue& rhs): CMsgQueue(rhs)
    {
        m_speed = rhs.m_speed;
        m_tv.tv_sec  = 0;
        m_tv.tv_usec = 0;
    }
    virtual ~CPopLimitedMsgQueue() {}

    virtual void pop(CSmartPtr<CMsgCommon>& msg);

private:
    INT4 m_speed;
    struct timeval m_tv;
};

typedef std::map<CMsgKey, CSmartPtr<CMsgQueue> > CMsgQueueMap;
typedef CHashMap<std::string, CSmartPtr<CMsgQueue> > CMsgQueueHMap;
typedef CStringLFHashMap<CSmartPtr<CMsgQueue> > CMsgQueueLFHMap;
typedef CStringLKHashMap<CSmartPtr<CMsgQueue> > CMsgQueueLKHMap;

class CMsgOperator;

class CMsgQueues {
public:
    /*
     * 构造函�?
     */
    CMsgQueues(CMsgPath& path, UINT4 bucketNum, CMsgOperator* oper=NULL);

    /*
     * 默认析构函数
     */
    ~CMsgQueues();

    /*
     * push消息进消息队�?
     * ret: 成功 or 失败
     */
    INT4 pushMsg(CSmartPtr<CMsgCommon>& msg);
    
    /*
     * pop出一条消�?
     * ret: CSmartPtr<CMsgCommon>
     */
    CSmartPtr<CMsgCommon> popMsg();

    /*
     * pop出一系列消息
     * ret: CSmartPtr<CMsgQueue>
     */
    CSmartPtr<CMsgQueue> popMsgQueue();

    /*
     * 将有消息派发给消费�?
     * ret: 成功 or 失败
     */
    INT4 distribute(CMsgQueues* consumer);

private:
    /*
     * 根据key从CMsgQueues中查找具体的消息队列
     * ret: CSmartPtr<CMsgQueue>
     */
    CSmartPtr<CMsgQueue> getQueue(const CMsgKey& key);

    /*
     * 根据key在CMsgQueues中创建具体的消息队列
     * ret: CSmartPtr<CMsgQueue>
     */
    CSmartPtr<CMsgQueue> createQueue(const CMsgKey& key);

    CSmartPtr<CMsgQueue> allocQueue(const CMsgKey& key);

    /*
     * 根据key在CMsgQueues中删除具体的消息队列
     */
    void deleteQueue(const CMsgKey& key);

    /*
     * push消息进消息队�?
     */
    void push(CSmartPtr<CMsgQueue>& queue, CSmartPtr<CMsgCommon>& msg);

    /*
     * msgQueue的copy函数
     */
    static void copyMsgQueue(CSmartPtr<CMsgQueue>& rhs, CSmartPtr<CMsgQueue>& lhs);

private:
    CMsgPath m_path;
    CMsgOperator* m_operator;
#if defined(USING_STL_MAP)
    CMsgQueueMap m_queues;
    CRWLock m_rwlock;
#elif defined(USING_HASH_MAP)
    CMsgQueueHMap m_queues;
    CRWLock m_rwlock;
    UINT4 m_bucket;
#elif defined(USING_LF_HASH_MAP)
    CMsgQueueLFHMap m_queues;
#elif defined(USING_LK_HASH_MAP)
    CMsgQueueLKHMap m_queues;
    struct timeval m_tv;
#endif

    UINT4 m_pos;
    CMutex m_mutex;
};

#endif

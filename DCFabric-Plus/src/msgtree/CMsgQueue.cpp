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
*   File Name   : CMsgTreeNode.cpp                                            *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CMsgTreeNode.h"
#include "bnc-inet.h"
#include "bnc-error.h"
#include "CMsg.h"
#include "CMsgHandler.h"
#include "log.h"
#include "comm-util.h"
#include "CConf.h"
#include "CEventReportor.h"
#include "COfMsgUtil.h"

//#define POP_LOCK_GRANULARITY_LARGE 1
#define POP_LOCK_GRANULARITY_LITTLE 1

#if 0
static time_t timePeriod = 0;
#define procStats() \
{\
    if (0 == timePeriod)\
    {\
        timePeriod = time(NULL);\
    }\
    else\
    {\
        time_t tmCurr = time(NULL);\
        if ((tmCurr - timePeriod >= 10) && \
            ((m_path.compare(g_of13MsgPathIp) == 0) || (m_path.compare(g_of13MsgPathArpReq) == 0)))\
        {\
            timePeriod = tmCurr;\
            LOG_WARN_FMT("-------- path[%s] has %llu queues --------", m_path.c_str(), m_queues.size());\
        }\
    }\
}
#endif

INT4 CMsgQueue::push(CSmartPtr<CMsgCommon>& msg)
{
    CMsgQueueBase::push(msg);
    return BNC_OK;
}

void CMsgQueue::pop(CSmartPtr<CMsgCommon>& msg)
{
    CMsgQueueBase::pop(msg);
}

INT4 CPushLimitedMsgQueue::push(CSmartPtr<CMsgCommon>& msg)
{
    if ((0 == m_tv.tv_sec) && (0 == m_tv.tv_usec))
    {
        gettimeofday(&m_tv, NULL);
        CMsgQueueBase::push(msg);
        return BNC_OK;
    }
    else
    {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) == 0)
        {
            if (!((tv.tv_sec == m_tv.tv_sec) && (tv.tv_usec - m_tv.tv_usec < m_speed)))
            {
                m_tv.tv_sec  = tv.tv_sec;
                m_tv.tv_usec = tv.tv_usec;
                CMsgQueueBase::push(msg);
                return BNC_OK;
            }
        }
    }

    return BNC_ERR;
}

void CPopLimitedMsgQueue::pop(CSmartPtr<CMsgCommon>& msg)
{
    if ((0 == m_tv.tv_sec) && (0 == m_tv.tv_usec))
    {
        gettimeofday(&m_tv, NULL);
        CMsgQueueBase::pop(msg);
    }
    else
    {
        struct timeval tv;
        if (gettimeofday(&tv, NULL) == 0)
        {
            if ((tv.tv_sec == m_tv.tv_sec) && (tv.tv_usec - m_tv.tv_usec < m_speed))
                usleep(m_speed-(tv.tv_usec-m_tv.tv_usec));
            m_tv.tv_sec  = tv.tv_sec;
            m_tv.tv_usec = tv.tv_usec;
            CMsgQueueBase::pop(msg);
        }
    }
}

CMsgQueues::CMsgQueues(CMsgPath& path, UINT4 bucketNum, CMsgOperator* oper):
    m_path(path),
    m_operator(oper),
#if defined(USING_HASH_MAP)
    m_queues(bucketNum, hashString, copyMsgQueue),
    m_bucket(0),
#elif defined(USING_LF_HASH_MAP) || defined(USING_LK_HASH_MAP)
    m_queues(bucketNum),
#endif
    m_pos(0)
{
#if defined(USING_LK_HASH_MAP)
    m_tv.tv_sec = 0;
    m_tv.tv_usec = 0;
#endif
}

CMsgQueues::~CMsgQueues()
{
}

INT4 CMsgQueues::pushMsg(CSmartPtr<CMsgCommon>& msg)
{
    LOG_INFO_FMT("push oper[%d]msg[%s]key[%s] into queue of %s ...", 
        msg->getOperation(), msg->getPath().c_str(), msg->getKey().c_str(), 
        (NULL!=m_operator)?m_operator->toString():"producer");

#if defined(USING_LK_HASH_MAP)
    UINT4 index = m_queues.bucketIndex(msg->getKey());
    m_queues.bucketLock(index);
#endif

    CSmartPtr<CMsgQueue> queue = getQueue(msg->getKey());
    if (queue.isNull())
    {
        queue = createQueue(msg->getKey());
        if (queue.isNull())
        {
            LOG_WARN_FMT("%s[%p] createQueue for path[%s]key[%s] failed !", 
                (NULL!=m_operator)?m_operator->toString():"producer", m_operator, m_path.c_str(), msg->getKey().c_str());
#if defined(USING_LK_HASH_MAP)
            m_queues.bucketUnlock(index);
#endif
            return BNC_ERR;
        }
    }

    LOG_INFO_FMT("%s[%p] before push, path[%s]key[%s] has msg count[%lu] ...", 
        (NULL!=m_operator)?m_operator->toString():"producer", m_operator, m_path.c_str(), msg->getKey().c_str(), queue->size());
    push(queue, msg);
    LOG_INFO_FMT("%s[%p] after pushed and rearranged, path[%s]key[%s] has msg count[%lu] ...", 
        (NULL!=m_operator)?m_operator->toString():"producer", m_operator, m_path.c_str(), msg->getKey().c_str(), queue->size());

#if defined(USING_LK_HASH_MAP)
    m_queues.bucketUnlock(index);
#endif

#if 0
    if ((NULL==m_operator)||
        !strcmp(m_operator->toString(),"CSwitchEventTester")||
        !strcmp(m_operator->toString(),"CLinkEventTester")||
        !strcmp(m_operator->toString(),"CPortEventTester"))
    {
        LOG_WARN_FMT("######%s[%p] after pushed and rearranged, path[%s]key[%s] has msg count[%lu] ...", 
            (NULL!=m_operator)?m_operator->toString():"producer", m_operator, m_path.c_str(), msg->getKey().c_str(), queue->size());
        LOG_WARN_FMT("######%s[%p] after pushed msg, path[%s] has queues count[%lu] ...", 
            (NULL!=m_operator)?m_operator->toString():"producer", m_operator, m_path.c_str(), m_queues.size());
    }
#endif

    return BNC_OK;
}

CSmartPtr<CMsgCommon> CMsgQueues::popMsg()
{
    LOG_INFO_FMT("%s[%p] pop msg ...", m_operator->toString(), m_operator);
    LOG_INFO_FMT("%s[%p] before pop msg, path[%s] has queue count[%lu] ...", 
        m_operator->toString(), m_operator, m_path.c_str(), m_queues.size());

    CSmartPtr<CMsgCommon> msg(NULL);

#if defined(USING_STL_MAP)
    //m_rwlock.rlock();

    UINT4 size = m_queues.size();
    if (size <= 0)
    {
        //m_rwlock.unlock();
        return msg;
    }

    m_mutex.lock();

    m_pos %= size;
    CMsgQueueMap::iterator it = m_queues.begin();
    if (m_pos > 0)
        advance(it, m_pos);

    for (UINT4 i = 0; i < size; ++i)
    {
        if (it->second.isNull() || it->second->empty())
        {
            ++ m_pos;
            if (++it == m_queues.end())
            {
                it = m_queues.begin();
                m_pos = 0;
            }
        }
        else
        {
            it->second->pop(msg);
#if 0
            if (it->second->empty())
            {
                deleteQueue(it->first);
                if ((--m_count % 5000 == 0) && (m_count > 0))
                    LOG_WARN_FMT("-------- path[%s] has %llu queues --------", m_path.c_str(), m_count);
                break;
            }
#else
            ++ m_pos;
#endif
            break;
        }
    }

    m_mutex.unlock();
    //m_rwlock.unlock();
#elif defined(USING_HASH_MAP)
    //m_rwlock.rlock();

    UINT4 bucketNum = m_queues.bucketNum();
    if (bucketNum <= 0)
    {
        //m_rwlock.unlock();
        return msg;
    }

    m_mutex.lock();

    m_bucket %= bucketNum;
    CMsgQueueHMap::CBucket* map = m_queues.bucket(m_bucket);

    for (UINT4 i = 0; i < bucketNum+1; ++i)
    {
        if (!map->empty() && (m_pos < map->size()))
        {
            CMsgQueueHMap::CBucket::iterator it = map->begin();
            if (m_pos > 0)
                advance(it, m_pos);
            while (it != map->end())
            {
                if (it->second.isNull() || it->second->empty())
                {
                    ++ it;
                    ++ m_pos;
                }
                else
                {
                    it->second->pop(msg);
                    ++ m_pos;
                    break;
                }
            }
        }

        if (msg.isNotNull())
        {
            break;
        }
        else
        {
            m_pos = 0;
            ++ m_bucket;
            m_bucket %= bucketNum;
            map = m_queues.bucket(m_bucket);
        }
    }

    m_mutex.unlock();
    //m_rwlock.unlock();
#elif defined(USING_LF_HASH_MAP)
    UINT4 size = m_queues.size();
    if (size <= 0)
        return msg;

    m_mutex.lock();

    m_pos %= size;
    CMsgQueueLFHMap::CIterator it = m_queues.begin();
    if (m_pos > 0)
        advance(it, m_pos);

    for (UINT4 i = 0; i < size; ++i)
    {
        if (it->second.isNull() || it->second->empty())
        {
            ++ m_pos;
            if (++it == m_queues.end())
            {
                it = m_queues.begin();
                m_pos = 0;
            }
        }
        else
        {
            it->second->pop(msg);
#if 0
            if (it->second->empty())
            {
                deleteQueue(it->first);
                if ((m_queues.size() > 0) && (m_queues.size() % 5000 == 0))
                    LOG_WARN_FMT("-------- path[%s] has %llu queues --------", m_path.c_str(), m_queues.size());
                break;
            }
#endif
            ++ m_pos;
            break;
        }
    }

    m_mutex.unlock();
#elif defined(USING_LK_HASH_MAP)
    UINT4 size = m_queues.size();
    if (size <= 0)
        return msg;

    m_mutex.lock();

    m_pos %= size;
    CMsgQueueLKHMap::CReferenceIterator it = m_queues.begin();
    if (m_pos > 0)
        advance(it, m_pos);

    for (UINT4 i = 0; i <= size; i++)
    {
        CMsgKey key = it->first;
        CSmartPtr<CMsgQueue> queue = it->second;

#if defined(POP_LOCK_GRANULARITY_LARGE)
        UINT4 index = m_queues.bucketIndex(key);
        //m_queues.bucketLock(index);
        if (m_queues.bucketTrylock(index) != 0)
        {
            ++ it;
            ++ m_pos;
            if (it == m_queues.end())
            {
                it = m_queues.begin();
                m_pos = 0;
            }
            continue;
        }
#endif

        if (queue.isNull() || queue->empty())
        {
            ++ it;
#if defined(POP_LOCK_GRANULARITY_LARGE)
            m_queues.bucketUnlock(index);
#endif
            ++ m_pos;
            if (it == m_queues.end())
            {
                it = m_queues.begin();
                m_pos = 0;
            }
        }
        else
        {
            queue->pop(msg);
#if defined(POP_LOCK_GRANULARITY_LARGE)
            if (queue->empty())
            {
                deleteQueue(key);
                m_queues.bucketUnlock(index);
                break;
            }
            m_queues.bucketUnlock(index);
#elif defined(POP_LOCK_GRANULARITY_LITTLE)
            if (queue->empty())
            {
                UINT4 index = m_queues.bucketIndex(key);
                //m_queues.bucketLock(index);
                if (m_queues.bucketTrylock(index) == 0)
                {
                    if (queue->empty())
                    {
                        deleteQueue(key);
                        m_queues.bucketUnlock(index);
                        break;
                    }
                    m_queues.bucketUnlock(index);
                }
            }
#endif
            ++ m_pos;
            break;
        }
    }

    m_mutex.unlock();
#endif
    
    LOG_INFO_FMT("%s[%p] after poped msg, path[%s] has queue count[%lu] ...", 
        m_operator->toString(), m_operator, m_path.c_str(), m_queues.size());
#if 0
    if (!m_queues.empty()&&
        (!strcmp(m_operator->toString(),"CSwitchEventTester")||
        !strcmp(m_operator->toString(),"CLinkEventTester")||
        !strcmp(m_operator->toString(),"CPortEventTester")))
        if (m_queues.size() == 1)
        {
            CMsgKey key;
            CMsgQueue* queue = NULL;
            m_queues.front(key, queue);
            LOG_WARN_FMT("######%s[%p] after poped msg, path[%s] has only key[%s] queue size[%lu] ...", 
                m_operator->toString(), m_operator, m_path.c_str(), key.c_str(), queue->size());
        }
        else
        {
            LOG_WARN_FMT("######%s[%p] after poped msg, path[%s] has queue count[%lu] ...", 
                m_operator->toString(), m_operator, m_path.c_str(), m_queues.size());
        }
#endif

    return msg;
}

CSmartPtr<CMsgQueue> CMsgQueues::popMsgQueue()
{
    LOG_INFO_FMT("%s[%p] pop msg queue ...", m_operator->toString(), m_operator);
    LOG_INFO_FMT("%s[%p] before pop msg queue, path[%s] has queue count[%lu] ...", 
        m_operator->toString(), m_operator, m_path.c_str(), m_queues.size());

    CSmartPtr<CMsgQueue> queue(NULL);
    
#if defined(USING_STL_MAP)
    //m_rwlock.rlock();

    UINT4 size = m_queues.size();
    if (size <= 0)
    {
        //m_rwlock.unlock();
        return queue;
    }

    m_mutex.lock();

    m_pos %= size;
    CMsgQueueMap::iterator it = m_queues.begin();
    if (m_pos > 0)
        advance(it, m_pos);

    for (UINT4 i = 0; i < size; i++)
    {
        if (it->second.isNull() || it->second->empty() || it->second->isBusy())
        {
            ++ m_pos;
            if (++it == m_queues.end())
            {
                it = m_queues.begin();
                m_pos = 0;
            }
        }
        else
        {
            it->second->setBusy(TRUE);
            queue = it->second;
            ++ m_pos;
            break;
        }
    }

    m_mutex.unlock();
    //m_rwlock.unlock();
#elif defined(USING_HASH_MAP)
    //m_rwlock.rlock();

    UINT4 bucketNum = m_queues.bucketNum();
    if (bucketNum <= 0)
    {
        //m_rwlock.unlock();
        return queue;
    }

    m_mutex.lock();

    m_bucket %= bucketNum;
    CMsgQueueHMap::CBucket* map = m_queues.bucket(m_bucket);

    for (UINT4 i = 0; i < bucketNum+1; ++i)
    {
        if (!map->empty() && (m_pos < map->size()))
        {
            CMsgQueueHMap::CBucket::iterator it = map->begin();
            if (m_pos > 0)
                advance(it, m_pos);
            while (it != map->end())
            {
                if (it->second.isNull() || it->second->empty() || it->second->isBusy())
                {
                    ++ it;
                    ++ m_pos;
                }
                else
                {
                    it->second->setBusy(TRUE);
                    queue = it->second;
                    ++ m_pos;
                    break;
                }
            }
        }

        if (queue.isNotNull())
        {
            break;
        }
        else
        {
            m_pos = 0;
            ++ m_bucket;
            m_bucket %= bucketNum;
            map = m_queues.bucket(m_bucket);
        }
    }

    m_mutex.unlock();
    //m_rwlock.unlock();
#elif defined(USING_LF_HASH_MAP)
    UINT4 size = m_queues.size();
    if (size <= 0)
        return queue;

    m_mutex.lock();

    m_pos %= size;
    CMsgQueueLFHMap::CIterator it = m_queues.begin();
    if (m_pos > 0)
        advance(it, m_pos);

    for (UINT4 i = 0; i < size; i++)
    {
        if (it->second.isNull() || it->second->empty() || it->second->isBusy())
        {
            ++ m_pos;
            if (++it == m_queues.end())
            {
                it = m_queues.begin();
                m_pos = 0;
            }
        }
        else
        {
            it->second->setBusy(TRUE);
            queue = it->second;
            ++ m_pos;
            break;
        }
    }

    m_mutex.unlock();
#elif defined(USING_LK_HASH_MAP)
    UINT4 size = m_queues.size();
    if (size <= 0)
        return queue;

    m_mutex.lock();

    m_pos %= size;
    CMsgQueueLKHMap::CReferenceIterator it = m_queues.begin();
    if (m_pos > 0)
        advance(it, m_pos);

    for (UINT4 i = 0; i <= size; i++)
    {
        CMsgKey key = it->first;
        CSmartPtr<CMsgQueue> que = it->second;

#if defined(POP_LOCK_GRANULARITY_LARGE) || defined(POP_LOCK_GRANULARITY_LITTLE)
        UINT4 index = m_queues.bucketIndex(key);
        //m_queues.bucketLock(index);
        if (m_queues.bucketTrylock(index) != 0)
        {
            ++ it;
            ++ m_pos;
            continue;
        }
#endif

        if (que.isNull() || que->empty())
        {
            it++;
#if defined(POP_LOCK_GRANULARITY_LARGE) || defined(POP_LOCK_GRANULARITY_LITTLE)
            deleteQueue(key);
            m_queues.bucketUnlock(index);
#else
            ++ m_pos;
#endif
        }
        else if (que->isBusy())
        {
            ++ it;
#if defined(POP_LOCK_GRANULARITY_LARGE) || defined(POP_LOCK_GRANULARITY_LITTLE)
            m_queues.bucketUnlock(index);
#endif
            ++ m_pos;
        }
        else
        {
            que->setBusy(TRUE);
#if defined(POP_LOCK_GRANULARITY_LARGE) || defined(POP_LOCK_GRANULARITY_LITTLE)
            m_queues.bucketUnlock(index);
#endif
            queue = que;
            ++ m_pos;
            break;
        }

        if (it == m_queues.end())
        {
            it = m_queues.begin();
            m_pos = 0;
        }
    }

    m_mutex.unlock();
#endif

    LOG_INFO_FMT("%s[%p] after pop msg queue, path[%s] has queue count[%lu] ...", 
        m_operator->toString(), m_operator, m_path.c_str(), m_queues.size());

    return queue;
}

INT4 CMsgQueues::distribute(CMsgQueues* consumer)
{
    if (m_queues.empty())
        return BNC_ERR;

#if defined(USING_STL_MAP)
    //m_rwlock.lock();
    for (CMsgQueueMap::iterator it = m_queues.begin(); it != m_queues.end(); ++ it)
    {
        if (it->second.isNull() || it->second->empty())
            continue;

        CMsgQueue* queue = new CMsgQueue(*(it->second));
        if (NULL == queue)
        {
            LOG_ERROR("new CMsgQueue failed!");
            //m_rwlock.unlock();
            return BNC_ERR;
        }

        std::pair<CMsgQueueMap::iterator, bool> result =
            consumer->m_queues.insert(CMsgQueueMap::value_type(it->first, CSmartPtr<CMsgQueue>(queue)));
        if (!result.second)
        {
            LOG_WARN_FMT("STL map insert CSmartPtr<CMsgQueue> for path[%s]key[%s] failed !", m_path.c_str(), it->first.c_str());
            //m_rwlock.unlock();
            return BNC_ERR;
        }
    }
    //m_rwlock.unlock();
#elif defined(USING_HASH_MAP)
    //m_rwlock.lock();
    consumer->m_queues = m_queues;
    //m_rwlock.unlock();
#elif defined(USING_LF_HASH_MAP)
    for (CMsgQueueLFHMap::CIterator it = m_queues.begin(); it != m_queues.end(); ++ it)
    {
        if (it->second.isNull() || it->second->empty())
            continue;

        CMsgQueue* queue = new CMsgQueue(*(it->second));
        if (NULL == queue)
        {
            LOG_ERROR("new CMsgQueue failed!");
            return BNC_ERR;
        }

        if (!consumer->m_queues.insert(it->first, CSmartPtr<CMsgQueue>(queue)))
        {
            LOG_WARN_FMT("CLFHashMap insert CSmartPtr<CMsgQueue> for path[%s]key[%s] failed !", m_path.c_str(), it->first.c_str());
            return BNC_ERR;
        }
    }
#elif defined(USING_LK_HASH_MAP)
    for (CMsgQueueLKHMap::CReferenceIterator it = m_queues.begin(); it != m_queues.end(); ++ it)
    {
        if (it->second.isNull() || it->second->empty())
            continue;

        CMsgQueue* queue = new CMsgQueue(*(it->second));
        if (NULL == queue)
        {
            LOG_ERROR("new CMsgQueue failed!");
            return BNC_ERR;
        }

        UINT4 index = m_queues.bucketIndex(it->first);
        if (consumer->m_queues.insert(index, it->first, CSmartPtr<CMsgQueue>(queue)) == NULL)
        {
            LOG_WARN_FMT("CLKHashMap insert CSmartPtr<CMsgQueue> for path[%s]key[%s] failed !", m_path.c_str(), it->first.c_str());
            return BNC_ERR;
        }
    }
#endif

    LOG_WARN_FMT(">>== on path[%s], after distributed, producer has %lu queues, and consumer has %lu queues ==<<", 
        m_path.c_str(), m_queues.size(), consumer->m_queues.size());
    return BNC_OK;
}

CSmartPtr<CMsgQueue> CMsgQueues::getQueue(const CMsgKey& key)
{
    CSmartPtr<CMsgQueue> queue;

#if defined(USING_STL_MAP)
    //m_rwlock.rlock();
    CMsgQueueMap::iterator it = m_queues.find(key);
    if (it != m_queues.end())
        queue = it->second;
    //m_rwlock.unlock();
#elif defined(USING_HASH_MAP)
    //m_rwlock.rlock();
    CSmartPtr<CMsgQueue>* pqueue = m_queues.find(key);
    if (NULL != pqueue)
        queue = *pqueue;
    //m_rwlock.unlock();
#elif defined(USING_LF_HASH_MAP)
    CMsgQueueLFHMap::CPair* item = NULL;
    if (m_queues.find(key, &item))
        queue = item->second;
#elif defined(USING_LK_HASH_MAP)
    UINT4 index = m_queues.bucketIndex(key);
    CSmartPtr<CMsgQueue>* pqueue = m_queues.find(index, key);
    if (NULL != pqueue)
        queue = *pqueue;
    //procStats();
#endif

    return queue;
}

CSmartPtr<CMsgQueue> CMsgQueues::createQueue(const CMsgKey& key)
{
#if defined(USING_STL_MAP)
    m_rwlock.wlock();

    CMsgQueueMap::iterator it = m_queues.find(key);
    if ((it != m_queues.end()) && it->second.isNotNull())
    {
        m_rwlock.unlock();
        return it->second;
    }

#if 0
    CMsgQueue* queue = new CMsgQueue();
    if (NULL == queue)
    {
        LOG_ERROR("new CMsgQueue failed!");
        m_rwlock.unlock();
        return CSmartPtr<CMsgQueue>(NULL);
    }
    CSmartPtr<CMsgQueue> squeue(queue);
#else
    CSmartPtr<CMsgQueue> squeue = allocQueue(key);
    if (squeue.isNull())
    {
        m_rwlock.unlock();
        return CSmartPtr<CMsgQueue>(NULL);
    }
#endif

    if (it != m_queues.end())
    {
        it->second = squeue;
    }
    else
    {
        std::pair<CMsgQueueMap::iterator, bool> result =
            m_queues.insert(CMsgQueueMap::value_type(key, squeue));
        if (!result.second)
        {
            LOG_WARN_FMT("STL map insert CSmartPtr<CMsgQueue> for path[%s]key[%s] failed !", m_path.c_str(), key.c_str());
            m_rwlock.unlock();
            return CSmartPtr<CMsgQueue>(NULL);
        }
    }

    if (m_queues.size() % 5000 == 0)
        LOG_WARN_FMT("-------- path[%s] has %llu queues --------", m_path.c_str(), m_queues.size());

    m_rwlock.unlock();

    return squeue;
#elif defined(USING_HASH_MAP)
    m_rwlock.wlock();

    CSmartPtr<CMsgQueue>* pqueue = m_queues.find(key);
    if ((NULL != pqueue) && pqueue->isNotNull())
    {
        m_rwlock.unlock();
        return *pqueue;
    }

#if 0
    CMsgQueue* queue = new CMsgQueue();
    if (NULL == queue)
    {
        LOG_ERROR("new CMsgQueue failed!");
        m_rwlock.unlock();
        return CSmartPtr<CMsgQueue>(NULL);
    }
    CSmartPtr<CMsgQueue> squeue(queue);
#else
    CSmartPtr<CMsgQueue> squeue = allocQueue(key);
    if (squeue.isNull())
    {
        m_rwlock.unlock();
        return CSmartPtr<CMsgQueue>(NULL);
    }
#endif

    if (NULL == pqueue)
    {
        if (m_queues.insert(key, squeue) == NULL)
        {
            LOG_WARN_FMT("CHashMap insert CSmartPtr<CMsgQueue> for path[%s]key[%s] failed !", m_path.c_str(), key.c_str());
            m_rwlock.unlock();
            return CSmartPtr<CMsgQueue>(NULL);
        }
    }
    else
    {
        *pqueue = squeue;
    }

    if (m_queues.size() % 5000 == 0)
        LOG_WARN_FMT("-------- path[%s] has %llu queues --------", m_path.c_str(), m_queues.size());

    m_rwlock.unlock();

    return squeue;
#elif defined(USING_LF_HASH_MAP)
#if 0
    CMsgQueue* queue = new CMsgQueue();
    if (NULL == queue)
    {
        LOG_ERROR("new CMsgQueue failed!");
        return CSmartPtr<CMsgQueue>(NULL);
    }
    CSmartPtr<CMsgQueue> squeue(queue);
#else
    CSmartPtr<CMsgQueue> squeue = allocQueue(key);
    if (squeue.isNull())
    {
        return CSmartPtr<CMsgQueue>(NULL);
    }
#endif

    if (!m_queues.insert(key, squeue))
    {
        LOG_WARN_FMT("CLFHashMap insert CSmartPtr<CMsgQueue> for path[%s]key[%s] failed !", m_path.c_str(), key.c_str());
        return CSmartPtr<CMsgQueue>(NULL);
    }

#if 0
    if (ret.isNotNull())
        if (m_queues.size() % 5000 == 0)
            LOG_WARN_FMT("-------- path[%s] has %llu queues --------", m_path.c_str(), m_queues.size());
#endif

    return squeue;
#elif defined(USING_LK_HASH_MAP)
#if 0
    CMsgQueue* queue = new CMsgQueue();
    if (NULL == queue)
    {
        LOG_ERROR("new CMsgQueue failed!");
        return CSmartPtr<CMsgQueue>(NULL);
    }
    CSmartPtr<CMsgQueue> squeue(queue);
#else
    CSmartPtr<CMsgQueue> squeue = allocQueue(key);
    if (squeue.isNull())
    {
        return CSmartPtr<CMsgQueue>(NULL);
    }
#endif

    UINT4 index = m_queues.bucketIndex(key);
    if (m_queues.insert(index, key, squeue) == NULL)
    {
        LOG_WARN_FMT("CLKHashMap insert CSmartPtr<CMsgQueue> for path[%s]key[%s] failed !", m_path.c_str(), key.c_str());
        return CSmartPtr<CMsgQueue>(NULL);
    }

    //procStats();
    return squeue;
#endif
}

CSmartPtr<CMsgQueue> CMsgQueues::allocQueue(const CMsgKey& key)
{
    CMsgQueue* queue = NULL;
    if ((m_path.compare(g_of13MsgPathArpReq) == 0) && !key.empty())
    {
        queue = new CPushLimitedMsgQueue(MSG_PUSH_SPEED_ONCE_20MS);
        if (NULL == queue)
            LOG_ERROR("new CPushLimitedMsgQueue failed!");
    }
    else if ((m_path.compare(g_of13MsgPathIp) == 0) && !key.empty())
    {
        queue = new CPopLimitedMsgQueue(MSG_POP_SPEED_ONCE_10MS);
        if (NULL == queue)
            LOG_ERROR("new CPopLimitedMsgQueue failed!");
    }
    else
    {
        queue = new CMsgQueue();
        if (NULL == queue)
            LOG_ERROR("new CMsgQueue failed!");
    }

    return CSmartPtr<CMsgQueue>(queue);
}

void CMsgQueues::deleteQueue(const CMsgKey& key)
{
#if defined(USING_STL_MAP)
    m_rwlock.wlock();
    m_queues.erase(key);
    m_rwlock.unlock();
#elif defined(USING_HASH_MAP)
    m_rwlock.wlock();
    m_queues.remove(key);
    m_rwlock.unlock();
#elif defined(USING_LF_HASH_MAP)
    m_queues.erase(key);
#elif defined(USING_LK_HASH_MAP)
    UINT4 index = m_queues.bucketIndex(key);
    m_queues.remove(index, key);
#endif
}

void CMsgQueues::push(CSmartPtr<CMsgQueue>& queue, CSmartPtr<CMsgCommon>& msg)
{
    CSmartPtr<CMsgCommon> drop;
    
    switch (msg->getOperation())
    {
        case MSG_OPER_REALTIME:
            if (!msg->getKey().empty())
            {
                if (queue->empty())
                {
                    if (queue->push(msg) == BNC_OK)
                        if ((NULL != m_operator) && (m_operator->getType() == MSG_OPERATOR_CONSUMER))
                            m_operator->notify(m_path);
                }
                //drop
                break;
            }
            //goto following
        case MSG_OPER_HOLD:
            if (queue->push(msg) == BNC_OK)
            {
                if (queue->size() > CConf::getInstance()->getMsgHoldNumber())
                    queue->pop(drop);
                else if ((NULL != m_operator) && (m_operator->getType() == MSG_OPERATOR_CONSUMER))
                    m_operator->notify(m_path);
            }
            break;
        case MSG_OPER_OVERRIDE:
            if (queue->push(msg) == BNC_OK)
            {
                if (queue->size() > 1)
                    queue->pop(drop);
                else if ((NULL != m_operator) && (m_operator->getType() == MSG_OPERATOR_CONSUMER))
                    m_operator->notify(m_path);
            }
            break;
        case MSG_OPER_PASSON:
            if (queue->push(msg) == BNC_OK)
                if ((NULL != m_operator) && (m_operator->getType() == MSG_OPERATOR_CONSUMER))
                    m_operator->notify(m_path);
            break;
        default:
            break;
    }
}

void CMsgQueues::copyMsgQueue(CSmartPtr<CMsgQueue>& rhs, CSmartPtr<CMsgQueue>& lhs)
{
    if (rhs.isNotNull() && !rhs->empty())
    {
        if (lhs.isNull())
        {
            CMsgQueue* queue = new CMsgQueue(*rhs);
            if (NULL == queue)
            {
                LOG_ERROR("new CMsgQueue failed!");
                return;
            }
            lhs = CSmartPtr<CMsgQueue>(queue);
        }
        else
        {
            lhs->clear();
            *lhs = *rhs;
        }
    }
}


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

extern const CMsgPath g_of13MsgPathArpReq;
extern const CMsgPath g_of13MsgPathArpRsp;
extern const CMsgPath g_of13MsgPathIp;

CMsgTreeNode::CMsgTreeNode(CMsgKey& path):
    m_path(path),
    m_prodQueues(path, getQueuesLevel(path))
{
}

CMsgTreeNode::~CMsgTreeNode()
{
}

INT4 CMsgTreeNode::onregister(CMsgOperator* oper)
{
    if (NULL == oper)
        return BNC_ERR;

    INT4 ret = BNC_ERR;

    if (oper->getType() == MSG_OPERATOR_PRODUCER)
    {
        ret = registerProducer(oper);
    }
    else if (oper->getType() == MSG_OPERATOR_CONSUMER)
    {
        ret = registerConsumer(oper);
    }

    return ret;
}

void CMsgTreeNode::deregister(CMsgOperator* oper)
{
    if (NULL == oper)
        return;

    if (oper->getType() == MSG_OPERATOR_PRODUCER)
    {
        LOG_INFO_FMT("#%s[%p]# before deregister, producer size[%lu] in node of path[%s] ...", 
            oper->toString(), oper, m_producers.size(), m_path.c_str());
        m_producers.erase(oper);
        LOG_INFO_FMT("#%s[%p]# after deregister, producer size[%lu] in node of path[%s] ...", 
            oper->toString(), oper, m_producers.size(), m_path.c_str());
    }
    else
    {
        LOG_INFO_FMT("#%s[%p]# before deregister, consumer size[%lu] in node of path[%s] ...", 
            oper->toString(), oper, m_consumers.size(), m_path.c_str());
        m_rwlock.wlock();
        m_consumers.erase(oper);
        m_rwlock.unlock();
        LOG_INFO_FMT("#%s[%p]# after deregister, consumer size[%lu] in node of path[%s] ...", 
            oper->toString(), oper, m_consumers.size(), m_path.c_str());
    }
}

INT4 CMsgTreeNode::pushMsg(CSmartPtr<CMsgCommon>& msg)
{
    LOG_INFO_FMT("push oper[%d]msg[%s]key[%s] into tree node[%s] ...", 
        msg->getOperation(), msg->getPath().c_str(), msg->getKey().c_str(), m_path.c_str());

    //非实时消息同时分发给生产�?
    if ((MSG_OPER_HOLD == msg->getOperation()) || (MSG_OPER_OVERRIDE == msg->getOperation()))
        m_prodQueues.pushMsg(msg);
    
    //所有消息同时分发给消费�?
    //m_rwlock.rlock();
    STL_FOR_LOOP(m_consumers, it)
        it->second.pushMsg(msg);
    //m_rwlock.unlock();
   
    return BNC_OK;
}

CSmartPtr<CMsgCommon> CMsgTreeNode::popMsg(CMsgOperator* oper)
{
    CSmartPtr<CMsgCommon> msg(NULL);

    if (NULL == oper)
        return msg;

    LOG_INFO_FMT("%s[%p] pop msg on path[%s] ...", oper->toString(), oper, m_path.c_str());

    CMsgQueues* queues = getQueues(oper);
    if (NULL != queues)
        msg = queues->popMsg();

    return msg;
}

CSmartPtr<CMsgQueue> CMsgTreeNode::popMsgQueue(CMsgOperator* oper)
{
    CSmartPtr<CMsgQueue> queue(NULL);

    if (NULL == oper)
        return queue;

    LOG_INFO_FMT("%s[%p] pop msg queue on path[%s] ...", oper->toString(), oper, m_path.c_str());

    CMsgQueues* queues = getQueues(oper);
    if (NULL != queues)
        queue = queues->popMsgQueue();

    return queue;
}

INT4 CMsgTreeNode::registerProducer(CMsgOperator* oper)
{
    LOG_INFO_FMT("#%s[%p]# before register, producer size[%lu] in node of path[%s] ...", 
        oper->toString(), oper, m_producers.size(), m_path.c_str());

    INT4 ret = BNC_ERR;

    std::map<CMsgOperator*, BOOL>::iterator it = m_producers.find(oper);        
    if (it == m_producers.end())
    {
        std::pair<std::map<CMsgOperator*, BOOL>::iterator, bool> result =
            m_producers.insert(std::pair<CMsgOperator*, BOOL>(oper, TRUE));
        if (result.second)
            ret = BNC_OK;
    }
    
    LOG_INFO_FMT("#%s[%p]# after registered, producer size[%lu] in node of path[%s] ...", 
        oper->toString(), oper, m_producers.size(), m_path.c_str());

    return ret;
}

INT4 CMsgTreeNode::registerConsumer(CMsgOperator* oper)
{
    LOG_INFO_FMT("#%s[%p]# before register, consumers size[%lu] in node of path[%s] ...", 
        oper->toString(), oper, m_consumers.size(), m_path.c_str());

    INT4 ret = BNC_ERR;

    CMsgQueues* queues = getQueues(oper);
    if (NULL == queues)
    {
        queues = createQueues(oper);
        if (NULL == queues)
        {
            LOG_WARN_FMT("%s[%p] createQueues failed !", oper->toString(), oper);
            return BNC_ERR;
        }
        ret = BNC_OK;
    }

    if ((BNC_OK == ret) && (NULL != queues))
    {
        if (m_prodQueues.distribute(queues) == BNC_OK)
            oper->notify(m_path);
    }

    LOG_INFO_FMT("#%s[%p]# after registered, consumers size[%lu] in node of path[%s] ...", 
        oper->toString(), oper, m_consumers.size(), m_path.c_str());

    return ret;
}

CMsgQueues* CMsgTreeNode::getQueues(CMsgOperator* oper)
{
    CMsgQueues* queues = NULL;

    //m_rwlock.rlock();
    CMsgQueuesMap::iterator it = m_consumers.find(oper);        
    if (it != m_consumers.end())
        queues = &(it->second);
    //m_rwlock.unlock();

    return queues;
}

CMsgQueues* CMsgTreeNode::createQueues(CMsgOperator* oper)
{
    CMsgQueues* queues = NULL;

    m_rwlock.wlock();

    CMsgQueuesMap::iterator it = m_consumers.find(oper);
    if (it == m_consumers.end())
    {
        std::pair<CMsgQueuesMap::iterator, bool> result =
            m_consumers.insert(CMsgQueuesMap::value_type(oper, CMsgQueues(m_path, getQueuesLevel(m_path), oper)));
        if (result.second)
            queues = &(result.first->second);
    }
    else
        queues = &(it->second);

    m_rwlock.unlock();

    return queues;
}

INT4 CMsgTreeNode::getQueuesLevel(CMsgKey& path)
{
    if ((path.compare(g_of13MsgPathIp) == 0) || 
        (path.compare(g_of13MsgPathArpReq) == 0) ||
        (path.compare(g_of13MsgPathArpRsp) == 0))
        return MSG_QUEUES_LEVEL_100K;
    else if ((path.compare(g_portEventPath[0]) == 0) ||
        (path.compare(g_topoLinkEventPath[0]) == 0))
        return MSG_QUEUES_LEVEL_50K;
    else if (path.compare(0, 3, g_switchEventPath[0]) == 0)
        return MSG_QUEUES_LEVEL_10K;
    else
        return MSG_QUEUES_LEVEL_1;
}


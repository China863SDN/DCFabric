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
*   File Name   : CMsgTree.cpp                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CMsgTree.h"
#include "COfMsgUtil.h"
#include "comm-util.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "openflow-common.h"
#include "bnc-inet.h"
#include "bnc-error.h"
#include "log.h"
#include "CMsgTreeNode.h"
#include "CMsg.h"
#include "CMsgHandler.h"

CMsgTree::CMsgTree():m_nodes(hash_bucket_number)
{
}

CMsgTree::~CMsgTree()
{
}

INT4 CMsgTree::init()
{
    return BNC_OK;
}

INT4 CMsgTree::onregister(CMsgPath& path, CMsgOperator* oper)
{
    if (path.empty() || (NULL == oper))
        return BNC_ERR;

    LOG_INFO_FMT("#%s[%p]# before register to path[%s], tree map size[%lu] ...", 
        oper->toString(), oper, path.c_str(), m_nodes.size());

    CMsgTreeNode* node = getNode(path);
    if (NULL == node)
    {
        node = createNode(path);
        if (NULL == node)
        {
            LOG_WARN_FMT("%s[%p] createNode for path[%s] failed !", oper->toString(), oper, path.c_str());
            return BNC_ERR;
        }
    }

    INT4 result = node->onregister(oper);

    LOG_INFO_FMT("#%s[%p]# after registered to path[%s] %s, tree map size[%lu] ...", 
        oper->toString(), oper, path.c_str(), (result==BNC_OK)?"success":"failed", m_nodes.size());
    return result;
}

void CMsgTree::deregister(CMsgPath& path, CMsgOperator* oper)
{
    if (path.empty() || (NULL == oper))
        return;

    LOG_INFO_FMT("before deregister path[%s] for %s[%p], tree map size[%lu] ...", 
        path.c_str(), oper->toString(), oper, m_nodes.size());

    CMsgTreeNode* node = getNode(path);
    if (NULL != node)
        node->deregister(oper);

    LOG_INFO_FMT("after deregister path[%s] for %s[%p], tree map size[%lu] ...", 
        path.c_str(), oper->toString(), oper, m_nodes.size());
}

INT4 CMsgTree::pushMsg(CSmartPtr<CMsgCommon>& msg)
{
    LOG_INFO_FMT("push oper[%d]msg[%s]key[%s] into tree ...", 
        msg->getOperation(), msg->getPath().c_str(), msg->getKey().c_str());

    CMsgTreeNode* node = getNode(msg->getPath());
    if (NULL == node)
    {
        LOG_INFO_FMT("msg path[%s] unregistered ...", msg->getPath().c_str());
        return BNC_ERR;
    }

    return node->pushMsg(msg);
}

INT4 CMsgTree::pushMsg(CSmartPtr<CMsgCommon>& msg, INT4 state)
{
    CMsgPath pathState(msg->getPath());
    pathState += ":";
    pathState += to_string(state);

    LOG_INFO_FMT("push oper[%d]msg[%s]key[%s] into tree ...", 
        msg->getOperation(), pathState.c_str(), msg->getKey().c_str());

    CMsgTreeNode* node = getNode(pathState);
    if (NULL == node)
    {
        LOG_INFO_FMT("msg path[%s] unregistered ...", pathState.c_str());
        return BNC_ERR;
    }

    return node->pushMsg(msg);
}

CSmartPtr<CMsgCommon> CMsgTree::popMsg(CMsgPath& path, CMsgOperator* oper)
{
    CSmartPtr<CMsgCommon> msg;

    if (path.empty() || (NULL == oper))
        return msg;

    LOG_INFO_FMT("%s[%p] pop msg on path[%s] ...", oper->toString(), oper, path.c_str());

    CMsgTreeNode* node = getNode(path);
    if (NULL != node)
        msg = node->popMsg(oper);

    return msg;
}

CSmartPtr<CMsgQueue> CMsgTree::popMsgQueue(CMsgPath& path, CMsgOperator* oper)
{
    CSmartPtr<CMsgQueue> queue(NULL);

    if (path.empty() || (NULL == oper))
        return queue;

    LOG_INFO_FMT("%s[%p] pop msg queue on path[%s] ...", oper->toString(), oper, path.c_str());

    CMsgTreeNode* node = getNode(path);
    if (NULL != node)
        queue = node->popMsgQueue(oper);

    return queue;
}

CMsgTreeNode* CMsgTree::getNode(CMsgPath& path)
{
#if 1
#if 0
    CTreeNodeMap::iterator it = m_nodes.find(path);
    if (it != m_nodes.end())
        return &(it->second);
    return NULL;
#else
    return m_nodes.find(path);
#endif
#else
    CTreeNodeHMap::CPair* item = NULL;
    if (m_nodes.find(path, &item))
        return &(item->second);
    return NULL;
#endif
}

CMsgTreeNode* CMsgTree::createNode(CMsgPath& path)
{
#if 1
    CMsgTreeNode* node = NULL;

#if 0
    std::pair<CTreeNodeMap::iterator, bool> result =
        m_nodes.insert(CTreeNodeMap::value_type(path, CMsgTreeNode(path)));
    if (result.second)
        node = &(result.first->second);
#else
    m_rwlock.wlock();
    node = m_nodes.find(path);
    if (NULL == node)
        node = m_nodes.insert(path, CMsgTreeNode(path));
    m_rwlock.unlock();
#endif

    return node;
#else
    CTreeNodeHMap::CPair* item = NULL;
    if (!m_nodes.insert(path, CMsgTreeNode(path), &item))
    {
        LOG_WARN_FMT("add tree node for path[%s] failed !", path.c_str());
        return NULL;
    }
    return &(item->second);
#endif
}


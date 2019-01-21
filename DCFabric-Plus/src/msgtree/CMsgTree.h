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
*   File Name   : CMsgTree.h                                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMSGTREE_H
#define __CMSGTREE_H

#include "bnc-type.h"
#include "CSmartPtr.h"
#include "CMsgTreeNode.h"
#include "CHashMap.h"
#include "CRWLock.h"
#include "CLFHashMap.h"

class CMsgCommon;
class CMsgOperator;

typedef std::map<CMsgPath, CMsgTreeNode> CTreeNodeMap;
typedef CStringHashMap<CMsgTreeNode> CTreeNodeHMap;
//typedef CStringLFHashMap<CMsgTreeNode> CTreeNodeHMap;

/*
 * 消息树类
 */
class CMsgTree
{
public:
    /*
     * 默认构造函数
     */
    CMsgTree();

    /*
     * 默认析构函数
     */
    ~CMsgTree();

    /*
     * 初始化
     * ret: 成功 or 失败
     */
    INT4 init();

    /*
     * 为某handler注册消息路径
     * ret: 成功 or 失败
     */
    INT4 onregister(CMsgPath& path, CMsgOperator* oper);

    /*
     * 为某handler撤销注册消息路径
     */
    void deregister(CMsgPath& path, CMsgOperator* oper);

    /*
     * push无状态消息进树
     * ret: 成功 or 失败
     */
    INT4 pushMsg(CSmartPtr<CMsgCommon>& msg);

    /*
     * push有状态消息进树
     * ret: 成功 or 失败
     */
    INT4 pushMsg(CSmartPtr<CMsgCommon>& msg, INT4 state);

    /*
     * 从树中pop出一条消息
     * ret: CSmartPtr<CMsgCommon>* or NULL
     */
    CSmartPtr<CMsgCommon> popMsg(CMsgPath& path, CMsgOperator* oper);

    /*
     * 从树中pop出一系列消息
     * ret: CSmartPtr<CMsgQueue>
     */
    CSmartPtr<CMsgQueue> popMsgQueue(CMsgPath& path, CMsgOperator* oper);

private:
    /*
     * 根据path从消息树中查找相应的节点
     * ret: CMsgTreeNode* or NULL
     */
    CMsgTreeNode* getNode(CMsgPath& path);

    /*
     * 根据path在消息树中创建相应的节点
     * ret: CMsgTreeNode* or NULL
     */
    CMsgTreeNode* createNode(CMsgPath& path);

private:
    static const UINT4 hash_bucket_number = 1024;

    //CTreeNodeMap m_nodes;
    CTreeNodeHMap m_nodes;
    CRWLock m_rwlock;
};

#endif

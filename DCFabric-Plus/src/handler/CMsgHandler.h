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
*   File Name   : CMsgHandler.h                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMSGHANDLER_H
#define __CMSGHANDLER_H

#include "bnc-type.h"
#include "CSmartPtr.h"
#include "CMsg.h"
#include "CSemaphore.h"
#include "CMsgTreeNode.h"
#include "CMsgOperator.h"
#include "CThread.h"

class CSemThreads 
{
public:
    CSemThreads():m_integrated(FALSE),m_type(THREAD_TYPE_NORMAL) {}
    CSemThreads(BOOL integrated, INT4 type):m_integrated(integrated),m_type(type) {}
    ~CSemThreads() {}

    CSemaphore m_sem;
    std::vector<CThread> m_threads;
    BOOL m_integrated; //when integrated enabled, msg queue will be processed one time
    INT4 m_type;
};

/*
 * 消息处理handler基类
 * 负责具体的消息处理, 子类必须实现handle接口
 */
class CMsgHandler : public CMsgOperator
{
public:
    /*
     * 默认构造函数
     */
    CMsgHandler();

    /*
     * 默认析构函数
     */
    virtual ~CMsgHandler();

    /*
     * 完整的消息路径注册
     * 纯虚函数, 子类必须重新定义
     *
     * ret: 成功 or 失败
     */
    virtual INT4 onregister() = 0;

    /*
     * 完整的消息路径注册撤销
     * 纯虚函数, 子类必须重新定义
     *
     * ret: None
     */
    virtual void deregister() = 0;

    /*
     * 注册某消息路径
     *
     * @param: path         消息路径
     * @param: threadNum    处理相应消息的线程个数
     * @param: integrated   每次处理单个消息还是消息队列
     * @param: type         处理相应消息的线程类型，默认普通类型
     *
     * ret: 成功 or 失败
     */
    INT4 onregister(CMsgPath& path, INT4 threadNum, BOOL integrated=FALSE, INT4 type=THREAD_TYPE_NORMAL);

    /*
     * 撤销注册某消息路径
     *
     * @param: path         消息路径
     *
     * ret: None
     */
    void deregister(CMsgPath& path);

    /*
     * 注册某消息路径（特定状态）
     *
     * @param: path         消息路径
     * @param: state        特定消息状态
     * @param: threadNum    处理相应消息的线程个数
     * @param: integrated   每次处理单个消息还是消息队列
     * @param: type         处理相应消息的线程类型
     *
     * ret: 成功 or 失败
     */
    INT4 onregister(CMsgPath& path, INT4 state, INT4 threadNum, BOOL integrated, INT4 type);

    /*
     * 撤销注册某消息路径（特定状态）
     *
     * @param: path         消息路径
     * @param: state        特定消息状态
     *
     * ret: None
     */
    void deregister(CMsgPath& path, INT4 state);

    /*
     * 通知handler线程有新消息
     *
     * @param: path    消息路径
     *
     * @return: None
     */
    void notify(CMsgPath& path);

    /*
     * 撤销通知
     * @param: path    消息路径
     */
    void cancel(CMsgPath& path);

    /*
     * 事件处理接口
     * 默认从消息树中获取单条消息，然后调用单条消息处理接口handle(CSmartPtr<CMsgCommon>* msg)
     * 虚函数, 子类可以重新定义
     *
     * @param: path    发生事件的消息路径
     *
     * ret: 成功 or 失败
     */
    INT4 handle(CMsgPath& path);

    /*
     * 获取handler名称
     * ret: 返回handler名称
     */
    virtual const char* toString() {return "CMsgHandler";}

private:
    /*
     * 消息队列处理接口
     * 虚函数, 子类选择实现
     *
     * @param: queue    消息队列
     *
     * ret: 成功 or 失败
     */
    virtual INT4 handle(CSmartPtr<CMsgQueue> queue);

    /*
     * 消息处理接口
     * 虚函数, 子类选择实现
     *
     * @param: msg    消息
     *
     * ret: 成功 or 失败
     */
    virtual INT4 handle(CSmartPtr<CMsgCommon> msg);

    CSemThreads* getSemThreads(CMsgPath& path);

private:
    UINT4 m_count;
    CMsgPath m_firstPath;
    CSemThreads m_firstSem;
    std::map<CMsgPath, CSemThreads> m_morePaths;
};

#endif

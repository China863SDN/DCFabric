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
*   File Name   : CThread.h                                                   *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CTHREAD_H
#define __CTHREAD_H

#include "bnc-type.h"

typedef void* (*ROUTINE)(void*);

typedef enum {
    THREAD_TYPE_NORMAL = 0,
    THREAD_TYPE_PRIOR,
    THREAD_TYPE_BIND_CORE,
    THREAD_TYPE_OCCUPY_CORE
}thread_type_e;

/*
 * 定时器类
 */
class CThread
{
public:
    CThread(INT4 type=THREAD_TYPE_NORMAL);
    ~CThread();

    /*
     * 执行定时任务
     * param routine: 线程函数入口
     * param param: 线程函数所需要的参数
     * param owner: 线程启动者
     * ret: 成功 or 失败
     */
    INT4 init(ROUTINE routine, void* param, std::string owner);

    pthread_t getThreadId() {return m_threadId;}

private:
    INT4           m_type;
    pthread_attr_t m_attr;    
    pthread_t      m_threadId;
    ROUTINE        m_routine;
    void*          m_param;
    std::string    m_owner;
};

#endif

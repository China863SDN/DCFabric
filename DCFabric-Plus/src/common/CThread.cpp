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
*   File Name   : CThread.cpp                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CThread.h"
#include "log.h"
#include "comm-util.h"
#include "bnc-error.h"
#include "CCpuCtrl.h"

CThread::CThread(INT4 type):m_type(type),m_threadId(0),m_routine(NULL),m_param(NULL)
{
    pthread_attr_init(&m_attr);
    pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);
}

CThread::~CThread()
{
    pthread_attr_destroy(&m_attr);

    if (m_threadId > 0)
    {
        if ((THREAD_TYPE_BIND_CORE == m_type) || (THREAD_TYPE_OCCUPY_CORE == m_type))
            clearCpuAffinity(m_threadId);
    }
}

INT4 CThread::init(ROUTINE routine, void* param, std::string owner)
{
    m_routine = routine;
    m_param = param;
    m_owner = owner;

    if ((THREAD_TYPE_PRIOR == m_type) || (THREAD_TYPE_OCCUPY_CORE == m_type))
    {
        maxThreadPriority(m_attr);
    }

    INT4 ret = pthread_create(&m_threadId, &m_attr, routine, param);
    if (ret != 0) 
    {
        LOG_ERROR_FMT("pthread_create failed[%d]!", ret);
        return BNC_ERR;
    }

    if (THREAD_TYPE_OCCUPY_CORE == m_type)
        CCpuCtrl::getInstance()->occupyCpu(owner, m_threadId);

    return BNC_OK;
}


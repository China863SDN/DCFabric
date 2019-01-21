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
*   File Name   : CCond.cpp           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "CCond.h"
#include "CMutex.h"

CCond::CCond()
{
    pthread_cond_init(&m_oCond, NULL);
}

CCond::~CCond()
{
    pthread_cond_destroy(&m_oCond);
}

void CCond::wait(CMutex* mutex)
{
    pthread_cond_wait(&m_oCond, &mutex->m_mutex);
}

INT4 CCond::timedwait(CMutex* mutex, const struct timespec* abstime)
{
    return pthread_cond_timedwait(&m_oCond, &mutex->m_mutex, abstime);
}

void CCond::signal()
{
    pthread_cond_signal(&m_oCond);
}

void CCond::broadcast()
{
    pthread_cond_broadcast(&m_oCond);
}
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
*   File Name   : CRWLock.cpp                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CRWLock.h"

CRWLock::CRWLock()
{
    pthread_rwlock_init(&m_rwlock, NULL);
}

CRWLock::~CRWLock()
{
    pthread_rwlock_destroy(&m_rwlock);
}

int CRWLock::rlock()
{
    return pthread_rwlock_rdlock(&m_rwlock);
}

int CRWLock::wlock()
{
    return pthread_rwlock_wrlock(&m_rwlock);
}

void CRWLock::unlock()
{
    pthread_rwlock_unlock(&m_rwlock);
}

int CRWLock::tryrlock()
{
    return pthread_rwlock_tryrdlock(&m_rwlock);
}

int CRWLock::trywlock()
{
    return pthread_rwlock_trywrlock(&m_rwlock);
}

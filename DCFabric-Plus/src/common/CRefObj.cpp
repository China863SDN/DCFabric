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
*   File Name   : CRefObj.cpp                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CRefObj.h"
#include "CMutex.h"

CRefObj::CRefObj(BOOL isThreadSafe):m_bIsThreadSafe(isThreadSafe),m_pMutex(NULL)
{
    m_iRefCount = 0;
    if (isThreadSafe)
    {
        m_pMutex = new CMutex();
    }
}

CRefObj::~CRefObj()
{
    if (NULL != m_pMutex)
    {
        delete m_pMutex;
    }
}

INT4 CRefObj::GetRefCount() const
{
    return m_iRefCount;
}

INT4 CRefObj::AddRefCount()
{
    if (m_bIsThreadSafe)
    {
        INT4 ret = 0;
        m_pMutex->lock();
        ret = ++m_iRefCount;
        m_pMutex->unlock();

        return ret;
    }

    return ++m_iRefCount;
}

INT4 CRefObj::SubRefCount()
{
    if (m_bIsThreadSafe)
    {
        INT4 ret = 0;
        m_pMutex->lock();
        ret = --m_iRefCount;
        m_pMutex->unlock();

        return ret;
    }

    return --m_iRefCount;
}

void CRefObj::ResetRefCount()
{
    if (m_bIsThreadSafe)
    {
        m_pMutex->lock();
        m_iRefCount = 0;
        m_pMutex->unlock();
    }

    m_iRefCount = 0;
}

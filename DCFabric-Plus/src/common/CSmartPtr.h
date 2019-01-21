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
*   File Name   : CSmartPtr.h                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CSMARTPTR_H
#define __CSMARTPTR_H

#include "bnc-type.h"

/*
 * 智能指针模板类
 */
template <typename T>
class CSmartPtr
{
public:
    typedef void (DelFunc)(T* obj);

public:
    explicit CSmartPtr(T* p = NULL)
    {
        m_pRawObj = p;
        m_delFunc = NULL;
        if (p != NULL)
            p->AddRefCount();
    }

    CSmartPtr(T* p, DelFunc* delFunc)
    {
        m_pRawObj = p;
        m_delFunc = delFunc;
        if (p != NULL)
            p->AddRefCount();
    }

    CSmartPtr(const CSmartPtr& ref)
    {
        m_pRawObj = ref.m_pRawObj;
        m_delFunc = ref.m_delFunc;
        if (m_pRawObj != NULL)
            m_pRawObj->AddRefCount();
    }

    ~CSmartPtr()
    {
        if (m_pRawObj != NULL && m_pRawObj->SubRefCount() == 0)
        {
            if (m_delFunc != NULL)
                m_delFunc(m_pRawObj);
            else
                delete m_pRawObj;
        }
        m_pRawObj = NULL;
        m_delFunc = NULL;
    }

    T* operator->() const
    {
        return m_pRawObj;
    }

    T& operator()() const
    {
        assert(m_pRawObj != NULL);
        return *m_pRawObj;
    }

    T& operator*() const
    {
        assert(m_pRawObj != NULL);
        return *m_pRawObj;
    }

    T* getPtr() const
    {
        return m_pRawObj;
    }

    bool isNull() const
    {
        return (m_pRawObj == NULL);
    }

    bool isNotNull() const
    {
        return (m_pRawObj != NULL);
    }

    CSmartPtr& operator=(const CSmartPtr& ref)
    {
        if (this != &ref)
        {
            if (m_pRawObj != NULL && m_pRawObj->SubRefCount() == 0)
            {
                if (m_delFunc != NULL)
                    m_delFunc(m_pRawObj);
                else
                    delete m_pRawObj;
            }

            m_pRawObj = ref.m_pRawObj;
            m_delFunc = ref.m_delFunc;
            if (m_pRawObj != NULL)
                m_pRawObj->AddRefCount();
        }

        return *this;
    }

    bool operator==(const CSmartPtr& ref) const
    {
        return (m_pRawObj == ref.m_pRawObj);
    }
	
	bool operator!=(const CSmartPtr& ref) const
    {
        return (m_pRawObj != ref.m_pRawObj);
    }

private:
    T* m_pRawObj;
    DelFunc* m_delFunc;
};

#endif

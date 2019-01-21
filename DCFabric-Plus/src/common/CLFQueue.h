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
*   File Name   : CLFQueue.h           *
*   Author      : bnc xflu           *
*   Create Date : 2016-8-4           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef _CLFQUEUE_H
#define _CLFQUEUE_H

#include "bnc-type.h"
#include "CRefObj.h"
#include "concurrent_queue.h"

/*
 * TBB提供的一种无锁队列
 * 来自Intel并行计算库
 */
template<class T>
class CLFQueue: public CRefObj
{
public:
    CLFQueue():m_isBusy(FALSE) {}
    CLFQueue(const CLFQueue& rhs):m_isBusy(FALSE),m_queue(rhs.m_queue) {}
    ~CLFQueue() {clear();}

    void push(const T& x) {m_queue.push(x);}
    void pop(T& x) {m_queue.try_pop(x);}
    BOOL empty() {return m_queue.empty();}
    size_t size() {return m_queue.size();}
    void clear() {m_queue.clear();}

    BOOL isBusy() {return m_isBusy;}
    void setBusy(BOOL busy) {m_isBusy = busy;}

    CLFQueue& operator=(const CLFQueue& rhs) 
    {
        m_isBusy = FALSE;
        new (&m_queue) tbb::concurrent_bounded_queue<T>(rhs.m_queue);
        return *this;
    }

private:
    BOOL m_isBusy;
    tbb::concurrent_bounded_queue<T> m_queue;
};

#endif





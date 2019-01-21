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
*   File Name   : CTagMgr.cpp               *
*   Author      : bnc xflu                  *
*   Create Date : 2016-8-31                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CTagMgr.h"
#include "log.h"
#include "bnc-error.h"
#include "CClusterSync.h"

CTagMgr::CTagMgr():m_start(0),m_end(0)
{
}

CTagMgr::~CTagMgr()
{
}

INT4 CTagMgr::init()
{
    //暂时写死了, 从配置文件中读取? 或者作为参数传入?
    m_start = 100;
    m_end = 4096;
#if 0
    m_list.resize(m_end - m_start);
#else
    new (&m_bitset) CBitset(m_end - m_start + 1);
#endif

    return BNC_OK;
}

UINT4 CTagMgr::alloc()
{
    UINT4 tag = invalid_tag;

    m_mutex.lock();

#if 0
    for (UINT4 index = 0; index < m_end - m_start; ++index)
    {
        if (0 == m_list[index])
        {
            tag = index + m_start;
            m_list[index] = 1;
            break;
        }
    }
#else
    if (!m_bitset.all())
    {
        UINT4 pos = m_bitset.get();
        if (invalid_pos != pos)
        {
            m_bitset.set(pos);
            tag = m_start + pos;
            CClusterSync::syncTagInfo();
        }
    }
#endif

    m_mutex.unlock();

    return tag;
}

void CTagMgr::release(UINT4 tag)
{
    if ((tag < m_start) || (tag > m_end))
    {
        LOG_INFO_FMT("tag[%u] out of range.", tag);
        return;
    }

    m_mutex.lock();

#if 0
    UINT4 index = tag - m_start;
    m_list[index] = 0;
#else
    m_bitset.reset(tag - m_start);
    CClusterSync::syncTagInfo();
#endif

    m_mutex.unlock();
}

UINT2 CTagMgr::serialize(UINT1* data)
{
    return m_bitset.serialize(data);
}

INT4 CTagMgr::recover(const UINT1* data, UINT2 len)
{
    m_bitset.recover(data, len);

    return BNC_OK;
}


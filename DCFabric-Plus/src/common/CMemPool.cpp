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
*   File Name   : CMemPool.cpp                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CMemPool.h"
#include "bnc-error.h"
#include "log.h"
#include "jemalloc.h"
#include "comm-util.h"

CMemEntry::CMemEntry(BOOL isThreadSafe):m_isThreadSafe(isThreadSafe),m_size(0),m_count(0),m_head(NULL),m_tail(NULL)
{
}

CMemEntry::~CMemEntry()
{
    STL_FOR_LOOP(m_buffers, it)
        free(*it);
    m_size = 0;
    m_count = 0;
    m_head = NULL;
    m_tail = NULL;
}

INT4 CMemEntry::init(UINT4 size, UINT4 count, const INT1* owner)
{
    if ((0 == size) || (0 == count))
        return BNC_ERR;

    m_size = size;
    m_count = count;
    m_owner = owner;

    UINT4 total = (m_count * (sizeof(mem_node_t) + m_size));
    INT1* buffer = (INT1*)malloc(total);
    assert(NULL != buffer);
    m_buffers.push_back(buffer);

    INT1* ptr = buffer;
    m_head = (p_mem_node_t)ptr;

    for (UINT4 i = 0; i < m_count-1; ++i)
    {
        m_tail = (p_mem_node_t)ptr;
        ptr += (sizeof(mem_node_t) + m_size);
        m_tail->next = (p_mem_node_t)ptr;
        m_tail->size = m_size;
    }

    m_tail = (p_mem_node_t)ptr;
    m_tail->next = NULL;
    m_tail->size = m_size;

    assert((NULL != m_head) && (NULL != m_tail));

    LOG_WARN_FMT(">>> %s: CMemEntry initiated with total[%lu]size[%u]count[%u]buffer[%p]head[%p]tail[%p] <<<", 
        m_owner.c_str(), (INT1*)m_tail-buffer+(sizeof(mem_node_t) + m_size), m_size, m_count, buffer, m_head, m_tail);
    return BNC_OK;
}

p_mem_node_t CMemEntry::alloc()
{
    assert((NULL != m_head) && (NULL != m_tail));

    p_mem_node_t ret = NULL;

    if (m_isThreadSafe)
        m_mutex.lock();

    if (m_head != m_tail)
    {
        ret = m_head;
        m_head = m_head->next;
        assert(NULL != m_head);
    }
    else
    {
        LOG_WARN_FMT("!!! %s: CMemEntry preallocated %u nodes used out, will realloc %u nodes !!!",
            m_owner.c_str(), m_count, m_count/10);

        UINT4 count = m_count / 10;
        UINT4 total = (count * (sizeof(mem_node_t) + m_size));
        INT1* buffer = (INT1*)malloc(total);
        if (NULL == buffer)
        {
            LOG_ERROR_FMT("!!! %s: CMemEntry realloc %u bytes failed !!!", m_owner.c_str(), total);
            if (m_isThreadSafe)
                m_mutex.unlock();
            return ret;
        }

        m_count += count;
        m_buffers.push_back(buffer);
        
        if (!m_isThreadSafe)
            m_mutex.lock();

        m_tail->next = (p_mem_node_t)buffer;

        for (UINT4 i = 0; i < count-1; ++i)
        {
            m_tail = (p_mem_node_t)buffer;
            buffer += (sizeof(mem_node_t) + m_size);
            m_tail->next = (p_mem_node_t)buffer;
            m_tail->size = m_size;
        }
        
        m_tail = (p_mem_node_t)buffer;
        m_tail->next = NULL;
        m_tail->size = m_size;
        
        if (!m_isThreadSafe)
            m_mutex.unlock();

        ret = m_head;
        m_head = m_head->next;
        assert(NULL != m_head);
    }

    if (m_isThreadSafe)
        m_mutex.unlock();

    return ret;
}

void CMemEntry::release(p_mem_node_t node)
{
    assert(NULL != m_tail);

    if (NULL != node)
    {
        assert(node->size == m_size);
        node->next = NULL;

        m_mutex.lock();
        
        m_tail->next = node;
        m_tail = node;
        
        m_mutex.unlock();
    }
}

CMemPool::CMemPool()
{
}

CMemPool::~CMemPool()
{
}

INT4 CMemPool::init(const INT1* owner)
{
    m_byte8Pool.init(byte8_mem_node_size, byte8_mem_node_count, owner);
    m_byte32Pool.init(byte32_mem_node_size, byte32_mem_node_count, owner);
    m_byte128Pool.init(byte128_mem_node_size, byte128_mem_node_count, owner);
    m_byte256Pool.init(byte256_mem_node_size, byte256_mem_node_count, owner);
    m_byte512Pool.init(byte512_mem_node_size, byte512_mem_node_count, owner);
    m_byte1072Pool.init(byte1072_mem_node_size, byte1072_mem_node_count, owner);
    m_byte3152Pool.init(byte3152_mem_node_size, byte3152_mem_node_count, owner);
    m_byte6112Pool.init(byte6112_mem_node_size, byte6112_mem_node_count, owner);
    m_owner = owner;

    return BNC_OK;
}

void* CMemPool::alloc(UINT4 size)
{
    p_mem_node_t node = NULL;
    
    if (size <= byte8_mem_node_size)
        node = m_byte8Pool.alloc();
    else if (size <= byte32_mem_node_size)
        node = m_byte32Pool.alloc();
    else if (size <= byte128_mem_node_size)
        node = m_byte128Pool.alloc();
    else if (size <= byte256_mem_node_size)
        node = m_byte256Pool.alloc();
    else if (size <= byte512_mem_node_size)
        node = m_byte512Pool.alloc();
    else if (size <= byte1072_mem_node_size)
        node = m_byte1072Pool.alloc();
    else if (size <= byte3152_mem_node_size)
        node = m_byte3152Pool.alloc();
    else if (size <= byte6112_mem_node_size)
        node = m_byte6112Pool.alloc();
    else
    {
        node = (p_mem_node_t)malloc(sizeof(mem_node_t) + size);
        if (NULL == node)
        {
            LOG_ERROR_FMT("!!! %s: CMemPool malloc %lu bytes failed !!!", 
                m_owner.c_str(), sizeof(mem_node_t) + size);
            return NULL;
        }
        node->size = size;
    }

    return (NULL != node) ? node->data : NULL;
}

void CMemPool::release(void* data)
{
    if (NULL != data)
    {
        p_mem_node_t node = (p_mem_node_t)((INT1*)data - sizeof(mem_node_t));
        switch (node->size)
        {
            case byte8_mem_node_size:
                m_byte8Pool.release(node);
                break;
            case byte32_mem_node_size:
                m_byte32Pool.release(node);
                break;
            case byte128_mem_node_size:
                m_byte128Pool.release(node);
                break;
            case byte256_mem_node_size:
                m_byte256Pool.release(node);
                break;
            case byte512_mem_node_size:
                m_byte512Pool.release(node);
                break;
            case byte1072_mem_node_size:
                m_byte1072Pool.release(node);
                break;
            case byte3152_mem_node_size:
                m_byte3152Pool.release(node);
                break;
            case byte6112_mem_node_size:
                m_byte6112Pool.release(node);
                break;
            default:
                free(node);
                break;
        }
    }
}


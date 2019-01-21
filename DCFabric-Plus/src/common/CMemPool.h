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
*   File Name   : CMemPool.h                                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMEMPOOL_H
#define __CMEMPOOL_H

#include "bnc-type.h"
#include "CMutex.h"

typedef struct mem_node_s
{
    struct mem_node_s* next;
    UINT4 size;
    INT1 pad[4];
    INT1 data[0];
}mem_node_t, *p_mem_node_t;

class CMemEntry
{
public:
    CMemEntry(BOOL isThreadSafe=FALSE);
    ~CMemEntry();
    INT4 init(UINT4 size, UINT4 count, const INT1* owner);

    p_mem_node_t alloc();
    void release(p_mem_node_t node);

private:
    BOOL m_isThreadSafe;
    UINT4 m_size;
    UINT4 m_count;
    std::list<INT1*> m_buffers;
    p_mem_node_t m_head;
    p_mem_node_t m_tail;
    CMutex m_mutex;
    std::string m_owner;
};

/*
 * 内存池
 */
class CMemPool
{
public:
    CMemPool();
    ~CMemPool();
    INT4 init(const INT1* owner);

    void* alloc(UINT4 size);
    void release(void* data);

private:
    static const UINT4 byte8_mem_node_size = 8;
    static const UINT4 byte32_mem_node_size = 32;
    static const UINT4 byte128_mem_node_size = 128;
    static const UINT4 byte256_mem_node_size = 256;
    static const UINT4 byte512_mem_node_size = 512;
    static const UINT4 byte1072_mem_node_size = 1072;
    static const UINT4 byte3152_mem_node_size = 3152;
    static const UINT4 byte6112_mem_node_size = 6112;
    static const UINT4 byte8_mem_node_count = 100000;
    static const UINT4 byte32_mem_node_count = 100000;
    static const UINT4 byte128_mem_node_count = 1000000;
    static const UINT4 byte256_mem_node_count = 1000000;
    static const UINT4 byte512_mem_node_count = 100000;
    static const UINT4 byte1072_mem_node_count = 10000;
    static const UINT4 byte3152_mem_node_count = 10000;
    static const UINT4 byte6112_mem_node_count = 10000;

    CMemEntry m_byte8Pool;
    CMemEntry m_byte32Pool;
    CMemEntry m_byte128Pool;
    CMemEntry m_byte256Pool;
    CMemEntry m_byte512Pool;
    CMemEntry m_byte1072Pool;
    CMemEntry m_byte3152Pool;
    CMemEntry m_byte6112Pool;
    std::string m_owner;
};


#endif

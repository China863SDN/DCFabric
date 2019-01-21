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
*   File Name   : CBitset.cpp                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <bitset>
#include "CBitset.h"
#include "log.h"

#define U32_COUNT(u32)     ((u32 + 31) / 32)
#define U32_QUOTIENT(u32)  ((u32) / 32)
#define U32_REMAINDER(u32) ((u32) % 32)
#define U32_MASK(u32)      (0x80000000 >> U32_REMAINDER(u32))

#if 1
CBitset::CBitset():m_length(0),m_numSet(0),m_next(0),m_data(NULL)
{
}

CBitset::CBitset(UINT4 len)
{
    m_length = len;
    m_numSet = 0;
    m_next = 0;

    m_data = (UINT4*)malloc(4*U32_COUNT(len));
    if (NULL != m_data)
    {
        memset(m_data, 0, 4*U32_COUNT(len));
        if (U32_REMAINDER(len) != 0)
        {
            //set the left bits in last UINT4 to 1.
            m_data[U32_QUOTIENT(len)] &= (1 << (32 - U32_REMAINDER(len))) - 1;
        }
    }
}

CBitset::~CBitset()
{
    if (NULL != m_data)
    {
        free(m_data);
        m_data = NULL;
    }
}

BOOL CBitset::test(UINT4 pos)
{
    return ((NULL != m_data) && 
            (pos < m_length) && 
            ((m_data[U32_QUOTIENT(pos)] & U32_MASK(pos)) != 0));
}

UINT4 CBitset::get()
{
    for (UINT4 pos = m_next; pos < m_length; pos++)
        if (!test(pos))
            return pos;

    for (UINT4 pos = 0; pos < m_next; pos++)
        if (!test(pos))
            return pos;

    return invalid_pos;
}

void CBitset::set(UINT4 pos)
{
    if ((pos < m_length) && !test(pos))
    {
        m_data[U32_QUOTIENT(pos)] |= U32_MASK(pos);
        m_numSet++;
        m_next = (pos + 1) % m_length;
    }
}

void CBitset::reset(UINT4 pos)
{
    if ((pos < m_length) && test(pos))
    {
        m_data[U32_QUOTIENT(pos)] &= ~(U32_MASK(pos));
        m_numSet--;
    }
}

BOOL CBitset::all()
{
    return (m_numSet >= m_length);
}

UINT2 CBitset::serialize(UINT1* data)
{
    UINT2 len = 0;

    if (NULL != m_data)
    {
        *(UINT4*)data = m_length;
        data += sizeof(UINT4);
        len += sizeof(UINT4);

        *(UINT4*)data = m_numSet;
        data += sizeof(UINT4);
        len += sizeof(UINT4);

        *(UINT4*)data = m_next;
        data += sizeof(UINT4);
        len += sizeof(UINT4);

        memcpy(data, m_data, 4*U32_COUNT(m_length));
        len += 4*U32_COUNT(m_length);
    }

    return len;
}

void CBitset::recover(const UINT1* data, UINT2 len)
{
    UINT4 syncLen = *(UINT4*)data;
    if (3*sizeof(UINT4) + 4*U32_COUNT(syncLen) > len)
    {
        LOG_WARN("destroyed recovery data !");
        return;
    }

    if ((syncLen == m_length) && (NULL != m_data))
    {
        data += sizeof(UINT4);
        m_numSet = *(UINT4*)data;
        data += sizeof(UINT4);
        m_next = *(UINT4*)data;
        data += sizeof(UINT4);
        memcpy(m_data, data, 4*U32_COUNT(syncLen));
    }
    else
    {
        m_length = syncLen;
        data += sizeof(UINT4);
        m_numSet = *(UINT4*)data;
        data += sizeof(UINT4);
        m_next = *(UINT4*)data;
        data += sizeof(UINT4);
        if (NULL != m_data)
            free(m_data);
        m_data = (UINT4*)malloc(4*U32_COUNT(syncLen));
        if (NULL != m_data)
            memcpy(m_data, data, 4*U32_COUNT(syncLen));
    }
}

#else
#define BITSET_SIZE_U8      (1L << 8)
#define BITSET_SIZE_U12     (1L << 12)
#define BITSET_SIZE_U16     (1L << 16)
#define BITSET_SIZE_U20     (1L << 20)
#define BITSET_SIZE_U24     (1L << 24)
#define BITSET_SIZE_U28     (1L << 28)
#define BITSET_SIZE_U32     (1L << 32)

CBitset::CBitset(UINT4 size, UINT4 val):m_size(size),m_next(0)
{
    if (m_size <= BITSET_SIZE_U8)
        m_bitset = new std::bitset<BITSET_SIZE_U8>(val);
    else if (m_size <= BITSET_SIZE_U12)
        m_bitset = new std::bitset<BITSET_SIZE_U12>(val);
    else if (m_size <= BITSET_SIZE_U16)
        m_bitset = new std::bitset<BITSET_SIZE_U16>(val);
    else if (m_size <= BITSET_SIZE_U24)
        m_bitset = new std::bitset<BITSET_SIZE_U24>(val);
    else if (m_size <= BITSET_SIZE_U32)
        m_bitset = new std::bitset<BITSET_SIZE_U32>(val);

    if (NULL == m_bitset)
        LOG_ERROR_FMT("new std::bitset with size[%lu]val[%u] failed !", size, val);
}

CBitset::~CBitset()
{
    if (NULL != m_bitset)
    {
        if (m_size <= BITSET_SIZE_U8)
            delete (std::bitset<BITSET_SIZE_U8>*)m_bitset;
        else if (m_size <= BITSET_SIZE_U12)
            delete (std::bitset<BITSET_SIZE_U12>*)m_bitset;
        else if (m_size <= BITSET_SIZE_U16)
            delete (std::bitset<BITSET_SIZE_U16>*)m_bitset;
        else if (m_size <= BITSET_SIZE_U24)
            delete (std::bitset<BITSET_SIZE_U24>*)m_bitset;
        else if (m_size <= BITSET_SIZE_U32)
            delete (std::bitset<BITSET_SIZE_U32>*)m_bitset;
        m_bitset = NULL;
    }
}

BOOL CBitset::test(UINT4 pos)
{
    if (NULL != m_bitset)
    {
        if (m_size <= BITSET_SIZE_U8)
            return ((std::bitset<BITSET_SIZE_U8>*)m_bitset)->test(pos);
        else if (m_size <= BITSET_SIZE_U12)
            return ((std::bitset<BITSET_SIZE_U12>*)m_bitset)->test(pos);
        else if (m_size <= BITSET_SIZE_U16)
            return ((std::bitset<BITSET_SIZE_U16>*)m_bitset)->test(pos);
        else if (m_size <= BITSET_SIZE_U24)
            return ((std::bitset<BITSET_SIZE_U24>*)m_bitset)->test(pos);
        else if (m_size <= BITSET_SIZE_U32)
            return ((std::bitset<BITSET_SIZE_U32>*)m_bitset)->test(pos);
    }

    return FALSE;
}

UINT4 CBitset::get()
{
    if (NULL != m_bitset)
    {
        if (m_size <= BITSET_SIZE_U8)
            return getU8();
        else if (m_size <= BITSET_SIZE_U12)
            return getU12();
        else if (m_size <= BITSET_SIZE_U16)
            return getU16();
        else if (m_size <= BITSET_SIZE_U24)
            return getU24();
        else if (m_size <= BITSET_SIZE_U32)
            return getU32();
    }

    return invalid_pos;
}

void CBitset::set(UINT4 pos)
{
    if (NULL != m_bitset)
    {
        if (m_size <= BITSET_SIZE_U8)
            setU8(pos);
        else if (m_size <= BITSET_SIZE_U12)
            setU12(pos);
        else if (m_size <= BITSET_SIZE_U16)
            setU16(pos);
        else if (m_size <= BITSET_SIZE_U24)
            setU24(pos);
        else if (m_size <= BITSET_SIZE_U32)
            setU32(pos);
    }
}

void CBitset::reset(UINT4 pos)
{
    if (NULL != m_bitset)
    {
        if (m_size <= BITSET_SIZE_U8)
            ((std::bitset<BITSET_SIZE_U8>*)m_bitset)->reset(pos);
        else if (m_size <= BITSET_SIZE_U12)
            ((std::bitset<BITSET_SIZE_U12>*)m_bitset)->reset(pos);
        else if (m_size <= BITSET_SIZE_U16)
            ((std::bitset<BITSET_SIZE_U16>*)m_bitset)->reset(pos);
        else if (m_size <= BITSET_SIZE_U24)
            ((std::bitset<BITSET_SIZE_U24>*)m_bitset)->reset(pos);
        else if (m_size <= BITSET_SIZE_U32)
            ((std::bitset<BITSET_SIZE_U32>*)m_bitset)->reset(pos);
    }
}

BOOL CBitset::all()
{
    if (NULL != m_bitset)
    {
        if (m_size <= BITSET_SIZE_U8)
            return ((std::bitset<BITSET_SIZE_U8>*)m_bitset)->all();
        else if (m_size <= BITSET_SIZE_U12)
            return ((std::bitset<BITSET_SIZE_U12>*)m_bitset)->all();
        else if (m_size <= BITSET_SIZE_U16)
            return ((std::bitset<BITSET_SIZE_U16>*)m_bitset)->all();
        else if (m_size <= BITSET_SIZE_U24)
            return ((std::bitset<BITSET_SIZE_U24>*)m_bitset)->all();
        else if (m_size <= BITSET_SIZE_U32)
            return ((std::bitset<BITSET_SIZE_U32>*)m_bitset)->all();
    }

    return FALSE;
}

UINT4 CBitset::size()
{
    if (NULL != m_bitset)
    {
        if (m_size <= BITSET_SIZE_U8)
            return ((std::bitset<BITSET_SIZE_U8>*)m_bitset)->size();
        else if (m_size <= BITSET_SIZE_U12)
            return ((std::bitset<BITSET_SIZE_U12>*)m_bitset)->size();
        else if (m_size <= BITSET_SIZE_U16)
            return ((std::bitset<BITSET_SIZE_U16>*)m_bitset)->size();
        else if (m_size <= BITSET_SIZE_U24)
            return ((std::bitset<BITSET_SIZE_U24>*)m_bitset)->size();
        else if (m_size <= BITSET_SIZE_U32)
            return ((std::bitset<BITSET_SIZE_U32>*)m_bitset)->size();
    }

    return 0;
}

UINT8 CBitset::toVal()
{
    if (NULL != m_bitset)
    {
        if (m_size <= BITSET_SIZE_U8)
            return ((std::bitset<BITSET_SIZE_U8>*)m_bitset)->to_ulong();
        else if (m_size <= BITSET_SIZE_U12)
            return ((std::bitset<BITSET_SIZE_U12>*)m_bitset)->to_ulong();
        else if (m_size <= BITSET_SIZE_U16)
            return ((std::bitset<BITSET_SIZE_U16>*)m_bitset)->to_ulong();
        else if (m_size <= BITSET_SIZE_U24)
            return ((std::bitset<BITSET_SIZE_U24>*)m_bitset)->to_ulong();
        else if (m_size <= BITSET_SIZE_U32)
            return ((std::bitset<BITSET_SIZE_U32>*)m_bitset)->to_ulong();
    }

    return 0;
}

void CBitset::recover(UINT8 val)
{
    if (m_size <= BITSET_SIZE_U8)
        new (m_bitset) std::bitset<BITSET_SIZE_U8>(val);
    else if (m_size <= BITSET_SIZE_U12)
        new (m_bitset) std::bitset<BITSET_SIZE_U12>(val);
    else if (m_size <= BITSET_SIZE_U16)
        new (m_bitset) std::bitset<BITSET_SIZE_U16>(val);
    else if (m_size <= BITSET_SIZE_U24)
        new (m_bitset) std::bitset<BITSET_SIZE_U24>(val);
    else if (m_size <= BITSET_SIZE_U32)
        new (m_bitset) std::bitset<BITSET_SIZE_U32>(val);
}

UINT4 CBitset::getU8()
{
    std::bitset<BITSET_SIZE_U8>* bitset = (std::bitset<BITSET_SIZE_U8>*)m_bitset;
    if (bitset->all())
        return invalid_pos;
    
    for (UINT4 cur = m_next; cur < m_size; ++cur)
        if (!bitset->test(cur))
            return cur;
    for (UINT4 cur = 0; cur < m_next; ++cur)
        if (!bitset->test(cur))
            return cur;

    return invalid_pos;
}

UINT4 CBitset::getU12()
{
    std::bitset<BITSET_SIZE_U12>* bitset = (std::bitset<BITSET_SIZE_U12>*)m_bitset;
    if (bitset->all())
        return invalid_pos;
    
    for (UINT4 cur = m_next; cur < m_size; ++cur)
        if (!bitset->test(cur))
            return cur;
    for (UINT4 cur = 0; cur < m_next; ++cur)
        if (!bitset->test(cur))
            return cur;

    return invalid_pos;
}

UINT4 CBitset::getU16()
{
    std::bitset<BITSET_SIZE_U16>* bitset = (std::bitset<BITSET_SIZE_U16>*)m_bitset;
    if (bitset->all())
        return invalid_pos;
    
    for (UINT4 cur = m_next; cur < m_size; ++cur)
        if (!bitset->test(cur))
            return cur;
    for (UINT4 cur = 0; cur < m_next; ++cur)
        if (!bitset->test(cur))
            return cur;

    return invalid_pos;
}

UINT4 CBitset::getU24()
{
    std::bitset<BITSET_SIZE_U24>* bitset = (std::bitset<BITSET_SIZE_U24>*)m_bitset;
    if (bitset->all())
        return invalid_pos;
    
    for (UINT4 cur = m_next; cur < m_size; ++cur)
        if (!bitset->test(cur))
            return cur;
    for (UINT4 cur = 0; cur < m_next; ++cur)
        if (!bitset->test(cur))
            return cur;

    return invalid_pos;
}

UINT4 CBitset::getU32()
{
    std::bitset<BITSET_SIZE_U32>* bitset = (std::bitset<BITSET_SIZE_U32>*)m_bitset;
    if (bitset->all())
        return invalid_pos;
    
    for (UINT4 cur = m_next; cur < m_size; ++cur)
        if (!bitset->test(cur))
            return cur;
    for (UINT4 cur = 0; cur < m_next; ++cur)
        if (!bitset->test(cur))
            return cur;

    return invalid_pos;
}

void CBitset::setU8(UINT4 pos)
{
    std::bitset<BITSET_SIZE_U8>* bitset = (std::bitset<BITSET_SIZE_U8>*)m_bitset;
    if (!bitset->test(pos))
    {
        bitset->set(pos);
        m_next = (pos + 1) % m_size;
    }
}

void CBitset::setU12(UINT4 pos)
{
    std::bitset<BITSET_SIZE_U12>* bitset = (std::bitset<BITSET_SIZE_U12>*)m_bitset;
    if (!bitset->test(pos))
    {
        bitset->set(pos);
        m_next = (pos + 1) % m_size;
    }
}

void CBitset::setU16(UINT4 pos)
{
    std::bitset<BITSET_SIZE_U16>* bitset = (std::bitset<BITSET_SIZE_U16>*)m_bitset;
    if (!bitset->test(pos))
    {
        bitset->set(pos);
        m_next = (pos + 1) % m_size;
    }
}

void CBitset::setU24(UINT4 pos)
{
    std::bitset<BITSET_SIZE_U24>* bitset = (std::bitset<BITSET_SIZE_U24>*)m_bitset;
    if (!bitset->test(pos))
    {
        bitset->set(pos);
        m_next = (pos + 1) % m_size;
    }
}

void CBitset::setU32(UINT4 pos)
{
    std::bitset<BITSET_SIZE_U32>* bitset = (std::bitset<BITSET_SIZE_U32>*)m_bitset;
    if (!bitset->test(pos))
    {
        bitset->set(pos);
        m_next = (pos + 1) % m_size;
    }
}
#endif

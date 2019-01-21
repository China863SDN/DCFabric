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
*   File Name   : CBitset.h                                                   *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CBITSET_H
#define __CBITSET_H

#include "bnc-type.h"

static const UINT4 invalid_pos = -1;

/*
 * 位集合
 */
#if 1
class CBitset
{
public:
    CBitset();
    CBitset(UINT4 len);
    ~CBitset();

    BOOL test(UINT4 pos);
    UINT4 get();
    void set(UINT4 pos);
    void reset(UINT4 pos);
    BOOL all();

    //用于主备同步
    UINT2 serialize(UINT1* data);
    void recover(const UINT1* data, UINT2 len);

private:
    UINT4  m_length; //the total bit number
    UINT4  m_numSet; //the bit number have set
    UINT4  m_next;   //bit alloc from this number, next < length.
    UINT4* m_data;
};
#else
class CBitset
{
public:
    static const UINT4 invalid_pos = -1;

public:
    CBitset(UINT4 size, UINT4 val=0);
    ~CBitset();

    BOOL test(UINT4 pos);
    UINT4 get();
    void set(UINT4 pos);
    void reset(UINT4 pos);
    BOOL all();
    UINT4 size();

    //用于主备同步
    UINT8 toVal();
    void recover(UINT8 val);

private:
    UINT4 getU8();
    UINT4 getU12();
    UINT4 getU16();
    UINT4 getU24();
    UINT4 getU32();
    void setU8(UINT4 pos);
    void setU12(UINT4 pos);
    void setU16(UINT4 pos);
    void setU24(UINT4 pos);
    void setU32(UINT4 pos);

private:
    UINT4 m_size;
    UINT4 m_next;
    void* m_bitset;
};
#endif


#endif

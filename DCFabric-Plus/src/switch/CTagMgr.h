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
*   File Name   : CTagMgr.h                 *
*   Author      : bnc xflu                  *
*   Create Date : 2016-8-31                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CTAGMGR_H
#define _CTAGMGR_H

//#include <vector>
#include "bnc-type.h"
#include "CBitset.h"
#include "CMutex.h"

class CTagMgr
{
public:
    static const UINT4 invalid_tag = 0;

public:
    CTagMgr();
    ~CTagMgr();

    INT4 init();
    UINT4 alloc();
    void release(UINT4 tag);

    //用于主备同步
    UINT2 serialize(UINT1* data);
    INT4 recover(const UINT1* data, UINT2 len);

private:
#if 0
    std::vector<UINT4> m_list;
#else
    CBitset m_bitset;
#endif
    UINT4   m_start;
    UINT4   m_end;
    CMutex  m_mutex;
};





#endif

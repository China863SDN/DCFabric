/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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
*   File Name   : overload-mgr.h           *
*   Author      : bnc Administrator           *
*   Create Date : 2016-04-26           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef OVERLOAD_MGR_H_
#define OVERLOAD_MGR_H_
#include "common.h"
#include "gnflush-types.h"


typedef struct msg_counter
{
    gn_switch_t* sw;
    UINT4 ip;
    UINT1 mac[6];
    UINT4 count;
    UINT4 start;
    UINT4 stop;
    BOOL  flag;
    UINT8 timestamp;
    
    struct msg_counter* next;
}msg_counter_t;

void add_msg_counter(gn_switch_t* sw, UINT4 ip, const UINT1* mac);
INT4 init_overload_mgr();

#endif


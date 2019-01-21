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
*   File Name   : CSwitchSimulator.h                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CSWITCHSIMULATOR_H
#define _CSWITCHSIMULATOR_H

#include "CTimer.h"

extern UINT4 g_ctrlIp;
extern UINT2 g_ctrlPort;
extern INT4 g_proto;
extern INT4 g_switchNumber;
extern UINT4 g_hostNumber;
extern UINT4 g_hostStartIp;

typedef enum
{
    TEST_PROTO_NONE,
    TEST_PROTO_HELLO,
    TEST_PROTO_PKTIN_IP,
    TEST_PROTO_PKTIN_ARP_REQ,
    //...
    TEST_PROTO_INVALID
} test_proto_e;

class CSwitchSimulator
{
public:
    static void test(void* param);

public:
    CSwitchSimulator();
    ~CSwitchSimulator();

    void start();
    const char* toString() {return "CSwitchSimulator";}

private:
    std::vector<INT4> m_sockfds;
    std::vector<CTimer> m_timers;
};


#endif

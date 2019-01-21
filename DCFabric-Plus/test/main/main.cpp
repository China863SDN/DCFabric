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
*   File Name   : CMsgTreeNode.h                                              *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CSwitchSimulator.h"

static void usage()
{
    printf("./trafficGenerator <ip:port> <proto> <switch-number> <host-number> <host-start-ip>\n");
    printf("    proto: 1(hello) 2(packet-in ip) 3(packet-in arp req)\n");
}

int main(int argc, char** argv)
{	
    if (argc < 6)
    {
        usage();
        return -1;
    }

    char* ip = strchr(argv[1], ':');
    if (ip == NULL)
    {
        usage();
        return -1;
    }

    *ip = 0;
    g_ctrlIp = inet_addr(argv[1]);
    g_ctrlPort = atoi(ip+1);
    g_proto = atoi(argv[2]);
    g_switchNumber = atoi(argv[3]);
    if (g_switchNumber == 0)
        g_switchNumber = 1;
    g_hostNumber = atoi(argv[4]);
    if (g_hostNumber == 0)
        g_hostNumber = 1;
    g_hostStartIp = atoi(argv[5]);
    if (g_hostStartIp == 0)
        g_hostStartIp = 1;

    CSwitchSimulator switchSimulator;
    switchSimulator.start();

    while (1)
    {
        sleep(30);
    }

    return 0;
}

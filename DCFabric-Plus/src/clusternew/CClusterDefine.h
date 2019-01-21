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
*   File Name   : CClusterDefine.h                                            *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#ifndef __CCLUSTERDEFINE_H
#define __CCLUSTERDEFINE_H

#include "bnc-type.h"

typedef enum
{
	CLUSTER_ELECTION_INIT = 0,
	CLUSTER_ELECTION_START = 1,
	CLUSTER_ELECTION_ONGOING = 2,
	CLUSTER_ELECTION_WATCHING = 3,
	CLUSTER_ELECTION_FINISHED = 4
}cluster_election_state;

typedef enum
{
	CLUSTER_CONTROLLER_INIT = 0,
	CLUSTER_CONTROLLER_CONNECTED = 1,
	CLUSTER_CONTROLLER_NORESPONSE = 2,
	CLUSTER_CONTROLLER_UNREACHABLE = 3,
	CLUSTER_CONTROLLER_DISCONNECTED = 4
}cluster_controller_state;

typedef struct cluster_controller_s
{
    INT4  sockfd;
	UINT4 ip;
	UINT2 port;
    INT4  state;
    INT4  karetry;
}cluster_controller_t;

#endif /* __CCLUSTERDEFINE_H */

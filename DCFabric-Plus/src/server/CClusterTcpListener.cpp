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
*   File Name   : CClusterTcpListener.cpp                                     *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CClusterTcpListener.h"
#include "bnc-error.h"
#include "CClusterService.h"

CClusterTcpListener::CClusterTcpListener()
{
}

CClusterTcpListener::CClusterTcpListener(UINT4 ip, UINT2 port):
    CTcpListener(ip, port)
{
}

CClusterTcpListener::~CClusterTcpListener()
{
}

INT4 CClusterTcpListener::process(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port)
{
    return CClusterService::getInstance()->remoteConnect(sockfd, ip, port);
}


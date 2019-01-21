/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
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
*   File Name   : CClusterKeepalive.h                                         *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#ifndef __CCLUSTERKEEPALIVE_H
#define __CCLUSTERKEEPALIVE_H

#include "bnc-type.h"
#include "CClusterInterface.h"
#include "CTimer.h"

class CClusterKeepalive
{
public:
    static INT4 init();

    static INT4 processReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 sendReq(INT4 sockfd);

private:
    static INT4 sendRsp(INT4 sockfd);
    static void keepalive(void* param);

private:
    const static INT4 keepalive_interval = 5;
    const static INT4 keepalive_retry_times = 6;

    static CTimer m_timer;
};

#endif /* __CCLUSTERKEEPALIVE_H */

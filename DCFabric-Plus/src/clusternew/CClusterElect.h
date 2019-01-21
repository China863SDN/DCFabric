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
*   File Name   : CClusterElect.h                                             *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#ifndef __CCLUSTERELECT_H
#define __CCLUSTERELECT_H

#include "bnc-type.h"
#include "CClusterInterface.h"
#include "CTimer.h"
#include "CMutex.h"

class CClusterElection
{
public:
    static INT4 start();
    static void terminate(void* param);

    static INT4  getState() {return m_state;}
    static UINT4 getMasterIp() {return m_masterIp;}

    static INT4 processHelloReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processHelloRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processProposal(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processNotify(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processInform(INT4 sockfd, cluster_interface_t* itf);
    static INT4 sendHelloReq(INT4 sockfd);

private:
    static BOOL  checkRemoteDisconnected(UINT4 ip, INT4 sockfd);
    static UINT4 generateProposal0();
    static UINT4 generateProposal();
    static INT4  broadcastProposal(UINT4 ip);
    static INT4  broadcastNotify();
    static INT4  broadcastInform();
    static INT4  sendHelloRsp(INT4 sockfd);
    static INT4  sendProposal(INT4 sockfd, UINT4 ip);
    static INT4  sendNotify(INT4 sockfd);
    static INT4  sendInform(INT4 sockfd);

private:
    const static INT4 ELECTION_TIMEOUT_SECOND = 1;

    static INT4   m_state;
    static UINT4  m_masterIp;
    static CTimer m_timer;
    static CMutex m_mutex;
};

#endif /* __CCLUSTERELECT_H */

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
*   File Name   : COvsdbTcpListener.cpp                                       *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "log.h"
#include "COvsdbTcpListener.h"
#include "COvsdbMgr.h"
#include "CServer.h"
#include "comm-util.h"

COvsdbTcpListener::COvsdbTcpListener(UINT4 ip, UINT2 port):
    CTcpListener(ip, port)
{
}


COvsdbTcpListener::~COvsdbTcpListener()
{
}

INT4 COvsdbTcpListener::process(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port)
{
    if (COvsdbMgr::getInstance()->addClient(sockfd, mac, ip, port) != BNC_OK)
    {
        LOG_ERROR_FMT("%s addClient with sockfd[%d]ip[%s]port[%u] failed !", 
            toString(), sockfd, inet_htoa(ip), port);
        close(sockfd);
        return BNC_ERR;
    }
    
    CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
    if (worker.isNotNull())
    {
        if (worker->addConnFd(sockfd) != BNC_OK)
        {
            LOG_ERROR_FMT("%s addConnFd with sockfd[%d] failed !", toString(), sockfd);
            close(sockfd);
            return BNC_ERR;
        }
    }

    return BNC_OK;
}


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
*   File Name   : COfTcpListener.cpp                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COfTcpListener.h"
#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"
#include "CControl.h"
#include "COfRecvWorker.h"
#include "CServer.h"

COfTcpListener::COfTcpListener(UINT4 ip, UINT2 port):
    CTcpListener(ip, port/*, THREAD_TYPE_OCCUPY_CORE*/)
{
}


COfTcpListener::~COfTcpListener()
{
}

INT4 COfTcpListener::process(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port)
{
    //为新的交换机连接创建存储
    if (CControl::getInstance()->getSwitchMgr().addSwitch(sockfd, mac, ip, port) != BNC_OK)
    {
        LOG_ERROR_FMT("%s addSwitch with sockfd[%d] failed !", toString(), sockfd);
        close(sockfd);
        return BNC_ERR;
    }
    
    //为新的交换机连接sockfd加到相应COfRecvWorker的epoll�?
    CSmartPtr<COfRecvWorker> worker = CServer::getInstance()->mapOfRecvWorker(sockfd);
    if (worker.isNotNull())
    {
        if (worker->addConnFd(sockfd) != BNC_OK)
        {
            LOG_ERROR_FMT("%s addConnFd with sockfd[%d] failed !", toString(), sockfd);
            CControl::getInstance()->getSwitchMgr().delSwitch(sockfd);
            close(sockfd);
            return BNC_ERR;
        }
    }

    return BNC_OK;
}


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
*   File Name   : CTcpListener.cpp                                            *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "CTcpListener.h"
#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"
#include "CConf.h"
#include "CServer.h"

static void* start(void* param)
{
    if (NULL == param)
    {
        LOG_ERROR("start TCP listener failed, null param!");
        return NULL;
    }

    CTcpListener* pListener = (CTcpListener*)param;

    LOG_WARN_FMT("%s start pid[%d]threadId[%llu] ...", 
        pListener->toString(), getpid(), (UINT8)pthread_self());

	prctl(PR_SET_NAME, (unsigned long)pListener->toString());  

    pListener->accept();

    return NULL;
}

CTcpListener::CTcpListener():
    m_uiIp(0),
    m_uiPort(0),
    m_iSockfd(-1)
{
}

CTcpListener::CTcpListener(UINT4 ip, UINT2 port, INT4 type):
    m_uiIp(ip),
    m_uiPort(port),
    m_iSockfd(-1),
    m_thread(type)
{
}

CTcpListener::~CTcpListener()
{
}

INT4 CTcpListener::init()
{
    //监听端口
    if (this->listen() != BNC_OK)
    {
        LOG_ERROR_FMT("%s listen local port[%d] failed, error no [%d]!",toString(), m_uiPort, errno);
        return BNC_ERR;
    }

    //启动线程
    if (m_thread.init(start, (void*)this, toString()) != BNC_OK)
    {
        LOG_ERROR_FMT("%s init CThread failed[%d]!", toString(), errno);
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CTcpListener::listen()
{
   //创建监听socket
    INT4 sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOG_ERROR_FMT("%s create listen socket failed[%d]!", toString(), errno);
        return BNC_ERR;
    }

    //设置地址可重�?
    INT4 sockopt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (INT1*)&sockopt, sizeof(sockopt));
	//setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&sockopt, sizeof(sockopt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(m_uiPort);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    //绑定监听地址
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        LOG_ERROR_FMT("%s bind failed[%d]!", toString(), errno);
        close(sockfd);
        return BNC_ERR;
    }

    //监听端口
    const INT1* pMax = CConf::getInstance()->getConfig("controller", "max_switch");
    UINT4 max = (NULL == pMax) ? 3000 : atoll(pMax);
    if (::listen(sockfd, max) < 0)
    {
        LOG_ERROR_FMT("%s listen failed[%d]!", toString(), errno);
        close(sockfd);
        return BNC_ERR;
    }

    m_iSockfd = sockfd;

    LOG_WARN_FMT("%s listen on port[%u] success", toString(), m_uiPort);
    return BNC_OK;
}

INT4 CTcpListener::accept()
{
    static UINT4 count = 0;

    INT4 connfd = -1;
	INT4 iErrno = 0;
    struct sockaddr_in client;
    socklen_t addrLen = sizeof(client);

    usleep(500000);

    while (1)
    {
        connfd = ::accept(m_iSockfd, (struct sockaddr*)&client, &addrLen);
		if (connfd <= 0)
		{
			iErrno = errno;
            LOG_ERROR_FMT("%s: accept errno[%d]", toString(), iErrno);
			if (EINTR == iErrno)
			{
				continue;
			}
			break;
		}

		if(*(UINT4 *)&client.sin_addr == 0)
	    {
	        LOG_ERROR_FMT("%s accept a new switch connection failed, socket address invalid!", toString());
	        close(connfd);
            continue;
	    }
        INT1 mac[6] = {0}, macStr[20] = {0};
        UINT4 ip = ntohl(client.sin_addr.s_addr);
        UINT2 port = ntohs(client.sin_port);

        const INT1* itf = CConf::getInstance()->getConfig("controller", "controller_eth");
        getPeerMac(connfd, client, itf, mac);

        if (CConf::getInstance()->isMininet(ip))
            memset(mac, 0, 6);

        ++count;
        LOG_WARN_FMT("--- %s accept number[%u] new connection sockfd[%d] with mac[%s]ip[%s]port[%d] ---", 
            toString(), count, connfd, mac2str((UINT1*)mac, macStr), inet_ntoa(client.sin_addr), port);

        process(connfd, mac, ip, port);
    }

    close(m_iSockfd);
    return BNC_ERR;
}

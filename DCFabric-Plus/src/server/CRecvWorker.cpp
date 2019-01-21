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
*   File Name   : CRecvWorker.cpp                                             *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "CRecvWorker.h"
#include "log.h"
#include "comm-util.h"
#include "CMsg.h"
#include "COfMsgUtil.h"
#include "bnc-error.h"
#include "CServer.h"

static void* work(void* param)
{
    static UINT4 count = 0;

    if (NULL == param)
    {
        return NULL;
    }

    CRecvWorker* worker = (CRecvWorker*)param;

    LOG_WARN_FMT("%s start pid[%d]threadId[%llu] ...", worker->toString(), getpid(), (UINT8)pthread_self());

	prctl(PR_SET_NAME, (unsigned long)worker->toString());  

    INT4 ret = -1;
    epoll_event events[CRecvWorker::max_epoll_files] = {0};
    epoll_event* event = NULL;
    INT4 epollFd = worker->getEpollFd();
    INT4 sockfd = -1;

    while (1)
    {
        try
        {
            if ((ret = epoll_wait(epollFd, events, CRecvWorker::max_epoll_files, -1)) < 0)
            {
                LOG_ERROR_FMT("%s epoll_wait failed !", worker->toString());
                continue;
            }

            for (INT4 i = 0; i < ret; i++)
            {
                event = &(events[i]);
                sockfd = event->data.fd; 
                LOG_INFO_FMT("--- %s epoll_wait on sockfd[%d],events[%d] ---", worker->toString(), sockfd, events[i].events);

                if (event->events & EPOLLRDHUP)
                {
                    ++count;
                    LOG_WARN_FMT("--- %s find number[%u] disconnected sockfd[%d] by EPOLLRDHUP ! ---", worker->toString(), count, sockfd);
                    worker->processPeerDisconn(sockfd);
                    continue;
                }

                if (event->events & EPOLLIN)
                {
                    worker->receive(sockfd);
                }
                else
                {
                    LOG_WARN_FMT("%s epoll_wait error on sockfd[%d],events[%d] !", worker->toString(), sockfd, events[i].events);
                    worker->processPeerDisconn(sockfd);
                }
            }
        }
        catch (...)
        {
            LOG_ERROR_FMT("%s catch exception !", worker->toString());
        }
    }
}


CRecvWorker::CRecvWorker(INT4 type):m_epollFd(-1),m_thread(type)
{
}

CRecvWorker::~CRecvWorker()
{
}

INT4 CRecvWorker::init()
{
    m_epollFd = epoll_create(max_epoll_files);
    if (m_epollFd < 0)
    {
        LOG_ERROR_FMT("%s epoll_create failed[%d]!", toString(), errno);
        return BNC_ERR;
    }

    //启动线程
    if (m_thread.init(work, (void*)this, toString()) != BNC_OK)
    {
        LOG_ERROR_FMT("%s init CThread failed[%d]!", toString(), errno);
        return BNC_ERR;
    }

    LOG_INFO_FMT("init %s success", toString());
    return BNC_OK;
}

INT4 CRecvWorker::addConnFd(INT4 fd)
{
    LOG_INFO_FMT("%s add sockfd[%d], threadId[%llu] ...", toString(), fd, (UINT8)m_thread.getThreadId());
    return addFd(m_epollFd, fd);
}

void CRecvWorker::delConnFd(INT4 fd)
{
    LOG_INFO_FMT("%s del sockfd[%d], threadId[%llu] ...", toString(), fd, (UINT8)m_thread.getThreadId());
    delFd(m_epollFd, fd);
}

void CRecvWorker::receive(INT4 sockfd)
{
    UINT4 len = 0;
    INT4 ret = -1, errnum = 0;
    BOOL error = FALSE;

    while (1)
    {
        try
        {
            ret = recv(sockfd, m_buffer+len, max_recv_buffer_size-len, 0);
            errnum = errno;
            if (ret < 0)
            {
                LOG_INFO_FMT("--- %s recv errno[%d], threadId[%llu] ---", toString(), errnum, (UINT8)pthread_self());
                if (EINTR == errnum)
                    continue;

                if ((errnum == EAGAIN) || (errnum == EWOULDBLOCK))
                {
                    process(sockfd, m_buffer, len);
                }
                else
                {
                    LOG_WARN_FMT("--- %s recv errno[%d] on sockfd[%d] ! ---", toString(), errnum, sockfd);
                    error = TRUE;
                }
                break;
            }
            else if (0 == ret)
            {
                LOG_WARN_FMT("--- %s recv 0 bytes on sockfd[%d], errno[%d] ! ---", toString(), sockfd, errnum);
                error = TRUE;
                break;
            }
            else
            {
                len += ret;
            }
        }
        catch (...)
        {
            LOG_ERROR_FMT("%s catch exception !", toString());
        }
    }

    if (error)
    {
        processPeerDisconn(sockfd);
    }
}

void CRecvWorker::processPeerDisconn(INT4 sockfd)
{
    LOG_DEBUG_FMT("%s: peer disconnect sockfd[%d] ...", toString(), sockfd);
    delConnFd(sockfd);
    close(sockfd);
}


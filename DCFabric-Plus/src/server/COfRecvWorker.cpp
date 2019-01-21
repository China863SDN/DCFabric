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
*   File Name   : COfRecvWorker.cpp                                           *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "openflow-common.h"
#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"
#include "CControl.h"
#include "COfRecvWorker.h"
#include "CServer.h"
#include "COfConnectMgr.h"

static UINT8 total = 0;
static time_t timeStart = 0;
static UINT8 totalPeriod = 0;
static time_t timePeriod = 0;

#define procStats() \
{\
    if (0 == total)\
    {\
        timeStart = time(NULL);\
        timePeriod = timeStart;\
    }\
    if (++total%100000 == 0)\
    {\
        time_t tmCurr = time(NULL);\
        time_t pdDlta = tmCurr - timePeriod;\
        if (pdDlta >= 10)\
        {\
            UINT8 currSpeed = (UINT8)((total - totalPeriod) / pdDlta);\
            if (currSpeed > 0)\
            {\
                LOG_WARN_FMT("%s received %llu pkts with average speed %llu pkts/s, and current speed %llu pkts/s ...", \
                    toString(), total, (UINT8)(total/(tmCurr-timeStart)), currSpeed);\
                timePeriod = tmCurr;\
                totalPeriod = total;\
            }\
        }\
    }\
}

static void deleteCmsg(CMsgCommon* msg)
{
    if (NULL != msg)
    {
        CMsg* ofmsg = (CMsg*)msg;
        INT4 sockfd = ofmsg->getSockfd();

        ofmsg->~CMsg();

        CSmartPtr<COfRecvWorker> worker = CServer::getInstance()->mapOfRecvWorker(sockfd);
        if (worker.isNotNull())
            worker->getMemPool().release(ofmsg);
        else
            LOG_WARN_FMT("memory[%p] not released for none COfRecvWorker by sockfd[%d] !", ofmsg, sockfd);
    }
}

COfRecvWorker::COfRecvWorker():CRecvWorker(THREAD_TYPE_OCCUPY_CORE)
{
}

COfRecvWorker::~COfRecvWorker()
{
}

INT4 COfRecvWorker::init()
{
    m_memPool.init(toString());
    return CRecvWorker::init();
}

INT4 COfRecvWorker::addConnFd(INT4 fd)
{
    CSmartPtr<CLoopBuffer> recvBuffer = createRecvBuffer(fd);
    if (recvBuffer.isNull())
        return BNC_ERR;
    
    return CRecvWorker::addConnFd(fd);
}

INT4 COfRecvWorker::process(INT4 sockfd, INT1* buffer, UINT4 len)
{
    CSmartPtr<CLoopBuffer>& recvBuffer = m_recvBuffer[sockfd];
    if (recvBuffer.isNull())
    {
        LOG_WARN_FMT("m_recvBuffer[%d] is null !", sockfd);
        return BNC_ERR;
    }

    UINT4 recvBufLen = recvBuffer->length();
    if (0 == recvBufLen)
    {
        return procRecvBuffer(sockfd, buffer, len, recvBuffer);
    }
    else
    {
        if (recvBufLen + len < sizeof(struct ofp_header))
        {
            recvBuffer->write(buffer, len);
            return BNC_OK;
        }

        struct ofp_header header = {0};
        INT1* ptr = (INT1*)&header;
        if (recvBufLen < sizeof(struct ofp_header))
        {
            recvBuffer->read(ptr, recvBufLen, TRUE);
            memcpy(ptr+recvBufLen, buffer, sizeof(struct ofp_header)-recvBufLen);
        }
        else
        {
            recvBuffer->read(ptr, sizeof(struct ofp_header), TRUE);
        }
        UINT2 lenTotal = ntohs(header.length);

        if (recvBufLen + len < lenTotal)
        {
            recvBuffer->write(buffer, len);
        }
        else
        {
            procStats();

            INT1* pdata = (INT1*)m_memPool.alloc(lenTotal);
            if (NULL == pdata)
            {
                LOG_ERROR_FMT("alloc %u bytes failed!", lenTotal);
                return BNC_ERR;
            }
            recvBuffer->read(pdata, recvBufLen);
            memcpy(pdata+recvBufLen, buffer, lenTotal-recvBufLen);

            INT1* pmsg = (INT1*)m_memPool.alloc(sizeof(CMsg));
            if (NULL == pmsg)
            {
                LOG_ERROR_FMT("alloc CMsg failed !");
                m_memPool.release(pdata);
                return BNC_ERR;
            }
            CMsg* ofmsg = new(pmsg) CMsg(sockfd, header.version, header.type, pdata, lenTotal);

            ofmsg->setPath();
            ofmsg->setKey();

            CSmartPtr<CMsgCommon> msg(ofmsg, deleteCmsg);
            if (CControl::getInstance()->getMsgTree().pushMsg(msg) != BNC_OK)
            {
                LOG_INFO_FMT("push msg[%s]key[%s] from sockfd[%d] into tree failed", 
                    ofmsg->getPath().c_str(), ofmsg->getKey().c_str(), sockfd);
            }

            INT1* bufLeft = buffer + (lenTotal - recvBufLen);
            UINT4 lenLeft = len - (lenTotal - recvBufLen);
            if (lenLeft > 0)
                return procRecvBuffer(sockfd, bufLeft, lenLeft, recvBuffer);
        }    
    }

    return BNC_OK;
}

void COfRecvWorker::processPeerDisconn(INT4 sockfd)
{
    processConnClose(sockfd);

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    COfConnectMgr::processPeerDisconn(sw);
}

void COfRecvWorker::processConnClose(INT4 sockfd)
{
    //释放recvBuffer
    delRecvBuffer(sockfd);

    //从epoll中移除监听并关闭连接
    CRecvWorker::processPeerDisconn(sockfd);
}

CSmartPtr<CLoopBuffer> COfRecvWorker::getRecvBuffer(INT4 sockfd)
{
#if 0
    CSmartPtr<CLoopBuffer>* recvBuffer = m_recvBuffers.find(sockfd);
    if (NULL != recvBuffer)
        return *recvBuffer;
#else
    return m_recvBuffer[sockfd];
#endif

    return CSmartPtr<CLoopBuffer>(NULL);
}

CSmartPtr<CLoopBuffer> COfRecvWorker::createRecvBuffer(INT4 sockfd)
{
#if 0
    CSmartPtr<CLoopBuffer>* recvBuffer = m_recvBuffers.find(sockfd);
    if (NULL == recvBuffer)
#else
    CSmartPtr<CLoopBuffer>& recvBuffer = m_recvBuffer[sockfd];
    if (recvBuffer.isNull())
#endif
    {
        CLoopBuffer* buffer = new CLoopBuffer(recv_buffer_size);
        if (NULL == buffer)
        {
            LOG_ERROR_FMT("new CLoopBuffer with %u buffer size failed !", recv_buffer_size);
            return CSmartPtr<CLoopBuffer>(NULL);
        }

#if 0
        CSmartPtr<CLoopBuffer> sbuffer(buffer);
        recvBuffer = m_recvBuffers.insert(sockfd, sbuffer);
        if (NULL == recvBuffer)
        {
            LOG_WARN_FMT("CHashMap insert CSmartPtr<CLoopBuffer> failed !");
            return CSmartPtr<CLoopBuffer>(NULL);
        }
#else
        recvBuffer = CSmartPtr<CLoopBuffer>(buffer);
#endif
    }
    else
    {
        recvBuffer->reset();
    }

    return recvBuffer;
}

void COfRecvWorker::delRecvBuffer(INT4 sockfd)
{
#if 0
    m_recvBuffers.remove(sockfd);
#else
    //m_recvBuffer[sockfd].~CSmartPtr();
#endif
}

INT4 COfRecvWorker::procRecvBuffer(INT4 sockfd, INT1* buffer, UINT4 len, CSmartPtr<CLoopBuffer>& recvBuffer)
{
    struct ofp_header* header = NULL;
    const UINT4 headerLen = sizeof(struct ofp_header);
    UINT2 lenTotal = 0;
    INT1* pdata = NULL, *pmsg = NULL;
    CMsg* ofmsg = NULL;
    CMsgTree& msgTree = CControl::getInstance()->getMsgTree();
    
    while (len >= headerLen)
    {
        header = (struct ofp_header*)buffer;
        lenTotal = ntohs(header->length);
        if (lenTotal < headerLen)
        {
            LOG_WARN_FMT("%s drop packet when invalid ofp_header.length %u !!!", toString(), lenTotal);
            return BNC_ERR;
        }
        if (len < lenTotal)
            break;

        procStats();

        pdata = (INT1*)m_memPool.alloc(lenTotal);
        if (NULL == pdata)
        {
            LOG_ERROR_FMT("alloc %u bytes failed!", lenTotal);
            return BNC_ERR;
        }
        memcpy(pdata, buffer, lenTotal);

        pmsg = (INT1*)m_memPool.alloc(sizeof(CMsg));
        if (NULL == pmsg)
        {
            LOG_ERROR_FMT("alloc CMsg failed !");
            m_memPool.release(pdata);
            return BNC_ERR;
        }
        ofmsg = new(pmsg) CMsg(sockfd, header->version, header->type, pdata, lenTotal);

        ofmsg->setPath();
        ofmsg->setKey();

        CSmartPtr<CMsgCommon> msg(ofmsg, deleteCmsg);
        if (msgTree.pushMsg(msg) != BNC_OK)
        {
            LOG_INFO_FMT("push msg[%s]key[%s] from sockfd[%d] into tree failed", 
                ofmsg->getPath().c_str(), ofmsg->getKey().c_str(), sockfd);
        }

        buffer += lenTotal;
        len -= lenTotal;
    }
    
    if (len > 0)
        recvBuffer->write(buffer, len);

    return BNC_OK;
}



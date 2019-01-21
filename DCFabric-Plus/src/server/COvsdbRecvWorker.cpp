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
*   File Name   : COvsdbRecvWorker.cpp                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "log.h"
#include "COvsdbRecvWorker.h"
#include "COvsdbMgr.h"

COvsdbRecvWorker::COvsdbRecvWorker():m_buffers(hash_bucket_number)
{
}

COvsdbRecvWorker::~COvsdbRecvWorker()
{
}

INT4 COvsdbRecvWorker::addConnFd(INT4 fd)
{
    CSmartPtr<CBuffer> buffer = createBuffer(fd);
    if (buffer.isNull())
        return BNC_ERR;
    
    return CRecvWorker::addConnFd(fd);
}

INT4 COvsdbRecvWorker::process(INT4 sockfd, INT1* buffer, UINT4 len)
{
    LOG_INFO_FMT("COvsdbRecvWorker receive JSON[%u][%s] ...\n", len, buffer);

    CSmartPtr<CBuffer> segBuffer = getBuffer(sockfd);
    if (segBuffer.isNull())
    {
        LOG_WARN_FMT("CBuffer for sockfd[%d] is not created !", sockfd);
        return BNC_ERR;
    }

    INT1 jsonString[max_recv_buffer_size] = {0};
    UINT4 jsonLen = 0;
    
    if (segBuffer->empty())
    {
        while (len > 0)
        {
            //提取一个完整的JSON消息
            jsonLen = COvsdbMgr::getInstance()->genJson(buffer, len);
            if (0 == jsonLen)
                break;

            if (jsonLen > max_recv_buffer_size - 1)
                jsonLen = max_recv_buffer_size - 1;
            memcpy(jsonString, buffer, jsonLen);
            jsonString[jsonLen] = '\0';
        
            //处理一个完整的JSON消息
            COvsdbMgr::getInstance()->process(sockfd, jsonString, jsonLen);
        
            buffer += jsonLen;
            len -= jsonLen;
        }
        
        if (len > 0)
            segBuffer->write(buffer, len);
    }
    else
    {
        segBuffer->write(buffer, len);

        INT1* segData = segBuffer->getData();
        UINT4 segLen = segBuffer->length();

        while (segLen > 0)
        {
            //提取一个完整的JSON消息
            jsonLen = COvsdbMgr::getInstance()->genJson(segData, segLen);
            if (0 == jsonLen)
                break;

            segBuffer->read(jsonString, jsonLen);
            jsonString[jsonLen] = '\0';
        
            //处理一个完整的JSON消息
            COvsdbMgr::getInstance()->process(sockfd, jsonString, jsonLen);
        
            segData += jsonLen;
            segLen -= jsonLen;
        }
    }

    return BNC_OK;
}

void COvsdbRecvWorker::processPeerDisconn(INT4 sockfd)
{
    //释放CBuffer
    delBuffer(sockfd);

    //清除相应的OVSDB信息
    COvsdbMgr::getInstance()->delClient(sockfd);

    //从epoll中移除监听并关闭连接
    CRecvWorker::processPeerDisconn(sockfd);
}

CSmartPtr<CBuffer> COvsdbRecvWorker::getBuffer(INT4 sockfd)
{
    CSmartPtr<CBuffer> ret(NULL);
        
    CBufferHMap::CPair* item = NULL;
    if (m_buffers.find(sockfd, &item))
        ret = item->second;

    return ret;
}

CSmartPtr<CBuffer> COvsdbRecvWorker::createBuffer(INT4 sockfd)
{
    CBuffer* buffer = new CBuffer(segment_buffer_size);
    if (NULL == buffer)
    {
        LOG_ERROR_FMT("new CBuffer with %u size failed !", segment_buffer_size);
        return CSmartPtr<CBuffer>(NULL);
    }
    
    CSmartPtr<CBuffer> sBuffer = CSmartPtr<CBuffer>(buffer);
    if (!m_buffers.insert(sockfd, sBuffer))
    {
        LOG_WARN_FMT("add mapping sockfd[%d] to CSmartPtr<CBuffer> failed !", sockfd);
        return CSmartPtr<CBuffer>(NULL);
    }

    return sBuffer;
}

void COvsdbRecvWorker::delBuffer(INT4 sockfd)
{
    m_buffers.erase(sockfd);
}


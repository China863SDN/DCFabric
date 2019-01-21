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
*   File Name   : CRedisSubscriber.cpp                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "bnc-param.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "log.h"
#include "CRedisSubscriber.h"
#include "CConf.h"
#include "CSyncMgr.h"

#define REDIS_PORT      6379

static void connectCb(const redisAsyncContext *ctx, int status) 
{  
    if (status != REDIS_OK) 
    {  
        LOG_WARN_FMT("redisAsyncConnect failed[%s] !", ctx->errstr);        
        return;  
    }  

    LOG_WARN("Connected...");        
    //CRedisSubscriber::getInstance()->notify();
}  

static void disconnectCb(const redisAsyncContext *ctx, int status) 
{  
    if (status != REDIS_OK) 
    {  
        LOG_WARN_FMT("redisAsyncDisconnect failed[%s] !", ctx->errstr);        
        return;  
    }  
    LOG_WARN("Disconnected...");        
} 

static void subscribeCb(redisAsyncContext *ctx, void *rep, void *priv) 
{  
    redisReply *reply = (redisReply*)rep;  
    if (reply == NULL) 
        return;  

    //LOG_WARN_FMT("reply->type[%d]reply->elements[%llu]", reply->type, reply->elements);
    if ((REDIS_REPLY_ARRAY == reply->type) && (3 == reply->elements))
    {
#if 0
        for (size_t i = 0; i < reply->elements; ++i)
        {
            LOG_WARN_FMT("reply->element[%llu]->type[%d]", i, reply->element[i]->type);
            if ((REDIS_REPLY_STRING == reply->element[i]->type) && (NULL != reply->element[i]->str))
                LOG_WARN_FMT("reply->element[%llu]->str[%s]", i, reply->element[i]->str);
        }
#endif
        if ((strcmp(reply->element[0]->str, "message") == 0) &&
            (strcmp(reply->element[1]->str, REDIS_SUBSCRIBE_CHANNEL) == 0) &&
            (REDIS_REPLY_STRING == reply->element[2]->type) &&
            (NULL != reply->element[2]->str))
            CSyncMgr::getInstance()->recover(reply->element[2]->str);
    }
}  

static void* subscribeRoutine(void* param)
{
    CRedisSubscriber* sub = (CRedisSubscriber*)param;
    if (NULL != sub)
        sub->subscribe();

    return NULL;
}

CRedisSubscriber* CRedisSubscriber::m_instance = NULL;

CRedisSubscriber* CRedisSubscriber::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CRedisSubscriber();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return m_instance;
}

CRedisSubscriber::CRedisSubscriber():
    m_serverPort(0),
    m_redisAsyncCtx(NULL),
    m_eventBase(NULL)
{
}

CRedisSubscriber::~CRedisSubscriber()
{
	if (NULL != m_redisAsyncCtx)
    {
        redisAsyncDisconnect(m_redisAsyncCtx);
        redisAsyncFree(m_redisAsyncCtx);
        m_redisAsyncCtx = NULL;
    }
	if (NULL != m_eventBase)
    {
        event_base_free(m_eventBase);
        m_eventBase = NULL;
    }
}

INT4 CRedisSubscriber::init()
{
    const INT1* pConf = CConf::getInstance()->getConfig("cluster_conf", "redis_server_ip");
    m_serverIp = (NULL != pConf) ? pConf : "127.0.0.1";

    pConf = CConf::getInstance()->getConfig("cluster_conf", "redis_server_port");
    m_serverPort = (NULL != pConf) ? atoi(pConf) : REDIS_PORT;

	LOG_INFO_FMT("redis_server_ip: %s, redis_server_port: %u", m_serverIp.c_str(), m_serverPort);

    m_redisAsyncCtx = redisAsyncConnect(m_serverIp.c_str(), m_serverPort); 
	if (NULL == m_redisAsyncCtx)
	{        
        LOG_WARN("redisAsyncConnect failed: can't allocate redisAsyncContext !");        
		return BNC_ERR;
	}
	if (0 != m_redisAsyncCtx->err)
	{        
        LOG_WARN_FMT("redisAsyncConnect failed[%s] !", m_redisAsyncCtx->errstr);
        redisAsyncFree(m_redisAsyncCtx);
        m_redisAsyncCtx = NULL;
		return BNC_ERR;
	}

    m_eventBase = event_base_new();
	if (NULL == m_eventBase)
	{        
        LOG_WARN("event_base_new failed !");        
        redisAsyncFree(m_redisAsyncCtx);
        m_redisAsyncCtx = NULL;
		return BNC_ERR;
	}

    redisLibeventAttach(m_redisAsyncCtx, m_eventBase);

    redisAsyncSetConnectCallback(m_redisAsyncCtx, connectCb);  
    redisAsyncSetDisconnectCallback(m_redisAsyncCtx, disconnectCb);

    redisAsyncCommand(m_redisAsyncCtx, subscribeCb, this, "SUBSCRIBE %s", REDIS_SUBSCRIBE_CHANNEL);

    return m_thread.init(subscribeRoutine, this, "CRedisSubscriber");
}

INT4 CRedisSubscriber::subscribe()
{
    //m_semaphore.wait();

	prctl(PR_SET_NAME, (unsigned long)"CRedisSubscriber");  

#if 1
    usleep(10000);
    //LOG_WARN("block to dispatch...");        
    event_base_dispatch(m_eventBase); 
#else
    if (redisAsyncCommand(m_redisAsyncCtx, subscribeCb, this, "SUBSCRIBE %s", REDIS_SUBSCRIBE_CHANNEL) == 0)
    {
        LOG_WARN_FMT("SUBSCRIBE %s success", REDIS_SUBSCRIBE_CHANNEL);        
        event_base_dispatch(m_eventBase); 
        return BNC_OK;
    }
    LOG_WARN_FMT("SUBSCRIBE %s failed !", REDIS_SUBSCRIBE_CHANNEL);        
#endif

    return BNC_ERR;
}

void CRedisSubscriber::notify()
{
    m_semaphore.post();
}

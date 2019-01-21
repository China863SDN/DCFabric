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
*   File Name   : CRedisClient.cpp                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-param.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "log.h"
#include "CRedisClient.h"
#include "CControl.h"
#include "CConf.h"
#include "CServer.h"
#include "CRedisSubscriber.h"
#include "CSyncDefine.h"

#define REDIS_PORT      6379

CRedisClient::CRedisClient():
    m_serverPort(0),
    m_redisCtx(NULL)
{
}

CRedisClient::~CRedisClient()
{
	if (NULL != m_redisCtx)
    {
        redisFree(m_redisCtx);
        m_redisCtx = NULL;
    }
}

INT4 CRedisClient::init()
{
    INT1 ipStr[20] = {0};
    number2ip(htonl(CServer::getInstance()->getControllerIp()), ipStr);
    m_clientIp = ipStr;

    const INT1* pConf = CConf::getInstance()->getConfig("cluster_conf", "redis_server_ip");
    m_serverIp = (NULL != pConf) ? pConf : "127.0.0.1";

    pConf = CConf::getInstance()->getConfig("cluster_conf", "redis_server_port");
    m_serverPort = (NULL != pConf) ? atoi(pConf) : REDIS_PORT;

	LOG_INFO_FMT("redis_server_ip: %s, redis_server_port: %u", m_serverIp.c_str(), m_serverPort);

    struct timeval tv = {1, 0};
	m_redisCtx = redisConnectWithTimeout(m_serverIp.c_str(), m_serverPort, tv);
	if (NULL == m_redisCtx)
	{        
        LOG_WARN("Connection error: can't allocate redis context !");        
		return BNC_ERR;
	}
	if (0 != m_redisCtx->err)
	{        
        LOG_WARN_FMT("redisConnectWithTimeout failed[%s] !", m_redisCtx->errstr);
        redisFree(m_redisCtx);
        m_redisCtx = NULL;
		return BNC_ERR;
	}

	if (CRedisSubscriber::getInstance()->init() != BNC_OK)
	{        
        LOG_WARN("init CRedisSubscriber failed !");        
		//return BNC_ERR;
	}

    return BNC_OK;
}

INT4 CRedisClient::persistData(INT4 nodeType, const INT1* data, UINT4 len)
{
    if ((NULL == data) || (0 == len) || (NODE_TYPE_MAX <= nodeType))
        return BNC_ERR;

    LOG_INFO_FMT("******CRedisClient: persistData nodeType[%d]len[%u]******", nodeType, len);

    m_mutex.lock();
	//redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "SET %s-%s %b", m_clientIp.c_str(), nodeTypeString[nodeType], data, (size_t)len);
	redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "SET %s %b", nodeTypeString[nodeType], data, (size_t)len);
    m_mutex.unlock();

	if ((NULL == reply) || 
        (REDIS_REPLY_NIL == reply->type) || 
        (REDIS_REPLY_ERROR == reply->type))
	{
		if (m_redisCtx->err)
			LOG_WARN_FMT("persistData: redisCommand SET nodeType[%d]len[%u] failed[%s] !", nodeType, len, m_redisCtx->errstr);
		if (NULL != reply)
            freeReplyObject(reply);
		return BNC_ERR;
	}

	freeReplyObject(reply);
    
    publish(nodeTypeString[nodeType]);
    
    return BNC_OK;
}

INT4 CRedisClient::persistData(const INT1* key, const INT1* data, UINT4 len)
{
    if ((NULL == key) || (NULL == data) || (0 == len))
        return BNC_ERR;

    LOG_INFO_FMT("******CRedisClient: persistData key[%s]len[%u]******", key, len);

    m_mutex.lock();
	//redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "SET %s-%s %b", m_clientIp.c_str(), key, data, (size_t)len);
	redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "SET %s %b", key, data, (size_t)len);
    m_mutex.unlock();

	if ((NULL == reply) || 
        (REDIS_REPLY_NIL == reply->type) || 
        (REDIS_REPLY_ERROR == reply->type))
	{
		if (m_redisCtx->err)
			LOG_WARN_FMT("persistData: redisCommand SET key[%s]len[%u] failed[%s] !", key, len, m_redisCtx->errstr);
		if (NULL != reply)
            freeReplyObject(reply);
		return BNC_ERR;
	}

	freeReplyObject(reply);
    
    publish(key);
    
    return BNC_OK;
}

INT4 CRedisClient::persistValue(const INT1* key, const INT1* val)
{
    if ((NULL == key) || (NULL == val))
        return BNC_ERR;

    LOG_INFO_FMT("******CRedisClient: persistValue key[%s]val[%s]******", key, val);

    m_mutex.lock();
	//redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "SET %s-%s %s", m_clientIp.c_str(), key, val);
	redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "SET %s %s", key, val);
    m_mutex.unlock();

	if ((NULL == reply) || 
        (REDIS_REPLY_NIL == reply->type) || 
        (REDIS_REPLY_ERROR == reply->type))
	{
		if (m_redisCtx->err)
			LOG_WARN_FMT("persistValue: redisCommand SET key[%s]val[%s] failed[%s] !", key, val, m_redisCtx->errstr);
		if (NULL != reply)
            freeReplyObject(reply);
		return BNC_ERR;
	}

	freeReplyObject(reply);

    publish(key);
    
    return BNC_OK;
}

INT4 CRedisClient::queryData(INT4 nodeType, INT1* data, UINT4& len)
{
    if ((NULL == data) || (NODE_TYPE_MAX <= nodeType))
        return BNC_ERR;

    m_mutex.lock();
	//redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "GET %s-%s", m_clientIp.c_str(), nodeTypeString[nodeType]);
	redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "GET %s", nodeTypeString[nodeType]);
    m_mutex.unlock();

	if ((NULL == reply) || 
        (REDIS_REPLY_NIL == reply->type) || 
        (REDIS_REPLY_ERROR == reply->type))
	{
		if (m_redisCtx->err)
			LOG_WARN_FMT("queryData: redisCommand GET nodeType[%d] failed[%s] !", nodeType, m_redisCtx->errstr);
		if (NULL != reply)
            freeReplyObject(reply);
		return BNC_ERR;
	}

	if (reply->len > 0)
	{
		memcpy(data, reply->str, reply->len);
        len = reply->len;
	}
	freeReplyObject(reply);
    
    LOG_INFO_FMT("******queryValue: nodeType[%d]len[%u]******", nodeType, len);
    return BNC_OK;
}

INT4 CRedisClient::queryData(const INT1* key, INT1* data, UINT4& len)
{
    if ((NULL == key) || (NULL == data))
        return BNC_ERR;

    m_mutex.lock();
	//redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "GET %s-%s", m_clientIp.c_str(), key);
	redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "GET %s", key);
    m_mutex.unlock();

	if ((NULL == reply) || 
        (REDIS_REPLY_NIL == reply->type) || 
        (REDIS_REPLY_ERROR == reply->type))
	{
		if (m_redisCtx->err)
			LOG_WARN_FMT("queryData: redisCommand GET key[%s] failed[%s] !", key, m_redisCtx->errstr);
		if (NULL != reply)
            freeReplyObject(reply);
		return BNC_ERR;
	}

	if (reply->len > 0)
	{
		memcpy(data, reply->str, reply->len);
        len = reply->len;
	}
	freeReplyObject(reply);
    
    LOG_INFO_FMT("******queryValue: key[%s]len[%u]******", key, len);
    return BNC_OK;
}

INT4 CRedisClient::queryValue(const INT1* key, INT1* val)
{
    if ((NULL == key) || (NULL == val))
        return BNC_ERR;

    m_mutex.lock();
	//redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "GET %s-%s", m_clientIp.c_str(), key);
	redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "GET %s", key);
    m_mutex.unlock();

	if ((NULL == reply) || 
        (REDIS_REPLY_NIL == reply->type) || 
        (REDIS_REPLY_ERROR == reply->type))
	{
		if (m_redisCtx->err)
			LOG_WARN_FMT("queryValue: redisCommand GET key[%s] failed[%s] !", key, m_redisCtx->errstr);
		if (NULL != reply)
            freeReplyObject(reply);
		return BNC_ERR;
	}

	if (reply->len > 0)
	{
		memcpy(val, reply->str, reply->len);
	}
	freeReplyObject(reply);
    
    LOG_INFO_FMT("******queryValue: key[%s]val[%s]******", key, val);
    return BNC_OK;
}

void CRedisClient::publish(const INT1* msg)
{
    if (NULL == msg)
        return;

    m_mutex.lock();
	redisReply* reply = (redisReply *)redisCommand(m_redisCtx, "PUBLISH %s %s", REDIS_SUBSCRIBE_CHANNEL, msg);
    m_mutex.unlock();

	if ((NULL == reply) || 
        (REDIS_REPLY_NIL == reply->type) || 
        (REDIS_REPLY_ERROR == reply->type))
	{
		if (m_redisCtx->err)
			LOG_WARN_FMT("publish: redisCommand PUBLISH %s %s failed[%s] !", REDIS_SUBSCRIBE_CHANNEL, msg, m_redisCtx->errstr);
	}

    if (NULL != reply)
        freeReplyObject(reply);

    //LOG_WARN_FMT("PUBLISH %s %s success", REDIS_SUBSCRIBE_CHANNEL, msg);        
}


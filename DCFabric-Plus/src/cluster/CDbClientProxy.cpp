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
*   File Name   : CDbClientProxy.cpp                                          *
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
#include "CDbClientProxy.h"
#include "CRedisClient.h"

CDbClientProxy::CDbClientProxy()
{
}

CDbClientProxy::~CDbClientProxy()
{
}

INT4 CDbClientProxy::init()
{
    m_client = new CRedisClient();
    if (NULL != m_client)
    {
        if (m_client->init() != BNC_OK)
        {
            LOG_ERROR("Init CRedisClient failed !");
            delete m_client;
            m_client = NULL;
            return BNC_ERR;
        }
    }

    return BNC_OK;
}

INT4 CDbClientProxy::persistData(INT4 nodeType, const INT1* data, UINT4 len)
{
    return (NULL != m_client) ? m_client->persistData(nodeType, data, len) : BNC_ERR;
}

INT4 CDbClientProxy::persistData(const INT1* key, const INT1* data, UINT4 len)
{
    return (NULL != m_client) ? m_client->persistData(key, data, len) : BNC_ERR;
}

INT4 CDbClientProxy::persistValue(const INT1* key, const INT1* val)
{
    return (NULL != m_client) ? m_client->persistValue(key, val) : BNC_ERR;
}

INT4 CDbClientProxy::queryData(INT4 nodeType, INT1* data, UINT4& len)
{
    return (NULL != m_client) ? m_client->queryData(nodeType, data, len) : BNC_ERR;
}

INT4 CDbClientProxy::queryData(const INT1* key, INT1* data, UINT4& len)
{
    return (NULL != m_client) ? m_client->queryData(key, data, len) : BNC_ERR;
}

INT4 CDbClientProxy::queryValue(const INT1* key, INT1* val)
{
    return (NULL != m_client) ? m_client->queryValue(key, val) : BNC_ERR;
}


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
*   File Name   : CHttpRecvWorker.cpp                                         *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CHttpRecvWorker.h"
#include "CRestManager.h"
#include "bnc-error.h"

CHttpRecvWorker::CHttpRecvWorker()
{
}

CHttpRecvWorker::~CHttpRecvWorker()
{
}

INT4 CHttpRecvWorker::process(INT4 sockfd, INT1* buffer, UINT4 len)
{
    //LOG_WARN_FMT("%s receive request[%u][%s] ", toString(), len, buffer);

    std::string raw(buffer, len);

    // 创建request和response
	CRestRequest* request = new CRestRequest(raw);
	CRestResponse* response = new CRestResponse();

	// 打印收到的RestApi
	/*LOG_WARN_FMT("Receive Rest Api: %s %s", 
	    (request->getMethod()==bnc::restful::HTTP_GET)?"GET":
	    (request->getMethod()==bnc::restful::HTTP_POST)?"POST":
	    (request->getMethod()==bnc::restful::HTTP_PUT)?"PUT":
	    (request->getMethod()==bnc::restful::HTTP_DELETE)?"DELETE":
	    (request->getMethod()==bnc::restful::HTTP_ANY)?"ANY":"OTHER",
	    request->getPath().c_str());*/

	// 通过restAPI提供的接口来处理
	CRestManager::getInstance()->getRestApiMgr()->process(request, response);

	//LOG_WARN_FMT("Response body: %s", response->getBody().c_str());

	// 发送response
	response->writeHttp(sockfd);

	// 删除指针
	delete request;
	delete response;

    return BNC_OK;
}


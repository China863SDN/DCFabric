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
*   File Name   : CRestApiHandler.h          *
*   Author      : bnc xflu           		*
*   Create Date : 2016-7-22           		*
*   Version     : 1.0           			*
*   Function    : .           				*
*                                                                             *
******************************************************************************/
#ifndef _CRESTAPIMGR_H
#define _CRESTAPIMGR_H

#include "CRestApi.h"

/*
 * RestAPI
 */

// path, param
typedef std::pair<std::string, bnc::restful::http_method> restVar;

// var + handler
typedef std::map<restVar, restApiHandler> restApi;

class CRestApiMgr
{
public:
	/*
	 * 默认析构函数
	 */
	~CRestApiMgr();
	/*
	 * 获取RestApi实例
	 */
	static CRestApiMgr* getInstance();

	/*
	 * 获取RestApi总数
	 */
	INT4 getRestApiCount();

	/*
	 * 获取RestApi中所有已经注册的path
	 * @param: list: 用来存储所有path的list
	 * @return: TRUE:success; FALSE:fail
	 */
	BOOL getRestApiPathList(std::list<std::string> & list);

	/*
	 * 获取RestApi中所有已经注册的method + path
	 * @param: list: 用来存储所有path的list
	 * @return: TRUE:success; FALSE:fail
	 */
	BOOL getRestApiPathList(std::list<restVar> & list);

	/*
	 * 注册RestApi
	 *
	 * @param: path         http uri的path
	 * @param: handler      注册处理函数
	 *
	 * @return: BOOL        TRUE: success; FALSE: fail
	 */
	BOOL registerRestApi(bnc::restful::http_method,
	        const std::string & path, restApiHandler handler);

	/*
	 * 注册默认RestApi
	 *
	 * @return: BOOL         TRUE: success; FALSE: fail
	 */
	BOOL registerDefaultRestApi();

	/*
	 * 判断path对应的restApi是否存在
	 *
	 * @param: path      http uri的path
	 *
	 * @return: BOOL     TRUE: exist; FALSE: not exist
	 */
	BOOL isExist(const restVar & path);

	/*
	 * 处理对于RestApi的请求
	 *
	 * @param: path         http uri的path
	 * @param: body         http的body
	 *
	 * @return: BOOL        TRUE: success; FALSE: fail
	 */
	// BOOL process(const std::string & path, const std::string & body);
	BOOL process(CRestRequest* request, CRestResponse* response);

private:
	/*
	 * 默认构造函数
	 */
	CRestApiMgr();

	/*
	 * 查找RestApi
	 * @param: path: http uri的path
	 * @return: 处理对应path的function handler
	 */
	restApiHandler findRestApi(const restVar & var);

	/*
	 * 初始化
	 * @return: TRUE: success; FALSE: fail
	 */
	BOOL init();

	static CRestApiMgr* m_pInstance;		///< RestApi实例
	restApi m_restApiList;					///< RestApi列表
};

#endif

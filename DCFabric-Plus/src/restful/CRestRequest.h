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
*   File Name   : CRestRequest.h	*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-21         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#ifndef _CRESTREQUEST_H
#define _CRESTREQUEST_H

#include "CRestHttp.h"
#include "CRestResponse.h"

/*
 * HTTP Request相关类
 */
class CRestRequest : public CRestHttp
{
public:
	/*
	 * 使用string构造函数
	 */
	CRestRequest(std::string & raw);

	/*
	 * 使用类型构造函数
	 */
	CRestRequest(bnc::restful::http_version ver, bnc::restful::http_method method,
	             bnc::restful::content_type type, const std::string & path, const std::string & body);


	/*
	 * 默认析构函数
	 */
	~CRestRequest();

	/*
	 * 获取method
	 */
	const bnc::restful::http_method getMethod()	const { return m_enHttpMethod; }

	/*
	 * 获取path
	 * 例如: /get/sw
	 */
	const std::string & getPath() const { return m_strPath; }

	/*
	 * 获取带参数的path
	 * 例如: /get/sw/{00:01:02:03:04:05}
	 */
	const std::string & getPathWithParam() const { return m_strPathWithParam; }

	/*
	 * 获取带参数的path中的参数列表
	 */
	const std::list<std::string> & getPathParamList() const { return m_pathParamList; }

private:
	/*
	 * 默认构造函数
	 */
	CRestRequest();

    /*
     * 从Http header中取得path
     * 解析后的结果为带参数的Path
     */
    BOOL parsePath(const std::string & firstLine);

    /*
     * 解析path中的参数
     * 解析后的结果为不带参数的Path和参数列表
     * 例如: 传入参数:path: /echo/{123}/reply/{456}
     * 解析后: m_strPath: /echo/reply
     * m_pathParamList中存储了两个参数: 123,456
     */
    BOOL parsePathParam(const std::string & path);

    /*
     * 创建http头
     *
     * @param: version:         http版本号
     * @param: status:          返回状态
     * @param: conetent_length: body的长度
     * @param: header:          用来保存http头的字符串
     */
    BOOL createHeader(bnc::restful::http_version version,
                      bnc::restful::http_method method,
                      bnc::restful::content_type type,
                      const std::string & path,
                      UINT4 content_length,
                      std::string & header);

	bnc::restful::http_method m_enHttpMethod;		///< http method
	std::string m_strPath;							///< http uri path
	std::string m_strPathWithParam;                 ///< http uri path with parameters
	std::list<std::string> m_pathParamList;         ///< http uri path parameters list
};

#endif

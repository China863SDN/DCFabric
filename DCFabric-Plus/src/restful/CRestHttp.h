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
*   File Name   : CRestHttp.h             *
*   Author      : bnc xflu           		*
*   Create Date : 2016-7-22           		*
*   Version     : 1.0           			*
*   Function    : .           				*
*                                                                             *
******************************************************************************/
#ifndef _CRESTHTTP_H
#define _CRESTHTTP_H

#include "CRestDefine.h"

#include <string>
#include <bnc-type.h>

/*
 * HTTP context相关的基�? 提供共有的存取控�?
 * the base class of HTTP context related
 */
class CRestHttp
{
public:
	/*
	 * 默认构造函�?
	 */
	CRestHttp();

	/*
	 * 默认析构函数
	 */
	~CRestHttp();

	/*
	 * 从字符串构�?
	 */
	CRestHttp(std::string raw);

	/*
	 * 获取http版本�?
	 * @return: http version
	 */
	const bnc::restful::http_version getVersion() const { return m_enHttpVersion; }

	/*
	 * 获取content类型
	 */
	const bnc::restful::content_type getContentType() const { return m_enContentType; }

	/*
	 * 获取http头部
	 * @return: http header
	 */
	const std::string & getHeader() const { return m_strHeader; }

	/*
	 * 获取http body
	 * @return: http body
	 */
	const std::string & getBody() const { return m_strBody; }

	/*
	 * 获取http header和body
	 */
	const std::string & getRaw(std::string & raw) const
	{ 	return (raw = m_strHeader + "\r\n\r\n" + m_strBody); }

	/*
	 * 获取http头部首行
	 * request: 包括了method/path/version
	 * response: 包括了version/status
	 * @return: http first line
	 */
	const std::string & getHeaderFirstLine() const { return m_strFirstLine; }

	/*
	 * 验证http内容有效
	 * @return: TRUE: valid; FALSE: invalid
	 */
	BOOL validate();

	/*
	 * 将字符串格式化为http内容
	 * @param: raw: original string
	 * @return: TRUE:sucess; FALSE: fail
	 */
	BOOL parse(std::string raw);

	/*
	 * 发送Http Request/Response
	 */
	BOOL writeHttp(INT4 sockfd);

	/*
	 * 设置header
	 */
	void setHeader(const std::string & header) { m_strHeader = header; }

	/*
	 * 设置body
	 */
	void setBody(const std::string & body) { m_strBody = body; }

    /*
     * 设置content type
     */
    void setContentType(bnc::restful::content_type type) { m_enContentType = type; }

    /*
     * 设置token信息
     */
    void setToken(const std::string & token);


private:

	bnc::restful::http_version m_enHttpVersion;		///< http版本
    bnc::restful::content_type m_enContentType;     ///< http content type
	std::string m_strHeader;						///< http头部
	std::string m_strBody;							///< http body
	std::string m_strFirstLine;						///< http头部首行
};

#endif

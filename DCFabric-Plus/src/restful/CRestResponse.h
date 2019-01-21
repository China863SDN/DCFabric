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
*   File Name   : CRestResponce.h	*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-21         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#ifndef _CRESTRESPONESE_H
#define _CRESTRESPONESE_H

#include <string>
#include "CRestHttp.h"

/*
 * HTTP response
 */
class CRestResponse : public CRestHttp
{
public:
	/*
	 * 默认构造函数
	 */
	CRestResponse();

	/*
	 * 带参数的构造函数
	 */
	CRestResponse(const std::string & raw);

	/*
	 * 默认析构函数
	 */
	~CRestResponse();

	/*
	 * 带参数的构造函数
	 *
	 * @param: version: http版本
	 * @param: status:  返回状态
	 * @param: body:	http body
	 */
	CRestResponse(bnc::restful::http_version version,
				  bnc::restful::http_status status,
				  const std::string body);

	/*
	 * 设置response参数
	 */
	BOOL setResponse(bnc::restful::http_version version,
					 bnc::restful::http_status status,
					 const std::string & body);

private:
	/*
	 * 创建http头
	 *
	 * @param: version:			http版本号
	 * @param: status:			返回状态
	 * @param: conetent_length:	body的长度
	 * @param: header:			用来保存http头的字符串
	 */
	BOOL createHeader(bnc::restful::http_version version,
					  bnc::restful::http_status status,
					  UINT4 content_length,
					  std::string & header);


	BOOL createHeader(bnc::restful::http_version version,
                      bnc::restful::http_status status,
                      bnc::restful::content_type type,
                      UINT4 content_length,
                      std::string & header);

	bnc::restful::http_status m_enHttpStatus;		///< http status


};

#endif

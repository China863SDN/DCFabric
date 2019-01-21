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
*   File Name   : CRestRequest.cpp   *
*   Author      : bnc xflu           *
*   Create Date : 2016-7-21          *
*   Version     : 1.0           	 *
*   Function    : .           		 *
*                                                                             *
******************************************************************************/
#include "CRestResponse.h"
#include "log.h"

CRestResponse::CRestResponse()
{
}

CRestResponse::~CRestResponse()
{
}

CRestResponse::CRestResponse(const std::string & raw)
{
    // 将string转换成可用的http
    parse(raw);

    // 验证http有效
    if (validate()) {
        // 获取http status
        m_enHttpStatus = CRestDefine::getInstance()->getStatusFromStr(getHeaderFirstLine());
    }
}

CRestResponse::CRestResponse(bnc::restful::http_version version,
				  	  	  	 bnc::restful::http_status status,
							 const std::string body)
{
	//	example: ("HTTP/1.1 200 OK\r\nContent-Type: application/json"
	//			"\r\nPragma: no-cache\r\n\r\n{123}")

	std::string raw;
	if (createHeader(version, status, body.size(), raw))
	{
		raw.append("\r\n\r\n");
		raw.append(body);
	}

	parse(raw);
}

BOOL CRestResponse::createHeader(bnc::restful::http_version version,
                                 bnc::restful::http_status status,
                                 UINT4 content_length,
                                 std::string & header)
{
    BOOL ret = createHeader(version, status, bnc::restful::CONTENT_JSON, content_length, header);
    return ret;
}

BOOL CRestResponse::createHeader(bnc::restful::http_version version,
								 bnc::restful::http_status status,
								 bnc::restful::content_type type,
								 UINT4 content_length,
								 std::string & header)
{
	if ((bnc::restful::HTTP_VERSION_OHTER == version) || (bnc::restful::STATUS_OTHER == status))
	{
		LOG_ERROR("create response failed. invalid version or status.");
		return FALSE;
	}

	m_enHttpStatus = status;

	std::string strVersion;
	std::string strStatus;
	std::string strContentType;

	CRestDefine::getInstance()->getStrFromVersion(version, strVersion);
	CRestDefine::getInstance()->getStrFromStatus(status, strStatus);
	CRestDefine::getInstance()->getStrFromContentType(type, strContentType);

	if ((strStatus.empty()) || (strVersion.empty()))
	{
		LOG_ERROR("create response failed. status or version string empty");
		return FALSE;
	}

	std::string strContentTypeSeg("Content-Type: ");
	std::string strContentLength("Content-Length: ");

	INT1 strLength[24] = {0};
	content_length = (0 == content_length) ? 500 : content_length;
	sprintf(strLength, "%d", content_length);

	header.append(strVersion);
	header.append(" ");
	header.append(strStatus);
	header.append("\r\n");
	header.append(strContentTypeSeg);
	header.append(strContentType);
	header.append("\r\n");
	header.append("Access-Control-Allow-Origin: *");
	header.append("\r\n");
	header.append("Access-Control-Allow-Methods:GET, POST, PUT, DELETE, OPTIONS");
	//header.append("Access-Control-Allow-Methods:*");
	header.append("\r\n");
	header.append("Access-Control-Allow-Headers:Content-Type");
	header.append("\r\n");
	header.append(strContentLength);
	header.append(strLength);
	
	

	return TRUE;
}

BOOL CRestResponse::setResponse(bnc::restful::http_version version,
							    bnc::restful::http_status status,
							    const std::string & body)
{
	std::string header;
	if (createHeader(version, status, body.size(), header))
	{
		setHeader(header);
	}

	setBody(body);

	return TRUE;
}




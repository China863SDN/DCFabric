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
*   File Name   : CRestHttp.cpp      *
*   Author      : bnc xflu           *
*   Create Date : 2016-7-22          *
*   Version     : 1.0                *
*   Function    : .                  *
*                                                                             *
******************************************************************************/
#include "CRestHttp.h"
#include "comm-util.h"
#include "log.h"

CRestHttp::CRestHttp()
{
}

CRestHttp::~CRestHttp()
{
}

CRestHttp::CRestHttp(std::string raw)
{
	parse(raw);
}

BOOL CRestHttp::validate()
{
	if ((m_strHeader.empty())
		|| (bnc::restful::HTTP_VERSION_OHTER == m_enHttpVersion))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CRestHttp::parse(std::string raw)
{
	LOG_IF_RETURN (raw.empty(), FALSE, "Fail to parse, string is empty.");

	std::string separator("\r\n\r\n");
	size_t pos = raw.find(separator, 0);

	LOG_IF_RETURN (0 >= pos, FALSE, "Fail to find, string is invalid.");

	m_strHeader = raw.substr(0, pos);
	if(std::string::npos != pos)
	{
		m_strBody = raw.substr(pos + separator.size(), raw.size());
	}

	pos = m_strHeader.find("\n", 0);

	LOG_IF_RETURN (0 >= pos, FALSE, "Fail to find first line, string is invalid");

	m_strFirstLine = m_strHeader.substr(0, pos);

	m_enHttpVersion = CRestDefine::getInstance()->getVersionFromStr(m_strFirstLine);

	/* only support content json */
	m_enContentType = bnc::restful::CONTENT_JSON;

	return TRUE;
}

BOOL CRestHttp::writeHttp(INT4 sockfd)
{
    std::string res;
    getRaw(res);

    sendMsgOut(sockfd, res.c_str(), res.size());

    close(sockfd);

    return TRUE;
}

void CRestHttp::setToken(const std::string & token)
{
    if (std::string::npos == m_strHeader.find("X-Auth-Token"))
    {
        m_strHeader.append("\r\n");
        m_strHeader.append("X-Auth-Token:");
        m_strHeader.append(" ");
        m_strHeader.append(token);
    }
}




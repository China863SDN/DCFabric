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
#include "CRestRequest.h"
#include "CRestManager.h"
#include "comm-util.h"
#include "log.h"

// 定义的参数分割符�? 在Url�?{ �?%7B; }�?7D
std::string param_split_begin = "(";
std::string param_split_end = ")";

CRestRequest::CRestRequest()
{
}

CRestRequest::CRestRequest(std::string & raw)
{
	// 将string转换成可用的http
	parse(raw);

	// 验证http有效
	if (validate()) 
    {
		// 获取method和path
	    parsePath(getHeaderFirstLine());
	    parsePathParam(m_strPathWithParam);
		m_enHttpMethod = CRestDefine::getInstance()->getMethodFromStr(getHeaderFirstLine());
	}
#if 0
    LOG_WARN_FMT("$$$ method: %d", m_enHttpMethod);
    LOG_WARN_FMT("$$$ path: %s", m_strPath.c_str());
    LOG_WARN_FMT("$$$ param string: %s", m_strPathWithParam.c_str());
    LOG_WARN_FMT("$$$ param:");
    STL_FOR_LOOP(m_pathParamList, it)
        LOG_WARN_FMT("$$$        %s", (*it).c_str());
#endif
}

CRestRequest::CRestRequest(bnc::restful::http_version version,
                           bnc::restful::http_method method,
                           bnc::restful::content_type type,
                           const std::string & path,
                           const std::string & body)
{
    std::string raw;
    if (createHeader(version, method, type, path, body.size(), raw))
    {
        raw.append("\r\n\r\n");
        raw.append(body);
    }

    parse(raw);

    m_enHttpMethod = method;
    m_strPath = path;
    m_strPathWithParam = path;
}

CRestRequest::~CRestRequest()
{
}

BOOL CRestRequest::parsePath(const std::string & firstLine)
{
    // 从Http头中解析path的字�?
    std::list<std::string> singleList;
    split(firstLine, " ", singleList);

    STL_FOR_LOOP(singleList, iter)
    {
       std::string str = *iter;
       if (str[0] == '/')
       {
           m_strPathWithParam = str;
       }
    }

    return TRUE;
}

BOOL CRestRequest::parsePathParam(const std::string & path)
{
    std::list<std::string> singleList;
    split(path, "/", singleList);

    STL_FOR_LOOP(singleList, iter)
    {
        std::string str = *iter;
        size_t split_begin_pos =  str.find(param_split_begin);

        if (std::string::npos != split_begin_pos)
        {
            str.replace(split_begin_pos, split_begin_pos + param_split_begin.size(), "");

            size_t split_end_pos = str.find(param_split_end);
            if (std::string::npos != split_end_pos) {
                str.replace(split_end_pos, split_end_pos + param_split_end.size(), "");
                m_pathParamList.push_back(str);
            }
            else {
                LOG_INFO("Url Parameter is Wrong. Please check the parameter end split.");
                return FALSE;
            }
        }
        else {
            if (iter != singleList.begin()) {
                m_strPath.append("/");
            }

            m_strPath.append(str);
        }
    }

    return TRUE;
}

BOOL CRestRequest::createHeader(bnc::restful::http_version version,
                                bnc::restful::http_method method,
                                bnc::restful::content_type type,
                                const std::string & path,
                                UINT4 content_length,
                                std::string & header)
{
    if ((bnc::restful::HTTP_VERSION_OHTER == version) || (bnc::restful::HTTP_METHOD_OTHER == method))
    {
        LOG_ERROR("create response failed. invalid version or method.");
        return FALSE;
    }

    std::string strVersion;
    std::string strMethod;
    std::string strContentType;

    CRestDefine::getInstance()->getStrFromMethod(method, strMethod);
    CRestDefine::getInstance()->getStrFromVersion(version, strVersion);
    CRestDefine::getInstance()->getStrFromContentType(type, strContentType);

    if ((strMethod.empty()) || (strVersion.empty()))
    {
        LOG_ERROR("create response failed. status or version string empty");
        return FALSE;
    }

    std::string strContentTypeSeg("Content-Type: ");
    std::string strContentLength("Content-Length: ");

    INT1 strLength[24] = {0};
    content_length = (0 == content_length) ? 500 : content_length;
    sprintf(strLength, "%d", content_length);

    header.append(strMethod);
    header.append(" ");
    header.append(path);
    header.append(" ");
    header.append(strVersion);
    header.append("\r\n");
    header.append(strContentTypeSeg);
    header.append(strContentType);
    header.append("\r\n");
    header.append(strContentLength);
    header.append(strLength);

    return TRUE;
}




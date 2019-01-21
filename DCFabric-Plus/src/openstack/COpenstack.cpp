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
*   File Name   : COpenstack.cpp            *
*   Author      : bnc xflu                  *
*   Create Date : 2016-8-31                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "COpenstack.h"
#include "log.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "COpenstackDefine.h"
#include "comm-util.h"
#include "CRestRequest.h"

using namespace rapidjson;

COpenstack::COpenstack()
{
}

COpenstack::~COpenstack()
{
}

COpenstack::COpenstack(UINT4 server_ip, UINT4 server_port_no,
        UINT4 token_ip, UINT4 token_port_no,
        const std::string & tenant,
        const std::string & user,
        const std::string & password):
        server_ip(server_ip),
        server_port_no(server_port_no),
        token_ip(token_ip),
        token_port_no(token_port_no),
        auth_tenant_name(tenant),
        auth_user_name(user),
        auth_password(password)
{
    if (0 == server_ip)
    {
        LOG_ERROR("Openstack ip is invalid.");
    }

    if (0 == server_port_no)
    {
        LOG_ERROR("Openstack port number is invalid.");
    }

    if (0 == token_ip)
    {
        LOG_ERROR("Openstack token ip is invalid.");
    }

    if (0 == token_port_no)
    {
        LOG_ERROR("Openstack token port number is invalid.");
    }

    if (tenant.empty())
    {
        LOG_ERROR("Openstack authentic tenant name is empty.");
    }

    if (user.empty())
    {
        LOG_ERROR("Openstack authentic user name is empty.");
    }

    if (password.empty())
    {
        LOG_ERROR("Openstack authentic password is empty.");
    }

    LOG_INFO_FMT("Create Openstack: server_ip:%d, server_port:%d, token_ip:%d, token_port:%d",
            server_ip, server_port_no, token_ip, token_port_no);

    // 初始化资源管理类
    m_resource = new COpenstackResource();
}

void COpenstack::init()
{
    // 什么都不做
}

void COpenstack::loadResources()
{
    LOG_INFO("Load Openstack Resource");

    std::string result;

	int t1= time(NULL);
    STL_FOR_LOOP(m_resource->getResourceList(), iter) 
    {
		m_resource->resetResourceData(*iter);
        if (loadResource(bnc::openstack::v2, *iter, result))
        {
			m_resource->parseResourceData(*iter, result);
			m_resource->clearResourceNode(*iter);
		}
    }
	int t2= time(NULL);
	LOG_DEBUG_FMT("Load Openstack Resource, T2-T1=%d",t2-t1);
}

INT4 COpenstack::connectServer(UINT4 ip, UINT4 port_no)
{
    INT4 sockfd = ::socket(AF_INET , SOCK_STREAM , 0);
    if(sockfd < 0 )
    {
       LOG_ERROR("create socket with openstack server failure");
       return 0;
    }

    struct sockaddr_in address;
    memset(&address , 0 , sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(port_no);
    address.sin_addr.s_addr = ip;

    if (::connect(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
       LOG_INFO("openstack server connect error.");
       return 0;
    }

    return sockfd;
}

BOOL COpenstack::createAuthData(std::string & path, std::string & body)
{
    /*
    * Create Openstack Auth param
    * like
    * http://192.168.56.200:35357/v2.0/tokens
    * {
    *     "auth":
    *     {
    *         "tenantName": "admin",
    *         "passwordCredentials":
    *         {
    *             "username": "admin",
    *             "password": "0ecddbf07f684e7c"
    *         }
    *     }
    * }
    */

    if ((0 == token_ip) || (0 ==  token_port_no))
    {
        LOG_INFO("Auth param error.");
        return FALSE;
    }

    /*
     * create auth uri
     */
    INT1 str_ip[32] = {0};
    number2ip(token_ip, str_ip);
    INT1 buf[10];
    sprintf(buf, "%d", token_port_no);
    std::string str_port = buf;

    path.append("http://");
    path.append(str_ip);
    path.append(":");
    path.append(str_port);
    path.append("/v2.0/tokens");

    /*
     * create auth body
     */
    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    {
        writer.Key("auth");

        writer.StartObject();
        {
            writer.Key("tenantName");
            writer.String(auth_tenant_name.c_str());

            writer.Key("passwordCredentials");
            writer.StartObject();
            {
                writer.Key("username");
                writer.String(auth_user_name.c_str());

                writer.Key("password");
                writer.String(auth_password.c_str());
            }
            writer.EndObject();
        }
        writer.EndObject();
    }
    writer.EndObject();

    body.append(strBuff.GetString());

    return TRUE;
}

BOOL COpenstack::readReplyData(INT4 sockfd, std::string & result)
{
    // 读取数据
    char buf[4096];
    fd_set fd_read;

    while (TRUE)
    {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&fd_read);
        FD_SET(sockfd, &fd_read);

        INT4 selValue = select(sockfd +1, &fd_read, NULL, NULL, &tv);

        if (selValue > 0)
        {
            memset(buf, 0, 4096);
            INT4 i= read(sockfd, buf, 4095);
            result.append(buf);
            if (0 == i)
            {
              return TRUE;
            }
        }
        else if (selValue < 0)
        {
            LOG_INFO("Fail to select data.");
            return FALSE;
        }
    }

    return TRUE;
}

BOOL COpenstack::parseAuthData(const std::string & result)
{
    Document document;
    if (document.Parse(result.c_str()).HasParseError())
    {
        LOG_INFO("Auth result has error.");
        return FALSE;
    }

    if (document.HasMember("access"))
    {
        if (document["access"].HasMember("token"))
        {
            if (document["access"]["token"].HasMember("id"))
            {
                auth_token_id = document["access"]["token"]["id"].GetString();
                return TRUE;
            }
        }
    }

    LOG_INFO("Fail to find token id in result.");

    return FALSE;
}

BOOL COpenstack::authenticate()
{
    // define values
    std::string path;
    std::string body;
    std::string res;
    std::string result;

    // 连接认证服务器
    INT4 sockfd = connectServer(token_ip, token_port_no);
    if (0 == sockfd)
    {
        LOG_INFO("Openstack authenticate failed, Can't connect the server.");
        return FALSE;
    }

    // 创建认证数据
    if (FALSE == createAuthData(path, body))
    {
        LOG_INFO("Openstack authenticate failed. Can't create auth data.");
        return FALSE;
    }

    // 创建认证用的Http请求数据
    CRestRequest* request = new CRestRequest(bnc::restful::HTTP_1_0,
            bnc::restful::HTTP_POST, bnc::restful::CONTENT_JSON, path, body);

    // 发送Http请求数据
    INT4 ret = send(sockfd, request->getRaw(res).c_str(), request->getRaw(res).size(), 0);

    delete request;

    if (ret < 0)
    {
        LOG_INFO("Openstack authenticate failed. Fail to send auth data.");
        return FALSE;
    }

    // 读取认证结果
    readReplyData(sockfd, result);

    // 关闭连接
    close(sockfd);

    // LOG_INFO_FMT("result is %s", result.c_str());

    // 解析认证结果
    CRestResponse* response = new CRestResponse(result);
    if (FALSE == parseAuthData(response->getBody()))
    {
        LOG_INFO("Openstack authenticate failed. Fail to parse token id.");
        delete response;
        return FALSE;
    }

    delete response;
    return TRUE;
}


BOOL COpenstack::createResourceData(bnc::openstack::version version,
        bnc::openstack::network_type type, std::string & path, std::string & body)
{
    /*
    * Create Openstack resource param
    * like
    * http://192.168.53.94:9696/v2.0/ports
    * X-Auth-Token: 1234567890
    *
    * {
    * }
    */

    if ((0 == server_ip) || (0 ==  server_port_no))
    {
        LOG_INFO("resource param error.");
        return FALSE;
    }

    /*
     * create resourse uri
     */
    INT1 str_ip[32] = {0};
    number2ip(server_ip, str_ip);
    INT1 buf[10];
    sprintf(buf, "%d", server_port_no);
    std::string str_port = buf;
    std::string str_uri;
    COpenstackDefine::getInstance()->getStrFromNetwork(version, type, str_uri);

    if (str_uri.empty())
    {
        LOG_INFO("resource uri is not exist.");
        return FALSE;
    }

    path.append("http://");
    path.append(str_ip);
    path.append(":");
    path.append(str_port);
    path.append(str_uri);

    /*
     * create resource body
     */
    body.append("{}");

    return TRUE;
}

BOOL COpenstack::loadResource(bnc::openstack::version version, bnc::openstack::network_type type, std::string& str)
{
    std::string path;
    std::string body;
    std::string res;
    std::string result;
	BOOL bResult = TRUE;

	LOG_INFO_FMT("loadResource %d", type);

    INT4 sockfd = connectServer(server_ip, server_port_no);
    if (0 == sockfd)
    {
		
		LOG_ERROR("can't connect server");
        return FALSE;
    }

    if (FALSE == authenticate())
    {
		LOG_ERROR("can't pass auth");
        return FALSE;
    }

    if (FALSE == createResourceData(version, type, path, body))
    {
		LOG_ERROR("can't create resource data");
        return FALSE;
    }

    // 创建认证用的Http请求数据
    CRestRequest* request = new CRestRequest(bnc::restful::HTTP_1_0,
          bnc::restful::HTTP_GET, bnc::restful::CONTENT_JSON, path, body);

    request->setToken(auth_token_id);

    //LOG_INFO_FMT("send data is %s", request->getRaw(res).c_str());

    // 发送Http请求数据
    INT4 ret = send(sockfd, request->getRaw(res).c_str(), request->getRaw(res).size(), 0);

    delete request;

    if (ret < 0)
    {
      LOG_INFO("Openstack authenticate failed. Fail to send auth data.");
      return FALSE;
    }

    // 读取认证结果
    bResult = readReplyData(sockfd, result);

    // 关闭连接
    close(sockfd);

    //LOG_INFO_FMT("result is %s", result.c_str());

    // 解析认证结果
    CRestResponse* response = new CRestResponse(result);

    str = response->getBody();

    delete response;
    return bResult;
}


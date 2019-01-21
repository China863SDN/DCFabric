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
*   File Name   : CRestApi.h                                                  *
*   Author      : bnc xflu           		                                  *
*   Create Date : 2016-7-22           		                                  *
*   Version     : 1.0           			                                  *
*   Function    : .           				                                  *
*                                                                             *
******************************************************************************/
#ifndef _CRESTAPI_H
#define _CRESTAPI_H

#include "bnc-type.h"
#include "bnc-param.h"
#include "CRestRequest.h"
#include "CRestResponse.h"

/*
 * RestAPI handler template
 * use function pointer to register
 */
typedef void(*restApiHandler)(CRestRequest* request, CRestResponse* response);

/*
 * 定义了所有使用的RestAPI
 */
class CRestApi
{
public:
    /*
     * 测试�?
     * 会返回和请求相同的Body
     */
    static void api_echo(CRestRequest* request, CRestResponse* response);

    /*
     * 测试�?
     * 会列出所有已经注册的Api
     */
    static void api_list(CRestRequest* request, CRestResponse* response);

    /*
     * 测试�?
     * 会列出所有已经注册的Api的详细信�?
     */
    static void api_list_detail(CRestRequest* request, CRestResponse* response);

    /*
     * 测试�?
     * 默认处理函数, 如果请求的path不存�? 会调用这个函�?
     */
    static void api_default(CRestRequest* request, CRestResponse* response);

    /*
     * 测试�?
     * 一个简单的json数据dom的转换和使用
     */
    static void api_json_example(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的交换机信�?
     */
    static void api_switches(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的主机信息
     */
    static void api_hosts(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的网络信息
     */
    static void api_networks(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的子网信息
     */
    static void api_subnets(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的浮动ip信息
     */
    static void api_floatingips(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的路由器信�?
     */
    static void api_routers(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的外联口信�?
     */
    static void api_externalports(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的管理交换机信�?
     */
    static void api_manageswitches(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的拓扑信息
     */
    static void api_topolinks(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的安全组信�?
     */
    static void api_securitygroups(CRestRequest* request, CRestResponse* response);

    /*
     * 列出所有的端口转发信息
     */
    static void api_portforwards(CRestRequest* request, CRestResponse* response);

	/*
	 * 列出所有的qos rules
	 */
	static void api_qosrules(CRestRequest* request, CRestResponse* response);

	/*
	 * 列出所有的qos 绑定 port
	 */
	static void api_qosbindports(CRestRequest* request, CRestResponse* response);

	static void api_nathosts(CRestRequest* request, CRestResponse* response);

	static void api_cluster(CRestRequest* request, CRestResponse* response);

    static void api_flowentries(CRestRequest* request, CRestResponse* response);
};

#endif

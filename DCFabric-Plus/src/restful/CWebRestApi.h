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
*   File Name   : CWebRestApi.h                                               *
*   Author      : bnc xflu                                                    *
*   Create Date : 2016-8-31                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#ifndef _CRESTWEBAPI_H
#define _CRESTWEBAPI_H

#include "CRestApi.h"

/*
 * ��������webҳ������Ҫʹ�õ�RestApi
 */
class CWebRestApi
{
public:
	/*
	 * ȡ�����еĽ�������Ϣ
	 * ����ԭ��api
	 */
    static void api_get_all_switch_info(CRestRequest* request, CRestResponse* response);

    static void api_get_switch_info(CRestRequest* request, CRestResponse* response);

    static void api_get_topo_link(CRestRequest* request, CRestResponse* response);

    static void api_get_topo_link_info(CRestRequest* request, CRestResponse* response);

    static void api_get_host_by_dpid(CRestRequest* request, CRestResponse* response);

	/*
	 *ͨ��API����Openstack external��Ϣ
	 */
    static void api_setup_fabric_external(CRestRequest* request, CRestResponse* response);

	/*
	 * ͨ��APIɾ��Openstack external
	 */
    static void api_delete_fabric_external(CRestRequest* request, CRestResponse* response);

	/*
	 * ��ȡOpenstack External�����Ϣ
	 */
    static void api_get_fabric_external(CRestRequest* request, CRestResponse* response);

    /*
     * ��ȡOpenstack Floatingip�����Ϣ
     */
    static void api_get_fabric_floatingip(CRestRequest* request, CRestResponse* response);

    /*
     * ��ȡOpenstack security group�����Ϣ
     */
    static void api_get_fabric_security_group(CRestRequest* request, CRestResponse* response);

    /*
     * ��ȡOpenstack network�����Ϣ
     */
    static void api_get_fabric_network(CRestRequest* request, CRestResponse* response);

    /*
     * ��ȡOpenstack subnet�����Ϣ
     */
    static void api_get_fabric_subnet(CRestRequest* request, CRestResponse* response);

	/*
	 * ��ȡOpenstack qos bind�����Ϣ
	 */
	static void api_get_qosbind_ports(CRestRequest* request, CRestResponse* response);

	/*
	 * ��ȡOpenstack qos rule�����Ϣ
	 */
	static void api_get_qosrules(CRestRequest* request, CRestResponse* response);

    static void api_extend_sws_get(CRestRequest* request, CRestResponse* response);
    static void api_extend_sws_post(CRestRequest* request, CRestResponse* response);
    static void api_extend_sws_delete(CRestRequest* request, CRestResponse* response);

    static void api_extend_hosts_get(CRestRequest* request, CRestResponse* response);
    static void api_extend_hosts_post(CRestRequest* request, CRestResponse* response);
    static void api_extend_hosts_delete(CRestRequest* request, CRestResponse* response);

	static void api_extend_links_get(CRestRequest* request, CRestResponse* response);
	static void api_extend_links_post(CRestRequest* request, CRestResponse* response);
	static void api_extend_links_delete(CRestRequest* request, CRestResponse* response);
	
	static void api_extend_mediaflow_delete(CRestRequest* request, CRestResponse* response);
    static void api_get_flow_entries(CRestRequest* request, CRestResponse* response);
    static void api_post_flow_entry(CRestRequest* request, CRestResponse* response);
    static void api_delete_flow_entry(CRestRequest* request, CRestResponse* response);

    // ...
};

#endif

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
*   File Name   : CRestApiHandler.cpp          *
*   Author      : bnc xflu           		*
*   Create Date : 2016-7-21           		*
*   Version     : 1.0           			*
*   Function    : .           				*
*                                                                             *
******************************************************************************/
#include "CRestApiMgr.h"
#include "CRestManager.h"
#include "CWebRestApi.h"
#include "COpenstackRestApi.h"
#include "comm-util.h"
#include "../service/Service_NetworkingConfig_byRestful.h"
#include "log.h"

CRestApiMgr* CRestApiMgr::m_pInstance = 0;

CRestApiMgr::CRestApiMgr()
{
}

CRestApiMgr::~CRestApiMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

CRestApiMgr* CRestApiMgr::getInstance()
{
	if (NULL == m_pInstance) {
		m_pInstance = new CRestApiMgr();
		m_pInstance->init();
	}

	return (m_pInstance);
}

INT4 CRestApiMgr::getRestApiCount()
{
	return (m_restApiList.size());
}

BOOL CRestApiMgr::getRestApiPathList(std::list<std::string> & list)
{
	STL_FOR_LOOP(m_restApiList, iter)
	{
		list.push_back(iter->first.first);
	}

	list.sort();
	list.unique();

	return TRUE;
}

BOOL CRestApiMgr::getRestApiPathList(std::list<restVar> & list)
{
	STL_FOR_LOOP(m_restApiList, iter)
	{
		list.push_back(iter->first);
	}

	return TRUE;
}

BOOL CRestApiMgr::registerRestApi(bnc::restful::http_method method,
								  const std::string & path, restApiHandler handler)
{
	LOG_INFO_FMT("register RestApi %s", path.c_str());

	if (bnc::restful::HTTP_ANY == method)
	{
		method_map methods = CRestManager::getInstance()->getRestDefine()->getMethodList();

		STL_FOR_LOOP(methods, iter)
		{
			m_restApiList.insert(std::make_pair(std::make_pair(path, iter->first), handler));
		}
	}
	else
	{
		m_restApiList.insert(std::make_pair(std::make_pair(path, method), handler));
	}

	return TRUE;
}

BOOL CRestApiMgr::registerDefaultRestApi()
{
    /*
     * RestApi part
     */
	registerRestApi(bnc::restful::HTTP_GET,    "/list",           CRestApi::api_list);
	registerRestApi(bnc::restful::HTTP_GET,    "/list/detail",    CRestApi::api_list_detail);
	registerRestApi(bnc::restful::HTTP_ANY,    "/echo",           CRestApi::api_echo);
	registerRestApi(bnc::restful::HTTP_GET,    "/default",        CRestApi::api_default);
	registerRestApi(bnc::restful::HTTP_GET,    "/json/example",   CRestApi::api_json_example);
	registerRestApi(bnc::restful::HTTP_GET,    "/switches",       CRestApi::api_switches);
	registerRestApi(bnc::restful::HTTP_GET,    "/sws",            CRestApi::api_switches);
	registerRestApi(bnc::restful::HTTP_GET,    "/hosts",          CRestApi::api_hosts);
	registerRestApi(bnc::restful::HTTP_GET,    "/networks",       CRestApi::api_networks);
	registerRestApi(bnc::restful::HTTP_GET,    "/subnets",        CRestApi::api_subnets);
	registerRestApi(bnc::restful::HTTP_GET,    "/floatingips",    CRestApi::api_floatingips);
	registerRestApi(bnc::restful::HTTP_GET,    "/routers",        CRestApi::api_routers);
	registerRestApi(bnc::restful::HTTP_GET,    "/externalports",  CRestApi::api_externalports);
	registerRestApi(bnc::restful::HTTP_GET,    "/managesws",      CRestApi::api_manageswitches);
	registerRestApi(bnc::restful::HTTP_GET,    "/topolinks",      CRestApi::api_topolinks);
	registerRestApi(bnc::restful::HTTP_GET,    "/securitygroups", CRestApi::api_securitygroups);
	registerRestApi(bnc::restful::HTTP_GET,    "/portforwards",   CRestApi::api_portforwards);
	registerRestApi(bnc::restful::HTTP_GET,    "/qos_rules",      CRestApi::api_qosrules);
	registerRestApi(bnc::restful::HTTP_GET,    "/qos_ports",      CRestApi::api_qosbindports);
	registerRestApi(bnc::restful::HTTP_GET,    "/nathosts",       CRestApi::api_nathosts);
	registerRestApi(bnc::restful::HTTP_GET,    "/cluster",        CRestApi::api_cluster);
	registerRestApi(bnc::restful::HTTP_GET,    "/flowentries",    CRestApi::api_flowentries);

    /*
     * WebRestApi part
     */
	registerRestApi(bnc::restful::HTTP_GET,    "/gn/switchinfo/json",           CWebRestApi::api_get_all_switch_info);
	registerRestApi(bnc::restful::HTTP_GET,    "/gn/switch/json",               CWebRestApi::api_get_switch_info);
	registerRestApi(bnc::restful::HTTP_GET,    "/gn/topo/links/json",           CWebRestApi::api_get_topo_link_info);
	registerRestApi(bnc::restful::HTTP_GET,    "/dcf/get/host/json",            CWebRestApi::api_get_host_by_dpid);
	registerRestApi(bnc::restful::HTTP_GET,    "/bnc/external/sws",             CWebRestApi::api_extend_sws_get);
	registerRestApi(bnc::restful::HTTP_POST,   "/bnc/external/sws",             CWebRestApi::api_extend_sws_post);
	registerRestApi(bnc::restful::HTTP_DELETE, "/bnc/external/sws",             CWebRestApi::api_extend_sws_delete);
	registerRestApi(bnc::restful::HTTP_GET,    "/bnc/external/hosts",           CWebRestApi::api_extend_hosts_get);
	registerRestApi(bnc::restful::HTTP_POST,   "/bnc/external/hosts",           CWebRestApi::api_extend_hosts_post);
	registerRestApi(bnc::restful::HTTP_DELETE, "/bnc/external/hosts",           CWebRestApi::api_extend_hosts_delete);
	registerRestApi(bnc::restful::HTTP_GET,    "/bnc/external/links", 			CWebRestApi::api_extend_links_get);
	registerRestApi(bnc::restful::HTTP_POST,   "/bnc/external/links", 			CWebRestApi::api_extend_links_post);
	registerRestApi(bnc::restful::HTTP_DELETE, "/bnc/external/links", 			CWebRestApi::api_extend_links_delete);
	registerRestApi(bnc::restful::HTTP_DELETE, "/bnc/extend/mediaflowcut", 	    CWebRestApi::api_extend_mediaflow_delete);
	registerRestApi(bnc::restful::HTTP_GET,    "/dcf/debug/qosrules",           CWebRestApi::api_get_qosrules);
	registerRestApi(bnc::restful::HTTP_GET,    "/dcf/debug/qosbinds",           CWebRestApi::api_get_qosbind_ports);
    registerRestApi(bnc::restful::HTTP_POST,   "/gn/fabric/external/json",      CWebRestApi::api_setup_fabric_external);
    registerRestApi(bnc::restful::HTTP_DELETE, "/gn/fabric/external/json",      CWebRestApi::api_delete_fabric_external);
    registerRestApi(bnc::restful::HTTP_GET,    "/gn/fabric/external/json",      CWebRestApi::api_get_fabric_external);
    registerRestApi(bnc::restful::HTTP_GET,    "/gn/fabric/floatingip/json",    CWebRestApi::api_get_fabric_floatingip);
    registerRestApi(bnc::restful::HTTP_GET,    "/gn/fabric/securitygroup/json", CWebRestApi::api_get_fabric_security_group);
    registerRestApi(bnc::restful::HTTP_GET,    "/gn/fabric/network/json",       CWebRestApi::api_get_fabric_network);
    registerRestApi(bnc::restful::HTTP_GET,    "/gn/fabric/subnet/json",        CWebRestApi::api_get_fabric_subnet);
    registerRestApi(bnc::restful::HTTP_GET,    "/gn/flow/json",                 CWebRestApi::api_get_flow_entries);
    registerRestApi(bnc::restful::HTTP_POST,   "/gn/flow/json",                 CWebRestApi::api_post_flow_entry);
    registerRestApi(bnc::restful::HTTP_DELETE, "/gn/flow/json",                 CWebRestApi::api_delete_flow_entry);
	
    /*
     * OpenstackRestApi part
     */
	registerRestApi(bnc::restful::HTTP_GET,    "/gn/neutron/networks", COpenstackRestApi::api_neutron_networks_get);
	registerRestApi(bnc::restful::HTTP_POST,   "/gn/neutron/networks", COpenstackRestApi::api_neutron_networks_post);
	registerRestApi(bnc::restful::HTTP_PUT,    "/gn/neutron/networks", COpenstackRestApi::api_neutron_networks_put);
	registerRestApi(bnc::restful::HTTP_DELETE, "/gn/neutron/networks", COpenstackRestApi::api_neutron_networks_delete);
	registerRestApi(bnc::restful::HTTP_GET,    "/gn/neutron/subnets",  COpenstackRestApi::api_neutron_subnets_get);
	registerRestApi(bnc::restful::HTTP_POST,   "/gn/neutron/subnets",  COpenstackRestApi::api_neutron_subnets_post);
	registerRestApi(bnc::restful::HTTP_PUT,    "/gn/neutron/subnets",  COpenstackRestApi::api_neutron_subnets_put);
	registerRestApi(bnc::restful::HTTP_DELETE, "/gn/neutron/subnets",  COpenstackRestApi::api_neutron_subnets_delete);
	registerRestApi(bnc::restful::HTTP_GET,    "/gn/neutron/ports",    COpenstackRestApi::api_neutron_ports_get);
	registerRestApi(bnc::restful::HTTP_POST,   "/gn/neutron/ports",    COpenstackRestApi::api_neutron_ports_post);
	registerRestApi(bnc::restful::HTTP_PUT,    "/gn/neutron/ports",    COpenstackRestApi::api_neutron_ports_put);
	registerRestApi(bnc::restful::HTTP_DELETE, "/gn/neutron/ports",    COpenstackRestApi::api_neutron_ports_delete);

    /*
     * NetworkingRestApi part
     */
	registerRestApi(bnc::restful::HTTP_GET, 	"/networking/floatings", CNetworkingRestApi::GET_R_Floating);
	registerRestApi(bnc::restful::HTTP_POST, 	"/networking/floatings", CNetworkingRestApi::POST_C_Floating);
	registerRestApi(bnc::restful::HTTP_PUT, 	"/networking/floating",  CNetworkingRestApi::PUT_U_Floating);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/floating",  CNetworkingRestApi::DELETE_D_Floating);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/floatings", CNetworkingRestApi::DELETE_D_Floatings);
	registerRestApi(bnc::restful::HTTP_GET, 	"/networking/networks",  CNetworkingRestApi::GET_R_Network);
	registerRestApi(bnc::restful::HTTP_POST, 	"/networking/networks",  CNetworkingRestApi::POST_C_Network);
	registerRestApi(bnc::restful::HTTP_PUT, 	"/networking/network", 	 CNetworkingRestApi::PUT_U_Network);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/network",   CNetworkingRestApi::DELETE_D_Network);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/networks",  CNetworkingRestApi::DELETE_D_Networks);
	registerRestApi(bnc::restful::HTTP_GET, 	"/networking/routers", 	 CNetworkingRestApi::GET_R_Router);
	registerRestApi(bnc::restful::HTTP_POST, 	"/networking/routers",   CNetworkingRestApi::POST_C_Router);
	registerRestApi(bnc::restful::HTTP_PUT, 	"/networking/router", 	 CNetworkingRestApi::PUT_U_Router);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/router", 	 CNetworkingRestApi::DELETE_D_Router);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/routers", 	 CNetworkingRestApi::DELETE_D_Routers);
	registerRestApi(bnc::restful::HTTP_GET, 	"/networking/subnets",   CNetworkingRestApi::GET_R_Subnet);
	registerRestApi(bnc::restful::HTTP_POST, 	"/networking/subnets",   CNetworkingRestApi::POST_C_Subnet);
	registerRestApi(bnc::restful::HTTP_PUT, 	"/networking/subnet", 	 CNetworkingRestApi::PUT_U_Subnet);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/subnet", 	 CNetworkingRestApi::DELETE_D_Subnet);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/subnets", 	 CNetworkingRestApi::DELETE_D_Subnets);
	registerRestApi(bnc::restful::HTTP_GET, 	"/networking/ports", 	 CNetworkingRestApi::GET_R_Port);
	registerRestApi(bnc::restful::HTTP_POST, 	"/networking/ports", 	 CNetworkingRestApi::POST_C_Port);
	registerRestApi(bnc::restful::HTTP_PUT, 	"/networking/port", 	 CNetworkingRestApi::PUT_U_Port);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/port", 	 CNetworkingRestApi::DELETE_D_Port);
	registerRestApi(bnc::restful::HTTP_DELETE, 	"/networking/ports", 	 CNetworkingRestApi::DELETE_D_Ports);
	
	return TRUE;
}

BOOL CRestApiMgr::isExist(const restVar & path)
{
	return STL_EXIST(m_restApiList, path);
}

restApiHandler CRestApiMgr::findRestApi(const restVar & var)
{
//	restApi::iterator iter = m_restApiList.find(var);
//
//	if (iter != m_restApiList.end())
//	{
//		return iter->second;
//	}

    /*
     * 为兼容openstack的RestApi
     * 使用这种方式
     * 比较url字符串的前多少位
     * 如果与注册的Api的Url相同
     * 则认为是相同的
     */
    STL_FOR_LOOP(m_restApiList, it)
    {
        restVar res = it->first;

        if (res.second == var.second) {
            if (0 == memcmp(var.first.c_str(), res.first.c_str(), res.first.size()))
            {
                return it->second;
            }
        }
    }
	return NULL;
}

BOOL CRestApiMgr::init()
{
	// clear rest api
	m_restApiList.clear();

	// register default restapi
	registerDefaultRestApi();

	return TRUE;
}

BOOL CRestApiMgr::process(CRestRequest* request, CRestResponse* response)
{
	LOG_IF_RETURN (NULL == request, FALSE,
			"fail to deal with restapi, request is empty.");

	LOG_IF_RETURN (NULL == response, FALSE,
			"fail to deal with restapi, response is empty.");

	LOG_IF_RETURN (request->getPath().empty(), FALSE,
			"fail to deal with restapi, request path is empty.");

	// find RestApi by path
	restApiHandler handler = findRestApi(std::make_pair(request->getPath(), request->getMethod()));
	if (NULL == handler)
	{
		// set default handler
		handler = findRestApi(std::make_pair("/default", bnc::restful::HTTP_GET));
    	if (NULL == handler)
    	{
            LOG_ERROR_FMT("fail to find default hander %s", request->getPath().c_str());
            return FALSE;
    	}
	}

	// call the register function
	try
	{
		(*handler)(request, response);
	}
	catch(...)
	{
		LOG_ERROR("Exception happen ");
	}

	return TRUE;
}


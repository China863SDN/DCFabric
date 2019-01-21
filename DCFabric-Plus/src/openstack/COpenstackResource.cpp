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
*   File Name   : COpenstackResource.cpp    *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "COpenstackDefine.h"
#include "COpenstackResource.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "log.h"
#include "CNotifyMgr.h"
#include "comm-util.h"
#include "COpenstackMgr.h"

#include "BaseFloatingConventor.h"
#include "BaseNetworkConventor.h"
#include "BaseSubnetConventor.h"
#include "BasePortConventor.h"
#include "CQosEventReport.h"
#include "CSecurityGroupEventReportor.h"
#include "json.h"
#include "CHostMgr.h"
#include "CSecurityGroupEventReportor.h"
#include "CPortforwardEventReportor.h"

using namespace rapidjson;

COpenstackResource::COpenstackResource()
{
    init();
}

COpenstackResource::~COpenstackResource()
{
}


void COpenstackResource::init()
{
    // 初始化需要读取的列表
   m_resourceList.push_back(bnc::openstack::networks);
   m_resourceList.push_back(bnc::openstack::subnets);
   m_resourceList.push_back(bnc::openstack::ports);
   m_resourceList.push_back(bnc::openstack::floatingips);
   m_resourceList.push_back(bnc::openstack::qosrules);
   m_resourceList.push_back(bnc::openstack::securitygrouprules);
   m_resourceList.push_back(bnc::openstack::routers);

}

void COpenstackResource::resetOpenstackNetworkList()
{
	 STL_FOR_LOOP(m_network_list,iter)
	 {
	 	iter->second->setCheckStatus(CHECK_UNCHECKED);
	 }
}
void COpenstackResource::resetOpenstackSubnetList()
{
	STL_FOR_LOOP(m_subnet_list,iter)
	{
		iter->second->setCheckStatus(CHECK_UNCHECKED);
	}
}
void COpenstackResource::resetOpenstackPortList()
{
	STL_FOR_LOOP(m_port_list,iter)
	{
		iter->second->setCheckStatus(CHECK_UNCHECKED);
	}
}
void COpenstackResource::resetOpenstackFloatingIpList()
{
	STL_FOR_LOOP(m_floatingip_list,iter)
	{
		iter->second->setCheckStatus(CHECK_UNCHECKED);
	}
}
void COpenstackResource::resetOpenstackSecurityGroupList()
{
	STL_FOR_LOOP(m_securitygroup_list,iter)
	{
		iter->second->setCheckStatus(CHECK_UNCHECKED);
	}
}
void COpenstackResource::resetOpenstackRouterList()
{
	STL_FOR_LOOP(m_router_list,iter)
	{
		iter->second->setCheckStatus(CHECK_UNCHECKED);
	}
}
void COpenstackResource::resetOpenstackQosRuleList()
{
	STL_FOR_LOOP(m_qosrule_list,iter)
	{
		iter->second->setCheckStatus(CHECK_UNCHECKED);
	}
}

void COpenstackResource::resetOpenstackQosBindList()
{
	STL_FOR_LOOP(m_qosbind_list,iter)
	{
		iter->second->setCheckStatus(CHECK_UNCHECKED);
	}
}

void COpenstackResource::clearOpenstackNetworkNode()
{
	std::map<string, COpenstackNetwork*>::iterator it;
	STL_FOR_MAP(m_network_list,iter)
	{
		it = iter;
		iter++;
		if(CHECK_UNCHECKED == it->second->getCheckStatus())
		{
			Base_Network* NeedDeleteNetwork =G_NetworkMgr.targetNetwork_ByID(it->second->getId());
            if(NeedDeleteNetwork)
            {
                G_NetworkingEventReporter.report_D_Network(NeedDeleteNetwork,PERSTANCE_FALSE);
            }
			
			//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyDelNetwork(it->second->getId());
			delete it->second;
			m_network_list.erase(it);
			
		}
	}
}
void COpenstackResource::clearOpenstackSubnetNode()
{
	std::map<string, COpenstackSubnet*>::iterator it;
	STL_FOR_MAP(m_subnet_list,iter)
	{
		it = iter;
		iter++;
		if(CHECK_UNCHECKED == it->second->getCheckStatus())
		{
			Base_Subnet* NeedDeleteSubnet =G_SubnetMgr.targetSubnet_ByID(it->second->getId());
			if(NeedDeleteSubnet)
			{
				G_NetworkingEventReporter.report_D_Subnet(NeedDeleteSubnet,PERSTANCE_FALSE);
			}
            
        	//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyDelSubnet(it->second->getId());
			delete it->second;
			m_subnet_list.erase(it);
		}
	}
}
void COpenstackResource::clearOpenstackPortNode()
{
	COpenstackPortMap::iterator iter = m_port_list.begin();
	while (iter != m_port_list.end())
	{
		COpenstackPortMap::iterator it = iter++;
		if (CHECK_UNCHECKED == it->second->getCheckStatus())
		{
			Base_Port* NeedDeletePort =G_PortMgr.targetPort_ByID(it->second->getId());
			if(NeedDeletePort)
			{
				G_NetworkingEventReporter.report_D_Port(NeedDeletePort,PERSTANCE_FALSE);
			}
			
			 // 通知主机新增
        	//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyDelPort(it->second->getId());

			m_port_list.erase(it);
		}
	}
}
void COpenstackResource::clearOpenstackFloatingIpNode()
{
	std::map<string, COpenstackFloatingip*>::iterator it;
	STL_FOR_MAP(m_floatingip_list,iter)
	{
		it = iter;
		iter++;
		if(CHECK_UNCHECKED == it->second->getCheckStatus())
		{
			Base_Floating * NeedDeleteFloating =G_FloatingMgr.targetFloating_ByID(it->second->getId());
			if(NeedDeleteFloating)
			{
				G_NetworkingEventReporter.report_D_Floating(NeedDeleteFloating,PERSTANCE_FALSE);
			}
			
			
        	//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyDelFloatingIp(it->second->getId());
			delete it->second;
			m_floatingip_list.erase(it);
		}
	}
}
void COpenstackResource::clearOpenstackSecurityGroupNode()
{
	COpenstackSecurityGroupMap::iterator iter = m_securitygroup_list.begin();
	while (iter != m_securitygroup_list.end())
	{
		COpenstackSecurityGroupMap::iterator it = iter++;
		if (CHECK_UNCHECKED == it->second->getCheckStatus())
			m_securitygroup_list.erase(it);
	}
}
void COpenstackResource::clearOpenstackRouterNode()
{
	COpenstackRouterMap::iterator iter = m_router_list.begin();
	while (iter != m_router_list.end())
	{
		COpenstackRouterMap::iterator it = iter++;
		if (CHECK_UNCHECKED == it->second->getCheckStatus())
			m_router_list.erase(it);
	}
}
void COpenstackResource::clearOpenstackQosRuleNode()
{
	std::map<string, COpenstackQosRule*>::iterator it;
	STL_FOR_MAP(m_qosrule_list, iter)
	{
		it = iter;
		iter++;
		if(CHECK_UNCHECKED == it->second->getCheckStatus())
		{
			delete it->second;
			m_qosrule_list.erase(it);
		}
	}
}

void COpenstackResource::clearOpenstackQosBindNode()
{
	std::map<string, COpenstackQosBind*>::iterator it;
	STL_FOR_MAP(m_qosbind_list, iter)
	{
		it = iter;
		iter++;
		if(CHECK_UNCHECKED == it->second->getCheckStatus())
		{
			delete it->second;
			m_qosbind_list.erase(it);
		}
	}
}

void COpenstackResource::resetResourceData(bnc::openstack::network_type type)
{
	switch (type)
    {
    case bnc::openstack::networks:
        resetOpenstackNetworkList();
        break;
    case bnc::openstack::subnets:
        resetOpenstackSubnetList();
        break;
    case bnc::openstack::ports:
        resetOpenstackPortList();
		resetOpenstackQosBindList();
        break;
    case bnc::openstack::floatingips:
        resetOpenstackFloatingIpList();
        break;
    case bnc::openstack::securitygrouprules:
        resetOpenstackSecurityGroupList();
        break;
    case bnc::openstack::routers:
        resetOpenstackRouterList();
        break;
	case bnc::openstack::qosrules:
		resetOpenstackQosRuleList();
		break;
    default:
        break;
    }

    return ;
}

void COpenstackResource::clearResourceNode(bnc::openstack::network_type type)
{
	switch (type)
    {
    case bnc::openstack::networks:
        clearOpenstackNetworkNode();
        break;
    case bnc::openstack::subnets:
        clearOpenstackSubnetNode();
        break;
    case bnc::openstack::ports:
        clearOpenstackPortNode();
		clearOpenstackQosBindNode();
        break;
    case bnc::openstack::floatingips:
        clearOpenstackFloatingIpNode();
        break;
    case bnc::openstack::securitygrouprules:
        clearOpenstackSecurityGroupNode();
        break;
    case bnc::openstack::routers:
        clearOpenstackRouterNode();
        break;
	case bnc::openstack::qosrules:
		clearOpenstackQosRuleNode();
		break;
    default:
        break;
    }
}

BOOL COpenstackResource::parseResourceData(bnc::openstack::network_type type,
                       const std::string & result)
{
	BOOL bResult = TRUE;
    switch (type)
    {
    case bnc::openstack::networks:
        bResult = parseNetworkData(result);
        break;
    case bnc::openstack::subnets:
        bResult = parseSubnetData(result);
        break;
    case bnc::openstack::ports:
        bResult = parsePortData(result);
        break;
    case bnc::openstack::floatingips:
        bResult = parseFloatingipData(result);
        break;
    case bnc::openstack::securitygrouprules:
        bResult = parseSecurityGroupData(result);
        break;
    case bnc::openstack::routers:
        bResult = parseRouterData(result);
        break;
	case bnc::openstack::qosrules:
		bResult = parseQosRuleData(result);
		break;
    default:
		bResult = FALSE;
        break;
    }

    return bResult;
}


BOOL COpenstackResource::parseNetworkData(const std::string & result)
{
    //LOG_WARN_FMT("\n==>Parse Network Data:\n##%s", result.c_str());
	/*
	 * parse data example:
	 * http://developer.openstack.org/api-ref/networking/v2
	 *
	 * {
			"networks": [
				{
					"status": "ACTIVE",
					"subnets": [
						"54d6f61d-db07-451c-9ab3-b9609b6b6f0b"
					],
					"name": "private-network",
					"provider:physical_network": null,
					"admin_state_up": true,
					"tenant_id": "4fd44f30292945e481c7b8a0c8908869",
					"qos_policy_id": "6a8454ade84346f59e8d40665f878b2e",
					"provider:network_type": "local",
					"router:external": true,
					"mtu": 0,
					"shared": true,
					"id": "d32019d3-bc6e-4319-9c1d-6722fc136a22",
					"provider:segmentation_id": null
				},
				{
					"status": "ACTIVE",
					"subnets": [
						"08eae331-0402-425a-923c-34f7cfe39c1b"
					],
					"name": "private",
					"provider:physical_network": null,
					"admin_state_up": true,
					"tenant_id": "26a7980765d0414dbc1fc1f88cdb7e6e",
					"qos_policy_id": "bfdb6c39f71e4d44b1dfbda245c50819",
					"provider:network_type": "local",
					"router:external": true,
					"mtu": 0,
					"shared": true,
					"id": "db193ab3-96e3-4cb3-8fc5-05f4296d0324",
					"provider:segmentation_id": null
				}
			]
		}
	 */
	Document document;
	if (document.Parse(result.c_str()).HasParseError())
	{
	  LOG_INFO("Openstack network Data has error.");
	  return FALSE;
	}

	if (document.HasMember("networks") && (document["networks"].IsArray()))
	{
		Value& networks_array = document["networks"];

		UINT4 index = 0;

		for (; index < networks_array.Size(); ++index)
		{
			COpenstackNetwork* network = new COpenstackNetwork();

			const Value& networks_object = networks_array[index];

			if (networks_object.HasMember("status") && networks_object["status"].IsString())
			{
				// LOG_INFO_FMT("status is %s", networks_object["status"].GetString());
				network->setStatus(networks_object["status"].GetString());
			}

			if (networks_object.HasMember("name") && networks_object["name"].IsString())
			{
				//LOG_INFO_FMT("name is %s", networks_object["name"].GetString());
				network->setName(networks_object["name"].GetString());
			}

			if (networks_object.HasMember("provider:physical_network")  && networks_object["provider:physical_network"].IsString())
			{
				//LOG_INFO_FMT("name is %s", networks_object["provider:physical_network"].GetString());
				network->setProviderPhysicalNetwork(networks_object["provider:physical_network"].GetString());
			}

			if (networks_object.HasMember("admin_state_up")  && networks_object["admin_state_up"].IsBool())
			{
				//LOG_INFO_FMT("admin_state_up is %d", networks_object["admin_state_up"].GetBool());
				network->setAdminStateUp(networks_object["admin_state_up"].GetBool());
			}

			if (networks_object.HasMember("tenant_id")  && networks_object["tenant_id"].IsString())
			{
				//LOG_INFO_FMT("tenant_id is %s", networks_object["tenant_id"].GetString());
				network->setTenantId(networks_object["tenant_id"].GetString());
			}

			if (networks_object.HasMember("qos_policy_id")  && networks_object["qos_policy_id"].IsString())
			{
				//LOG_INFO_FMT("qos_policy_id is %s", networks_object["qos_policy_id"].GetString());
				network->setQosPolicyId(networks_object["qos_policy_id"].GetString());
			}

			if (networks_object.HasMember("provider:network_type")  && networks_object["provider:network_type"].IsString())
			{
				//LOG_INFO_FMT("provider:network_type is %s", networks_object["provider:network_type"].GetString());
				network->setProviderNetworkType(networks_object["provider:network_type"].GetString());
			}

			if (networks_object.HasMember("router:external")  && networks_object["router:external"].IsBool())
			{
				//LOG_INFO_FMT("router:external  is %d", networks_object["router:external"].GetBool());
				network->setRouterExternal(networks_object["router:external"].GetBool());
			}

			if (networks_object.HasMember("port_security_enabled")  && networks_object["port_security_enabled"].IsBool())
			{
				//LOG_INFO_FMT("port_security_enabled is %d", networks_object["port_security_enabled"].GetBool());
				network->setPortSecurityEnabled(networks_object["port_security_enabled"].GetBool());
			}

			if (networks_object.HasMember("mtu") && networks_object["mtu"].IsUint())
			{
				//LOG_INFO_FMT("mtu is %d",networks_object["mtu"].GetUint());
				network->setMtu(networks_object["mtu"].GetUint());
			}
			if (networks_object.HasMember("shared")  && networks_object["shared"].IsBool())
			{
				//LOG_INFO_FMT("shared is %d", networks_object["shared"].GetBool());
				network->setShared(networks_object["shared"].GetBool());
			}
			if (networks_object.HasMember("id")  && networks_object["id"].IsString())
			{
				//LOG_INFO_FMT("id is %s", networks_object["id"].GetString());
				network->setId(networks_object["id"].GetString());
			}

			if (networks_object.HasMember("provider:segmentation_id") && networks_object["provider:segmentation_id"].IsUint())
			{
				//LOG_INFO_FMT("provider:segmentation_id is %d",networks_object["provider:segmentation_id"].GetUint());
				network->setProviderSegmentationId(networks_object["provider:segmentation_id"].GetUint());
			}

			if (networks_object.HasMember("subnets")  && networks_object["subnets"].IsArray())
			{
                std::list<std::string> networks_list;
				const Value& subnets_array = networks_object["subnets"];

				UINT4 subnets_index = 0;
				for (; subnets_index < subnets_array.Size(); ++subnets_index)
				{
					if (subnets_array[subnets_index].IsString())
					{
						networks_list.push_back(subnets_array[subnets_index].GetString());
						//LOG_INFO_FMT("subnets is %s", subnets_array[subnets_index].GetString());
					}
				}
                network->setSubnets(networks_list);
			}

			if (FALSE == addOpenstackNetwork(network))
			{
				delete network;
			}
		}
	}

    return TRUE;
}

BOOL COpenstackResource::parseSubnetData(const std::string & result)
{
    //LOG_WARN_FMT("\n==>Parse Subnet Data:\n##%s", result.c_str());
	/*
	 * parse data example:
     * http://developer.openstack.org/api-ref/networking/v2
     *
     * {
			"subnets": [
				{
					"name": "private-subnet",
					"enable_dhcp": true,
					"network_id": "db193ab3-96e3-4cb3-8fc5-05f4296d0324",
					"tenant_id": "26a7980765d0414dbc1fc1f88cdb7e6e",
					"dns_nameservers": [],
					"allocation_pools": [
						{
							"start": "10.0.0.2",
							"end": "10.0.0.254"
						}
					],
					"host_routes": [],
					"ip_version": 4,
					"gateway_ip": "10.0.0.1",
					"cidr": "10.0.0.0/24",
					"id": "08eae331-0402-425a-923c-34f7cfe39c1b"
				},
				{
					"name": "my_subnet",
					"enable_dhcp": true,
					"network_id": "d32019d3-bc6e-4319-9c1d-6722fc136a22",
					"tenant_id": "4fd44f30292945e481c7b8a0c8908869",
					"dns_nameservers": [],
					"allocation_pools": [
						{
							"start": "192.0.0.2",
							"end": "192.255.255.254"
						}
					],
					"host_routes": [],
					"ip_version": 4,
					"gateway_ip": "192.0.0.1",
					"cidr": "192.0.0.0/8",
					"id": "54d6f61d-db07-451c-9ab3-b9609b6b6f0b"
				}
			]
		}
	 */
	Document document;
	if (document.Parse(result.c_str()).HasParseError())
	{
	  LOG_INFO("Openstack Port Data has error.");
	  return FALSE;
	}
	if (document.HasMember("subnets") && (document["subnets"].IsArray()))
	{
		Value& subnets_array = document["subnets"];

		UINT4 index = 0;

		for (; index < subnets_array.Size(); ++index)
		{
			COpenstackSubnet* subnet = new COpenstackSubnet();

			const Value& subnets_object = subnets_array[index];
			if (subnets_object.HasMember("name") && subnets_object["name"].IsString())
			{
				//LOG_INFO_FMT("name is %s",subnets_object["name"].GetString());
				subnet->setName(subnets_object["name"].GetString());
			}

			if (subnets_object.HasMember("enable_dhcp") && subnets_object["enable_dhcp"].IsBool())
			{
				//LOG_INFO_FMT("enable_dhcp is %d",subnets_object["enable_dhcp"].GetBool());
				subnet->setEnableDhcp(subnets_object["enable_dhcp"].GetBool());
			}

			if (subnets_object.HasMember("network_id") && subnets_object["network_id"].IsString())
			{
				//LOG_INFO_FMT("network_id is %s",subnets_object["network_id"].GetString());
				subnet->setNetworkId(subnets_object["network_id"].GetString());
			}

			if (subnets_object.HasMember("tenant_id") && subnets_object["tenant_id"].IsString())
			{
				//LOG_INFO_FMT("tenant_id is %s",subnets_object["tenant_id"].GetString());
				subnet->setTenantId(subnets_object["tenant_id"].GetString());
			}

		    if (subnets_object.HasMember("dns_nameservers") && subnets_object["dns_nameservers"].IsArray())
			{
				//LOG_INFO_FMT("allowed_address_pairs  is %s", subnets_object["dns_nameservers "].GetArray());
			}

		    if (subnets_object.HasMember("allocation_pools") && subnets_object["allocation_pools"].IsArray())
		    {
		    	const Value& pools_array = subnets_object["allocation_pools"];
		    	UINT4 pools_index = 0;
		    	for (; pools_index < pools_array.Size(); ++pools_index)
		    	{
		    		map<string, string> pools_map;
		    		const Value& pools_object = pools_array[pools_index];
		    		if (pools_object.HasMember("start") && pools_object["start"].IsString()
		    				&& pools_object.HasMember("end") && pools_object["end"].IsString())
		    		{
		    			string start = pools_object["start"].GetString();
		    			string end = pools_object["end"].GetString();

		    			//LOG_INFO_FMT("start is %s",pools_object["start"].GetString());
		    			//LOG_INFO_FMT("end is %s",pools_object["end"].GetString());

		    			pools_map.insert(make_pair(start,end));
						subnet->setStart(start);
						subnet->setEnd(end);
		    		}
		    		subnet->setAlloctionPools(pools_map);
		    	}
		    }

		    if (subnets_object.HasMember("host_routes") && subnets_object["host_routes"].IsArray())
		    {
		    	//LOG_INFO_FMT("host_routes is %s",subnets_object["host_routes"].GetArray());
		    }

		    if (subnets_object.HasMember("ip_version") && subnets_object["ip_version"].IsUint())
		    {
		    	//LOG_INFO_FMT("ip_version is %d",subnets_object["ip_version"].GetUint());
		    	subnet->setIpVersion(subnets_object["ip_version"].GetUint());
		    }

		    if (subnets_object.HasMember("gateway_ip") && subnets_object["gateway_ip"].IsString())
		    {
		    	//LOG_INFO_FMT("gateway_ip is %s",subnets_object["gateway_ip"].GetString());
                subnet->setGatewayIp(subnets_object["gateway_ip"].GetString());
		    }

		    if (subnets_object.HasMember("cidr") && subnets_object["cidr"].IsString())
			{
				//LOG_INFO_FMT("cidr is %s",subnets_object["cidr"].GetString());
				subnet->setCidr(subnets_object["cidr"].GetString());
			}
		    if (subnets_object.HasMember("id") && subnets_object["id"].IsString())
			{
				//LOG_INFO_FMT("id is %s",subnets_object["id"].GetString());
				subnet->setId(subnets_object["id"].GetString());
			}

		    if (FALSE == addOpenstackSubnet(subnet))
			{
				delete subnet;
			}
		}
	}
    return TRUE;
}

BOOL COpenstackResource::parsePortData(const std::string & result)
{
    //LOG_WARN_FMT("\n==>Parse Port Data:\n##%s", result.c_str());
	//LOG_ERROR_FMT("Parse Port Data:\n%s", result.c_str());
    /*
     * parse data example:
     * http://developer.openstack.org/api-ref/networking/v2
     *
     * {
            "ports": [
                {
                    "status": "ACTIVE",
                    "name": "",
                    "allowed_address_pairs": [],
                    "admin_state_up": true,
                    "network_id": "70c1db1f-b701-45bd-96e0-a313ee3430b3",
                    "tenant_id": "",
                    "extra_dhcp_opts": [],
                    "device_owner": "network:router_gateway",
                    "mac_address": "fa:16:3e:58:42:ed",
                    "fixed_ips": [
                        {
                            "subnet_id": "008ba151-0b8c-4a67-98b5-0d2b87666062",
                            "ip_address": "172.24.4.2"
                        }
                    ],
                    "id": "d80b1a3b-4fc1-49f3-952e-1e2ab7081d8b",
                    "security_groups": ["85cc3048-abc3-43cc-89b3-377341426ac5"],
                    "device_id": "9ae135f4-b6e0-4dad-9e91-3c223e385824"
                },
                {
                    "status": "ACTIVE",
                    "name": "",
                    "allowed_address_pairs": [],
                    "admin_state_up": true,
                    "network_id": "f27aa545-cbdd-4907-b0c6-c9e8b039dcc2",
                    "tenant_id": "d397de8a63f341818f198abb0966f6f3",
                    "extra_dhcp_opts": [],
                    "device_owner": "network:router_interface",
                    "mac_address": "fa:16:3e:bb:3c:e4",
                    "fixed_ips": [
                        {
                            "subnet_id": "288bf4a1-51ba-43b6-9d0a-520e9005db17",
                            "ip_address": "10.0.0.1"
                        }
                    ],
                    "id": "f71a6703-d6de-4be1-a91a-a570ede1d159",
                    "security_groups": ["85cc3048-abc3-43cc-89b3-377341426ac5"],
                    "device_id": "9ae135f4-b6e0-4dad-9e91-3c223e385824"
                }
            ]
        }
     */
    //huayun
    /*
     "ports": [
     			{		
     				"status": "ACTIVE",		
     				"qos": [],		
     				"description": "",		
     				"allowed_address_pairs": [],		
     				"binding:host_id": "controller03",		
     				"extra_dhcp_opts": [],		
     				"updated_at": "2018-02-26T05:52:37",		
     				"device_owner": "network:dhcp",		
     				"port_security_enabled": false,		
     				"binding:profile": {					
     										},		
     				"fixed_ips": [
     								{			
     									"subnet_id": "5dc28bd9-c438-47d5-b0f8-6179bf7ab099",			
     									"ip_address": "169.255.16.2"		
     								}
     							],		
     				"id": "046eb022-bf42-4ebd-ad32-0aa76bfda7af",		
     				"security_groups": [],		
     				"device_id": "dhcp9fc65dbd-ecf2-50a6-b85c-5d852848e5fb-31e58c07-ea64-4107-8036-e6eecacf51b2",		
     				"name": "",		
     				"admin_state_up": true,		
     				"network_id": "31e58c07-ea64-4107-8036-e6eecacf51b2",		
     				"dns_name": null,		
     				"binding:vif_details": {			
     											"port_filter": true,			
     											"ovs_hybrid_plug": true		
     										},		
     				"binding:vnic_type": "normal",		
     				"binding:vif_type": "ovs",		
     				"tenant_id": "b7a9461b96034b989406ea8d36553ff3",		
     				"mac_address": "fa:16:3e:ad:b1:33",		
     				"created_at": "2018-02-07T06:57:27"	
     			},
     			{		
     				"status": "DOWN",		
     				"qos": ["495f91a9-c2eb-4e01-b321-db46b3c49737",		"706823ec-c432-44f7-bcb3-6dfe2153db4b"],		
     				"description": null,		
     				"allowed_address_pairs": [],		
     				"binding:host_id": "controller03",		
     				"extra_dhcp_opts": [],		
     				"updated_at": "2018-02-26T08:15:12",		
     				"device_owner": "network:LOADBALANCER_VIP",		
     				"port_security_enabled": true,		
     				"binding:profile": {					
     					},		
     				"fixed_ips": [
     								{			
     									"subnet_id": "a04967e7-bfa5-422c-b881-265a88062772",			
     									"ip_address": "192.168.52.146"		
     								}
     							],		
     				"id": "82afca5a-5d11-4a46-91a0-f449e30ae378",		
     				"security_groups": ["2521b73b-48a6-4687-a7bc-5e59e29282cd"],		
     				"device_id": "7b1257b6-f23e-4e12-8020-bbb516edc55d",		
     				"name": "",		"admin_state_up": true,		
     				"network_id": "ca8027e3-1407-47a0-8c61-64f8d8016170",		
     				"dns_name": null,		
     				"binding:vif_details": {					
     				},		
     				"binding:vnic_type": "normal",		
     				"binding:vif_type": "binding_failed",		
     				"tenant_id": "aebf7515b0a343948d68a57d4a8ef580",		
     				"mac_address": "fa:16:3e:99:58:ba",		
     				"created_at": "2018-02-24T08:07:08"	
     			}
     		]
	*/
    Document document;
    if (document.Parse(result.c_str()).HasParseError())
    {
      LOG_INFO("Openstack Port Data has error.");
      return FALSE;
    }

    if (document.HasMember("ports") && (document["ports"].IsArray()))
    {
        Value& ports_array = document["ports"];

        UINT4 index = 0;

        for (; index < ports_array.Size(); ++index)
        {
            COpenstackPort* port = new COpenstackPort();
            if (NULL == port)
            {
              LOG_ERROR("new COpenstackPort failed !");
              return FALSE;
            }
			COpenstackQosBind* qosBindport = new COpenstackQosBind();
            const Value& port_object = ports_array[index];

            if (port_object.HasMember("opt_value") && port_object["opt_value"].IsString())
            {
                // LOG_INFO_FMT("opt_value is %s", port_object["opt_value"].GetString());
                port->setOptValue(port_object["opt_value"].GetString());
            }

            if (port_object.HasMember("status") && port_object["status"].IsString())
            {
                // LOG_INFO_FMT("status is %s", port_object["status"].GetString());
                port->setStatus(port_object["status"].GetString());
            }

            if (port_object.HasMember("name")  && port_object["name"].IsString())
            {
                // LOG_INFO_FMT("name is %s", port_object["name"].GetString());
                port->setName(port_object["name"].GetString());
            }

            if (port_object.HasMember("allowed_address_pairs") && port_object["allowed_address_pairs"].IsArray())
            {
                // LOG_INFO_FMT("allowed_address_pairs  is %s", port_object["allowed_address_pairs "].GetArray());
            }

            if (port_object.HasMember("admin_state_up")  && port_object["admin_state_up"].IsBool())
            {
                // LOG_INFO_FMT("admin_state_up is %d", port_object["admin_state_up"].GetBool());
                port->setAdminStateUp(port_object["admin_state_up"].GetBool());
            }

            if (port_object.HasMember("network_id")  && port_object["network_id"].IsString())
            {
                // LOG_INFO_FMT("network_id is %s", port_object["network_id"].GetString());
                port->setNetworkId(port_object["network_id"].GetString());
            }

            if (port_object.HasMember("ip_address")  && port_object["ip_address"].IsString())
            {
                // LOG_INFO_FMT("ip_address is %s", port_object["ip_address"].GetString());
                port->setIpAddress(port_object["ip_address"].GetString());
            }

            if (port_object.HasMember("extra_dhcp_opts")  && port_object["extra_dhcp_opts"].IsArray())
            {
                // LOG_INFO_FMT("ip_address is %s", port_object["extra_dhcp_opts"].GetArray());
            }

            if (port_object.HasMember("opt_name")  && port_object["opt_name"].IsString())
            {
                // LOG_INFO_FMT("opt_name is %s", port_object["opt_name"].GetString());
                port->setOptName(port_object["opt_name"].GetString());
            }

            if (port_object.HasMember("updated_at")  && port_object["updated_at"].IsString())
            {
                // LOG_INFO_FMT("updated_at is %s", port_object["updated_at"].GetString());
                port->setUpdateAt(port_object["updated_at"].GetString());
            }

            if (port_object.HasMember("id")  && port_object["id"].IsString())
            {
                // LOG_INFO_FMT("id is %s", port_object["id"].GetString());
                port->setId(port_object["id"].GetString());
				qosBindport->SetPortId(port_object["id"].GetString());
            }

            if (port_object.HasMember("ports")  && port_object["ports"].IsArray())
            {
                // LOG_INFO_FMT("ports is %s", port_object["ports"].GetArray());
            }

            if (port_object.HasMember("subnet_id")  && port_object["subnet_id"].IsString())
            {
                // LOG_INFO_FMT("subnet_id is %s", port_object["subnet_id"].GetString());
                port->setSubnetId(port_object["subnet_id"].GetString());
            }

            if (port_object.HasMember("device_owner")  && port_object["device_owner"].IsString())
            {
                // LOG_INFO_FMT("device_owner is %s", port_object["device_owner"].GetString());
                port->setDeviceOwner(port_object["device_owner"].GetString());
            }

            if (port_object.HasMember("tenant_id")  && port_object["tenant_id"].IsString())
            {
                // LOG_INFO_FMT("tenant_id is %s", port_object["tenant_id"].GetString());
                port->setTenantId(port_object["tenant_id"].GetString());
            }

            if (port_object.HasMember("mac_address")  && port_object["mac_address"].IsString())
            {
                // LOG_INFO_FMT("mac_address is %s", port_object["mac_address"].GetString());
                port->setMacAddress(port_object["mac_address"].GetString());
				qosBindport->SetPortMac(port->getMacAddress());
            }

            if (port_object.HasMember("port_security_enabled ")  && port_object["port_security_enabled"].IsBool())
            {
                // LOG_INFO_FMT("port_security_enabled  is %d", port_object["port_security_enabled"].GetBool());
                port->setPortSecurityEnabled(port_object["port_security_enabled"].GetBool());
            }

            if (port_object.HasMember("fixed_ips")  && port_object["fixed_ips"].IsArray())
            {
                // LOG_INFO_FMT("fixed_ips is %s", port_object["fixed_ips"].GetArray());

                const Value& fixedip_array = port_object["fixed_ips"];

                UINT4 fixedip_index = 0;
                for (; fixedip_index < port_object["fixed_ips"].Size(); ++fixedip_index)
                {
                    std::map<std::string, std::string> fixedip_list;

                    const Value& fixedip_object = fixedip_array[fixedip_index];

                    if ((fixedip_object.HasMember("subnet_id") && fixedip_object["subnet_id"].IsString())
                            && fixedip_object.HasMember("ip_address") && fixedip_object["ip_address"].IsString())
                    {
                    	
                         //LOG_INFO_FMT("subnet_id is %s ip_address is %s", fixedip_object["subnet_id"].GetString(), fixedip_object["ip_address"].GetString());

                        std::string strSubnet = fixedip_object["subnet_id"].GetString();
                        std::string strIpAddress = fixedip_object["ip_address"].GetString();

                        fixedip_list.insert(std::make_pair(strIpAddress, strSubnet));
                    }

                    port->setFixedIps(fixedip_list);
                }
            }

            if (port_object.HasMember("created_at")  && port_object["created_at"].IsString())
            {
                // LOG_INFO_FMT("created_at is %s", port_object["created_at"].GetString());
                port->setCreatedAt(port_object["created_at"].GetString());
            }

            if (port_object.HasMember("security_groups")  && port_object["security_groups"].IsArray())
            {
                list<CSecurityGroupId> security_groups;

                const Value& sgp_array = port_object["security_groups"];
                for (UINT4 index = 0; index < sgp_array.Size(); ++index)
                {
					if (sgp_array[index].IsString())
                    {
                        LOG_DEBUG_FMT("===> host[%s] attached to security_group_id[%s]", 
                            port->getMacAddress().c_str(), sgp_array[index].GetString());
                        security_groups.push_back(sgp_array[index].GetString());
                    }
                }

                port->setSecurtyGroups(security_groups);
            }

            if (port_object.HasMember("device_id")  && port_object["device_id"].IsString())
            {
                // LOG_INFO_FMT("device_id is %s", port_object["device_id"].GetString());
                port->setDeviceId(port_object["device_id"].GetString());
            }

			if(port_object.HasMember("qos") && port_object["qos"].IsArray())
			{
				 const Value& qosbind_array = port_object["qos"];
				 
				 if(0 != port_object["qos"].Size())
 			 	 {
					//LOG_ERROR_FMT("qosbind_array[0].isstring is %d ", qosbind_array[0].IsString());
					//LOG_ERROR_FMT("qosbind_array[1].isstring is %d ", qosbind_array[1].IsString());
					//LOG_ERROR_FMT("qosbind_array[0].string is %s ", qosbind_array[0].GetString());
					//LOG_ERROR_FMT("qosbind_array[1].string is %s ", qosbind_array[1].GetString());
					COpenstackQosRule* qosRule = NULL;
					for(UINT4 qos_index = 0; qos_index < port_object["qos"].Size(); qos_index++)
					{
						qosRule = findOpenstackQosRule(qosbind_array[qos_index].GetString());
						if(NULL != qosRule)
						{
							(TRUE == qosRule->GetDerection())? qosBindport->SetIngressQosId(qosbind_array[qos_index].GetString()): qosBindport->SetEgressQosId(qosbind_array[qos_index].GetString());
						}
					}
					qosBindport->SetPortIp(port->getFixedFirstIp());
					if(FALSE == addOpenstackQosBindPort(qosBindport))
					{
						delete qosBindport;
					}
 			 	 }
				 else
			 	 {
			 	 	delete qosBindport;
			 	 }
			}
            addOpenstackPort(CSmartPtr<COpenstackPort>(port));
        }
    }

    return TRUE;
}

BOOL COpenstackResource::parseFloatingipData(const std::string & result)
{
    //LOG_WARN_FMT("\n==>Parse Floaitngip Data:\n##%s", result.c_str());
	/*
	 *  parse data example:
     * http://developer.openstack.org/api-ref/networking/v2
	 * {
            "floatingips": [
                {
                    "router_id": "d23abc8d-2991-4a55-ba98-2aaea84cc72f",
                    "tenant_id": "4969c491a3c74ee4af974e6d800c62de",
                    "floating_network_id": "376da547-b977-4cfe-9cba-275c80debf57",
                    "fixed_ip_address": "10.0.0.3",
                    "floating_ip_address": "172.24.4.228",
                    "port_id": "ce705c24-c1ef-408a-bda3-7bbd946164ab",
                    "id": "2f245a7b-796b-4f26-9cf9-9e82d248fda7",
                    "status": "ACTIVE"
                },
                {
                    "router_id": null,
                    "tenant_id": "4969c491a3c74ee4af974e6d800c62de",
                    "floating_network_id": "376da547-b977-4cfe-9cba-275c80debf57",
                    "fixed_ip_address": null,
                    "floating_ip_address": "172.24.4.227",
                    "port_id": null,
                    "id": "61cea855-49cb-4846-997d-801b70c71bdd",
                    "status": "DOWN"
                }
            ]
        }
	 */

	Document document;
	if (document.Parse(result.c_str()).HasParseError())
	{
		LOG_INFO("Floatingip result has error.");
		return FALSE;
	}
	if (document.HasMember("floatingips") && (document["floatingips"].IsArray()))
	{
		Value& floating_array = document["floatingips"];
		UINT4 index = 0;
		for (; index < floating_array.Size(); ++index)
		{
			COpenstackFloatingip* floatingip = new COpenstackFloatingip();
			const Value& floating_object = floating_array[index];

			if (floating_object.HasMember("router_id") && floating_object["router_id"].IsString())
			{
//				LOG_INFO_FMT("router_id is %s", floating_object["router_id"].GetString());
				floatingip->setRouterId(floating_object["router_id"].GetString());
//                LOG_INFO_FMT("router_id is: %s", floatingip->getRouterId().c_str());
			}

			if (floating_object.HasMember("tenant_id") && floating_object["tenant_id"].IsString())
			{
//				LOG_INFO_FMT("tenant_id is %s", floating_object["tenant_id"].GetString());
				floatingip->setTenantId(floating_object["tenant_id"].GetString());
//                LOG_INFO_FMT("tenant_id is: %s", floatingip->getTenantId().c_str());
			}

			if (floating_object.HasMember("floating_network_id") && floating_object["floating_network_id"].IsString())
			{
//				LOG_INFO_FMT("floating_network_id is %s", floating_object["floating_network_id"].GetString());
				floatingip->setFloatingNetId(floating_object["floating_network_id"].GetString());
//                LOG_INFO_FMT("floating_network_id is: %s", floatingip->getFloatingNetId().c_str());

			}

			if (floating_object.HasMember("fixed_ip_address") && floating_object["fixed_ip_address"].IsString())
			{
//				LOG_INFO_FMT("fixed_ip_address is %s", floating_object["fixed_ip_address"].GetString());
				floatingip->setFixedIp(floating_object["fixed_ip_address"].GetString());
//                LOG_INFO_FMT("fixed_ip_address is: %s", floatingip->getFixedIp().c_str());

			}

			if (floating_object.HasMember("floating_ip_address") && floating_object["floating_ip_address"].IsString())
			{
//				LOG_INFO_FMT("floating_ip_address is %s", floating_object["floating_ip_address"].GetString());
				floatingip->setFloatingIp(floating_object["floating_ip_address"].GetString());
//                LOG_INFO_FMT("floating_ip_address is: %s", floatingip->getRouterId().c_str());
			}

			if (floating_object.HasMember("port_id") && floating_object["port_id"].IsString())
			{
//				LOG_INFO_FMT("port_id is %s", floating_object["port_id"].GetString());
				floatingip->setPortId(floating_object["port_id"].GetString());
//                LOG_INFO_FMT("port_id is: %s", floatingip->getPortId().c_str());
			}

			if (floating_object.HasMember("id") && floating_object["id"].IsString())
			{
//				LOG_INFO_FMT("id is %s", floating_object["id"].GetString());
				floatingip->setId(floating_object["id"].GetString());
//                LOG_INFO_FMT("id is: %s", floatingip->getId().c_str());
			}
			if (floating_object.HasMember("status") && floating_object["status"].IsString())
			{
//				LOG_INFO_FMT("status is %s", floating_object["status"].GetString());
				floatingip->setStatus(floating_object["status"].GetString());
//                LOG_INFO_FMT("status is: %s", floatingip->getStatus().c_str());
			}
			if (FALSE == addOpenstackFloatingip(floatingip))
			{
				delete floatingip;
			}

		}

	}
    return TRUE;
}

BOOL COpenstackResource::parseSecurityGroupData(const std::string & result)
{
    //LOG_WARN_FMT("\n==>Parse SecurityGroup Data:\n##%s", result.c_str());
	/*
	 * parse data example:
	 * http://developer.openstack.org/api-ref/networking/v2
	 * {
			"security_group_rules": [
				{
                    "direction": "egress",
                    "protocol": "icmp",
                    "description": "",
                    "port_range_max": 0,
                    "updated_at": "2018-05-16T02:35:58",
                    "id": "49212629-3b7d-4bfe-8eb1-1a0bfdbd01ea",
                    "remote_group_id": null,
                    "remote_ip_prefix": "192.168.53.0/24",
                    "created_at": "2018-05-14T09:59:36",
                    "enabled": true,
                    "security_group_id": "8d8a9d6e-6e7b-490a-b58d-516dacfc43ce",
                    "priority": 10,
                    "tenant_id": "aebf7515b0a343948d68a57d4a8ef580",
                    "port_range_min": 8,
                    "ethertype": "IPv4",
                    "action": "drop"
				},
				{
                    "direction": "ingress",
                    "protocol": "icmp",
                    "description": "",
                    "port_range_max": 0,
                    "updated_at": "2018-05-14T10:03:00",
                    "id": "4e22b37a-da3c-4103-a789-3e88eab234ce",
                    "remote_group_id": null,
                    "remote_ip_prefix": "192.168.53.0/24",
                    "created_at": "2018-05-14T02:59:05",
                    "enabled": true,
                    "security_group_id": "8d8a9d6e-6e7b-490a-b58d-516dacfc43ce",
                    "priority": 10,
                    "tenant_id": "aebf7515b0a343948d68a57d4a8ef580",
                    "port_range_min": 8,
                    "ethertype": "IPv4",
                    "action": "accept"
				}
			]
		}
	 */
	Document document;
	if (document.Parse(result.c_str()).HasParseError())
	{
		LOG_INFO("security group result has error.");
		return FALSE;
	}
	if (document.HasMember("security_group_rules") && (document["security_group_rules"].IsArray()))
	{
        COpenstackSecurityGroupMap sgps_new;

		Value& security_group_array = document["security_group_rules"];
		for (UINT4 index = 0; index < security_group_array.Size(); ++index)
		{
			COpenstackSecurityGroup* security_group = new COpenstackSecurityGroup();
            if (NULL == security_group)
            {
                LOG_ERROR("new COpenstackSecurityGroup failed !");
                return FALSE;
            }

			const Value& security_group_object = security_group_array[index];
			if (security_group_object.HasMember("direction") && security_group_object["direction"].IsString())
			{
				//LOG_INFO_FMT("direction is %s",security_group_object["direction"].GetString());
				security_group->setDirection(security_group_object["direction"].GetString());
			}

			if (security_group_object.HasMember("ethertype") && security_group_object["ethertype"].IsString())
			{
				//LOG_INFO_FMT("ethertype is %s",security_group_object["ethertype"].GetString());
				security_group->setEthertype(security_group_object["ethertype"].GetString());
			}

			if (security_group_object.HasMember("id") && security_group_object["id"].IsString())
			{
				//LOG_INFO_FMT("id is %s",security_group_object["id"].GetString());
				security_group->setId(security_group_object["id"].GetString());
			}

			if (security_group_object.HasMember("protocol") && security_group_object["protocol"].IsString())
			{
				//LOG_INFO_FMT("protocol is %s",security_group_object["protocol"].GetString());
				security_group->setProtocol(security_group_object["protocol"].GetString());
			}

            if (security_group->getProtocol().compare("icmp") == 0)
            {
                security_group->setPortRangeMax(ICMP_CODE_NULL);
                security_group->setPortRangeMin(ICMP_TYPE_NULL);
            }

			if (security_group_object.HasMember("port_range_max") && security_group_object["port_range_max"].IsUint())
			{
				//LOG_INFO_FMT("port range max is %d",security_group_object["port_range_max"].GetUint());
				security_group->setPortRangeMax(security_group_object["port_range_max"].GetUint());
			}

			if (security_group_object.HasMember("port_range_min") && security_group_object["port_range_min"].IsUint())
			{
				//LOG_INFO_FMT("port_range_min is %d",security_group_object["port_range_min"].GetUint());
				security_group->setPortRangeMin(security_group_object["port_range_min"].GetUint());
			}

			if (security_group_object.HasMember("remote_group_id") && security_group_object["remote_group_id"].IsString())
			{
				//LOG_INFO_FMT("remote group id is %s",security_group_object["remote_group_id"].GetString());
				security_group->setRemoteGroupId(security_group_object["remote_group_id"].GetString());
			}

			if (security_group_object.HasMember("remote_ip_prefix") && security_group_object["remote_ip_prefix"].IsString())
			{
				//LOG_INFO_FMT("remote_ip_prefix is %s",security_group_object["remote_ip_prefix"].GetString());
				security_group->setRemoteIpPrefix(security_group_object["remote_ip_prefix"].GetString());
			}

			if (security_group_object.HasMember("security_group_id") && security_group_object["security_group_id"].IsString())
			{
				//LOG_INFO_FMT("security_group_id is %s",security_group_object["security_group_id"].GetString());
				security_group->setSecurityGroupId(security_group_object["security_group_id"].GetString());
			}

			if (security_group_object.HasMember("tenant_id") && security_group_object["tenant_id"].IsString())
			{
				//LOG_INFO_FMT("tenant_id is %s",security_group_object["tenant_id"].GetString());
				security_group->setTenantId(security_group_object["tenant_id"].GetString());
			}

			if (security_group_object.HasMember("priority") && security_group_object["priority"].IsUint())
			{
				//LOG_INFO_FMT("priority is %d",security_group_object["priority"].GetUint());
				security_group->setPriority(security_group_object["priority"].GetUint());
			}

			if (security_group_object.HasMember("enabled") && security_group_object["enabled"].IsBool())
			{
				//LOG_INFO_FMT("enabled is %d",security_group_object["enabled"].GetBool());
				security_group->setEnabled(security_group_object["enabled"].GetBool());
			}

			if (security_group_object.HasMember("action") && security_group_object["action"].IsString())
			{
				//LOG_INFO_FMT("action is %s",security_group_object["action"].GetString());
				security_group->setAction(security_group_object["action"].GetString());
			}

            sgps_new.insert(std::make_pair(security_group->getId(), CSmartPtr<COpenstackSecurityGroup>(security_group)));
		}

        addOpenstackSecurityGroups(sgps_new);
	}
    return TRUE;
}

BOOL COpenstackResource::parseRouterData(const std::string & result)
{
    //LOG_WARN_FMT("\n==>Parse Router Data:\n##%s", result.c_str());
	/*
	 * parse data example:
	 * http://developer.openstack.org/api-ref/networking/v2
    {
    	"routers": [
    		{
    			"status": "ACTIVE",
    			"external_gateway_info": {
    				"network_id": "ca8027e3-1407-47a0-8c61-64f8d8016170",
    				"enable_snat": true,
    				"external_fixed_ips": [
    					{
    						"subnet_id": "a04967e7-bfa5-422c-b881-265a88062772",
    						"ip_address": "192.168.52.161"
    					}
    				]
    			},
    			"availability_zone_hints": [
    			],
    			"availability_zones": [
    			],
    			"portforwardings": [
    				{
    					"status": "ENABLE",
    					"inside_addr": "10.40.40.4",
    					"protocol": "tcp",
    					"outside_port": "234-2340",
    					"inside_port": "234-2340"
    				},
    				{
    					"status": "ENABLE",
    					"inside_addr": "10.40.40.6",
    					"protocol": "tcp",
    					"outside_port": "6523",
    					"inside_port": "532"
    				},
    				{
    					"status": "ENABLE",
    					"inside_addr": "10.40.40.4",
    					"protocol": "udp",
    					"outside_port": "78-78",
    					"inside_port": "78-78"
    				}
    			],
    			"description": "",
    			"gw_port_id": "90741336-4e08-40a9-972a-e2767558bc70",
    			"admin_state_up": true,
    			"tenant_id": "96afa7b2288640939726dfd054fd92f3",
    			"created_at": "2018-04-09T02:14:35",
    			"distributed": false,
    			"updated_at": "2018-04-09T02:14:37",
    			"routes": [
    			],
    			"ha": false,
    			"id": "30fddd34-81e6-4fff-820e-61716f1f7002",
    			"name": "liuqq_route"
    		},
    		{
    			"status": "ACTIVE",
    			"external_gateway_info": {
    				"network_id": "ca8027e3-1407-47a0-8c61-64f8d8016170",
    				"enable_snat": true,
    				"external_fixed_ips": [
    					{
    						"subnet_id": "a04967e7-bfa5-422c-b881-265a88062772",
    						"ip_address": "192.168.52.164"
    					}
    				]
    			},
    			"availability_zone_hints": [
    			],
    			"availability_zones": [
    			],
    			"portforwardings": [
    				{
    					"status": "ENABLE",
    					"inside_addr": "192.168.122.4",
    					"protocol": "tcp",
    					"outside_port": "80-88",
    					"inside_port": "80-88"
    				},
    				{
    					"status": "ENABLE",
    					"inside_addr": "192.168.122.4",
    					"protocol": "udp",
    					"outside_port": "8080",
    					"inside_port": "8080"
    				}
    			],
    			"description": "",
    			"gw_port_id": "e64a0232-5c7f-4a6e-9d7b-1126a9acc0c2",
    			"admin_state_up": true,
    			"tenant_id": "96afa7b2288640939726dfd054fd92f3",
    			"created_at": "2018-04-09T05:56:54",
    			"distributed": false,
    			"updated_at": "2018-04-09T05:56:59",
    			"routes": [
    			],
    			"ha": false,
    			"id": "7e8ab6ef-b7d4-44c8-9774-0f0ed13cdfc6",
    			"name": "\u8def\u7531\u56682"
    		},
    		{
    			"status": "ACTIVE",
    			"external_gateway_info": {
    				"network_id": "ca8027e3-1407-47a0-8c61-64f8d8016170",
    				"enable_snat": true,
    				"external_fixed_ips": [
    					{
    						"subnet_id": "a04967e7-bfa5-422c-b881-265a88062772",
    						"ip_address": "192.168.52.160"
    					}
    				]
    			},
    			"availability_zone_hints": [
    			],
    			"availability_zones": [
    			],
    			"portforwardings": [
    				{
    					"status": "ENABLE",
    					"inside_addr": "172.16.16.16",
    					"protocol": "tcp",
    					"outside_port": "16-16666",
    					"inside_port": "16-16666"
    				},
    				{
    					"status": "ENABLE",
    					"inside_addr": "172.16.16.12",
    					"protocol": "tcp",
    					"outside_port": "8888",
    					"inside_port": "6666"
    				},
    				{
    					"status": "ENABLE",
    					"inside_addr": "10.10.10.30",
    					"protocol": "udp",
    					"outside_port": "2520-6547",
    					"inside_port": "2520-6547"
    				}
    			],
    			"description": "",
    			"gw_port_id": "af90dac7-4736-4e32-978d-1d8e36bd45ce",
    			"admin_state_up": true,
    			"tenant_id": "aebf7515b0a343948d68a57d4a8ef580",
    			"created_at": "2018-04-08T02:35:02",
    			"distributed": false,
    			"updated_at": "2018-04-08T02:35:03",
    			"routes": [
    			],
    			"ha": false,
    			"id": "9c0f1188-98f7-4877-877b-bd24bd922de3",
    			"name": "test1"
    		}
    	]
    }
	 */
	Document document;
	if (document.Parse(result.c_str()).HasParseError())
	{
		LOG_INFO("router result has error.");
		return FALSE;
	}
	if (document.HasMember("routers") && (document["routers"].IsArray()))
	{
        COpenstackRouterMap routers;

		Value& router_array = document["routers"];
		for (UINT4 index = 0; index < router_array.Size(); ++index)
		{
			COpenstackRouter* router = new COpenstackRouter();
            if (NULL == router)
            {
                LOG_ERROR("new COpenstackRouter failed !");
                return FALSE;
            }

			const Value& router_object = router_array[index];
			if (router_object.HasMember("status") && router_object["status"].IsString())
			{
				//LOG_INFO_FMT("status is %s",router_object["status"].GetString());
				router->setStatus(router_object["status"].GetString());
			}

			if (router_object.HasMember("external_gateway_info") && router_object["external_gateway_info"].IsObject())
			{
				//LOG_INFO_FMT("external_gateway_info is Object");
    			const Value& external_gateway_object = router_object["external_gateway_info"];
    			if (external_gateway_object.HasMember("network_id") && external_gateway_object["network_id"].IsString())
    			{
    				//LOG_INFO_FMT("network_id is %s",external_gateway_object["network_id"].GetString());
    				router->setExtNetworkId(external_gateway_object["network_id"].GetString());
    			}

                if (external_gateway_object.HasMember("enable_snat") && external_gateway_object["enable_snat"].IsBool())
                {
                    //LOG_INFO_FMT("enable_snat is %d",external_gateway_object["enable_snat"].GetBool());
                    router->setEnableSnat(external_gateway_object["enable_snat"].GetBool());
                }

                if (external_gateway_object.HasMember("external_fixed_ips") && (external_gateway_object["external_fixed_ips"].IsArray()))
                {
                    const Value& fixed_ips_array = external_gateway_object["external_fixed_ips"];
                    for (UINT4 i = 0; i < fixed_ips_array.Size(); ++i)
                    {
                        const Value& fixed_ip_object = fixed_ips_array[i];
                        if (fixed_ip_object.HasMember("subnet_id") && fixed_ip_object["subnet_id"].IsString())
                        {
                            //LOG_INFO_FMT("subnet_id is %s",fixed_ip_object["subnet_id"].GetString());
                            router->setExtSubnetId(fixed_ip_object["subnet_id"].GetString());
                        }
                        if (fixed_ip_object.HasMember("ip_address") && fixed_ip_object["ip_address"].IsString())
                        {
                            //LOG_INFO_FMT("ip_address is %s",fixed_ip_object["ip_address"].GetString());
                            router->setExtFixedIp(fixed_ip_object["ip_address"].GetString());
                        }
                    }
                }
			}

            if (router_object.HasMember("portforwardings") && (router_object["portforwardings"].IsArray()))
            {
                const Value& portforwarding_array = router_object["portforwardings"];
                for (UINT4 i = 0; i < portforwarding_array.Size(); ++i)
                {
                    COpenstackPortforward portforward;
                    const Value& portforwarding_object = portforwarding_array[i];
                    if (portforwarding_object.HasMember("status") && portforwarding_object["status"].IsString())
                    {
                        //LOG_INFO_FMT("status is %s",portforwarding_object["status"].GetString());
                        portforward.m_status = portforwarding_object["status"].GetString();
                    }
                    if (portforwarding_object.HasMember("inside_addr") && portforwarding_object["inside_addr"].IsString())
                    {
                        //LOG_INFO_FMT("inside_addr is %s",portforwarding_object["inside_addr"].GetString());
                        portforward.m_insideAddr = portforwarding_object["inside_addr"].GetString();
                    }
                    if (portforwarding_object.HasMember("protocol") && portforwarding_object["protocol"].IsString())
                    {
                        //LOG_INFO_FMT("protocol is %s",portforwarding_object["protocol"].GetString());
                        portforward.m_protocol = portforwarding_object["protocol"].GetString();
                    }
                    if (portforwarding_object.HasMember("outside_port") && portforwarding_object["outside_port"].IsString())
                    {
                        //LOG_INFO_FMT("outside_port is %s",portforwarding_object["outside_port"].GetString());
                        portforward.m_outsidePort = portforwarding_object["outside_port"].GetString();
                    }
                    if (portforwarding_object.HasMember("inside_port") && portforwarding_object["inside_port"].IsString())
                    {
                        //LOG_INFO_FMT("inside_port is %s",portforwarding_object["inside_port"].GetString());
                        portforward.m_insidePort = portforwarding_object["inside_port"].GetString();
                    }

                    router->setPortforward(portforward);
                }
            }

			if (router_object.HasMember("name") && router_object["name"].IsString())
			{
				//LOG_INFO_FMT("name is %s",router_object["name"].GetString());
				router->setName(router_object["name"].GetString());
			}

			if (router_object.HasMember("gw_port_id") && router_object["gw_port_id"].IsString())
			{
				//LOG_INFO_FMT("gw_port_id is %s",router_object["gw_port_id"].GetString());
				router->setGwPortId(router_object["gw_port_id"].GetString());
			}

			if (router_object.HasMember("admin_state_up") && router_object["admin_state_up"].IsBool())
			{
				//LOG_INFO_FMT("admin_state_up is %d",router_object["admin_state_up"].GetBool());
				router->setAdminStateUp(router_object["admin_state_up"].GetBool());
			}

			if (router_object.HasMember("tenant_id") && router_object["tenant_id"].IsString())
			{
				//LOG_INFO_FMT("tenant_id is %s",router_object["tenant_id"].GetString());
				router->setTenantId(router_object["tenant_id"].GetString());
			}

			if (router_object.HasMember("distributed") && router_object["distributed"].IsBool())
			{
				//LOG_INFO_FMT("distributed is %d",router_object["distributed"].GetBool());
				router->setDistributed(router_object["distributed"].GetBool());
			}

			if (router_object.HasMember("ha") && router_object["ha"].IsBool())
			{
				//LOG_INFO_FMT("ha is %d",router_object["ha"].GetBool());
				router->setHa(router_object["ha"].GetBool());
			}

			if (router_object.HasMember("id") && router_object["id"].IsString())
			{
				//LOG_INFO_FMT("id is %s",router_object["id"].GetString());
				router->setId(router_object["id"].GetString());
			}

            routers.insert(std::make_pair(router->getId(), CSmartPtr<COpenstackRouter>(router)));
		}

        addOpenstackRouters(routers);
	}
    return TRUE;
}

BOOL COpenstackResource::parseQosRuleData(const std::string & result)
{
	//LOG_INFO_FMT("Parse QosRule Data:\n%s", result.c_str());
	/*
	"qoses": 
		[
			{		
				"description": "\u51fa\u53e3",		
				"tags": [],		
				"updated_at": "2018-02-24T06:56:58",		
				"id": "0a3b9905-6b8c-4eb9-b1af-4f2875fe0ae8",		
				"name": "test1",		
				"admin_state_up": true,		
				"tenant_id": "aebf7515b0a343948d68a57d4a8ef580",		
				"created_at": "2018-02-24T06:56:13",		
				"policies": {			
								"direction": "egress",			
								"max_rate": "20000000",			
								"protocol": "ip"		
							},		
				"shared": false,		
				"type": "ratelimit"	
			},	
			{		
				"description": "\u5165\u53e3",		
				"tags": [],		
				"updated_at": "2018-02-24T06:56:46",		
				"id": "44cc03eb-c2b2-46c6-a9cc-f266e7c5702f",		
				"name": "test2",		
				"admin_state_up": true,		
				"tenant_id": "aebf7515b0a343948d68a57d4a8ef580",		
				"created_at": "2018-02-24T06:56:46",		
				"policies": {			
								"direction": "ingress",			
								"max_rate": "20000000",			
								"protocol": "ip"		
							},		
				"shared": false,		
				"type": "ratelimit"	
			},	
			{		
				"description": "\u9ed8\u8ba4\u5165\u53e3QoS",		
				"tags": ["default"],		
				"updated_at": "2018-02-10T08:33:14",		
				"id": "495f91a9-c2eb-4e01-b321-db46b3c49737",		
				"name": "default_ingress",		
				"admin_state_up": true,		
				"tenant_id": "b7a9461b96034b989406ea8d36553ff3",		
				"created_at": "2018-02-10T08:33:14",		
				"policies": {			
								"direction": "ingress",			
								"max_rate": "100000000",			
								"protocol": "ip"		
							},		
				"shared": true,		
				"type": "ratelimit"	
			},	
			{		
				"description": "\u9ed8\u8ba4\u51fa\u53e3QoS",		
				"tags": ["default"],		
				"updated_at": "2018-02-10T08:33:15",		
				"id": "706823ec-c432-44f7-bcb3-6dfe2153db4b",		
				"name": "default_egress",		
				"admin_state_up": true,		
				"tenant_id": "b7a9461b96034b989406ea8d36553ff3",		
				"created_at": "2018-02-10T08:33:15",		
				"policies": {			
								"direction": "egress",			
								"max_rate": "100000000",			
								"protocol": "ip"		
							},		
				"shared": true,		
				"type": "ratelimit"	
			}
		]
	*/
	Document document;
	if (document.Parse(result.c_str()).HasParseError())
	{
		LOG_INFO("Qos rule result has error.");
		return FALSE;
	}
	if (document.HasMember("qoses") && (document["qoses"].IsArray()))
	{
		Value& qos_rule_array = document["qoses"];
		UINT4 index = 0;
		
		for (; index < qos_rule_array.Size(); ++index)
		{
			COpenstackQosRule* qos_rule = new COpenstackQosRule();
			const Value& qos_rule_object = qos_rule_array[index];
			if (qos_rule_object.HasMember("id") && qos_rule_object["id"].IsString())
			{
				LOG_DEBUG_FMT("id is %s",qos_rule_object["id"].GetString());
				qos_rule->SetQosId(qos_rule_object["id"].GetString());
			}
			if (qos_rule_object.HasMember("name") && qos_rule_object["name"].IsString())
			{
				LOG_DEBUG_FMT("name is %s",qos_rule_object["name"].GetString());
				qos_rule->SetRuleName(qos_rule_object["name"].GetString());
			}
			if (qos_rule_object.HasMember("description") && qos_rule_object["description"].IsString())
			{
				LOG_DEBUG_FMT("description is %s",qos_rule_object["description"].GetString());
				qos_rule->SetDescript(qos_rule_object["description"].GetString());
			}
			if (qos_rule_object.HasMember("tenant_id") && qos_rule_object["tenant_id"].IsString())
			{
				LOG_DEBUG_FMT("tenant_id is %s",qos_rule_object["tenant_id"].GetString());
				qos_rule->SetTenantId(qos_rule_object["tenant_id"].GetString());
			}
			if (qos_rule_object.HasMember("type") && qos_rule_object["type"].IsString())
			{
				LOG_DEBUG_FMT("type is %s",qos_rule_object["type"].GetString());
				qos_rule->SetQosType(qos_rule_object["type"].GetString());
			}

			if (qos_rule_object.HasMember("shared") && qos_rule_object["shared"].IsBool())
			{
				LOG_DEBUG_FMT("shared is %d",qos_rule_object["shared"].GetBool());
				qos_rule->SetShared(qos_rule_object["shared"].GetBool());
			}

			if (qos_rule_object.HasMember("policies"))
			{
				LOG_DEBUG_FMT("policies is %d",qos_rule_object["policies"].IsArray());

				const Value& qos_policy = qos_rule_object["policies"];
				if (qos_policy.HasMember("direction") && qos_policy["direction"].IsString())
				{
					LOG_DEBUG_FMT("direction is %s",qos_policy["direction"].GetString());
					BOOL ret = (0 == strcmp("ingress", qos_policy["direction"].GetString()))? TRUE : FALSE;
					qos_rule->SetDerection(ret);
				}
				if (qos_policy.HasMember("max_rate") && qos_policy["max_rate"].IsString())
				{
					LOG_DEBUG_FMT("max_rate is %s",qos_policy["max_rate"].GetString());
					UINT8 max_rate = atol(qos_policy["max_rate"].GetString());
					qos_rule->SetMaxRate(max_rate);	
				}
				if (qos_policy.HasMember("protocol") && qos_policy["protocol"].IsString())
				{
					LOG_DEBUG_FMT("protocol is %s",qos_policy["protocol"].GetString());
					qos_rule->SetProtocol(qos_policy["protocol"].GetString());
				}
			}
			
			if (FALSE == addOpenstackQosRule(qos_rule))
			{
				delete qos_rule;
			}	
		}
	}
	return TRUE;
}

BOOL COpenstackResource::addOpenstackPort(CSmartPtr<COpenstackPort> port)
{
	if (port.isNull())
		return FALSE;
	
	//LOG_ERROR_FMT("%s %d port->getFixedFirstIp=0x%x port->getFixedFristSubnetId().c_str=%s",FN,LN, port->getFixedFirstIp(), port->getFixedFristSubnetId().c_str());
	CSmartPtr<COpenstackPort> portOld = findOpenstackPort(port->getId());
    if (portOld.isNull())
    {
        // 添加
		Base_Port* CreatedPort =BasePortConventor_fromCOpenstackPort(*port);
		G_NetworkingEventReporter.report_C_Port(CreatedPort,PERSTANCE_FALSE);
		
        port->setCheckStatus(CHECK_CREATE);
        m_port_list.insert(std::make_pair(port->getId(), port));

        //LOG_INFO_FMT("add port:%s", port->getMacAddress().c_str());

        // 通知主机新增
        //CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyAddPort(port);

        // 上报事件：主机关联安全组新建
        reportSecurityGroupEvent(port, portOld);
    }
    else
    {
		portOld->setCheckStatus(CHECK_UPDATE);
		port->setCheckStatus(CHECK_UPDATE);

        // 判断信息更新
		if (!portOld->Compare(*port))
		{
			Base_Port* UpdatedPort =BasePortConventor_fromCOpenstackPort(*port);
			G_NetworkingEventReporter.report_U_Port(UpdatedPort,PERSTANCE_FALSE);

			// 通知主机更新
			//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyUpdatePort(port);  

            // 上报事件：主机关联安全组更新
            if (portOld->securityGroupUpdated(*port))
                reportSecurityGroupEvent(port, portOld);

			m_port_list.erase(port->getId());
			m_port_list.insert(std::make_pair(port->getId(), port));
            return TRUE;
		}

        // 上报事件：主机关联安全组更新
        if (portOld->securityGroupUpdated(*port))
        {
            reportSecurityGroupEvent(port, portOld);
            portOld->setSecurtyGroups(port->getSecurtyGroups());
        }
    }

    return FALSE;
}

CSmartPtr<COpenstackPort> COpenstackResource::findOpenstackPort(const std::string & id)
{
    COpenstackPortMap::iterator iter = m_port_list.find(id);
    if (iter != m_port_list.end())
        return iter->second;

    return CSmartPtr<COpenstackPort>(NULL);
}

CSmartPtr<COpenstackPort> COpenstackResource::findOpenstackPortByIp(const UINT4 ip_address)
{
    STL_FOR_LOOP(m_port_list, iter)
    {
        if (ip_address == iter->second->getFixedFirstIp())
        {
            //LOG_INFO_FMT("ip address is %s",ip2number((*iter)->getIpAddress().c_str()));
            return iter->second;
        }

    }

    return CSmartPtr<COpenstackPort>(NULL);
}

COpenstackSubnet* COpenstackResource::findSubnetByNetworkId(const std::string& network_id)
{
   STL_FOR_LOOP(m_subnet_list, iter)
   {
       if (iter->second->getNetworkId() == network_id)
       {
           return iter->second;
       }
   }
  
   return NULL;
}

BOOL COpenstackResource::addOpenstackFloatingip(COpenstackFloatingip* floatingip)
{
	if(NULL == floatingip)
	{
		return FALSE;
	}
	COpenstackFloatingip* findFloatingIpNode = findOpenstackFloatingip(floatingip->getId());
	// 如果不存�?
	if (NULL == findFloatingIpNode)
	{
		Base_Floating *CreatedFloating =BaseFloatingConventor_fromCOpenstackFloatingip(floatingip);
		G_NetworkingEventReporter.report_C_Floating(CreatedFloating,PERSTANCE_FALSE);
		
		// 添加
		floatingip->setCheckStatus(CHECK_CREATE);
		m_floatingip_list.insert(std::make_pair(floatingip->getId(), floatingip));
		// 通知新增
       // CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyAddFloatingIp(floatingip);		
	}
	else
	{
		findFloatingIpNode->setCheckStatus(CHECK_UPDATE);
		floatingip->setCheckStatus(CHECK_UPDATE);
		if(TRUE != floatingip->Compare(findFloatingIpNode))
		{
			Base_Floating *UpdatedFloating =BaseFloatingConventor_fromCOpenstackFloatingip(floatingip);
			G_NetworkingEventReporter.report_U_Floating(UpdatedFloating,PERSTANCE_FALSE);
			
			//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyUpdateFloatingIp(floatingip);
			//findFloatingIpNode->SetObjectValue(floatingip);
			m_floatingip_list.erase(floatingip->getId());
			m_floatingip_list.insert(std::make_pair(floatingip->getId(), floatingip));

		}
	}

	return TRUE;
}

COpenstackFloatingip* COpenstackResource::findOpenstackFloatingip(const std::string& id)
{
	std::map<string ,COpenstackFloatingip* >::iterator iter;
	iter = m_floatingip_list.find(id);
	if(m_floatingip_list.end() == iter)
	{
		return NULL;
	}
	return iter->second;
}

BOOL COpenstackResource::addOpenstackSecurityGroups(const COpenstackSecurityGroupMap& securityGroups)
{
    LOG_INFO_FMT("group rule number[%lu]", securityGroups.size());

    for (COpenstackSecurityGroupMap::const_iterator it_new = securityGroups.begin();
         it_new != securityGroups.end();
         ++ it_new)
    {
        COpenstackSecurityGroupMap::iterator it_old = m_securitygroup_list.find(it_new->second->getId()); 
        if (it_old != m_securitygroup_list.end())
        {
            if (!it_old->second->Compare(*it_new->second))
            {
                //updated
                reportSecurityGroupRuleEvent(it_new->second, EVENT_TYPE_SECURITY_GROUP_RULE_U, EVENT_REASON_SECURITY_GROUP_RULE_U);
            }

            m_securitygroup_list.erase(it_old);
        }
        else
        {
            //created
            reportSecurityGroupRuleEvent(it_new->second, EVENT_TYPE_SECURITY_GROUP_RULE_C, EVENT_REASON_SECURITY_GROUP_RULE_C);
        }
    }

    STL_FOR_LOOP(m_securitygroup_list, it)
    {
        //deleted
        reportSecurityGroupRuleEvent(it->second, EVENT_TYPE_SECURITY_GROUP_RULE_D, EVENT_REASON_SECURITY_GROUP_RULE_D);
    }

    m_securitygroup_list = securityGroups;

	return TRUE;
}

BOOL COpenstackResource::addOpenstackSecurityGroup(CSmartPtr<COpenstackSecurityGroup> securitygroup)
{
	CSmartPtr<COpenstackSecurityGroup> findSecurityGroupNode = findOpenstackSecurityGroup(securitygroup->getId());
	if (findSecurityGroupNode.isNull())
	{
		//添加
		securitygroup->setCheckStatus(CHECK_CREATE);
		m_securitygroup_list.insert(std::make_pair(securitygroup->getId(), securitygroup));
		//通知更新
		return TRUE;
	}
	else
	{
		findSecurityGroupNode->setCheckStatus(CHECK_UPDATE);
		//判断信息更新

		//通知更新
	}

	return FALSE;
}

CSmartPtr<COpenstackSecurityGroup> COpenstackResource::findOpenstackSecurityGroup(const std::string& id)
{
	COpenstackSecurityGroupMap::iterator iter = m_securitygroup_list.find(id);
	if (iter != m_securitygroup_list.end())
    	return iter->second;

    return CSmartPtr<COpenstackSecurityGroup>(NULL);
}

BOOL COpenstackResource::addOpenstackNetwork(COpenstackNetwork* network)
{
	if(NULL == network)
	{
		return FALSE;
	}
	COpenstackNetwork* findNetworkNode = findOpenstackNetwork(network->getId());
	if (NULL == findNetworkNode)
	{
		Base_Network* CreatedNetwork = BaseNetworkConventor_fromCOpenstackNetwork(network);
		G_NetworkingEventReporter.report_C_Network(CreatedNetwork,PERSTANCE_FALSE);
		
		//添加
		network->setCheckStatus(CHECK_CREATE);
		m_network_list.insert(std::make_pair(network->getId(), network));
		
		//通知更新
       // CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyAddNetwork(network);
	}
	else
	{
		findNetworkNode->setCheckStatus(CHECK_UPDATE);
		network->setCheckStatus(CHECK_UPDATE);
		//判断信息更新

		if(TRUE != network->Compare(findNetworkNode))
		{
			Base_Network* UpdatedNetwork = BaseNetworkConventor_fromCOpenstackNetwork(network);
			G_NetworkingEventReporter.report_U_Network(UpdatedNetwork,PERSTANCE_FALSE);
			
			//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyUpdateNetwork(network);
			//findNetworkNode->SetObjectValue( network);
			m_network_list.erase(network->getId());
			m_network_list.insert(std::make_pair(network->getId(), network));
		}

	}
	return TRUE;
}

COpenstackNetwork* COpenstackResource::findOpenstackNetwork(const std::string& id)
{
	std::map<string ,COpenstackNetwork* >::iterator iter;
	iter = m_network_list.find(id);
	if(m_network_list.end() == iter)
	{
		return NULL;
	}
	return iter->second;
}

BOOL COpenstackResource::addOpenstackSubnet(COpenstackSubnet* subnet)
{
	if(NULL == subnet)
	{
		return FALSE;
	}
	COpenstackSubnet* findSubnetNode = findOpenstackSubnet(subnet->getId());
	if (NULL == findSubnetNode)
	{
		Base_Subnet* CreatedSubnet =BaseSubnetConventor_fromCOpenstackSubnet(subnet);
		G_NetworkingEventReporter.report_C_Subnet(CreatedSubnet,PERSTANCE_FALSE);
		
		subnet->setCheckStatus(CHECK_CREATE);
		m_subnet_list.insert(std::make_pair(subnet->getId(), subnet));

		  // 通知主机新增
		//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyAddSubnet(subnet);

	}
	else
	{
		findSubnetNode->setCheckStatus(CHECK_UPDATE);
		subnet->setCheckStatus(CHECK_UPDATE);
		if(TRUE != subnet->Compare(findSubnetNode))
		{
			Base_Subnet* UpdatedSubnet =BaseSubnetConventor_fromCOpenstackSubnet(subnet);
			G_NetworkingEventReporter.report_U_Subnet(UpdatedSubnet,PERSTANCE_FALSE);
			
			//CNotifyMgr::getInstance()->getNotifyOpenstack()->notifyUpdateSubnet(subnet);
			//findSubnetNode->SetObjectValue(subnet);
			m_subnet_list.erase(subnet->getId());
			m_subnet_list.insert(std::make_pair(subnet->getId(), subnet));
		}
		//通知更新
	}
	return TRUE;
}

COpenstackSubnet* COpenstackResource::findOpenstackSubnet(const std::string& id)
{
	std::map<string ,COpenstackSubnet* >::iterator iter;
	iter = m_subnet_list.find(id);
	if(m_subnet_list.end() == iter)
	{
		return NULL;
	}
	return iter->second;
}

BOOL COpenstackResource::addOpenstackRouters(const COpenstackRouterMap& routers)
{
    LOG_INFO_FMT("openstack routers size[%lu]", routers.size());

    COpenstackRouterMap routers_new = routers;
    COpenstackRouterMap routers_old = m_router_list;

    //filter out unchanged
    COpenstackRouterMap::iterator itRouter_new = routers_new.begin();
    while (itRouter_new != routers_new.end())
    {
        CSmartPtr<COpenstackRouter>& router_new = itRouter_new->second;
        if (router_new.isNull())
        {
            ++ itRouter_new;
            continue;
        }

        COpenstackRouterMap::iterator itRouter_old = routers_old.find(router_new->getId());
        if (itRouter_old == routers_old.end())
        {
            ++ itRouter_new;
            continue;
        }

        CSmartPtr<COpenstackRouter>& router_old = itRouter_old->second;
        if (router_old.isNull())
        {
            ++ itRouter_new;
            continue;
        }

        const vector<string>& subnetIds_new = router_new->getExtSubnetIds();
        const vector<string>& fixedIps_new = router_new->getFixedIps();
        const vector<COpenstackPortforward>& portForwards_new = router_new->getPortForwards();
        const vector<string>& subnetIds_old = router_old->getExtSubnetIds();
        const vector<string>& fixedIps_old = router_old->getFixedIps();
        const vector<COpenstackPortforward>& portForwards_old = router_old->getPortForwards();

        //if network_id of external_gateway_info changed
        if (router_new->getExtNetworkId().compare(router_old->getExtNetworkId()) != 0)
        {
            ++ itRouter_new;
            continue;
        }

        //if external_fixed_ips of external_gateway_info changed
        if (subnetIds_new.size() != subnetIds_old.size())
        {
            ++ itRouter_new;
            continue;
        }
        if (fixedIps_new.size() != fixedIps_old.size())
        {
            ++ itRouter_new;
            continue;
        }
        BOOL fixedIpsChanged = FALSE;
        STL_FOR_LOOP(fixedIps_new, it_new)
        {
            BOOL fixedIpChanged = TRUE;
            STL_FOR_LOOP(fixedIps_old, it_old)
            {
                if ((*it_new).compare(*it_old) == 0)
                {
                    if (subnetIds_new[it_new-fixedIps_new.begin()].compare(subnetIds_old[it_old-fixedIps_old.begin()]) == 0)
                        fixedIpChanged = FALSE;
                    break;
                }
            }
            if (fixedIpChanged)
            {
                fixedIpsChanged = TRUE;
                break;
            }
        }
        if (fixedIpsChanged)
        {
            ++ itRouter_new;
            continue;
        }

        //if portforwardings changed
        if (portForwards_new.size() != portForwards_old.size())
        {
            ++ itRouter_new;
            continue;
        }
        BOOL portForwardsChanged = FALSE;
        STL_FOR_LOOP(portForwards_new, it_new)
        {
            BOOL portForwardChanged = TRUE;
            STL_FOR_LOOP(portForwards_old, it_old)
            {
                if ((*it_new).Compare(*it_old))
                {
                    portForwardChanged = FALSE;
                    break;
                }
            }
            if (portForwardChanged)
            {
                portForwardsChanged = TRUE;
                break;
            }
        }
        if (portForwardsChanged)
        {
            ++ itRouter_new;
            continue;
        }

        //erase unchanged
        COpenstackRouterMap::iterator itRouter_erase = itRouter_new++;
        routers_new.erase(itRouter_erase);
        routers_old.erase(itRouter_old);
    }

    if ((routers_old.size() > 0) || (routers_new.size() > 0))
        LOG_WARN_FMT("openstack routers size[%lu], delete routers size[%lu], create routers size[%lu]", 
            routers.size(), routers_old.size(), routers_new.size());

    //report delete event upon before changed
    STL_FOR_LOOP(routers_old, itRouter)
    {
        reportPortforwardRuleEvent(itRouter->second, EVENT_TYPE_PORTFORWARD_RULE_D, EVENT_REASON_PORTFORWARD_RULE_D);
    }

    //report create event upon after changed
    STL_FOR_LOOP(routers_new, itRouter)
    {
        reportPortforwardRuleEvent(itRouter->second, EVENT_TYPE_PORTFORWARD_RULE_C, EVENT_REASON_PORTFORWARD_RULE_C);
    }

    m_router_list = routers;

	return TRUE;
}

BOOL COpenstackResource::addOpenstackQosRule(COpenstackQosRule* qosRule)
{
	if(NULL == qosRule)
	{
		return FALSE;
	}
	COpenstackQosRule* findQosRuleNode = findOpenstackQosRule(qosRule->GetQosId());
	if (NULL == findQosRuleNode)
	{
		qosRule->setCheckStatus(CHECK_CREATE);
		m_qosrule_list.insert(std::make_pair(qosRule->GetQosId(), qosRule));
		CQosEventReport::getInstance()->report(EVENT_TYPE_QOS_RULE_C, EVENT_TYPE_QOS_RULE_C,  *qosRule);
	}
	else
	{
		findQosRuleNode->setCheckStatus(CHECK_UPDATE);
		qosRule->setCheckStatus(CHECK_UPDATE);
		if(TRUE != qosRule->CompareObjectValue(findQosRuleNode))
		{
			//Base_Subnet* UpdatedSubnet =BaseSubnetConventor_fromCOpenstackSubnet(subnet);
			//G_NetworkingEventReporter.report_U_Subnet(UpdatedSubnet);
			
			m_qosrule_list.erase(qosRule->GetQosId());
			m_qosrule_list.insert(std::make_pair(qosRule->GetQosId(), qosRule));
			
			CQosEventReport::getInstance()->report(EVENT_TYPE_QOS_RULE_U, EVENT_TYPE_QOS_RULE_U,  *qosRule);
		}
	}
		
	return TRUE;
}

COpenstackQosRule* COpenstackResource::findOpenstackQosRule(const std::string & qos_id)
{
	std::map<string ,COpenstackQosRule* >::iterator iter;
	iter = m_qosrule_list.find(qos_id);
	if(m_qosrule_list.end() == iter)
	{
		return NULL;
	}
	return iter->second;
}


BOOL COpenstackResource::addOpenstackQosBindPort(COpenstackQosBind* qosBind)
{
	if(NULL == qosBind)
	{
		return FALSE;
	}
	COpenstackQosBind* findQosBindPort = findOpenstackQosBindPort(qosBind->GetPortId());
	if(NULL == findQosBindPort)
	{
		qosBind->setCheckStatus(CHECK_CREATE);
		m_qosbind_list.insert(std::make_pair(qosBind->GetPortId(), qosBind));
		CQosEventReport::getInstance()->report(EVENT_TYPE_QOS_ATTCH, EVENT_TYPE_QOS_ATTCH, qosBind->GetPortMac(), qosBind->GetIngressQosId(), qosBind->GetEgressQosId());
	}
	else
	{
		findQosBindPort->setCheckStatus(CHECK_CREATE);
		qosBind->setCheckStatus(CHECK_CREATE);

		if(TRUE != qosBind->CompareObjectValue(findQosBindPort))
		{
			m_qosbind_list.erase(qosBind->GetPortId());
			m_qosbind_list.insert(std::make_pair(qosBind->GetPortId(), qosBind));
			
			CQosEventReport::getInstance()->report(EVENT_TYPE_QOS_UPDATE, EVENT_TYPE_QOS_UPDATE, qosBind->GetPortMac(), qosBind->GetIngressQosId(), qosBind->GetEgressQosId());
		}
	}
	return TRUE;
}

COpenstackQosBind* COpenstackResource::findOpenstackQosBindPort(const std::string & qosBind_portid)
{
	std::map<string ,COpenstackQosBind* >::iterator iter;
	iter = m_qosbind_list.find(qosBind_portid);
	if(m_qosbind_list.end() == iter)
	{
		return NULL;
	}
	return iter->second;
}
UINT4 COpenstackResource::getFloatingipbyFixedIp(const UINT4& fixedip)
{
    std::map<string, COpenstackFloatingip*> floatingipList = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getOpenstackFloatingipList();
    STL_FOR_LOOP(floatingipList,iter)
    {
        if (ip2number(iter->second->getFixedIp().c_str()) == fixedip)
        {
            return ip2number(iter->second->getFloatingIp().c_str());
        }
    }
    return 0;
}


UINT4 COpenstackResource::getFixedipByFloatingip(const UINT4& floatingip)
{
    std::map<string, COpenstackFloatingip*> floatingipList = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getOpenstackFloatingipList();
    STL_FOR_LOOP(floatingipList,iter)
    {
        if (ip2number(iter->second->getFloatingIp().c_str()) == floatingip)
        {
            return ip2number(iter->second->getFixedIp().c_str());
        }
    }
    return 0;
}

void COpenstackResource::reportSecurityGroupEvent(CSmartPtr<COpenstackPort> portNew, CSmartPtr<COpenstackPort> portOld)
{
    if (portNew.isNull())
        return;

    if (portOld.isNull())
    {
        const list<CSecurityGroupId>& sgp_new = portNew->getSecurtyGroups();
        STL_FOR_LOOP(sgp_new, it)
            CSecurityGroupEventReportor::getInstance()->report(EVENT_TYPE_SECURITY_GROUP_ATTACH, 
                EVENT_REASON_SECURITY_GROUP_ATTACH, portNew->getMacAddress(), *it);
    }
    else
    {
        list<CSecurityGroupId> sgp_new = portNew->getSecurtyGroups();
        list<CSecurityGroupId> sgp_old = portOld->getSecurtyGroups();

        //filter out unchanged
        list<CSecurityGroupId>::iterator it_new = sgp_new.begin();
        while (it_new != sgp_new.end())
        {
            BOOL unchanged = FALSE;

            for (list<CSecurityGroupId>::iterator it_old = sgp_old.begin(); 
                 it_old != sgp_old.end(); 
                 ++ it_old)
            {
                if ((*it_old).compare(*it_new) == 0)
                {
                    sgp_old.erase(it_old);
                    unchanged = TRUE;
                    break;
                }
            }

            if (unchanged)
            {
                list<CSecurityGroupId>::iterator it_tmp = it_new ++;
                sgp_new.erase(it_tmp);
            }
            else
            {
                ++ it_new;
            }
        }

        STL_FOR_LOOP(sgp_old, it)
            CSecurityGroupEventReportor::getInstance()->report(EVENT_TYPE_SECURITY_GROUP_DETACH, 
                EVENT_REASON_SECURITY_GROUP_DETACH, portOld->getMacAddress(), *it);

        STL_FOR_LOOP(sgp_new, it)
            CSecurityGroupEventReportor::getInstance()->report(EVENT_TYPE_SECURITY_GROUP_ATTACH, 
                EVENT_REASON_SECURITY_GROUP_ATTACH, portNew->getMacAddress(), *it);
    }
}

void COpenstackResource::reportSecurityGroupRuleEvent(const CSmartPtr<COpenstackSecurityGroup> securityGroupRule, INT4 eventId, INT4 reason)
{
    if (securityGroupRule.isNull())
        return;

    if (securityGroupRule->getEthertype().compare("IPv4") != 0)
        return;
    
    CSecurityGroupRule rule;
    rule.setEnabled(securityGroupRule->getEnabled());
    rule.setRuleId(securityGroupRule->getId());
    rule.setGroupId(securityGroupRule->getSecurityGroupId());
    rule.setProtocol(securityGroupRule->getProtocol());
    rule.setDirection(securityGroupRule->getDirection());
    rule.setPortMin(securityGroupRule->getPortRangeMin());
    rule.setPortMax(securityGroupRule->getPortRangeMax());
    rule.setPriority(securityGroupRule->getPriority());
    rule.setAction(securityGroupRule->getAction());

    UINT4 ip = 0, mask = 0;
    getIPAndMaskByCidr(securityGroupRule->getRemoteIpPrefix(), ip, mask);
    rule.setRemoteIp(ip);
    rule.setRemoteIpMask(mask);

    CSecurityGroupEventReportor::getInstance()->report(eventId, reason, rule);
}

void COpenstackResource::getIPAndMaskByCidr(const std::string& cidr, UINT4& ip, UINT4& mask)
{
    ip = 0;
    mask = 0;

    if (cidr.empty())
        return;

    std::string::size_type pos = cidr.find("/");
    if (std::string::npos != pos)
    {
        std::string ipStr = cidr.substr(0, pos);
        std::string maskStr = cidr.substr(pos+1);
        ip = ntohl(ip2number(ipStr.c_str()));
        mask = atoi(maskStr.c_str());
        if (mask > 32)
            mask = 32;
        ip >>= (32 - mask);
        ip <<= (32 - mask);
    }
    else
    {
        ip = ntohl(ip2number(cidr.c_str()));
        mask = 32;
    }
    LOG_DEBUG_FMT("cidr[%s]==>ip[%s]mask[%u]", cidr.c_str(), inet_htoa(ip), mask);
}

void COpenstackResource::reportPortforwardRuleEvent(const CSmartPtr<COpenstackRouter> router, INT4 eventId, INT4 reason)
{
    if (router.isNull())
        return;

    LOG_INFO_FMT("report event[%d]reason[%d] on networkId[%s]", eventId, reason, router->getExtNetworkId().c_str());

    const vector<string>& subnetIds = router->getExtSubnetIds();
    const vector<string>& fixedIps = router->getFixedIps();
    const vector<COpenstackPortforward>& portForwards = router->getPortForwards();
    if (subnetIds.empty() || fixedIps.empty() || portForwards.empty())
        return;

    for (UINT4 i = 0; i < fixedIps.size(); ++i)
    {
        if (i >= subnetIds.size())
            break;

        STL_FOR_LOOP(portForwards, it)
        {
            CPortforwardRule rule;

            BOOL enabled = (it->m_status.compare("ENABLE") == 0) || (it->m_status.compare("enable") == 0);
            rule.setEnabled(enabled);
            rule.setProtocol(it->m_protocol);
            UINT4 outIp = ip2number(fixedIps[i].c_str());
            rule.setOutsideIp(ntohl(outIp));
            UINT4 inIp = ip2number(it->m_insideAddr.c_str());
            rule.setInsideIp(ntohl(inIp));
            
            //ensure start <= end for outside port range
            const INT1* portStr = it->m_outsidePort.c_str();
            UINT2 portStart = atoi(portStr);
            UINT2 portEnd   = portStart;
            const char* ptr = strchr(portStr, '-');
            if (NULL != ptr) 
            {
                portEnd = atoi(ptr+1);
                if (portEnd < portStart) 
                {
                    UINT2 portSwap = portStart;
                    portStart = portEnd;
                    portEnd = portSwap;
                }
            }
            rule.setOutsidePort(portStart, portEnd);
            
            //ensure start <= end for inside port range
            portStr = it->m_insidePort.c_str();
            portStart = atoi(portStr);
            portEnd   = portStart;
            ptr = strchr(portStr, '-');
            if (NULL != ptr) 
            { 
                portEnd = atoi(ptr+1);
                if (portEnd < portStart) 
                {
                    UINT2 portSwap = portStart;
                    portStart = portEnd;
                    portEnd = portSwap;
                }
            }
            rule.setInsidePort(portStart, portEnd);

            rule.setNetworkId(router->getExtNetworkId());
            rule.setSubnetId(subnetIds[i]);

            CPortforwardEventReportor::getInstance()->report(eventId, reason, rule);
        }
    }
}


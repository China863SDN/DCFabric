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
*   File Name   : COpenstackDefine.cpp  *
*   Author      : bnc xflu              *
*   Create Date : 2016-9-05             *
*   Version     : 1.0                   *
*   Function    : .                     *
*                                                                             *
******************************************************************************/
#include "COpenstackDefine.h"
#include "log.h"

using namespace bnc::openstack;

COpenstackDefine* COpenstackDefine::m_pInstance = 0;
COpenstackDefine::COpenstackDefine()
{
}

COpenstackDefine::~COpenstackDefine()
{
}

COpenstackDefine* COpenstackDefine::getInstance()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new COpenstackDefine();
    }

    return m_pInstance;
}

BOOL COpenstackDefine::getStrFromNetwork(bnc::openstack::version ver, bnc::openstack::network_type type, std::string& str)
{
    // 获取版本号
    if (!getStrFromVersion(ver, str))
    {
        str.clear();
        return FALSE;
    }

    // 获取字符串
    if (!getStrFromType(type, str))
    {
        str.clear();
        return FALSE;
    }

    // 暂时没有增加这个功能
    if (!isValid(0, str))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL COpenstackDefine::getStrFromVersion(bnc::openstack::version ver, std::string & str)
{
    str.append("/");

    if (bnc::openstack::empty_version == ver)
    {
        return TRUE;
    }

    if (bnc::openstack::v2 == ver)
    {
        str.append("v2.0");

        return TRUE;
    }

    LOG_INFO_FMT("unsupported openstack version %d", ver);
    return FALSE;
}

BOOL COpenstackDefine::getStrFromType(bnc::openstack::network_type type, std::string & str)
{
    str.append("/");

    switch (type)
    {
    case versions:
        str.append("");
        break;
    case extensions:
        str.append("extensions");
        break;
    case networks:
        str.append("networks");
        break;
    case subnets:
        str.append("subnets");
        break;
    case ports:
        str.append("ports");
        break;
    case subnetpools:
        str.append("subnetpools");
        break;
    case routers:
        str.append("routers");
        break;
    case floatingips:
        str.append("floatingips");
        break;
    case securitygroups:
        str.append("security-groups");
        break;
    case securitygrouprules:
        str.append("security-group-rules");
        break;
    case quotas:
        str.append("quotas");
        break;
    case serviceproviders:
        str.append("service-providers");
        break;
    case flavors:
        str.append("flavors");
        break;
    case tags:
        /*
         * 注意,Tag的用法
         * /v2.0/{resource_type}/{resource_id}/tags
         */
        str.append("tags");
        break;
    case networkipavailability:
        str.append("network-ip-availabilities");
        break;
    case qos:
        str.append("qos");
        break;
	case qosrules:
		str.append("qoses");
		break;
    case metering:
        str.append("metering");
        break;
    case lbaas:
        str.append("lbaas");
        break;
    case lbaas_v1:
        str.append("lb");
        break;
    case firewallpolicies:
        str.append("firewall_policies");
        break;
    default:
        LOG_INFO_FMT("Unsupport openstack type: %d", type);
        return FALSE;
        break;
    }

    return TRUE;
}

BOOL COpenstackDefine::isValid(UINT4 openstack_version, const std::string & str) const
{
    return TRUE;
}

bnc::openstack::device_type COpenstackDefine::getDeviceTypeFromStr
                                              (const std::string & str) const
{
    bnc::openstack::device_type return_type = bnc::openstack::device_unknown;

    if ((str == std::string("compute:nova"))||(str == std::string("compute:None")))
    {
        return_type =  bnc::openstack::device_host;
    }
    else if (str == std::string("network:dhcp"))
    {
        return_type =  bnc::openstack::device_dhcp;
    }
    else if (str == std::string("network:router_interface"))
    {
        return_type = bnc::openstack::device_interface;
    }
    else if (str == std::string("network:floatingip"))
    {
        return_type = bnc::openstack::device_floatingip;
    }
    else if (str == std::string("neutron:LOADBALANCER"))
    {
        return_type = bnc::openstack::device_loadbalance;
    }
    else if (str == std::string("network:router_gateway"))
    {
        return_type = bnc::openstack::device_gateway;
    }
	else if(str == std::string("network:LOADBALANCER_HA"))
	{
		return_type = bnc::openstack::device_cloadblance_ha;
	}
	else if(str == std::string("network:LOADBALANCER_VIP"))
	{
		return_type = bnc::openstack::device_clloadblance_vip;
	}
    else
    {
        return_type = bnc::openstack::device_unknown;
    }

    return return_type;
}





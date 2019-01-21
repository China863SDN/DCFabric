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
*   File Name   : COpenstackPort.cpp            *
*   Author      : bnc xflu                      *
*   Create Date : 2016-9-7                      *
*   Version     : 1.0                           *
*   Function    : .                             *
*                                                                             *
******************************************************************************/
#include "COpenstackPort.h"
#include "log.h"
#include "comm-util.h"
#include "CSecurityGroupEventReportor.h"
#include "CHostMgr.h"

COpenstackPort::COpenstackPort():m_admin_state_up(FALSE),m_port_security_enabled(FALSE)
{

}

COpenstackPort::~COpenstackPort()
{

}

BOOL  COpenstackPort::Compare (const COpenstackPort& port)
{
	if ((0 != port.m_id.compare(this->m_id))||(0 != port.m_network_id.compare(this->m_network_id))||
		(0 != port.m_subnet_id.compare(this->m_subnet_id))||(0 != port.m_tenant_id.compare(this->m_tenant_id))||
		(0 != port.m_device_id.compare(this->m_device_id))||(port.getFixedFirstIp() != this->getFixedFirstIp())||
		(0 != port.m_mac_address.compare(this->m_mac_address)))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL  COpenstackPort::SetObjectValue(const COpenstackPort& port)
{
	this->m_opt_value = port.m_opt_value;
    this->m_status = port.m_status;
    this->m_name = port.m_name;
    //list<address_pair> m_allowed_address_pairs;
    this->m_admin_state_up = port.m_admin_state_up;
    this->m_network_id = port.m_network_id;
    this->m_ip_address = port.m_ip_address;
    //list<string> m_extra_dhcp_opts;
    this->m_opt_name = port.m_opt_name ;
    this-> m_updated_at = port.m_updated_at ;
    this-> m_id = port.m_id; 
    this-> m_subnet_id = port.m_subnet_id;
    this-> m_device_owner = port.m_device_owner;
    this-> m_tenant_id = port.m_tenant_id;
    this-> m_mac_address = port.m_mac_address ;
	this->m_port_security_enabled = port.m_port_security_enabled;
	this->m_device_id = port.m_device_id;
   // BOOL m_port_security_enabled;
    //map<string, string> m_fixed_ips;
   // string m_created_at;
   // list<CSecurityGroupId> m_security_groups;
    //string m_device_id;
	return TRUE;
}
BOOL  COpenstackPort::CompareObjectQosId(std::string & qos_ingress_id, std::string & qos_egress_id) const
{
	return TRUE;
}
BOOL COpenstackPort::securityGroupUpdated(const COpenstackPort& port)
{
	if (m_security_groups.size() != port.m_security_groups.size())
		return TRUE;

    STL_FOR_LOOP(port.m_security_groups, it_new)
    {
        BOOL existed = FALSE;
        STL_FOR_LOOP(m_security_groups, it_old)
        {
            if ((*it_old).compare(*it_new) == 0)
            {
                existed = TRUE;
                break;
            }
        }
        if (!existed)
        	return FALSE;
    }

	return TRUE;
}

UINT4 COpenstackPort::getFixedFirstIp() const
{
    STL_FOR_LOOP(m_fixed_ips, iter)
    {
        if (!iter->first.empty())
        {
            // LOG_INFO_FMT("fixed ip is %s", iter->first.c_str());
            return ip2number(iter->first.c_str());
        }
    }

    return 0;
}

const string & COpenstackPort::getFixedFristSubnetId() const
{
    STL_FOR_LOOP(m_fixed_ips, iter)
    {
        if (!iter->second.empty())
        {
            return iter->second;
        }
    }

    return NULL;
}

UINT1* COpenstackPort::getMac(UINT1* mac) const
{
    mac = macstr2hex((INT1*)m_mac_address.c_str(), mac);

    return mac;
}

bnc::openstack::device_type COpenstackPort::getDeviceType() const
{
    return COpenstackDefine::getInstance()->getDeviceTypeFromStr(m_device_owner);
}


void COpenstackPort::printParam() const
{
    LOG_INFO_FMT("m_opt_value: %s", m_opt_value.c_str());
    LOG_INFO_FMT("m_status: %s", m_status.c_str());
    LOG_INFO_FMT("m_name: %s", m_name.c_str());
    // LOG_INFO_FMT("allowed address pair: %s", m_name);
    LOG_INFO_FMT("m_admin_state_up: %d", m_admin_state_up);
    LOG_INFO_FMT("m_network_id: %s", m_network_id.c_str());
    LOG_INFO_FMT("m_ip_address: %s", m_ip_address.c_str());
    // LOG_INFO_FMT("m_extra_dhcp_opts: %s", m_ip_address);
    LOG_INFO_FMT("m_opt_name: %s", m_opt_name.c_str());
    LOG_INFO_FMT("m_updated_at: %s", m_updated_at.c_str());
    LOG_INFO_FMT("m_id: %s", m_id.c_str());
    LOG_INFO_FMT("m_subnet_id: %s", m_subnet_id.c_str());
    LOG_INFO_FMT("m_device_owner: %s", m_device_owner.c_str());
    LOG_INFO_FMT("m_tenant_id: %s", m_tenant_id.c_str());
    LOG_INFO_FMT("m_mac_address: %s", m_mac_address.c_str());
    LOG_INFO_FMT("m_port_security_enabled: %d", m_port_security_enabled);
    STL_FOR_LOOP(m_fixed_ips, iter)
    {
        LOG_INFO_FMT("m_fixed_ips: %s, %s", iter->first.c_str(), iter->second.c_str());
    }
    LOG_INFO_FMT("m_created_at: %s", m_created_at.c_str());
    // LOG_INFO_FMT("m_security_groups: %s", m_created_at);
    LOG_INFO_FMT("m_device_id: %s", m_device_id.c_str());
}


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
*   File Name   : COpenstackPort.h    *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_PORT_H
#define _COPENSTACK_PORT_H
#include <string>
#include <vector>
#include <map>
#include "bnc-type.h"
#include "COpenstackDefine.h"
#include "CRefObj.h"

/*
 * openstack Port
 * GET /v2.0/ports
 * 参考:http://developer.openstack.org/api-ref/networking/v2/
 * 参考:/neutron/callbacks/resources.py
 */
using namespace std;

typedef string address_pair;
typedef string CSecurityGroupId;

class COpenstackPort : public CRefObj
{
public:
    COpenstackPort();
    ~COpenstackPort();


	//compare
	BOOL  Compare(const COpenstackPort& port);
	BOOL  SetObjectValue(const COpenstackPort& port);
	BOOL  securityGroupUpdated(const COpenstackPort& port);
	
    /*
     * set方法
     */
    void setOptValue(const string & opt_value) { m_opt_value = opt_value; }

    void setStatus(const string & status) { m_status = status; }

    void setName(const string & name) { m_name = name; }

    // setAllowedAddressPair

    void setAdminStateUp(BOOL state) { m_admin_state_up = state; }

    void setNetworkId(const string & network_id) { m_network_id = network_id; }

    void setIpAddress(const string & ip_address) { m_ip_address = ip_address; }

    // setExtraDhcpOpts

    void setOptName(const string & opt_name) { m_opt_name = opt_name; }

    void setUpdateAt(const string & update_at) { m_updated_at = update_at; }

    void setId(const string & id) { m_id = id; }

    void setSubnetId(const string & subnet_id) { m_subnet_id = subnet_id; }

    void setDeviceOwner(const string & device_owner) { m_device_owner = device_owner; }

    void setTenantId(const string & tenant_id) { m_tenant_id = tenant_id; }

    void setMacAddress(const string & mac_address) { m_mac_address = mac_address; }

    void setPortSecurityEnabled(BOOL port_security_enabled) { m_port_security_enabled = port_security_enabled; }

    void setFixedIps(const map<string, string> & fixed_ips) { m_fixed_ips = fixed_ips; }

    void setCreatedAt(const string & created_at) { m_created_at = created_at; }

    void setSecurtyGroups(const list<CSecurityGroupId>& security_groups) {m_security_groups = security_groups;}

    void setDeviceId(const string & device_id) { m_device_id = device_id; }

	void setCheckStatus(UINT1 status){m_checkStatus = status; }
	

    /*
     * get方法
     */
    const string & getOptValue() const { return m_opt_value; }

    const string & getStatus() const { return m_status; }

    const string & getName() const { return m_name; }

    // setAllowedAddressPair

    BOOL getAdminStateUp() const { return m_admin_state_up; }

    const string & getNetworkId() const { return m_network_id; }

    const string & getIpAddress() const { return m_ip_address; }

    // setExtraDhcpOpts

    const string & getOptName() const { return m_opt_name; }

    const string & getUpdateAt() const { return m_updated_at; }

    const string & getId() const { return m_id; }

    const string & getSubnetId() const { return m_subnet_id; }

    const string & getDeviceOwner() const { return m_device_owner; }

    const string & getTenantId() const { return m_tenant_id; }

    const string & getMacAddress() const { return m_mac_address; }

    BOOL getPortSecurityEnabled() const { return m_port_security_enabled; }

    const map<string, string> & getFixedIps() const { return m_fixed_ips; }

    const string & getCreatedAt() const { return m_created_at; }

    const list<CSecurityGroupId>& getSecurtyGroups() const { return m_security_groups; }

    const string & getDeviceId() const { return m_device_id; }

    /*
     * 新增的适合内部存储的数据类型
     */
    UINT4 getFixedFirstIp() const;

    const string & getFixedFristSubnetId() const;

    UINT1* getMac(UINT1* mac) const;
	
	UINT1  getCheckStatus(){ return m_checkStatus; }
	
	BOOL  CompareObjectQosId(std::string & qos_ingress_id, std::string & qos_egress_id) const;
	
    bnc::openstack::device_type getDeviceType() const;

    /*
     * 测试用
     */
    void printParam() const;

private:
    string m_opt_value;
    string m_status;
    string m_name;
    list<address_pair> m_allowed_address_pairs;
    BOOL m_admin_state_up;
    string m_network_id;
    string m_ip_address;
    list<string> m_extra_dhcp_opts;
    string m_opt_name;
    string m_updated_at;
    string m_id;
    string m_subnet_id;
    string m_device_owner;
    string m_tenant_id;
    string m_mac_address;
    BOOL m_port_security_enabled;
    map<string, string> m_fixed_ips;
    string m_created_at;
    list<CSecurityGroupId> m_security_groups;
    string m_device_id;
	UINT1 m_checkStatus;
};


#endif

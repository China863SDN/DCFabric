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
*   File Name   : COpenstackSecurityGroup.h    *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_SECURITYGROUP_H
#define _COPENSTACK_SECURITYGROUP_H
#include<string>
#include"bnc-type.h"
#include"CRefObj.h"

using namespace std;

#define ICMP_TYPE_NULL  0xFF
#define ICMP_CODE_NULL  0xFF

class COpenstackSecurityGroup : public CRefObj
{
public:
    COpenstackSecurityGroup();
    ~COpenstackSecurityGroup();

	BOOL Compare(const COpenstackSecurityGroup& securityGroup);

    /*
     * set 方法
     */
    void setRemoteGroupId(const string& remote_group_id) { m_strRemoteGroupId = remote_group_id; }
    void setDirection(const string& direction) { m_strDirection = direction; }
    void setProtocol(const string& protocol) { m_strProtocol = protocol; }
    void setEthertype(const string& ethertype) { m_strEthertype = ethertype; }
    void setPortRangeMax(UINT2 port_range_max) { m_uiPortRangeMax = port_range_max; }
    void setPortRangeMin(UINT2 port_range_min) { m_uiPortRangeMin = port_range_min; }
    void setSecurityGroupId(const string& security_group_id) { m_strSecurityGroupId = security_group_id; }
    void setTenantId(const string& tenant_id) { m_strTenantId = tenant_id; }
    void setRemoteIpPrefix(const string& remote_ip_prefix) { m_strRemoteIpPrefix = remote_ip_prefix; }
    void setId(const string& id) { m_strId = id; }
    void setPriority(UINT2 priority) { m_priority = priority; }
    void setEnabled(BOOL enabled) { m_enabled = enabled; }
    void setAction(const string& action) { m_strAction = action; }

    /*
     * get 方法
     */
    const string& getRemoteGroupId() const { return m_strRemoteGroupId; }
    const string& getDirection() const { return m_strDirection; }
    const string& getProtocol() const { return m_strProtocol; }
    const string& getEthertype() const { return m_strEthertype; }
    UINT2 getPortRangeMax() const { return m_uiPortRangeMax; }
    UINT2 getPortRangeMin() const { return m_uiPortRangeMin; }
    const string& getSecurityGroupId() const { return m_strSecurityGroupId; }
    const string& getTenantId() const { return m_strTenantId; }
    const string& getRemoteIpPrefix() const { return m_strRemoteIpPrefix; }
    const string& getId() const { return m_strId; }
    UINT2 getPriority() const { return m_priority; }
    BOOL getEnabled() const { return m_enabled; }
    const string& getAction() const { return m_strAction; }

    void setCheckStatus(INT4 status) { m_checkStatus = status; }
    INT4 getCheckStatus() const { return m_checkStatus; }
	 
private:
    string m_strRemoteGroupId;
    string m_strDirection;
    string m_strProtocol;
    string m_strEthertype;
    UINT2 m_uiPortRangeMax;
    UINT2 m_uiPortRangeMin;
    string m_strSecurityGroupId;
    string m_strTenantId;
    string m_strRemoteIpPrefix;
    string m_strId;
	UINT2 m_priority;
	BOOL m_enabled;
    string m_strAction;
	INT4 m_checkStatus;
};


#endif

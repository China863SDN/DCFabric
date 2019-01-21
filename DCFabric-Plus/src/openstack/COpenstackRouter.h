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
*   File Name   : COpenstackRouter.h        *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_ROUTER_H
#define _COPENSTACK_ROUTER_H
#include<string>
#include"bnc-type.h"
#include"CRefObj.h"

using namespace std;

class COpenstackPortforward
{
public:
    COpenstackPortforward() {}
    ~COpenstackPortforward() {}

	BOOL Compare(const COpenstackPortforward& portforward) const;

    string m_status;
    string m_insideAddr;
    string m_protocol;
    string m_outsidePort;
    string m_insidePort;
};

class COpenstackRouter : public CRefObj
{
public:
    COpenstackRouter();
    ~COpenstackRouter();

	BOOL Compare(const COpenstackRouter& router);

    /*
     * set 方法
     */
    void setStatus(const string& status) { m_status = status; }
    void setExtNetworkId(const string& extNetworkId) { m_extNetworkId = extNetworkId; }
    void setEnableSnat(BOOL enableSnat) { m_enableSnat = enableSnat; }
    void setExtSubnetId(const string& extSubnetId) { m_extSubnetIds.push_back(extSubnetId); }
    void setExtFixedIp(const string&     extFixedIp) { m_extFixedIps.push_back(extFixedIp); }
    void setPortforward(const COpenstackPortforward&      portforward) { m_portForwards.push_back(portforward); }
    void setName(const string& name) { m_name = name; }
    void setGwPortId(const string& gwPortId) { m_gwPortId = gwPortId; }
    void setAdminStateUp(BOOL adminStateUp) { m_adminStateUp = adminStateUp; }
    void setTenantId(const string& tenant_id) { m_tenantId = tenant_id; }
    void setDistributed(BOOL distributed) { m_distributed = distributed; }
    void setHa(BOOL ha) { m_ha = ha; }
    void setId(const string& id) { m_id = id; }

    /*
     * get 方法
     */
    const string& getStatus() const { return m_status; }
    const string& getExtNetworkId() const { return m_extNetworkId; }
    const vector<string>& getExtSubnetIds() const { return m_extSubnetIds; }
    const vector<string>& getFixedIps() const { return m_extFixedIps; }
    const vector<COpenstackPortforward>& getPortForwards() const { return m_portForwards; }
    const string& getTenantId() const { return m_tenantId; }
    const string& getId() const { return m_id; }

    void setCheckStatus(INT4 status) { m_checkStatus = status; }
    INT4 getCheckStatus() const { return m_checkStatus; }
	 
private:
    string m_status;
    string m_extNetworkId;
    BOOL m_enableSnat;
    vector<string> m_extSubnetIds;
    vector<string> m_extFixedIps;
    vector<COpenstackPortforward> m_portForwards;
    string m_gwPortId;
	BOOL m_adminStateUp;
    string m_tenantId;
	BOOL m_distributed;
	BOOL m_ha;
    string m_id;
	INT4 m_checkStatus;
    string m_name;
};


#endif

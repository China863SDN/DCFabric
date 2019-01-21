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
*   File Name   : COpenstackFloatingip.h    *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_FLOATINGIP_H
#define _COPENSTACK_FLOATINGIP_H
#include "bnc-type.h"

using namespace std;
class COpenstackFloatingip
{
public:
    COpenstackFloatingip();
    ~COpenstackFloatingip();
	BOOL Compare(COpenstackFloatingip* floatingip);
	BOOL SetObjectValue(COpenstackFloatingip* floatingip);
    /*
     * get ID of the router
     */
    const string& getRouterId() const { return m_strRouterId; }

    /*
     * get network status
     */
    const string& getStatus() const { return m_strStatus; }

    /*
     * get the ID of tenant
     */
    const string& getTenantId() const { return m_strTenantId; }

    /*
     * get  the UUID of the network
     */
    const string& getFloatingNetId() const { return m_strFloatingNetId; }

    const string& getFixedIp() const { return m_strFixedIp; }

    const string& getFloatingIp() const { return m_strFloatingIp; }

    /*
     * get UUID of port
     */
    const string& getPortId() const { return m_strPortId; }

    /*
     * get UUID of network
     */
    const string& getId() const { return m_strId; }

    /*
     * set方法
     */
    void setRouterId(const string& router_id) { m_strRouterId = router_id; }

    void setStatus(const string& status) { m_strStatus = status; }

    void setTenantId(const string& tenant_id) { m_strTenantId = tenant_id; }

    void setFloatingNetId(const string& floating_net_id) { m_strFloatingNetId = floating_net_id; }

    void setFixedIp(const string& fixed_ip) { m_strFixedIp = fixed_ip; }

    void setPortId(const string& port_id) { m_strPortId = port_id; }

    void setFloatingIp(const string& floating_ip) { m_strFloatingIp = floating_ip; }

    void setId(const string& id) { m_strId = id; }
	void setCheckStatus(UINT1 status){ m_checkStatus = status;}
	
	UINT1 getCheckStatus(){ return m_checkStatus ;}

private:
    string m_strRouterId;           ///< 路由ID
    string m_strStatus;             ///< 状态
    string m_strTenantId;           ///< 租户ID
    string m_strFloatingNetId;      ///< 网络ID
    string m_strFixedIp;            ///< 对应的IP
    string m_strFloatingIp;         ///< Floating的IP
    string m_strPortId;             ///< PortID
    string m_strId;                 ///< DeviceID
    UINT1 m_checkStatus;
};


#endif

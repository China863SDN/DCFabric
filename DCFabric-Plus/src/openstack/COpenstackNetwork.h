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
*   File Name   : COpenstackNetwork.h    *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_NETWORK_H
#define _COPENSTACK_NETWORK_H
#include"bnc-type.h"

using namespace std;
class COpenstackNetwork
{
public:
    COpenstackNetwork();
    ~COpenstackNetwork();

	BOOL Compare(COpenstackNetwork* network);
	BOOL SetObjectValue(COpenstackNetwork* network);
    /*
     * set status
     */
    void setStatus(const string& status) { m_strStatus = status; }

    /*
     *set weather network is external accessible
     */
    void setRouterExternal(BOOL router_external) { m_bRouterExternal = router_external; }

    void setName(const string& name) { m_strName = name; }

	/*
	 * set administrative state
	 */
    void setAdminStateUp(BOOL admin_state_up) { m_bAdminStateUp = admin_state_up; }

    void setTenantId( const string& tenant_id) { m_strTenantId = tenant_id; }

    /*
     * set MTU
     */
    void setMtu(const UINT4& mtu) { m_uiMtu = mtu; }

    void setSubnets(const list<string>& subnets) { m_liSubnets = subnets; }

    /*
     * set shared state
     */
    void setShared(BOOL shared) { m_bShared = shared; }

    /*
     * set network UUID
     */
    void setId(const string& id) { m_strId = id; }

    /*
     * set Qos policy UUID
     */
    void setQosPolicyId(const string& qos_policy_id) { m_strQosPolicyId = qos_policy_id; }

    void setProviderNetworkType(const string& provider_network_type)
    {
    	m_strProviderNetworkType = provider_network_type;
    }

    void setProviderPhysicalNetwork(const string& provider_physical_network)
    {
    	m_strProviderPhysicalNetwork = provider_physical_network;
    }

    void setProviderSegmentationId(const UINT4& provider_segmentation_id)
    {
    	m_uiProviderSegmentationId = provider_segmentation_id;
    }

    /*
     * set port security
     */
    void setPortSecurityEnabled(BOOL port_security_enabled)
    {
    	m_bPortSecurityEnabled = port_security_enabled;
    }

    /*
     * get id方法
     *
     * @return:string                               network_id
     */
    const string& getId() { return m_strId; }

    /*
     * 获取状态
     *
     * @return:string                               网络状态
     */
    const string& getStatus() { return m_strStatus; }

    /*
     * 获取RouterExternal
     *
     * @return:BOOL                                 RouterExternal
     */
    const BOOL getRouterExternal() { return m_bRouterExternal; }

    /*
     * 获取网络名称
     *
     * @return:string                               网络名称
     */
    const string& getName() { return m_strName; }

    /*
     * 获取AdminStateUp
     *
     * @return:BOOL                                 AdminStateUp
     */
    const BOOL getAdminStateUp() { return m_bAdminStateUp; }

    /*
     * 获取租户ID
     *
     * @return:string                               租户ID
     */
    const string& getTenantId() { return m_strTenantId; }

    /*
     * 获取Mtu
     *
     * @return:UINT4                               Mtu
     */
    const UINT4& getMtu() { return m_uiMtu; }

    /*
     * 获取子网列表
     *
     * @return:list<string>                         子网列表
     */
    const list<string>& getSubnetsList() { return m_liSubnets; }

    /*
     * 获取Shared属性
     *
     * @return:BOOL                                 Shared属性
     */
    const BOOL getShared() { return m_bShared; }

    /*
     * 获取QosPolicyId
     *
     * @return:string                               QosPolicyId
     */
    const string& getQosPolicyId() { return m_strQosPolicyId; }

    /*
     * 获取ProviderNetworkType
     *
     * @return:string                               ProviderNetworkType
     */
    const string& getProviderNetworkType() { return m_strProviderNetworkType; }

    /*
     * 获取ProviderPhysicalNetwork
     *
     * @return:string                               ProviderPhysicalNetwork
     */
    const string& getProviderPhysicalNetwork() { return m_strProviderPhysicalNetwork; }

    /*
     * 获取ProviderSegmentationId
     *
     * @return:UINT4                               ProviderSegmentationId
     */
    const UINT4& getProviderSegmentationId() { return m_uiProviderSegmentationId; }

    /*
     * 获取PortSecurityEnabled
     *
     * @return:BOOL                                 PortSecurityEnabled
     */
    const BOOL getPortSecurityEnabled() { return m_bPortSecurityEnabled; }

	void setCheckStatus(UINT1 status){ m_checkStatus = status; }
	
	UINT1 getCheckStatus(){ return m_checkStatus; }
private:
    string m_strStatus;
    BOOL m_bRouterExternal;
    string m_strName;
    BOOL m_bAdminStateUp;
    string m_strTenantId;
    UINT4 m_uiMtu;
    list<string> m_liSubnets;
    BOOL m_bShared;
    string m_strId;
    string m_strQosPolicyId;
    string m_strProviderNetworkType;
    string m_strProviderPhysicalNetwork;
	UINT4 m_uiProviderSegmentationId;
	BOOL  m_bPortSecurityEnabled;
	UINT1 m_checkStatus;
};


#endif

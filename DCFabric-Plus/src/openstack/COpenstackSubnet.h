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
*   File Name   : COpenstackSubnet.h    *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_SUBNET_H
#define _COPENSTACK_SUBNET_H
#include"bnc-type.h"

using namespace std;
class COpenstackSubnet
{
public:
    COpenstackSubnet();
    ~COpenstackSubnet();

	BOOL Compare(COpenstackSubnet* subnet);
	BOOL SetObjectValue(COpenstackSubnet* subnet);
    /*
     *set方法
     */
    void setName(const string& name) { m_strName = name; }

    /*
     * 设置DHCP状态
     */
    void setEnableDhcp(BOOL enable_dhcp) { m_bEnableDhcp = enable_dhcp; }

    void setNetworkId(const string& network_id) { m_strNetworkId = network_id;}

    void setTenantId(const string& tenant_id) { m_strTenantId = tenant_id; }

    /*
     * set分配的地址池，包括start，end
     */
    void setAlloctionPools(const map<string, string>& alloction_pools)
    {
    	m_mapAlloctionPools = alloction_pools;
    }

    /*
     * set start,end
     */
    void setStart(const string& start) { m_strStart = start; }

    void setEnd(const string& end) { m_strEnd = end; }

    /*
     * set ip version
     */
    void setIpVersion(const UINT4& ip_version) { m_uiIpVersion = ip_version; }

    void setGatewayIp(const string& gateway_ip) { m_strGatewayIp = gateway_ip; }

    /*
     * set CIDR
     */
    void setCidr(const string& cidr) { m_strCidr = cidr; }

    void setId(const string& id) { m_strId = id; }

    /*
     * get方法
     *
     * @return:string                       子网ID
     */
    const string& getId() { return m_strId; }

    /*
     *
     * 获取network id
     *
     * @return:string                        网络ID
     */
    const string& getNetworkId() { return m_strNetworkId; }

    /*
     * 获取网关IP
     *
     * @return:string                        网关IP
     */
    const string& getGatewayIp() { return m_strGatewayIp; }

    /*
     * 获取子网名
     *
     * @return:string                        子网名
     */
    const string& getName() { return m_strName; }

    /*
     * 获取DHCP状态
     *
     * @return:BOOL                          DHCP状态
     */
    const BOOL getEnableDhcp() { return m_bEnableDhcp; }

    /*
     * 获取租户ID
     *
     * @return:string                        租户ID
     */
    const string& getTenantId() { return m_strTenantId; }

    /*
     * 获取DNS服务器名
     *
     * @return:list<string>                  DNS服务器名
     */
    const list<string> getDnsNameserversList() { return m_liDnsNameservers; }

    /*
     * 获取host routes
     *
     * @return:list<string>                  租户ID
     */
    const list<string> getHostRoutes() { return m_liHostRoutes; }

    /*
     * 获取Cidr
     *
     * @return:string                        Cidr
     */
    const string& getCidr() { return m_strCidr; }

    /*
     * 获取AlloctionPools
     *
     * @return:map<string,string>             AlloctionPools
     */
    const map<string,string> getAlloctionPools() { return m_mapAlloctionPools; }

	 /*
     * get start,end
     */
	 const string & getStart()const {return m_strStart; }
	 const string & getEnd() const {return m_strEnd; }
    /*
     * 获取IP版本
     *
     * @return:UINT4                         IP版本
     */
    const UINT4 getIpVersion() { return m_uiIpVersion; }
	//set check status
	void  setCheckStatus(UINT1 status){m_checkStatus = status;}
	//get check status
	UINT1 getCheckStatus(){return m_checkStatus;}
private:
    string m_strName;
    BOOL m_bEnableDhcp;
    string m_strNetworkId;
    string m_strTenantId;
    list<string> m_liDnsNameservers;
    map<string, string> m_mapAlloctionPools;
    string m_strStart;
    string m_strEnd;
    list<string> m_liHostRoutes;
    UINT4 m_uiIpVersion;
    string m_strGatewayIp;
    string m_strCidr;
    string m_strId;
	UINT1 m_checkStatus;
};


#endif

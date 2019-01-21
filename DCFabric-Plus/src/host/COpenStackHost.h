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
*   File Name   : COpenstackHost.h		*
*   Author      : bnc xflu          	*
*   Create Date : 2016-7-27         	*
*   Version     : 1.0           		*
*   Function    : .           			*
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_HOST_H
#define _COPENSTACK_HOST_H

//#include "CHost.h"
#include "COpenstackDefine.h"

/*
 * Openstack主机类
 */
class COpenStackHost //: public CHost
{
public:
	/*
	 * 默认构造函数
	 */
	COpenStackHost();

	/*
	 * 默认析构函数
	 */
	~COpenStackHost();

	/*
	 * 带参数的构造函数
	 *
     * @param: ptrSw            交换机指针
     * @param: portNo           交换机和主机连接的端口号
     * @param: mac              主机Mac地址
     * @param: ip               主机IP
     * @param: dpid             主机dpid(默认是0)
     * @param: device_type      主机类型(Openstack专有,主机/DHCP服务器/网关..)
     * @param: tenant_id        租户ID(Openstack专有)
     * @param: network_id       网络ID(Openstack专有)
     * @param: subnet_id        子网ID(Openstack专有)
     * @param: port_id          端口ID(Openstack专有)
     *
	 */
	COpenStackHost(/*CSmartPtr<CSwitch> ptrSw,
                    UINT8 dpid,
                    UINT4 portNo,
                    UINT1* mac,
                    UINT4 ip,*/
                    bnc::openstack::device_type type,
                    const std::string & tenant_id,
                    const std::string & network_id,
                    const std::string & subnet_id,
                    const std::string & port_id)
                    : //CHost(ptrSw, dpid, portNo, mac, ip),
                    m_enDeviceType(type),
                    m_strTenantId(tenant_id),
                    m_strNetworkId(network_id),
                    m_strSubnetId(subnet_id),
                    m_strPortId(port_id)
               
	{
		//setHostType(bnc::host::OPENSTACK_HOST);
	}

	/*
	 * 取得租户id
	 *
	 * @return: string          租户ID
	 */
	const std::string& getTenantId() const { return m_strTenantId; }

	/*
	 * 取得网络id
	 *
	 * @return: string          网络ID
	 */
	const std::string& getNetworkId() const { return m_strNetworkId; }

	/*
	 * 取得子网id
	 *
	 * @return: string          子网ID
	 */
	const std::string& getSubnetId() const { return m_strSubnetId; }

	/*
	 * 取得portid
	 *
	 * @return: string          端口ID
	 */
	const std::string& getPortId() const { return m_strPortId; }

	/*
	 * 取得Device Type
	 *
	 * @return: device_type     设备类型(host/dhcp/gateway..)
	 */
	const bnc::openstack::device_type getDeviceType() const { return m_enDeviceType; }


	/*
	 * 判断是不是主机
	 *
	 * @return: BOOL        TRUE:是主机; FALSE:不是主机
	 */
	virtual BOOL isHost() const;

    /*
     * 判断是否为网关
     * 是一个虚函数, 子类可以重载,
     * 对于普通主机而言, 所有的主机都不是网关
     *
     * @rturn: BOOL         TRUE:是网关, FALSE: 不是网关
     */
    virtual BOOL isGateway() const;

	/*
	 * 判断主机是否同一网段
	 *
	 * @return: BOOL        TRUE:是同一网段; FALSE:不是同一网段
	 */
	virtual BOOL isSameSubnet(const  COpenStackHost* host) const;

	/*
     * 获取子网id
     *
     * @return: string		 子网ID
     */
    const std::string & getSubnetId() { return m_strSubnetId; }

    /*
     * 判断是否为代理host（floatingIP/NAT）
     *
     * @return:BOOL          TRUE:是代理IP   FALSE：不是代理IP
     */
    virtual BOOL isProxyHost() const;

private:
	std::string m_strSubnetId;
	std::string m_strTenantId;                      ///< 租户id
	std::string m_strNetworkId;                     ///< 网络id
	//std::string m_strSubnetId;                      ///< 子网id
	std::string m_strPortId;                        ///< portid
	bnc::openstack::device_type m_enDeviceType;     ///< openstack类型
	// security group
};

#endif

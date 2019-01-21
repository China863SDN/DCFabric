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
*   File Name   : COpenstackExternal.h      *
*   Author      : bnc mmzhang               *
*   Create Date : 2016-9-18                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_EXTERNAL_H
#define _COPENSTACK_EXTERNAL_H

#include"bnc-type.h"

/*
 * 用来存储外部网关端口信息
 */
class COpenstackExternal
{
public:
	/*
	 *无参构造析构函数
	 */
	COpenstackExternal();
	~COpenstackExternal();

	/*
	 * 带参构造函数
	 *
     * @param: external_subnet_id                子网ID
     * @param: external_gateway_ip               网关IP
     * @param: external_outer_interface_ip       外部接口IP
     * @param: external_gateway_mac              外部网关Mac
     * @param: external_outer_interface_Mac      外部接口Mac
     * @param: external_dpid                     DPID
     * @param: external_port                     port
     * @param: valid                             状态是否有效
	 */
	COpenstackExternal(std::string external_subnet_id,
		UINT4 external_gateway_ip,
		UINT4 external_outer_interface_ip,
		UINT1 external_gateway_mac[6],
		UINT1 external_outer_interface_Mac[6],
		UINT8 external_dpid,
		UINT4 external_port,
		BOOL valued);
	/*
	 * 获取subnet Id
	 *
	 * @param:void
	 *
	 * @return: string                          网络ID
	 */
	const std::string& getExternalSubnetId() const
	{
		 return m_strExternalSubnetId;
	}
	/*
	 * 获取public net gateway
	 *
	 * @param:void
	 *
	 * @return: UINT4                                                      网关IP
	 */
	UINT4 getExternalGatewayIp() const
	{
		return m_uiExternalGatewayIp;
	}

	const UINT1* getExternalGatewayMac() const
	{
		return m_ucExternalGatewayMac;
	}

	/*
	 * 获取外部接口
	 *
	 * @param:void
	 *
	 * @return: UINT4                                                   外部接口IP
	 */
	UINT4 getExternalOuterInterfaceIp() const
	{
		return m_uiExternalOuterInterfaceIp;
	}

	const UINT1* getExternalOuterInterfaceMac() const
	{
		return m_ucExternalOuterInterfaceMac;
	}

	/*
	 * 获取Dpid
	 *
	 * @param:void
	 *
	 * @return: UINT8                           DPID
	 */
	UINT8 getExternalDpid() const
	{
		return m_ucExternalDpid;
	}

	/*
	 * 获取route port
	 *
	 * @param:void
	 *
	 * @return: UINT4                           port
	 */
	UINT4 getExternalPort() const
	{
		return m_uiExternalPort;
	}

	/*
	 * 获取External value信息
	 *
	 *@param:void
	 *
     *@return: BOOL                            是否有效
	 */
	BOOL getValued() const
	{
		return m_bValid;
	}

	/*
	 * set subnet id
	 *
	 *@param:string                             子网ID
	 *
     *@return: void
	 */
	void setExternalSubnetId(const std::string& subnet_id)
	{
		m_strExternalSubnetId = subnet_id;
	}

	/*
	 * 设置 ExternalGateway相关消息
	 *
	 *@param: UINT4                                                          网关IP
	 *
     *@return: void
	 */
	void setExternalGatewayIp(const UINT4 gateway_ip)
	{
		m_uiExternalGatewayIp = gateway_ip;
	}

	void setExternalGatewayMac(const UINT1* gateway_mac)
	{
		memcpy(m_ucExternalGatewayMac,gateway_mac, 6);
	}

	/*
	 * 设置 ExternalOuterInterfaceIp相关消息
	 * @param: UINT4                                                        外部接口IP
	 * @return: void
	 */
	void setExternalOuterInterfaceIp(const UINT4 outer_interface_ip)
	{
		m_uiExternalOuterInterfaceIp = outer_interface_ip;
	}

	void setExternalOuterInterfaceMac(const UINT1* outer_interface_mac)
	{
		memcpy(m_ucExternalOuterInterfaceMac,outer_interface_mac, 6);
	}

	/*
	 * 设置external dpid
	 *
	 * @param: UINT4                             DPID
	 *
	 * @return: void
	 */
	void setExternalDpid(const UINT8 dpid)
	{
		m_ucExternalDpid = dpid;
	}

	/*
	 * 设置外部网关
	 *
	 * @param: UINT4                             port
	 *
	 * @return: void
	 */
	void setExternalPort(const UINT4 port)
	{
		m_uiExternalPort = port;
	}

	/*
	 * 设置external的当前状态是否有效
	 *
	 * @param: BOOL                              是否有效
	 *
	 * @return: void
	 */
	void setValid(const BOOL valid)
	{
		m_bValid = valid;
	}
	/*
	 * 判断External状态是否为有效状态
	 *
	 * @return: BOOL                            判断是否有效
	 */
	BOOL isValid();


private:
	std::string m_strExternalSubnetId;
	UINT4 m_uiExternalGatewayIp;         ///public net gateway
	UINT4 m_uiExternalOuterInterfaceIp;  ///外部接口
	UINT1 m_ucExternalGatewayMac[6];
	UINT1 m_ucExternalOuterInterfaceMac[6];
	UINT8 m_ucExternalDpid;   ///openflow route dpid
	UINT4 m_uiExternalPort;   ///openflow route port
	BOOL m_bValid;
};

#endif




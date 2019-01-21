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
*   File Name   : CHost.h			*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-27         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#ifndef _CHOST_H
#define _CHOST_H

#include <vector>
#include "CSwitch.h"
#include "bnc-type.h"
#include "CHostDefine.h"
#include "comm-util.h"
#include "CRefObj.h"

/*
 * 主机的基�?
 */
class CHost : public CRefObj
{
public:
	/*
	 * 默认构造函�?
	 */
	CHost();

	/*
	 * 用于ipv4的带参数的构造函�?
	 *
	 * @param: sw               交换机指�?
	 * @param: dpid             主机Dpid
	 * @param: portNo           交换机和主机连接的端�?
	 * @param: mac              主机Mac地址
	 * @param: ip               主机IP地址
	 */
	CHost(CSmartPtr<CSwitch> sw, UINT8 dpid, UINT4 portNo, const UINT1* mac, UINT4 ip);

	/*
	 * 用于ipv4的带参数的构造函�?
	 *
	 * @param: sw               交换机指�?
	 * @param: dpid             主机Dpid
	 * @param: portNo           交换机和主机连接的端�?
	 * @param: mac              主机Mac地址
	 * @param: ip               主机IP地址
	 * @param: subnetid         主机子网id
	 */
	CHost(CSmartPtr<CSwitch> sw, UINT8 dpid, UINT4 portNo, const UINT1* mac, UINT4 ip, bnc::host::host_type  type,const std::string& subnetid,const std::string& tenantid);

	CHost(const UINT1* mac, UINT4 ip, bnc::host::host_type  type,const std::string& subnetid,const std::string& tenantid);

	/*
	 * 默认析构函数
	 */
	virtual ~CHost();

	/*
	 * 获取主机类型
	 *
	 * @return: host_type        主机类型:(普通主�?Openstack主机)
	 */
	bnc::host::host_type getHostType() const { return m_enType; }

	/*
	 * 获取交换机信�?
	 *
	 * @return: CSmartPtr<CSwitch>      交换机的智能指针
	 */
	CSmartPtr<CSwitch> getSw() const { return m_ptrSw; }

	/*
	 * 获取dpid信息
	 *
	 * @return: UINT8       主机的Dpid
	 */
	UINT8 getDpid() const { return m_iDpid; }

	/*
	 * 获取端口�?
	 *
	 * @return: UINT4       主机的端口号
	 */
	UINT4 getPortNo() const { return m_iPortNo; }

	/*
	 * 获取mac地址
	 *
	 * @return: UINT1*      主机的Mac地址的指�?
	 */
	UINT1* getMac()  { return m_oMac; }

	/*
	 * 获取ipv4地址
	 *
	 * @return: INT4        主机的Ip地址(此处只返回了Ip列表中的第一个Ip,如果多Ip,需要修�?
	 */
	UINT4 getIp() const { return m_oIpList[0]; }

	const std::vector<UINT4>& getIpList() const { return m_oIpList; }

	/*
	 * 判断ipv4存在
	 * 判断这个Ip是不是主机的Ip地址之一
	 *
	 * @param: ip           ip地址
	 *
	 * @return: BOOL        TRUE:存在这个IP; FALSE:不存在这个IP
	 */
	BOOL isIpExist(INT4 ip) const;

	/*
	 * 设置主机类型
	 *
	 * @param: host_type        主机类型:(普通主�?Openstack主机)
	 *
	 * @return: None
	 */
	void setHostType(bnc::host::host_type type) { m_enType = type; }

	/*
	 * 设置交换�?
	 *
	 * @param: pSwitch          交换机指�?
	 *
	 * @return: None
	 */
	void setSw(CSmartPtr<CSwitch> pSwitch) { m_ptrSw = pSwitch; }

	/*
	 * 设置dpid信息
	 *
	 * @param: dpid             主机Dpid信息
	 *
	 * @return: None
	 */
	void setDpid(UINT8 dpid) { m_iDpid = dpid; }

	/*
	 * 设置端口�?
	 *
	 * @param: portNo           主机端口�?
	 *
	 * @return: None
	 */
	void setPortNo(UINT4 portNo) { m_iPortNo = portNo; }

	/*
	 * 设置mac地址
	 *
	 * @param: mac              主机Mac地址指针
	 *
	 * @return: None
	 */
	void setMac(UINT1* mac) { memcpy(m_oMac, mac, 6); }


	void setIp(UINT4 ip) { m_oIpList[0] = ip;}
	
	void setIpList(const std::vector<UINT4>& ipList) { m_oIpList = ipList; }

	/*
	 * 设置子网ID
	 *
	 * @param: subnetid 子网id
	 *
	 * @return: None
	 */
	void setSubnetId(const std::string& subnetid){m_strSubnetId = subnetid ;}

	/*
	 * 获取子网ID
	 *
	 * @param: None
	 *
	 * @return: m_subnetid 子网id
	 */
	const std::string& getSubnetId() const { return m_strSubnetId;}


	/*
	 * 设置子网ID
	 *
	 * @param: subnetid 子网id
	 *
	 * @return: None
	 */
	void setTenantId(const std::string& tenantid){m_strTenantId = tenantid ;}

	/*
	 * 获取子网ID
	 *
	 * @param: None
	 *
	 * @return: m_subnetid 子网id
	 */
	const std::string& getTenantId() const{ return m_strTenantId;}

	/*
	 * 判断是否为主�?
	 * 是一个虚函数, 子类可以重载,
	 * 对于普通主机而言, 所有通过连接创建的的都是主机
	 *
	 * @return: BOOL        TRUE:是主�? FALSE:不是主机
	 */
	virtual BOOL isHost() const { return TRUE; }

	/*
	 * 判断是否为网�?
	 * 是一个虚函数, 子类可以重载,
	 * 对于普通主机而言, 所有的主机都不是网�?
	 *
	 * @rturn: BOOL         TRUE:是网�? FALSE: 不是网关
	 */
	virtual BOOL isGateway() const {return (bnc::host::HOST_ROUTER == m_enType); };


	/*
	 * 判断是否为网�?
	 * 是一个虚函数, 子类可以重载,
	 * 对于普通主机而言, 所有的主机都不是dhcp port
	 *
	 * @rturn: BOOL         TRUE:是dhcp port, FALSE: 不是dhcp port
	 */
	virtual BOOL isDhcp() const {return (bnc::host::HOST_DHCP == m_enType); };

	/*
	 * 判断是否同一网段
	 *
	 * @return: BOOL        TRUE:是同一网段; FALSE:不是同一网段
	 */
	virtual BOOL isSameSubnet(const CHost& host) const {return (0 == m_strSubnetId.compare(host.m_strSubnetId));}

    /*
     * 判断是否为代理主机（floatingIP/NAT�?
     *
     * @return:BOOL          TRUE:是代理主�?  FALSE：不是代理主�?
     */
	virtual BOOL isProxyHost() const { return FALSE; }

	virtual void  setfixIp(UINT4 ip) = 0;
	virtual	UINT4 getfixIp()const =0;

private:
	bnc::host::host_type m_enType;	///< 主机类型
	CSmartPtr<CSwitch> m_ptrSw;		///< 交换机指�?
	UINT8 m_iDpid;					///< dpid
	UINT4 m_iPortNo;				///< 端口�?
	UINT1 m_oMac[6];				///< mac地址
	std::string m_strSubnetId;
	std::string m_strTenantId;

protected:
	std::vector<UINT4> m_oIpList;	/// ipv4 list
	std::vector<std::string> m_oIpv6List;	///< ipv6 list
};
#endif

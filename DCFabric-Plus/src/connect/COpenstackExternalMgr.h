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
*   File Name   : COpenstackExternalMgr.h   *
*   Author      : bnc mmzhang               *
*   Create Date : 2016-9-18                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/

#ifndef _COPENSTACK_EXTERNALMGR_H
#define _COPENSTACK_EXTERNALMGR_H

#include "COpenstackExternal.h"
#include "bnc-type.h"
#include "CMutex.h"
class COpenstackExternalMgr
{
public:
	/*
	 * 默认析构函数
	 */
	~COpenstackExternalMgr();

	/*
	 * 获取实例
	 *
	 * @param:void
	 *
	 * @return:COpenstackExternalMgr             静态实例
	 */
	static COpenstackExternalMgr* getInstance();

	/*
	 * 增加OpenstackExternal
	 *
	 * @param: external_subnet_id                子网ID
     * @param: external_gateway_ip               网关IP
     * @param: external_outer_interface_ip       外部接口IP
     * @param: external_gateway_mac              外部网关Mac
     * @param: external_outer_interface_Mac      外部接口Mac
     * @param: external_dpid                     DPID
     * @param: external_port                     port
     *
     * @return：void
	 */
	void addOpenstackExternal( std::string external_subnet_id,
			UINT4 external_gateway_ip,
			UINT4 external_outer_interface_ip,
			UINT1* external_gateway_mac,
			UINT1* external_outer_interface_Mac,
			UINT8 external_dpid,
			UINT4 external_port);

	/*
	 * 删除OpenstackExternal
	 *
	 * @param:UINT4                                                             外部接口IP
	 */
	void removeOpenstackExternalByOuterIp(const UINT4 outerInterfaceIp);

	/*
     * 删除OpenstackExternal
     *
     * @param:string                              子网id
     */
    void removeOpenstackExternalBySubnetId(const std::string subnetid);

	/*
	 * 根据outer interface查找External
	 *
	 * @param:UINT4                                                             外部接口IP
	 *
	 * @return:COpenstackExternal                 external
	 */
	COpenstackExternal* findOpenstackExternalByOuterIp(const UINT4 outerInterfaceIp);

	COpenstackExternal* findOpenstackExternalByOuterMac(const UINT1* outerInterfaceMac);

    COpenstackExternal* findOpenstackExternalBySubnetId(const std::string& subnetid);

    COpenstackExternal* findOpenstackExternalAny();

    /*
     * 更新openstack external
     *
     * @param: external_subnet_id                子网ID
     * @param: external_gateway_ip               网关IP
     *
     * @return：COpenstackExternal               external
     */
    void updateOpenstackExternalBySubnetId(std::string external_subnet_id,UINT4 external_gateway_ip);

    /*
     * 获取 external list
     *
     * @param: external_subnet_id                子网ID
     * @param: external_gateway_ip               网关IP
     *
     * @return：COpenstackExternal               external
     */
    std::list<COpenstackExternal*> getOpenstackExternalList() { return m_listOpenstackExternal; }


    /*
     * 更新openstack external list中的External gateway_mac
     *
     * @param: external_gateway_mac              外部网关Mac
     *
     * @return：void
     */
    void updateOpenstackExternalAll(UINT4 external_gateway_ip,UINT1* external_gateway_mac);

    /*
     * 根据交换机的Dpid判断是不是外联交换机
     */
    BOOL isExternalSwitch(UINT8 dpid);

private:
	/*
	 * 默认构造函数
	 */
	COpenstackExternalMgr();

	/*
	 * 创建openstack external
	 *
	 * @param: external_subnet_id                子ID
     * @param: external_gateway_ip               网关IP
     * @param: external_outer_interface_ip       外部接口IP
     * @param: external_gateway_mac              外部网关Mac
     * @param: external_outer_interface_Mac      外部接口Mac
     * @param: external_dpid                     DPID
     * @param: external_port                     port
     *
     * @return：COpenstackExternal               external
	 */
	COpenstackExternal* createOpenstackExternal(std::string external_subnet_id,
			UINT4 external_gateway_ip,
			UINT4 external_outer_interface_ip,
			UINT1* external_gateway_mac,
			UINT1* external_outer_interface_Mac,
			UINT8 external_dpid,
			UINT4 external_port);

	/*
	 * 更新openstack external
	 *
	 * @param: external_subnet_id                子网ID
     * @param: external_gateway_ip               网关IP
     * @param: external_outer_interface_ip       外部接口IP
     * @param: external_gateway_mac              外部网关Mac
     * @param: external_outer_interface_Mac      外部接口Mac
     * @param: external_dpid                     DPID
     * @param: external_port                     port
     *
     * @return：COpenstackExternal               external
	 */
	COpenstackExternal* updateOpenstackExternal(COpenstackExternal* openstackExternal,
			  	  	  	std::string external_subnet_id,
						UINT4 external_gateway_ip,
						UINT1 external_gateway_mac[6],
						UINT1 external_outer_interface_Mac[6],
						UINT8 external_dpid,
						UINT4 external_port);

	/*
	 * 初始化
	 *
	 * @param:void
	 *
	 * @return:void
	 */
	void init();

	/*
	 * 从配置文件读取Openstack External
	 *
	 * @param:void
	 *
	 * @return:void
	 */
	void loadExternalConfig();

    /*
     * 从配置文件读取Openstack External
     *
     * @param:void
     *
     * @return:void
     */
//    void loadExternalResource();

    /*
     * 定时询问external的mac
     *
     */
    static void* requestExternalMacPeriodically(void* param);

    /*
     * 定时询问external的mac
     */
    void* requestExternalListPeriodically(void* param);



	static COpenstackExternalMgr* m_pInstance; ///静态实例成员
	std::list<COpenstackExternal*> m_listOpenstackExternal;
    CMutex m_oMutex;
    pthread_t m_externalThreadId;             ///external mac request的线程
};




#endif

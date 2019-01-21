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
*   File Name   : CNotifyHandler.h          *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFY_HANDLER_H
#define _CNOTIFY_HANDLER_H

#include "CNotificationCenter.h"

class CNotificationCenter;

/*
 *  处理内部通知的基类
 *  定义了所有的虚函数
 */
class CNotifyHandler
{
public:
    /*
     * 带参数的默认构造函数
     *
     * @param: center       通知处理中心类
     * @param: type         通知类型
     *
     */
    CNotifyHandler(CNotificationCenter* center,
                   bnc::notify::type type) :
                   m_pNotificationCenter(center),
                   m_enType(type)
                   { }


    /*
     * 默认构造函数
     */
    CNotifyHandler() : m_pNotificationCenter(NULL),
                       m_enType(bnc::notify::UNKNOWN)
                       { }

    /*
     * 默认析构函数
     */
    virtual ~CNotifyHandler() { }

    /*
     * 获取当前通知处理者的类型
     *
     * @return: type            当前的通知类型
     */
    virtual bnc::notify::type getType() { return m_enType; }

    /*
     * 处理增加OpenstackPort
     *
     * @param: port             OpenstackPort指针
     *
     * @return: None
     */
    virtual void notifyAddPort(COpenstackPort* port) {}

	 /*
     * 处理增加Base_Port
     *
     * @param: port             Base_Port指针
     *
     * @return: None
     */
    virtual void notifyAddPort(Base_Port* port) {}

    /*
     * 处理删除OpenstackPort
     *
     * @param: port_id          OpenstackPort的PortId
     *
     * @return: None
     */
    virtual void notifyDelPort(const std::string & port_id) {}

	 /*
     * 处理update OpenstackPort
     *
     * @param:  port             OpenstackPort指针
     *
     * @return: None
     */
    virtual void notifyUpdatePort(COpenstackPort* port) {}

	/*
     * 处理update Base_Port
     *
     * @param:  port             Base_Port指针
     *
     * @return: None
     */
    virtual void notifyUpdatePort(Base_Port* port) {}

	/*
     * 处理增加OpenstackNetwork
     *
     * @param: network             OpenstackNetwork指针
     *
     * @return: None
     */
    virtual void notifyAddNetwork(COpenstackNetwork* network) {}

	/*
     * 处理增加Base_Network
     *
     * @param: network             Base_Network指针
     *
     * @return: None
     */
    virtual void notifyAddNetwork(Base_Network* network) {}

	/*
     * 处理删除OpenstackNetwork
     *
     * @param: network            network id
     *
     * @return: None
     */
    virtual void notifyDelNetwork(const std::string & network_id) {}

	/*
     * 处理update OpenstackNetwork
     *
     * @param: network             OpenstackNetwork指针
     *
     * @return: None
     */
    virtual void notifyUpdateNetwork(COpenstackNetwork* network) {}

	/*
     * 处理update Base_Network
     *
     * @param: network             Base_Network指针
     *
     * @return: None
     */
    virtual void notifyUpdateNetwork(Base_Network* network) {}
	
    /*
     * 处理删除OpenstackSubnet
     *
     * @param: subent             OpenstackSubent指针
     *
     * @return: None
     */
    virtual void notifyAddSubnet(COpenstackSubnet* subnet) {}

	/*
     * 处理删除Base_Subnet
     *
     * @param: subent             Base_Subnet指针
     *
     * @return: None
     */
    virtual void notifyAddSubnet(Base_Subnet* subnet) {}
    /*
     * 处理删除OpenstackSubnet
     *
     * @param: subnet_id          OpenstackSubnet的SubnetId
     *
     * @return: None
     */
    virtual void notifyDelSubnet(const std::string & subnet_id) {}

	  /*
     * 处理update OpenstackSubnet
     *
     * @param: subnet             OpenstackSubnet指针
     *
     * @return: None
     */
    virtual void notifyUpdateSubnet(COpenstackSubnet* subnet) {}

	 /*
     * 处理update Base_Subnet
     *
     * @param: subnet             Base_Subnet指针
     *
     * @return: None
     */
    virtual void notifyUpdateSubnet(Base_Subnet* subnet) {}

	/*
     * 处理增加Floatingip
     *
     * @param: Floatingip             Floatingip指针
     *
     * @return: None
     */
    virtual void notifyAddFloatingIp(COpenstackFloatingip* floatingip) {}

	/*
     * 处理增加Base_Floating
     *
     * @param: Floatingip             Base_Floating指针
     *
     * @return: None
     */
    virtual void notifyAddFloatingIp(Base_Floating* floatingip) {}

    /*
     * 处理删除Floatingip
     *
     * @param: Floatingip id          Floatingip的Floatingip id
     *
     * @return: None
     */
    virtual void notifyDelFloatingIp(const std::string & floatingip_id) {}


	
	/*
     * 处理update Floatingip
     *
     * @param: Floatingip             Floatingip指针
     *
     * @return: None
     */
    virtual void notifyUpdateFloatingIp(COpenstackFloatingip* floatingip) {}

	
	/*
		 * 处理update Floatingip
		 *
		 * @param: Floatingip			  Floatingip指针
		 *
		 * @return: None
		 */
	virtual void notifyUpdateFloatingIp(Base_Floating* floatingip) {}
    /*
     * 处理增加path
     *
     * @param: src_sw           路径的起始交换机
     * @param: dst_sw           路径的终点交换机
     * @param: port_no          交换机连接的端口号
     *
     * @return: None
     */
    virtual void notifyAddPath(const CSmartPtr<CSwitch> & src_sw,
                               const CSmartPtr<CSwitch> & dst_sw,
                               UINT4 port_no) {}

protected:
    /*
     * 设置管理内部通知的中介类
     *
     * @param: center           通知的中心类
     *
     * @return: None
     */
    virtual void setMedia(CNotificationCenter* center)
                         { m_pNotificationCenter = center; }

    CNotificationCenter*     m_pNotificationCenter;     ///< 存储通知中心类指针

    bnc::notify::type m_enType;                         ///< 存储当前的通知类型
};


#endif

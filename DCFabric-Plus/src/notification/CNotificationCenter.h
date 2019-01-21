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
*   File Name   : CNotificationCenter.h     *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFICATION_CENTER_H
#define _CNOTIFICATION_CENTER_H

#include <list>
#include <string>
#include "bnc-type.h"
#include "CNotificationDefine.h"
#include "COpenstackPort.h"
#include "CSmartPtr.h"
#include "CSwitch.h"
#include "COpenstackNetwork.h"
#include "COpenstackSubnet.h"
#include "COpenstackFloatingip.h"

#include "BasePort.h"
#include "BaseNetwork.h"
#include "BaseSubnet.h"
#include "BaseFloating.h"
#include "BaseFloatingManager.h"


class CNotifyHandler;

/*
 * 管理所有内部通知的中介类
 */
using namespace std;

class CNotificationCenter
{
public:
    /*
     * 默认构造函数
     */
    CNotificationCenter();

    /*
     * 默认析构函数
     */
    virtual ~CNotificationCenter();

    /*
     * 增加通知生产者 - 处理者
     *
     * @param: res          事件源
     * @param: handler      事件处理者
     *
     * @return: None
     */
    virtual void addNotifyHandler(CNotifyHandler* res, CNotifyHandler* handler);

    /*
     * 删除通知生产者 - 处理者
     *
     * @param: res          事件源
     * @param: handler      事件处理者
     *
     * @return: None
     */
    virtual void delNotifyHandler(CNotifyHandler* res, CNotifyHandler* handler);

    /*
     * 通知增加OpenstackPort
     *
     * @param: handler          事件源
     * @param: port             新增的CopenstackPort信息
     *
     * @return: None
     */
    virtual void notifyAddPortHandler(CNotifyHandler* handler, COpenstackPort* port);


	/*
     * 通知增加Base_Port
     *
     * @param: handler          事件源
     * @param: port             新增的Base_Port信息
     *
     * @return: None
     */
    virtual void notifyAddPortHandler(CNotifyHandler* handler, Base_Port* port);

    /*
     * 通知删除OpenstackPort/Base_Port
     *
     * @param: handler          事件源
     * @param: port_id          删除的OpenstackPortId
     *
     * @return: None
     */
    virtual void notifyDeletePortHandler(CNotifyHandler* handler, const string & port_id);

    /*
     * 通知update OpenstackPort
     *
     * @param: handler          事件源
     * @param: port             新增的CopenstackPort信息
     *
     * @return: None
     */
    virtual void notifyUpdatePortHandler(CNotifyHandler* handler, COpenstackPort* port);

	 /*
     * 通知update Base_Port
     *
     * @param: handler          事件源
     * @param: port             新增的Base_Port信息
     *
     * @return: None
     */
    virtual void notifyUpdatePortHandler(CNotifyHandler* handler, Base_Port* port);

    /*
     * 通知增加path
     *
     * @param: handler          事件源
     * @param: src_sw           路径的起始交换机
     * @param: dst_sw           路径的终点交换机
     * @param: port_no          经过的端口号
     */
    virtual void notifyAddPathHandler(CNotifyHandler* handler,
                                      const CSmartPtr<CSwitch> & src_sw,
                                      const CSmartPtr<CSwitch> & dst_sw,
                                      UINT4 port_no);

	
    /*
     * 通知增加OpenstackNetwork
     *
     * @param: handler            事件源
     * @param: network             新增的OpenstackNetwork信息
     *
     * @return: None
     */
    virtual void notifyAddNetworkHandler(CNotifyHandler* handler, COpenstackNetwork* network);

	 /*
     * 通知增加Base_Network
     *
     * @param: handler            事件源
     * @param: network             新增的Base_Network信息
     *
     * @return: None
     */
    virtual void notifyAddNetworkHandler(CNotifyHandler* handler, Base_Network* network);
	 
    /*
     * 通知删除COpenstackNetwork/Base_Network
     *
     * @param: handler            事件源
     * @param: network id          删除的OpenstackNetwork id
     *
     * @return: None
     */
    virtual void notifyDeleteNetworkHandler(CNotifyHandler* handler, const string & network_id);

	
    /*
     * 通知增加OpenstackNetwork
     *
     * @param: handler            事件源
     * @param: network             新增的OpenstackNetwork信息
     *
     * @return: None
     */
    virtual void notifyUpdateNetworkHandler(CNotifyHandler* handler, COpenstackNetwork* network);

	
	/*
	   * 通知增加Base_Network
	   *
	   * @param: handler			事件源
	   * @param: network			 新增的Base_NetworkBase_Network信息
	   *
	   * @return: None
	   */
	virtual void notifyUpdateNetworkHandler(CNotifyHandler* handler, Base_Network* network);
	/*
     * 通知增加OpenstackSubnet
     *
     * @param: handler            事件源
     * @param: subnet             新增的CopenstackSubnet信息
     *
     * @return: None
     */
    virtual void notifyAddSubnetHandler(CNotifyHandler* handler, COpenstackSubnet* subent);

	/*
     * 通知增加Base_Subnet
     *
     * @param: handler            事件源
     * @param: subnet             新增的Base_Subnet信息
     *
     * @return: None
     */
    virtual void notifyAddSubnetHandler(CNotifyHandler* handler, Base_Subnet* subent);

    /*
     * 通知删除OpenstackSubnet/Base_Subnet
     *
     * @param: handler            事件源
     * @param: subnet_id          删除的OpenstackSubnetId
     *
     * @return: None
     */
    virtual void notifyDeleteSubnetHandler(CNotifyHandler* handler, const string & subent_id);


	
	/*
     * 通知update OpenstackSubnet
     *
     * @param: handler            事件源
     * @param: subnet             新增的CopenstackSubnet信息
     *
     * @return: None
     */
    virtual void notifyUpdateSubnetHandler(CNotifyHandler* handler, COpenstackSubnet* subent);

	/*
     * 通知update Base_Subnet
     *
     * @param: handler            事件源
     * @param: subnet             新增的Base_Subnet信息
     *
     * @return: None
     */
    virtual void notifyUpdateSubnetHandler(CNotifyHandler* handler, Base_Subnet* subent);
	
	/*
     * 通知增加floatingip
     *
     * @param: handler            事件源
     * @param: floatingip             新增的CopenstackFloatingIp信息
     *
     * @return: None
     */
    virtual void notifyAddFloatingIpHandler(CNotifyHandler* handler, COpenstackFloatingip* floatingip);

	
	/*
		 * 通知增加Base_Floating
		 *
		 * @param: handler			  事件源
		 * @param: floatingip			  新增的Base_Floating信息
		 *
		 * @return: None
		 */
	virtual void notifyAddFloatingIpHandler(CNotifyHandler* handler, Base_Floating* floatingip);
	
	/*
     * 通知删除floatingip/Base_Floating
     *
     * @param: handler            事件源
     * @param: floatingip id          删除的floatingip id
     *
     * @return: None
     */
    virtual void notifyDelFloatingIpHandler(CNotifyHandler* handler, const string & floatingip_id);

	
	/*
     * 通知增加floatingip
     *
     * @param: handler            事件源
     * @param: floatingip             新增的CopenstackFloatingIp信息
     *
     * @return: None
     */
    virtual void notifyUpdateFloatingIpHandler(CNotifyHandler* handler, COpenstackFloatingip* floatingip);

	/*
     * 通知增加Base_Floating
     *
     * @param: handler            事件源
     * @param: floatingip             新增的Base_Floating信息
     *
     * @return: None
     */
    virtual void notifyUpdateFloatingIpHandler(CNotifyHandler* handler, Base_Floating* floatingip);
private:
    std::list<std::pair<CNotifyHandler*, CNotifyHandler*> > m_pNotifyList;
};


#endif

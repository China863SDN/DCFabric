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
*   File Name   : CNotifyOpenstack.h    *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-13                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFY_OPENSTACK_H
#define _CNOTIFY_OPENSTACK_H

#include "CNotifyHandler.h"
/*
 * OpenstackPort通知的处理类
 */
class CNotifyOpenstack : public CNotifyHandler
{
public:
    /*
     * 带参数的默认构造函数
     *
     * @param: center           事件处理中心类
     *
     */
    CNotifyOpenstack(CNotificationCenter* center) :
                         CNotifyHandler(center, bnc::notify::OPENSTACK) { }

    /*
     * 处理增加OpenstackPort
     *
     * @param: port             OpenstackPort指针
     *
     * @return: None
     */
    virtual void notifyAddPort(COpenstackPort* port);

    /*
     * 处理删除OpenstackPort
     *
     * @param: port_id          OpenstackPort的port_id
     */
    virtual void notifyDelPort(const std::string & port_id);

	/*
     * 处理update OpenstackPort
     *
     * @param: port             OpenstackPort指针
     *
     * @return: None
     */
    virtual void notifyUpdatePort(COpenstackPort* port);

	 /*
    * 处理增加OpenstackNetwork
    *
    * @param: Network             OpenstackNetwork指针
    *
    * @return: None
    */
   virtual void notifyAddNetwork(COpenstackNetwork* network);

   /*
    * 处理删除OpenstackNetwork
    *
    * @param: network_id          OpenstackNetwork的network_id
    */
   virtual void notifyDelNetwork(const std::string & network_id);


    /*
    * 处理update OpenstackNetwork
    *
    * @param: Network             OpenstackNetwork指针
    *
    * @return: None
    */
   virtual void notifyUpdateNetwork(COpenstackNetwork* network);
	
    /*
    * 处理增加OpenstackSubnet
    *
    * @param: subnet             OpenstackSubnet指针
    *
    * @return: None
    */
   virtual void notifyAddSubnet(COpenstackSubnet* subnet);

   /*
    * 处理删除OpenstackSubnet
    *
    * @param: subnet_id          OpenstackSubent的subent_id
    */
   virtual void notifyDelSubent(const std::string & subnet_id);

     /*
    * 处理update OpenstackSubnet
    *
    * @param: subnet             OpenstackSubnet指针
    *
    * @return: None
    */
   virtual void notifyUpdateSubnet(COpenstackSubnet* subnet);

	 /*
    * 处理增加OpenstackFloatingIp
    *
    * @param: floatingip             OpenstackFloatingIp指针
    *
    * @return: None
    */
   virtual void notifyAddFloatingIp(COpenstackFloatingip* floatingip);

	 /*
    * 处理删除OpenstackFloatingIp
    *
    * @param: floatingip             OpenstackFloatingIp id
    *
    * @return: None
    */
    virtual void notifyDelFloatingIp(const std::string & floatingip_id);

	 /*
    * 处理update OpenstackFloatingIp
    *
    * @param: floatingip             OpenstackFloatingIp指针
    *
    * @return: None
    */
   virtual void notifyUpdateFloatingIp(COpenstackFloatingip* floatingip);

private:
    /*
    * 默认构造函数
    */
   CNotifyOpenstack();

   /*
    * 默认析构函数
    */
   ~CNotifyOpenstack();

};

#endif

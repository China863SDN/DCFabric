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
*   File Name   : CNotifyBaseNetworking.h    *
*   Author      : bnc cyyang                  *
*   Create Date : 2018-1-31               *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFY_BASENETWORKING_H
#define _CNOTIFY_BASENETWORKING_H

#include "CNotifyHandler.h"
/*
 * OpenstackPort通知的处理类
 */
class CNotifyBaseNetworking: public CNotifyHandler
{
public:
    /*
     * 带参数的默认构造函数
     *
     * @param: center           事件处理中心类
     *
     */
    CNotifyBaseNetworking(CNotificationCenter* center) :
                         CNotifyHandler(center, bnc::notify::BASENETWORKING) { }

    /*
     * 处理增加Base_Port
     *
     * @param: port             Base_Port指针
     *
     * @return: None
     */
    virtual void notifyAddPort(Base_Port* port);

    /*
     * 处理删除Base_Port
     *
     * @param: port_id          Base_Port的port_id
     */
    virtual void notifyDelPort(const std::string & port_id);

	/*
     * 处理update Base_Port
     *
     * @param: port             Base_Port指针
     *
     * @return: None
     */
    virtual void notifyUpdatePort(Base_Port* port);

	 /*
    * 处理增加Base_Network
    *
    * @param: Network             Base_Network指针
    *
    * @return: None
    */
   virtual void notifyAddNetwork(Base_Network* network);

   /*
    * 处理删除Base_Network
    *
    * @param: network_id          Base_Network的network_id
    */
   virtual void notifyDelNetwork(const std::string & network_id);


    /*
    * 处理update Base_Network
    *
    * @param: Network             Base_Network指针
    *
    * @return: None
    */
   virtual void notifyUpdateNetwork(Base_Network* network);
	
    /*
    * 处理增加Base_Subnet
    *
    * @param: subnet             Base_Subnet指针
    *
    * @return: None
    */
   virtual void notifyAddSubnet(Base_Subnet* subnet);

   /*
    * 处理删除Base_Subnet
    *
    * @param: subnet_id          Base_Subnet的subent_id
    */
   virtual void notifyDelSubent(const std::string & subnet_id);

     /*
    * 处理update Base_Subnet
    *
    * @param: subnet             Base_Subnet指针
    *
    * @return: None
    */
   virtual void notifyUpdateSubnet(Base_Subnet* subnet);

	 /*
    * 处理增加Base_Floating
    *
    * @param: floatingip             Base_Floating指针
    *
    * @return: None
    */
   virtual void notifyAddFloatingIp(Base_Floating* floatingip);

	 /*
    * 处理删除Base_Floating
    *
    * @param: floatingip             Base_Floating id
    *
    * @return: None
    */
    virtual void notifyDelFloatingIp(const std::string & floatingip_id);

	 /*
    * 处理update Base_Floating
    *
    * @param: floatingip             Base_Floating指针
    *
    * @return: None
    */
   virtual void notifyUpdateFloatingIp(Base_Floating* floatingip);

private:
    /*
    * 默认构造函数
    */
   CNotifyBaseNetworking();

   /*
    * 默认析构函数
    */
   ~CNotifyBaseNetworking();

};

#endif


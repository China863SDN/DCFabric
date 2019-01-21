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
 * OpenstackPort֪ͨ�Ĵ�����
 */
class CNotifyBaseNetworking: public CNotifyHandler
{
public:
    /*
     * ��������Ĭ�Ϲ��캯��
     *
     * @param: center           �¼�����������
     *
     */
    CNotifyBaseNetworking(CNotificationCenter* center) :
                         CNotifyHandler(center, bnc::notify::BASENETWORKING) { }

    /*
     * ��������Base_Port
     *
     * @param: port             Base_Portָ��
     *
     * @return: None
     */
    virtual void notifyAddPort(Base_Port* port);

    /*
     * ����ɾ��Base_Port
     *
     * @param: port_id          Base_Port��port_id
     */
    virtual void notifyDelPort(const std::string & port_id);

	/*
     * ����update Base_Port
     *
     * @param: port             Base_Portָ��
     *
     * @return: None
     */
    virtual void notifyUpdatePort(Base_Port* port);

	 /*
    * ��������Base_Network
    *
    * @param: Network             Base_Networkָ��
    *
    * @return: None
    */
   virtual void notifyAddNetwork(Base_Network* network);

   /*
    * ����ɾ��Base_Network
    *
    * @param: network_id          Base_Network��network_id
    */
   virtual void notifyDelNetwork(const std::string & network_id);


    /*
    * ����update Base_Network
    *
    * @param: Network             Base_Networkָ��
    *
    * @return: None
    */
   virtual void notifyUpdateNetwork(Base_Network* network);
	
    /*
    * ��������Base_Subnet
    *
    * @param: subnet             Base_Subnetָ��
    *
    * @return: None
    */
   virtual void notifyAddSubnet(Base_Subnet* subnet);

   /*
    * ����ɾ��Base_Subnet
    *
    * @param: subnet_id          Base_Subnet��subent_id
    */
   virtual void notifyDelSubent(const std::string & subnet_id);

     /*
    * ����update Base_Subnet
    *
    * @param: subnet             Base_Subnetָ��
    *
    * @return: None
    */
   virtual void notifyUpdateSubnet(Base_Subnet* subnet);

	 /*
    * ��������Base_Floating
    *
    * @param: floatingip             Base_Floatingָ��
    *
    * @return: None
    */
   virtual void notifyAddFloatingIp(Base_Floating* floatingip);

	 /*
    * ����ɾ��Base_Floating
    *
    * @param: floatingip             Base_Floating id
    *
    * @return: None
    */
    virtual void notifyDelFloatingIp(const std::string & floatingip_id);

	 /*
    * ����update Base_Floating
    *
    * @param: floatingip             Base_Floatingָ��
    *
    * @return: None
    */
   virtual void notifyUpdateFloatingIp(Base_Floating* floatingip);

private:
    /*
    * Ĭ�Ϲ��캯��
    */
   CNotifyBaseNetworking();

   /*
    * Ĭ����������
    */
   ~CNotifyBaseNetworking();

};

#endif


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
*   File Name   : CNotifyExternal.h         *
*   Author      : bnc mmzhang               *
*   Create Date : 2016-9-26                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/

#ifndef _CNOTIFY_EXTERNAL_H_
#define _CNOTIFY_EXTERNAL_H_

#include "CNotifyHandler.h"
#include "comm-util.h"

/*
 * OpenstackExternal通知的处理类
 */
class CNotifyExternal : public CNotifyHandler
{
public:
    /*
     * 带参的默认构造函数
     */
    CNotifyExternal(CNotificationCenter* center):
                                    CNotifyHandler(center, bnc::notify::EXTERNAL){}
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

private:

    /*
     * 默认构造函数
     */
    CNotifyExternal();
    /*
     * 默认构造函数
     */
    ~CNotifyExternal();

};



#endif

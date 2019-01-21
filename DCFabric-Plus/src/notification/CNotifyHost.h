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
*   File Name   : CNotifyHost.h             *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFY_HOST_H
#define _CNOTIFY_HOST_H

#include "CNotifyHandler.h"

/*
 * 主机相关通知处理类
 */
class CNotifyHost : public CNotifyHandler
{
public:
    /*
     * 带参数的默认构造函数
     *
     * @param: center           通知中心类
     */
    CNotifyHost(CNotificationCenter* center)
                : CNotifyHandler(center, bnc::notify::HOST) { }

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
     * @param: port_id          OpenstackPort的PortId
     *
     * @return: None
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
     * 处理增加Base_Port
     *
     * @param: port             Base_Port指针
     *
     * @return: None
     */
	virtual void notifyAddPort(Base_Port* port);
	/*
     * 处理update Base_Port
     *
     * @param: port             Base_Port指针
     *
     * @return: None
     */
	virtual void notifyUpdatePort(Base_Port* port);
	
private:
    /*
     * 默认构造函数
     */
    CNotifyHost();

    /*
     * 默认析构函数
     */
    ~CNotifyHost();
};

#endif

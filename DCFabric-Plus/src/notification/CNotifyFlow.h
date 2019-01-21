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
*   File Name   : CNotifyFlow.h             *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFY_FLOW_H
#define _CNOTIFY_FLOW_H

#include "CNotifyHandler.h"
#include "CSwitch.h"
#include "CSmartPtr.h"

/*
 * 主机相关通知处理类
 */
class CNotifyFlow : public CNotifyHandler
{
public:
    /*
     * 带参数的默认构造函数
     *
     * @param: center       通知处理中心类
     */
    CNotifyFlow(CNotificationCenter* center)
                : CNotifyHandler(center, bnc::notify::FLOW) { }

    /*
     * 处理增加路径
     *
     * @param: src_sw           路径起始交换机
     * @param: dst_sw           路径终点交换机
     * @param: port_no          路径连接的端口
     *
     * @return: None
     */
    virtual void notifyAddPath(const CSmartPtr<CSwitch> & src_sw,
                               const CSmartPtr<CSwitch> & dst_sw,
                               UINT4 port_no);

private:
    /*
     * 默认构造函数
     */
    CNotifyFlow();

    /*
     * 默认析构函数
     */
    ~CNotifyFlow();
};

#endif

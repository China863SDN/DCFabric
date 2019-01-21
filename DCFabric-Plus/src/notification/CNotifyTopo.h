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
*   File Name   : CNotifyTopo.h             *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFY_TOPO_H
#define _CNOTIFY_TOPO_H
#include "CNotifyHandler.h"

/*
 * 拓扑通知类
 */
class CNotifyTopo : public CNotifyHandler
{
public:
    /*
     * 带参数的默认构造函数
     *
     * @param: center       通知处理中心类
     */
    CNotifyTopo(CNotificationCenter* center)
                : CNotifyHandler(center, bnc::notify::TOPO) { }

    /*
     * 处理增加路径
     *
     * @param: src_sw           初始化交换机
     * @param: dst_sw           目的交换机
     * @param: port_no          交换机连接的端口号
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
    CNotifyTopo();

    /*
     * 默认析构函数
     */
    ~CNotifyTopo();
};



#endif



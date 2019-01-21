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
*   File Name   : CNotificationDefine.h     *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFICATION_DEFINE_H
#define _CNOTIFICATION_DEFINE_H

namespace bnc
{
    namespace notify
    {
        /*
         * 定义了所有所涉及的内部通知类型
         */
        enum type
        {
            UNKNOWN = 0,
            OPENSTACK,
            BASENETWORKING,
            HOST,
            PATH,
            FLOW,
            TOPO,
            EXTERNAL,

			NETWORK,
			SUBNET,
			FLOATINGIP,
        };
    }
}

/*
 * 定义了Notify相关的一些参数的转换和使用
 */
class CNofityDefine
{
public:
    /*
     * 默认构造函数
     */
    CNofityDefine();

    /*
     * 默认析构函数
     */
    ~CNofityDefine();
};





#endif

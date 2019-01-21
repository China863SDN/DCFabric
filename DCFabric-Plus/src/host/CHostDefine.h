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
*   File Name   : CHostDefine.h  	*
*   Author      : bnc xflu          *
*   Create Date : 2016-8-1          *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#ifndef _CHOSTDEFINE_H
#define _CHOSTDEFINE_H
/*
 * 定义了一些Host中所使用
 */

namespace bnc
{
	namespace host
	{
		enum host_type
		{
			HOST_UNKNOWN = 0,
			HOST_NORMAL,
			HOST_ROUTER,
			HOST_GATEWAY,
			HOST_DHCP,
			HOST_LOADBALANCE,
			HOST_FLOATINGIP,
			HOST_CLBHA,
			HOST_CLBVIP,
			HOST_EXTERNAL,
			OPENSTACK_HOST,
			//...
			HOST_MAX
		};
	}
}


#endif

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
*   File Name   : L3ServiceDefine.h  	*
*   Author      : bnc cyyang          *
*   Create Date : 2017-12-21          *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#ifndef _L3SERVICEDEFINE_H_
#define _L3SERVICEDEFINE_H_
/*
 * 定义了一些Host中所使用
 */

namespace bnc
{
	namespace l3service
	{
		enum service_type
		{
			SERVICE_UNKNOWN = 0,
			SERVICE_INSIDECOMM,
			SERVICE_FLOATING,
            SERVICE_PORTFORWARD,
			SERVICE_NATICMPCOMM,
			SERVICE_NATCOMM,
			SERVICE_DHCP,
			SERVICE_DSTUNKNOWN,
			SERVICE_FIREWALL,

            //to be added above
            SERVICE_MAX
		};
	}
}


#endif


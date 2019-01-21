
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
*   File Name   : CHostExternal.h			*
*   Author      : bnc cyyang          *
*   Create Date : 2017-12-26         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/

#ifndef _CHOSTEXTERNAL_H_
#define _CHOSTEXTERNAL_H_

#include "CHost.h"
class CHostExternal: public CHost
{
	public:
		CHostExternal() {}
		~CHostExternal() {}

		CHostExternal(CSmartPtr<CSwitch> ptrSw, UINT8 dpid, UINT4 portNo, UINT1* mac, UINT4 ip):
		    CHost(ptrSw, dpid, portNo, mac, ip) {}
		
		CHostExternal(CSmartPtr<CSwitch> ptrSw, UINT8 dpid, UINT4 portNo, UINT1* mac, UINT4 ip, 
		    bnc::host::host_type  type, const std::string& subnetid, const std::string& tenantid):
			CHost(ptrSw, dpid, portNo, mac, ip, type, subnetid, tenantid) {}

        CHostExternal(UINT1* mac, UINT4 ip, bnc::host::host_type  type, const std::string& subnetid, const std::string& tenantid):
            CHost(mac, ip, type, subnetid, tenantid) {}
};

#endif


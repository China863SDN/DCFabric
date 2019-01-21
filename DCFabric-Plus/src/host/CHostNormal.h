
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
*   File Name   : CHostNormal.h			*
*   Author      : bnc cyyang          *
*   Create Date : 2017-12-26         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/

#ifndef _CHOSTNORMAL_H_
#define _CHOSTNORMAL_H_

#include "CHost.h"
class CHostNormal: public CHost
{
	public:
		CHostNormal(){};
		~CHostNormal(){};

		CHostNormal(CSmartPtr<CSwitch> sw, UINT8 dpid, UINT4 portNo, const UINT1* mac, UINT4 ip):
            CHost(sw, dpid, portNo, mac, ip) {}
		
		CHostNormal(CSmartPtr<CSwitch> sw, UINT8 dpid, UINT4 portNo, const UINT1* mac, UINT4 ip, 
		    bnc::host::host_type type, const std::string& subnetid, const std::string& tenantid):
			CHost(sw, dpid, portNo, mac, ip, type, subnetid, tenantid) {}

        CHostNormal(const UINT1* mac, UINT4 ip, bnc::host::host_type type, const std::string& subnetid, const std::string& tenantid):
            CHost(mac, ip, type, subnetid, tenantid) {}
        
	public:
		void  setfixIp(UINT4 ip) {m_oIpList[0] = ip;}
		UINT4 getfixIp() const {return m_oIpList[0];}
};

#endif


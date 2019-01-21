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
*   File Name   : CServiceMgr.h           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef  _CSERVICEMGR_H_
#define  _CSERVICEMGR_H_

#include "L3Service.h"
#include "L3ServiceDefine.h"

typedef std::list<L3Service*>  								CServiceList;
typedef std::map<bnc::l3service::service_type, L3Service*>  CServiceMap;

class  CServiceMgr
{
	public:
		~CServiceMgr();
		INT4 AddService(bnc::l3service::service_type servicetype, L3Service* service );
		INT4 DelService(bnc::l3service::service_type servicetype);
		INT4 ArpProcess(CMsg* msg);
		INT4 IpProcess(CMsg* msg);
		static CServiceMgr* getInstance();

	private:
		CServiceMgr();
		void init();
		void printArp(arp_t* pkt);
		void printPacketInfo(ip_t* pkt);
		bnc::l3service::service_type getServiceType(ip_t* ipPktInfo);

	private:
		static CServiceMgr* m_pInstance;

        CServiceList m_L3ServiceList;
		CServiceMap  m_L3ServiceMap;
};


#endif

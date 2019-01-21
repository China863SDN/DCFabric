
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
*   File Name   : L3Service.h           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef _L3SERVICE_H_
#define  _L3SERVICE_H_

#include "bnc-type.h"
#include "bnc-inet.h"
#include "CHost.h"
#include "L3ServiceDefine.h"
#include "Service_ExternalDetecting.h"

class L3Service
{
	public:
		L3Service();
		L3Service(bnc::l3service::service_type type);
		virtual ~L3Service();

	public:
		 bnc::l3service::service_type GetServiceType() const {return m_type;};
		 INT4 ArpRequestHandler(const CSmartPtr<CSwitch>& srcSw, 
                                      CSmartPtr<CHost>& srcHost, 
                                      CSmartPtr<CHost>& dstHost, 
                                      packet_in_info_t* data, 
                                      arp_t* arpPkt);
		 INT4 ArpResponseHandler(const CSmartPtr<CSwitch>& srcSw, 
                                       CSmartPtr<CHost>& srcHost, 
                                       CSmartPtr<CHost>& dstHost, 
                                       packet_in_info_t* data, 
                                       arp_t* arpPkt);

		 virtual INT4 IpHandler(const CSmartPtr<CSwitch>& srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, ip_t* pkt, packet_in_info_t* packetIn) = 0;
		 virtual INT4 GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, UINT4 srcIp, UINT4 dstIp,  UINT2 srcPort, UINT2 dstPort, UINT2 proto) = 0;

	public:
		 bnc::l3service::service_type m_type;
};
#endif

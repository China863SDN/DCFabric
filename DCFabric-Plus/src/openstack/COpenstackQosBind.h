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
*   File Name   : COpenstackQosBind.h                                       *
*   Author      : bnc cyyang                                                 *
*   Create Date : 2018-3-10                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/

#ifndef _COPENSTACKQOSBIND_H_
#define _COPENSTACKQOSBIND_H_

#include <string>
#include "bnc-type.h"

class COpenstackQosBind
{
	public:
		COpenstackQosBind(): m_ingressid(""), m_egressid(""),m_portMac(""),m_portIp(0){}
		COpenstackQosBind(const std::string & port_id, const std::string & ingress_id, const std::string & egress_id, const std::string& mac, UINT4 ip);
		~COpenstackQosBind(){}

	public:
		void SetPortId(const std::string & port_id){m_portid = port_id; }
		const std::string & GetPortId() const {return m_portid; }

		void setQosId(const std::string & ingress_id, const std::string & egress_id){m_ingressid = ingress_id; m_egressid = egress_id; }
		void SetIngressQosId(const std::string & ingress_id){m_ingressid = ingress_id; }
		void SetEgressQosId(const std::string & egress_id){m_egressid = egress_id; }
		const std::string & GetIngressQosId() const {return m_ingressid; }
		const std::string & GetEgressQosId() const {return m_egressid; }		

		void  SetPortIp(UINT4 ip) {m_portIp = ip; }
		UINT4 GetPortIp() const {return m_portIp; }

		void  SetPortMac(const std::string& str_mac){m_portMac = str_mac; }
		const std::string & GetPortMac() const {return m_portMac; }

		
		BOOL CompareObjectValue(const COpenstackQosBind* qosBind);
		
		BOOL SetObjectValue(const COpenstackQosBind* qosBind);

		
		void  setCheckStatus(UINT1 status){m_checkStatus = status; }
		UINT1 getCheckStatus(){ return m_checkStatus; }
		
	private:
		std::string    m_portid;
		std::string    m_ingressid;
		std::string    m_egressid;
		std::string    m_portMac;
		UINT4		   m_portIp;
		UINT1 		   m_checkStatus;
};

#endif

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
*   File Name   : COpenstackQosBind.cpp                                       *
*   Author      : bnc cyyang                                                 *
*   Create Date : 2018-3-10                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/

#include "COpenstackQosBind.h"

COpenstackQosBind::COpenstackQosBind(const std::string & port_id, const std::string & ingress_id, const std::string & egress_id,  const std::string& mac, UINT4 ip)
:m_portid(port_id), m_ingressid(ingress_id), m_egressid(egress_id), m_portMac(mac), m_portIp(ip)
{
	
}

BOOL COpenstackQosBind::CompareObjectValue(const COpenstackQosBind* qosBind)
{
	if(NULL == qosBind)
	{
		return FALSE;
	}
	if((m_portid != qosBind->GetPortId())||(m_ingressid != qosBind->GetIngressQosId())
		||(m_egressid != qosBind->GetEgressQosId())||(m_portIp != qosBind->GetPortIp()))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL COpenstackQosBind::SetObjectValue(const COpenstackQosBind* qosBind)
{
	if(NULL == qosBind)
	{
		return FALSE;
	}
	this->SetPortId(qosBind->GetPortId());
	this->setQosId(qosBind->GetIngressQosId(), qosBind->GetEgressQosId());
	this->SetPortMac(qosBind->GetPortMac());
	this->SetPortIp(qosBind->GetPortIp());
	return TRUE;
}
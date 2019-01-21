
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
*   File Name   : COpenstackQosRule.cpp                                      *
*   Author      : bnc cyyang                                                 *
*   Create Date : 2018-3-10                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COpenstackQosRule.h"

COpenstackQosRule::COpenstackQosRule(const std::string & ruleid, const std::string & rulename, const std::string & description, const std::string & tenantid, const std::string & proto, const std::string & type,
			UINT1 shared, UINT8 max_rate, BOOL derection_in):m_ruleid(ruleid),m_rulename(rulename),m_description(description),m_tenant_id(tenantid),m_protocol(proto),m_type(type),
			m_shared(shared),m_max_rate(max_rate),m_bIngress(derection_in)
{
	
}
BOOL  COpenstackQosRule::CompareObjectValue(const COpenstackQosRule* qosRule)
{
	if(NULL == qosRule)
	{
		return FALSE;
	}
	if((m_ruleid != qosRule->GetQosId())||(m_rulename != qosRule->GetRuleName())||(m_tenant_id != qosRule->GetTenantId())
		||(m_protocol != qosRule->GetProtocol())||(m_type != qosRule->GetTenantId())||(m_max_rate != qosRule->GetMaxRate())
		||(m_bIngress != qosRule->GetDerection()))
	{
		return FALSE;
	}
	return TRUE;
}



BOOL  COpenstackQosRule::SetObjectValue(const COpenstackQosRule* qosRule)
{
	if(NULL == qosRule)
	{
		return FALSE;
	}
	this->SetQosId(qosRule->GetQosId());
	this->SetRuleName(qosRule->GetRuleName());
	this->SetDescript(qosRule->GetDescript());
	this->SetTenantId(qosRule->GetTenantId());
	this->SetProtocol(qosRule->GetProtocol());
	this->SetQosType(qosRule->GetQosType());
	this->SetMaxRate(qosRule->GetMaxRate());
	this->SetDerection(qosRule->GetDerection());
	this->SetShared(qosRule->GetShared());
	return TRUE;
	
}



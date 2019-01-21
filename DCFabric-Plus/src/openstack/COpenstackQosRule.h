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
*   File Name   : COpenstackQosRule.h                                      *
*   Author      : bnc cyyang                                                 *
*   Create Date : 2018-3-10                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/

#ifndef _COPENSTACKQOSRULE_H_
#define _COPENSTACKQOSRULE_H_

#include <string>
#include "bnc-type.h"

class COpenstackQosRule
{
	public:
		COpenstackQosRule(){};
		COpenstackQosRule(const std::string & ruleid, const std::string & rulename, const std::string & description, const std::string & tenantid, const std::string & proto, const std::string & type,
			UINT1 shared, UINT8 max_rate, BOOL derection_in);
		~COpenstackQosRule(){};
	public:
		void SetQosId(const std::string & qos_rule_id) { m_ruleid = qos_rule_id; }
		const std::string & GetQosId() const {return m_ruleid; }

		void SetRuleName(const std::string & qos_rule_name){m_rulename = qos_rule_name; }
		const std::string & GetRuleName() const { return m_rulename; }

		void SetDescript(const std::string & descript) {m_description = descript; }
		const std::string & GetDescript() const {return  m_description; }

		void SetTenantId(const std::string & tenantid) {m_tenant_id = tenantid; }
		const std::string & GetTenantId() const {return m_tenant_id; }

		void SetProtocol(const std::string & proto) {m_protocol = proto; }
		const std::string & GetProtocol() const {return m_protocol; }

		void SetQosType(const std::string & type) {m_type = type; }
		const std::string & GetQosType() const { return m_type; }


		void  SetShared(BOOL shared) {m_shared = shared; }
		BOOL  GetShared() const { return m_shared; }

		void  SetMaxRate(UINT8 max_rate){m_max_rate = max_rate; }
		UINT8 GetMaxRate() const {return m_max_rate; }

		void  SetDerection(BOOL derection_ingress) {m_bIngress = derection_ingress; }
		BOOL  GetDerection() const {return m_bIngress; }

		BOOL  CompareObjectValue(const COpenstackQosRule* qosRule);
		BOOL  SetObjectValue(const COpenstackQosRule* qosRule);

		void  setCheckStatus(UINT1 status){m_checkStatus = status; }
		UINT1 getCheckStatus(){ return m_checkStatus; }

	private:
		std::string 		m_ruleid;				
		std::string			m_rulename;			
		std::string  		m_description;		
		std::string 		m_tenant_id;					
		std::string 		m_protocol;	
		std::string	 		m_type;	
		BOOL 				m_shared;	
		UINT8 				m_max_rate;		//:bit/s bps
		BOOL    			m_bIngress;
		UINT1 				m_checkStatus;
};

#endif

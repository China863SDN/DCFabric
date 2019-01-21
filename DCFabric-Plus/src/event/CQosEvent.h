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
*   File Name   : CQosEvent.h                                       *
*   Author      : bnc cyyang                                                 *
*   Create Date : 2018-5-10                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/


#ifndef _CQOSEVENT_H_
#define _CQOSEVENT_H_

#include "bnc-type.h"
#include "CEvent.h"
#include "COpenstackQosRule.h"
class CQosRuleEvent:  public CEvent
{
	public:
		CQosRuleEvent();
		~CQosRuleEvent();
		CQosRuleEvent(INT4 event, INT4 reason, const COpenstackQosRule& qosRule);
	private:
		COpenstackQosRule    m_qosRule;
};

class CQosBindEvent:  public CEvent
{
	public:
		CQosBindEvent();
		~CQosBindEvent();
		CQosBindEvent(INT4 event, INT4 reason, const std::string & mac, const std::string& ingressId, const std::string & egressId);
	private:
		std::string m_strmac;
		std::string m_ingressId;
		std::string m_egressId;
};
#endif

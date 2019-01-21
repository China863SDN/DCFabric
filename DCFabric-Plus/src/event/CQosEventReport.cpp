

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
*   File Name   : CQosEventReport.h                                       *
*   Author      : bnc cyyang                                                 *
*   Create Date : 2018-5-10                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "CQosEventReport.h"

CQosEventReport* CQosEventReport::m_pInstance = NULL;

CQosEventReport::CQosEventReport()
{
	
}

CQosEventReport::~CQosEventReport()
{
	
}
INT4 CQosEventReport::report(INT4 event, INT4 reason, const std::string & mac, const std::string& ingressId, const std::string & egressId)
{
	if (mac.empty() || ingressId.empty()||egressId.empty())
        return BNC_ERR;

    if (!((EVENT_TYPE_QOS_ATTCH  <= event) && (EVENT_TYPE_QOS_UPDATE  >= event)))
        return BNC_ERR;

    CQosBindEvent* evt = new CQosBindEvent(event, reason, mac, ingressId, egressId);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CQosBindEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    INT1 descStr[1024];
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], qos {ingress:[%s] egress:[%s]} %s to host[%s]",
             event, path.c_str(), reason, ingressId.c_str(), egressId.c_str(),
             (EVENT_TYPE_QOS_ATTCH ==event)?"ATTACHED":"UPDATE",
             mac.c_str());
    //LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);
    
    return CEventReportor::report(evt);
	return BNC_OK;
}
INT4 CQosEventReport::report(INT4 event, INT4 reason, const COpenstackQosRule& qosRule)
{

	if (!((EVENT_TYPE_QOS_RULE_C  <= event) && (EVENT_TYPE_QOS_RULE_DS >= event)))
        return BNC_ERR;

    CQosRuleEvent* evt = new CQosRuleEvent(event, reason, qosRule);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CQosRuleEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    INT1 descStr[1024];
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], %s qos rule[%s:%s:%u:%llu/%s:%s,%s]",
             event, path.c_str(), reason, 
             (EVENT_TYPE_QOS_RULE_C == event)?"CREATED":
             (EVENT_TYPE_QOS_RULE_U ==event)?"UPDATED":
             (EVENT_TYPE_QOS_RULE_D ==event)?"DELETED":"UNKNOWN",
             qosRule.GetQosId().c_str(), qosRule.GetRuleName().c_str(), qosRule.GetDerection(), qosRule.GetMaxRate(),
             qosRule.GetProtocol().c_str(), qosRule.GetRuleName().c_str(), qosRule.GetQosType().c_str());
    //LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);
    
    return CEventReportor::report(evt);
	return BNC_OK;
}

CMsgPath CQosEventReport::getMsgPath(INT4 event)
{
    return g_qosPolicyEventPath[0];
}


CQosEventReport* CQosEventReport::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CQosEventReport();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
}



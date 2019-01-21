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
*   File Name   : CSecurityGroupEventReportor.cpp                             *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CSecurityGroupEventReportor.h"
#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"

CSecurityGroupEventReportor* CSecurityGroupEventReportor::m_pInstance = NULL;       

CSecurityGroupEventReportor* CSecurityGroupEventReportor::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CSecurityGroupEventReportor();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return (m_pInstance);
}

CSecurityGroupEventReportor::CSecurityGroupEventReportor()
{
}

CSecurityGroupEventReportor::~CSecurityGroupEventReportor()
{
}

INT4 CSecurityGroupEventReportor::report(INT4 event, INT4 reason, const std::string& macAddress, const std::string& securityGroupId)
{
    if (macAddress.empty() || securityGroupId.empty())
        return BNC_ERR;

    if (!((EVENT_TYPE_SECURITY_GROUP_ATTACH <= event) && (EVENT_TYPE_SECURITY_GROUP_DETACH >= event)))
        return BNC_ERR;

    CSecurityGroupEvent* evt = new CSecurityGroupEvent(event, reason, macAddress, securityGroupId);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CSecurityGroupEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    INT1 descStr[1024];
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], security group[%s] %s to host[%s]",
             event, path.c_str(), reason, securityGroupId.c_str(),
             (EVENT_TYPE_SECURITY_GROUP_ATTACH==event)?"ATTACHED":"DETACHED",
             macAddress.c_str());
    //LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);
    
    return CEventReportor::report(evt);
}

INT4 CSecurityGroupEventReportor::report(INT4 event, INT4 reason, const CSecurityGroupRule& rule)
{
    if (!((EVENT_TYPE_SECURITY_GROUP_RULE_C <= event) && (EVENT_TYPE_SECURITY_GROUP_RULE_DS >= event)))
        return BNC_ERR;

    CSecurityGroupRuleEvent* evt = new CSecurityGroupRuleEvent(event, reason, rule);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CSecurityGroupRuleEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    INT1 descStr[1024];
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], %s security group rule[%s:%s:%s:%u:%s/%u:[%u,%u]]",
             event, path.c_str(), reason, 
             (EVENT_TYPE_SECURITY_GROUP_RULE_C==event)?"CREATED":
             (EVENT_TYPE_SECURITY_GROUP_RULE_U==event)?"UPDATED":
             (EVENT_TYPE_SECURITY_GROUP_RULE_D==event)?"DELETED":"UNKNOWN",
             rule.getGroupId().c_str(), rule.getRuleId().c_str(), rule.getDirection().c_str(), rule.getProtocol(),
             inet_htoa(rule.getRemoteIp()), rule.getRemoteIpMask(), rule.getPortMin(), rule.getPortMax());
    //LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);
    
    return CEventReportor::report(evt);
}

CMsgPath CSecurityGroupEventReportor::getMsgPath(INT4 event)
{
    return g_securityGroupEventPath[0];
}


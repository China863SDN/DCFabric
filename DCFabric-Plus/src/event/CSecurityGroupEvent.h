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
*   File Name   : CSecurityGroupEvent.h                                       *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CSECURITYGROUPEVENT_H
#define __CSECURITYGROUPEVENT_H

#include "bnc-type.h"
#include "bnc-param.h"
#include "CEvent.h"
#include "CFirewallRule.h"

typedef CFirewallRule CSecurityGroupRule;

class CSecurityGroupEvent : public CEvent
{
public:
    CSecurityGroupEvent();
    CSecurityGroupEvent(INT4 event, INT4 reason, const std::string& macAddress, const std::string& securityGroupId);
    ~CSecurityGroupEvent();

    const std::string& getMacAddress() {return m_macAddress;}
    const std::string& getSecurityGroupId() {return m_securityGroupId;}

private:
    std::string m_macAddress;
    std::string m_securityGroupId;
};

class CSecurityGroupRuleEvent : public CEvent
{
public:
    CSecurityGroupRuleEvent();
    CSecurityGroupRuleEvent(INT4 event, INT4 reason, const CSecurityGroupRule& rule);
    ~CSecurityGroupRuleEvent();

    const CSecurityGroupRule& getSecurityGroupRule() {return m_rule;}

private:
    CSecurityGroupRule m_rule;
};

#endif

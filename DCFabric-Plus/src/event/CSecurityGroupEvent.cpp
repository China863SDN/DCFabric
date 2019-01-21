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
*   File Name   : CSecurityGroupEvent.cpp                                     *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CSecurityGroupEvent.h"
#include "bnc-error.h"
#include "bnc-param.h"

CSecurityGroupEvent::CSecurityGroupEvent()
{
}

CSecurityGroupEvent::CSecurityGroupEvent(INT4 event, INT4 reason, const std::string& macAddress, const std::string& securityGroupId):
    CEvent(MSG_OPER_PASSON, event, reason),
    m_macAddress(macAddress),
    m_securityGroupId(securityGroupId)
{
}

CSecurityGroupEvent::~CSecurityGroupEvent()
{
}

CSecurityGroupRuleEvent::CSecurityGroupRuleEvent()
{
}

CSecurityGroupRuleEvent::CSecurityGroupRuleEvent(INT4 event, INT4 reason, const CSecurityGroupRule& rule):
    CEvent(MSG_OPER_PASSON, event, reason),
    m_rule(rule)
{
}

CSecurityGroupRuleEvent::~CSecurityGroupRuleEvent()
{
}



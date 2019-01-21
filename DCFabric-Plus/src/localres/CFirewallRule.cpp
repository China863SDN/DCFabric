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
*   File Name   : CFirewallRule.cpp                                           *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CFirewallRule.h"
#include "bnc-error.h"
#include "bnc-inet.h"

CFirewallRule::CFirewallRule():
    m_enabled(FALSE),
    m_protocol(0),
    m_remoteIp(0),
    m_remoteIpMask(0),
    m_portMin(0),
    m_portMax(0),
    m_priority(0)
{
}

CFirewallRule::CFirewallRule(const CFirewallRule& rule):
    m_enabled(rule.m_enabled),
    m_ruleId(rule.m_ruleId),
    m_groupId(rule.m_groupId),
    m_protocol(rule.m_protocol),
    m_direction(rule.m_direction),
    m_remoteIp(rule.m_remoteIp),
    m_remoteIpMask(rule.m_remoteIpMask),
    m_portMin(rule.m_portMin),
    m_portMax(rule.m_portMax),
    m_priority(rule.m_priority),
    m_action(rule.m_action)
{
}

CFirewallRule::~CFirewallRule()
{
}

BOOL CFirewallRule::Compare(const CFirewallRule& rule)
{
	if ((rule.m_enabled == m_enabled) &&
        (0 == rule.m_ruleId.compare(m_ruleId)) &&
        (0 == rule.m_groupId.compare(m_groupId)) &&
		(rule.m_protocol == m_protocol) &&
        (0 == rule.m_direction.compare(m_direction)) &&
		(rule.m_remoteIp == m_remoteIp) &&
		(rule.m_remoteIpMask == m_remoteIpMask) &&
		(rule.m_portMin == m_portMin) &&
		(rule.m_portMax == m_portMax) &&
		(rule.m_priority == m_priority) &&
		(0 == rule.m_action.compare(m_action)))
		return TRUE;

	return FALSE;
}

void CFirewallRule::setProtocol(const std::string& protocol)
{
    if (protocol.empty())
        m_protocol = 0;
    else if (protocol.compare("tcp") == 0)
        m_protocol = IP_TCP;
    else if (protocol.compare("udp") == 0)
        m_protocol = IP_UDP;
    else if (protocol.compare("icmp") == 0)
        m_protocol = IP_ICMP;
    else if (protocol.compare("vrrp") == 0)
        m_protocol = IP_VRRP;
    else
        m_protocol = 0xFF;
}


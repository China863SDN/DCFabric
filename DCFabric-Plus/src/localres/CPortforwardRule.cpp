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
*   File Name   : CPortforwardRule.cpp                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CPortforwardRule.h"
#include "log.h"
#include "bnc-error.h"
#include "bnc-inet.h"

CPortforwardRule::CPortforwardRule():
    m_enabled(FALSE),
    m_protocol(0),
    m_outIp(0),
    m_inIp(0),
    m_outPortStart(0),
    m_outPortEnd(0),
    m_inPortStart(0),
    m_inPortEnd(0)
{
}

CPortforwardRule::CPortforwardRule(const CPortforwardRule& rule):
    m_enabled(rule.m_enabled),
    m_protocol(rule.m_protocol),
    m_outIp(rule.m_outIp),
    m_inIp(rule.m_inIp),
    m_outPortStart(rule.m_outPortStart),
    m_outPortEnd(rule.m_outPortEnd),
    m_inPortStart(rule.m_inPortStart),
    m_inPortEnd(rule.m_inPortEnd),
    m_networkId(rule.m_networkId),
    m_subnetId(rule.m_subnetId)
{
}

CPortforwardRule::~CPortforwardRule()
{
}

BOOL CPortforwardRule::Compare(const CPortforwardRule& rule)
{
	if ((rule.m_enabled == m_enabled) &&
		(rule.m_protocol == m_protocol) &&
		(rule.m_outIp == m_outIp) &&
		(rule.m_inIp == m_inIp) &&
		(rule.m_outPortStart == m_outPortStart) &&
		(rule.m_outPortEnd == m_outPortEnd) &&
		(rule.m_inPortStart == m_inPortStart) &&
		(rule.m_inPortEnd == m_inPortEnd) &&
		(rule.m_outPortStart == m_outPortStart) &&
		(0 == rule.m_networkId.compare(m_networkId)) &&
		(0 == rule.m_subnetId.compare(m_subnetId)))
		return TRUE;

	return FALSE;
}

void CPortforwardRule::setProtocol(const std::string& protocol)
{
    if (protocol.compare("tcp") == 0)
        m_protocol = IP_TCP;
    else if (protocol.compare("udp") == 0)
        m_protocol = IP_UDP;
    else
        m_protocol = 0;
}


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
*   File Name   : CFirewallRule.h                                             *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CFIREWALLRULL_H
#define __CFIREWALLRULL_H

#include "bnc-param.h"
#include "CMutex.h"
#include "CRefObj.h"
#include "CRWLock.h"

#define FIREWALL_DIRECTION_IN			"ingress"
#define FIREWALL_DIRECTION_OUT			"egress"

#define FIREWALL_ACTION_ACCEPT			"accept"
#define FIREWALL_ACTION_DROP			"drop"

class CFirewallRule : public CRefObj
{
public:
    CFirewallRule();
    CFirewallRule(const CFirewallRule& rule);
    ~CFirewallRule();

	BOOL Compare(const CFirewallRule& rule);

    void setEnabled(BOOL enabled) {m_enabled = enabled;}
    void setRuleId(const std::string& ruleId) {m_ruleId = ruleId;}
    void setGroupId(const std::string& groupId) {m_groupId = groupId;}
    void setProtocol(const std::string& protocol);
    void setDirection(const std::string& direction) {m_direction = direction;}
    void setRemoteIp(UINT4 ip) {m_remoteIp = ip;}
    void setRemoteIpMask(UINT4 ipMask) {m_remoteIpMask = ipMask;}
    void setPortMin(UINT2 portMin) {m_portMin = portMin;}
    void setPortMax(UINT2 portMax) {m_portMax = portMax;}
    void setPriority(UINT2 priority) {m_priority = priority;}
    void setAction(const std::string& action) {m_action = action;}

    BOOL getEnabled() const {return m_enabled;}
    const std::string& getRuleId() const {return m_ruleId;}
    const std::string& getGroupId() const {return m_groupId;}
    UINT1 getProtocol() const {return m_protocol;}
    const std::string& getDirection() const {return m_direction;}
    UINT4 getRemoteIp() const {return m_remoteIp;}
    UINT4 getRemoteIpMask() const {return m_remoteIpMask;}
    UINT2 getPortMin() const {return m_portMin;}
    UINT2 getPortMax() const {return m_portMax;}
    UINT2 getPriority() const {return m_priority;}
    const std::string& getAction() const {return m_action;}

private:
    BOOL        m_enabled;
    std::string m_ruleId;
    std::string m_groupId;
    UINT1       m_protocol;
    std::string m_direction;
    UINT4       m_remoteIp;
    UINT4       m_remoteIpMask;
    UINT2       m_portMin;
    UINT2       m_portMax;
    UINT2       m_priority;
    std::string m_action;
};

#endif

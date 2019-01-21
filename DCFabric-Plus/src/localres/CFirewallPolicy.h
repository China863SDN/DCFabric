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
*   File Name   : CFirewallPolicy.h                                           *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CFIREWALLPOLICY_H
#define __CFIREWALLPOLICY_H

#include "CFirewallRule.h"
#include "CLFHashMap.h"
#include "CRWLock.h"
#include "CSmartPtr.h"
#include "CEventNotifier.h"
#include "CTimer.h"
#include "CHost.h"

typedef std::list<std::string> CHostMacList; //list of host macs
typedef CStringLFHashMap<CHostMacList> CGroupHostListHMap; //group id <-> list of host macs

typedef std::list<std::string> CGroupList; //list of group ids
typedef CStringLFHashMap<CGroupList> CHostGroupListHMap; //host mac <-> list of group ids

typedef CStringLFHashMap<CSmartPtr<CFirewallRule> > CRuleHMap; //group rule id <-> group rule
typedef CStringLFHashMap<CRuleHMap> CGroupHMap; //group id <-> mapping of group rule ids

typedef std::list<CSmartPtr<CFirewallRule> > CFWRuleList; //list of group rules
typedef CUint32LFHashMap<CFWRuleList> CRemoteRuleListHMap; //remote ip <-> list of group rules
typedef CStringLFHashMap<CRemoteRuleListHMap> CPolicyHMap; //host mac <-> mapping of remote ips
typedef CStringLFHashMap<BOOL> CFWEnforcedHMap; //host mac <-> policy already enforced

class CFirewallPolicy : public CEventNotifier
{
public:
    static const UINT4 hash_bucket_number = 1024;

    static void firewallGuard(void* param);

public:
    static CFirewallPolicy* getInstance();

    ~CFirewallPolicy();

    INT4 init();

    INT4 onregister();
    void deregister();

    INT4 apply(const UINT1* mac);
    INT4 installFirewallFlow(const std::string& macAddress, const CSmartPtr<CFirewallRule>& firewallRule, UINT1 command);

    INT4 applyOut(const CSmartPtr<CSwitch>& sw, const UINT1* srcMac, UINT4 srcIp, UINT4 dstIp, UINT1 protocol, UINT2 srcPort, UINT2 dstPort);
    INT4 applyIn(const CSmartPtr<CSwitch>& sw, UINT4 srcIp, UINT4 dstIp, UINT1 protocol, UINT2 srcPort, UINT2 dstPort);

    CGroupHMap& getGroups() {return m_groups;}
    CPolicyHMap& getPolicy() {return m_policy;}
    BOOL getEnfored(const std::string& macAddress);
    void setEnfored(const std::string& macAddress, BOOL enforced);

    const char* toString() {return "CFirewallPolicy";}

    void dump();

private:
    CFirewallPolicy();

    INT4 consume(CSmartPtr<CMsgCommon> evt);

    INT4 attachSecurityGroup(const std::string& macAddress, const std::string& groupId);
    void detachSecurityGroup(const std::string& macAddress, const std::string& groupId);
    INT4 createSecurityGroupRule(const CFirewallRule& groupRule);
    INT4 updateSecurityGroupRule(const CFirewallRule& groupRule);
    void deleteSecurityGroupRule(const CFirewallRule& groupRule);

    CHostMacList* findHostList(const std::string& groupId);
    INT4 createGroupHostListMapping(const std::string& groupId, const std::string& macAddress);
    void deleteGroupHostListMapping(const std::string& groupId, const std::string& macAddress);

    CGroupList* findGroupList(const std::string& macAddress);
    INT4 createHostGroupListMapping(const std::string& macAddress, const std::string& groupId);
    void deleteHostGroupListMapping(const std::string& macAddress, const std::string& groupId);

    CRuleHMap* findGroupRuleMap(const std::string& groupId);
    CRuleHMap* createGroupRuleMap(const std::string& groupId);
    void deleteGroupRuleMap(const std::string& groupId);
    CSmartPtr<CFirewallRule> findGroupRule(CRuleHMap& groupRuleMap, const std::string& groupRuleId);
    CSmartPtr<CFirewallRule> updateGroupRule(CRuleHMap& groupRuleMap, const CFirewallRule& groupRule);
    void deleteGroupRule(CRuleHMap& groupRuleMap, const std::string& groupRuleId);

    CRemoteRuleListHMap* findRemoteRuleListMap(const std::string& macAddress);
    CRemoteRuleListHMap* createRemoteRuleListMap(const std::string& macAddress);

    CFWRuleList* findRemoteRuleList(CRemoteRuleListHMap& remoteRuleListMap, UINT4 ip);
    CFWRuleList* createRemoteRuleList(CRemoteRuleListHMap& remoteRuleListMap, UINT4 ip);

    INT4 enforceGroupRuleMap(const std::string& macAddress, CRuleHMap& groupRuleMap);
    INT4 enforceGroupRule(const std::string& macAddress, const CSmartPtr<CFirewallRule>& groupRule);
    INT4 enforceGroupRule(const std::string& macAddress, CRemoteRuleListHMap& remoteRuleListMap, const CSmartPtr<CFirewallRule>& groupRule);
    INT4 enforceGroupRule(const std::string& macAddress, CFWRuleList& remoteRuleList, const CSmartPtr<CFirewallRule>& groupRule);

    void unenforceGroupRuleMap(const std::string& macAddress, CRuleHMap& groupRuleMap);
    void unenforceGroupRule(const std::string& macAddress, const CSmartPtr<CFirewallRule>& groupRule);
    void unenforceGroupRule(const std::string& macAddress, CRemoteRuleListHMap& remoteRuleListMap, const CSmartPtr<CFirewallRule>& groupRule);
    void unenforceGroupRule(const std::string& macAddress, CFWRuleList& remoteRuleList, const CSmartPtr<CFirewallRule>& groupRule);

    INT4 installFirewallOutEphemeralFlow(const std::string& macAddress, 
        UINT4 srcIP, UINT4 dstIP, UINT1 protocol, UINT2 srcPort, UINT2 dstPort, UINT2 priority, BOOL accept);
    INT4 installFirewallInEphemeralFlow(const std::string& macAddress, 
        UINT4 srcIP, UINT4 dstIP, UINT1 protocol, UINT2 srcPort, UINT2 dstPort, UINT2 priority, BOOL accept);

private:
    static CFirewallPolicy* m_instance;      

    CGroupHostListHMap m_groupHostMap;
    CHostGroupListHMap m_hostGroupMap;
    CGroupHMap         m_groups;
    CPolicyHMap        m_policy;
    CFWEnforcedHMap    m_enforced;
    CRWLock            m_rwlock;
    CTimer             m_timer;
};

#endif

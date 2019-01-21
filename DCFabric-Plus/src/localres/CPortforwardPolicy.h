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
*   File Name   : CPortforwardPolicy.h                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CPORTFORWARDPOLICY_H
#define __CPORTFORWARDPOLICY_H

#include "CPortforwardRule.h"
#include "CLFHashMap.h"
#include "CRWLock.h"
#include "CSmartPtr.h"
#include "CEventNotifier.h"
#include "CTimer.h"

typedef std::list<CSmartPtr<CPortforwardRule> > CPFRuleList; //list of portforward rules
typedef CUint32LFHashMap<CPFRuleList> CPFRuleListHMap; //outside/inside ip <-> list of portforward rules
typedef CUint32LFHashMap<BOOL> CPFEnforcedHMap; //inside ip <-> policy already enforced

class CPortforwardPolicy : public CEventNotifier
{
public:
    static const UINT4 hash_bucket_number = 1024;

    static void portforwardGuard(void* param);

public:
    static CPortforwardPolicy* getInstance();

    ~CPortforwardPolicy();

    INT4 init();

    INT4 onregister();
    void deregister();

    INT4 apply(UINT4 ip);
    INT4 installPortforwardFlow(const CSmartPtr<CPortforwardRule>& portforwardRule, UINT1 command);

    BOOL checkIn(UINT4 outIp, UINT2 outPort, INT4 protocol, UINT4& inIp, UINT2& inPort);
    BOOL checkOut(UINT4 inIp, UINT2 inPort, INT4 protocol, UINT4& outIp, UINT2& outPort);
    INT4 applyIn(UINT4 dstIp, UINT2 dstPort, UINT1 protocol);
    INT4 applyOut(UINT4 srcIp, UINT2 srcPort, UINT1 protocol);

    CPFRuleListHMap& getOutPolicy() {return m_outPolicy;}
    CPFRuleListHMap& getInPolicy() {return m_inPolicy;}
    BOOL getEnfored(UINT4 ip);
    void setEnfored(UINT4 ip, BOOL enforced);

    const char* toString() {return "CPortforwardPolicy";}

    void dump();

private:
    CPortforwardPolicy();

    INT4 consume(CSmartPtr<CMsgCommon> evt);

    INT4 createPortforwardRule(const CPortforwardRule& rule);
    void deletePortforwardRule(const CPortforwardRule& rule);

    CPFRuleList* findOutRuleList(UINT4 outIp);
    CPFRuleList* createOutRuleList(UINT4 outIp);
    CPFRuleList* findInRuleList(UINT4 inIp);
    CPFRuleList* createInRuleList(UINT4 inIp);

    INT4 enforcePortforwardRule(CPFRuleList& ruleList, const CSmartPtr<CPortforwardRule>& rule);
    void unenforcePortforwardRule(CPFRuleList& ruleList, const CSmartPtr<CPortforwardRule>& rule);

    INT4 installPortforwardEphemeralFlow(UINT4 outIP, UINT2 outPort, UINT1 protocol, UINT4 inIP, UINT2 inPort);

private:
    static CPortforwardPolicy* m_instance;      

    CPFRuleListHMap m_outPolicy; //key is outside ip
    CPFRuleListHMap m_inPolicy; //key is inside ip
    CPFEnforcedHMap m_enforced;
    CRWLock         m_rwlock;
    CTimer          m_timer;
};

#endif

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
*   File Name   : CPortforwardPolicy.cpp                                      *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CPortforwardPolicy.h"
#include "log.h"
#include "bnc-type.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CPortforwardEvent.h"
#include "CHostMgr.h"
#include "CFlowMgr.h"
#include "openflow-common.h"
#include "CClusterService.h"
#include "CConf.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "CControl.h"

extern const char* g_portforwardEventPath[];

CPortforwardPolicy* CPortforwardPolicy::m_instance = NULL;      

CPortforwardPolicy* CPortforwardPolicy::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CPortforwardPolicy();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return m_instance;
}

void CPortforwardPolicy::portforwardGuard(void* param)
{
    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return;

    CPFRuleListHMap& policy = CPortforwardPolicy::getInstance()->getInPolicy();
    STL_FOR_LOOP(policy, ipIt)
    {
        CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByIp(htonl(ipIt->first));
        if ((host.isNull()) || host->getSw().isNull())
            continue;
        
        CPFRuleList& ruleList = ipIt->second;
        STL_FOR_LOOP(ruleList, ruleIt)
        {
            CSmartPtr<CPortforwardRule>& rule = *ruleIt;
        
            INT1 inIpStr[20] = {0}, outIpStr[20] = {0};
            number2ip(htonl(rule->getInsideIp()), inIpStr);
            number2ip(htonl(rule->getOutsideIp()), outIpStr);
            UINT2 outStart = 0, outEnd = 0, inStart = 0, inEnd = 0;
            rule->getOutsidePort(outStart, outEnd);
            rule->getInsidePort(inStart, inEnd);
            LOG_WARN_FMT(">>> host[%s] apply PORTFORWARD RULE: "
                "enabled[%s]protocol[%d]outIp[%s]inIp[%s]outPort[%u-%u]inPort[%u-%u]networkId[%s]subnetId[%s]",
                inIpStr, rule->getEnabled()?"true":"false", rule->getProtocol(), outIpStr, inIpStr, 
                outStart, outEnd, inStart, inEnd, rule->getNetworkId().c_str(), rule->getSubnetId().c_str());
        
            if (BNC_OK == CPortforwardPolicy::getInstance()->installPortforwardFlow(rule, OFPFC_ADD))
                CPortforwardPolicy::getInstance()->setEnfored(rule->getInsideIp(), TRUE);
        }

    }
}

CPortforwardPolicy::CPortforwardPolicy():
    m_outPolicy(hash_bucket_number),
    m_inPolicy(hash_bucket_number),
    m_enforced(hash_bucket_number)
{
}

CPortforwardPolicy::~CPortforwardPolicy()
{
    deregister();
}

INT4 CPortforwardPolicy::init()
{
	if (onregister())
    {
        LOG_WARN_FMT("CPortforwardPolicy register failed !");
        return BNC_ERR;
    }

    const INT1* pConf = CConf::getInstance()->getConfig("openstack", "reload_interval");
    UINT4 interval = (pConf == NULL) ? 0 : atol(pConf);
    LOG_INFO_FMT("openstack reload_interval %us", interval);

    if (0 == interval)
        return BNC_OK;

    if (m_timer.schedule(interval, interval, CPortforwardPolicy::portforwardGuard, this) != BNC_OK)
    {
        LOG_ERROR("CPortforwardPolicy schedule portforwardGuard failed !");
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CPortforwardPolicy::onregister()
{
    CMsgPath path(g_portforwardEventPath[0]);
    return CEventNotifier::onregister(path);
}

void CPortforwardPolicy::deregister()
{
    CMsgPath path(g_portforwardEventPath[0]);
    CEventNotifier::deregister(path);
}

INT4 CPortforwardPolicy::consume(CSmartPtr<CMsgCommon> evt)
{
    INT4 ret = BNC_ERR;
    
    if (evt.isNull())
        return ret;

    CPortforwardEvent* pEvt = (CPortforwardEvent*)evt.getPtr();
    //LOG_WARN(pEvt->getDesc().c_str());

    switch (pEvt->getEvent())
    {
        case EVENT_TYPE_PORTFORWARD_RULE_C:
            LOG_DEBUG("EVENT_TYPE_PORTFORWARD_RULE_C");
            ret = createPortforwardRule(pEvt->getPortforwardRule());
            break;
        case EVENT_TYPE_PORTFORWARD_RULE_D:
            LOG_DEBUG("EVENT_TYPE_PORTFORWARD_RULE_D");
            deletePortforwardRule(pEvt->getPortforwardRule());
            ret = BNC_OK;
            break;
        default:
            LOG_WARN_FMT("%s consume invalid event[%d] !", toString(), pEvt->getEvent());
            break;
    }

    return ret;
}

INT4 CPortforwardPolicy::apply(UINT4 ip)
{
    CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByIp(htonl(ip));
    if ((host.isNull()) || host->getSw().isNull())
        return BNC_ERR;
    
    //m_rwlock.rlock();

    CPFRuleList* ruleList = findInRuleList(ip);
    if (NULL != ruleList)
    {
        STL_FOR_LOOP(*ruleList, ruleIt)
        {
            CSmartPtr<CPortforwardRule>& rule = *ruleIt;
        
            INT1 inIpStr[20] = {0}, outIpStr[20] = {0};
            number2ip(htonl(rule->getInsideIp()), inIpStr);
            number2ip(htonl(rule->getOutsideIp()), outIpStr);
            UINT2 outStart = 0, outEnd = 0, inStart = 0, inEnd = 0;
            rule->getOutsidePort(outStart, outEnd);
            rule->getInsidePort(inStart, inEnd);
            LOG_WARN_FMT(">>> host[%s] apply PORTFORWARD RULE: "
                "enabled[%s]protocol[%d]outIp[%s]inIp[%s]outPort[%u-%u]inPort[%u-%u]networkId[%s]subnetId[%s]",
                inIpStr, rule->getEnabled()?"true":"false", rule->getProtocol(), outIpStr, inIpStr, 
                outStart, outEnd, inStart, inEnd, rule->getNetworkId().c_str(), rule->getSubnetId().c_str());
        
            if (BNC_OK == installPortforwardFlow(rule, OFPFC_ADD))
                setEnfored(rule->getInsideIp(), TRUE);
        }
    }

    //m_rwlock.unlock();

    return BNC_OK;
}

INT4 CPortforwardPolicy::installPortforwardFlow(const CSmartPtr<CPortforwardRule>& portforwardRule, UINT1 command)
{
    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return BNC_ERR;

	if (portforwardRule.isNull() || !portforwardRule->getEnabled())
		return BNC_ERR;

    UINT4 inIp = portforwardRule->getInsideIp();
    UINT4 outIp = portforwardRule->getOutsideIp();
    INT1 inIpStr[20] = {0}, outIpStr[20] = {0};
    number2ip(htonl(inIp), inIpStr);
    number2ip(htonl(outIp), outIpStr);

    CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByIp(htonl(inIp));
	if (host.isNull())
	{
        LOG_WARN_FMT("Can not find host via inside ip[%s] !", inIpStr);
		return BNC_ERR;
	}

    CSmartPtr<CSwitch> hostSw = host->getSw();
	if (hostSw.isNull())
	{
        LOG_WARN_FMT("Host with inside ip[%s] is not tracked !", inIpStr);
		return BNC_ERR;
	}
    
	Base_External* baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(htonl(inIp));
    if (NULL == baseExternalPort)
    {
        LOG_WARN_FMT("Can not find baseExternalPort via fixedIp[%s] !", inIpStr);
        return BNC_ERR;
    }

    if (0 == baseExternalPort->get_switch_DPID())
    {
        LOG_WARN("external switch is not recognized !");
        return BNC_ERR;
    }
	CSmartPtr<CSwitch> gwSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
    if (gwSw.isNull())
    {
        LOG_WARN_FMT("Can not find external switch via dpid[%llx] !", baseExternalPort->get_switch_DPID());
        return BNC_ERR;
    }
    
    UINT2 outStart = 0, outEnd = 0, inStart = 0, inEnd = 0;
    portforwardRule->getOutsidePort(outStart, outEnd);
    portforwardRule->getInsidePort(inStart, inEnd);
    LOG_WARN_FMT("inside ip[%s]port[%u-%u] <==> outside ip[%s]port[%u-%u] protocol[%d]", 
        inIpStr, inStart, inEnd, outIpStr, outStart, outEnd, portforwardRule->getProtocol());

    if ((outStart == outEnd) && (inStart == inEnd))
    {
        if (hostSw != gwSw)
        {
            //traffic in
            //gwSw --> hostSw
            UINT4 outputPort = CControl::getInstance()->getTopoMgr().get_out_port_between_switch(gwSw, hostSw);        
            CFlowMgr::getInstance()->install_portforwardIn_output_flow(gwSw, 0, 0, outIp, outStart, 
                portforwardRule->getProtocol(), host->getMac(), 0, 0, hostSw->getTag(), outputPort, command);
            //hostSw --> host
            CFlowMgr::getInstance()->install_portforwardIn_output_flow(hostSw, 0, 0, outIp, outStart, 
                portforwardRule->getProtocol(), NULL, inIp, inStart, 0, host->getPortNo(), command);

            //traffic out
            //hostSw --> gwSw
            outputPort = CControl::getInstance()->getTopoMgr().get_out_port_between_switch(hostSw, gwSw);        
            CFlowMgr::getInstance()->install_portforwardOut_output_flow(hostSw, inIp, inStart, 0, 0, portforwardRule->getProtocol(), 
                baseExternalPort->get_gateway_MAC(), outIp, outStart, gwSw->getTag(), outputPort, command);
            //gwSw output
            CFlowMgr::getInstance()->install_portforwardOut_external_flow(gwSw, outIp, portforwardRule->getProtocol(), 
                outStart, baseExternalPort->get_switch_port(), command);
        }
        else
        {
            //traffic in
            //gwSw --> host
            CFlowMgr::getInstance()->install_portforwardIn_output_flow(gwSw, 0, 0, outIp, outStart, 
                portforwardRule->getProtocol(), host->getMac(), inIp, inStart, 0, host->getPortNo(), command);

            //traffic out
            //gwSw output
            CFlowMgr::getInstance()->install_portforwardOut_output_flow(hostSw, inIp, inStart, 0, 0, portforwardRule->getProtocol(), 
                baseExternalPort->get_gateway_MAC(), outIp, outStart, 0, baseExternalPort->get_switch_port(), command);
        }
    }
    else
    {
        //traffic in
        //gwSw --> controller
        CFlowMgr::getInstance()->install_portforwardIn_gotoController_flow(gwSw, outIp, portforwardRule->getProtocol(), command);
        
        //traffic out
        //hostSw --> controller
        CFlowMgr::getInstance()->install_portforwardOut_gotoController_flow(hostSw, inIp, portforwardRule->getProtocol(), command);
    }

    return BNC_OK;
}

BOOL CPortforwardPolicy::checkIn(UINT4 outIp, UINT2 outPort, INT4 protocol, UINT4& inIp, UINT2& inPort)
{
    BOOL ret = FALSE;

    //m_rwlock.rlock();

    CPFRuleList* ruleList = findOutRuleList(outIp);
    if (NULL != ruleList)
    {
        STL_FOR_LOOP(*ruleList, it)
        {
            CSmartPtr<CPortforwardRule>& rule = *it;
            if (rule.isNotNull() &&
                rule->getEnabled() &&
                (rule->getProtocol() == protocol) &&
                (rule->getOutsideIp() == outIp))
            {
                UINT2 outStart = 0, outEnd = 0;
                rule->getOutsidePort(outStart, outEnd);
                if ((outStart <= outPort) && (outPort <= outEnd))
                {
                    UINT2 inStart = 0, inEnd = 0;
                    rule->getInsidePort(inStart, inEnd);
                    inPort = inStart + outPort - outStart;
                    if (inPort > inEnd)
                        inPort = inEnd;
                    inIp = rule->getInsideIp();

                    INT1 outIpStr[20] = {0}, inIpStr[20] = {0};
                    number2ip(htonl(outIp), outIpStr);
                    number2ip(htonl(inIp), inIpStr);
                    LOG_WARN_FMT(">>> outIp[%s]port[%u]protocol[%d] match PORTFORWARD RULE: "
                        "enabled[true]protocol[%d]outIp[%s]inIp[%s]outPort[%u-%u]inPort[%u-%u]networkId[%s]subnetId[%s] <<<",
                        outIpStr, outPort, protocol, rule->getProtocol(), outIpStr, inIpStr, outStart, outEnd, 
                        inStart, inEnd, rule->getNetworkId().c_str(), rule->getSubnetId().c_str());

                    ret = TRUE;
                    break;
                }
            }
        }
    }

    //m_rwlock.unlock();

    return ret;
}

BOOL CPortforwardPolicy::checkOut(UINT4 inIp, UINT2 inPort, INT4 protocol, UINT4& outIp, UINT2& outPort)
{
    BOOL ret = FALSE;

    //m_rwlock.rlock();

    CPFRuleList* ruleList = findInRuleList(inIp);
    if (NULL != ruleList)
    {
        STL_FOR_LOOP(*ruleList, it)
        {
            CSmartPtr<CPortforwardRule>& rule = *it;
            if (rule.isNotNull() &&
                rule->getEnabled() &&
                (rule->getProtocol() == protocol) &&
                (rule->getInsideIp() == inIp))
            {
                UINT2 inStart = 0, inEnd = 0;
                rule->getInsidePort(inStart, inEnd);
                if ((inStart <= inPort) && (inPort <= inEnd))
                {
                    UINT2 outStart = 0, outEnd = 0;
                    rule->getOutsidePort(outStart, outEnd);
                    outPort = outStart + inPort - inStart;
                    if (outPort > outEnd)
                        outPort = outEnd;
                    outIp = rule->getOutsideIp();

                    INT1 outIpStr[20] = {0}, inIpStr[20] = {0};
                    number2ip(htonl(outIp), outIpStr);
                    number2ip(htonl(inIp), inIpStr);
                    LOG_WARN_FMT(">>> inIp[%s]port[%u]protocol[%d] match PORTFORWARD RULE: "
                        "enabled[true]protocol[%d]outIp[%s]inIp[%s]outPort[%u-%u]inPort[%u-%u]networkId[%s]subnetId[%s] <<<",
                        inIpStr, inPort, protocol, rule->getProtocol(), outIpStr, inIpStr, outStart, outEnd, 
                        inStart, inEnd, rule->getNetworkId().c_str(), rule->getSubnetId().c_str());

                    ret = TRUE;
                    break;
                }
            }
        }
    }

    //m_rwlock.unlock();

    return ret;
}

INT4 CPortforwardPolicy::applyIn(UINT4 dstIp, UINT2 dstPort, UINT1 protocol)
{
    UINT4 inIp = 0;
    UINT2 inPort = 0;

    if (!checkIn(dstIp, dstPort, protocol, inIp, inPort))
        return BNC_ERR;

    INT1 dstIpStr[20] = {0}, inIpStr[20] = {0};
    number2ip(htonl(dstIp), dstIpStr);
    number2ip(htonl(inIp), inIpStr);
    LOG_WARN_FMT(">>> traffic in: outIp[%s]port[%u] ==> inIp[%s]port[%u], protocol[%d] ...",
        dstIpStr, dstPort, inIpStr, inPort, protocol);
    
    return installPortforwardEphemeralFlow(dstIp, dstPort, protocol, inIp, inPort);
}

INT4 CPortforwardPolicy::applyOut(UINT4 srcIp, UINT2 srcPort, UINT1 protocol)
{
    UINT4 outIp = 0;
    UINT2 outPort = 0;

    if (!checkOut(srcIp, srcPort, protocol, outIp, outPort))
        return BNC_ERR;

    INT1 srcIpStr[20] = {0}, outIpStr[20] = {0};
    number2ip(htonl(srcIp), srcIpStr);
    number2ip(htonl(outIp), outIpStr);
    LOG_WARN_FMT(">>> traffic out: inIp[%s]port[%u] ==> outIp[%s]port[%u], protocol[%d] ...",
        srcIpStr, srcPort, outIpStr, outPort, protocol);
    
    return installPortforwardEphemeralFlow(outIp, outPort, protocol, srcIp, srcPort);
}

BOOL CPortforwardPolicy::getEnfored(UINT4 ip)
{
    BOOL enforced = FALSE;

    CPFEnforcedHMap::CPair* item = NULL;
    if (m_enforced.find(ip, &item))
        enforced = item->second;

    return enforced;
}

void CPortforwardPolicy::setEnfored(UINT4 ip, BOOL enforced)
{
    CPFEnforcedHMap::CPair* item = NULL;
    if (m_enforced.find(ip, &item))
    {
        item->second = enforced;
    }
    else
    {
        m_enforced.insert(ip, enforced);
    }
}

void CPortforwardPolicy::dump()
{
    m_rwlock.rlock();

    STL_FOR_LOOP(m_outPolicy, policyIt)
    {
        STL_FOR_LOOP(policyIt->second, ruleIt)
        {
            CSmartPtr<CPortforwardRule>& rule = *ruleIt;
            if (rule.isNull())
                continue;

            INT1 outIpStr[20] = {0}, inIpStr[20] = {0};
            number2ip(htonl(rule->getOutsideIp()), outIpStr);
            number2ip(htonl(rule->getInsideIp()), inIpStr);
            UINT2 outPortStart = 0, outPortEnd = 0, inPortStart = 0, inPortEnd = 0;
            rule->getOutsidePort(outPortStart, outPortEnd);
            rule->getInsidePort(inPortStart, inPortEnd);
            
            LOG_WARN_FMT("###DUMP### outside[%s] policy enforced PORTFORWARD RULE: "
                "%s,protocol[%d],outIp[%s],inIp[%s],outPort[%u-%u],inPort[%u-%u],networkId[%s],subnetId[%s]",
                outIpStr, rule->getEnabled()?"enabled":"disabled", rule->getProtocol(),
                outIpStr, inIpStr, outPortStart, outPortEnd, inPortStart, inPortEnd, 
                rule->getNetworkId().c_str(), rule->getSubnetId().c_str());
        }
    }

    STL_FOR_LOOP(m_inPolicy, policyIt)
    {
        STL_FOR_LOOP(policyIt->second, ruleIt)
        {
            CSmartPtr<CPortforwardRule>& rule = *ruleIt;
            if (rule.isNull())
                continue;

            INT1 outIpStr[20] = {0}, inIpStr[20] = {0};
            number2ip(htonl(rule->getOutsideIp()), outIpStr);
            number2ip(htonl(rule->getInsideIp()), inIpStr);
            UINT2 outPortStart = 0, outPortEnd = 0, inPortStart = 0, inPortEnd = 0;
            rule->getOutsidePort(outPortStart, outPortEnd);
            rule->getInsidePort(inPortStart, inPortEnd);
            
            LOG_WARN_FMT("###DUMP### inside[%s] policy enforced PORTFORWARD RULE: "
                "%s,protocol[%d],outIp[%s],inIp[%s],outPort[%u-%u],inPort[%u-%u],networkId[%s],subnetId[%s]",
                inIpStr, rule->getEnabled()?"enabled":"disabled", rule->getProtocol(),
                outIpStr, inIpStr, outPortStart, outPortEnd, inPortStart, inPortEnd, 
                rule->getNetworkId().c_str(), rule->getSubnetId().c_str());
        }
    }

    m_rwlock.unlock();
}

INT4 CPortforwardPolicy::createPortforwardRule(const CPortforwardRule& rule)
{
    CPortforwardRule* ruleNew = new CPortforwardRule(rule);
    if (NULL == ruleNew)
    {
        LOG_ERROR_FMT("%s new CPortforwardRule failed !", toString());
        return BNC_ERR;
    }
    CSmartPtr<CPortforwardRule> ruleCreate(ruleNew);

    CPFRuleList* outRuleList = findOutRuleList(rule.getOutsideIp());
    if (NULL == outRuleList)
    {
        outRuleList = createOutRuleList(rule.getOutsideIp());
        if (NULL == outRuleList)
            return BNC_ERR;
    }

    CPFRuleList* inRuleList = findInRuleList(rule.getInsideIp());
    if (NULL == inRuleList)
    {
        inRuleList = createInRuleList(rule.getInsideIp());
        if (NULL == inRuleList)
            return BNC_ERR;
    }

    enforcePortforwardRule(*outRuleList, ruleCreate);
    enforcePortforwardRule(*inRuleList, ruleCreate);

    //dump();

    return installPortforwardFlow(ruleCreate, OFPFC_ADD);
}

void CPortforwardPolicy::deletePortforwardRule(const CPortforwardRule& rule)
{
    CPortforwardRule* ruleNew = new CPortforwardRule(rule);
    if (NULL == ruleNew)
    {
        LOG_ERROR_FMT("%s new CPortforwardRule failed !", toString());
        return;
    }
    CSmartPtr<CPortforwardRule> ruleDelete(ruleNew);

    CPFRuleList* outRuleList = findOutRuleList(rule.getOutsideIp());
    if (NULL != outRuleList)
    {
        unenforcePortforwardRule(*outRuleList, ruleDelete);
    }

    CPFRuleList* inRuleList = findInRuleList(rule.getInsideIp());
    if (NULL != inRuleList)
    {
        unenforcePortforwardRule(*inRuleList, ruleDelete);
    }

    //dump();

    installPortforwardFlow(ruleDelete, OFPFC_DELETE_STRICT);
}

CPFRuleList* CPortforwardPolicy::findOutRuleList(UINT4 outIp)
{
    CPFRuleListHMap::CPair* item = NULL;
    if (m_outPolicy.find(outIp, &item))
        return &item->second;

    return NULL;
}

CPFRuleList* CPortforwardPolicy::createOutRuleList(UINT4 outIp)
{
    CPFRuleListHMap::CPair* item = NULL;
    if (m_outPolicy.insert(outIp, CPFRuleList(), &item))
        return &item->second;

    return NULL;
}

CPFRuleList* CPortforwardPolicy::findInRuleList(UINT4 inIp)
{
    CPFRuleListHMap::CPair* item = NULL;
    if (m_inPolicy.find(inIp, &item))
        return &item->second;

    return NULL;
}

CPFRuleList* CPortforwardPolicy::createInRuleList(UINT4 inIp)
{
    CPFRuleListHMap::CPair* item = NULL;
    if (m_inPolicy.insert(inIp, CPFRuleList(), &item))
        return &item->second;

    return NULL;
}

INT4 CPortforwardPolicy::enforcePortforwardRule(CPFRuleList& ruleList, const CSmartPtr<CPortforwardRule>& rule)
{
    if (rule.isNull())
        return BNC_ERR;

    ruleList.push_back(rule);

    INT1 outIpStr[20] = {0}, inIpStr[20] = {0};
    number2ip(htonl(rule->getOutsideIp()), outIpStr);
    number2ip(htonl(rule->getInsideIp()), inIpStr);
    UINT2 outPortStart = 0, outPortEnd = 0, inPortStart = 0, inPortEnd = 0;
    rule->getOutsidePort(outPortStart, outPortEnd);
    rule->getInsidePort(inPortStart, inPortEnd);
    
    LOG_INFO_FMT("rule list size[%lu] after enforced PORTFORWARD RULE: "
        "%s,protocol[%d],outIp[%s],inIp[%s],outPort[%u-%u],inPort[%u-%u],networkId[%s],subnetId[%s]",
        ruleList.size(), rule->getEnabled()?"enabled":"disabled", rule->getProtocol(),
        outIpStr, inIpStr, outPortStart, outPortEnd, inPortStart, inPortEnd, 
        rule->getNetworkId().c_str(), rule->getSubnetId().c_str());
    return BNC_OK;
}

void CPortforwardPolicy::unenforcePortforwardRule(CPFRuleList& ruleList, const CSmartPtr<CPortforwardRule>& rule)
{
    if (rule.isNull())
        return;

    STL_FOR_LOOP(ruleList, it)
    {
        if ((*it)->Compare(*rule))
        {
            ruleList.erase(it);
            break;
        }
    }

    INT1 outIpStr[20] = {0}, inIpStr[20] = {0};
    number2ip(htonl(rule->getOutsideIp()), outIpStr);
    number2ip(htonl(rule->getInsideIp()), inIpStr);
    UINT2 outPortStart = 0, outPortEnd = 0, inPortStart = 0, inPortEnd = 0;
    rule->getOutsidePort(outPortStart, outPortEnd);
    rule->getInsidePort(inPortStart, inPortEnd);
    
    LOG_INFO_FMT("rule list size[%lu] after unenforced PORTFORWARD RULE: "
        "%s,protocol[%d],outIp[%s],inIp[%s],outPort[%u-%u],inPort[%u-%u],networkId[%s],subnetId[%s]",
        ruleList.size(), rule->getEnabled()?"enabled":"disabled", rule->getProtocol(),
        outIpStr, inIpStr, outPortStart, outPortEnd, inPortStart, inPortEnd, 
        rule->getNetworkId().c_str(), rule->getSubnetId().c_str());
}

INT4 CPortforwardPolicy::installPortforwardEphemeralFlow(UINT4 outIP, UINT2 outPort, UINT1 protocol, UINT4 inIP, UINT2 inPort)
{
    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return BNC_ERR;

    INT1 inIpStr[20] = {0}, outIpStr[20] = {0};
    number2ip(htonl(inIP), inIpStr);
    number2ip(htonl(outIP), outIpStr);

    CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByIp(htonl(inIP));
	if (host.isNull())
	{
        LOG_WARN_FMT("Can not find CHost via host ip[%s] !", inIpStr);
		return BNC_ERR;
	}

    CSmartPtr<CSwitch> hostSw = host->getSw();
	if (hostSw.isNull())
	{
        LOG_WARN_FMT("Host with ip[%s] is not tracked !", inIpStr);
		return BNC_ERR;
	}
    
	Base_External* baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(htonl(inIP));
    if (NULL == baseExternalPort)
    {
        LOG_WARN_FMT("Can not find baseExternalPort via inIp[%s] !", inIpStr);
        return BNC_ERR;
    }

    if (0 == baseExternalPort->get_switch_DPID())
    {
        LOG_WARN("external switch is not recognized !");
        return BNC_ERR;
    }
	CSmartPtr<CSwitch> gwSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
    if (gwSw.isNull())
    {
        LOG_WARN_FMT("Can not find external switch via dpid[%llx] !", baseExternalPort->get_switch_DPID());
        return BNC_ERR;
    }

    if (hostSw != gwSw)
    {
        //traffic in
        //gwSw --> hostSw
        UINT4 outputPort = CControl::getInstance()->getTopoMgr().get_out_port_between_switch(gwSw, hostSw);        
        CFlowMgr::getInstance()->install_portforwardIn_ephemeral_flow(gwSw, outIP, outPort, protocol, 
            host->getMac(), 0, 0, hostSw->getTag(), outputPort);
        //hostSw --> host
        CFlowMgr::getInstance()->install_portforwardIn_ephemeral_flow(hostSw, outIP, outPort, protocol, 
            NULL, inIP, inPort, 0, host->getPortNo());
    
        //traffic out
        //hostSw --> gwSw
        outputPort = CControl::getInstance()->getTopoMgr().get_out_port_between_switch(hostSw, gwSw);        
        CFlowMgr::getInstance()->install_portforwardOut_ephemeral_flow(hostSw, inIP, inPort, protocol, 
            baseExternalPort->get_gateway_MAC(), outIP, outPort, gwSw->getTag(), outputPort);
        //gwSw output
        CFlowMgr::getInstance()->install_portforwardOut_external_flow(gwSw, outIP, protocol, outPort,
            baseExternalPort->get_switch_port(), OFPFC_ADD, TRUE);
    }
    else
    {
        //traffic in
        //gwSw --> host
        CFlowMgr::getInstance()->install_portforwardIn_ephemeral_flow(gwSw, outIP, outPort, protocol, 
            host->getMac(), inIP, inPort, 0, host->getPortNo());
    
        //traffic out
        //gwSw output
        CFlowMgr::getInstance()->install_portforwardOut_ephemeral_flow(hostSw, inIP, inPort, protocol, 
            baseExternalPort->get_gateway_MAC(), outIP, outPort, 0, baseExternalPort->get_switch_port());
    }

    return BNC_OK;
}


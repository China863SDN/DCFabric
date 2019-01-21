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
*   File Name   : CFirewallPolicy.cpp                                         *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CFirewallPolicy.h"
#include "log.h"
#include "bnc-type.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CSecurityGroupEvent.h"
#include "CHostMgr.h"
#include "CFlowMgr.h"
#include "openflow-common.h"
#include "bnc-inet.h"
#include "BaseExternalManager.h"
#include "CControl.h"
#include "CClusterService.h"
#include "CConf.h"
#include "CFloatingIp.h"
#include "CFloatingIpMgr.h"
#include "CRouterGateMgr.h"
#include "CPortforwardPolicy.h"

extern const char* g_securityGroupEventPath[];

CFirewallPolicy* CFirewallPolicy::m_instance = NULL;      

CFirewallPolicy* CFirewallPolicy::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CFirewallPolicy();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return m_instance;
}

void CFirewallPolicy::firewallGuard(void* param)
{
#if 0
    const char* testip = "192.168.52.165";
    UINT4 nip = ip2number(testip);
    UINT4 hip = ntohl(nip);

    const char* testprefix = "192.168.52.0";
    UINT4 nprefix = ip2number(testprefix);
    UINT4 hprefix = ntohl(nprefix);

    LOG_WARN_FMT("TEST: testip[%s]nip[0x%x]hip[0x%x], testprefix[%s]nprefix[0x%x]hprefix[0x%x]",
        testip, nip, hip, testprefix, nprefix, hprefix);

    UINT4 nip1 = nip >> 8;
    nip1 <<= 8;
    UINT4 hip1 = hip >> 8;
    hip1 <<= 8;
    LOG_WARN_FMT("TEST: nip>>8<<8[0x%x] %s nprefix[0x%x], hip>>8<<8[0x%x] %s hprefix[0x%x]",
        nip1, (nip1==nprefix)?"==":"!=", nprefix, hip1, (hip1==hprefix)?"==":"!=", hprefix);
#endif

    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return;

    CPolicyHMap& policy = CFirewallPolicy::getInstance()->getPolicy();
    STL_FOR_LOOP(policy, macIt)
    {
        const std::string& macAddress = macIt->first;
        if (CFirewallPolicy::getInstance()->getEnfored(macAddress))
            continue;

        UINT1 mac[6] = {0};
        macstr2hex(macAddress.c_str(), mac);

        CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(mac);
        if (host.isNull() || host->getSw().isNull())
            continue;
        
        CRemoteRuleListHMap& listHMap = macIt->second;
        STL_FOR_LOOP(listHMap, listIt)
        {
            CFWRuleList& ruleList = listIt->second;
            STL_FOR_LOOP(ruleList, ruleIt)
            {
                CSmartPtr<CFirewallRule>& firewallRule = *ruleIt;

                INT1 hostIpStr[20] = {0}, remoteIpStr[20] = {0};
                number2ip(host->getfixIp(), hostIpStr);
                number2ip(htonl(firewallRule->getRemoteIp()), remoteIpStr);
                LOG_WARN_FMT(">>> host[%s][%s] apply FIREWALL-%s RULE[%s]: "
                    "enabled[%s]direction[%s]protocol[%s]remoteIp[%s/%u]port[%u-%u]priority[%u]action[%s]",
                    macAddress.c_str(), hostIpStr, 
                    (firewallRule->getDirection().compare(FIREWALL_DIRECTION_IN)==0)?"IN":"OUT",
                    firewallRule->getRuleId().c_str(), firewallRule->getEnabled()?"true":"false",
                    firewallRule->getDirection().c_str(), 
                    (firewallRule->getProtocol()==IP_ICMP)?"ICMP":
                    (firewallRule->getProtocol()==IP_TCP)?"TCP":
                    (firewallRule->getProtocol()==IP_UDP)?"UDP":
                    (firewallRule->getProtocol()==IP_VRRP)?"VRRP":"NONE",
                    remoteIpStr, firewallRule->getRemoteIpMask(), firewallRule->getPortMin(), 
                    firewallRule->getPortMax(), firewallRule->getPriority(),
                    firewallRule->getAction().c_str());

                if (BNC_OK == CFirewallPolicy::getInstance()->installFirewallFlow(macAddress, firewallRule, OFPFC_ADD))
                    CFirewallPolicy::getInstance()->setEnfored(macAddress, TRUE);
            }
        }
    }
}

CFirewallPolicy::CFirewallPolicy():
    m_groupHostMap(hash_bucket_number),
    m_hostGroupMap(hash_bucket_number),
    m_groups(hash_bucket_number),
    m_policy(hash_bucket_number),
    m_enforced(hash_bucket_number)
{
}

CFirewallPolicy::~CFirewallPolicy()
{
    deregister();
}

INT4 CFirewallPolicy::init()
{
	if (onregister())
    {
        LOG_WARN_FMT("CFirewallPolicy register failed !");
        return BNC_ERR;
    }

    if (!CConf::getInstance()->isSecurityGroupOn())
        return BNC_OK;

    const INT1* pConf = CConf::getInstance()->getConfig("openstack", "reload_interval");
    UINT4 interval = (pConf == NULL) ? 0 : atol(pConf);
    LOG_INFO_FMT("openstack reload_interval %us", interval);

    if (0 == interval)
        return BNC_OK;

    if (m_timer.schedule(interval, interval, CFirewallPolicy::firewallGuard, this) != BNC_OK)
    {
        LOG_ERROR("CFirewallPolicy schedule firewallGuard failed !");
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CFirewallPolicy::onregister()
{
    CMsgPath path(g_securityGroupEventPath[0]);
    return CEventNotifier::onregister(path);
}

void CFirewallPolicy::deregister()
{
    CMsgPath path(g_securityGroupEventPath[0]);
    CEventNotifier::deregister(path);
}

INT4 CFirewallPolicy::consume(CSmartPtr<CMsgCommon> evt)
{
    INT4 ret = BNC_ERR;
    
    if (evt.isNull())
        return ret;

    CEvent* pEvt = (CEvent*)evt.getPtr();
    //LOG_WARN(pEvt->getDesc().c_str());

    switch (pEvt->getEvent())
    {
        case EVENT_TYPE_SECURITY_GROUP_ATTACH:
        {
            LOG_DEBUG("EVENT_TYPE_SECURITY_GROUP_ATTACH");
            CSecurityGroupEvent* pGroupEvt = (CSecurityGroupEvent*)pEvt;
            ret = attachSecurityGroup(pGroupEvt->getMacAddress(), pGroupEvt->getSecurityGroupId());
            break;
        }
        case EVENT_TYPE_SECURITY_GROUP_DETACH:
        {
            LOG_DEBUG("EVENT_TYPE_SECURITY_GROUP_DETACH");
            CSecurityGroupEvent* pGroupEvt = (CSecurityGroupEvent*)pEvt;
            detachSecurityGroup(pGroupEvt->getMacAddress(), pGroupEvt->getSecurityGroupId());
            ret = BNC_OK;
            break;
        }
        case EVENT_TYPE_SECURITY_GROUP_RULE_C:
        {
            LOG_DEBUG("EVENT_TYPE_SECURITY_GROUP_RULE_C");
            CSecurityGroupRuleEvent* pRuleEvt = (CSecurityGroupRuleEvent*)pEvt;
            ret = createSecurityGroupRule(pRuleEvt->getSecurityGroupRule());
            break;
        }
        case EVENT_TYPE_SECURITY_GROUP_RULE_U:
        {
            LOG_DEBUG("EVENT_TYPE_SECURITY_GROUP_RULE_U");
            CSecurityGroupRuleEvent* pRuleEvt = (CSecurityGroupRuleEvent*)pEvt;
            ret = updateSecurityGroupRule(pRuleEvt->getSecurityGroupRule());
            break;
        }
        case EVENT_TYPE_SECURITY_GROUP_RULE_D:
        {
            LOG_DEBUG("EVENT_TYPE_SECURITY_GROUP_RULE_D");
            CSecurityGroupRuleEvent* pRuleEvt = (CSecurityGroupRuleEvent*)pEvt;
            deleteSecurityGroupRule(pRuleEvt->getSecurityGroupRule());
            ret = BNC_OK;
            break;
        }
        default:
            LOG_WARN_FMT("%s consume invalid event[%d] !", toString(), pEvt->getEvent());
            break;
    }

    return ret;
}

INT4 CFirewallPolicy::apply(const UINT1* mac)
{
    if (!CConf::getInstance()->isSecurityGroupOn())
        return BNC_ERR;

    if (NULL == mac)
        return BNC_ERR;

    CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(mac);
    if (host.isNull() || host->getSw().isNull())
        return BNC_ERR;
    
    INT1 macStr[20] = {0};
    mac2str(mac, macStr);
    std::string macAddress(macStr);

    //m_rwlock.rlock();

    CRemoteRuleListHMap* remoteRuleListHMap = findRemoteRuleListMap(macAddress);
    if (NULL != remoteRuleListHMap)
    {
        STL_FOR_LOOP(*remoteRuleListHMap, listIt)
        {
            STL_FOR_LOOP(listIt->second, ruleIt)
            {
                CSmartPtr<CFirewallRule>& firewallRule = *ruleIt;

                INT1 hostIpStr[20] = {0}, remoteIpStr[20] = {0};
                number2ip(host->getfixIp(), hostIpStr);
                number2ip(htonl(firewallRule->getRemoteIp()), remoteIpStr);
                LOG_WARN_FMT(">>> host[%s][%s] apply FIREWALL-%s RULE[%s]: "
                    "direction[%s]protocol[%s]remoteIp[%s/%u]port[%u-%u]priority[%u]action[%s]",
                    macAddress.c_str(), hostIpStr, 
                    (firewallRule->getDirection().compare(FIREWALL_DIRECTION_IN)==0)?"IN":"OUT",
                    firewallRule->getRuleId().c_str(), firewallRule->getDirection().c_str(), 
                    (firewallRule->getProtocol()==IP_ICMP)?"ICMP":
                    (firewallRule->getProtocol()==IP_TCP)?"TCP":
                    (firewallRule->getProtocol()==IP_UDP)?"UDP":
                    (firewallRule->getProtocol()==IP_VRRP)?"VRRP":"NONE",
                    remoteIpStr, firewallRule->getRemoteIpMask(), firewallRule->getPortMin(), 
                    firewallRule->getPortMax(), firewallRule->getPriority(),
                    firewallRule->getAction().c_str());

                if (installFirewallFlow(macAddress, firewallRule, OFPFC_ADD) == BNC_OK)
                    setEnfored(macAddress, TRUE);
            }
        }
    }

    //m_rwlock.unlock();

    return BNC_OK;
}

INT4 CFirewallPolicy::installFirewallFlow(const std::string& macAddress, const CSmartPtr<CFirewallRule>& firewallRule, UINT1 command)
{
    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return BNC_ERR;

	if (macAddress.empty() || firewallRule.isNull() || !firewallRule->getEnabled())
		return BNC_ERR;

    UINT1 mac[6] = {0};
    macstr2hex(macAddress.c_str(), mac);

    CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(mac);
	if (host.isNull())
	{
        LOG_WARN_FMT("Can not find host via mac[%s] !", macAddress.c_str());
		return BNC_ERR;
	}

    CSmartPtr<CSwitch> sw = host->getSw();
	if (sw.isNull())
	{
        LOG_WARN_FMT("Host with mac[%s] is not tracked !", macAddress.c_str());
		return BNC_ERR;
	}
    
    UINT4 innerIp = ntohl(host->getfixIp());
    UINT4 outerIp = 0;

	CFloatingIp* floatingIp = CFloatingIpMgr::getInstance()->findFloatingIpNodeByfixedIp(htonl(innerIp));
	if (NULL != floatingIp)
    {
        outerIp = ntohl(floatingIp->getFloatingIp());
        LOG_WARN_FMT("innerIp[0x%x] ==> floatingIp[0x%x] !", innerIp, outerIp);
    }
    else
    {   
    	CRouter* router = CRouterGateMgr::getInstance()->FindRouterNodeByHostMac(mac);
    	if (NULL == router)
    	{
            LOG_WARN_FMT("Can not find CRouter via host mac[%s] !", macAddress.c_str());
    		return BNC_ERR;
    	}
        outerIp = ntohl(router->getRouterIp());
        LOG_WARN_FMT("innerIp[0x%x] ==> routerIp[0x%x] !", innerIp, outerIp);
    }

	Base_External* baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(htonl(innerIp));
    if (NULL == baseExternalPort)
    {
        INT1 ipStr[20] = {0};
        number2ip(host->getfixIp(), ipStr);
        LOG_WARN_FMT("Can not find baseExternalPort via fixedIp[%s] !", ipStr);
        return BNC_ERR;
    }

    if (0 == baseExternalPort->get_switch_DPID())
    {
        LOG_WARN("external switch is not recognized !");
        return BNC_ERR;
    }
	CSmartPtr<CSwitch> extSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
    if (extSw.isNull())
    {
        LOG_WARN_FMT("Can not find external switch via dpid[%llx] !", baseExternalPort->get_switch_DPID());
        return BNC_ERR;
    }
    
    BOOL accept = (firewallRule->getAction().compare(FIREWALL_ACTION_ACCEPT) == 0);

	if (firewallRule->getDirection().compare(FIREWALL_DIRECTION_IN) == 0)
    {
        if (IP_ICMP == firewallRule->getProtocol())
        {
            CFlowMgr::getInstance()->install_firewallIn_withPort_flow(extSw, firewallRule->getRemoteIp(), outerIp, 
                firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPortMin(), 
                firewallRule->getPortMax(), firewallRule->getPriority(), command, accept);
            if (sw != extSw)
            {
                CFlowMgr::getInstance()->install_firewallIn_withPort_flow(sw, firewallRule->getRemoteIp(), outerIp, 
                    firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPortMin(), 
                    firewallRule->getPortMax(), firewallRule->getPriority(), command, accept);
            }
        }
        else
        {
            if (firewallRule->getPortMin() == firewallRule->getPortMax())
            {
                if (firewallRule->getPortMin() == 0)
                {
                    CFlowMgr::getInstance()->install_firewallIn_withoutPort_flow(extSw, firewallRule->getRemoteIp(), outerIp, 
                        firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    if (sw != extSw)
                    {
                        CFlowMgr::getInstance()->install_firewallIn_withoutPort_flow(sw, firewallRule->getRemoteIp(), outerIp, 
                            firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    }
                }
                else
                {
                    CFlowMgr::getInstance()->install_firewallIn_withPort_flow(extSw, firewallRule->getRemoteIp(), outerIp, 
                        firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPortMin(), 
                        firewallRule->getPortMin(), firewallRule->getPriority(), command, accept);
                    if (sw != extSw)
                    {
                        CFlowMgr::getInstance()->install_firewallIn_withPort_flow(sw, firewallRule->getRemoteIp(), outerIp, 
                            firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPortMin(), 
                            firewallRule->getPortMin(), firewallRule->getPriority(), command, accept);
                    }
                }
            }
            else if (firewallRule->getPortMin() < firewallRule->getPortMax())
            {
                if ((firewallRule->getPortMax() == 65535) && (firewallRule->getPortMin() <= 1))
                {
                    CFlowMgr::getInstance()->install_firewallIn_withoutPort_flow(extSw, firewallRule->getRemoteIp(), outerIp, 
                        firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    if (sw != extSw)
                    {
                        CFlowMgr::getInstance()->install_firewallIn_withoutPort_flow(sw, firewallRule->getRemoteIp(), outerIp, 
                            firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    }
                }
                else
                {
                    CFlowMgr::getInstance()->install_firewallIn_gotoController_flow(extSw, firewallRule->getRemoteIp(), outerIp, 
                        firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
#if 0
                    if (sw != extSw)
                    {
                        CFlowMgr::getInstance()->install_firewallIn_gotoController_flow(sw, firewallRule->getRemoteIp(), outerIp, 
                            firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), command, accept);
                    }
#endif
                }
            }        
        }
    }
    else if (firewallRule->getDirection().compare(FIREWALL_DIRECTION_OUT) == 0)
    {
        if (IP_ICMP == firewallRule->getProtocol())
        {
            CFlowMgr::getInstance()->install_firewallOut_withPort_flow(sw, innerIp, firewallRule->getRemoteIp(), 
                firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPortMin(), 
                firewallRule->getPortMax(), firewallRule->getPriority(), command, accept);
            if (sw != extSw)
            {
                CFlowMgr::getInstance()->install_firewallOut_withPort_flow(extSw, outerIp, firewallRule->getRemoteIp(), 
                    firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPortMin(), 
                    firewallRule->getPortMax(), firewallRule->getPriority(), command, accept);
            }
        }
        else
        {
            if (firewallRule->getPortMin() == firewallRule->getPortMax())
            {
                if (firewallRule->getPortMin() == 0)
                {
                    CFlowMgr::getInstance()->install_firewallOut_withoutPort_flow(sw, innerIp, firewallRule->getRemoteIp(), 
                        firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    if (sw != extSw)
                    {
                        CFlowMgr::getInstance()->install_firewallOut_withoutPort_flow(extSw, outerIp, firewallRule->getRemoteIp(), 
                            firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    }
                }
                else
                {
                    CFlowMgr::getInstance()->install_firewallOut_withPort_flow(sw, innerIp, firewallRule->getRemoteIp(), 
                        firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPortMin(), 
                        firewallRule->getPortMin(), firewallRule->getPriority(), command, accept);
                    if (sw != extSw)
                    {
                        CFlowMgr::getInstance()->install_firewallOut_withPort_flow(extSw, outerIp, firewallRule->getRemoteIp(), 
                            firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPortMin(), 
                            firewallRule->getPortMin(), firewallRule->getPriority(), command, accept);
                    }
                }
            }
            else if (firewallRule->getPortMin() < firewallRule->getPortMax())
            {
                if ((firewallRule->getPortMax() == 65535) && (firewallRule->getPortMin() <= 1))
                {
                    CFlowMgr::getInstance()->install_firewallOut_withoutPort_flow(sw, innerIp, firewallRule->getRemoteIp(), 
                        firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    if (sw != extSw)
                    {
                        CFlowMgr::getInstance()->install_firewallOut_withoutPort_flow(extSw, outerIp, firewallRule->getRemoteIp(), 
                            firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    }
                }
                else
                {
                    CFlowMgr::getInstance()->install_firewallOut_gotoController_flow(sw, innerIp, firewallRule->getRemoteIp(), 
                        firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
#if 0
                    if (sw != extSw)
                    {
                        CFlowMgr::getInstance()->install_firewallOut_withoutPort_flow(extSw, outerIp, firewallRule->getRemoteIp(), 
                            firewallRule->getRemoteIpMask(), firewallRule->getProtocol(), firewallRule->getPriority(), command, accept);
                    }
#endif
                }
            }
        }
    }

    return BNC_OK;
}

INT4 CFirewallPolicy::applyOut(const CSmartPtr<CSwitch>& sw, const UINT1* srcMac, UINT4 srcIp, UINT4 dstIp, UINT1 protocol, UINT2 srcPort, UINT2 dstPort)
{
    if (sw.isNull() || (NULL == srcMac))
        return BNC_ERR;

    INT1 macStr[20] = {0};
    mac2str(srcMac, macStr);
    std::string macAddress(macStr);

    INT1 hostIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(srcIp), hostIpStr);
    number2ip(htonl(dstIp), dstIpStr);
    LOG_WARN_FMT(">>> host[%s][%s:%u] ==> remote[%s:%u], protocol[%u] ...",
        macAddress.c_str(), hostIpStr, srcPort, dstIpStr, dstPort, protocol);

    CRemoteRuleListHMap* remoteRuleListHMap = findRemoteRuleListMap(macAddress);
    if (NULL != remoteRuleListHMap)
    {
        STL_FOR_LOOP(*remoteRuleListHMap, listIt)
        {
            STL_FOR_LOOP(listIt->second, ruleIt)
            {
                CSmartPtr<CFirewallRule>& firewallRule = *ruleIt;
                if (firewallRule.isNull() || !firewallRule->getEnabled())
                    continue;

                if (0 != firewallRule->getDirection().compare(FIREWALL_DIRECTION_OUT))
                    continue;
                if (firewallRule->getProtocol() > 0)
                {
                    if (protocol != firewallRule->getProtocol())
                        continue;
                }
                if (firewallRule->getRemoteIp() > 0)
                {
                    UINT4 rmtIp = dstIp;
                    rmtIp >>= (32 - firewallRule->getRemoteIpMask());
                    rmtIp <<= (32 - firewallRule->getRemoteIpMask());
                    if (rmtIp != firewallRule->getRemoteIp())
                        continue;
                }
                if (firewallRule->getPortMin() > 0)
                {
                    if (srcPort < firewallRule->getPortMin())
                        continue;
                }
                if (firewallRule->getPortMax() > 0)
                {
                    if (srcPort > firewallRule->getPortMax())
                        continue;
                }

                INT1 remoteIpStr[20] = {0};
                number2ip(htonl(firewallRule->getRemoteIp()), remoteIpStr);
                LOG_WARN_FMT(">>> host[%s][%s:%u] ==> remote[%s:%u] apply FIREWALL-OUT RULE[%s]: "
                    "direction[%s]protocol[%s]remoteIp[%s/%u]port[%u-%u]priority[%u]action[%s]",
                    macAddress.c_str(), hostIpStr, srcPort, dstIpStr, dstPort,
                    firewallRule->getRuleId().c_str(), firewallRule->getDirection().c_str(), 
                    (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
                    remoteIpStr, firewallRule->getRemoteIpMask(), firewallRule->getPortMin(), 
                    firewallRule->getPortMax(), firewallRule->getPriority(), firewallRule->getAction().c_str());

                if (firewallRule->getPortMin() == firewallRule->getPortMax())
                    return BNC_OK;
                else if ((firewallRule->getPortMax() == 65535) && (firewallRule->getPortMin() <= 1))
                    return BNC_OK;
                else
                    return installFirewallOutEphemeralFlow(macAddress, srcIp, dstIp, protocol, srcPort, dstPort, 
                        firewallRule->getPriority(), (0==firewallRule->getAction().compare(FIREWALL_ACTION_ACCEPT)));
            }
        }
    }

    LOG_WARN_FMT("<<< dropped: host[%s][%s:%u] ==> remote[%s:%u], protocol[%u] !!!",
        macAddress.c_str(), hostIpStr, srcPort, dstIpStr, dstPort, protocol);
    return BNC_ERR;
}

INT4 CFirewallPolicy::applyIn(const CSmartPtr<CSwitch>& sw, UINT4 srcIp, UINT4 dstIp, UINT1 protocol, UINT2 srcPort, UINT2 dstPort)
{
    if (sw.isNull())
        return BNC_ERR;

    INT1 srcIpStr[20] = {0}, dstIpStr[20] = {0};
    number2ip(htonl(srcIp), srcIpStr);
    number2ip(htonl(dstIp), dstIpStr);
    LOG_WARN_FMT(">>> remote[%s:%u] ==> local[%s:%u], protocol[%u] ...",
        srcIpStr, srcPort, dstIpStr, dstPort, protocol);

    INT1 innerIpStr[20] = {0};
    UINT4 innerIp = 0;
    UINT2 innerPort = 0;

	CFloatingIp* floatingIp = CFloatingIpMgr::getInstance()->findFloatingIpNodeByfloatingIp(htonl(dstIp));
	if (NULL != floatingIp)
    {
        innerIp = ntohl(floatingIp->getFixedIp());
        innerPort = dstPort;

        number2ip(htonl(innerIp), innerIpStr);
        LOG_WARN_FMT("floatingIp[%s:%u] ==> innerIp[%s:%u]", dstIpStr, dstPort, innerIpStr, innerPort);
    }
    else
    {
        UINT4 inIp = 0;
        UINT2 inPort = 0;
        if (CPortforwardPolicy::getInstance()->checkIn(dstIp, dstPort, protocol, inIp, inPort))
        {
            innerIp = inIp;
            innerPort = inPort;

            number2ip(htonl(innerIp), innerIpStr);
            LOG_WARN_FMT("portforwardIp[%s:%u] ==> innerIp[%s:%u], protocol[%u]", dstIpStr, dstPort, innerIpStr, innerPort, protocol);
        }
    }

    if (innerIp == 0)
    {
        LOG_WARN_FMT("traffic in: none floating or portforward dstIp[%s] !", dstIpStr);
        return BNC_ERR;
    }

    CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByIp(htonl(innerIp));
	if (host.isNull())
	{
        LOG_WARN_FMT("Can not find CHost via host ip[%s] !", innerIpStr);
		return BNC_ERR;
	}

    INT1 macStr[20] = {0};
    mac2str(host->getMac(), macStr);
    std::string macAddress(macStr);

    CRemoteRuleListHMap* remoteRuleListHMap = findRemoteRuleListMap(macAddress);
    if (NULL != remoteRuleListHMap)
    {
        STL_FOR_LOOP(*remoteRuleListHMap, listIt)
        {
            STL_FOR_LOOP(listIt->second, ruleIt)
            {
                CSmartPtr<CFirewallRule>& firewallRule = *ruleIt;
                if (firewallRule.isNull() || !firewallRule->getEnabled())
                    continue;

                if (0 != firewallRule->getDirection().compare(FIREWALL_DIRECTION_IN))
                    continue;
                if (firewallRule->getProtocol() > 0)
                {
                    if (protocol != firewallRule->getProtocol())
                        continue;
                }
                if (firewallRule->getRemoteIp() > 0)
                {
                    UINT4 rmtIp = srcIp;
                    rmtIp >>= (32 - firewallRule->getRemoteIpMask());
                    rmtIp <<= (32 - firewallRule->getRemoteIpMask());
                    if (rmtIp != firewallRule->getRemoteIp())
                        continue;
                }
                if (firewallRule->getPortMin() > 0)
                {
                    if (innerPort < firewallRule->getPortMin())
                        continue;
                }
                if (firewallRule->getPortMax() > 0)
                {
                    if (innerPort > firewallRule->getPortMax())
                        continue;
                }

                INT1 remoteIpStr[20] = {0};
                number2ip(htonl(firewallRule->getRemoteIp()), remoteIpStr);
                LOG_WARN_FMT(">>> host[%s][%s:%u] ==> remote[%s:%u] apply FIREWALL-OUT RULE[%s]: "
                    "direction[%s]protocol[%s]remoteIp[%s/%u]port[%u-%u]priority[%u]action[%s]",
                    macAddress.c_str(), innerIpStr, innerPort, srcIpStr, srcPort,
                    firewallRule->getRuleId().c_str(), firewallRule->getDirection().c_str(), 
                    (IP_ICMP==protocol)?"ICMP":(IP_TCP==protocol)?"TCP":(IP_UDP==protocol)?"UDP":(IP_VRRP==protocol)?"VRRP":"NONE",
                    remoteIpStr, firewallRule->getRemoteIpMask(), firewallRule->getPortMin(), 
                    firewallRule->getPortMax(), firewallRule->getPriority(), firewallRule->getAction().c_str());

                if (firewallRule->getPortMin() == firewallRule->getPortMax())
                    return BNC_OK;
                else if ((firewallRule->getPortMax() == 65535) && (firewallRule->getPortMin() <= 1))
                    return BNC_OK;
                else
                    return installFirewallInEphemeralFlow(macAddress, srcIp, dstIp, protocol, srcPort, dstPort, 
                        firewallRule->getPriority(), (0==firewallRule->getAction().compare(FIREWALL_ACTION_ACCEPT)));
            }
        }
    }

    LOG_WARN_FMT("<<< dropped: remote[%s:%u] ==> local[%s:%u], protocol[%u] !!!",
        srcIpStr, srcPort, dstIpStr, dstPort, protocol);
    return BNC_ERR;
}

BOOL CFirewallPolicy::getEnfored(const std::string& macAddress)
{
    BOOL enforced = FALSE;

    CFWEnforcedHMap::CPair* item = NULL;
    if (m_enforced.find(macAddress, &item))
        enforced = item->second;

    return enforced;
}

void CFirewallPolicy::setEnfored(const std::string& macAddress, BOOL enforced)
{
    CFWEnforcedHMap::CPair* item = NULL;
    if (m_enforced.find(macAddress, &item))
    {
        item->second = enforced;
    }
    else
    {
        m_enforced.insert(macAddress, enforced);
    }
}

void CFirewallPolicy::dump()
{
    STL_FOR_LOOP(m_policy, policyIt)
    {
        STL_FOR_LOOP(policyIt->second, listIt)
        {
            STL_FOR_LOOP(listIt->second, ruleIt)
            {
                CSmartPtr<CFirewallRule>& firewallRule = *ruleIt;
                INT1 remoteIpStr[20] = {0};
                number2ip(htonl(firewallRule->getRemoteIp()), remoteIpStr);
                LOG_WARN_FMT("###DUMP### host[%s] enforced FIREWALL RULE[%s]: "
                    "direction[%s]protocol[%s]remoteIp[%s/%u]port[%u-%u]priority[%u]",
                    policyIt->first.c_str(), firewallRule->getRuleId().c_str(), 
                    firewallRule->getDirection().c_str(), 
                    (firewallRule->getProtocol()==IP_ICMP)?"ICMP":
                    (firewallRule->getProtocol()==IP_TCP)?"TCP":
                    (firewallRule->getProtocol()==IP_UDP)?"UDP":
                    (firewallRule->getProtocol()==IP_VRRP)?"VRRP":"NONE",
                    remoteIpStr, firewallRule->getRemoteIpMask(),
                    firewallRule->getPortMin(), firewallRule->getPortMax(), firewallRule->getPriority());
            }
        }
    }
}

INT4 CFirewallPolicy::attachSecurityGroup(const std::string& macAddress, const std::string& groupId)
{
    INT4 ret = BNC_ERR;

    createGroupHostListMapping(groupId, macAddress);
    createHostGroupListMapping(macAddress, groupId);

    CRuleHMap* groupRuleHMap = findGroupRuleMap(groupId);
    if (NULL != groupRuleHMap)
    {
        ret = enforceGroupRuleMap(macAddress, *groupRuleHMap);
    }
    else
    {
        if (createGroupRuleMap(groupId) != NULL)
            ret = BNC_OK;
    }

    //dump();

    return ret;
}

void CFirewallPolicy::detachSecurityGroup(const std::string& macAddress, const std::string& groupId)
{
    deleteGroupHostListMapping(groupId, macAddress);
    deleteHostGroupListMapping(macAddress, groupId);

    CRuleHMap* groupRuleHMap = findGroupRuleMap(groupId);
    if (NULL != groupRuleHMap)
        unenforceGroupRuleMap(macAddress, *groupRuleHMap);

    //dump();
}

INT4 CFirewallPolicy::createSecurityGroupRule(const CFirewallRule& groupRule)
{
    CRuleHMap* groupRuleHMap = findGroupRuleMap(groupRule.getGroupId());
    if (NULL == groupRuleHMap)
    {
        groupRuleHMap = createGroupRuleMap(groupRule.getGroupId());
        if (NULL == groupRuleHMap)
            return BNC_ERR;
    }

    CSmartPtr<CFirewallRule> groupRuleUpdate = updateGroupRule(*groupRuleHMap, groupRule);
    if (groupRuleUpdate.isNotNull())
    {
        CHostMacList* hostList = findHostList(groupRule.getGroupId());
        if (NULL != hostList)
        {
            STL_FOR_LOOP(*hostList, it)
            {
                if (enforceGroupRule(*it, groupRuleUpdate) != BNC_OK)
                    return BNC_ERR;
            }
        }
    }

    //dump();

    return BNC_OK;
}

INT4 CFirewallPolicy::updateSecurityGroupRule(const CFirewallRule& groupRule)
{
    return createSecurityGroupRule(groupRule);
}

void CFirewallPolicy::deleteSecurityGroupRule(const CFirewallRule& groupRule)
{
    CSmartPtr<CFirewallRule> groupRuleDelete(NULL);
    
    CRuleHMap* groupRuleHMap = findGroupRuleMap(groupRule.getGroupId());
    if (NULL == groupRuleHMap)
        return;

    groupRuleDelete = findGroupRule(*groupRuleHMap, groupRule.getRuleId());
    if (groupRuleDelete.isNull())
        return;

    CHostMacList* hostList = findHostList(groupRule.getGroupId());
    if (NULL != hostList)
    {
        STL_FOR_LOOP(*hostList, it)
        {
            unenforceGroupRule(*it, groupRuleDelete);
        }
    }

    deleteGroupRule(*groupRuleHMap, groupRule.getRuleId());

    //dump();
}

CHostMacList* CFirewallPolicy::findHostList(const std::string& groupId)
{
    CGroupHostListHMap::CPair* item = NULL;
    if (m_groupHostMap.find(groupId, &item))
        return &item->second;

    return NULL;
}

INT4 CFirewallPolicy::createGroupHostListMapping(const std::string& groupId, const std::string& macAddress)
{
    CGroupHostListHMap::CPair* item = NULL;
    if (!m_groupHostMap.find(groupId, &item))
        m_groupHostMap.insert(groupId, CHostMacList(), &item);

    if (NULL != item)
    {
        item->second.push_back(macAddress);
        LOG_INFO_FMT("after created, group[%s] has host size[%lu]", groupId.c_str(), item->second.size());
        return BNC_OK;
    }

    return BNC_ERR;
}

void CFirewallPolicy::deleteGroupHostListMapping(const std::string& groupId, const std::string& macAddress)
{
    CGroupHostListHMap::CPair* item = NULL;
    if (m_groupHostMap.find(groupId, &item))
        item->second.remove(macAddress);
}

CGroupList* CFirewallPolicy::findGroupList(const std::string& macAddress)
{
    CHostGroupListHMap::CPair* item = NULL;
    if (m_hostGroupMap.find(macAddress, &item))
        return &item->second;

    return NULL;
}

INT4 CFirewallPolicy::createHostGroupListMapping(const std::string& macAddress, const std::string& groupId)
{
    CHostGroupListHMap::CPair* item = NULL;
    if (!m_hostGroupMap.find(macAddress, &item))
        m_hostGroupMap.insert(macAddress, CGroupList(), &item);

    if (NULL != item)
    {
        item->second.push_back(groupId);
        LOG_INFO_FMT("after created, host[%s] has group size[%lu]", macAddress.c_str(), item->second.size());
        return BNC_OK;
    }

    return BNC_ERR;
}

void CFirewallPolicy::deleteHostGroupListMapping(const std::string& macAddress, const std::string& groupId)
{
    CHostGroupListHMap::CPair* item = NULL;
    if (m_hostGroupMap.find(macAddress, &item))
        item->second.remove(groupId);
}

CRuleHMap* CFirewallPolicy::findGroupRuleMap(const std::string& groupId)
{
    CGroupHMap::CPair* item = NULL;
    if (m_groups.find(groupId, &item))
        return &item->second;

    return NULL;
}

CRuleHMap* CFirewallPolicy::createGroupRuleMap(const std::string& groupId)
{
    CGroupHMap::CPair* item = NULL;
    if (m_groups.insert(groupId, CRuleHMap(hash_bucket_number), &item))
        return &item->second;

    return NULL;
}

void CFirewallPolicy::deleteGroupRuleMap(const std::string& groupId)
{
    m_groups.erase(groupId);
}

CSmartPtr<CFirewallRule> CFirewallPolicy::findGroupRule(CRuleHMap& groupRuleMap, const std::string& groupRuleId)
{
    CSmartPtr<CFirewallRule> groupRuleFind(NULL);

    CRuleHMap::CPair* item = NULL;
    if (groupRuleMap.find(groupRuleId, &item))
        groupRuleFind = item->second;

    return groupRuleFind;
}

CSmartPtr<CFirewallRule> CFirewallPolicy::updateGroupRule(CRuleHMap& groupRuleMap, const CFirewallRule& groupRule)
{
    CFirewallRule* groupRuleNew = new CFirewallRule(groupRule);
    if (NULL == groupRuleNew)
    {
        LOG_ERROR_FMT("%s new CFirewallRule failed !", toString());
        return CSmartPtr<CFirewallRule>(NULL);
    }
    CSmartPtr<CFirewallRule> groupRuleUpdate(groupRuleNew);

    groupRuleMap.erase(groupRule.getRuleId());
    groupRuleMap.insert(groupRule.getRuleId(), groupRuleUpdate);

    LOG_INFO_FMT("after updated rule[%s], group[%s] has rule size[%lu]", 
        groupRule.getRuleId().c_str(), groupRule.getGroupId().c_str(), groupRuleMap.size());
    return groupRuleUpdate;
}

void CFirewallPolicy::deleteGroupRule(CRuleHMap& groupRuleMap, const std::string& groupRuleId)
{
    groupRuleMap.erase(groupRuleId);
}

CRemoteRuleListHMap* CFirewallPolicy::findRemoteRuleListMap(const std::string& macAddress)
{
    CPolicyHMap::CPair* item = NULL;
    if (m_policy.find(macAddress, &item))
        return &item->second;

    return NULL;
}

CRemoteRuleListHMap* CFirewallPolicy::createRemoteRuleListMap(const std::string& macAddress)
{
    CPolicyHMap::CPair* item = NULL;
    if (m_policy.insert(macAddress, CRemoteRuleListHMap(hash_bucket_number), &item))
        return &item->second;

    return NULL;
}

CFWRuleList* CFirewallPolicy::findRemoteRuleList(CRemoteRuleListHMap& remoteRuleListMap, UINT4 ip)
{
    CRemoteRuleListHMap::CPair* item = NULL;
    if (remoteRuleListMap.find(ip, &item))
        return &item->second;

    return NULL;
}

CFWRuleList* CFirewallPolicy::createRemoteRuleList(CRemoteRuleListHMap& remoteRuleListMap, UINT4 ip)
{
    CRemoteRuleListHMap::CPair* item = NULL;
    if (remoteRuleListMap.insert(ip, CFWRuleList(), &item))
        return &item->second;

    return NULL;
}

INT4 CFirewallPolicy::enforceGroupRuleMap(const std::string& macAddress, CRuleHMap& groupRuleMap)
{
    INT4 ret = BNC_ERR;

    if (groupRuleMap.empty())
        return ret;

    STL_FOR_LOOP(groupRuleMap, it)
    {
        const CSmartPtr<CFirewallRule>& groupRule = it->second;
        if (groupRule.isNotNull())
            ret = enforceGroupRule(macAddress, groupRule);
            if (BNC_OK != ret)
                break;
    }

    return ret;
}

INT4 CFirewallPolicy::enforceGroupRule(const std::string& macAddress, const CSmartPtr<CFirewallRule>& groupRule)
{
    INT4 ret = BNC_ERR;

    if (groupRule.isNull())
        return ret;

    m_rwlock.wlock();

    CRemoteRuleListHMap* remoteRuleListHMap = findRemoteRuleListMap(macAddress);
    if (NULL == remoteRuleListHMap)
        remoteRuleListHMap = createRemoteRuleListMap(macAddress);

    if (NULL != remoteRuleListHMap)
    {
        ret = enforceGroupRule(macAddress, *remoteRuleListHMap, groupRule);
        LOG_INFO_FMT("after enforced rule[%s], host[%s] has remote size[%lu]", 
            groupRule->getRuleId().c_str(), macAddress.c_str(), remoteRuleListHMap->size());
    }

    m_rwlock.unlock();

    return ret;
}

INT4 CFirewallPolicy::enforceGroupRule(const std::string& macAddress, CRemoteRuleListHMap& remoteRuleListMap, const CSmartPtr<CFirewallRule>& groupRule)
{
    INT4 ret = BNC_ERR;

    if (groupRule.isNull())
        return ret;

    CFWRuleList* ruleList = findRemoteRuleList(remoteRuleListMap, groupRule->getRemoteIp());
    if (NULL == ruleList)
        ruleList = createRemoteRuleList(remoteRuleListMap, groupRule->getRemoteIp());

    if (NULL != ruleList)
    {
        ret = enforceGroupRule(macAddress, *ruleList, groupRule);
        LOG_INFO_FMT("after enforced rule[%s], for host[%s], remote[%s] has rule size[%lu]", 
            groupRule->getRuleId().c_str(), macAddress.c_str(), inet_htoa(groupRule->getRemoteIp()), ruleList->size());
    }

    return ret;
}

INT4 CFirewallPolicy::enforceGroupRule(const std::string& macAddress, CFWRuleList& remoteRuleList, const CSmartPtr<CFirewallRule>& groupRule)
{
    if (groupRule.isNull())
        return BNC_ERR;

    STL_FOR_LOOP(remoteRuleList, it)
    {
        if (groupRule->getRuleId().compare((*it)->getRuleId()) == 0)
        {
            if (!groupRule->Compare(*(*it)))
            {
                installFirewallFlow(macAddress, *it, OFPFC_DELETE_STRICT);
                installFirewallFlow(macAddress, groupRule, OFPFC_ADD);
            }

            *it = groupRule;
            return BNC_OK;
        }
    }

    remoteRuleList.push_back(groupRule);
    installFirewallFlow(macAddress, groupRule, OFPFC_ADD);

    LOG_INFO_FMT("host[%s] enforced FIREWALL RULE[%s]: "
        "direction[%s]protocol[%s]remoteIp[%s/%u]port[%u-%u]priority[%u]",
        macAddress.c_str(), groupRule->getRuleId().c_str(), 
        groupRule->getDirection().c_str(), 
        (groupRule->getProtocol()==IP_ICMP)?"ICMP":
        (groupRule->getProtocol()==IP_TCP)?"TCP":
        (groupRule->getProtocol()==IP_UDP)?"UDP":
        (groupRule->getProtocol()==IP_VRRP)?"VRRP":"NONE",
        inet_htoa(groupRule->getRemoteIp()), groupRule->getRemoteIpMask(),
        groupRule->getPortMin(), groupRule->getPortMax(), groupRule->getPriority());
    return BNC_OK;
}

void CFirewallPolicy::unenforceGroupRuleMap(const std::string& macAddress, CRuleHMap& groupRuleMap)
{
    if (groupRuleMap.empty())
        return;

    STL_FOR_LOOP(groupRuleMap, it)
    {
        const CSmartPtr<CFirewallRule>& groupRule = it->second;
        if (groupRule.isNotNull())
            unenforceGroupRule(macAddress, groupRule);
    }
}

void CFirewallPolicy::unenforceGroupRule(const std::string& macAddress, const CSmartPtr<CFirewallRule>& groupRule)
{
    if (groupRule.isNull())
        return;

    m_rwlock.wlock();

    CRemoteRuleListHMap* remoteRuleListHMap = findRemoteRuleListMap(macAddress);
    if (NULL != remoteRuleListHMap)
        unenforceGroupRule(macAddress, *remoteRuleListHMap, groupRule);

    m_rwlock.unlock();
}

void CFirewallPolicy::unenforceGroupRule(const std::string& macAddress, CRemoteRuleListHMap& remoteRuleListMap, const CSmartPtr<CFirewallRule>& groupRule)
{
    if (groupRule.isNull())
        return;

    CFWRuleList* ruleList = findRemoteRuleList(remoteRuleListMap, groupRule->getRemoteIp());
    if (NULL != ruleList)
        unenforceGroupRule(macAddress, *ruleList, groupRule);
}

void CFirewallPolicy::unenforceGroupRule(const std::string& macAddress, CFWRuleList& remoteRuleList, const CSmartPtr<CFirewallRule>& groupRule)
{
    if (groupRule.isNull())
        return;

    STL_FOR_LOOP(remoteRuleList, it)
    {
        if (groupRule->getRuleId().compare((*it)->getRuleId()) == 0)
        {
            installFirewallFlow(macAddress, *it, OFPFC_DELETE_STRICT);

            remoteRuleList.erase(it);
            break;
        }
    }
}

INT4 CFirewallPolicy::installFirewallOutEphemeralFlow(const std::string& macAddress, 
    UINT4 srcIP, UINT4 dstIP, UINT1 protocol, UINT2 srcPort, UINT2 dstPort, UINT2 priority, BOOL accept)
{
    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return BNC_ERR;

	if (macAddress.empty())
		return BNC_ERR;

    UINT1 mac[6] = {0};
    macstr2hex(macAddress.c_str(), mac);

    CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(mac);
	if (host.isNull())
	{
        LOG_WARN_FMT("Can not find CHost via host mac[%s] !", macAddress.c_str());
		return BNC_ERR;
	}

    CSmartPtr<CSwitch> sw = host->getSw();
	if (sw.isNull())
	{
        LOG_WARN_FMT("Host with mac[%s] is not tracked !", macAddress.c_str());
		return BNC_ERR;
	}
    
	Base_External* baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(htonl(srcIP));
    if (NULL == baseExternalPort)
    {
        INT1 ipStr[20] = {0};
        number2ip(htonl(srcIP), ipStr);
        LOG_WARN_FMT("Can not find baseExternalPort via fixedIp[%s] !", ipStr);
        return BNC_ERR;
    }

    if (0 == baseExternalPort->get_switch_DPID())
    {
        LOG_WARN("external switch is not recognized !");
        return BNC_ERR;
    }
	CSmartPtr<CSwitch> extSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
    if (extSw.isNull())
    {
        LOG_WARN_FMT("Can not find external switch via dpid[%llx] !", baseExternalPort->get_switch_DPID());
        return BNC_ERR;
    }
    
    CFlowMgr::getInstance()->install_firewallOut_ephemeral_flow(sw, srcIP, dstIP, protocol, srcPort, dstPort, priority+1, accept);
    if (sw != extSw)
    {
        CFlowMgr::getInstance()->install_firewallOut_ephemeral_flow(extSw, 0, dstIP, protocol, 0, dstPort, priority+1, accept);
    }

    return BNC_OK;
}

INT4 CFirewallPolicy::installFirewallInEphemeralFlow(const std::string& macAddress, 
    UINT4 srcIP, UINT4 dstIP, UINT1 protocol, UINT2 srcPort, UINT2 dstPort, UINT2 priority, BOOL accept)
{
    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return BNC_ERR;

	if (macAddress.empty())
		return BNC_ERR;

    UINT1 mac[6] = {0};
    macstr2hex(macAddress.c_str(), mac);

    CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(mac);
	if (host.isNull())
	{
        LOG_WARN_FMT("Can not find CHost via host mac[%s] !", macAddress.c_str());
		return BNC_ERR;
	}

    CSmartPtr<CSwitch> sw = host->getSw();
	if (sw.isNull())
	{
        LOG_WARN_FMT("Host with mac[%s] is not tracked !", macAddress.c_str());
		return BNC_ERR;
	}
    
	Base_External* baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(host->getfixIp());
    if (NULL == baseExternalPort)
    {
        INT1 ipStr[20] = {0};
        number2ip(host->getfixIp(), ipStr);
        LOG_WARN_FMT("Can not find baseExternalPort via fixedIp[%s] !", ipStr);
        return BNC_ERR;
    }

    if (0 == baseExternalPort->get_switch_DPID())
    {
        LOG_WARN("external switch is not recognized !");
        return BNC_ERR;
    }
	CSmartPtr<CSwitch> extSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
    if (extSw.isNull())
    {
        LOG_WARN_FMT("Can not find external switch via dpid[%llx] !", baseExternalPort->get_switch_DPID());
        return BNC_ERR;
    }
    
    CFlowMgr::getInstance()->install_firewallIn_ephemeral_flow(extSw, srcIP, dstIP, protocol, srcPort, dstPort, priority+1, accept);
    if (sw != extSw)
    {
        CFlowMgr::getInstance()->install_firewallIn_ephemeral_flow(sw, srcIP, dstIP, protocol, srcPort, dstPort, priority+1, accept);
    }

    return BNC_OK;
}


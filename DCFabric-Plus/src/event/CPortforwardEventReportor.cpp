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
*   File Name   : CPortforwardEventReportor.cpp                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CPortforwardEventReportor.h"
#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"

CPortforwardEventReportor* CPortforwardEventReportor::m_pInstance = NULL;       

CPortforwardEventReportor* CPortforwardEventReportor::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CPortforwardEventReportor();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return (m_pInstance);
}

CPortforwardEventReportor::CPortforwardEventReportor()
{
}

CPortforwardEventReportor::~CPortforwardEventReportor()
{
}

INT4 CPortforwardEventReportor::report(INT4 event, INT4 reason, const CPortforwardRule& rule)
{
    if (!((EVENT_TYPE_PORTFORWARD_RULE_C <= event) && (EVENT_TYPE_PORTFORWARD_RULE_DS >= event)))
        return BNC_ERR;

    CPortforwardEvent* evt = new CPortforwardEvent(event, reason, rule);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CPortforwardEvent failed!");
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    INT1 outIpStr[20] = {0}, inIpStr[20] = {0};
    number2ip(htonl(rule.getOutsideIp()), outIpStr);
    number2ip(htonl(rule.getInsideIp()), inIpStr);
    UINT2 outPortStart = 0, outPortEnd = 0, inPortStart = 0, inPortEnd = 0;
    rule.getOutsidePort(outPortStart, outPortEnd);
    rule.getInsidePort(inPortStart, inPortEnd);

    INT1 descStr[1024];
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], %s portforward rule"
             "(%s,protocol[%d],outIp[%s],inIp[%s],outPort[%u-%u],inPort[%u-%u],networkId[%s],subnetId[%s])",
             event, path.c_str(), reason, 
             (EVENT_TYPE_PORTFORWARD_RULE_C==event)?"CREATED":
             (EVENT_TYPE_PORTFORWARD_RULE_U==event)?"UPDATED":
             (EVENT_TYPE_PORTFORWARD_RULE_D==event)?"DELETED":"UNKNOWN",
             rule.getEnabled()?"enabled":"disabled", rule.getProtocol(),
             outIpStr, inIpStr, outPortStart, outPortEnd, inPortStart, inPortEnd, 
             rule.getNetworkId().c_str(), rule.getSubnetId().c_str());
    //LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);
    
    return CEventReportor::report(evt);
}

CMsgPath CPortforwardEventReportor::getMsgPath(INT4 event)
{
    return g_portforwardEventPath[0];
}


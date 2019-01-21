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
*   File Name   : CSwitchEventReportor.cpp                                    *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CSwitchEventReportor.h"
#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"
#include "CMemPool.h"
#include "CSwitchEvent.h"
#include "CSwitch.h"

CSwitchEventReportor* CSwitchEventReportor::m_pInstance = NULL;       

CSwitchEventReportor* CSwitchEventReportor::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CSwitchEventReportor();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return (m_pInstance);
}

CSwitchEventReportor::CSwitchEventReportor()
{
}

CSwitchEventReportor::~CSwitchEventReportor()
{
}

INT4 CSwitchEventReportor::report(CSmartPtr<CSwitch>& sw, INT4 event, INT4 reason, const switch_dupl_conn_t* duplConn)
{
    if (sw.isNull())
        return BNC_ERR;
    
    if ((EVENT_TYPE_SWITCH_RECONNECT < event) || (EVENT_TYPE_SWITCH_CONNECT > event))
        return BNC_ERR;
    
    CSwitchEvent* evt = new CSwitchEvent(event, reason, sw);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CSwitchEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    CMsgKey key = getMsgKey(sw);
    evt->setKey(key);

    INT1 macStr[20] = {0}, ipStr[20] = {0};
    mac2str((UINT1*)sw->getSwMac(), macStr);
    number2ip(htonl(sw->getSwIp()), ipStr);

    INT1 descStr[1024];
    if ((EVENT_TYPE_SWITCH_RECONNECT == event) && (NULL != duplConn))
    {
        evt->setDuplConn(*duplConn);

        INT1 dulpIpStr[20] = {0};
        number2ip(htonl(duplConn->ip), dulpIpStr);
        snprintf(descStr, sizeof(descStr), 
                 "Reported event[0x%x][%s] with reason[0x%x] on switch[%llx] with mac[%s]ip[%s]port[%u]sockfd[%d], "
                 "previous connection state[%d] with ip[%s]port[%u]sockfd[%d]",
                 event, path.c_str(), reason, sw->getDpid(), macStr, ipStr, sw->getSwPort(), sw->getSockfd(), 
                 duplConn->state, dulpIpStr, duplConn->port, duplConn->sockfd);
    }
    else
    {
        snprintf(descStr, sizeof(descStr), 
                 "Reported event[0x%x][%s] with reason[0x%x] on switch[%llx] with mac[%s]ip[%s]port[%u]sockfd[%d]",
                 event, path.c_str(), reason, sw->getDpid(), macStr, ipStr, sw->getSwPort(), sw->getSockfd());
    }
    LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);

    return CEventReportor::report(evt);
}

CMsgPath CSwitchEventReportor::getMsgPath(INT4 event)
{
    return g_switchEventPath[event-EVENT_TYPE_SWITCH_CONNECT];
}

CMsgKey CSwitchEventReportor::getMsgKey(CSmartPtr<CSwitch>& sw)
{
    INT1 macStr[50] = {0};
    mac2str((UINT1*)sw->getSwMac(), macStr);
    return macStr;
}


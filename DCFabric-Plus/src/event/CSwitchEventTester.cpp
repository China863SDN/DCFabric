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
*   File Name   : CSwitchEventTester.cpp                                      *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CSwitchEventTester.h"
#include "log.h"
#include "bnc-error.h"
#include "CSwitchEvent.h"
#include "CEventReportor.h"
#include "comm-util.h"

CSwitchEventTester::CSwitchEventTester()
{
}

CSwitchEventTester::~CSwitchEventTester()
{
}

void CSwitchEventTester::test(INT4 event)
{
    CMsgPath path = g_switchEventPath[event-EVENT_TYPE_SWITCH_CONNECT];
    for (INT4 i = 0; i < 30; ++i)
        CEventConsumer::consume(path);
}

INT4 CSwitchEventTester::consume(CSmartPtr<CMsgCommon> evt)
{
    if (evt.isNull())
        return BNC_ERR;

    CEvent* pEvt = (CEvent*)evt.getPtr();
    if ((EVENT_TYPE_SWITCH_RECONNECT < pEvt->getEvent()) || (EVENT_TYPE_SWITCH_CONNECT > pEvt->getEvent()))
    {
        LOG_WARN_FMT("%s consume invalid event[%d] !", toString(), pEvt->getEvent());
        return BNC_ERR;
    }
#if 1
    CSwitchEvent* pSwEvt = (CSwitchEvent*)pEvt;
    INT1 macStr[20] = {0}, ipStr[20] = {0};
    mac2str((UINT1*)pSwEvt->getMac(), macStr);
    number2ip(htonl(pSwEvt->getIp()), ipStr);

    if (EVENT_TYPE_SWITCH_RECONNECT == pSwEvt->getEvent())
    {
        const switch_dupl_conn_t& duplConn = pSwEvt->getDuplConn();
        INT1 dulpIpStr[20] = {0};
        number2ip(htonl(duplConn.ip), dulpIpStr);
        LOG_DEBUG_FMT("######%s consumed event[%d][%s] with reason[%d] on switch[%llx] with mac[%s]ip[%s]port[%u]sockfd[%d], "
                     "previous connection state[%d] with ip[%s]port[%u]sockfd[%d]",
                     toString(), pSwEvt->getEvent(), pSwEvt->getPath().c_str(), pSwEvt->getReason(), 
                     pSwEvt->getDpid(), macStr, ipStr, pSwEvt->getPort(), pSwEvt->getSockfd(),
                     duplConn.state, dulpIpStr, duplConn.port, duplConn.sockfd);
    }
    else
    {
        LOG_DEBUG_FMT("######%s consumed event[%d][%s] with reason[%d] on switch[%llx] with mac[%s]ip[%s]port[%u]sockfd[%d]",
                     toString(), pSwEvt->getEvent(), pSwEvt->getPath().c_str(), pSwEvt->getReason(), 
                     pSwEvt->getDpid(), macStr, ipStr, pSwEvt->getPort(), pSwEvt->getSockfd());
    }
    LOG_DEBUG(pSwEvt->getDesc().c_str());
#endif
    return BNC_OK;
}


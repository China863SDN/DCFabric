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
*   File Name   : CLinkEventTester.cpp                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CLinkEventTester.h"
#include "log.h"
#include "bnc-error.h"
#include "CLinkEvent.h"
#include "CEventReportor.h"

CLinkEventTester::CLinkEventTester()
{
}

CLinkEventTester::~CLinkEventTester()
{
}

void CLinkEventTester::test()
{
    CMsgPath path(g_topoLinkEventPath[0]);
    for (INT4 i = 0; i < 30; ++i)
        CEventConsumer::consume(path);
}

INT4 CLinkEventTester::consume(CSmartPtr<CMsgCommon> evt)
{
    if (evt.isNull())
        return BNC_ERR;

    CEvent* pEvt = (CEvent*)evt.getPtr();
    if ((EVENT_TYPE_TOPO_LINK_RECOVER < pEvt->getEvent()) || (EVENT_TYPE_TOPO_LINK_ESTABLISH > pEvt->getEvent()))
    {
        LOG_WARN_FMT("%s consume invalid event[%d] !", toString(), pEvt->getEvent());
        return BNC_ERR;
    }
#if 1
    CLinkEvent* pLnEvt = (CLinkEvent*)pEvt;
    LOG_DEBUG_FMT("######%s consumed event[%d][%s] with reason[%d], %s link: switch[%llx]port[%u] & neighbor switch[%llx]port[%u]",
                 toString(), pLnEvt->getEvent(), pLnEvt->getPath().c_str(), pLnEvt->getReason(), 
                 (EVENT_TYPE_TOPO_LINK_ESTABLISH==pLnEvt->getEvent())?"ESTABLISHED":
                 (EVENT_TYPE_TOPO_LINK_DELETE==pLnEvt->getEvent())?"DELETED":"UNALIVE", 
                 pLnEvt->getSrcDpid(), pLnEvt->getSrcPort(), pLnEvt->getDstDpid(), pLnEvt->getDstPort());
    LOG_DEBUG(pLnEvt->getDesc().c_str());
#endif
    return BNC_OK;
}


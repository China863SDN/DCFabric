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
*   File Name   : CPortEventTester.cpp                                        *
*   Author      : bnc jqjiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CPortEventTester.h"
#include "log.h"
#include "bnc-error.h"
#include "CPortEvent.h"
#include "CEventReportor.h"

CPortEventTester::CPortEventTester()
{
}

CPortEventTester::~CPortEventTester()
{
}

void CPortEventTester::test()
{
    CMsgPath path(g_portEventPath[0]);
    for (INT4 i = 0; i < 30; ++i)
        CEventConsumer::consume(path);
}

INT4 CPortEventTester::consume(CSmartPtr<CMsgCommon> evt)
{
    if (evt.isNull())
        return BNC_ERR;

    CEvent* pEvt = (CEvent*)evt.getPtr();
    if ((EVENT_TYPE_PORT_DOWN < pEvt->getEvent()) || (EVENT_TYPE_PORT_UP > pEvt->getEvent()))
    {
        LOG_WARN_FMT("%s consume invalid event[%d] !", toString(), pEvt->getEvent());
        return BNC_ERR;
    }
#if 1
    CPortEvent* pPortEvt = (CPortEvent*)pEvt;
    if (PORT_EVENT_ADD == pPortEvt->getType())
    {
        LOG_DEBUG_FMT("######%s consumed event[%d][%s] with reason[%d] on switch[%llx], port[%u] with state[%d] added",
                     toString(), pPortEvt->getEvent(), pPortEvt->getPath().c_str(), pPortEvt->getReason(), 
                     pPortEvt->getDpid(), pPortEvt->getPort(), pPortEvt->getStateNew());
    }
    else if (PORT_EVENT_MODIFY == pPortEvt->getType())
    {
        LOG_DEBUG_FMT("######%s consumed event[%d][%s] with reason[%d] on switch[%llx], state of port[%u] changed from %d to %d",
                     toString(), pPortEvt->getEvent(), pPortEvt->getPath().c_str(), pPortEvt->getReason(), 
                     pPortEvt->getDpid(), pPortEvt->getPort(), pPortEvt->getState(), pPortEvt->getStateNew());
    }
    else if (PORT_EVENT_DELETE == pPortEvt->getType())
    {
        LOG_DEBUG_FMT("######%s consumed event[%d][%s] with reason[%d] on switch[%llx], port[%u] with state[%d] deleted",
                     toString(), pPortEvt->getEvent(), pPortEvt->getPath().c_str(), pPortEvt->getReason(), 
                     pPortEvt->getDpid(), pPortEvt->getPort(), pPortEvt->getState());
    }
    LOG_DEBUG_FMT(pPortEvt->getDesc().c_str());
#endif
    return BNC_OK;
}


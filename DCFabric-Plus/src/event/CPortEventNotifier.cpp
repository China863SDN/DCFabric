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
*   File Name   : CPortEventConsumer.cpp                                        *
*   Author      : bnc cyyang                                                 *
*   Create Date : 2018-04-24                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "COfMsgUtil.h"
#include "CPortEvent.h"
#include "CEventReportor.h"
#include "CPortEventNotifier.h"



INT4 CPortEventNotifier::onregister() 
{
	CMsgPath path(g_portEventPath[0]);
	if (CEventNotifier::onregister(path, FALSE))
    {
        LOG_WARN_FMT("CPortEventNotifier register to path[%s] failed !", path.c_str());
        CEventNotifier::deregister(path);
        return BNC_ERR;
    }

    return BNC_OK;
}
void CPortEventNotifier::deregister() 
{
	CMsgPath path(g_portEventPath[0]);
	CEventNotifier::deregister(path);
    return ;
}


INT4 CPortEventNotifier::consume(CSmartPtr<CMsgCommon> evt)
{
    if (evt.isNull())
        return BNC_ERR;

    CEvent* pEvt = (CEvent*)evt.getPtr();
    if ((EVENT_TYPE_PORT_DOWN < pEvt->getEvent()) || (EVENT_TYPE_PORT_UP > pEvt->getEvent()))
    {
        LOG_WARN_FMT("%s consume invalid event[%d] !", toString(), pEvt->getEvent());
        return BNC_ERR;
    }
    CPortEvent* pPortEvt = (CPortEvent*)pEvt;
    if (PORT_EVENT_ADD == pPortEvt->getType())
    {
        LOG_INFO_FMT("######%s consumed event[%d][%s] with reason[%d] on switch[%llx], port[%u] with state[%d] added",
                     toString(), pPortEvt->getEvent(), pPortEvt->getPath().c_str(), pPortEvt->getReason(), 
                     pPortEvt->getDpid(), pPortEvt->getPort(), pPortEvt->getStateNew());
    }
    else if (PORT_EVENT_MODIFY == pPortEvt->getType())
    {
        LOG_INFO_FMT("######%s consumed event[%d][%s] with reason[%d] on switch[%llx], state of port[%u] changed from %d to %d",
                     toString(), pPortEvt->getEvent(), pPortEvt->getPath().c_str(), pPortEvt->getReason(), 
                     pPortEvt->getDpid(), pPortEvt->getPort(), pPortEvt->getState(), pPortEvt->getStateNew());
		if((PORT_EVENT_ADD == pPortEvt->getState())&&(PORT_EVENT_DELETE == pPortEvt->getStateNew()))
		{
			COfMsgUtil::remove_flows_by_sw_port(pPortEvt->getDpid(), pPortEvt->getPort());
		}
    }
    else if (PORT_EVENT_DELETE == pPortEvt->getType())
    {
        LOG_INFO_FMT("######%s consumed event[%d][%s] with reason[%d] on switch[%llx], port[%u] with state[%d] deleted",
                     toString(), pPortEvt->getEvent(), pPortEvt->getPath().c_str(), pPortEvt->getReason(), 
                     pPortEvt->getDpid(), pPortEvt->getPort(), pPortEvt->getState());
		COfMsgUtil::remove_flows_by_sw_port(pPortEvt->getDpid(), pPortEvt->getPort());
    }
    LOG_WARN_FMT(pPortEvt->getDesc().c_str());
    return BNC_OK;
}


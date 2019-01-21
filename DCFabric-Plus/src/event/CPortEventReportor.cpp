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
*   File Name   : CPortEventReportor.cpp                                      *
*   Author      : bnc jqjiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CPortEventReportor.h"
#include "bnc-error.h"
#include "log.h"
#include "CMemPool.h"
#include "CPortEvent.h"
#include "comm-util.h"

static const char* g_eventTypeString[] = {
    "EVENT_NONE",
    "EVENT_ADD_PORT",
    "EVENT_MODIFY_PORT",
    "EVENT_DELETE_PORT",
    "EVENT_MAX"
};

CPortEventReportor* CPortEventReportor::m_pInstance = NULL;       

CPortEventReportor* CPortEventReportor::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CPortEventReportor();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return (m_pInstance);
}

CPortEventReportor::CPortEventReportor()
{
}

CPortEventReportor::~CPortEventReportor()
{
}

INT4 CPortEventReportor::report(CSmartPtr<CSwitch> sw, UINT4 port_no, INT4 event, INT4 reason, INT4 type, INT4 state, INT4 stateNew)
{
    if (sw.isNull())
        return BNC_ERR;
    
    if ((EVENT_TYPE_PORT_DOWN < event) || (EVENT_TYPE_PORT_UP > event))
        return BNC_ERR;
    
    if ((PORT_EVENT_DELETE < type) || (PORT_EVENT_ADD > type))
        return BNC_ERR;
    
    CPortEvent* evt = new CPortEvent(event, reason, type, sw->getSockfd(), sw->getDpid(), port_no);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CPortEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    CMsgKey key = getMsgKey(sw->getDpid(), port_no);
    evt->setKey(key);

    evt->setState(state);
    evt->setStateNew(stateNew);

    INT1 descStr[1024];
    if (PORT_EVENT_ADD == type)
        snprintf(descStr, sizeof(descStr), 
                 "Reported event[0x%x][%s] with reason[0x%x] on switch[%llx], port[%u] with state[%d] added",
                 event, path.c_str(), reason, sw->getDpid(), port_no, stateNew);
    else if (PORT_EVENT_MODIFY == type)
        snprintf(descStr, sizeof(descStr), 
                 "Reported event[0x%x][%s] with reason[0x%x] on switch[%llx], "
                 "state of port[%u] changed from %d to %d, when %s happened",
                 event, path.c_str(), reason, sw->getDpid(), port_no, state, stateNew, g_eventTypeString[type]);
    else if (PORT_EVENT_DELETE == type)
        snprintf(descStr, sizeof(descStr), 
                 "Reported event[0x%x][%s] with reason[0x%x] on switch[%llx], port[%u] with state[%d] deleted",
                 event, path.c_str(), reason, sw->getDpid(), port_no, state);
    LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);

    return CEventReportor::report(evt);
}

CMsgPath CPortEventReportor::getMsgPath(INT4 event)
{
    return g_portEventPath[0];
    //return g_portEventPath[event-EVENT_TYPE_PORT_UP];
}

CMsgKey CPortEventReportor::getMsgKey(UINT8 dpid, UINT4 port)
{
    CMsgKey key;
    key = to_string(dpid);
    key += ":";
    key += to_string(port);
    
    return key;
}


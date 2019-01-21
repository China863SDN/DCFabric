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
*   File Name   : CLinkEventReportor.cpp                                      *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CLinkEventReportor.h"
#include "bnc-error.h"
#include "log.h"
#include "CMemPool.h"
#include "CLinkEvent.h"
#include "comm-util.h"

CLinkEventReportor* CLinkEventReportor::m_pInstance = NULL;       

CLinkEventReportor* CLinkEventReportor::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CLinkEventReportor();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return (m_pInstance);
}

CLinkEventReportor::CLinkEventReportor()
{
}

CLinkEventReportor::~CLinkEventReportor()
{
}

INT4 CLinkEventReportor::report(INT4 event, INT4 reason, UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort)
{
    if ((0 == dpid) || (0 == neighDpid))
        return BNC_ERR;

    if ((EVENT_TYPE_TOPO_LINK_RECOVER < event) || (EVENT_TYPE_TOPO_LINK_ESTABLISH > event))
        return BNC_ERR;

    CLinkEvent* evt = new CLinkEvent(event, reason, dpid, port, neighDpid, neighPort);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CLinkEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    CMsgKey key = getMsgKey(dpid, port, neighDpid, neighPort);
    evt->setKey(key);

    INT1 descStr[1024];
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], %s link: switch[%llx]port[%u] & neighbor switch[%llx]port[%u]",
             event, path.c_str(), reason, 
             (EVENT_TYPE_TOPO_LINK_ESTABLISH==event)?"ESTABLISHED":
             (EVENT_TYPE_TOPO_LINK_DELETE==event)?"DELETED":"UNALIVE", 
             dpid, port, neighDpid, neighPort);
    LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);
    
    return CEventReportor::report(evt);
}

CMsgPath CLinkEventReportor::getMsgPath(INT4 event)
{
    return g_topoLinkEventPath[0];
    //return g_topoLinkEventPath[event-EVENT_TYPE_TOPO_LINK_ESTABLISH];
}

CMsgKey CLinkEventReportor::getMsgKey(UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort)
{
    CMsgKey key;
    key = to_string(dpid);
    key += ":";
    key += to_string(port);
    key += ":";
    key += to_string(neighDpid);
    key += ":";
    key += to_string(neighPort);

    return key;
}


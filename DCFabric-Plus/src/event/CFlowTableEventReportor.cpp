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
*   File Name   : CFlowTableEventReportor.cpp                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"
#include "CFlowTableEventReportor.h"
#include "CFlowTableEvent.h"

CFlowTableEventReportor* CFlowTableEventReportor::m_pInstance = NULL;       

CFlowTableEventReportor* CFlowTableEventReportor::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CFlowTableEventReportor();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return (m_pInstance);
}

CFlowTableEventReportor::CFlowTableEventReportor()
{
}

CFlowTableEventReportor::~CFlowTableEventReportor()
{
}

INT4 CFlowTableEventReportor::report(INT4 event, INT4 reason, CSmartPtr<CSwitch> sw, gn_flow_t& flow)
{
    if (!((EVENT_TYPE_FLOW_TABLE_ADD <= event) && (EVENT_TYPE_FLOW_TABLE_RECOVER >= event)))
        return BNC_ERR;

    CFlowTableEvent* evt = new CFlowTableEvent(event, reason, sw, flow);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CFlowTableEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);

    INT1 descStr[1024];
    INT1 ipStr[20] = {0};
    number2ip(htonl(sw->getSwIp()), ipStr);
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], %s flow table with uuid[%s]table_id[%u] on switch[0x%llx][%s]",
             event, path.c_str(), reason, 
             (EVENT_TYPE_FLOW_TABLE_ADD==event)?"INSTALLED":
             (EVENT_TYPE_FLOW_TABLE_MOD==event)?"MODIFIED":
             (EVENT_TYPE_FLOW_TABLE_MOD_STRICT==event)?"STRICT MODIFIED":
             (EVENT_TYPE_FLOW_TABLE_DEL==event)?"DELETED":
             (EVENT_TYPE_FLOW_TABLE_DEL_STRICT==event)?"STRICT DELETED":
             (EVENT_TYPE_FLOW_TABLE_RECOVER==event)?"RECOVERED":"UNKNOWN",
             flow.uuid, flow.table_id, sw->getDpid(), ipStr);
    //LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);
    
    return CEventReportor::report(evt);
}

CMsgPath CFlowTableEventReportor::getMsgPath(INT4 event)
{
    return g_flowTableEventPath[0];
}


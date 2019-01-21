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
*   File Name   : CEventReportor.cpp                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CEventReportor.h"
#include "bnc-error.h"
#include "log.h"
#include "CControl.h"
#include "comm-util.h"
#include "CClusterService.h"
#include "openflow-common.h"

const char* g_switchEventPath[] = 
{
    "sw.conn",
    "sw.quit",
    "sw.inStable",
    "sw.inUnreach",
    "sw.outUnreach",
    "sw.disconn",
    "sw.powerOff",
    "sw.reconn"
};

const char* g_portEventPath[] = 
{
    "port.status"
};

const char* g_topoLinkEventPath[] = 
{
    "link.status"
};

const char* g_NetworkingEventPath[] = 
{
    "networking"
};

const char* g_securityGroupEventPath[] = 
{
    "security.group"
};

const char* g_portforwardEventPath[] = 
{
    "portforward"
};

const char* g_qosPolicyEventPath[] = 
{
	"qos.policy"
};

const char* g_flowTableEventPath[] = 
{
	"flow.table"
};

const char* g_tagflowEventPath[] = 
{
	"tag.flow"
};

CEventReportor::CEventReportor():CMsgOperator(MSG_OPERATOR_PRODUCER)
{
}

CEventReportor::~CEventReportor()
{
    STL_FOR_LOOP(m_paths, it)
    {
        CMsgPath path = it->first;
        CMsgOperator::deregister(path);
    }
}

INT4 CEventReportor::onregister(CMsgPath& path)
{
    if (CMsgOperator::onregister(path) == BNC_OK)
    {
        m_paths.insert(std::pair<CMsgPath, BOOL>(path, TRUE));
        return BNC_OK;
    }

    return BNC_ERR;
}

void CEventReportor::deregister(CMsgPath& path)
{
    CMsgOperator::deregister(path);
    m_paths.erase(path);
}

INT4 CEventReportor::report(CEvent* event)
{
    if (NULL == event)
        return BNC_ERR;

#if 0
	if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
    {
        delete event;
        return BNC_ERR;
    }
#endif

    if (!isRegistered(event->getPath()))
    {
        m_mutex.lock();

        if (!isRegistered(event->getPath()))
        {
            if (onregister(event->getPath()) != BNC_OK)
            {
                LOG_WARN_FMT("#%s[%p]# register to event path[%s] failed !", 
                    toString(), this, event->getPath().c_str());
                m_mutex.unlock();
                delete event;
                return BNC_ERR;
            }
        }

        m_mutex.unlock();
    }

    event->setState(EVENT_STATE_ACTIVE);   

    CSmartPtr<CMsgCommon> msg(event);
    if (CControl::getInstance()->getMsgTree().pushMsg(msg) != BNC_OK)
    {
        LOG_WARN_FMT("push event[%d] with path[%s]key[%s] into tree failed !", 
            event->getEvent(), event->getPath().c_str(), event->getKey().c_str());
        return BNC_ERR;
    }

    //LOG_WARN(event->getDesc().c_str());

    return BNC_OK;
}

BOOL CEventReportor::isRegistered(CMsgPath& path)
{
    std::map<CMsgPath, BOOL>::iterator it = m_paths.find(path);
    return (it != m_paths.end());
}


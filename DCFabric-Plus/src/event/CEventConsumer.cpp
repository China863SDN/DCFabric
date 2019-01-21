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
*   File Name   : CEventConsumer.cpp                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CEventConsumer.h"
#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CControl.h"

CEventConsumer::CEventConsumer():CMsgOperator(MSG_OPERATOR_CONSUMER)
{
}

CEventConsumer::~CEventConsumer()
{
    STL_FOR_LOOP(m_paths, it)
    {
        CMsgPath path = it->first;
        CMsgOperator::deregister(path);
    }
}

INT4 CEventConsumer::onregister(CMsgPath& path, BOOL integrated)
{
    if (CMsgOperator::onregister(path) == BNC_OK)
    {
        m_paths.insert(std::pair<CMsgPath, BOOL>(path, integrated));
        return BNC_OK;
    }

    return BNC_ERR;
}

void CEventConsumer::deregister(CMsgPath& path)
{
    CMsgOperator::deregister(path);
    m_paths.erase(path);
}

INT4 CEventConsumer::consume(CMsgPath& path, BOOL integrated)
{
    if (!isRegistered(path))
	{
        if (onregister(path, integrated) != BNC_OK)
        {
            LOG_WARN_FMT("%s[%p] register to event path[%s] failed !", toString(), this, path.c_str());
            return BNC_ERR;
        }
    }
    
    if (integrated)
    {
        LOG_INFO_FMT("%s[%p] pop single event with path[%s] ...", toString(), this, path.c_str());
        CSmartPtr<CMsgCommon> evt = CControl::getInstance()->getMsgTree().popMsg(path, this);
        if (evt.isNotNull())
            return consume(evt);
    }
    else
    {
        LOG_INFO_FMT("%s[%p] pop event queue with path[%s] ...", toString(), this, path.c_str());
        CSmartPtr<CMsgQueue> queue = CControl::getInstance()->getMsgTree().popMsgQueue(path, this);
        if (queue.isNotNull())
            return consume(queue);
    }

    return BNC_ERR;
}

INT4 CEventConsumer::consume(CSmartPtr<CMsgQueue> queue)
{
    LOG_INFO("CEventConsumer consume event queue");
    return BNC_OK;
}

INT4 CEventConsumer::consume(CSmartPtr<CMsgCommon> evt)
{
    LOG_INFO("CEventConsumer consume single event");
    return BNC_OK;
}

BOOL CEventConsumer::isRegistered(CMsgPath& path)
{
    std::map<CMsgPath, BOOL>::iterator it = m_paths.find(path);
    return (it != m_paths.end());
}


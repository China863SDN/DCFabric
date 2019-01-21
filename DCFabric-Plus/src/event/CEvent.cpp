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
*   File Name   : CEvent.cpp                                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CEvent.h"
#include "bnc-error.h"
#include "CMemPool.h"

CEvent::CEvent():
    CMsgCommon(MSG_OPER_HOLD),
    m_state(EVENT_STATE_INIT),
    m_event(EVENT_TYPE_NONE),
    m_reason(EVENT_REASON_NONE)
{
}

CEvent::CEvent(INT4 oper, INT4 event, INT4 reason):
    CMsgCommon(oper),
    m_state(EVENT_STATE_INIT),
    m_event(event),
    m_reason(reason)
{
}

CEvent::~CEvent()
{
}


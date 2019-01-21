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
*   File Name   : CPortEvent.cpp                                              *
*   Author      : bnc jqjiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CPortEvent.h"
#include "bnc-error.h"
#include "log.h"
#include "bnc-param.h"

CPortEvent::CPortEvent():
    m_type(PORT_EVENT_NONE),
    m_sockfd(-1),
    m_dpid(0),
    m_port_no(0),
    m_state(PORT_STATE_INIT),
    m_stateNew(PORT_STATE_INIT)
{
}

CPortEvent::CPortEvent(INT4 event, INT4 reason, INT4 type, INT4 sockfd, INT4 dpid, INT4 port_no):
    CEvent(MSG_OPER_OVERRIDE, event, reason),
    m_type(type),
    m_sockfd(sockfd),
    m_dpid(dpid),
    m_port_no(port_no),
    m_state(PORT_STATE_INIT),
    m_stateNew(PORT_STATE_INIT)
{
}

CPortEvent::~CPortEvent()
{
}



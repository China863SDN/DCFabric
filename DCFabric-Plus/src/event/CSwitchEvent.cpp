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
*   File Name   : CSwitchEvent.cpp                                            *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CSwitchEvent.h"
#include "bnc-error.h"
#include "CMemPool.h"

CSwitchEvent::CSwitchEvent():
    m_dpid(0),
    m_ip(0),
    m_port(0),
    m_sockfd(-1)
{
    memset(&m_mac, 0, MAC_LEN);
    memset(&m_duplConn, 0, sizeof(m_duplConn));
}

CSwitchEvent::CSwitchEvent(INT4 event, INT4 reason, CSmartPtr<CSwitch>& sw):
    CEvent(MSG_OPER_HOLD, event, reason),
    m_dpid(sw->getDpid()),
    m_ip(sw->getSwIp()),
    m_port(sw->getSwPort()),
    m_sockfd(sw->getSockfd())
{
    memcpy(&m_mac, sw->getSwMac(), MAC_LEN);
    memset(&m_duplConn, 0, sizeof(m_duplConn));
}

CSwitchEvent::~CSwitchEvent()
{
}


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
*   File Name   : CLinkEvent.cpp                                              *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CLinkEvent.h"
#include "bnc-error.h"
#include "bnc-param.h"

CLinkEvent::CLinkEvent():
    m_srcDpid(0),
    m_srcPort(0),
    m_dstDpid(0),
    m_dstPort(0)
{
}

CLinkEvent::CLinkEvent(INT4 event, INT4 reason, UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort):
    CEvent(MSG_OPER_OVERRIDE, event, reason),
    m_srcDpid(dpid),
    m_srcPort(port),
    m_dstDpid(neighDpid),
    m_dstPort(neighPort)
{
}

CLinkEvent::~CLinkEvent()
{
}


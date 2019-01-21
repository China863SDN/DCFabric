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
*   File Name   : COpenstackRouter.cpp          *
*   Author      : bnc xflu                      *
*   Create Date : 2016-9-7                      *
*   Version     : 1.0                           *
*   Function    : .                             *
*                                                                             *
******************************************************************************/
#include "COpenstackRouter.h"
#include "bnc-param.h"

BOOL COpenstackPortforward::Compare(const COpenstackPortforward& portforward) const
{
	if ((0 == portforward.m_status.compare(m_status)) &&
        (0 == portforward.m_insideAddr.compare(m_insideAddr)) &&
        (0 == portforward.m_protocol.compare(m_protocol)) &&
        (0 == portforward.m_outsidePort.compare(m_outsidePort)) &&
        (0 == portforward.m_insidePort.compare(m_insidePort)))
		return TRUE;

	return FALSE;
}


COpenstackRouter::COpenstackRouter():
    m_enableSnat(TRUE),
    m_adminStateUp(TRUE),
    m_distributed(FALSE),
    m_checkStatus(CHECK_CREATE)
{
}

COpenstackRouter::~COpenstackRouter()
{
}

BOOL COpenstackRouter::Compare(const COpenstackRouter& router)
{
	if (0 == router.m_id.compare(m_id))
		return TRUE;

	return FALSE;
}


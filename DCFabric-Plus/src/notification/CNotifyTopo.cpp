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
*   File Name   : CNotifyTopo.cpp  *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-13                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CNotifyTopo.h"
#include "CNotificationCenter.h"

CNotifyTopo::CNotifyTopo()
{

}

CNotifyTopo::~CNotifyTopo()
{

}

void CNotifyTopo::notifyAddPath(const CSmartPtr<CSwitch> & src_sw,
                                const CSmartPtr<CSwitch> & dst_sw,
                                UINT4 port_no)
{
    // LOG_INFO("topo changed");
}

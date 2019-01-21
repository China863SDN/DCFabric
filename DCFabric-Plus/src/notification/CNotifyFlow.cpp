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
*   File Name   : CNotifyHost.cpp           *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-14                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CNotificationCenter.h"
#include "bnc-type.h"
#include "CNotifyFlow.h"
#include "CFlowMgr.h"

CNotifyFlow::CNotifyFlow()
{

}

CNotifyFlow::~CNotifyFlow()
{

}

void CNotifyFlow::notifyAddPath(const CSmartPtr<CSwitch> & src_sw,
                                const CSmartPtr<CSwitch> & dst_sw,
                                UINT4 port_no)
{
   // CFlowMgr::getInstance()->install_swap_flow(src_sw, dst_sw, port_no);
   // CFlowMgr::getInstance()->install_swap_input_flow(src_sw, dst_sw, port_no);
    CFlowMgr::getInstance()->install_different_switch_flow(dst_sw, src_sw,  port_no);
}

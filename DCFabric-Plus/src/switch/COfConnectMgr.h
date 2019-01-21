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
*   File Name   : COfConnectMgr.h                                             *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __COFCONNECTMGR__H
#define __COFCONNECTMGR__H

#include "CSwitchEvent.h"
#include "CTimer.h"
#include "CMutex.h"

class COfConnectMgr
{
public:
    static void processPeerConnect(CSmartPtr<CSwitch>& sw);
    static void processPeerQuit(CSmartPtr<CSwitch>& sw, INT4 reason);
    static void processPeerEnterStable(CSmartPtr<CSwitch>& sw);
    static void processPeerEnterUnreach(CSmartPtr<CSwitch>& sw, INT4 reason=EVENT_REASON_SEND_MSG_FAIL);
    static void processPeerExitUnreach(CSmartPtr<CSwitch>& sw, INT4 reason=EVENT_REASON_CONNECT_RECOVER);
    static void processPeerDisconn(CSmartPtr<CSwitch>& sw);
    static void processPeerPowerOff(CSmartPtr<CSwitch>& sw);
    static void processPeerReconn(CSmartPtr<CSwitch>& sw, switch_dupl_conn_t& duplConn);
    static void processSystemFailure(INT4 sockfd);

private:
    static BOOL needPowerOffCheck(CSmartPtr<CSwitch>& sw);
    static void startPowerOffCheck(CSmartPtr<CSwitch>& sw);
    static void stopPowerOffCheck(CSmartPtr<CSwitch>& sw);
    static void checkPowerOff(void* param);
    static BOOL checkPowerOff(CSmartPtr<CSwitch>& sw);

private:
    const static INT4 power_off_check_interval = 30; //s
    
    static CTimer m_timer;
    static std::map<UINT8, CSmartPtr<CSwitch> > m_sws;
    static CMutex m_mutex;
};

#endif

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
*   File Name   : CLinkEventReportor.h                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CLINKEVENTREPORTOR_H
#define __CLINKEVENTREPORTOR_H

#include "bnc-type.h"
#include "CEventReportor.h"
#include "CSwitch.h"
#include "CLinkEvent.h"

class CLinkEventReportor : public CEventReportor
{
public:
    static CLinkEventReportor* getInstance();

    ~CLinkEventReportor();

    INT4 report(INT4 event, INT4 reason, UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort);

    const char* toString() {return "CLinkEventReportor";}

private:
    CLinkEventReportor();

    CMsgPath getMsgPath(INT4 event);
    CMsgKey getMsgKey(UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort);

private:
    static CLinkEventReportor* m_pInstance;       
};

#endif

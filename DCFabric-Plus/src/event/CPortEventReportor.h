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
*   File Name   : CPortEventReportor.h                                        *
*   Author      : bnc jqjiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CPORTEVENTREPORTOR_H
#define __CPORTEVENTREPORTOR_H

#include "bnc-type.h"
#include "CEventReportor.h"
#include "CSwitch.h"

class CPortEventReportor : public CEventReportor
{
public:
    static CPortEventReportor* getInstance();

    ~CPortEventReportor();

    INT4 report(CSmartPtr<CSwitch> sw, UINT4 port_no, INT4 event, INT4 reason, INT4 type, INT4 state, INT4 stateNew);

    const char* toString() {return "CPortEventReportor";}
    
private:
    CPortEventReportor();

    CMsgPath getMsgPath(INT4 event);
    CMsgKey getMsgKey(UINT8 dpid, UINT4 port);

private:
    static CPortEventReportor* m_pInstance;       
};


#endif

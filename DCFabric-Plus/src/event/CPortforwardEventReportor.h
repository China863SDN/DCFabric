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
*   File Name   : CPortforwardEventReportor.h                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CPORTFORWARDEVENTREPORTOR_H
#define __CPORTFORWARDEVENTREPORTOR_H

#include "bnc-type.h"
#include "CEventReportor.h"
#include "CPortforwardEvent.h"

class CPortforwardEventReportor : public CEventReportor
{
public:
    static CPortforwardEventReportor* getInstance();

    ~CPortforwardEventReportor();

    INT4 report(INT4 event, INT4 reason, const CPortforwardRule& rule);

    const char* toString() {return "CPortforwardEventReportor";}

private:
    CPortforwardEventReportor();

    CMsgPath getMsgPath(INT4 event);

private:
    static CPortforwardEventReportor* m_pInstance;       
};

#endif

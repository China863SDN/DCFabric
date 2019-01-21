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
*   File Name   : CEventReportor.h                                            *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CEVENTREPORTOR_H
#define __CEVENTREPORTOR_H

#include "bnc-type.h"
#include "CEvent.h"
#include "CMsgOperator.h"
#include "CMutex.h"

extern const char* g_switchEventPath[];
extern const char* g_portEventPath[];
extern const char* g_topoLinkEventPath[];
extern const char* g_NetworkingEventPath[];
extern const char* g_securityGroupEventPath[];
extern const char* g_portforwardEventPath[];
extern const char* g_qosPolicyEventPath[];
extern const char* g_flowTableEventPath[];
extern const char* g_tagflowEventPath[];

/*
 * 事件生产者基类
 */
class CEventReportor : public CMsgOperator
{
public:
    CEventReportor();
    virtual ~CEventReportor();

    INT4 onregister(CMsgPath& path);
    void deregister(CMsgPath& path);

    INT4 report(CEvent* event);

    virtual const char* toString() {return "CEventReportor";}

private:
    virtual CMsgPath getMsgPath(INT4 event) = 0;
    BOOL isRegistered(CMsgPath& path);

private:
    std::map<CMsgPath, BOOL> m_paths;
    CMutex m_mutex;
};

#endif

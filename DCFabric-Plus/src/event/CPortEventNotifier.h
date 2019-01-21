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
*   File Name   : CPortEventNotifier.h                                          *
*   Author      : bnc cyyang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CPORTEVENTNOTIFIER_H
#define _CPORTEVENTNOTIFIER_H

#include "CEventNotifier.h"

/*
 * 端口事件测试类
 */
class CPortEventNotifier : public CEventNotifier
{
public:
    CPortEventNotifier(){};
    ~CPortEventNotifier(){};
	INT4 onregister() ;
	void deregister() ;
	

    const char* toString() {return "CPortEventNotifier";}

private:
    INT4 consume(CSmartPtr<CMsgCommon> evt);
};


#endif

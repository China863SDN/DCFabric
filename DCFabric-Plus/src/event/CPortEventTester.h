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
*   File Name   : CPortEventTester.h                                          *
*   Author      : bnc jqjiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CPORTEVENTTESTER_H
#define _CPORTEVENTTESTER_H

#include "CEventConsumer.h"

/*
 * 端口事件测试类
 */
class CPortEventTester : public CEventConsumer
{
public:
    CPortEventTester();
    ~CPortEventTester();

    void test();
    const char* toString() {return "CPortEventTester";}

private:
    INT4 consume(CSmartPtr<CMsgCommon> evt);
};


#endif

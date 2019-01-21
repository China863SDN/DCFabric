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
*   File Name   : CSwitchEventTester.h                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CSWITCHEVENTTESTER_H
#define _CSWITCHEVENTTESTER_H

#include "CEventConsumer.h"

/*
 * 交换机事件测试类
 */
class CSwitchEventTester : public CEventConsumer
{
public:
    CSwitchEventTester();
    ~CSwitchEventTester();

    void test(INT4 event);
    const char* toString() {return "CSwitchEventTester";}

private:
    INT4 consume(CSmartPtr<CMsgCommon> evt);
};


#endif

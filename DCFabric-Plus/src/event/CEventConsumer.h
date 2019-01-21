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
*   File Name   : CEventConsumer.h                                            *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CEVENTCONSUMER_H
#define _CEVENTCONSUMER_H

#include "CMsgOperator.h"
#include "CMsgTreeNode.h"

/*
 * 事件主动消费者基类
 */
class CEventConsumer : public CMsgOperator
{
public:
    CEventConsumer();
    virtual ~CEventConsumer();

    INT4 consume(CMsgPath& path, BOOL integrated=FALSE);

    virtual const char* toString() {return "CEventConsumer";}

private:
    INT4 onregister(CMsgPath& path, BOOL integrated);
    void deregister(CMsgPath& path);
    virtual INT4 consume(CSmartPtr<CMsgQueue> queue);
    virtual INT4 consume(CSmartPtr<CMsgCommon> evt);
    BOOL isRegistered(CMsgPath& path);    

private:
    std::map<CMsgPath, BOOL> m_paths;
};


#endif

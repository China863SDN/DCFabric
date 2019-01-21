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
*   File Name   : CMsgHandlerMgr.cpp                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-param.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "COfMsgUtil.h"
#include "CMsgHandlerMgr.h"
#include "COfHelloHandler.h"
#include "CHeartbeatHandler.h"
#include "COfFeaturesReplyHandler.h"
#include "COfGetConfigReplyHandler.h"
#include "COfMultipartReplyHandler.h"
#include "COfBarrierReplyHandler.h"
#include "COfRoleReplyHandler.h"
#include "CTopoDiscovery.h"
#include "COfPortStatusHandler.h"
#include "CEtherIPHandler.h"
#include "CEtherArpHandler.h"
//#include "NetworkingEventConsumer.h"
#include "COfFlowRemovedHandler.h"

//#include "../networking/BaseSubnetManager.h"
//#include "../networking/BaseFloatingManager.h"
//#include "../networking/BaseNetworkManager.h"
//#include "../networking/BaseRouterManager.h"
//#include "../networking/BasePortManager.h"

CMsgHandlerMgr::CMsgHandlerMgr()
{
}

CMsgHandlerMgr::~CMsgHandlerMgr()
{
}

INT4 CMsgHandlerMgr::init()
{
    //register OpenFlow message handler here
    CMsgHandler* handler = new COfHelloHandler();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

    handler = new CHeartbeatHandler();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

    handler = new COfFeaturesReplyHandler();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

    handler = new COfGetConfigReplyHandler();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

    handler = new COfMultipartReplyHandler();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

    handler = new COfBarrierReplyHandler();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

    handler = new COfRoleReplyHandler();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

    handler = new CTopoDiscovery();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

    handler = new COfPortStatusHandler();
    if (NULL != handler)
    {
        if (handler->onregister() == BNC_OK)
            m_handlers.push_back(handler);
        else
            delete handler;
    }

	handler = new CEtherArpHandler();
	if (NULL != handler)
	{
	   if (handler->onregister() == BNC_OK)
		   m_handlers.push_back(handler);
	   else
		   delete handler;
	}

	handler = new CEtherIPHandler();
	if (NULL != handler)
	{
	   if (handler->onregister() == BNC_OK)
		   m_handlers.push_back(handler);
	   else
		   delete handler;
	}

	handler = new COfFlowRemovedHandler();
	if (NULL != handler)
	{
	   if (handler->onregister() == BNC_OK)
		   m_handlers.push_back(handler);
	   else
		   delete handler;
	}

    //add more ...

    //others to be added here

    return BNC_OK;
}

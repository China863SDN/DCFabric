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
*   File Name   : COfGetConfigReplyHandler.cpp                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COfGetConfigReplyHandler.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "COfMsgUtil.h"
#include "CControl.h"
#include "CRecvWorker.h"
#include "CServer.h"
#include "COfConnectMgr.h"
#include "log.h"
#include "bnc-error.h"

COfGetConfigReplyHandler::COfGetConfigReplyHandler()
{
}

COfGetConfigReplyHandler::~COfGetConfigReplyHandler()
{
}

INT4 COfGetConfigReplyHandler::onregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_GET_CONFIG_REPLY);
    return CMsgHandler::onregister(path, 1);
}

void COfGetConfigReplyHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_GET_CONFIG_REPLY);
    CMsgHandler::deregister(path);
}

INT4 COfGetConfigReplyHandler::handle(CSmartPtr<CMsgCommon> msg)
{
    CMsg* ofmsg = (CMsg*)msg.getPtr();

    LOG_DEBUG_FMT("%s[%p] handle new msg of path[%s] on sockfd[%d] ...", 
        toString(), this, ofmsg->getPath().c_str(), ofmsg->getSockfd());

    INT4 sockfd = ofmsg->getSockfd();
    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (sw.isNull())
    {
        LOG_WARN_FMT("CSwitch not created for sockfd[%d] !", sockfd);
        COfConnectMgr::processSystemFailure(sockfd);
        return BNC_ERR;
    }
    
    INT1* data = ofmsg->getData();
    if (NULL == data)
    {
        LOG_WARN_FMT("%s[%p] received msg with no data from sockfd[%d] !", 
            toString(), this, sockfd);
        return BNC_ERR;
    }

    LOG_DEBUG_FMT("send OFPT13_BARRIER_REQUEST to switch with sockfd[%d] ...", sockfd);
    BOOL ret = COfMsgUtil::sendOfpMsg(sockfd, OFP13_VERSION, OFPT13_BARRIER_REQUEST);
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_BARRIER_REQUEST to switch with sockfd[%d] failed !", sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    return BNC_OK;
}


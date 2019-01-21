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
*   File Name   : COfHelloHandler.cpp                                         *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COfHelloHandler.h"
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

COfHelloHandler::COfHelloHandler()
{
}

COfHelloHandler::~COfHelloHandler()
{
}

INT4 COfHelloHandler::onregister()
{
    CMsgPath path1 = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_HELLO);
    if (CMsgHandler::onregister(path1, 1, FALSE, THREAD_TYPE_OCCUPY_CORE))
    {
        LOG_WARN_FMT("COfHelloHandler register to path[%s] failed !", path1.c_str());
        return BNC_ERR;
    }

    CMsgPath path2 = COfMsgUtil::getMsgPath(OFP10_VERSION, OFPT13_HELLO);
    if (CMsgHandler::onregister(path2, 1))
    {
        LOG_WARN_FMT("COfHelloHandler register to path[%s] failed !", path2.c_str());
        CMsgHandler::deregister(path1);
        return BNC_ERR;
    }

    return BNC_OK;
}

void COfHelloHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP10_VERSION, OFPT13_HELLO);
    CMsgHandler::deregister(path);

    path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_HELLO);
    CMsgHandler::deregister(path);
}

INT4 COfHelloHandler::handle(CSmartPtr<CMsgCommon> msg)
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

    UINT1 version = ((struct ofp_header*)data)->version;
    if (OFP13_VERSION != version)
    {
        LOG_WARN_FMT("%s[%p] received HELLO with unsupported version[%d] from sockfd[%d] !", 
            toString(), this, version, sockfd);
        COfConnectMgr::processPeerQuit(sw, EVENT_REASON_NEGO_VER_FAIL);
        return BNC_ERR;
    }

    sw->setVersion(version);
    sw->setState(SW_STATE_CONNECTED);

    LOG_DEBUG_FMT("send OFPT13_HELLO to switch with sockfd[%d] ...", sockfd);
    LOG_DEBUG_FMT("send OFPT13_FEATURES_REQUEST to switch with sockfd[%d] ...", sockfd);
    BOOL ret1 = COfMsgUtil::sendOfpMsg(sockfd, OFP13_VERSION, OFPT13_HELLO);
    BOOL ret2 = COfMsgUtil::sendOfpMsg(sockfd, OFP13_VERSION, OFPT13_FEATURES_REQUEST);
    if (!ret1 || !ret2)
    {
        LOG_WARN_FMT("send OFPT13_HELLO or OFPT13_FEATURES_REQUEST to switch with sockfd[%d] failed !", sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    return BNC_OK;
}


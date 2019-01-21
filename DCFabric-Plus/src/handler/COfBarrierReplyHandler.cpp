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
*   File Name   : COfBarrierReplyHandler.cpp                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COfBarrierReplyHandler.h"
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
#include "comm-util.h"
#include "CFlowMgr.h"
#include "CClusterService.h"
#include "SwitchManageMode.h"
#include "SwitchManagePresistence.h"

COfBarrierReplyHandler::COfBarrierReplyHandler()
{
}

COfBarrierReplyHandler::~COfBarrierReplyHandler()
{
}

INT4 COfBarrierReplyHandler::onregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_BARRIER_REPLY);
    return CMsgHandler::onregister(path, 1);
}

void COfBarrierReplyHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_BARRIER_REPLY);
    CMsgHandler::deregister(path);
}

INT4 COfBarrierReplyHandler::handle(CSmartPtr<CMsgCommon> msg)
{
    CMsg* ofmsg = (CMsg*)msg.getPtr();

    LOG_DEBUG_FMT("%s[%p] handle new msg of path[%s] on sockfd[%d] ...", 
        toString(), this, ofmsg->getPath().c_str(), ofmsg->getSockfd());

    INT4 sockfd = ofmsg->getSockfd();

    INT1* data = ofmsg->getData();
    if (NULL == data)
    {
        LOG_WARN_FMT("%s[%p] received msg with no data from sockfd[%d] !", 
            toString(), this, sockfd);
        return BNC_ERR;
    }

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (sw.isNull())
    {
        LOG_WARN_FMT("CSwitch not created for sockfd[%d] !", sockfd);
        COfConnectMgr::processSystemFailure(sockfd);
        return BNC_ERR;
    }
    SwitchManageMode*  switch_manage = SwitchManagePresistence::GetInstance()->findManageSwitchByDpid(sw->getDpid());
	if(NULL != switch_manage)
	{
		sw->setManageMode(switch_manage->GetSwitchManageMode());
		//LOG_DEBUG_FMT("sw->getDpid()=0x%llx sw->getManageMode()=%d", sw->getDpid(), sw->getManageMode());
	}
	else
	{
		//LOG_DEBUG_FMT("sw->getDpid()=0x%llx SwitchManagePresistence::GetInstance()->isGlobalManageModeOn()=%d SWITCH_MONITOR_FLAG=%d", sw->getDpid(),SwitchManagePresistence::GetInstance()->isGlobalManageModeOn(),SWITCH_MONITOR_FLAG);
		if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn())
		{
			sw->setManageMode(SWITCH_MONITOR_FLAG);
		}
		else
		{
			sw->setManageMode(SWITCH_MANAGE_FLAG);
		}
	}
    port_stats_req_data_t port_stats_req_data = {0};
    port_stats_req_data.port_no = OFPP13_ANY;
    LOG_DEBUG_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_PORT_STATS to switch with sockfd[%d] ...", sockfd);
    BOOL ret = COfMsgUtil::sendOfp13MultiPartRequest(sockfd, OFPMP_PORT_STATS, &port_stats_req_data);
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_PORT_STATS to switch with sockfd[%d] failed !", sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    //COfMsgUtil::sendOfp13TableMiss(sockfd);

    //TBD

    sw->setState(SW_STATE_STABLE);

    COfConnectMgr::processPeerEnterStable(sw);

    if (OFPCR_ROLE_SLAVE != CClusterService::getInstance()->getControllerRole())
    {
    	CFlowMgr::getInstance()->clear_flow(sw);
    	CFlowMgr::getInstance()->install_base_dcfabric_flows(sw);
    	CFlowMgr::getInstance()->install_base_default_gotonext_flow(sw);
    }
    return BNC_OK;
}


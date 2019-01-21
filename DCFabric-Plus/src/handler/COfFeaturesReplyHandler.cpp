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
*   File Name   : COfFeaturesReplyHandler.cpp                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COfFeaturesReplyHandler.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "COfMsgUtil.h"
#include "CControl.h"
#include "CRecvWorker.h"
#include "CClusterService.h"
#include "COfConnectMgr.h"
#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CClusterSync.h"
#include "SwitchManageMode.h"
#include "SwitchManagePresistence.h"


COfFeaturesReplyHandler::COfFeaturesReplyHandler()
{
}

COfFeaturesReplyHandler::~COfFeaturesReplyHandler()
{
}

INT4 COfFeaturesReplyHandler::onregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_FEATURES_REPLY);
    return CMsgHandler::onregister(path, 1);
}

void COfFeaturesReplyHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_FEATURES_REPLY);
    CMsgHandler::deregister(path);
}

INT4 COfFeaturesReplyHandler::handle(CSmartPtr<CMsgCommon> msg)
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

    struct ofp13_switch_features* features = (struct ofp13_switch_features*)data;
    if (0 == features->datapath_id)
    {
        LOG_WARN_FMT("invalid DPID in features reply via sockfd[%d] !", sockfd);
        COfConnectMgr::processSystemFailure(sockfd);
        return BNC_ERR;
    }

    sw->setDpid(ntohll(features->datapath_id));
    sw->setNBuffers(ntohl(features->n_buffers));
    sw->setNTables(features->n_tables);
    sw->setCapabilities(ntohl(features->capabilities));
	
    LOG_DEBUG_FMT("send OFPT13_SET_CONFIG to switch[%llx] with sockfd[%d] ...", sw->getDpid(), sockfd);

	SwitchManageMode*  switch_manage = SwitchManagePresistence::GetInstance()->findManageSwitchByDpid(sw->getDpid());
	if(NULL != switch_manage)
	{
		sw->setManageMode(switch_manage->GetSwitchManageMode());
		//LOG_DEBUG_FMT("sw->getDpid()=0x%llx sw->getManageMode()=%d", sw->getDpid(), sw->getManageMode());
	}
	else
	{
		if(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn())
		{
			sw->setManageMode(SWITCH_MONITOR_FLAG);
		}
		else
		{
			sw->setManageMode(SWITCH_MANAGE_FLAG);
		}
	}

    BOOL ret = COfMsgUtil::sendOfp13SetConfig(sockfd);
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_SET_CONFIG to switch[%llx] with sockfd[%d] failed !", sw->getDpid(), sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    LOG_DEBUG_FMT("send OFPT13_GET_CONFIG_REQUEST to switch[%llx] with sockfd[%d] ...", sw->getDpid(), sockfd);
    ret = COfMsgUtil::sendOfpMsg(sockfd, OFP13_VERSION, OFPT13_GET_CONFIG_REQUEST);
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_GET_CONFIG_REQUEST to switch[%llx] with sockfd[%d] failed !", sw->getDpid(), sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    LOG_DEBUG_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_DESC to switch[%llx] with sockfd[%d] ...", sw->getDpid(), sockfd);
    ret = COfMsgUtil::sendOfp13MultiPartRequest(sockfd, OFPMP_DESC);
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_DESC to switch[%llx] with sockfd[%d] failed !", sw->getDpid(), sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    LOG_DEBUG_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_PORT_DESC to switch[%llx] with sockfd[%d] ...", sw->getDpid(), sockfd);
    ret = COfMsgUtil::sendOfp13MultiPartRequest(sockfd, OFPMP_PORT_DESC);
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_PORT_DESC to switch[%llx] with sockfd[%d] failed !", sw->getDpid(), sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    LOG_DEBUG_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_TABLE to switch[%llx] with sockfd[%d] ...", sw->getDpid(), sockfd);
    ret = COfMsgUtil::sendOfp13MultiPartRequest(sockfd, OFPMP_TABLE);
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_TABLE to switch[%llx] with sockfd[%d] failed !", sw->getDpid(), sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    LOG_DEBUG_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_METER_FEATURES to switch[%llx] with sockfd[%d] ...", sw->getDpid(), sockfd);
    ret = COfMsgUtil::sendOfp13MultiPartRequest(sockfd, OFPMP_METER_FEATURES);
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_MULTIPART_REQUEST|OFPMP_METER_FEATURES to switch[%llx] with sockfd[%d] failed !", sw->getDpid(), sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    LOG_DEBUG_FMT("send OFPT13_ROLE_REQUEST with role[%d] to switch[%llx] with sockfd[%d] ...", 
        CClusterService::getInstance()->getControllerRole(), sw->getDpid(), sockfd);
    ret = COfMsgUtil::sendOfp13RoleRequest(sockfd, CClusterService::getInstance()->getControllerRole());
    if (!ret)
    {
        LOG_WARN_FMT("send OFPT13_ROLE_REQUEST with role[%d] to switch[%llx] with sockfd[%d] failed !", 
            CClusterService::getInstance()->getControllerRole(), sw->getDpid(), sockfd);
        COfConnectMgr::processPeerEnterUnreach(sw);
        return BNC_ERR;
    }

    //TBD: QoS

    CControl::getInstance()->getSwitchMgr().addDpidMap(sw->getDpid(), sw);    

    CControl::getInstance()->getTopoMgr().addSwitch(sw->getDpid());

    CClusterSync::syncSwitchTag(sw->getDpid(), sw->getTag());

    return BNC_OK;
}


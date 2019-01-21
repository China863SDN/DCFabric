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
*   File Name   : COfPortStatusHandler.cpp                                    *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COfPortStatusHandler.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "COfMsgUtil.h"
#include "CControl.h"
#include "CRecvWorker.h"
#include "CServer.h"
#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "COfMultipartReplyHandler.h"
#include "CConf.h"
#include "COfConnectMgr.h"
#include "CEvent.h"
#include "CPortEvent.h"
#include "CEventReportor.h"
#include "CPortEventReportor.h"

COfPortStatusHandler::COfPortStatusHandler()
{
}

COfPortStatusHandler::~COfPortStatusHandler()
{
}

INT4 COfPortStatusHandler::onregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PORT_STATUS);
    return CMsgHandler::onregister(path, 1);
}

void COfPortStatusHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PORT_STATUS);
    CMsgHandler::deregister(path);
}

INT4 COfPortStatusHandler::handle(CSmartPtr<CMsgCommon> msg)
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

    struct ofp13_port_status *status = (struct ofp13_port_status *)data;
    if (ntohs(status->header.length) < sizeof(struct ofp13_port_status))
    {
        LOG_WARN_FMT("%s[%p] received invalid port status msg from sockfd[%d] !", 
            toString(), this, sockfd);
        return BNC_ERR;
    }

    UINT4 port_no = ntohl(status->desc.port_no);
    INT4 state = ntohl(status->desc.state);
	INT4 newState =
        (0 == state) ? PORT_STATE_UP :
        (OFPPS13_LINK_DOWN == state) ? PORT_STATE_DOWN :
        (OFPPS13_BLOCKED == state) ? PORT_STATE_BLOCKED :
        (OFPPS13_LIVE == state) ? PORT_STATE_FAILOVER : PORT_STATE_DOWN;

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (sw.isNull())
    {
        LOG_WARN_FMT("CSwitch not created for sockfd[%d] !", sockfd);
        COfConnectMgr::processSystemFailure(sockfd);
        return BNC_ERR;
    }

	INT4 curState = PORT_STATE_INIT;
    gn_port_t* port = sw->getPort(port_no);
    if (NULL != port)
	    curState = port->state;

    LOG_DEBUG_FMT("%s[%p] received OFPT13_PORT_STATUS with reason[%d] on sockfd[%d] ...", 
        toString(), this, status->reason, sockfd);

    switch (status->reason)
    {
        case OFPPR_ADD:
        {
            LOG_DEBUG_FMT("%s[%p] handle OFPT13_PORT_STATUS|OFPPR_ADD port[%u]state[%u] ...", 
                toString(), this, port_no, state);
            COfMultipartReplyHandler::handlePortDesc(sw, &(status->desc));
			CPortEventReportor::getInstance()->report(
                sw,
                port_no,
                (PORT_STATE_UP==newState)?EVENT_TYPE_PORT_UP:EVENT_TYPE_PORT_DOWN,
                (PORT_STATE_UP==newState)?EVENT_REASON_PORT_UP:EVENT_REASON_PORT_DOWN,
                PORT_EVENT_ADD,
                PORT_STATE_INIT,
                newState);
            break;
        }
		
        case OFPPR_MODIFY:
        {
            LOG_DEBUG_FMT("%s[%p] handle OFPT13_PORT_STATUS|OFPPR_MODIFY port[%u]state[%u] ...", 
                toString(), this, port_no, state);
            COfMultipartReplyHandler::handlePortDesc(sw, &(status->desc));
            if (curState != newState)
    			CPortEventReportor::getInstance()->report(
                    sw,
                    port_no,
                    (PORT_STATE_UP==newState)?EVENT_TYPE_PORT_UP:EVENT_TYPE_PORT_DOWN,
                    (PORT_STATE_UP==newState)?EVENT_REASON_PORT_UP:EVENT_REASON_PORT_DOWN,
                    PORT_EVENT_MODIFY,
                    curState,
                    newState);
            break;
        }
        case OFPPR_DELETE:
        {
            LOG_DEBUG_FMT("%s[%p] handle OFPT13_PORT_STATUS|OFPPR_DELETE port[%u]state[%u] ...", 
                toString(), this, port_no, state);
            //TBD: flow table
            sw->deletePort(port_no);
            CControl::getInstance()->getTopoMgr().deleteLink(sw->getDpid(), port_no);
			CPortEventReportor::getInstance()->report(
                sw,
                port_no,
                EVENT_TYPE_PORT_DOWN,
                EVENT_REASON_PORT_DOWN,
                PORT_EVENT_DELETE,
                curState,
                newState);
			break;
        }
        default:
        {
            break;
        }
    }

    return BNC_OK;
}


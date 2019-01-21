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
*   File Name   : COfMultipartReplyHandler.cpp                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COfMultipartReplyHandler.h"
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
#include "CConf.h"
#include "CPhysicalSwitch.h"
#include "CPica8PhySwitch.h"

COfMultipartReplyHandler::COfMultipartReplyHandler()
{
}

COfMultipartReplyHandler::~COfMultipartReplyHandler()
{
}

INT4 COfMultipartReplyHandler::onregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_MULTIPART_REPLY);
    return CMsgHandler::onregister(path, 1);
}

void COfMultipartReplyHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_MULTIPART_REPLY);
    CMsgHandler::deregister(path);
}

INT4 COfMultipartReplyHandler::handle(CSmartPtr<CMsgCommon> msg)
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

    struct ofp13_multipart_reply *reply = (struct ofp13_multipart_reply *)data;
    if (ntohs(reply->header.length) < sizeof(struct ofp13_multipart_reply))
    {
        LOG_WARN_FMT("%s[%p] received invalid multipart reply msg from sockfd[%d] !", 
            toString(), this, sockfd);
        return BNC_ERR;
    }
    UINT2 bodyLen = ntohs(reply->header.length) - sizeof(struct ofp_multipart_reply);

    LOG_DEBUG_FMT("%s[%p] received OFPT13_MULTIPART_REPLY with subtype[%d] on sockfd[%d] ...", 
        toString(), this, ntohs(reply->type), sockfd);
    switch (ntohs(reply->type))
    {
        case OFPMP_DESC:
        {
            LOG_DEBUG_FMT("%s[%p] handle OFPMP_DESC ...", toString(), this);
            switch_desc_t *desc = (switch_desc_t *)(reply->body);
            sw->setSwDesc(*desc);
            if (strncasecmp(desc->mfr_desc, "Nicira", 6) != 0)
            {
                CPhysicalSwitch* pphySw =( 0 != strncasecmp(desc->mfr_desc, "Pica8", 5)) ? new CPhysicalSwitch(sw): new CPica8PhySwitch(sw);
				
               //CPhysicalSwitch* pphySw = new CPhysicalSwitch(sw);
				if (NULL == pphySw)
                {
                    LOG_ERROR_FMT("new CPhysicalSwitch failed[%d]!", errno);
                    return BNC_ERR;
                }
				pphySw->setSwType(SWITCH_PHYSICAL);
				LOG_ERROR_FMT("sw->getDpid()=0x%llx sw->getManageMode()=%d", sw->getDpid(), sw->getManageMode());
                CSmartPtr<CSwitch> phySw(pphySw);
                phySw->setTag(sw->getTag());
                CControl::getInstance()->getSwitchMgr().updSwitch(sw, sockfd, phySw);
				
				LOG_ERROR_FMT("phySw->getDpid()=0x%llx phySw->getManageMode()=%d", phySw->getDpid(), phySw->getManageMode());
            }
            break;
        }
        case OFPMP_PORT_DESC:
        {
            LOG_DEBUG_FMT("%s[%p] handle OFPMP_PORT_DESC ...", toString(), this);
            struct ofp13_port* ports = (struct ofp13_port*)(reply->body);
            UINT2 count = bodyLen / sizeof(struct ofp13_port);
            handlePortDesc(sw, ports, count);
            break;
        }
        case OFPMP_TABLE_FEATURES:
        {
            LOG_DEBUG_FMT("%s[%p] do nothing with OFPMP_TABLE_FEATURES ...", toString(), this);
            break;
        }
        case OFPMP_TABLE:
        {
            LOG_DEBUG_FMT("%s[%p] do nothing with OFPMP_TABLE ...", toString(), this);
            break;
        }
        case OFPMP_GROUP_FEATURES:
        {
            LOG_DEBUG_FMT("%s[%p] do nothing with OFPMP_GROUP_FEATURES ...", toString(), this);
            break;
        }
        case OFPMP_METER_FEATURES:
        {
            LOG_DEBUG_FMT("%s[%p] do nothing with OFPMP_METER_FEATURES ...", toString(), this);
            break;
        }
        case OFPMP_FLOW:
        {
            LOG_DEBUG_FMT("%s[%p] TBD OFPMP_METER_FEATURES ...", toString(), this);
            break;
        }
        case OFPMP_PORT_STATS:
        {
            LOG_DEBUG_FMT("%s[%p] handle OFPMP_PORT_STATS ...", toString(), this);
            struct ofp13_port_stats* stats = (struct ofp13_port_stats*)(reply->body);
            UINT2 count = bodyLen / sizeof(struct ofp13_port_stats);
            handlePortStats(sw, stats, count);
            break;
        }
        default:
        {
            break;
        }
    }

    return BNC_OK;
}

INT4 COfMultipartReplyHandler::handlePortDesc(CSmartPtr<CSwitch>& sw, struct ofp13_port* ports, UINT2 count)
{
    for (UINT2 i = 0; i < count; i++)
    {
        gn_port_t newPort = {0};
        COfMsgUtil::ofp13PortConvertter(&(ports[i]), &newPort);
        newPort.stats.max_speed = 10737418; //1073741824 = 1024^3, 1048576 = 1024^2 10737418=1024^3/100
    
        if (newPort.port_no == OFPP13_LOCAL)
        {
            newPort.type = PORT_TYPE_MGMT;
            sw->setLoPort(newPort);
            continue;
        }
    
        sw->updatePort(newPort);

        if (PORT_STATE_UP == newPort.state)
        {
            LOG_DEBUG_FMT("MULTIPART-REPLY: send OFPT13_PACKET_OUT|LLDP to switch port[%u] with sockfd[%d] ...",
                newPort.port_no, sw->getSockfd());
            BOOL ret = COfMsgUtil::sendOfp13Lldp(sw, newPort);
            if (!ret)
            {
                LOG_WARN_FMT("MULTIPART-REPLY: send OFPT13_PACKET_OUT|LLDP to switch port[%u] with sockfd[%d] failed !", 
                    newPort.port_no, sw->getSockfd());
                COfConnectMgr::processPeerEnterUnreach(sw);
            }
        }
        else
        {
            CControl::getInstance()->getTopoMgr().deleteLink(sw->getDpid(), newPort.port_no);
        }
    }

    return BNC_OK;
}

INT4 COfMultipartReplyHandler::handlePortStats(CSmartPtr<CSwitch>& sw, struct ofp13_port_stats* stats, UINT2 count)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    UINT4 timestamp = time.tv_sec;

    for (UINT2 i = 0; i < count; i++)
    {
        gn_port_t* port = sw->getPort(ntohl(stats[i].port_no));
        if (NULL != port)
        {
            struct ofp13_port_stats newStats = {0};
            COfMsgUtil::ofp13PortStatsConvertter(&(stats[i]), &newStats);

            UINT4 duration_sec = newStats.duration_sec - port->stats.duration_sec;
            if (0 == duration_sec)
                continue;

            if (0 != port->stats.timestamp)
            {
                port->stats.rx_kbps = (newStats.rx_bytes - port->stats.rx_bytes) / duration_sec;
                port->stats.tx_kbps = (newStats.tx_bytes - port->stats.tx_bytes) / duration_sec;
                port->stats.rx_kpps = (newStats.rx_packets - port->stats.rx_packets) / duration_sec;
                port->stats.tx_kpps = (newStats.tx_packets - port->stats.tx_packets) / duration_sec;
            }
            
            port->stats.rx_packets = newStats.rx_packets;
            port->stats.tx_packets = newStats.tx_packets;
            port->stats.rx_bytes = newStats.rx_bytes;
            port->stats.tx_bytes = newStats.tx_bytes;
            port->stats.duration_sec = newStats.duration_sec;
            port->stats.timestamp = timestamp;
        }
    }

    return BNC_OK;
}



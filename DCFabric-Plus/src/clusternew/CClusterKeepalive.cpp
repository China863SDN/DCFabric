/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
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
*   File Name   : cluster_keepalive.cpp                                       *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#include "CClusterDefine.h"
#include "CClusterKeepalive.h"
#include "CClusterService.h"
#include "CClusterElect.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "log.h"
#include "CServer.h"

CTimer CClusterKeepalive::m_timer;

INT4 CClusterKeepalive::init()
{
    if (m_timer.schedule(keepalive_interval, keepalive_interval, keepalive, NULL) != BNC_OK)
    {
        LOG_ERROR("CClusterKeepalive schedule keepalive failed !");
        return BNC_ERR;
    }

    return BNC_OK;
}

void CClusterKeepalive::keepalive(void* param)
{
    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if ((CServer::getInstance()->getControllerIp() != controller.ip) &&
            ((CLUSTER_CONTROLLER_CONNECTED == controller.state) || 
             (CLUSTER_CONTROLLER_NORESPONSE == controller.state)))
        {
            if (controller.karetry < keepalive_retry_times)
            {
                sendReq(controller.sockfd);
                controller.karetry ++;
                if (controller.karetry > 1)
                    controller.state = CLUSTER_CONTROLLER_NORESPONSE;
            }
            else
            {
                LOG_WARN_FMT("$$$ remote CLUSTER %s[%s]sockfd[%d] unreachable $$$", 
                    (CClusterElection::getMasterIp()==controller.ip)?"MASTER":"SLAVE",
                    inet_htoa(controller.ip), controller.sockfd);
                controller.state = CLUSTER_CONTROLLER_UNREACHABLE;
                INT4 ret = CClusterService::getInstance()->connectRemote(controller.ip);
                if ((BNC_OK != ret) && (CClusterElection::getMasterIp() == controller.ip))
                {
                    CClusterElection::start();
                }
            }
        }
    }
}

INT4 CClusterKeepalive::processReq(INT4 sockfd, cluster_interface_t* itf)
{
    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if (sockfd == controller.sockfd)
        {
            controller.state = CLUSTER_CONTROLLER_CONNECTED;
            controller.karetry = 0;
            break;
        }
    }
    
    cluster_keepalive_req_t* req = (cluster_keepalive_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_KEEPALIVE_REQ) || (itf->length < (INT4)sizeof(cluster_keepalive_req_t)))
    {
        LOG_WARN_FMT("invalid keepalive req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }
    if (req->master != CClusterElection::getMasterIp())
    {
        LOG_WARN_FMT("CLUSTER keepalive told master[0x%x], but my master[0x%x]", 
            req->master, CClusterElection::getMasterIp());
        //TBD
    }

    return sendRsp(sockfd);
}

INT4 CClusterKeepalive::processRsp(INT4 sockfd, cluster_interface_t* itf)
{
    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if (sockfd == controller.sockfd)
        {
            controller.state = CLUSTER_CONTROLLER_CONNECTED;
            controller.karetry = 0;
            break;
        }
    }
    
    cluster_keepalive_rsp_t* rsp = (cluster_keepalive_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_KEEPALIVE_RSP) || (itf->length < (INT4)sizeof(cluster_keepalive_rsp_t)))
    {
        LOG_WARN_FMT("invalid keepalive rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }
    if (rsp->master != CClusterElection::getMasterIp())
    {
        LOG_WARN_FMT("CLUSTER keepalive told master[0x%x], but my master[0x%x]", 
            rsp->master, CClusterElection::getMasterIp());
        //TBD
    }

    return BNC_OK;
}

INT4 CClusterKeepalive::sendReq(INT4 sockfd)
{
    INT4 length = sizeof(cluster_interface_t) + sizeof(cluster_keepalive_req_t);
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = CLUSTER_OPER_KEEPALIVE_REQ;
    itf->length = sizeof(cluster_keepalive_req_t);

    cluster_keepalive_req_t* req = (cluster_keepalive_req_t*)ptr;
    req->master = CClusterElection::getMasterIp();

    return sendMsgOut(sockfd, buffer, length)?BNC_OK:BNC_ERR;
}

INT4 CClusterKeepalive::sendRsp(INT4 sockfd)
{
    INT4 length = sizeof(cluster_interface_t) + sizeof(cluster_keepalive_rsp_t);
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = CLUSTER_OPER_KEEPALIVE_RSP;
    itf->length = sizeof(cluster_keepalive_rsp_t);

    cluster_keepalive_rsp_t* rsp = (cluster_keepalive_rsp_t*)ptr;
    rsp->cause = CLUSTER_CAUSE_SUCC;
    rsp->master = CClusterElection::getMasterIp();

    return sendMsgOut(sockfd, buffer, length)?BNC_OK:BNC_ERR;
}


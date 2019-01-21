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
*   File Name   : cluster_elect.cpp                                       *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#include <stdio.h>  
#include <stdlib.h>  
#include "CClusterDefine.h"
#include "CClusterElect.h"
#include "CClusterInterface.h"
#include "CClusterSync.h"
#include "CClusterKeepalive.h"
#include "CClusterService.h"
#include "bnc-error.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "CConf.h"
#include "CServer.h"
#include "bnc-inet.h"
#include "comm-util.h"
#include "log.h"

extern UINT1 BROADCAST_MAC[];

INT4        CClusterElection::m_state = CLUSTER_ELECTION_INIT;
UINT4       CClusterElection::m_masterIp = 0;
CTimer      CClusterElection::m_timer;
CMutex      CClusterElection::m_mutex;

INT4 CClusterElection::start()
{
    m_mutex.lock();
    
    UINT4 proposal = generateProposal0();
    if (proposal > 0)
    {
        if (proposal < CServer::getInstance()->getControllerIp())
            proposal = CServer::getInstance()->getControllerIp();

        LOG_WARN_FMT(">>> start election, my proposal is %s ...", inet_htoa(proposal));
        m_masterIp = proposal;
        m_state = CLUSTER_ELECTION_START;

        broadcastProposal(proposal);

        //election will end after some interval
        m_timer.schedule(ELECTION_TIMEOUT_SECOND, 0, terminate, NULL);
    }
    else
    {
        m_timer.stop();

        LOG_WARN_FMT(">>> no proposal, i am MASTER <<<");
        m_masterIp = CServer::getInstance()->getControllerIp();
        m_state = CLUSTER_ELECTION_FINISHED;
        CClusterService::getInstance()->updateRole(OFPCR_ROLE_MASTER);
    }

    m_mutex.unlock();

    return BNC_OK;
}

void CClusterElection::terminate(void* param)
{
    //LOG_INFO_FMT("determined master[%s]", inet_htoa(m_masterIp));

    m_mutex.lock();
    
    if (CLUSTER_ELECTION_FINISHED != m_state)
    {
        LOG_WARN_FMT("<<< after election, MASTER is %s, i am %s", 
            inet_htoa(m_masterIp), 
            (m_masterIp==CServer::getInstance()->getControllerIp())?"MASTER":"SLAVE");
        
        m_state = CLUSTER_ELECTION_FINISHED;
        CClusterService::getInstance()->updateRole(
            (m_masterIp==CServer::getInstance()->getControllerIp())?OFPCR_ROLE_MASTER:OFPCR_ROLE_SLAVE);

        broadcastNotify();
    }

    m_mutex.unlock();
}

BOOL CClusterElection::checkRemoteDisconnected(UINT4 ip, INT4 sockfd)
{
    if (CClusterKeepalive::sendReq(sockfd) != BNC_OK)
    {
        LOG_WARN_FMT("$$$ remote CLUSTER %s[%s]sockfd[%d] disconnected $$$", 
            (m_masterIp==ip)?"MASTER":"SLAVE", inet_htoa(ip), sockfd);
        return TRUE;
    }

    return FALSE;
}

UINT4 CClusterElection::generateProposal0()
{
    UINT4 proposal = 0;

    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if ((CServer::getInstance()->getControllerIp() != controller.ip) &&
            (CLUSTER_CONTROLLER_CONNECTED == controller.state) &&
            !checkRemoteDisconnected(controller.ip, controller.sockfd))
        {
            if (proposal < controller.ip)
                proposal = controller.ip;
        }
    }

    return proposal;
}

UINT4 CClusterElection::generateProposal()
{
    UINT4 proposal = generateProposal0();
    if (proposal < CServer::getInstance()->getControllerIp())
        proposal = CServer::getInstance()->getControllerIp();

    return proposal;
}

INT4 CClusterElection::broadcastProposal(UINT4 ip)
{
    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if ((CServer::getInstance()->getControllerIp() != controller.ip) &&
            ((CLUSTER_CONTROLLER_CONNECTED == controller.state) || 
             (CLUSTER_CONTROLLER_NORESPONSE == controller.state)))
        {
            sendProposal(controller.sockfd, ip);
        }
    }

    return BNC_OK;
}

INT4 CClusterElection::broadcastNotify()
{
    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if ((CServer::getInstance()->getControllerIp() != controller.ip) &&
            ((CLUSTER_CONTROLLER_CONNECTED == controller.state) || 
             (CLUSTER_CONTROLLER_NORESPONSE == controller.state)))
        {
            sendNotify(controller.sockfd);
        }
    }

    return BNC_OK;
}

INT4 CClusterElection::broadcastInform()
{
    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if ((CServer::getInstance()->getControllerIp() != controller.ip) &&
            ((CLUSTER_CONTROLLER_CONNECTED == controller.state) || 
             (CLUSTER_CONTROLLER_NORESPONSE == controller.state)))
        {
            sendInform(controller.sockfd);
        }
    }

    return BNC_OK;
}

INT4 CClusterElection::processHelloReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_hello_req_t* req = (cluster_hello_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_HELLO_REQ) || (itf->length < (INT4)sizeof(cluster_hello_req_t)))
    {
        LOG_WARN_FMT("invalid hello: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    UINT4 ip = 0;
    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if (sockfd == controller.sockfd)
        {
            ip = controller.ip;
            break;
        }
    }
    
    //LOG_INFO_FMT("receive hello req with state[%d] master[%s]", req->state, inet_htoa(req->master));

    m_mutex.lock();
    
    if (CLUSTER_STATE_INIT == req->state)
    {
        //LOG_INFO("CLUSTER remote connected after restarted");
        if (CLUSTER_ELECTION_FINISHED == m_state)
        {
            if (ip == m_masterIp)
            {
                LOG_INFO_FMT("$$$ previous CLUSTER MASTER[%s] restarted $$$", inet_htoa(ip));
                m_state = CLUSTER_ELECTION_START;
                sendHelloRsp(sockfd);
                start();
            }
            else if (CServer::getInstance()->getControllerIp() == m_masterIp)
            {
                //only master response to new cluster remote
                sendHelloRsp(sockfd);
                //CClusterSync::syncAll(sockfd);
            }
        }
        else if (CLUSTER_ELECTION_INIT != m_state)
        {

            //notify new cluster remote to watch
            sendHelloRsp(sockfd);
        }
        //else: election will be triggered later
    }
    else
    {
        //LOG_INFO_FMT("CLUSTER remote re-connected with state[%d]master[%u], my state[%d]master[%u]", 
        //    req->state, req->master, m_state, m_masterIp);
        if ((CLUSTER_ELECTION_FINISHED == m_state) &&
            (CServer::getInstance()->getControllerIp() == m_masterIp))
        {
            sendHelloRsp(sockfd);
            //CClusterSync::syncAll(sockfd);
        }
    }

    m_mutex.unlock();

    return BNC_OK;
}

INT4 CClusterElection::processHelloRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_hello_rsp_t* rsp = (cluster_hello_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_HELLO_RSP) || (itf->length < (INT4)sizeof(cluster_hello_rsp_t)))
    {
        LOG_WARN_FMT("invalid hello: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    //LOG_INFO_FMT("receive hello rsp with state[%d] master[%s]", rsp->state, inet_htoa(rsp->master));

    m_mutex.lock();
    
    if (CLUSTER_STATE_WORKING == rsp->state)
    {
        LOG_WARN_FMT("MASTER is %s, i am %s", 
            inet_htoa(rsp->master), 
            (rsp->master==CServer::getInstance()->getControllerIp())?"MASTER":"SLAVE");

        m_masterIp = rsp->master;
        m_state = CLUSTER_ELECTION_FINISHED;
        CClusterService::getInstance()->updateRole(
            (m_masterIp==CServer::getInstance()->getControllerIp())?OFPCR_ROLE_MASTER:OFPCR_ROLE_SLAVE);
    }
    else if (CLUSTER_STATE_ELECTING == rsp->state)
    {
        LOG_WARN_FMT("electiong is ongoing, i am watching");
        m_state = CLUSTER_ELECTION_WATCHING;
    }
    //else: election will be triggered later

    m_mutex.unlock();

    return BNC_OK;
}

INT4 CClusterElection::processProposal(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_election_proposal_t* req = (cluster_election_proposal_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_ELECTION_PROPOSAL) || (itf->length < (INT4)sizeof(cluster_election_proposal_t)))
    {
        LOG_WARN_FMT("invalid election proposal: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    //LOG_INFO_FMT("receive proposal[%s]", inet_htoa(req->ip));

    m_mutex.lock();

    switch (m_state)
    {
        case CLUSTER_ELECTION_INIT:
        {
            //election is triggered during startup by other controller
            UINT4 proposal = generateProposal();
            if (proposal > req->ip)
            {
                LOG_WARN_FMT("INIT: my proposal is %s", inet_htoa(proposal));
                sendProposal(sockfd, proposal);
            }
            m_masterIp = (proposal > req->ip) ? proposal : req->ip;
            m_state = CLUSTER_ELECTION_ONGOING;
            break;
        }
        case CLUSTER_ELECTION_START:
        {
            //election trigger controller receive other controller's proposal
            if (m_masterIp < req->ip)
            {
                LOG_WARN_FMT("START: update proposal as %s", inet_htoa(req->ip));
                m_masterIp = req->ip;
            }
            //m_state = CLUSTER_ELECTION_ONGOING;
            break;
        }
        case CLUSTER_ELECTION_ONGOING:
        {
            //during election receive other controller's proposal
            if (m_masterIp > req->ip)
            {
                LOG_WARN_FMT("ONGOING: my proposal is %s", inet_htoa(m_masterIp));
                sendProposal(sockfd, m_masterIp);
            }
            else if (m_masterIp < req->ip)
            {
                LOG_WARN_FMT("ONGOING: update proposal as %s", inet_htoa(req->ip));
                m_masterIp = req->ip;
            }
            break;
        }
        case CLUSTER_ELECTION_WATCHING:
        {
            //watching controller won't participate in election
            LOG_WARN_FMT("WATCHING: proposal is %s", inet_htoa(req->ip));
            break;
        }
        case CLUSTER_ELECTION_FINISHED:
        {
            //re-election is triggered by other controller
            UINT4 proposal = generateProposal();
            if (proposal > req->ip)
            {
                LOG_WARN_FMT("FINISHED: my proposal is %s", inet_htoa(proposal));
                sendProposal(sockfd, proposal);
            }
            m_masterIp = (proposal > req->ip) ? proposal : req->ip;
            m_state = CLUSTER_ELECTION_ONGOING;
            break;
        }
    }

    m_mutex.unlock();

    return BNC_OK;
}

INT4 CClusterElection::processNotify(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_election_notify_t* req = (cluster_election_notify_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_ELECTION_NOTIFY) || (itf->length < (INT4)sizeof(cluster_election_notify_t)))
    {
        LOG_WARN_FMT("invalid election notify: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    m_timer.stop();

    //LOG_INFO_FMT("notified master[%s]", inet_htoa(req->master));

    m_mutex.lock();

    if (CLUSTER_ELECTION_FINISHED != m_state)
    {
        LOG_WARN_FMT("%s after notified, MASTER is %s, i am %s %s", 
            (CLUSTER_ELECTION_START==m_state)?"<<<":">>>",
            inet_htoa(req->master), 
            (req->master==CServer::getInstance()->getControllerIp())?"MASTER":"SLAVE",
            (CLUSTER_ELECTION_START==m_state)?"":"<<<");

        m_masterIp = req->master;
        m_state = CLUSTER_ELECTION_FINISHED;
        CClusterService::getInstance()->updateRole(
            (m_masterIp==CServer::getInstance()->getControllerIp())?OFPCR_ROLE_MASTER:OFPCR_ROLE_SLAVE);

        //deal with scenario: some controller doesn't receive election-notify
        broadcastInform();
    }
    else
    {
        if (req->master != m_masterIp)
        {
            LOG_WARN_FMT("CLUSTER notified MASTER[0x%x], but mine[0x%x]", req->master, m_masterIp);
            //TBD
        }
    }

    m_mutex.unlock();

    return BNC_OK;
}

INT4 CClusterElection::processInform(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_election_inform_t* req = (cluster_election_inform_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_ELECTION_INFORM) || (itf->length < (INT4)sizeof(cluster_election_inform_t)))
    {
        LOG_WARN_FMT("invalid election inform: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    //LOG_INFO_FMT("informed master[%s]", inet_htoa(req->master));

    m_mutex.lock();

    if (m_masterIp != req->master)
    {
        LOG_INFO_FMT(">>> after informed, MASTER is %s, i am %s <<<", 
            inet_htoa(req->master), 
            (req->master==CServer::getInstance()->getControllerIp())?"MASTER":"SLAVE");

        m_masterIp = req->master;
        m_state = CLUSTER_ELECTION_FINISHED;
        CClusterService::getInstance()->updateRole(
            (m_masterIp==CServer::getInstance()->getControllerIp())?OFPCR_ROLE_MASTER:OFPCR_ROLE_SLAVE);
    }

    m_mutex.unlock();

    return BNC_OK;
}

INT4 CClusterElection::sendHelloReq(INT4 sockfd)
{
    INT4 state = (CLUSTER_ELECTION_INIT==m_state)?CLUSTER_STATE_INIT:
                 (CLUSTER_ELECTION_FINISHED==m_state)?CLUSTER_STATE_WORKING:
                 CLUSTER_STATE_ELECTING;
    //LOG_INFO_FMT("send hello req with state[%d] master[%s] via sockfd[%d]", 
    //    state, inet_htoa(m_masterIp), sockfd);

    INT4 length = sizeof(cluster_interface_t) + sizeof(cluster_hello_req_t);
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = CLUSTER_OPER_HELLO_REQ;
    itf->length = sizeof(cluster_hello_req_t);

    cluster_hello_req_t* req = (cluster_hello_req_t*)ptr;
    req->state = state;
    req->master = m_masterIp;

    return sendMsgOut(sockfd, buffer, length);
}

INT4 CClusterElection::sendHelloRsp(INT4 sockfd)
{
    INT4 state = (CLUSTER_ELECTION_INIT==m_state)?CLUSTER_STATE_INIT:
                 (CLUSTER_ELECTION_FINISHED==m_state)?CLUSTER_STATE_WORKING:
                 CLUSTER_STATE_ELECTING;
    //LOG_INFO_FMT("send hello rsp with state[%d] master[%s] via sockfd[%d]", 
    //    state, inet_htoa(m_masterIp), sockfd);

    INT4 length = sizeof(cluster_interface_t) + sizeof(cluster_hello_rsp_t);
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = CLUSTER_OPER_HELLO_RSP;
    itf->length = sizeof(cluster_hello_rsp_t);

    cluster_hello_rsp_t* rsp = (cluster_hello_rsp_t*)ptr;
    rsp->state = state;
    rsp->master = m_masterIp;

    return sendMsgOut(sockfd, buffer, length);
}

INT4 CClusterElection::sendProposal(INT4 sockfd, UINT4 ip)
{
    INT4 length = sizeof(cluster_interface_t) + sizeof(cluster_election_proposal_t);
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = CLUSTER_OPER_ELECTION_PROPOSAL;
    itf->length = sizeof(cluster_election_proposal_t);

    cluster_election_proposal_t* req = (cluster_election_proposal_t*)ptr;
    req->ip = ip;

    return sendMsgOut(sockfd, buffer, length);
}

INT4 CClusterElection::sendNotify(INT4 sockfd)
{
    //LOG_INFO_FMT("notify master[%s]", inet_htoa(m_masterIp));

    INT4 length = sizeof(cluster_interface_t) + sizeof(cluster_election_notify_t);
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = CLUSTER_OPER_ELECTION_NOTIFY;
    itf->length = sizeof(cluster_election_notify_t);

    cluster_election_notify_t* req = (cluster_election_notify_t*)ptr;
    req->master = m_masterIp;

    return sendMsgOut(sockfd, buffer, length);
}

INT4 CClusterElection::sendInform(INT4 sockfd)
{
    //LOG_INFO_FMT("inform master[%s]", inet_htoa(m_masterIp));

    INT4 length = sizeof(cluster_interface_t) + sizeof(cluster_election_inform_t);
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = CLUSTER_OPER_ELECTION_INFORM;
    itf->length = sizeof(cluster_election_inform_t);

    cluster_election_inform_t* req = (cluster_election_inform_t*)ptr;
    req->master = m_masterIp;

    return sendMsgOut(sockfd, buffer, length);
}


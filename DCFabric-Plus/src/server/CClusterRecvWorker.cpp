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
*   File Name   : CClusterRecvWorker.cpp                                      *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "log.h"
#include "CClusterRecvWorker.h"
#include "CClusterKeepalive.h"
#include "CClusterElect.h"
#include "CClusterSync.h"
#include "CClusterService.h"

CClusterRecvWorker::CClusterRecvWorker()
{
    m_size = sizeof(cluster_interface_t) + 
             sizeof(cluster_sync_req_t) + 
             CClusterSync::FIELD_MEM_SIZE;
    m_buffer = (INT1*)malloc(m_size);
    if (NULL == m_buffer)
    {
        LOG_ERROR_FMT("malloc %d bytes m_buffer failed !", m_size);
        exit(-1);
    }

    CLoopBuffer* loopBuffer = new CLoopBuffer((UINT4)2*m_size);
    if (NULL == loopBuffer)
    {
        LOG_ERROR_FMT("malloc %d bytes CLoopBuffer failed !", 2*m_size);
        exit(-1);
    }
    m_loopBuffer = CSmartPtr<CLoopBuffer>(loopBuffer);
}

CClusterRecvWorker::~CClusterRecvWorker()
{
    if (NULL != m_buffer)
    {
        free(m_buffer);
        m_buffer = NULL;
    }
}

INT4 CClusterRecvWorker::process(INT4 sockfd, INT1* buffer, UINT4 len)
{
    INT4 ret = BNC_ERR;

    if (m_loopBuffer->write(buffer, len))
    {
        while (1)
        {
            if (!m_loopBuffer->read(m_buffer, sizeof(cluster_interface_t), TRUE))
                break;

            cluster_interface_t* itf = (cluster_interface_t*)m_buffer;
            UINT4 msgLen = sizeof(cluster_interface_t) + itf->length;
            LOG_INFO_FMT("msg total len[%u], m_loopBuffer->length[%u] on sockfd[%d]", msgLen, m_loopBuffer->length(), sockfd);
            if (m_loopBuffer->length() < msgLen)
                break;

            if (!m_loopBuffer->read(m_buffer, msgLen, FALSE))
                break;

            ret = processMsg(sockfd, m_buffer);
        }
    }

    return ret;
}

void CClusterRecvWorker::processPeerDisconn(INT4 sockfd)
{
    CClusterService::getInstance()->remoteDisconnect(sockfd);
}

INT4 CClusterRecvWorker::processMsg(INT4 sockfd, INT1* buffer)
{
    INT4 ret = BNC_ERR;
    cluster_interface_t* itf = (cluster_interface_t*)buffer;

    LOG_INFO_FMT("CLUSTER %s process operation[0x%02x]length[%lu]sockfd[%d]", 
        (OFPCR_ROLE_MASTER==CClusterService::getInstance()->getControllerRole())?"MASTER":"SLAVE", 
        itf->operation, itf->length+sizeof(cluster_interface_t), sockfd);
    switch (itf->operation)
    {
        case CLUSTER_OPER_KEEPALIVE_REQ:
            ret = CClusterKeepalive::processReq(sockfd, itf);
            break;
        case CLUSTER_OPER_KEEPALIVE_RSP:
            ret = CClusterKeepalive::processRsp(sockfd, itf);
            break;

        case CLUSTER_OPER_HELLO_REQ:
            ret = CClusterElection::processHelloReq(sockfd, itf);
            break;
        case CLUSTER_OPER_HELLO_RSP:
            ret = CClusterElection::processHelloRsp(sockfd, itf);
            break;
        case CLUSTER_OPER_ELECTION_PROPOSAL:
            ret = CClusterElection::processProposal(sockfd, itf);
            break;
        case CLUSTER_OPER_ELECTION_NOTIFY:
            ret = CClusterElection::processNotify(sockfd, itf);
            break;
        case CLUSTER_OPER_ELECTION_INFORM:
            ret = CClusterElection::processInform(sockfd, itf);
            break;

        //host
        case CLUSTER_OPER_SYNC_HOST_LIST_REQ:
            ret = CClusterSync::processSyncHostListReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_HOST_LIST_RSP:
            ret = CClusterSync::processSyncHostListRsp(sockfd, itf);
            break;

        //NAT ICMP
        case CLUSTER_OPER_SYNC_NAT_ICMP_LIST_REQ:
            ret = CClusterSync::processSyncNatIcmpListReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_NAT_ICMP_LIST_RSP:
            ret = CClusterSync::processSyncNatIcmpListRsp(sockfd, itf);
            break;

        //SNAT
        case CLUSTER_OPER_SYNC_NAT_HOST_LIST_REQ:
            ret = CClusterSync::processSyncNatHostListReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_NAT_HOST_LIST_RSP:
            ret = CClusterSync::processSyncNatHostListRsp(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_ADD_NAT_HOST_REQ:
            ret = CClusterSync::processSyncAddNatHostReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_ADD_NAT_HOST_RSP:
            ret = CClusterSync::processSyncAddNatHostRsp(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_DEL_NAT_HOST_REQ:
            ret = CClusterSync::processSyncDelNatHostReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_DEL_NAT_HOST_RSP:
            ret = CClusterSync::processSyncDelNatHostRsp(sockfd, itf);
            break;

        //topo
        case CLUSTER_OPER_SYNC_TOPO_LINK_LIST_REQ:
            ret = CClusterSync::processSyncTopoLinkListReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_TOPO_LINK_LIST_RSP:
            ret = CClusterSync::processSyncTopoLinkListRsp(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_TOPO_LINK_REQ:
            ret = CClusterSync::processSyncTopoLinkReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_TOPO_LINK_RSP:
            ret = CClusterSync::processSyncTopoLinkRsp(sockfd, itf);
            break;

        //tag
        case CLUSTER_OPER_SYNC_SW_TAG_LIST_REQ:
            ret = CClusterSync::processSyncSwitchTagListReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_SW_TAG_LIST_RSP:
            ret = CClusterSync::processSyncSwitchTagListRsp(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_SW_TAG_REQ:
            ret = CClusterSync::processSyncSwitchTagReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_SW_TAG_RSP:
            ret = CClusterSync::processSyncSwitchTagRsp(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_TAG_INFO_REQ:
            ret = CClusterSync::processSyncTagInfoReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_TAG_INFO_RSP:
            ret = CClusterSync::processSyncTagInfoRsp(sockfd, itf);
            break;

        //flow entry
        case CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_REQ:
            ret = CClusterSync::processSyncFlowEntryListReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_RSP:
            ret = CClusterSync::processSyncFlowEntryListRsp(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_REQ:
            ret = CClusterSync::processSyncAddFlowEntryReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_RSP:
            ret = CClusterSync::processSyncAddFlowEntryRsp(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_REQ:
            ret = CClusterSync::processSyncDelFlowEntryReq(sockfd, itf);
            break;
        case CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_RSP:
            ret = CClusterSync::processSyncDelFlowEntryRsp(sockfd, itf);
            break;

        default:
            LOG_INFO_FMT("CLUSTER server receive unsupported operation[0x%02x]sockfd[%d]", 
                itf->operation, sockfd);
            break;
    }
    
    return ret;
}


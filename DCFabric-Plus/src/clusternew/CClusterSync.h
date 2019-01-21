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
*   File Name   : CClusterSync.h                                              *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#ifndef __CLUSTERSYNC_H
#define __CLUSTERSYNC_H

#include "bnc-type.h"
#include "CClusterInterface.h"
#include "CTimer.h"
#include "bnc-param.h"
#include "bnc-inet.h"
#include "CLFQueue.h"
#include "CSemaphore.h"
#include "CSNatConn.h"

class SyncData
{
public:
    INT4 m_operation;
    INT4 m_sockfd;
    union {
        neighbor_t m_neigh;
        INT1 m_dpidTag[12];
        INT1 m_snatConn[sizeof(CSNatConn)];
        INT1 m_dpidUuid[8+UUID_LEN];
    };
};

typedef CLFQueue<SyncData> CSyncQueue;

class CClusterSync
{
public:
    const static INT4 SYNC_INTERVAL_SECOND = 30;
    const static INT4 FIELD_NUM_MAX        = 204800;
    const static INT4 FIELD_MEM_SIZE       = (sizeof(tlv_t) + 64) * FIELD_NUM_MAX;

    static INT4 init();
    static void clear();
    static void syncAll(INT4 sockfd=-1);

    static INT4 syncHostList(INT4 sockfd);
    static INT4 syncNatIcmpList(INT4 sockfd);

    static INT4 syncNatHostList(INT4 sockfd);
    static INT4 syncAddNatHost(const CSNatConn& snatConn);
    static INT4 syncDelNatHost(const CSNatConn& snatConn);

    static INT4 syncTopoLinkList(INT4 sockfd);
    static INT4 syncTopoLink(const neighbor_t& neigh);

    static INT4 syncSwitchTagList(INT4 sockfd);
    static INT4 syncSwitchTag(UINT8 dpid, UINT4 tag);

    static INT4 syncTagInfo(INT4 sockfd=-1);

    static INT4 syncFlowEntryList(INT4 sockfd);
    static INT4 syncAddFlowEntry(UINT8 dpid, const INT1* uuid);
    static INT4 syncDelFlowEntry(UINT8 dpid, const INT1* uuid);

    static INT4 processSyncHostListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncHostListRsp(INT4 sockfd, cluster_interface_t* itf);

    static INT4 processSyncNatIcmpListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncNatIcmpListRsp(INT4 sockfd, cluster_interface_t* itf);

    static INT4 processSyncNatHostListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncNatHostListRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncAddNatHostReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncAddNatHostRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncDelNatHostReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncDelNatHostRsp(INT4 sockfd, cluster_interface_t* itf);

    static INT4 processSyncTopoLinkListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncTopoLinkListRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncTopoLinkReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncTopoLinkRsp(INT4 sockfd, cluster_interface_t* itf);

    static INT4 processSyncSwitchTagListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncSwitchTagListRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncSwitchTagReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncSwitchTagRsp(INT4 sockfd, cluster_interface_t* itf);

    static INT4 processSyncTagInfoReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncTagInfoRsp(INT4 sockfd, cluster_interface_t* itf);

    static INT4 processSyncFlowEntryListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncFlowEntryListRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncAddFlowEntryReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncAddFlowEntryRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncDelFlowEntryReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncDelFlowEntryRsp(INT4 sockfd, cluster_interface_t* itf);

#if 0
    static INT4 processSyncSwitchListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncSwitchListRsp(INT4 sockfd, cluster_interface_t* itf);

    static INT4 processSyncOpenstackExternalListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncOpenstackExternalListRsp(INT4 sockfd, cluster_interface_t* itf);

    static INT4 processSyncOpenstackLbaasMembersListReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncOpenstackLbaasMembersListRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncAddOpenstackLbaasMembersReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncAddOpenstackLbaasMembersRsp(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncDelOpenstackLbaasMembersReq(INT4 sockfd, cluster_interface_t* itf);
    static INT4 processSyncDelOpenstackLbaasMembersRsp(INT4 sockfd, cluster_interface_t* itf);
#endif

private:
    static void* syncRoutine(void* param);
    static void syncPeriodically(void* param);

    static INT4 syncHostList0(INT4 sockfd);
    static INT4 recoverHostList(const INT1* data, INT4 len);

    static INT4 syncNatIcmpList0(INT4 sockfd);
    static INT4 recoverNatIcmpList(const INT1* data, INT4 len);

    static INT4 syncNatHostList0(INT4 sockfd);
    static INT4 recoverNatHostList(const INT1* data, INT4 len);
    static INT4 syncAddNatHost0(const CSNatConn& snatConn);
    static INT4 recoverAddNatHost(const INT1* data, INT4 len);
    static INT4 syncDelNatHost0(const CSNatConn& snatConn);
    static INT4 recoverDelNatHost(const INT1* data, INT4 len);

    static INT4 syncTopoLinkList0(INT4 sockfd);
    static INT4 recoverTopoLinkList(const INT1* data, INT4 len);
    static INT4 syncTopoLink0(const neighbor_t& neigh);
    static INT4 recoverTopoLink(const INT1* data, INT4 len);

    static INT4 syncSwitchTagList0(INT4 sockfd);
    static INT4 recoverSwitchTagList(const INT1* data, INT4 len);
    static INT4 syncSwitchTag0(UINT8 dpid, UINT4 tag);
    static INT4 recoverSwitchTag(const INT1* data, INT4 len);

    static INT4 syncTagInfo0(INT4 sockfd);
    static INT4 recoverTagInfo(const INT1* data, INT4 len);

    static INT4 syncFlowEntryList0(INT4 sockfd);
    static INT4 recoverFlowEntryList(const INT1* data, INT4 len);
    static void recoverFlowEntry(UINT8 dpid, gn_flow_t& flow, gn_action_t* actions);
    static INT4 syncAddFlowEntry0(UINT8 dpid, const INT1* uuid);
    static INT4 recoverAddFlowEntry(const INT1* data, INT4 len);
    static INT4 syncDelFlowEntry0(UINT8 dpid, const INT1* uuid);
    static INT4 recoverDelFlowEntry(const INT1* data, INT4 len);

#if 0
    static INT4 syncSwitchList0(INT4 sockfd=-1);
    static INT4 syncSwitchList(INT4 sockfd=-1);
    static INT4 recoverSwitchList(INT4 num, const field_pad_t* field_pad_p);

    static INT4 syncOpenstackExternalList0(INT4 sockfd=-1);
    static INT4 syncOpenstackExternalList(INT4 sockfd=-1);
    static INT4 recoverOpenstackExternalList(INT4 num, const field_pad_t* field_pad_p);

    static INT4 syncOpenstackLbaasMembersList0(INT4 sockfd=-1);
    static INT4 syncOpenstackLbaasMembersList(INT4 sockfd=-1);
    static INT4 recoverOpenstackLbaasMembersList(INT4 num, const field_pad_t* field_pad_p);
    static INT4 syncAddOpenstackLbaasMembers0(openstack_lbaas_connect_p connect_ips);
    static INT4 syncAddOpenstackLbaasMembers(openstack_lbaas_connect_p connect_ips);
    static INT4 recoverAddOpenstackLbaasMembers(INT4 num, const field_pad_t* field_pad_p);
    static INT4 syncDelOpenstackLbaasMembers0(openstack_lbaas_connect_p connect_ips);
    static INT4 syncDelOpenstackLbaasMembers(openstack_lbaas_connect_p connect_ips);
    static INT4 recoverDelOpenstackLbaasMembers(INT4 num, const field_pad_t* field_pad_p);
#endif

    static INT4 sendReq(INT4 operation, INT4 sockfd=-1);
    static INT4 sendRsp(INT4 operation, INT4 cause, INT4 sockfd);

    static BOOL syncNeeded();
    static BOOL recoverNeeded();

private:
    static CTimer     m_timer;
    static CThread    m_thread;
    static CSemaphore m_sem;
    static CSyncQueue m_queue;

    static INT4       m_fieldLen;   
    static INT1*      m_fieldMem;
};

#endif /* __CLUSTERSYNC_H */

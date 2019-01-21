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
*   File Name   : cluster_sync.cpp                                            *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "log.h"
#include "bnc-error.h"
#include "CClusterSync.h"
#include "CClusterDefine.h"
#include "CClusterService.h"
#include "CClusterElect.h"
#include "CControl.h"
#include "CConf.h"
#include "CServer.h"
#include "CHostMgr.h"
#include "CHostFloating.h"
#include "CHostNormal.h"
#include "CNatIcmpEntryMgr.h"
#include "CSNatConnMgr.h"
#include "CFlowMgr.h"
#include "CFlowMod.h"
#include "CFlowTableEventReportor.h"

#define SYNC_DATA_INTEGER(fieldType, intType, intVal)                         \
{                                                                             \
    if (m_fieldLen + sizeof(tlv_t) + sizeof(intType) > (UINT4)FIELD_MEM_SIZE) \
    {                                                                         \
        stopped = TRUE;                                                       \
        break;                                                                \
    }                                                                         \
    tlv = (tlv_t*)(m_fieldMem + m_fieldLen);                                  \
    tlv->type = fieldType;                                                    \
    tlv->len = sizeof(intType);                                               \
    *(intType*)tlv->data = intVal;                                            \
    m_fieldLen += sizeof(tlv_t) + tlv->len;                                   \
}
#define SYNC_DATA_BYTES(fieldType, bytes, length)                    \
{                                                                    \
    if (m_fieldLen + sizeof(tlv_t) + length > (UINT4)FIELD_MEM_SIZE) \
    {                                                                \
        stopped = TRUE;                                              \
        break;                                                       \
    }                                                                \
    tlv = (tlv_t*)(m_fieldMem + m_fieldLen);                         \
    tlv->type = fieldType;                                           \
    tlv->len = length;                                               \
    memcpy(tlv->data, (UINT1*)bytes, length);                        \
    m_fieldLen += sizeof(tlv_t) + tlv->len;                          \
}
#define SYNC_DATA_STRING(fieldType, str)                                     \
{                                                                            \
    if (m_fieldLen + sizeof(tlv_t) + str.length()+1 > (UINT4)FIELD_MEM_SIZE) \
    {                                                                        \
        stopped = TRUE;                                                      \
        break;                                                               \
    }                                                                        \
    tlv = (tlv_t*)(m_fieldMem + m_fieldLen);                                 \
    tlv->type = fieldType;                                                   \
    tlv->len = str.length()+1;                                               \
    str.copy((INT1*)tlv->data, str.length(), 0);                             \
    tlv->data[str.length()] = '\0';                                          \
    m_fieldLen += sizeof(tlv_t) + tlv->len;                                  \
}
    
#define RETURN_IF_LENGTH_INVALID()                                       \
    if ((UINT4)len < sizeof(tlv_t) + tlv->len)                           \
    {                                                                    \
        LOG_WARN_FMT("invalid len[%d] < sizeof(tlv_t) + tlv->len[%d] !", \
            len, tlv->len);                                              \
        return BNC_ERR;                                                  \
    }
#define BREAK_IF_LENGTH_INVALID()                                        \
    if ((UINT4)len < sizeof(tlv_t) + tlv->len)                           \
    {                                                                    \
        LOG_WARN_FMT("invalid len[%d] < sizeof(tlv_t) + tlv->len[%d] !", \
            len, tlv->len);                                              \
        break;                                                           \
    }
#define RETURN_IF_TYPE_INVALID(fieldType)              \
    if (tlv->type != fieldType)                        \
    {                                                  \
        LOG_WARN_FMT("invalid tlv-type[0x%x] != %s !", \
            tlv->type, #fieldType);                    \
        return BNC_ERR;                                \
    }

#define GET_DATA_INTEGER(val, intType) \
{                                      \
    val = *(intType*)tlv->data;        \
    len -= sizeof(tlv_t) + tlv->len;   \
    data += sizeof(tlv_t) + tlv->len;  \
}
#define GET_DATA_BYTES(bytes, length) \
{                                     \
    memcpy((UINT1*)bytes, tlv->data, length); \
    len -= sizeof(tlv_t) + tlv->len;  \
    data += sizeof(tlv_t) + tlv->len; \
}
#define GET_DATA_STRING(str)          \
{                                     \
    memcpy(str, tlv->data, tlv->len); \
    str[tlv->len-1] = '\0';           \
    len -= sizeof(tlv_t) + tlv->len;  \
    data += sizeof(tlv_t) + tlv->len; \
}

#define RETURN_IF_REQUEST_INVALID(itf, oper)                                           \
    if ((itf->operation != oper) || (itf->length < (INT4)sizeof(cluster_sync_req_t))) \
    {                                                                                  \
        LOG_WARN_FMT("invalid sync req: operation[0x%02x], length[%d]",                \
            itf->operation, itf->length);                                              \
        return BNC_ERR;                                                                \
    }
#define RETURN_IF_RESPONSE_INVALID(itf, oper)                                          \
    if ((itf->operation != oper) || (itf->length < (INT4)sizeof(cluster_sync_rsp_t))) \
    {                                                                                  \
        LOG_WARN_FMT("invalid sync rsp: operation[0x%02x], length[%d]",                \
            itf->operation, itf->length);                                              \
        return BNC_ERR;                                                                \
    }

#define SYNC_OXM_FIELDS(oxm) \
{\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IN_PORT))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_INPORT, UINT4, oxm.in_port);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IN_PHY_PORT))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_INPHYPORT, UINT4, oxm.in_phy_port);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_METADATA))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_METADATA, UINT8, oxm.metadata);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_DST))\
        SYNC_DATA_BYTES(FLOW_ENTRY_OXM_ETHDST, oxm.eth_dst, MAC_LEN);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_SRC))\
        SYNC_DATA_BYTES(FLOW_ENTRY_OXM_ETHSRC, oxm.eth_src, MAC_LEN);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_TYPE))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_ETHTYPE, UINT2, oxm.eth_type);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_VLAN_VID))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_VLANID, UINT2, oxm.vlan_vid);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_VLAN_PCP))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_VLANPCP, UINT1, oxm.vlan_pcp);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_DSCP))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPDSCP, UINT1, oxm.ip_dscp);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_ECN))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPECN, UINT1, oxm.ip_ecn);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_PROTO))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPPROTO, UINT1, oxm.ip_proto);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_SRC))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPV4SRC, UINT4, oxm.ipv4_src);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_DST))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPV4DST, UINT4, oxm.ipv4_dst);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TCP_SRC))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_TCPSRC, UINT2, oxm.tcp_src);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TCP_DST))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_TCPDST, UINT2, oxm.tcp_dst);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_UDP_SRC))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_UDPSRC, UINT2, oxm.udp_src);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_UDP_DST))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_UDPDST, UINT2, oxm.udp_dst);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ICMPV4_TYPE))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_ICMPV4TYPE, UINT1, oxm.icmpv4_type);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ICMPV4_CODE))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_ICMPV4CODE, UINT1, oxm.icmpv4_code);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_OP))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_ARPOP, UINT1, oxm.arp_op);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_SPA))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_ARPSPA, UINT4, oxm.arp_spa);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_TPA))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_ARPTPA, UINT4, oxm.arp_tpa);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_SHA))\
        SYNC_DATA_BYTES(FLOW_ENTRY_OXM_ARPSHA, oxm.arp_sha, MAC_LEN);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_THA))\
        SYNC_DATA_BYTES(FLOW_ENTRY_OXM_ARPTHA, oxm.arp_tha, MAC_LEN);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_SRC))\
        SYNC_DATA_BYTES(FLOW_ENTRY_OXM_IPV6SRC, oxm.ipv6_src, IPV6_LEN);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_DST))\
        SYNC_DATA_BYTES(FLOW_ENTRY_OXM_IPV6DST, oxm.ipv6_dst, IPV6_LEN);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_MPLS_LABEL))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_MPLSLABEL, UINT4, oxm.mpls_label);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TUNNEL_ID))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_TUNNELID, UINT4, oxm.tunnel_id);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_SRC_PREFIX))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPV4SRCPREFIX, UINT4, oxm.ipv4_src_prefix);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_DST_PREFIX))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPV4DSTPREFIX, UINT4, oxm.ipv4_dst_prefix);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_SRC_PREFIX))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPV6SRCPREFIX, UINT4, oxm.ipv6_src_prefix);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_DST_PREFIX))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_IPV6DSTPREFIX, UINT4, oxm.ipv6_dst_prefix);\
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_METADATA_MASK))\
        SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_METADATAMASK, UINT8, oxm.metadata_mask);\
    SYNC_DATA_INTEGER(FLOW_ENTRY_OXM_MASK, UINT8, oxm.mask);\
}
#define RECOVER_OXM_FIELDS(type, oxm) \
{\
    switch (type)\
    {\
        case FLOW_ENTRY_OXM_INPORT:\
            GET_DATA_INTEGER(oxm.in_port, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_INPHYPORT:\
            GET_DATA_INTEGER(oxm.in_phy_port, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_METADATA:\
            GET_DATA_INTEGER(oxm.metadata, UINT8);\
            break;\
        case FLOW_ENTRY_OXM_ETHDST:\
            GET_DATA_BYTES(oxm.eth_dst, MAC_LEN);\
            break;\
        case FLOW_ENTRY_OXM_ETHSRC:\
            GET_DATA_BYTES(oxm.eth_src, MAC_LEN);\
            break;\
        case FLOW_ENTRY_OXM_ETHTYPE:\
            GET_DATA_INTEGER(oxm.eth_type, UINT2);\
            break;\
        case FLOW_ENTRY_OXM_VLANID:\
            GET_DATA_INTEGER(oxm.vlan_vid, UINT2);\
            break;\
        case FLOW_ENTRY_OXM_VLANPCP:\
            GET_DATA_INTEGER(oxm.vlan_pcp, UINT1);\
            break;\
        case FLOW_ENTRY_OXM_IPDSCP:\
            GET_DATA_INTEGER(oxm.ip_dscp, UINT1);\
            break;\
        case FLOW_ENTRY_OXM_IPECN:\
            GET_DATA_INTEGER(oxm.ip_ecn, UINT1);\
            break;\
        case FLOW_ENTRY_OXM_IPPROTO:\
            GET_DATA_INTEGER(oxm.ip_proto, UINT1);\
            break;\
        case FLOW_ENTRY_OXM_IPV4SRC:\
            GET_DATA_INTEGER(oxm.ipv4_src, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_IPV4DST:\
            GET_DATA_INTEGER(oxm.ipv4_dst, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_TCPSRC:\
            GET_DATA_INTEGER(oxm.tcp_src, UINT2);\
            break;\
        case FLOW_ENTRY_OXM_TCPDST:\
            GET_DATA_INTEGER(oxm.tcp_dst, UINT2);\
            break;\
        case FLOW_ENTRY_OXM_UDPSRC:\
            GET_DATA_INTEGER(oxm.udp_src, UINT2);\
            break;\
        case FLOW_ENTRY_OXM_UDPDST:\
            GET_DATA_INTEGER(oxm.udp_dst, UINT2);\
            break;\
        case FLOW_ENTRY_OXM_ICMPV4TYPE:\
            GET_DATA_INTEGER(oxm.icmpv4_type, UINT1);\
            break;\
        case FLOW_ENTRY_OXM_ICMPV4CODE:\
            GET_DATA_INTEGER(oxm.icmpv4_code, UINT1);\
            break;\
        case FLOW_ENTRY_OXM_ARPOP:\
            GET_DATA_INTEGER(oxm.arp_op, UINT1);\
            break;\
        case FLOW_ENTRY_OXM_ARPSPA:\
            GET_DATA_INTEGER(oxm.arp_spa, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_ARPTPA:\
            GET_DATA_INTEGER(oxm.arp_tpa, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_ARPSHA:\
            GET_DATA_BYTES(oxm.arp_sha, MAC_LEN);\
            break;\
        case FLOW_ENTRY_OXM_ARPTHA:\
            GET_DATA_BYTES(oxm.arp_tha, MAC_LEN);\
            break;\
        case FLOW_ENTRY_OXM_IPV6SRC:\
            GET_DATA_BYTES(oxm.ipv6_src, IPV6_LEN);\
            break;\
        case FLOW_ENTRY_OXM_IPV6DST:\
            GET_DATA_BYTES(oxm.ipv6_dst, IPV6_LEN);\
            break;\
        case FLOW_ENTRY_OXM_MPLSLABEL:\
            GET_DATA_INTEGER(oxm.mpls_label, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_TUNNELID:\
            GET_DATA_INTEGER(oxm.tunnel_id, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_IPV4SRCPREFIX:\
            GET_DATA_INTEGER(oxm.ipv4_src_prefix, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_IPV4DSTPREFIX:\
            GET_DATA_INTEGER(oxm.ipv4_dst_prefix, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_IPV6SRCPREFIX:\
            GET_DATA_INTEGER(oxm.ipv6_src_prefix, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_IPV6DSTPREFIX:\
            GET_DATA_INTEGER(oxm.ipv6_dst_prefix, UINT4);\
            break;\
        case FLOW_ENTRY_OXM_METADATAMASK:\
            GET_DATA_INTEGER(oxm.metadata_mask, UINT8);\
            break;\
        default:\
            break;\
    }\
}

CTimer     CClusterSync::m_timer;
CThread    CClusterSync::m_thread;
CSemaphore CClusterSync::m_sem;
CSyncQueue CClusterSync::m_queue;

INT4       CClusterSync::m_fieldLen = 0;   
INT1*      CClusterSync::m_fieldMem = NULL;

INT4 CClusterSync::init()
{
    m_fieldMem = (INT1*)malloc(FIELD_MEM_SIZE);
    if (NULL == m_fieldMem)
    {
        LOG_ERROR_FMT("malloc %d bytes m_fieldMem failed !", FIELD_MEM_SIZE);
        return BNC_ERR;
    }

    if (m_thread.init(syncRoutine, NULL, "CClusterSync") != BNC_OK)
    {
        LOG_ERROR("INIT thread for CClusterSync failed !");
        clear();
        return BNC_ERR;
    }

    const INT1* pInterval = CConf::getInstance()->getConfig("cluster_conf", "sync_interval");
    UINT4 interval = (pInterval == NULL) ? SYNC_INTERVAL_SECOND : atol(pInterval);
    LOG_INFO_FMT("sync_interval %us", interval);

    if (0 == interval)
        return BNC_OK;

    if (m_timer.schedule(interval, interval, syncPeriodically, NULL) != BNC_OK)
    {
        LOG_ERROR("CClusterSync schedule syncPeriodically failed !");
        return BNC_ERR;
    }

    return BNC_OK;
}

void CClusterSync::clear()
{
    if (NULL != m_fieldMem)
    {
        free(m_fieldMem);
        m_fieldMem = NULL;
    }
}

void* CClusterSync::syncRoutine(void* param)
{
	prctl(PR_SET_NAME, (unsigned long)"ClusterSyncThread");  

    SyncData sync;
	
	while(1)
	{
        try
        {
            if (m_sem.wait() == 0)
            {
                m_queue.pop(sync);
                switch (sync.m_operation)
                {
                    case CLUSTER_OPER_SYNC_HOST_LIST_REQ:
                        syncHostList0(sync.m_sockfd);
                        break;
                    case CLUSTER_OPER_SYNC_NAT_ICMP_LIST_REQ:
                        syncNatIcmpList0(sync.m_sockfd);
                        break;
                    case CLUSTER_OPER_SYNC_NAT_HOST_LIST_REQ:
                        syncNatHostList0(sync.m_sockfd);
                        break;
                    case CLUSTER_OPER_SYNC_ADD_NAT_HOST_REQ:
                        syncAddNatHost0(*(CSNatConn*)sync.m_snatConn);
                        break;
                    case CLUSTER_OPER_SYNC_DEL_NAT_HOST_REQ:
                        syncDelNatHost0(*(CSNatConn*)sync.m_snatConn);
                        break;
                    case CLUSTER_OPER_SYNC_TOPO_LINK_LIST_REQ:
                        syncTopoLinkList0(sync.m_sockfd);
                        break;
                    case CLUSTER_OPER_SYNC_TOPO_LINK_REQ:
                        syncTopoLink0(sync.m_neigh);
                        break;
                    case CLUSTER_OPER_SYNC_SW_TAG_LIST_REQ:
                        syncSwitchTagList0(sync.m_sockfd);
                        break;
                    case CLUSTER_OPER_SYNC_SW_TAG_REQ:
                        syncSwitchTag0(*(UINT8*)sync.m_dpidTag, *(UINT4*)(sync.m_dpidTag+8));
                        break;
                    case CLUSTER_OPER_SYNC_TAG_INFO_REQ:
                        syncTagInfo0(sync.m_sockfd);
                        break;
                    case CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_REQ:
                        syncFlowEntryList0(sync.m_sockfd);
                        break;
                    case CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_REQ:
                        syncAddFlowEntry0(*(UINT8*)sync.m_dpidUuid, sync.m_dpidUuid+8);
                        break;
                    case CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_REQ:
                        syncDelFlowEntry0(*(UINT8*)sync.m_dpidUuid, sync.m_dpidUuid+8);
                        break;
                    default:
                        break;
                }
            }
        }
        catch (...)
        {
            LOG_ERROR("catch exception !");
        }
	}

	return NULL;
}

void CClusterSync::syncPeriodically(void* param)
{
    syncAll();
}

void CClusterSync::syncAll(INT4 sockfd)
{
    if (!syncNeeded())
        return;

    syncHostList(sockfd);
    syncNatIcmpList(sockfd);
    syncNatHostList(sockfd);
    syncTopoLinkList(sockfd);
    syncSwitchTagList(sockfd);
    syncTagInfo(sockfd);
    syncFlowEntryList(sockfd);
}

INT4 CClusterSync::syncHostList(INT4 sockfd)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_HOST_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncHostList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

	CHostList& hostlist = CHostMgr::getInstance()->getHostList();
	STL_FOR_LOOP(hostlist, iter)
    {
        CSmartPtr<CHost> host = *iter;
        if (host.isNotNull() && (host->getIp() != 0))
        {
#if 0
            INT1 macStr[20] = {0};
            mac2str(host->getMac(), macStr);
            INT1 ipStr[20] = {0}, fixedIpStr[20] = {0};
            number2ip(host->getIp(), ipStr);
            number2ip(host->getfixIp(), fixedIpStr);
            LOG_WARN_FMT("### MASTER sync host: type[%d]sw[0x%p]dpid[0x%llu]port[%u]mac[%s]ip[%s]fixedIp[%s]subnetId[%s]tenantId[%s]", 
                host->getHostType(), host->getSw().getPtr(), host->getDpid(), host->getPortNo(), 
                macStr, ipStr, fixedIpStr, host->getSubnetId().c_str(), host->getTenantId().c_str());
#endif

            SYNC_DATA_INTEGER(HOST_TYPE,     INT4,  host->getHostType());
            SYNC_DATA_BYTES(HOST_MAC,        host->getMac(), MAC_LEN);
#if 0
            SYNC_DATA_INTEGER(HOST_IP,       UINT4, host->getIp());
#else
            STL_FOR_LOOP(host->getIpList(), it)
            {
                SYNC_DATA_INTEGER(HOST_IP,   UINT4, *it);
            }
            if (stopped)
                break;
#endif
            SYNC_DATA_INTEGER(HOST_SW_DPID,  UINT8, host->getDpid());
            SYNC_DATA_INTEGER(HOST_SW_PORT,  UINT4, host->getPortNo());
            SYNC_DATA_STRING(HOST_SUBNET_ID, host->getSubnetId());
            SYNC_DATA_STRING(HOST_TENANT_ID, host->getTenantId());
            SYNC_DATA_INTEGER(HOST_FIXED_IP, UINT4, host->getfixIp());
        }    
    }
    
    if (stopped)
        LOG_WARN("CClusterSync syncHostList0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_HOST_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverHostList(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    CHostList& hostList = CHostMgr::getInstance()->getHostList();
    hostList.clear();

    INT4 type = 0;
    UINT1 mac[MAC_LEN] = {0};
    UINT4 ip = 0;
    std::vector<UINT4> ipList;
    UINT8 dpid = 0;
    UINT4 port = 0;
    INT1 subnetId[64] = {0};
    INT1 tenantId[64] = {0};
    UINT4 fixedIp = 0;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(HOST_TYPE);
    GET_DATA_INTEGER(type, INT4);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case HOST_TYPE:
            //one record range in [HOST_TYPE, HOST_TYPE)
            //if (0 != ip)
            if (!ipList.empty() && (0 != ipList[0]))
                CHostMgr::getInstance()->addHost(type, mac, ipList, dpid, port, subnetId, tenantId, fixedIp);

            GET_DATA_INTEGER(type, INT4);
            memset(mac, 0, MAC_LEN);
            ipList.clear();
            //ip = 0;
            dpid = 0;
            port = 0;
            memset(subnetId, 0, sizeof(subnetId));
            memset(tenantId, 0, sizeof(tenantId));
            fixedIp = 0;
            break;
        case HOST_MAC:
            GET_DATA_BYTES(mac, MAC_LEN);
            break;
        case HOST_IP:
            GET_DATA_INTEGER(ip, UINT4);
            ipList.push_back(ip);
            break;
        case HOST_SW_DPID:
            GET_DATA_INTEGER(dpid, UINT8);
            break;
        case HOST_SW_PORT:
            GET_DATA_INTEGER(port, UINT4);
            break;
        case HOST_SUBNET_ID:
            GET_DATA_STRING(subnetId);
            break;
        case HOST_TENANT_ID:
            GET_DATA_STRING(tenantId);
            break;
        case HOST_FIXED_IP:
            GET_DATA_INTEGER(fixedIp, UINT4);
            break;
        default:
            break;
        }
    }

    //last record
    //if (0 != ip)
    if (!ipList.empty() && (0 != ipList[0]))
        CHostMgr::getInstance()->addHost(type, mac, ipList, dpid, port, subnetId, tenantId, fixedIp);

    return BNC_OK;
}

INT4 CClusterSync::syncNatIcmpList(INT4 sockfd)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_NAT_ICMP_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncNatIcmpList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

	CNatIcmpMap& map = CNatIcmpEntryMgr::getInstance()->getNatIcmpMapHead();
	STL_FOR_LOOP(map, iter)
    {
        CNatIcmpEntry* entry = iter->second;
        if (NULL != entry)
        {
#if 0
            INT1 macStr[20] = {0};
            mac2str(entry->getMac(), macStr);
            INT1 ipStr[20] = {0};
            number2ip(entry->getIp(), ipStr);
            LOG_WARN_FMT("### MASTER sync nat icmp: identifier[%d], hostIp[%s], hostMac[%s], dpid[0x%llx], port[%u]", 
                entry->getIdentifier(), ipStr, macStr, entry->getSwDpid(), entry->getInPort());
#endif

            SYNC_DATA_INTEGER(NAT_ICMP_IDENTIFIER, UINT2, entry->getIdentifier());
            SYNC_DATA_INTEGER(NAT_ICMP_HOST_IP,    UINT4, entry->getIp());
            SYNC_DATA_BYTES(NAT_ICMP_MAC,          entry->getMac(), MAC_LEN);
            SYNC_DATA_INTEGER(NAT_ICMP_SW_DPID,    UINT8, entry->getSwDpid());
            SYNC_DATA_INTEGER(NAT_ICMP_INPORT,     UINT4, entry->getInPort());
        }    
    }
    
    if (stopped)
        LOG_WARN("CClusterSync syncNatIcmpList0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_NAT_ICMP_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverNatIcmpList(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

	CNatIcmpMap& map = CNatIcmpEntryMgr::getInstance()->getNatIcmpMapHead();
    map.clear();

    UINT2 identifier = 0;
    UINT4 ip = 0;
    UINT1 mac[MAC_LEN] = {0};
    UINT8 dpid = 0;
    UINT4 port = 0;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(NAT_ICMP_IDENTIFIER);
    GET_DATA_INTEGER(identifier, UINT2);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case NAT_ICMP_IDENTIFIER:
            //one record range in [NAT_ICMP_IDENTIFIER, NAT_ICMP_IDENTIFIER)
            if ((0 != identifier) && (0 != ip) && (0 != dpid))
			    CNatIcmpEntryMgr::getInstance()->AddNatIcmpEntry(identifier, ip, mac, dpid, port);

            GET_DATA_INTEGER(identifier, UINT2);
            ip = 0;
            memset(mac, 0, MAC_LEN);
            dpid = 0;
            port = 0;
            break;
        case NAT_ICMP_HOST_IP:
            GET_DATA_INTEGER(ip, UINT4);
            break;
        case NAT_ICMP_MAC:
            GET_DATA_BYTES(mac, MAC_LEN);
            break;
        case NAT_ICMP_SW_DPID:
            GET_DATA_INTEGER(dpid, UINT8);
            break;
        case NAT_ICMP_INPORT:
            GET_DATA_INTEGER(port, UINT4);
            break;
        default:
            break;
        }
    }

    //last record
    if ((0 != identifier) && (0 != ip) && (0 != dpid))
        CNatIcmpEntryMgr::getInstance()->AddNatIcmpEntry(identifier, ip, mac, dpid, port);

    return BNC_OK;
}

INT4 CClusterSync::syncNatHostList(INT4 sockfd)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_NAT_HOST_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncNatHostList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

	CSNatConnsExtMap& extMap = CSNatConnMgr::getInstance()->getExtMap();
	STL_FOR_LOOP(extMap, extIt)
    {
        CSNatConnsExt& ext = extIt->second;
        STL_FOR_LOOP(ext.m_tcpHosts, hostIt)
        {
            CSNatConnPortMap& portMap = hostIt->second;
            STL_FOR_LOOP(portMap, portIt)
            {
                CSmartPtr<CSNatConn> snatConn = portIt->second;
                if (snatConn.isNull())
                    continue;
            
                SYNC_DATA_INTEGER(NAT_HOST_PROTOCOL,      UINT1, IPPROTO_TCP);
                SYNC_DATA_INTEGER(NAT_HOST_EXTERNAL_IP,   UINT4, snatConn->getExternalIp());
                SYNC_DATA_INTEGER(NAT_HOST_SNAT_IP,       UINT4, snatConn->getSNatIp());
                SYNC_DATA_INTEGER(NAT_HOST_SNAT_PORT,     UINT2, snatConn->getSNatPort());
                SYNC_DATA_INTEGER(NAT_HOST_INTERNAL_IP,   UINT4, snatConn->getInternalIp());
                SYNC_DATA_INTEGER(NAT_HOST_INTERNAL_PORT, UINT2, snatConn->getInternalPort());
                SYNC_DATA_BYTES(NAT_HOST_INTERNAL_MAC,    snatConn->getInternalMac(), MAC_LEN);
                SYNC_DATA_INTEGER(NAT_HOST_GATEWAY_DPID,  UINT8, snatConn->getGatewayDpid());
                SYNC_DATA_INTEGER(NAT_HOST_SWITCH_DPID,   UINT8, snatConn->getSwitchDpid());
            }    
            if (stopped)
                break;
        }    
        if (stopped)
            break;

        STL_FOR_LOOP(ext.m_udpHosts, hostIt)
        {
            CSNatConnPortMap& portMap = hostIt->second;
            STL_FOR_LOOP(portMap, portIt)
            {
                CSmartPtr<CSNatConn> snatConn = portIt->second;
                if (snatConn.isNull())
                    continue;
            
                SYNC_DATA_INTEGER(NAT_HOST_PROTOCOL,      UINT1, IPPROTO_UDP);
                SYNC_DATA_INTEGER(NAT_HOST_EXTERNAL_IP,   UINT4, snatConn->getExternalIp());
                SYNC_DATA_INTEGER(NAT_HOST_SNAT_IP,       UINT4, snatConn->getSNatIp());
                SYNC_DATA_INTEGER(NAT_HOST_SNAT_PORT,     UINT2, snatConn->getSNatPort());
                SYNC_DATA_INTEGER(NAT_HOST_INTERNAL_IP,   UINT4, snatConn->getInternalIp());
                SYNC_DATA_INTEGER(NAT_HOST_INTERNAL_PORT, UINT2, snatConn->getInternalPort());
                SYNC_DATA_BYTES(NAT_HOST_INTERNAL_MAC,    snatConn->getInternalMac(), MAC_LEN);
                SYNC_DATA_INTEGER(NAT_HOST_GATEWAY_DPID,  UINT8, snatConn->getGatewayDpid());
                SYNC_DATA_INTEGER(NAT_HOST_SWITCH_DPID,   UINT8, snatConn->getSwitchDpid());
            }    
            if (stopped)
                break;
        }    
        if (stopped)
            break;
    }

    if (stopped)
        LOG_WARN("CClusterSync syncNatHostList0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_NAT_HOST_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverNatHostList(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

	CSNatConnMgr::getInstance()->clear();

    UINT1 proto = 0;
    UINT4 externalIp = 0;
	UINT4 snatIp = 0;
	UINT2 snatPort = 0;
    UINT4 internalIp = 0;
    UINT2 internalPort = 0;
    UINT1 internalMac[MAC_LEN] = {0};
    UINT8 gatewayDpid = 0;
    UINT8 switchDpid = 0;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(NAT_HOST_PROTOCOL);
    GET_DATA_INTEGER(proto, UINT1);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case NAT_HOST_PROTOCOL:
            //one record range in [NAT_HOST_PROTOCOL, NAT_HOST_PROTOCOL)
            if (((IPPROTO_TCP == proto) || (IPPROTO_UDP == proto)) && 
                (0 != externalIp) && (0 != snatIp) && (0 != internalIp) && 
                (0 != snatPort) && (0 != internalPort))
			    CSNatConnMgr::getInstance()->createSNatConn(externalIp, snatIp, internalIp, 
			        snatPort, internalPort, proto, internalMac, gatewayDpid, switchDpid);

            GET_DATA_INTEGER(proto, UINT1);
            externalIp = 0;
            snatIp = 0;
            snatPort = 0;
            internalIp = 0;
            internalPort = 0;
            memset(internalMac, 0, MAC_LEN);
            gatewayDpid = 0;
            switchDpid = 0;
            break;
        case NAT_HOST_EXTERNAL_IP:
            GET_DATA_INTEGER(externalIp, UINT4);
            break;
        case NAT_HOST_SNAT_IP:
            GET_DATA_INTEGER(snatIp, UINT4);
            break;
        case NAT_HOST_SNAT_PORT:
            GET_DATA_INTEGER(snatPort, UINT2);
            break;
        case NAT_HOST_INTERNAL_IP:
            GET_DATA_INTEGER(internalIp, UINT4);
            break;
        case NAT_HOST_INTERNAL_PORT:
            GET_DATA_INTEGER(internalPort, UINT2);
            break;
        case NAT_HOST_INTERNAL_MAC:
            GET_DATA_BYTES(internalMac, MAC_LEN);
            break;
        case NAT_HOST_GATEWAY_DPID:
            GET_DATA_INTEGER(gatewayDpid, UINT8);
            break;
        case NAT_HOST_SWITCH_DPID:
            GET_DATA_INTEGER(switchDpid, UINT8);
            break;
        default:
            break;
        }
    }

    //last record
    if (((IPPROTO_TCP == proto) || (IPPROTO_UDP == proto)) && 
        (0 != externalIp) && (0 != snatIp) && (0 != internalIp) && 
        (0 != snatPort) && (0 != internalPort))
        CSNatConnMgr::getInstance()->createSNatConn(externalIp, snatIp, internalIp, 
            snatPort, internalPort, proto, internalMac, gatewayDpid, switchDpid);

    return BNC_OK;
}

INT4 CClusterSync::syncAddNatHost(const CSNatConn& snatConn)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_ADD_NAT_HOST_REQ;
    sync.m_sockfd = -1;
    *(CSNatConn*)sync.m_snatConn = snatConn;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncAddNatHost0(const CSNatConn& snatConn)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    do
    {
        SYNC_DATA_INTEGER(NAT_HOST_PROTOCOL,      UINT1, snatConn.getProtocol());
        SYNC_DATA_INTEGER(NAT_HOST_EXTERNAL_IP,   UINT4, snatConn.getExternalIp());
        SYNC_DATA_INTEGER(NAT_HOST_SNAT_IP,       UINT4, snatConn.getSNatIp());
        SYNC_DATA_INTEGER(NAT_HOST_SNAT_PORT,     UINT2, snatConn.getSNatPort());
        SYNC_DATA_INTEGER(NAT_HOST_INTERNAL_IP,   UINT4, snatConn.getInternalIp());
        SYNC_DATA_INTEGER(NAT_HOST_INTERNAL_PORT, UINT2, snatConn.getInternalPort());
        SYNC_DATA_BYTES(NAT_HOST_INTERNAL_MAC,    snatConn.getInternalMac(), MAC_LEN);
        SYNC_DATA_INTEGER(NAT_HOST_GATEWAY_DPID,  UINT8, snatConn.getGatewayDpid());
        SYNC_DATA_INTEGER(NAT_HOST_SWITCH_DPID,   UINT8, snatConn.getSwitchDpid());
    } while (FALSE);

    if (stopped)
        LOG_WARN("CClusterSync syncAddNatHost0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_ADD_NAT_HOST_REQ);
}

INT4 CClusterSync::recoverAddNatHost(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    UINT1 proto = 0;
    UINT4 externalIp = 0;
	UINT4 snatIp = 0;
	UINT2 snatPort = 0;
    UINT4 internalIp = 0;
    UINT2 internalPort = 0;
    UINT1 internalMac[MAC_LEN] = {0};
    UINT8 gatewayDpid = 0;
    UINT8 switchDpid = 0;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(NAT_HOST_PROTOCOL);
    GET_DATA_INTEGER(proto, UINT1);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case NAT_HOST_EXTERNAL_IP:
            GET_DATA_INTEGER(externalIp, UINT4);
            break;
        case NAT_HOST_SNAT_IP:
            GET_DATA_INTEGER(snatIp, UINT4);
            break;
        case NAT_HOST_SNAT_PORT:
            GET_DATA_INTEGER(snatPort, UINT2);
            break;
        case NAT_HOST_INTERNAL_IP:
            GET_DATA_INTEGER(internalIp, UINT4);
            break;
        case NAT_HOST_INTERNAL_PORT:
            GET_DATA_INTEGER(internalPort, UINT2);
            break;
        case NAT_HOST_INTERNAL_MAC:
            GET_DATA_BYTES(internalMac, MAC_LEN);
            break;
        case NAT_HOST_GATEWAY_DPID:
            GET_DATA_INTEGER(gatewayDpid, UINT8);
            break;
        case NAT_HOST_SWITCH_DPID:
            GET_DATA_INTEGER(switchDpid, UINT8);
            break;
        default:
            break;
        }
    }

    if (((IPPROTO_TCP == proto) || (IPPROTO_UDP == proto)) && 
        (0 != externalIp) && (0 != snatIp) && (0 != internalIp) && 
        (0 != snatPort) && (0 != internalPort))
        CSNatConnMgr::getInstance()->createSNatConn(externalIp, snatIp, internalIp, 
            snatPort, internalPort, proto, internalMac, gatewayDpid, switchDpid);

    return BNC_OK;
}

INT4 CClusterSync::syncDelNatHost(const CSNatConn& snatConn)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_DEL_NAT_HOST_REQ;
    sync.m_sockfd = -1;
    *(CSNatConn*)sync.m_snatConn = snatConn;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncDelNatHost0(const CSNatConn& snatConn)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    do
    {
        SYNC_DATA_INTEGER(NAT_HOST_PROTOCOL,      UINT1, snatConn.getProtocol());
        SYNC_DATA_INTEGER(NAT_HOST_EXTERNAL_IP,   UINT4, snatConn.getExternalIp());
        SYNC_DATA_INTEGER(NAT_HOST_SNAT_IP,       UINT4, snatConn.getSNatIp());
        SYNC_DATA_INTEGER(NAT_HOST_SNAT_PORT,     UINT2, snatConn.getSNatPort());
        SYNC_DATA_INTEGER(NAT_HOST_INTERNAL_IP,   UINT4, snatConn.getInternalIp());
        SYNC_DATA_INTEGER(NAT_HOST_INTERNAL_PORT, UINT2, snatConn.getInternalPort());
        SYNC_DATA_BYTES(NAT_HOST_INTERNAL_MAC,    snatConn.getInternalMac(), MAC_LEN);
        SYNC_DATA_INTEGER(NAT_HOST_GATEWAY_DPID,  UINT8, snatConn.getGatewayDpid());
        SYNC_DATA_INTEGER(NAT_HOST_SWITCH_DPID,   UINT8, snatConn.getSwitchDpid());
    } while (FALSE);

    if (stopped)
        LOG_WARN("CClusterSync syncDelNatHost0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_DEL_NAT_HOST_REQ);
}

INT4 CClusterSync::recoverDelNatHost(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    UINT1 proto = 0;
    UINT4 externalIp = 0;
	UINT4 snatIp = 0;
	UINT2 snatPort = 0;
    UINT4 internalIp = 0;
    UINT2 internalPort = 0;
    UINT1 internalMac[MAC_LEN] = {0};
    UINT8 gatewayDpid = 0;
    UINT8 switchDpid = 0;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(NAT_HOST_PROTOCOL);
    GET_DATA_INTEGER(proto, UINT1);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case NAT_HOST_EXTERNAL_IP:
            GET_DATA_INTEGER(externalIp, UINT4);
            break;
        case NAT_HOST_SNAT_IP:
            GET_DATA_INTEGER(snatIp, UINT4);
            break;
        case NAT_HOST_SNAT_PORT:
            GET_DATA_INTEGER(snatPort, UINT2);
            break;
        case NAT_HOST_INTERNAL_IP:
            GET_DATA_INTEGER(internalIp, UINT4);
            break;
        case NAT_HOST_INTERNAL_PORT:
            GET_DATA_INTEGER(internalPort, UINT2);
            break;
        case NAT_HOST_INTERNAL_MAC:
            GET_DATA_BYTES(internalMac, MAC_LEN);
            break;
        case NAT_HOST_GATEWAY_DPID:
            GET_DATA_INTEGER(gatewayDpid, UINT8);
            break;
        case NAT_HOST_SWITCH_DPID:
            GET_DATA_INTEGER(switchDpid, UINT8);
            break;
        default:
            break;
        }
    }

    //just to avoid compiling warnings
    LOG_TRACE_FMT("snat[0x%04x:%u],gateway[0x%llx],switch[0x%llx]", snatIp, snatPort, gatewayDpid, switchDpid);

    if (((IPPROTO_TCP == proto) || (IPPROTO_UDP == proto)) && 
        (0 != externalIp) && (0 != internalIp) && (0 != internalPort))
        CSNatConnMgr::getInstance()->removeSNatConnByInt(externalIp, internalIp, internalPort, proto);

    return BNC_OK;
}

INT4 CClusterSync::syncTopoLinkList(INT4 sockfd)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_TOPO_LINK_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncTopoLinkList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    std::vector<CNeighborMap>& neighMapVec = CControl::getInstance()->getTopoMgr().getNeighMapVector();
    STL_FOR_LOOP(neighMapVec, neighMapIt)
    {
        STL_FOR_LOOP(*neighMapIt, dpidIt)
        {
            CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpidIt->first);
            if (sw.isNull() || (sw->getState() >= SW_STATE_POWER_OFF))
                continue;

            STL_FOR_LOOP(dpidIt->second, portIt)
            {
                neighbor_t& neigh = portIt->second;
                if (NEIGH_STATE_DELETED == neigh.state)
                    continue;

                SYNC_DATA_INTEGER(TOPO_LINK_STATE,    INT4,  neigh.state);
                SYNC_DATA_INTEGER(TOPO_LINK_SRC_DPID, UINT8, neigh.src_dpid);
                SYNC_DATA_INTEGER(TOPO_LINK_SRC_PORT, UINT4, neigh.src_port);
                SYNC_DATA_INTEGER(TOPO_LINK_DST_DPID, UINT8, neigh.dst_dpid);
                SYNC_DATA_INTEGER(TOPO_LINK_DST_PORT, UINT4, neigh.dst_port);
                SYNC_DATA_INTEGER(TOPO_LINK_WEIGHT,   UINT8, neigh.weight);
            }
            if (stopped)
                break;
        }    
        if (stopped)
            break;
    }

    if (stopped)
        LOG_WARN("CClusterSync syncTopoLinkList0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_TOPO_LINK_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverTopoLinkList(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    std::vector<CNeighborMap>& neighMapVec = CControl::getInstance()->getTopoMgr().getNeighMapVector();
    STL_FOR_LOOP(neighMapVec, neighMapIt)
        STL_FOR_LOOP(*neighMapIt, dpidIt)
            dpidIt->second.clear();

    neighbor_t neigh = {0};

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(TOPO_LINK_STATE);
    GET_DATA_INTEGER(neigh.state, INT4);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case TOPO_LINK_STATE:
            //one record range in [TOPO_LINK_STATE, TOPO_LINK_STATE)
            if ((0 != neigh.src_dpid) && (0 != neigh.dst_dpid))
                CControl::getInstance()->getTopoMgr().recoverLink(neigh);

            memset(&neigh, 0, sizeof(neigh));
            GET_DATA_INTEGER(neigh.state, INT4);
            break;
        case TOPO_LINK_SRC_DPID:
            GET_DATA_INTEGER(neigh.src_dpid, UINT8);
            break;
        case TOPO_LINK_SRC_PORT:
            GET_DATA_INTEGER(neigh.src_port, UINT4);
            break;
        case TOPO_LINK_DST_DPID:
            GET_DATA_INTEGER(neigh.dst_dpid, UINT8);
            break;
        case TOPO_LINK_DST_PORT:
            GET_DATA_INTEGER(neigh.dst_port, UINT4);
            break;
        case TOPO_LINK_WEIGHT:
            GET_DATA_INTEGER(neigh.weight, UINT8);
            break;
        default:
            break;
        }
    }

    //last record
    if ((0 != neigh.src_dpid) && (0 != neigh.dst_dpid))
        CControl::getInstance()->getTopoMgr().recoverLink(neigh);
    CControl::getInstance()->getTopoMgr().notifyPathChanged();

    return BNC_OK;
}

INT4 CClusterSync::syncTopoLink(const neighbor_t& neigh)
{
    if (!syncNeeded())
        return BNC_ERR;

    LOG_WARN_FMT("*** syncTopoLink with state[%d]dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u] ***", 
        neigh.state, neigh.src_dpid, neigh.src_port, neigh.dst_dpid, neigh.dst_port);

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_TOPO_LINK_REQ;
    sync.m_sockfd = -1;
    sync.m_neigh = neigh;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncTopoLink0(const neighbor_t& neigh)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    do
    {
        SYNC_DATA_INTEGER(TOPO_LINK_STATE,    INT4,  neigh.state);
        SYNC_DATA_INTEGER(TOPO_LINK_SRC_DPID, UINT8, neigh.src_dpid);
        SYNC_DATA_INTEGER(TOPO_LINK_SRC_PORT, UINT4, neigh.src_port);
        SYNC_DATA_INTEGER(TOPO_LINK_DST_DPID, UINT8, neigh.dst_dpid);
        SYNC_DATA_INTEGER(TOPO_LINK_DST_PORT, UINT4, neigh.dst_port);
        SYNC_DATA_INTEGER(TOPO_LINK_WEIGHT,   UINT8, neigh.weight);
    } while (FALSE);

    if (stopped)
        LOG_WARN("CClusterSync syncTopoLink0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_TOPO_LINK_REQ);
}

INT4 CClusterSync::recoverTopoLink(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    neighbor_t neigh = {0};

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(TOPO_LINK_STATE);
    GET_DATA_INTEGER(neigh.state, INT4);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case TOPO_LINK_SRC_DPID:
            GET_DATA_INTEGER(neigh.src_dpid, UINT8);
            break;
        case TOPO_LINK_SRC_PORT:
            GET_DATA_INTEGER(neigh.src_port, UINT4);
            break;
        case TOPO_LINK_DST_DPID:
            GET_DATA_INTEGER(neigh.dst_dpid, UINT8);
            break;
        case TOPO_LINK_DST_PORT:
            GET_DATA_INTEGER(neigh.dst_port, UINT4);
            break;
        case TOPO_LINK_WEIGHT:
            GET_DATA_INTEGER(neigh.weight, UINT8);
            break;
        default:
            break;
        }
    }

    if ((0 != neigh.src_dpid) && (0 != neigh.dst_dpid))
    {
        CControl::getInstance()->getTopoMgr().recoverLink(neigh);
        CControl::getInstance()->getTopoMgr().notifyPathChanged();
        return BNC_OK;
    }
    
    return BNC_ERR;
}

INT4 CClusterSync::syncSwitchTagList(INT4 sockfd)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_SW_TAG_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncSwitchTagList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    CSwitchHMap& swMap = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    STL_FOR_LOOP(swMap, swit)
    {
		CSmartPtr<CSwitch> sw = swit->second;
		if (sw.isNotNull() && (0 != sw->getDpid()))
		{
            SYNC_DATA_INTEGER(SW_TAG_DPID, UINT8, sw->getDpid());
            SYNC_DATA_INTEGER(SW_TAG_TAG,  UINT4, sw->getTag());
        }
    }
    
	CControl::getInstance()->getSwitchMgr().unlock();

    if (stopped)
        LOG_WARN("CClusterSync syncSwitchTagList0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_SW_TAG_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverSwitchTagList(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    UINT8 dpid = 0;
    UINT4 tag = 0;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(SW_TAG_DPID);
    GET_DATA_INTEGER(dpid, UINT8);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case SW_TAG_DPID:
            //one record range in [SW_TAG_DPID, SW_TAG_DPID)
            if ((0 != dpid) && (0 != tag))
            {
                CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
                if (sw.isNotNull())
                    sw->setTag(tag);
            }

            GET_DATA_INTEGER(dpid, UINT8);
            tag = 0;
            break;
        case SW_TAG_TAG:
            GET_DATA_INTEGER(tag, UINT4);
            break;
        default:
            break;
        }
    }

    //last record
    if ((0 != dpid) && (0 != tag))
    {
        CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
        if (sw.isNotNull())
            sw->setTag(tag);
    }

    return BNC_OK;
}

INT4 CClusterSync::syncSwitchTag(UINT8 dpid, UINT4 tag)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_TOPO_LINK_REQ;
    sync.m_sockfd = -1;
    *(UINT8*)sync.m_dpidTag = dpid;
    *(UINT4*)(sync.m_dpidTag+8) = tag;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncSwitchTag0(UINT8 dpid, UINT4 tag)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    do
    {
        SYNC_DATA_INTEGER(SW_TAG_DPID, UINT8, dpid);
        SYNC_DATA_INTEGER(SW_TAG_TAG,  UINT4, tag);
    } while (FALSE);

    if (stopped)
        LOG_WARN("CClusterSync syncSwitchTag0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_SW_TAG_REQ);
}

INT4 CClusterSync::recoverSwitchTag(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    UINT8 dpid = 0;
    UINT4 tag = 0;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(SW_TAG_DPID);
    GET_DATA_INTEGER(dpid, UINT8);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case SW_TAG_TAG:
            GET_DATA_INTEGER(tag, UINT4);
            break;
        default:
            break;
        }
    }

    if ((0 != dpid) && (0 != tag))
    {
        CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
        if (sw.isNotNull())
        {
            sw->setTag(tag);
            return BNC_OK;
        }
    }

    return BNC_ERR;
}

INT4 CClusterSync::syncTagInfo(INT4 sockfd)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_TAG_INFO_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncTagInfo0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    tlv_t* tlv = (tlv_t*)m_fieldMem;
    tlv->type = TAG_INFO_VALUE;
    tlv->len = CControl::getInstance()->getTagMgr().serialize(tlv->data);
    m_fieldLen += sizeof(tlv_t) + tlv->len;
    
    return sendReq(CLUSTER_OPER_SYNC_TAG_INFO_REQ, sockfd);
}

INT4 CClusterSync::recoverTagInfo(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(TAG_INFO_VALUE);

    return CControl::getInstance()->getTagMgr().recover(tlv->data, tlv->len);
}

INT4 CClusterSync::syncFlowEntryList(INT4 sockfd)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncFlowEntryList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    CSwitchFlowsMap& swMap = CFlowMgr::getInstance()->getFlowCache().getFlowsMap();
	STL_FOR_LOOP(swMap, swIt)
    {
        CTableIdFlowsMap& tableMap = swIt->second;
        STL_FOR_LOOP(tableMap, tableIt)
        {
            CFlowList& flowList = tableIt->second;
            STL_FOR_LOOP(flowList, flowIt)
            {
                CFlow& flow = *flowIt;            
                //LOG_WARN_FMT("dpid[0x%llx],uuid[%s],table_id[%d]", 
                //    flow.getSw()->getDpid(), flow.getUuid().c_str(), flow.getTableId());

                SYNC_DATA_INTEGER(FLOW_ENTRY_SW_DPID,      UINT8, flow.getSw()->getDpid());
                SYNC_DATA_STRING(FLOW_ENTRY_UUID,          flow.getUuid());
                SYNC_DATA_INTEGER(FLOW_ENTRY_CREATE_TIME,  UINT8, flow.getCreateTime());
                SYNC_DATA_INTEGER(FLOW_ENTRY_TABLE_ID,     UINT1, flow.getTableId());
                SYNC_DATA_INTEGER(FLOW_ENTRY_IDLE_TIMEOUT, UINT2, flow.getIdleTimeout());
                SYNC_DATA_INTEGER(FLOW_ENTRY_HARD_TIMEOUT, UINT2, flow.getHardTimeout());
                SYNC_DATA_INTEGER(FLOW_ENTRY_PRIORITY,     UINT2, flow.getPriority());
                SYNC_DATA_INTEGER(FLOW_ENTRY_MATCH,        UINT1, 1);

                //match
                SYNC_OXM_FIELDS(flow.getMatch().oxm_fields);
                
                //instructions
                SYNC_DATA_INTEGER(FLOW_ENTRY_INSTRUCTIONS, UINT1, 1);

                CFlowInstructionMap& instructionMap = flow.getInstructions();
                STL_FOR_LOOP(instructionMap, instructionIt)
                {
                    CFlowInstruction& instruction = instructionIt->second;
                    switch (instruction.m_type)
                    {
                        case OFPIT_GOTO_TABLE:
                            SYNC_DATA_INTEGER(FLOW_ENTRY_INSTRUCION_GOTOTABLE, UINT1, instruction.m_table_id);
                            break;                        
                        case OFPIT_WRITE_METADATA:
                        {
                            UINT8 meta[2] = {instruction.m_metadata, instruction.m_metadata_mask};
                            SYNC_DATA_BYTES(FLOW_ENTRY_INSTRUCION_WRITEMETADATA, meta, sizeof(meta));
                            break;
                        }
                        case OFPIT_WRITE_ACTIONS:
                        case OFPIT_APPLY_ACTIONS:
                        {
                            SYNC_DATA_INTEGER((OFPIT_WRITE_ACTIONS==instruction.m_type)?FLOW_ENTRY_INSTRUCION_WRITEACTIONS:FLOW_ENTRY_INSTRUCION_APPLYACTIONS, UINT1, 1);

                            STL_FOR_LOOP(instruction.m_actions, actionIt)
                            {
                                CFlowAction& action = actionIt->second;
                                switch (action.m_type)
                                {
                                    case OFPAT13_OUTPUT:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_OUTPUT, UINT4, action.m_port);
                                        break;                        
                                    case OFPAT13_MPLS_TTL:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_MPLSTTL, UINT1, action.m_mpls_tt);
                                        break;                        
                                    case OFPAT13_PUSH_VLAN:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_PUSHVLAN, UINT1, 1);
                                        break;                        
                                    case OFPAT13_POP_VLAN:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_POPVLAN, UINT1, 1);
                                        break;                        
                                    case OFPAT13_PUSH_MPLS:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_PUSHMPLS, UINT1, 1);
                                        break;                        
                                    case OFPAT13_POP_MPLS:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_POPMPLS, UINT1, 1);
                                        break;                        
                                    case OFPAT13_SET_QUEUE:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_SETQUEUE, UINT4, action.m_queue_id);
                                        break;                        
                                    case OFPAT13_GROUP:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_GROUP, UINT4, action.m_group_id);
                                        break;                        
                                    case OFPAT13_SET_NW_TTL:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_SETNWTTL, UINT1, action.m_nw_tt);
                                        break;                        
                                    case OFPAT13_SET_FIELD:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_SETFIELD, UINT1, 1);
                                        SYNC_OXM_FIELDS(action.m_oxm_fields);
                                        break;                        
                                    case OFPAT13_PUSH_PBB:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_PUSHPBB, UINT1, 1);
                                        break;                        
                                    case OFPAT13_POP_PBB:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_POPPBB, UINT1, 1);
                                        break;                        
                                    case OFPAT13_EXPERIMENTER:
                                        SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_EXPERIMENTER, UINT4, action.m_experimenter);
                                        break;                        
                                    
                                    default:
                                        break;
                                }
                                if (stopped)
                                    break;
                            }
                            break;
                        }
                        case OFPIT_CLEAR_ACTIONS:
                            SYNC_DATA_INTEGER(FLOW_ENTRY_INSTRUCION_CLEARACTIONS, UINT1, 1);
                            break;                        
                        case OFPIT_METER:
                            SYNC_DATA_INTEGER(FLOW_ENTRY_INSTRUCION_METER, UINT4, instruction.m_meter_id);
                            break;
                        case OFPIT_EXPERIMENTER:
                        {
                            UINT4 experimenter[2] = {instruction.m_experimenterLen, instruction.m_experimenter};
                            SYNC_DATA_BYTES(FLOW_ENTRY_INSTRUCION_WRITEMETADATA, experimenter, sizeof(experimenter));
                            break;
                        }
                        
                        default:
                            break;
                    }
                    if (stopped)
                        break;
                }
                if (stopped)
                    break;
            }    
            if (stopped)
                break;
        }    
        if (stopped)
            break;
    }

    if (stopped)
        LOG_WARN("CClusterSync syncFlowEntryList0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverFlowEntryList(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    UINT8 dpid = 0;
    gn_flow_t flow = {0};
    gn_oxm_t oxm = {0};
    gn_action_t* actions = NULL;
    BOOL match_oxm = FALSE;

	CFlowMgr::getInstance()->getFlowCache().clear();

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(FLOW_ENTRY_SW_DPID);
    GET_DATA_INTEGER(dpid, UINT8);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case FLOW_ENTRY_SW_DPID:
        {
            //one record range in [FLOW_ENTRY_SW_DPID, FLOW_ENTRY_SW_DPID)
            recoverFlowEntry(dpid, flow, actions);
            memset(&flow, 0, sizeof(flow));
            memset(&oxm, 0, sizeof(oxm));
            actions = NULL;
            match_oxm = FALSE;

            GET_DATA_INTEGER(dpid, UINT8);
            break;
        }

        case FLOW_ENTRY_UUID:
            GET_DATA_STRING(flow.uuid);
            break;
        case FLOW_ENTRY_CREATE_TIME:
            GET_DATA_INTEGER(flow.create_time, UINT8);
            break;
        case FLOW_ENTRY_TABLE_ID:
            GET_DATA_INTEGER(flow.table_id, UINT1);
            break;
        case FLOW_ENTRY_IDLE_TIMEOUT:
            GET_DATA_INTEGER(flow.idle_timeout, UINT2);
            break;
        case FLOW_ENTRY_HARD_TIMEOUT:
            GET_DATA_INTEGER(flow.hard_timeout, UINT2);
            break;
        case FLOW_ENTRY_PRIORITY:
            GET_DATA_INTEGER(flow.priority, UINT2);
            break;

        case FLOW_ENTRY_MATCH:
        {
            UINT1 match = 0;
            GET_DATA_INTEGER(match, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("match[%d]", match);

            memset(&oxm, 0, sizeof(gn_oxm_t));
            match_oxm = TRUE;
            break;
        }

        //oxm fields
        case FLOW_ENTRY_OXM_INPORT:
        case FLOW_ENTRY_OXM_INPHYPORT:
        case FLOW_ENTRY_OXM_METADATA:
        case FLOW_ENTRY_OXM_ETHDST:
        case FLOW_ENTRY_OXM_ETHSRC:
        case FLOW_ENTRY_OXM_ETHTYPE:
        case FLOW_ENTRY_OXM_VLANID:
        case FLOW_ENTRY_OXM_VLANPCP:
        case FLOW_ENTRY_OXM_IPDSCP:
        case FLOW_ENTRY_OXM_IPECN:
        case FLOW_ENTRY_OXM_IPPROTO:
        case FLOW_ENTRY_OXM_IPV4SRC:
        case FLOW_ENTRY_OXM_IPV4DST:
        case FLOW_ENTRY_OXM_TCPSRC:
        case FLOW_ENTRY_OXM_TCPDST:
        case FLOW_ENTRY_OXM_UDPSRC:
        case FLOW_ENTRY_OXM_UDPDST:
        case FLOW_ENTRY_OXM_ICMPV4TYPE:
        case FLOW_ENTRY_OXM_ICMPV4CODE:
        case FLOW_ENTRY_OXM_ARPOP:
        case FLOW_ENTRY_OXM_ARPSPA:
        case FLOW_ENTRY_OXM_ARPTPA:
        case FLOW_ENTRY_OXM_ARPSHA:
        case FLOW_ENTRY_OXM_ARPTHA:
        case FLOW_ENTRY_OXM_IPV6SRC:
        case FLOW_ENTRY_OXM_IPV6DST:
        case FLOW_ENTRY_OXM_MPLSLABEL:
        case FLOW_ENTRY_OXM_TUNNELID:
        case FLOW_ENTRY_OXM_IPV4SRCPREFIX:
        case FLOW_ENTRY_OXM_IPV4DSTPREFIX:
        case FLOW_ENTRY_OXM_IPV6SRCPREFIX:
        case FLOW_ENTRY_OXM_IPV6DSTPREFIX:
        case FLOW_ENTRY_OXM_METADATAMASK:
            RECOVER_OXM_FIELDS(tlv->type, oxm);
            break;
        case FLOW_ENTRY_OXM_MASK:
        {
            GET_DATA_INTEGER(oxm.mask, UINT8);
            if (match_oxm)
            {
                flow.match.type = OFPMT_OXM;
                memcpy(&(flow.match.oxm_fields), &oxm, sizeof(gn_oxm_t));
            }
            else
            {
                for (gn_action_t* action = actions; NULL != action; action = action->next)
                {
                    if (OFPAT13_SET_FIELD == action->type)
                    {
                        gn_action_set_field_t* setField = (gn_action_set_field_t*)action;
                        memcpy(&(setField->oxm_fields), &oxm, sizeof(gn_oxm_t));
                        break;
                    }
                }
            }
            memset(&oxm, 0, sizeof(gn_oxm_t));
            break;
        }

        case FLOW_ENTRY_INSTRUCTIONS:
        {
            UINT1 instruct = 0;
            GET_DATA_INTEGER(instruct, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("instruct[%d]", instruct);

            flow.instructions = NULL;
            memset(&oxm, 0, sizeof(gn_oxm_t));
            actions = NULL;
            match_oxm = FALSE;
            break;
        }

        //instructions
        case FLOW_ENTRY_INSTRUCION_GOTOTABLE:
        {
            gn_instruction_goto_table_t* instruct = (gn_instruction_goto_table_t*)bnc_malloc(sizeof(gn_instruction_goto_table_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_goto_table_t failed !");
                return BNC_ERR;
            }

            instruct->type = OFPIT_GOTO_TABLE;
            GET_DATA_INTEGER(instruct->table_id, UINT1);
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_WRITEMETADATA:
        {
            gn_instruction_write_metadata_t* instruct = (gn_instruction_write_metadata_t*)bnc_malloc(sizeof(gn_instruction_write_metadata_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_write_metadata_t failed !");
                return BNC_ERR;
            }

            UINT8 meta[2] = {0};
            GET_DATA_BYTES(meta, sizeof(meta));
            instruct->type = OFPIT_WRITE_METADATA;
            instruct->metadata = meta[0];
            instruct->metadata_mask = meta[1];
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_WRITEACTIONS:
        case FLOW_ENTRY_INSTRUCION_APPLYACTIONS:
        {
            gn_instruction_actions_t* instruct = (gn_instruction_actions_t*)bnc_malloc(sizeof(gn_instruction_actions_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_write_metadata_t failed !");
                return BNC_ERR;
            }

            UINT1 applyActions = 0;
            GET_DATA_INTEGER(applyActions, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("applyActions[%d]", applyActions);

            instruct->type = (FLOW_ENTRY_INSTRUCION_WRITEACTIONS==tlv->type)?OFPIT_WRITE_ACTIONS:OFPIT_APPLY_ACTIONS;
            instruct->actions = NULL;
            //will set actions later
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_CLEARACTIONS:
        {
            gn_instruction_t* instruct = (gn_instruction_t*)bnc_malloc(sizeof(gn_instruction_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_t failed !");
                return BNC_ERR;
            }

            UINT1 clearActions = 0;
            GET_DATA_INTEGER(clearActions, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("clearActions[%d]", clearActions);

            instruct->type = OFPIT_CLEAR_ACTIONS;
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_METER:
        {
            gn_instruction_meter_t* instruct = (gn_instruction_meter_t*)bnc_malloc(sizeof(gn_instruction_meter_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_meter_t failed !");
                return BNC_ERR;
            }

            instruct->type = OFPIT_METER;
            GET_DATA_INTEGER(instruct->meter_id, UINT4);
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_EXPERIMENTER:
        {
            gn_instruction_experimenter_t* instruct = (gn_instruction_experimenter_t*)bnc_malloc(sizeof(gn_instruction_experimenter_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_experimenter_t failed !");
                return BNC_ERR;
            }

            UINT4 experimenter[2] = {0};
            GET_DATA_BYTES(experimenter, sizeof(experimenter));
            instruct->type = OFPIT_EXPERIMENTER;
            instruct->len = (UINT2)experimenter[0];
            instruct->experimenter = experimenter[1];
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }

        //actions
        case FLOW_ENTRY_ACION_OUTPUT:
        {
            gn_action_output_t* act = (gn_action_output_t*)bnc_malloc(sizeof(gn_action_output_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_output_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_OUTPUT;
            GET_DATA_INTEGER(act->port, UINT4);
            act->max_len = (OFPP13_CONTROLLER==act->port)?0xffff:0;
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_MPLSTTL:
        {
            gn_action_mpls_ttl_t* act = (gn_action_mpls_ttl_t*)bnc_malloc(sizeof(gn_action_mpls_ttl_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_mpls_ttl_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_MPLS_TTL;
            GET_DATA_INTEGER(act->mpls_tt, UINT1);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_PUSHVLAN:
        case FLOW_ENTRY_ACION_POPVLAN:
        case FLOW_ENTRY_ACION_PUSHMPLS:
        case FLOW_ENTRY_ACION_POPMPLS:
        case FLOW_ENTRY_ACION_PUSHPBB:
        case FLOW_ENTRY_ACION_POPPBB:
        {
            gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_t failed !");
                return BNC_ERR;
            }

            UINT1 pushPop = 0;
            GET_DATA_INTEGER(pushPop, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("pushPop[%d]", pushPop);

            act->type = (FLOW_ENTRY_ACION_PUSHVLAN==tlv->type)?OFPAT13_PUSH_VLAN:
                        (FLOW_ENTRY_ACION_POPVLAN==tlv->type)?OFPAT13_POP_VLAN:
                        (FLOW_ENTRY_ACION_PUSHMPLS==tlv->type)?OFPAT13_PUSH_MPLS:
                        (FLOW_ENTRY_ACION_POPMPLS==tlv->type)?OFPAT13_POP_MPLS:
                        (FLOW_ENTRY_ACION_PUSHPBB==tlv->type)?OFPAT13_PUSH_PBB:OFPAT13_POP_PBB;
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_SETQUEUE:
        {
            gn_action_set_queue_t* act = (gn_action_set_queue_t*)bnc_malloc(sizeof(gn_action_set_queue_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_set_queue_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_SET_QUEUE;
            GET_DATA_INTEGER(act->queue_id, UINT4);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_GROUP:
        {
            gn_action_group_t* act = (gn_action_group_t*)bnc_malloc(sizeof(gn_action_group_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_group_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_GROUP;
            GET_DATA_INTEGER(act->group_id, UINT4);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_SETNWTTL:
        {
            gn_action_nw_ttl_t* act = (gn_action_nw_ttl_t*)bnc_malloc(sizeof(gn_action_nw_ttl_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_nw_ttl_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_SET_NW_TTL;
            GET_DATA_INTEGER(act->nw_tt, UINT1);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_SETFIELD:
        {
            gn_action_set_field_t* act = (gn_action_set_field_t*)bnc_malloc(sizeof(gn_action_set_field_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_set_field_t failed !");
                return BNC_ERR;
            }

            UINT1 setField = 0;
            GET_DATA_INTEGER(setField, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("setField[%d]", setField);

            act->type = OFPAT13_SET_FIELD;
            memset(&(act->oxm_fields), 0, sizeof(gn_oxm_t));
            //will set oxm_fields later
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_EXPERIMENTER:
        {
            gn_action_experimenter_t* act = (gn_action_experimenter_t*)bnc_malloc(sizeof(gn_action_experimenter_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_experimenter_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_EXPERIMENTER;
            GET_DATA_INTEGER(act->experimenter, UINT4);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }

        default:
            break;
        }
    }

    //last record
    recoverFlowEntry(dpid, flow, actions);

    return BNC_OK;
}

void CClusterSync::recoverFlowEntry(UINT8 dpid, gn_flow_t& flow, gn_action_t* actions)
{
    //LOG_WARN_FMT("dpid[0x%llx],uuid[%s],table_id[%d]", dpid, flow.uuid, flow.table_id);

    if ((NULL != flow.instructions) && (NULL != actions))
    {
        for (gn_instruction_t* instruction = flow.instructions; NULL != instruction; instruction = instruction->next)
        {
            if ((OFPIT_WRITE_ACTIONS == instruction->type) ||
                (OFPIT_APPLY_ACTIONS == instruction->type))
            {
                gn_instruction_actions_t* instructionActions = (gn_instruction_actions_t*)instruction;
                instructionActions->actions = actions;
                actions = NULL;
                break;
            }
        }
    }
    if (NULL != actions)
    {
        gn_action_t* action = actions;
        while (NULL != action)
        {
            gn_action_t* next = action->next;
            bnc_free(action);
            action = next;
        }
    }

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
    if (sw.isNotNull())
        CFlowTableEventReportor::getInstance()->report(EVENT_TYPE_FLOW_TABLE_RECOVER, EVENT_REASON_FLOW_TABLE_RECOVER, sw, flow);

	CFlowMod::clear_instruction_data(flow.instructions);
}

INT4 CClusterSync::syncAddFlowEntry(UINT8 dpid, const INT1* uuid)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_REQ;
    sync.m_sockfd = -1;
    *(UINT8*)sync.m_dpidUuid = dpid;
    strncpy(sync.m_dpidUuid+8, uuid, UUID_LEN-1);
    sync.m_dpidUuid[8+UUID_LEN-1] = '\0';

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncAddFlowEntry0(UINT8 dpid, const INT1* uuid)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    CFlow* flow = CFlowMgr::getInstance()->getFlowCache().findFlow(dpid, uuid);
	if (NULL == flow)
	{
        LOG_WARN_FMT("failed to get flow by dpid[0x%llx] and uuid[%s]", dpid, uuid);
        return BNC_ERR;
	}

    do
    {
        SYNC_DATA_INTEGER(FLOW_ENTRY_SW_DPID,      UINT8, dpid);
        SYNC_DATA_STRING(FLOW_ENTRY_UUID,          flow->getUuid());
        SYNC_DATA_INTEGER(FLOW_ENTRY_CREATE_TIME,  UINT8, flow->getCreateTime());
        SYNC_DATA_INTEGER(FLOW_ENTRY_TABLE_ID,     UINT1, flow->getTableId());
        SYNC_DATA_INTEGER(FLOW_ENTRY_IDLE_TIMEOUT, UINT2, flow->getIdleTimeout());
        SYNC_DATA_INTEGER(FLOW_ENTRY_HARD_TIMEOUT, UINT2, flow->getHardTimeout());
        SYNC_DATA_INTEGER(FLOW_ENTRY_PRIORITY,     UINT2, flow->getPriority());
        SYNC_DATA_INTEGER(FLOW_ENTRY_MATCH,        UINT1, 1);

        //match
        SYNC_OXM_FIELDS(flow->getMatch().oxm_fields);
        
        //instructions
        SYNC_DATA_INTEGER(FLOW_ENTRY_INSTRUCTIONS, UINT1, 1);

        CFlowInstructionMap& instructionMap = flow->getInstructions();
        STL_FOR_LOOP(instructionMap, instructionIt)
        {
            CFlowInstruction& instruction = instructionIt->second;
            switch (instruction.m_type)
            {
                case OFPIT_GOTO_TABLE:
                    SYNC_DATA_INTEGER(FLOW_ENTRY_INSTRUCION_GOTOTABLE, UINT1, instruction.m_table_id);
                    break;                        
                case OFPIT_WRITE_METADATA:
                {
                    UINT8 meta[2] = {instruction.m_metadata, instruction.m_metadata_mask};
                    SYNC_DATA_BYTES(FLOW_ENTRY_INSTRUCION_WRITEMETADATA, meta, sizeof(meta));
                    break;
                }
                case OFPIT_WRITE_ACTIONS:
                case OFPIT_APPLY_ACTIONS:
                {
                    SYNC_DATA_INTEGER((OFPIT_WRITE_ACTIONS==instruction.m_type)?FLOW_ENTRY_INSTRUCION_WRITEACTIONS:FLOW_ENTRY_INSTRUCION_APPLYACTIONS, UINT1, 1);

                    STL_FOR_LOOP(instruction.m_actions, actionIt)
                    {
                        CFlowAction& action = actionIt->second;
                        switch (action.m_type)
                        {
                            case OFPAT13_OUTPUT:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_OUTPUT, UINT4, action.m_port);
                                break;                        
                            case OFPAT13_MPLS_TTL:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_MPLSTTL, UINT1, action.m_mpls_tt);
                                break;                        
                            case OFPAT13_PUSH_VLAN:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_PUSHVLAN, UINT1, 1);
                                break;                        
                            case OFPAT13_POP_VLAN:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_POPVLAN, UINT1, 1);
                                break;                        
                            case OFPAT13_PUSH_MPLS:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_PUSHMPLS, UINT1, 1);
                                break;                        
                            case OFPAT13_POP_MPLS:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_POPMPLS, UINT1, 1);
                                break;                        
                            case OFPAT13_SET_QUEUE:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_SETQUEUE, UINT4, action.m_queue_id);
                                break;                        
                            case OFPAT13_GROUP:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_GROUP, UINT4, action.m_group_id);
                                break;                        
                            case OFPAT13_SET_NW_TTL:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_SETNWTTL, UINT1, action.m_nw_tt);
                                break;                        
                            case OFPAT13_SET_FIELD:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_SETFIELD, UINT1, 1);
                                SYNC_OXM_FIELDS(action.m_oxm_fields);
                                break;                        
                            case OFPAT13_PUSH_PBB:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_PUSHPBB, UINT1, 1);
                                break;                        
                            case OFPAT13_POP_PBB:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_POPPBB, UINT1, 1);
                                break;                        
                            case OFPAT13_EXPERIMENTER:
                                SYNC_DATA_INTEGER(FLOW_ENTRY_ACION_EXPERIMENTER, UINT4, action.m_experimenter);
                                break;                        
                            
                            default:
                                break;
                        }
                        if (stopped)
                            break;
                    }
                    break;
                }
                case OFPIT_CLEAR_ACTIONS:
                    SYNC_DATA_INTEGER(FLOW_ENTRY_INSTRUCION_CLEARACTIONS, UINT1, 1);
                    break;                        
                case OFPIT_METER:
                    SYNC_DATA_INTEGER(FLOW_ENTRY_INSTRUCION_METER, UINT4, instruction.m_meter_id);
                    break;
                case OFPIT_EXPERIMENTER:
                {
                    UINT4 experimenter[2] = {instruction.m_experimenterLen, instruction.m_experimenter};
                    SYNC_DATA_BYTES(FLOW_ENTRY_INSTRUCION_WRITEMETADATA, experimenter, sizeof(experimenter));
                    break;
                }
                
                default:
                    break;
            }
            if (stopped)
                break;
        }
    } while (FALSE);

    if (stopped)
        LOG_WARN("CClusterSync syncAddFlowEntry0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_REQ);
}

INT4 CClusterSync::recoverAddFlowEntry(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    UINT8 dpid = 0;
    gn_flow_t flow = {0};
    gn_oxm_t oxm = {0};
    gn_action_t* actions = NULL;
    BOOL match_oxm = FALSE;

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(FLOW_ENTRY_SW_DPID);
    GET_DATA_INTEGER(dpid, UINT8);

    while (len > (INT4)sizeof(tlv_t))
    {
        tlv = (tlv_t*)data;
        BREAK_IF_LENGTH_INVALID();

        switch (tlv->type)
        {
        case FLOW_ENTRY_UUID:
            GET_DATA_STRING(flow.uuid);
            break;
        case FLOW_ENTRY_CREATE_TIME:
            GET_DATA_INTEGER(flow.create_time, UINT8);
            break;
        case FLOW_ENTRY_TABLE_ID:
            GET_DATA_INTEGER(flow.table_id, UINT1);
            break;
        case FLOW_ENTRY_IDLE_TIMEOUT:
            GET_DATA_INTEGER(flow.idle_timeout, UINT2);
            break;
        case FLOW_ENTRY_HARD_TIMEOUT:
            GET_DATA_INTEGER(flow.hard_timeout, UINT2);
            break;
        case FLOW_ENTRY_PRIORITY:
            GET_DATA_INTEGER(flow.priority, UINT2);
            break;

        case FLOW_ENTRY_MATCH:
        {
            UINT1 match = 0;
            GET_DATA_INTEGER(match, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("match[%d]", match);

            memset(&oxm, 0, sizeof(gn_oxm_t));
            match_oxm = TRUE;
            break;
        }

        //oxm fields
        case FLOW_ENTRY_OXM_INPORT:
        case FLOW_ENTRY_OXM_INPHYPORT:
        case FLOW_ENTRY_OXM_METADATA:
        case FLOW_ENTRY_OXM_ETHDST:
        case FLOW_ENTRY_OXM_ETHSRC:
        case FLOW_ENTRY_OXM_ETHTYPE:
        case FLOW_ENTRY_OXM_VLANID:
        case FLOW_ENTRY_OXM_VLANPCP:
        case FLOW_ENTRY_OXM_IPDSCP:
        case FLOW_ENTRY_OXM_IPECN:
        case FLOW_ENTRY_OXM_IPPROTO:
        case FLOW_ENTRY_OXM_IPV4SRC:
        case FLOW_ENTRY_OXM_IPV4DST:
        case FLOW_ENTRY_OXM_TCPSRC:
        case FLOW_ENTRY_OXM_TCPDST:
        case FLOW_ENTRY_OXM_UDPSRC:
        case FLOW_ENTRY_OXM_UDPDST:
        case FLOW_ENTRY_OXM_ICMPV4TYPE:
        case FLOW_ENTRY_OXM_ICMPV4CODE:
        case FLOW_ENTRY_OXM_ARPOP:
        case FLOW_ENTRY_OXM_ARPSPA:
        case FLOW_ENTRY_OXM_ARPTPA:
        case FLOW_ENTRY_OXM_ARPSHA:
        case FLOW_ENTRY_OXM_ARPTHA:
        case FLOW_ENTRY_OXM_IPV6SRC:
        case FLOW_ENTRY_OXM_IPV6DST:
        case FLOW_ENTRY_OXM_MPLSLABEL:
        case FLOW_ENTRY_OXM_TUNNELID:
        case FLOW_ENTRY_OXM_IPV4SRCPREFIX:
        case FLOW_ENTRY_OXM_IPV4DSTPREFIX:
        case FLOW_ENTRY_OXM_IPV6SRCPREFIX:
        case FLOW_ENTRY_OXM_IPV6DSTPREFIX:
        case FLOW_ENTRY_OXM_METADATAMASK:
            RECOVER_OXM_FIELDS(tlv->type, oxm);
            break;
        case FLOW_ENTRY_OXM_MASK:
        {
            GET_DATA_INTEGER(oxm.mask, UINT8);
            if (match_oxm)
            {
                flow.match.type = OFPMT_OXM;
                memcpy(&(flow.match.oxm_fields), &oxm, sizeof(gn_oxm_t));
            }
            else
            {
                for (gn_action_t* action = actions; NULL != action; action = action->next)
                {
                    if (OFPAT13_SET_FIELD == action->type)
                    {
                        gn_action_set_field_t* setField = (gn_action_set_field_t*)action;
                        memcpy(&(setField->oxm_fields), &oxm, sizeof(gn_oxm_t));
                        break;
                    }
                }
            }
            memset(&oxm, 0, sizeof(gn_oxm_t));
            break;
        }

        case FLOW_ENTRY_INSTRUCTIONS:
        {
            UINT1 instruct = 0;
            GET_DATA_INTEGER(instruct, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("instruct[%d]", instruct);

            flow.instructions = NULL;
            memset(&oxm, 0, sizeof(gn_oxm_t));
            actions = NULL;
            match_oxm = FALSE;
            break;
        }

        //instructions
        case FLOW_ENTRY_INSTRUCION_GOTOTABLE:
        {
            gn_instruction_goto_table_t* instruct = (gn_instruction_goto_table_t*)bnc_malloc(sizeof(gn_instruction_goto_table_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_goto_table_t failed !");
                return BNC_ERR;
            }

            instruct->type = OFPIT_GOTO_TABLE;
            GET_DATA_INTEGER(instruct->table_id, UINT1);
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_WRITEMETADATA:
        {
            gn_instruction_write_metadata_t* instruct = (gn_instruction_write_metadata_t*)bnc_malloc(sizeof(gn_instruction_write_metadata_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_write_metadata_t failed !");
                return BNC_ERR;
            }

            UINT8 meta[2] = {0};
            GET_DATA_BYTES(meta, sizeof(meta));
            instruct->type = OFPIT_WRITE_METADATA;
            instruct->metadata = meta[0];
            instruct->metadata_mask = meta[1];
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_WRITEACTIONS:
        case FLOW_ENTRY_INSTRUCION_APPLYACTIONS:
        {
            gn_instruction_actions_t* instruct = (gn_instruction_actions_t*)bnc_malloc(sizeof(gn_instruction_actions_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_write_metadata_t failed !");
                return BNC_ERR;
            }

            UINT1 applyActions = 0;
            GET_DATA_INTEGER(applyActions, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("applyActions[%d]", applyActions);

            instruct->type = (FLOW_ENTRY_INSTRUCION_WRITEACTIONS==tlv->type)?OFPIT_WRITE_ACTIONS:OFPIT_APPLY_ACTIONS;
            instruct->actions = NULL;
            //will set actions later
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_CLEARACTIONS:
        {
            gn_instruction_t* instruct = (gn_instruction_t*)bnc_malloc(sizeof(gn_instruction_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_t failed !");
                return BNC_ERR;
            }

            UINT1 clearActions = 0;
            GET_DATA_INTEGER(clearActions, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("clearActions[%d]", clearActions);

            instruct->type = OFPIT_CLEAR_ACTIONS;
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_METER:
        {
            gn_instruction_meter_t* instruct = (gn_instruction_meter_t*)bnc_malloc(sizeof(gn_instruction_meter_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_meter_t failed !");
                return BNC_ERR;
            }

            instruct->type = OFPIT_METER;
            GET_DATA_INTEGER(instruct->meter_id, UINT4);
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }
        case FLOW_ENTRY_INSTRUCION_EXPERIMENTER:
        {
            gn_instruction_experimenter_t* instruct = (gn_instruction_experimenter_t*)bnc_malloc(sizeof(gn_instruction_experimenter_t));
            if (NULL == instruct)
            {
                LOG_ERROR("bnc_malloc gn_instruction_experimenter_t failed !");
                return BNC_ERR;
            }

            UINT4 experimenter[2] = {0};
            GET_DATA_BYTES(experimenter, sizeof(experimenter));
            instruct->type = OFPIT_EXPERIMENTER;
            instruct->len = (UINT2)experimenter[0];
            instruct->experimenter = experimenter[1];
            instruct->next = flow.instructions;
            flow.instructions = (gn_instruction_t*)instruct;
            break;
        }

        //actions
        case FLOW_ENTRY_ACION_OUTPUT:
        {
            gn_action_output_t* act = (gn_action_output_t*)bnc_malloc(sizeof(gn_action_output_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_output_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_OUTPUT;
            GET_DATA_INTEGER(act->port, UINT4);
            act->max_len = (OFPP13_CONTROLLER==act->port)?0xffff:0;
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_MPLSTTL:
        {
            gn_action_mpls_ttl_t* act = (gn_action_mpls_ttl_t*)bnc_malloc(sizeof(gn_action_mpls_ttl_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_mpls_ttl_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_MPLS_TTL;
            GET_DATA_INTEGER(act->mpls_tt, UINT1);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_PUSHVLAN:
        case FLOW_ENTRY_ACION_POPVLAN:
        case FLOW_ENTRY_ACION_PUSHMPLS:
        case FLOW_ENTRY_ACION_POPMPLS:
        case FLOW_ENTRY_ACION_PUSHPBB:
        case FLOW_ENTRY_ACION_POPPBB:
        {
            gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_t failed !");
                return BNC_ERR;
            }

            UINT1 pushPop = 0;
            GET_DATA_INTEGER(pushPop, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("pushPop[%d]", pushPop);

            act->type = (FLOW_ENTRY_ACION_PUSHVLAN==tlv->type)?OFPAT13_PUSH_VLAN:
                        (FLOW_ENTRY_ACION_POPVLAN==tlv->type)?OFPAT13_POP_VLAN:
                        (FLOW_ENTRY_ACION_PUSHMPLS==tlv->type)?OFPAT13_PUSH_MPLS:
                        (FLOW_ENTRY_ACION_POPMPLS==tlv->type)?OFPAT13_POP_MPLS:
                        (FLOW_ENTRY_ACION_PUSHPBB==tlv->type)?OFPAT13_PUSH_PBB:OFPAT13_POP_PBB;
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_SETQUEUE:
        {
            gn_action_set_queue_t* act = (gn_action_set_queue_t*)bnc_malloc(sizeof(gn_action_set_queue_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_set_queue_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_SET_QUEUE;
            GET_DATA_INTEGER(act->queue_id, UINT4);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_GROUP:
        {
            gn_action_group_t* act = (gn_action_group_t*)bnc_malloc(sizeof(gn_action_group_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_group_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_GROUP;
            GET_DATA_INTEGER(act->group_id, UINT4);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_SETNWTTL:
        {
            gn_action_nw_ttl_t* act = (gn_action_nw_ttl_t*)bnc_malloc(sizeof(gn_action_nw_ttl_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_nw_ttl_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_SET_NW_TTL;
            GET_DATA_INTEGER(act->nw_tt, UINT1);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_SETFIELD:
        {
            gn_action_set_field_t* act = (gn_action_set_field_t*)bnc_malloc(sizeof(gn_action_set_field_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_set_field_t failed !");
                return BNC_ERR;
            }

            UINT1 setField = 0;
            GET_DATA_INTEGER(setField, UINT1);
            //just to avoid compiling warnings "set but not used"
            LOG_TRACE_FMT("setField[%d]", setField);

            act->type = OFPAT13_SET_FIELD;
            memset(&(act->oxm_fields), 0, sizeof(gn_oxm_t));
            //will set oxm_fields later
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }
        case FLOW_ENTRY_ACION_EXPERIMENTER:
        {
            gn_action_experimenter_t* act = (gn_action_experimenter_t*)bnc_malloc(sizeof(gn_action_experimenter_t));
            if (NULL == act)
            {
                LOG_ERROR("bnc_malloc gn_action_experimenter_t failed !");
                return BNC_ERR;
            }

            act->type = OFPAT13_EXPERIMENTER;
            GET_DATA_INTEGER(act->experimenter, UINT4);
            act->next = actions;
            actions = (gn_action_t*)act;
            break;
        }

        default:
            break;
        }
    }

    recoverFlowEntry(dpid, flow, actions);

    return BNC_OK;
}

INT4 CClusterSync::syncDelFlowEntry(UINT8 dpid, const INT1* uuid)
{
    if (!syncNeeded())
        return BNC_ERR;

    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_REQ;
    sync.m_sockfd = -1;
    *(UINT8*)sync.m_dpidUuid = dpid;
    strncpy(sync.m_dpidUuid+8, uuid, UUID_LEN-1);
    sync.m_dpidUuid[8+UUID_LEN-1] = '\0';

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncDelFlowEntry0(UINT8 dpid, const INT1* uuid)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, FIELD_MEM_SIZE);
    m_fieldLen = 0;

    BOOL stopped = FALSE;
    tlv_t* tlv = NULL;

    do
    {
        SYNC_DATA_INTEGER(FLOW_ENTRY_SW_DPID,      UINT8, dpid);
        std::string uuidStr(uuid);
        SYNC_DATA_STRING(FLOW_ENTRY_UUID,          uuidStr);
    } while (FALSE);

    if (stopped)
        LOG_WARN("CClusterSync syncDelFlowEntry0 stopped !");

    return sendReq(CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_REQ);
}

INT4 CClusterSync::recoverDelFlowEntry(const INT1* data, INT4 len)
{
    if ((NULL == data) || (len < (INT4)sizeof(tlv_t)))
        return BNC_ERR;

    UINT8 dpid = 0;
    INT1 uuid[UUID_LEN] = {0};

    tlv_t* tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(FLOW_ENTRY_SW_DPID);
    GET_DATA_INTEGER(dpid, UINT8);

    tlv = (tlv_t*)data;
    RETURN_IF_LENGTH_INVALID();
    RETURN_IF_TYPE_INVALID(FLOW_ENTRY_UUID);
    GET_DATA_STRING(uuid);

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
    if (sw.isNotNull())
    {
        gn_flow_t flow = {0};
        strncpy(flow.uuid, uuid, UUID_LEN-1);
        flow.uuid[UUID_LEN-1] = '\0';
        CFlowTableEventReportor::getInstance()->report(EVENT_TYPE_FLOW_TABLE_DEL_STRICT, EVENT_REASON_FLOW_TABLE_RECOVER, sw, flow);
    }
    
    return BNC_OK;
}

INT4 CClusterSync::processSyncHostListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_HOST_LIST_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover host list with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverHostList(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_HOST_LIST_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_HOST_LIST_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncHostListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_HOST_LIST_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync host list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncNatIcmpListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_NAT_ICMP_LIST_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover nat icmp list with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverNatIcmpList(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_NAT_ICMP_LIST_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_NAT_ICMP_LIST_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncNatIcmpListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_NAT_ICMP_LIST_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync nat icmp list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncNatHostListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_NAT_HOST_LIST_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover nat host list with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverNatHostList(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_NAT_HOST_LIST_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_NAT_HOST_LIST_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncNatHostListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_NAT_HOST_LIST_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync nat host list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncAddNatHostReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_ADD_NAT_HOST_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover add nat host with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverAddNatHost(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_ADD_NAT_HOST_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_ADD_NAT_HOST_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncAddNatHostRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_ADD_NAT_HOST_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync add nat host cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncDelNatHostReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_DEL_NAT_HOST_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover del nat host with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverDelNatHost(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_DEL_NAT_HOST_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_DEL_NAT_HOST_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncDelNatHostRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_DEL_NAT_HOST_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync del nat host cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncTopoLinkListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_TOPO_LINK_LIST_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover topo link list with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverTopoLinkList(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_TOPO_LINK_LIST_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_TOPO_LINK_LIST_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncTopoLinkListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_TOPO_LINK_LIST_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync topo link list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncTopoLinkReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_TOPO_LINK_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover topo link with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverTopoLink(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_TOPO_LINK_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_TOPO_LINK_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncTopoLinkRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_TOPO_LINK_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync topo link cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncSwitchTagListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_SW_TAG_LIST_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover switch tag list with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverSwitchTagList(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_SW_TAG_LIST_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_SW_TAG_LIST_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncSwitchTagListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_SW_TAG_LIST_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync switch tag list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncSwitchTagReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_SW_TAG_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover switch tag with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverSwitchTag(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_SW_TAG_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_SW_TAG_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncSwitchTagRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_SW_TAG_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync switch tag cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncTagInfoReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_TAG_INFO_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover tag info with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverTagInfo(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_TAG_INFO_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_TAG_INFO_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncTagInfoRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_TAG_INFO_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync tag info cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncFlowEntryListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover flow entry list with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverFlowEntryList(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_FLOW_ENTRY_LIST_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncFlowEntryListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync flow entry list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncAddFlowEntryReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover add flow entry with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverAddFlowEntry(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_ADD_FLOW_ENTRY_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncAddFlowEntryRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync add flow entry cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncDelFlowEntryReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    RETURN_IF_REQUEST_INVALID(itf, CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_REQ);

    LOG_WARN_FMT("CLUSTER SLAVE recover del flow entry with len[%d]", req->len);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverDelFlowEntry(req->data, req->len);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_DEL_FLOW_ENTRY_FAIL;

    return sendRsp(CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_RSP, cause, sockfd);
}

INT4 CClusterSync::processSyncDelFlowEntryRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    RETURN_IF_RESPONSE_INVALID(itf, CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_RSP);

    LOG_DEBUG_FMT("CLUSTER MASTER receive sync del flow entry cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}


#if 0
INT4 CClusterSync::syncSwitchList(INT4 sockfd)
{
    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_SW_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncSwitchList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
    m_fieldNum = 0;

    UINT8 pre_dpid = 0;
    p_fabric_sw_node sw_node = g_fabric_sw_list.node_list;
    while ((NULL != sw_node) && (NULL != sw_node->sw))
    {
        //save dpid
		m_fieldMem[m_fieldNum].type = SW_LIST_OWN_DPID;
		sprintf(m_fieldMem[m_fieldNum].pad, "%llu", sw_node->sw->dpid);
		m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
		m_fieldNum++;
		
        //save tag
		m_fieldMem[m_fieldNum].type = SW_LIST_OWN_TAG;
		sprintf(m_fieldMem[m_fieldNum].pad, "%u", sw_node->tag);
		m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
		m_fieldNum++;
        
        //save pre dpid
		m_fieldMem[m_fieldNum].type = SW_LIST_OWN_PREDPID;
		sprintf(m_fieldMem[m_fieldNum].pad, "%llu", pre_dpid);
		m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
		m_fieldNum++;

        UINT4 index = 0;
        neighbor_t* neighbor_sw = NULL;

        for (index = 0; index < sw_node->sw->n_ports; index++)
        {
            neighbor_sw = sw_node->sw->neighbor[index];
            if (neighbor_sw && neighbor_sw->bValid && (NULL != neighbor_sw->sw) && (NULL != neighbor_sw->port))
            {
                //save own port_no
                m_fieldMem[m_fieldNum].type = SW_LIST_OWN_PORT;
        		sprintf(m_fieldMem[m_fieldNum].pad, "%u", sw_node->sw->ports[index].port_no);
        		m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
        		m_fieldNum++;
				
                //save neighbor dpid
                m_fieldMem[m_fieldNum].type = SW_LIST_NEIGHBOR_DPID;
        		sprintf(m_fieldMem[m_fieldNum].pad, "%llu", neighbor_sw->sw->dpid);
        		m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
        		m_fieldNum++;
				
                //save neighbor port_no
                m_fieldMem[m_fieldNum].type = SW_LIST_NEIGHBOR_PORT;
        		sprintf(m_fieldMem[m_fieldNum].pad, "%u", neighbor_sw->port->port_no);
        		m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
        		m_fieldNum++;
            }
        }
		
        pre_dpid = sw_node->sw->dpid;
        sw_node = sw_node->next;       
    }

    return sendReq(CLUSTER_OPER_SYNC_SW_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverSwitchList(INT4 num, const field_pad_t* field_pad_p)
{
    INT4 index = 0;
    UINT8 own_dpid = 0;
    UINT4 own_tag = 0;
    UINT8 own_pre_dpid = 0;
    UINT4 own_port_no = 0;
    UINT8 neighbor_dpid = 0;
    UINT4 neighbor_port_no = 0;
	gn_switch_t *sw = NULL;
    p_fabric_sw_node node = NULL;

    while (index < num)
    {
        if (SW_LIST_OWN_DPID != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        own_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
        index++;

        if (SW_LIST_OWN_TAG != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        own_tag = atoll(field_pad_p[index].pad);
        index++;

        if (SW_LIST_OWN_PREDPID != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        own_pre_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
        index++;

		get_sw_from_dpid(own_dpid, &sw);
		if (sw && sw->sw_ip)
		{
        	sw->vlan_tag = own_tag;
		}
		
        node = get_fabric_sw_node_by_dpid(own_dpid);
        if (NULL == node)
        {
            continue;
        }
        node->tag = own_tag;

        if (0 == adjust_fabric_sw_node_list(own_pre_dpid, own_dpid))
        {
            continue;
        }

        while (SW_LIST_OWN_PORT == field_pad_p[index].type && index < num)
        {
            own_port_no = atoll(field_pad_p[index].pad);
            index++;

            if (SW_LIST_NEIGHBOR_DPID != field_pad_p[index].type)
            {
                index++;
                break;
            }
            neighbor_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
            index++;

            if (SW_LIST_NEIGHBOR_PORT != field_pad_p[index].type)
            {
                index++;
                break;
            }
            neighbor_port_no = atoll(field_pad_p[index].pad);
            index++;

            mapping_new_neighbor(node->sw, own_port_no, neighbor_dpid, neighbor_port_no);
		}
    }

    return BNC_OK;
}

INT4 CClusterSync::syncOpenstackExternalList(INT4 sockfd)
{
    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_OPENSTACK_EXT_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncOpenstackExternalList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
    m_fieldNum = 0;

	openstack_external_node_p node = g_openstack_external_list;
	while (NULL != node)
	{
	    external_port_p ext = (external_port_p)node->data;
        if (NULL != ext)
        {
            //save network id
            m_fieldMem[m_fieldNum].type = EXTERNAL_NETWORK_ID;
            sprintf(m_fieldMem[m_fieldNum].pad, "%s", ext->network_id);
            m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
            m_fieldNum++;

			m_fieldMem[m_fieldNum].type = EXTERNAL_SUBNETWORK_ID;
            sprintf(m_fieldMem[m_fieldNum].pad, "%s", ext->subnet_id);
            m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
            m_fieldNum++;
            
            //save ip
            m_fieldMem[m_fieldNum].type = EXTERNAL_GATEWAY_IP;
            sprintf(m_fieldMem[m_fieldNum].pad, "%u", ext->external_gateway_ip);
            m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
            m_fieldNum++;
            
            //save outer ip
            m_fieldMem[m_fieldNum].type = EXTERNAL_OUTER_INTERFACE_IP;
            sprintf(m_fieldMem[m_fieldNum].pad, "%u", ext->external_outer_interface_ip);
            m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
            m_fieldNum++;
            
            //save mac
            m_fieldMem[m_fieldNum].type = EXTERNAL_GATEWAY_MAC;
            mac2str(ext->external_gateway_mac, m_fieldMem[m_fieldNum].pad);
            m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
            m_fieldNum++;
            
            //save outer mac
            m_fieldMem[m_fieldNum].type = EXTERNAL_OUTER_INTERFACE_MAC;
            mac2str(ext->external_outer_interface_mac, m_fieldMem[m_fieldNum].pad);
            m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
            m_fieldNum++;
            
            //save dpid
            m_fieldMem[m_fieldNum].type = EXTERNAL_DPID;
            sprintf(m_fieldMem[m_fieldNum].pad, "%llu", ext->external_dpid);
            m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
            m_fieldNum++;
            
            //save port
            m_fieldMem[m_fieldNum].type = EXTERNAL_PORT;
            sprintf(m_fieldMem[m_fieldNum].pad, "%u", ext->external_port);
            m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
            m_fieldNum++;
        }
        
        node = node->next;
	}

    return sendReq(CLUSTER_OPER_SYNC_OPENSTACK_EXT_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverOpenstackExternalList(INT4 num, const field_pad_t* field_pad_p)
{
    INT4 index = 0;
    INT1 network_id[48] = {0};
	INT1 subnet_id[48] = {0};
	UINT4 external_gateway_ip = 0;
	UINT4 external_outer_interface_ip = 0;
	UINT1 external_gateway_mac[6] = {0};
	UINT1 external_outer_interface_mac[6] = {0};
	UINT8 external_dpid = 0;
	UINT4 external_port = 0;

	if (!g_openstack_on)
		return BNC_OK;

    while (index < num)
    {
        if (EXTERNAL_NETWORK_ID!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        strncpy(network_id, field_pad_p[index].pad, 48);
        index++;

		if (EXTERNAL_SUBNETWORK_ID!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        strncpy(subnet_id, field_pad_p[index].pad, 48);
        index++;
        
        if (EXTERNAL_GATEWAY_IP!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        external_gateway_ip = atoll(field_pad_p[index].pad);
        index++;
        
        if (EXTERNAL_OUTER_INTERFACE_IP!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        external_outer_interface_ip = atoll(field_pad_p[index].pad);
        index++;
        
        if (EXTERNAL_GATEWAY_MAC!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        macstr2hex((INT1*)field_pad_p[index].pad, external_gateway_mac);
        index++;
        
        if (EXTERNAL_OUTER_INTERFACE_MAC!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        macstr2hex((INT1*)field_pad_p[index].pad, external_outer_interface_mac);
        index++;
        
        if (EXTERNAL_DPID!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        external_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
        index++;
        
        if (EXTERNAL_PORT!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        external_port = atoll(field_pad_p[index].pad);
        index++;

        //add to list
        create_external_port_by_rest(
            external_gateway_ip,
            external_gateway_mac,
            external_outer_interface_ip,
            external_outer_interface_mac,
            external_dpid,
            external_port,
            network_id,
            subnet_id);
    }

    return BNC_OK;
}

INT4 CClusterSync::syncOpenstackLbaasMembersList(INT4 sockfd)
{
    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_OPENSTACK_LBAAS_MEMBERS_LIST_REQ;
    sync.m_sockfd = sockfd;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncOpenstackLbaasMembersList0(INT4 sockfd)
{
    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
    m_fieldNum = 0;

    openstack_lbaas_node_p node = g_openstack_lbaas_members_list;
    openstack_lbaas_members_p mem = NULL;
    openstack_lbaas_node_p connect_node = NULL;
    openstack_lbaas_connect_p connect_ips = NULL;

	while (NULL != node)
	{
	    mem  = (openstack_lbaas_members_p)node->data;
        if (NULL != mem)
        {
            connect_node = mem->connect_ips;
            while (NULL != connect_node && NULL != connect_node->data)
            {
                connect_ips = (openstack_lbaas_connect_p)connect_node->data;

                //save ext_ip
                m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_EXT_ID;
                sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->ext_ip);
                m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
                m_fieldNum++;

                //save inside_ip
                m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_INSIDE_IP;
                sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->inside_ip);
                m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
                m_fieldNum++;

                //save vip
                m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_VIP;
                sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->vip);
                m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
                m_fieldNum++;
                
                //save src_port_no
                m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_SRC_PORTNO;
                sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->src_port_no);
                m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
                m_fieldNum++;

                //save ext_port_no
                m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_EXT_PORTNO;
                sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->ext_port_no);
                m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
                m_fieldNum++;
                
                connect_node = connect_node->next;
            }
        }

        node = node->next;
    }

    return sendReq(CLUSTER_OPER_SYNC_OPENSTACK_LBAAS_MEMBERS_LIST_REQ, sockfd);
}

INT4 CClusterSync::recoverOpenstackLbaasMembersList(INT4 num, const field_pad_t* field_pad_p)
{
    INT4 index = 0;
	UINT4 ext_ip = 0;
	UINT4 inside_ip = 0;
	UINT4 vip = 0;
	UINT4 src_port_no = 0;
	UINT4 ext_port_no = 0;

	if (!g_openstack_on)
		return BNC_OK;

    while (index < num)
    {
        if (LBAAS_MEMBERS_EXT_ID != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        ext_ip = atoll(field_pad_p[index].pad);
        index++;
        
        if (LBAAS_MEMBERS_INSIDE_IP != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        inside_ip = atoll(field_pad_p[index].pad);
        index++;

        if (LBAAS_MEMBERS_VIP != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        vip = atoll(field_pad_p[index].pad);
        index++;

        if (LBAAS_MEMBERS_SRC_PORTNO != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        src_port_no = atoll(field_pad_p[index].pad);
        index++;
        
        if (LBAAS_MEMBERS_EXT_PORTNO != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        ext_port_no = atoll(field_pad_p[index].pad);
        index++;

        //add to list
        create_openstack_lbaas_connect(ext_ip, inside_ip, vip, src_port_no);
    }

    return BNC_OK;
}

INT4 CClusterSync::syncAddOpenstackLbaasMembers(openstack_lbaas_connect_p connect_ips)
{
    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_ADD_OPENSTACK_LBAAS_MEMBERS_REQ;
    sync.m_sockfd = -1;
    sync.u.lbaas_con = *connect_ips;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncAddOpenstackLbaasMembers0(openstack_lbaas_connect_p connect_ips)
{
    //LOG_PROC("INFO", "CLUSTER MASTER sync add openstack lbaas members.");

    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
    m_fieldNum = 0;

    //save ext_ip
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_EXT_ID;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->ext_ip);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;

    //save inside_ip
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_INSIDE_IP;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->inside_ip);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;

    //save vip
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_VIP;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->vip);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;
    
    //save src_port_no
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_SRC_PORTNO;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->src_port_no);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;

    //save ext_port_no
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_EXT_PORTNO;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->ext_port_no);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;
                
	return cluster_broadcast_sync_req(CLUSTER_OPER_SYNC_ADD_OPENSTACK_LBAAS_MEMBERS_REQ);
}

INT4 CClusterSync::recoverAddOpenstackLbaasMembers(INT4 num, const field_pad_t* field_pad_p)
{
    return recoverOpenstackLbaasMembersList(num, field_pad_p);
}

INT4 CClusterSync::syncDelOpenstackLbaasMembers(openstack_lbaas_connect_p connect_ips)
{
    SyncData sync;
    sync.m_operation = CLUSTER_OPER_SYNC_DEL_OPENSTACK_LBAAS_MEMBERS_REQ;
    sync.m_sockfd = -1;
    sync.u.lbaas_con = *connect_ips;

    m_queue.push(sync);
    m_sem.post();

    return BNC_OK;
}

INT4 CClusterSync::syncDelOpenstackLbaasMembers0(openstack_lbaas_connect_p connect_ips)
{
    //LOG_PROC("INFO", "CLUSTER master sync del openstack lbaas members.");

    if (NULL == m_fieldMem)
        return BNC_ERR;

    //memset((void*)m_fieldMem, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
    m_fieldNum = 0;

    //save ext_ip
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_EXT_ID;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->ext_ip);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;

    //save inside_ip
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_INSIDE_IP;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->inside_ip);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;

    //save vip
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_VIP;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->vip);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;
    
    //save src_port_no
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_SRC_PORTNO;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->src_port_no);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;

    //save ext_port_no
    m_fieldMem[m_fieldNum].type = LBAAS_MEMBERS_EXT_PORTNO;
    sprintf(m_fieldMem[m_fieldNum].pad, "%u", connect_ips->ext_port_no);
    m_fieldMem[m_fieldNum].len = strlen(m_fieldMem[m_fieldNum].pad);
    m_fieldNum++;
                
	return cluster_broadcast_sync_req(CLUSTER_OPER_SYNC_DEL_OPENSTACK_LBAAS_MEMBERS_REQ);
}

INT4 CClusterSync::recoverDelOpenstackLbaasMembers(INT4 num, const field_pad_t* field_pad_p)
{
    INT4 index = 0;
	UINT4 ext_ip = 0;
	UINT4 inside_ip = 0;
	UINT4 vip = 0;
	UINT4 src_port_no = 0;
	UINT4 ext_port_no = 0;

	if (!g_openstack_on)
		return BNC_OK;

    while (index < num)
    {
        if (LBAAS_MEMBERS_EXT_ID != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        ext_ip = atoll(field_pad_p[index].pad);
        index++;
        
        if (LBAAS_MEMBERS_INSIDE_IP != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        inside_ip = atoll(field_pad_p[index].pad);
        index++;

        if (LBAAS_MEMBERS_VIP != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        vip = atoll(field_pad_p[index].pad);
        index++;

        if (LBAAS_MEMBERS_SRC_PORTNO != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        src_port_no = atoll(field_pad_p[index].pad);
        index++;
        
        if (LBAAS_MEMBERS_EXT_PORTNO != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        ext_port_no = atoll(field_pad_p[index].pad);
        index++;

        remove_openstack_lbaas_connect(ext_ip, inside_ip, vip, src_port_no);
    }

    return BNC_OK;
}

INT4 CClusterSync::processSyncSwitchListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_SW_LIST_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover switch list with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverSwitchList(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_SW_LIST_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_SW_LIST_RSP, cause);
}

INT4 CClusterSync::processSyncSwitchListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_SW_LIST_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync switch list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncHostListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_HOST_LIST_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover host list with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverHostList(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_HOST_LIST_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_HOST_LIST_RSP, cause);
}

INT4 CClusterSync::processSyncHostListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_HOST_LIST_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync host list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncOpenstackExternalListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_OPENSTACK_EXT_LIST_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover openstack external list with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverOpenstackExternalList(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_OPENSTACK_EXT_LIST_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_OPENSTACK_EXT_LIST_RSP, cause);
}

INT4 CClusterSync::processSyncOpenstackExternalListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_OPENSTACK_EXT_LIST_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync openstack external list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncNatIcmpIdListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_NAT_ICMP_ID_LIST_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover nat icmp identifier list with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverNatIcmpIdList(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_NAT_ICMP_ID_LIST_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_NAT_ICMP_ID_LIST_RSP, cause);
}

INT4 CClusterSync::processSyncNatIcmpIdListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_NAT_ICMP_ID_LIST_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync nat icmp identifier list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncNatHostListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_NAT_HOST_LIST_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover nat host list with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverNatHostList(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_NAT_HOST_LIST_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_NAT_HOST_LIST_RSP, cause);
}

INT4 CClusterSync::processSyncNatHostListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_NAT_HOST_LIST_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync nat host list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncAddNatHostReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_ADD_NAT_HOST_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover add nat host with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverAddNatHost(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_ADD_NAT_HOST_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_ADD_NAT_HOST_RSP, cause);
}

INT4 CClusterSync::processSyncAddNatHostRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_ADD_NAT_HOST_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync add nat host cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncDelNatHostReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_DEL_NAT_HOST_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover del nat host with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = cluster_recover_del_nat_host(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_DEL_NAT_HOST_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_DEL_NAT_HOST_RSP, cause);
}

INT4 CClusterSync::processSyncDelNatHostRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_DEL_NAT_HOST_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync del nat host cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncOpenstackLbaasMembersListReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_OPENSTACK_LBAAS_MEMBERS_LIST_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover openstack lbaas members list with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = recoverOpenstackLbaasMembersList(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_OPENSTACK_LBAAS_MEMBERS_LIST_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_OPENSTACK_LBAAS_MEMBERS_LIST_RSP, cause);
}

INT4 CClusterSync::processSyncOpenstackLbaasMembersListRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_OPENSTACK_LBAAS_MEMBERS_LIST_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync openstack lbaas members list cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncAddOpenstackLbaasMembersReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_ADD_OPENSTACK_LBAAS_MEMBERS_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover add openstack lbaas members with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = cluster_recover_add_openstack_lbaas_members(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_ADD_OPENSTACK_LBAAS_MEMBERS_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_ADD_OPENSTACK_LBAAS_MEMBERS_RSP, cause);
}

INT4 CClusterSync::processSyncAddOpenstackLbaasMembersRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_ADD_OPENSTACK_LBAAS_MEMBERS_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync add openstack lbaas members cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}

INT4 CClusterSync::processSyncDelOpenstackLbaasMembersReq(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_req_t* req = (cluster_sync_req_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_DEL_OPENSTACK_LBAAS_MEMBERS_REQ) || (itf->length < sizeof(cluster_sync_req_t)))
    {
        LOG_PROC("WARNING", "invalid sync req: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER slave recover del openstack lbaas members with field num[%d]", req->num);

    INT4 cause = CLUSTER_CAUSE_SUCC;
    INT4 ret = cluster_recover_del_openstack_lbaas_members(req->num, (field_pad_t*)req->data);
    if (BNC_OK != ret)
        cause = CLUSTER_CAUSE_RECOVER_DEL_OPENSTACK_LBAAS_MEMBERS_FAIL;

    return sendRsp(sockfd, CLUSTER_OPER_SYNC_DEL_OPENSTACK_LBAAS_MEMBERS_RSP, cause);
}

INT4 CClusterSync::processSyncDelOpenstackLbaasMembersRsp(INT4 sockfd, cluster_interface_t* itf)
{
    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)itf->data;
    if ((itf->operation != CLUSTER_OPER_SYNC_DEL_OPENSTACK_LBAAS_MEMBERS_RSP) || (itf->length < sizeof(cluster_sync_rsp_t)))
    {
        LOG_PROC("WARNING", "invalid sync rsp: operation[0x%02x], length[%d]", itf->operation, itf->length);
        return BNC_ERR;
    }

    LOG_PROC("INFO", "CLUSTER master receive sync del openstack lbaas members cause[0x%02x] from sockfd[%d]", rsp->cause, sockfd);
    return BNC_OK;
}
#endif

INT4 CClusterSync::sendReq(INT4 operation, INT4 sockfd)
{
    if ((NULL == m_fieldMem) || (m_fieldLen <= 0))
        return BNC_ERR;

    INT4 length = sizeof(cluster_interface_t) + 
                  sizeof(cluster_sync_req_t) +
                  m_fieldLen;
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = operation;
    itf->length = length - sizeof(cluster_interface_t);

    cluster_sync_req_t* req = (cluster_sync_req_t*)ptr;
    ptr += sizeof(cluster_sync_req_t);
    req->len = m_fieldLen;
    memcpy(ptr, m_fieldMem, m_fieldLen);

    if (sockfd > 0)
    {
        LOG_DEBUG_FMT("CLUSTER master sync operation[0x%02x] length[%d] to slave sockfd[%d]", 
            operation, length, sockfd);
        return sendMsgOut(sockfd, buffer, length);
    }
    else
    {
        std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
        STL_FOR_LOOP(controllers, it)
        {
            cluster_controller_t& controller = *it;
            if ((CServer::getInstance()->getControllerIp() != controller.ip) &&
                ((CLUSTER_CONTROLLER_CONNECTED == controller.state) ||
                 (CLUSTER_CONTROLLER_NORESPONSE == controller.state)))
            {
                LOG_DEBUG_FMT("CLUSTER master sync operation[0x%02x] length[%d] to slave[%s]sockfd[%d]", 
                    operation, length, inet_htoa(controller.ip), controller.sockfd);
                sendMsgOut(controller.sockfd, buffer, length);
            }
        }

        return BNC_OK;
    }
}

INT4 CClusterSync::sendRsp(INT4 operation, INT4 cause, INT4 sockfd)
{
    LOG_INFO_FMT("CLUSTER slave send operation[0x%02x] cause[0x%02x] to master[%s]sockfd[%d]", 
        operation, cause, inet_htoa(CClusterElection::getMasterIp()), sockfd);

    INT4 length = sizeof(cluster_interface_t) + sizeof(cluster_sync_rsp_t);
    INT1 buffer[length], *ptr = buffer;

    cluster_interface_t* itf = (cluster_interface_t*)ptr;
    ptr += sizeof(cluster_interface_t);
    itf->operation = operation;
    itf->length = length - sizeof(cluster_interface_t);

    cluster_sync_rsp_t* rsp = (cluster_sync_rsp_t*)ptr;
    rsp->cause = cause;

    return sendMsgOut(sockfd, buffer, length)?BNC_OK:BNC_ERR;
}

BOOL CClusterSync::syncNeeded()
{
    return (CClusterService::getInstance()->isClusterOn() &&
            (OFPCR_ROLE_MASTER == CClusterService::getInstance()->getControllerRole()));
}

BOOL CClusterSync::recoverNeeded()
{
    return (CClusterService::getInstance()->isClusterOn() &&
            (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole()));
}


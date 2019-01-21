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
*   File Name   : COfFlowRemovedHandler.cpp                                   *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "COfFlowRemovedHandler.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "COfMsgUtil.h"
#include "CControl.h"
#include "CRecvWorker.h"
#include "CServer.h"
#include "log.h"
#include "bnc-error.h"
#include "COfConnectMgr.h"
#include "CSNatConnMgr.h"
#include "CFlowDefine.h"
#include "CFlowTableEventReportor.h"

COfFlowRemovedHandler::COfFlowRemovedHandler()
{
}

COfFlowRemovedHandler::~COfFlowRemovedHandler()
{
}

INT4 COfFlowRemovedHandler::onregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_FLOW_REMOVED);
    return CMsgHandler::onregister(path, 1);
}

void COfFlowRemovedHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_FLOW_REMOVED);
    CMsgHandler::deregister(path);
}

INT4 COfFlowRemovedHandler::handle(CSmartPtr<CMsgCommon> msg)
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

    struct ofp13_flow_removed *removed = (struct ofp13_flow_removed *)data;
    if (ntohs(removed->header.length) < sizeof(struct ofp13_flow_removed))
    {
        LOG_WARN_FMT("%s[%p] received invalid flow removed msg from sockfd[%d] !", 
            toString(), this, sockfd);
        return BNC_ERR;
    }

    gn_flow_t flow = {0};
    of13Parse(removed, flow);

    if ((OFPRR_IDLE_TIMEOUT == removed->reason) || (OFPRR_HARD_TIMEOUT == removed->reason))
    {
        if (FABRIC_TABLE_NAT_OUTTOR == flow.table_id)
        {
            gn_oxm_t& oxm = flow.match.oxm_fields;
            if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_PROTO) &&
                IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_SRC) &&
                IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_DST))
            {
                if ((IPPROTO_TCP == oxm.ip_proto) && IS_MASK_SET(oxm.mask, OFPXMT_OFB_TCP_SRC))
                {
                    CSNatConnMgr::getInstance()->removeSNatConnByInt(htonl(oxm.ipv4_dst), htonl(oxm.ipv4_src), oxm.tcp_src, IPPROTO_TCP);
                }
                if ((IPPROTO_UDP == oxm.ip_proto) && IS_MASK_SET(oxm.mask, OFPXMT_OFB_UDP_SRC))
                {
                    CSNatConnMgr::getInstance()->removeSNatConnByInt(htonl(oxm.ipv4_dst), htonl(oxm.ipv4_src), oxm.udp_src, IPPROTO_UDP);
                }
            }

        }

        CFlowTableEventReportor::getInstance()->report(EVENT_TYPE_FLOW_TABLE_DEL_STRICT, EVENT_REASON_FLOW_TABLE_DEL_STRICT, sw, flow);
    }

    return BNC_OK;
}

void COfFlowRemovedHandler::of13Parse(const struct ofp13_flow_removed* removed, gn_flow_t& flow)
{
    flow.table_id = removed->table_id;
    flow.idle_timeout = removed->idle_timeout;
    flow.hard_timeout = removed->hard_timeout;
	flow.priority = ntohs(removed->priority);
    flow.match.type = ntohs(removed->match.type);
    memset(&flow.match.oxm_fields, 0, sizeof(gn_oxm_t));

    const UINT1 *oxm = removed->match.oxm_fields;
    UINT1 fieldLen = 0;
    UINT2 oxmLen = 4;
    UINT2 totalLen = ntohs(removed->match.length);

    while (oxmLen < totalLen)
    {
        fieldLen = of13OxmConvertter(oxm, flow.match.oxm_fields);
        oxm += sizeof(struct ofp_oxm_header) + fieldLen;
        oxmLen += sizeof(struct ofp_oxm_header) + fieldLen;
    }

    //TBD: flow.uuid
#if 0
    LOG_WARN_FMT("$$$ uuid: %s", flow.uuid);
    LOG_WARN_FMT("$$$ table_id: %u", flow.table_id);
    LOG_WARN_FMT("$$$ priority: %u", flow.priority);
    LOG_WARN_FMT("$$$ match:");
    LOG_WARN_FMT("$$$     type: %u", flow.match.type);
    if IS_MASK_SET(flow.match.oxm_fields.mask, OFPXMT_OFB_IN_PORT)
        LOG_WARN_FMT("$$$     in port: %u", flow.match.oxm_fields.in_port);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_ETH_SRC)
    {
        INT1 str[48] = {0};
        mac2str(flow.match.oxm_fields.eth_src, str);
        LOG_WARN_FMT("$$$     eth src: %s", str);
    }
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_ETH_DST)
    {
        INT1 str[48] = {0};
        mac2str(flow.match.oxm_fields.eth_dst, str);
        LOG_WARN_FMT("$$$     eth dst: %s", str);
    }
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_ETH_TYPE)
        LOG_WARN_FMT("$$$     eth type: 0x%04x", flow.match.oxm_fields.eth_type);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_VLAN_VID)
        LOG_WARN_FMT("$$$     vlan id: %u", flow.match.oxm_fields.vlan_vid);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_IPV4_SRC)
        LOG_WARN_FMT("$$$     ipv4 src: %s", inet_htoa(flow.match.oxm_fields.ipv4_src));
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_IPV4_DST)
        LOG_WARN_FMT("$$$     ipv4 dst: %s", inet_htoa(flow.match.oxm_fields.ipv4_dst));
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_IP_PROTO)
        LOG_WARN_FMT("$$$     ip proto: %u", flow.match.oxm_fields.ip_proto);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_TCP_SRC)
        LOG_WARN_FMT("$$$     tcp src: %u", flow.match.oxm_fields.tcp_src);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_TCP_DST)
        LOG_WARN_FMT("$$$     tcp dst: %u", flow.match.oxm_fields.tcp_dst);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_UDP_SRC)
        LOG_WARN_FMT("$$$     udp src: %u", flow.match.oxm_fields.udp_src);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_UDP_DST)
        LOG_WARN_FMT("$$$     udp dst: %u", flow.match.oxm_fields.udp_dst);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_ICMPV4_TYPE)
        LOG_WARN_FMT("$$$     icmpv4 type: %u", flow.match.oxm_fields.icmpv4_type);
    if (flow.match.oxm_fields.mask, OFPXMT_OFB_ICMPV4_CODE)
        LOG_WARN_FMT("$$$     icmpv4 code: %u", flow.match.oxm_fields.icmpv4_code);
#endif
}

UINT1 COfFlowRemovedHandler::of13OxmConvertter(const UINT1* oxmInput, gn_oxm_t& oxmOutput)
{
    struct ofp_oxm_header *oxm = (struct ofp_oxm_header *)oxmInput;
    switch (oxm->oxm_field_hm >> 1)
    {
        case (OFPXMT_OFB_IN_PORT):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IN_PORT);
            oxmOutput.in_port = ntohl(*(UINT4 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_IN_PHY_PORT):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IN_PHY_PORT);
            oxmOutput.in_phy_port = ntohl(*(UINT4 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_METADATA):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_METADATA);
            oxmOutput.metadata = ntohll(*(UINT8 *)(oxm->data));
            if (oxm->length > 8)
            {
                SET_MASK(oxmOutput.mask, OFPXMT_OFB_METADATA_MASK);
                oxmOutput.metadata_mask = ntohll(*(UINT8 *)(oxm->data + 8));
            }
            break;
        }
        case (OFPXMT_OFB_ETH_DST):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ETH_DST);
            memcpy(oxmOutput.eth_dst, oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ETH_SRC):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ETH_SRC);
            memcpy(oxmOutput.eth_src, oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ETH_TYPE):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ETH_TYPE);
            oxmOutput.eth_type = ntohs(*(UINT2 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_VLAN_VID):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_VLAN_VID);
            oxmOutput.vlan_vid = ntohs(*(UINT2 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_VLAN_PCP):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_VLAN_PCP);
            oxmOutput.vlan_pcp = (*(UINT1 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_IP_DSCP):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IP_DSCP);
            oxmOutput.ip_dscp = *(UINT1 *)(oxm->data);
            break;
        }
        case (OFPXMT_OFB_IP_ECN):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IP_ECN);
            oxmOutput.ip_ecn = *(UINT1 *)(oxm->data);
            break;
        }
        case (OFPXMT_OFB_IP_PROTO):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IP_PROTO);
            oxmOutput.ip_proto = *(UINT1 *)(oxm->data);
            break;
        }
        case (OFPXMT_OFB_IPV4_SRC):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IPV4_SRC);
            oxmOutput.ipv4_src = ntohl(*(UINT4 *)(oxm->data));
    		break;
        }
		case (OFPXMT_OFB_IPV4_SRC_PREFIX):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IPV4_SRC_PREFIX);
            oxmOutput.ipv4_src_prefix = ntohl(*(UINT4 *)(oxm->data));
    		break;
        }
        case (OFPXMT_OFB_IPV4_DST):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IPV4_DST);
            oxmOutput.ipv4_dst = ntohl(*(UINT4 *)(oxm->data));
    		break;
        }
		case (OFPXMT_OFB_IPV4_DST_PREFIX):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IPV4_DST_PREFIX);
            oxmOutput.ipv4_dst_prefix = ntohl(*(UINT4 *)(oxm->data));
    		break;
        }
        case (OFPXMT_OFB_TCP_SRC):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_TCP_SRC);
            oxmOutput.tcp_src = ntohs(*(UINT2 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_TCP_DST):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_TCP_DST);
            oxmOutput.tcp_dst = ntohs(*(UINT2 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_UDP_SRC):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_UDP_SRC);
            oxmOutput.udp_src = ntohs(*(UINT2 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_UDP_DST):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_UDP_DST);
            oxmOutput.udp_dst = ntohs(*(UINT2 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_SCTP_SRC):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_SCTP_DST):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_ICMPV4_TYPE):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ICMPV4_TYPE);
            oxmOutput.icmpv4_type = *(UINT1 *)(oxm->data);
            break;
        }
        case (OFPXMT_OFB_ICMPV4_CODE):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ICMPV4_CODE);
            oxmOutput.icmpv4_code = *(UINT1 *)(oxm->data);
            break;
        }
        case (OFPXMT_OFB_ARP_OP):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ARP_OP);
            oxmOutput.arp_op = *(UINT1 *)(oxm->data);
            break;
        }
        case (OFPXMT_OFB_ARP_SPA):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ARP_SPA);
            oxmOutput.arp_spa = ntohl(*(UINT4 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_ARP_TPA):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ARP_TPA);
            oxmOutput.arp_tpa = ntohl(*(UINT4 *)(oxm->data));
            break;
        }
        case (OFPXMT_OFB_ARP_SHA):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ARP_SHA);
            memcpy(oxmOutput.arp_sha, oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_ARP_THA):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_ARP_THA);
            memcpy(oxmOutput.arp_tha, oxm->data, OFP_ETH_ALEN);
            break;
        }
        case (OFPXMT_OFB_IPV6_SRC):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IPV6_SRC);
            memcpy(oxmOutput.ipv6_src, oxm->data, OFPXMT_OFB_IPV6_SZ);
            if (oxm->length > OFPXMT_OFB_IPV6_SZ)
            {
                //TBD
            }
            break;
        }
        case (OFPXMT_OFB_IPV6_DST):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_IPV6_DST);
            memcpy(oxmOutput.ipv6_dst, oxm->data, OFPXMT_OFB_IPV6_SZ);
            if (oxm->length > OFPXMT_OFB_IPV6_SZ)
            {
                //TBD
            }
            break;
        }
        case (OFPXMT_OFB_IPV6_FLABEL):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_ICMPV6_TYPE):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_ICMPV6_CODE):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_TARGET):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_SLL):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_IPV6_ND_TLL):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_MPLS_LABEL):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_MPLS_LABEL);
            oxmOutput.mpls_label = ntohl(*(UINT4 *)oxm->data);
            break;
        }
        case (OFPXMT_OFB_MPLS_TC):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFP_MPLS_BOS):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_PBB_ISID):
        {
            //TBD
            break;
        }
        case (OFPXMT_OFB_TUNNEL_ID):
        {
            SET_MASK(oxmOutput.mask, OFPXMT_OFB_TUNNEL_ID);
            oxmOutput.tunnel_id = ntohl(*(UINT4 *)oxm->data);
            break;
        }
        case (OFPXMT_OFB_IPV6_EXTHDR):
        {
            //TBD
            break;
        }
        default:
            break;
    }

    return oxm->length;
}


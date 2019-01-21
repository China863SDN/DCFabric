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
*   File Name   : CFlowMod.cpp        *
*   Author      : bnc cyyang                *
*   Create Date : 2017-12-21               *
*   Version     : 1.0                     *
*   Function    : .                       *
*                                                                             *
******************************************************************************/
#include "log.h"
#include "bnc-error.h"
#include "bnc-type.h"
#include "bnc-param.h"
#include "bnc-inet.h"
#include "comm-util.h"
#include "CBuffer.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "CSwitch.h"
#include "CFlowMod.h"
#include "SwitchManageMode.h"
#include "SwitchManagePresistence.h"
#include "CFlowTableEventReportor.h"
#include "CClusterService.h"


//CMutex   CFlowMod::g_sendPktMutex;



CFlowMod::CFlowMod()
{
}

CFlowMod::~CFlowMod()
{
}



//by:yhy 生成一flow_param,分配内存空间并初始化
//by:yhy 初始化一个刘表结构体
flow_param_t* CFlowMod::init_flow_param()
{
	flow_param_t* flow_param =  (flow_param_t*)bnc_malloc(sizeof(flow_param_t));
	memset(flow_param, 0, sizeof(flow_param_t));
	flow_param->match_param =  (gn_oxm_t*)bnc_malloc(sizeof(gn_oxm_t));
	memset(flow_param->match_param, 0, sizeof(gn_oxm_t));
	flow_param->instruction_param = NULL;
	flow_param->action_param = NULL;
	flow_param->write_action_param = NULL;

	return flow_param;
}
//by:yhy 销毁一flow_param,资源回收
void CFlowMod::clear_flow_param(flow_param_t* flow_param)
{
	bnc_free(flow_param->match_param);
	clear_action_param(flow_param->instruction_param);
	clear_action_param(flow_param->action_param);
	clear_action_param(flow_param->write_action_param);
	bnc_free(flow_param);
}

// 增加匹配字段
UINT2 CFlowMod::of13_add_oxm_field(UINT1 *buf, gn_oxm_t *oxm_fields)
{
    UINT2 oxm_len = 0;
    struct ofp_oxm_header *oxm = (struct ofp_oxm_header *)buf;
    size_t oxm_field_sz = 0;
    UINT4 netmask = 0;
    UINT4 netmaskv6[4] = {0};

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IN_PORT))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IN_PORT << 1;
        oxm->length = OFPXMT_OFB_IN_PORT_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->in_port);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IN_PORT_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IN_PHY_PORT))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IN_PHY_PORT << 1;
        oxm->length = OFPXMT_OFB_IN_PORT_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->in_phy_port);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IN_PORT_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_METADATA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_METADATA << 1;
        oxm->length = 8;
        *(UINT8 *)(oxm->data) = htonll(oxm_fields->metadata);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_METADATA_MASK))
        {
            oxm->length = 16;
            *(UINT8 *)(oxm->data + 8) = htonll(oxm_fields->metadata_mask);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm_len += sizeof(struct ofp_oxm_header) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ETH_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_DST << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->eth_dst, OFP_ETH_ALEN);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ETH_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_SRC << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->eth_src, OFP_ETH_ALEN);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ETH_TYPE))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_TYPE << 1;
        oxm->length = OFPXMT_OFB_ETH_TYPE_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->eth_type);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_TYPE_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_VLAN_VID))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_VLAN_VID << 1;
        oxm->length = OFPXMT_OFB_VLAN_VID_SZ;
        oxm_fields->vlan_vid = OFPVID_PRESENT | oxm_fields->vlan_vid;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->vlan_vid);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_VLAN_VID_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_VLAN_PCP))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_VLAN_PCP << 1;
        oxm->length = OFPXMT_OFB_VLAN_PCP_SZ;
        *(UINT1 *)(oxm->data) = (oxm_fields->vlan_pcp);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_VLAN_PCP_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IP_DSCP))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_DSCP << 1;
        oxm->length = OFPXMT_OFB_IP_DSCP_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_dscp;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IP_DSCP_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IP_ECN))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_ECN << 1;
        oxm->length = OFPXMT_OFB_IP_DSCP_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_ecn;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IP_DSCP_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IP_PROTO))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_PROTO << 1;
        oxm->length = OFPXMT_OFB_IP_PROTO_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_proto;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IP_PROTO_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV4_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV4_SRC_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV4_SRC << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV4_SZ * 2;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_src);

			netmask = ntohl(cidr_to_subnet_mask(oxm_fields->ipv4_src_prefix));
            *(UINT4 *)(oxm->data + 4) = htonl(netmask);
            oxm_len += OFPXMT_OFB_IPV4_SZ;
        }
        else
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV4_SRC << 1;
            oxm->length = OFPXMT_OFB_IPV4_SZ;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_src);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV4_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV4_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV4_DST_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV4_DST << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV4_SZ * 2;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_dst);

			netmask = ntohl(cidr_to_subnet_mask(oxm_fields->ipv4_dst_prefix));
            *(UINT4 *)(oxm->data + 4) = htonl(netmask);
            oxm_len += OFPXMT_OFB_IPV4_SZ;
        }
        else
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV4_DST << 1;
            oxm->length = OFPXMT_OFB_IPV4_SZ;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_dst);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV4_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_TCP_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_TCP_SRC << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->tcp_src);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_L4_PORT_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_TCP_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_TCP_DST << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->tcp_dst);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_L4_PORT_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_UDP_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_UDP_SRC << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->udp_src);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_L4_PORT_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_UDP_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_UDP_DST << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->udp_dst);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_L4_PORT_SZ;
    }
    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ICMPV4_TYPE))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ICMPV4_TYPE << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->icmpv4_type;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 1;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ICMPV4_CODE))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ICMPV4_CODE << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->icmpv4_code;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 1;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_OP))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_OP << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->arp_op;

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 1;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_SPA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_SPA << 1;
        oxm->length = OFPXMT_OFB_IPV4_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->arp_spa);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV4_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_TPA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_TPA << 1;
        oxm->length = OFPXMT_OFB_IPV4_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->arp_tpa);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV4_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_SHA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_SHA << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->arp_sha, OFP_ETH_ALEN);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_TYPE_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_THA))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_THA << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->arp_tha, OFP_ETH_ALEN);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_ETH_TYPE_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV6_SRC))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV6_SRC_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV6_SRC << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV6_SZ + OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_src, OFPXMT_OFB_IPV6_SZ);

            switch(oxm_fields->ipv6_src_prefix)
            {
                case 8:
                {
                    netmaskv6[0] = htonl(0xff000000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 16:
                {
                    netmaskv6[0] = htonl(0xffff0000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 24:
                {
                    netmaskv6[0] = htonl(0xffffff00);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 32:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 40:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xff000000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 48:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffff0000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 56:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffff00);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 64:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 72:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xff000000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 80:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffff0000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 88:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffff00);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 96:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 104:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xff000000);
                    break;
                }
                case 112:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffff0000);
                    break;
                }
                case 120:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffff00);
                    break;
                }
                case 128:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffffff);
                    break;
                }
                default:break;
            }

            memcpy((UINT1 *)(oxm->data + OFPXMT_OFB_IPV6_SZ), netmaskv6, OFPXMT_OFB_IPV6_SZ);
            oxm_len += OFPXMT_OFB_IPV6_SZ;
        }
        else
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV6_SRC << 1;
            oxm->length = OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_src, 16);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV6_SZ;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV6_DST))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV6_DST_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV6_DST << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV6_SZ + OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_dst, OFPXMT_OFB_IPV6_SZ);

            switch(oxm_fields->ipv6_dst_prefix)
            {
                case 8:
                {
                    netmaskv6[0] = htonl(0xff000000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 16:
                {
                    netmaskv6[0] = htonl(0xffff0000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 24:
                {
                    netmaskv6[0] = htonl(0xffffff00);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 32:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 40:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xff000000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 48:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffff0000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 56:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffff00);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 64:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 72:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xff000000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 80:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffff0000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 88:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffff00);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 96:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 104:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xff000000);
                    break;
                }
                case 112:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffff0000);
                    break;
                }
                case 120:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffff00);
                    break;
                }
                case 128:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffffff);
                    break;
                }
            }
            memcpy((UINT1 *)(oxm->data + OFPXMT_OFB_IPV6_SZ), netmaskv6, OFPXMT_OFB_IPV6_SZ);
            oxm_len += OFPXMT_OFB_IPV6_SZ;
        }
        else

        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV6_DST << 1;
            oxm->length = OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_dst, 16);
        }

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + OFPXMT_OFB_IPV6_SZ;
    }
    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_MPLS_LABEL))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_MPLS_LABEL << 1;
        oxm->length = 4;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->mpls_label);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 4;
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_TUNNEL_ID))
    {
        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_TUNNEL_ID << 1;
        oxm->length = 4;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->tunnel_id);

        oxm_field_sz += sizeof(*oxm) + oxm->length;
        oxm = (struct ofp_oxm_header *)(buf + oxm_field_sz);
        oxm_len += sizeof(struct ofp_oxm_header) + 4;
    }


    return oxm_len;
}

// 增加设置字段
UINT2 CFlowMod::of13_add_set_field(UINT1 *buf, gn_oxm_t *oxm_fields)
{
    struct ofp_action_set_field *oa_set = (struct ofp_action_set_field *)buf;
    struct ofp_oxm_header *oxm = NULL;
    size_t oxm_field_sz = 0;
    UINT2 set_field_len = 0;
    UINT4 netmask = 0;
    UINT4 netmaskv6[4] = {0};

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IN_PORT))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IN_PORT << 1;
        oxm->length = OFPXMT_OFB_IN_PORT_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->in_port);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IN_PHY_PORT))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IN_PHY_PORT << 1;
        oxm->length = OFPXMT_OFB_IN_PORT_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->in_phy_port);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_METADATA))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_METADATA << 1;
        oxm->length = 8;
        *(UINT8 *)(oxm->data) = htonll(oxm_fields->metadata);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_METADATA_MASK))
        {
            oxm->length = 16;
            *(UINT8 *)(oxm->data + 8) = htonll(oxm_fields->metadata_mask);
        }

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ETH_DST))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_DST << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->eth_dst, OFP_ETH_ALEN);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ETH_SRC))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_SRC << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->eth_src, OFP_ETH_ALEN);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ETH_TYPE))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ETH_TYPE << 1;
        oxm->length = OFPXMT_OFB_ETH_TYPE_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->eth_type);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_VLAN_VID))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_VLAN_VID << 1;
        oxm->length = OFPXMT_OFB_VLAN_VID_SZ;
        oxm_fields->vlan_vid = OFPVID_PRESENT | oxm_fields->vlan_vid;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->vlan_vid);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_VLAN_PCP))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_VLAN_PCP << 1;
        oxm->length = OFPXMT_OFB_VLAN_PCP_SZ;
        *(UINT1 *)(oxm->data) = (oxm_fields->vlan_pcp);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IP_DSCP))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_DSCP << 1;
        oxm->length = OFPXMT_OFB_IP_DSCP_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_dscp;

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IP_ECN))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_ECN << 1;
        oxm->length = OFPXMT_OFB_IP_DSCP_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_ecn;

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IP_PROTO))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_IP_PROTO << 1;
        oxm->length = OFPXMT_OFB_IP_PROTO_SZ;
        *(UINT1 *)(oxm->data) = oxm_fields->ip_proto;

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV4_SRC))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV4_SRC_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV4_SRC << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV4_SZ * 2;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_src);
            switch(oxm_fields->ipv4_src_prefix)
            {
                case 8:
                {
                    netmask = 0xff000000;
                    break;
                }
                case 16:
                {
                    netmask = 0xffff0000;
                    break;
                }
                case 24:
                {
                    netmask = 0xffffff00;
                    break;
                }
                default:break;
            }

            *(UINT4 *)(oxm->data + 4) = htonl(netmask);
        }
        else
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV4_SRC << 1;
            oxm->length = OFPXMT_OFB_IPV4_SZ;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_src);
        }

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV4_DST))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV4_DST_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV4_DST << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV4_SZ * 2;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_dst);
            switch(oxm_fields->ipv4_dst_prefix)
            {
                case 8:
                {
                    netmask = 0xff000000;
                    break;
                }
                case 16:
                {
                    netmask = 0xffff0000;
                    break;
                }
                case 24:
                {
                    netmask = 0xffffff00;
                    break;
                }
                default:break;
            }

            *(UINT4 *)(oxm->data + 4) = htonl(netmask);
        }
        else
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV4_DST << 1;
            oxm->length = OFPXMT_OFB_IPV4_SZ;
            *(UINT4 *)(oxm->data) = htonl(oxm_fields->ipv4_dst);
        }

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_TCP_SRC))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_TCP_SRC << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->tcp_src);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_TCP_DST))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_TCP_DST << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->tcp_dst);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_UDP_SRC))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_UDP_SRC << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->udp_src);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_UDP_DST))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_UDP_DST << 1;
        oxm->length = OFPXMT_OFB_L4_PORT_SZ;
        *(UINT2 *)(oxm->data) = htons(oxm_fields->udp_dst);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ICMPV4_TYPE))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ICMPV4_TYPE << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->icmpv4_type;

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ICMPV4_CODE))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ICMPV4_CODE << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->icmpv4_code;

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_OP))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_OP << 1;
        oxm->length = 1;
        *(UINT1 *)(oxm->data) = oxm_fields->arp_op;

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_SPA))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_SPA << 1;
        oxm->length = OFPXMT_OFB_IPV4_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->arp_spa);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_TPA))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_TPA << 1;
        oxm->length = OFPXMT_OFB_IPV4_SZ;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->arp_tpa);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_SHA))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_SHA << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->arp_sha, OFP_ETH_ALEN);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_ARP_THA))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_ARP_THA << 1;
        oxm->length = OFPXMT_OFB_ETH_SZ;
        memcpy((UINT1 *)(oxm->data), oxm_fields->arp_tha, OFP_ETH_ALEN);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV6_SRC))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV6_SRC_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV6_SRC << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV6_SZ + OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_src, OFPXMT_OFB_IPV6_SZ);

            switch(oxm_fields->ipv6_src_prefix)
            {
                case 8:
                {
                    netmaskv6[0] = htonl(0xff000000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 16:
                {
                    netmaskv6[0] = htonl(0xffff0000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 24:
                {
                    netmaskv6[0] = htonl(0xffffff00);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 32:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 40:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xff000000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 48:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffff0000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 56:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffff00);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 64:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 72:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xff000000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 80:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffff0000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 88:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffff00);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 96:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 104:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xff000000);
                    break;
                }
                case 112:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffff0000);
                    break;
                }
                case 120:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffff00);
                    break;
                }
                case 128:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffffff);
                    break;
                }
                default:break;
            }

            memcpy((UINT1 *)(oxm->data + OFPXMT_OFB_IPV6_SZ), netmaskv6, OFPXMT_OFB_IPV6_SZ);
        }
        else   //鏃犳帺锟??
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV6_SRC << 1;
            oxm->length = OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_src, 16);
        }

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV6_DST))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);

        if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_IPV6_DST_PREFIX))
        {
            oxm->oxm_field_hm = (OFPXMT_OFB_IPV6_DST << 1) + 1 ;
            oxm->length = OFPXMT_OFB_IPV6_SZ + OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_dst, OFPXMT_OFB_IPV6_SZ);

            switch(oxm_fields->ipv6_dst_prefix)
            {
                case 8:
                {
                    netmaskv6[0] = htonl(0xff000000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 16:
                {
                    netmaskv6[0] = htonl(0xffff0000);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 24:
                {
                    netmaskv6[0] = htonl(0xffffff00);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 32:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = 0x0;
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 40:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xff000000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 48:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffff0000);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 56:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffff00);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 64:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = 0x0;
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 72:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xff000000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 80:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffff0000);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 88:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffff00);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 96:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = 0x0;
                    break;
                }
                case 104:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xff000000);
                    break;
                }
                case 112:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffff0000);
                    break;
                }
                case 120:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffff00);
                    break;
                }
                case 128:
                {
                    netmaskv6[0] = htonl(0xffffffff);
                    netmaskv6[1] = htonl(0xffffffff);
                    netmaskv6[2] = htonl(0xffffffff);
                    netmaskv6[3] = htonl(0xffffffff);
                    break;
                }
            }
            memcpy((UINT1 *)(oxm->data + OFPXMT_OFB_IPV6_SZ), netmaskv6, OFPXMT_OFB_IPV6_SZ);
        }
        else
        {
            oxm->oxm_field_hm = OFPXMT_OFB_IPV6_DST << 1;
            oxm->length = OFPXMT_OFB_IPV6_SZ;
            memcpy((UINT1 *)(oxm->data), oxm_fields->ipv6_dst, 16);
        }

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_MPLS_LABEL))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_MPLS_LABEL << 1;
        oxm->length = 4;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->mpls_label);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    if(IS_MASK_SET(oxm_fields->mask, OFPXMT_OFB_TUNNEL_ID))
    {
        oa_set->type = htons(OFPAT13_SET_FIELD);
        oxm = (struct ofp_oxm_header *)oa_set->field;

        oxm->oxm_class = htons(OFPXMC_OPENFLOW_BASIC);
        oxm->oxm_field_hm = OFPXMT_OFB_TUNNEL_ID << 1;
        oxm->length = 4;
        *(UINT4 *)(oxm->data) = htonl(oxm_fields->tunnel_id);

        oxm_field_sz = ALIGN_8(sizeof(struct ofp_action_set_field) - 4 + sizeof(struct ofp_oxm_header) + oxm->length);
        set_field_len += oxm_field_sz;
        oa_set->len = htons(oxm_field_sz);
        oa_set = (struct ofp_action_set_field *)(buf + set_field_len);
    }

    return set_field_len;
}



UINT2 CFlowMod::of13_add_action(UINT1 *buf, gn_action_t *action)
{
    UINT2 action_len = 0;
    UINT1 *act = buf;

    for(gn_action_t *p_action = action; NULL != p_action; p_action = p_action->next)
    {
        switch(p_action->type)
        {
            case OFPAT13_OUTPUT:
            {
                gn_action_output_t *p_action_output = (gn_action_output_t *)p_action;
                struct ofp13_action_output *ofp13_ao = (struct ofp13_action_output *)act;
                ofp13_ao->type = htons(OFPAT13_OUTPUT);
                ofp13_ao->len = htons(16);
                ofp13_ao->port = htonl(p_action_output->port);
                ofp13_ao->max_len = htons(p_action_output->max_len);
                memset(ofp13_ao->pad, 0x0, 6);

                act += 16;
                action_len += 16;
                break;
            }
            case OFPAT13_COPY_TTL_OUT:
            {
                break;
            }
            case OFPAT13_COPY_TTL_IN:
            {
                break;
            }
            case OFPAT13_MPLS_TTL:
            {
                gn_action_mpls_ttl_t *p_action_mpls_ttl = (gn_action_mpls_ttl_t *)p_action;
                struct ofp_action_mpls_ttl *ofp13_mpls_ttl = (struct ofp_action_mpls_ttl *)act;
                ofp13_mpls_ttl->type = htons(OFPAT13_MPLS_TTL);
                ofp13_mpls_ttl->len = htons(8);;
                ofp13_mpls_ttl->mpls_ttl = p_action_mpls_ttl->mpls_tt;
                memset(ofp13_mpls_ttl->pad, 0x0, 3);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_DEC_MPLS_TTL:
            {
                break;
            }
            case OFPAT13_PUSH_VLAN:
            {
//                printf("action push vlan\n");
                struct ofp_action_push *oa_push = (struct ofp_action_push *)act;
                oa_push->type = htons(OFPAT13_PUSH_VLAN);
                oa_push->len = htons(8);
                oa_push->ethertype = htons(ETH_TYPE_VLAN_8021Q);
                memset(oa_push->pad, 0x0, 2);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_POP_VLAN:
            {
                struct ofp_action_pop_mpls *oa_pop = (struct ofp_action_pop_mpls *)act;
                oa_pop->type = htons(OFPAT13_POP_VLAN);
                oa_pop->len = htons(8);
                oa_pop->ethertype = htons(ETH_TYPE_VLAN_8021Q);
                memset(oa_pop->pad, 0x0, 2);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_PUSH_MPLS:
            {
                struct ofp_action_push *oa_push = (struct ofp_action_push *)act;
                oa_push->type = htons(OFPAT13_PUSH_MPLS);
                oa_push->len = htons(8);
                oa_push->ethertype = htons(ETH_TYPE_MPLS_8021Q);
                memset(oa_push->pad, 0x0, 2);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_POP_MPLS:
            {
                struct ofp_action_pop_mpls *oa_pop = (struct ofp_action_pop_mpls *)act;
                oa_pop->type = htons(OFPAT13_POP_MPLS);
                oa_pop->len = htons(8);
                oa_pop->ethertype = htons(ETH_TYPE_MPLS_8021Q);
                memset(oa_pop->pad, 0x0, 2);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_SET_QUEUE:
            {
				gn_action_set_queue_t *p_action_queue = (gn_action_set_queue_t*)p_action;
				struct ofp_action_set_queue *ofp_queue = (struct ofp_action_set_queue*)act;
				ofp_queue->type = htons(OFPAT13_SET_QUEUE);
				ofp_queue->len = htons(8);
				ofp_queue->queue_id = htonl(p_action_queue->queue_id);

				act += 8;
				action_len += 8;
                break;
            }
            case OFPAT13_GROUP:
            {
                gn_action_group_t *p_action_group = (gn_action_group_t *)p_action;
                struct ofp_action_group *ofp13_ag = (struct ofp_action_group *)act;
                ofp13_ag->type = htons(OFPAT13_GROUP);
                ofp13_ag->len = htons(8);;
                ofp13_ag->group_id = htonl(p_action_group->group_id);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_SET_NW_TTL:
            {
                gn_action_nw_ttl_t *p_action_nw_ttl = (gn_action_nw_ttl_t *)p_action;
                struct ofp_action_nw_ttl *ofp13_nw_ttl = (struct ofp_action_nw_ttl *)act;
                ofp13_nw_ttl->type = htons(OFPAT13_SET_NW_TTL);
                ofp13_nw_ttl->len = htons(8);;
                ofp13_nw_ttl->nw_ttl = p_action_nw_ttl->nw_tt;
                memset(ofp13_nw_ttl->pad, 0x0, 3);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_DEC_NW_TTL:
            {
                break;
            }
            case OFPAT13_SET_FIELD:
            {
                gn_action_set_field_t *p_action_set_field = (gn_action_set_field_t *)p_action;
                UINT2 set_field_len = of13_add_set_field(act, &(p_action_set_field->oxm_fields));

                act += set_field_len;
                action_len += set_field_len;
                break;
            }
            case OFPAT13_PUSH_PBB:
            {
                struct ofp_action_push *oa_push = (struct ofp_action_push *)act;
                oa_push->type = htons(OFPAT13_PUSH_PBB);
                oa_push->len = htons(8);
                oa_push->ethertype = htons(ETH_TYPE_PBB_8021AH);
                memset(oa_push->pad, 0x0, 2);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_POP_PBB:
            {
                struct ofp_action_pop_mpls *oa_pop = (struct ofp_action_pop_mpls *)act;
                oa_pop->type = htons(OFPAT13_POP_PBB);
                oa_pop->len = htons(8);
                oa_pop->ethertype = htons(ETH_TYPE_PBB_8021AH);
                memset(oa_pop->pad, 0x0, 2);

                act += 8;
                action_len += 8;
                break;
            }
            case OFPAT13_EXPERIMENTER:
            {
				gn_action_experimenter_t *p_action_experimenter = (gn_action_experimenter_t*)p_action;
				struct ofp_action_experimenter_header *ofp_experimenter = (struct ofp_action_experimenter_header*)act;
				ofp_experimenter->type = htons(OFPAT13_EXPERIMENTER);
				ofp_experimenter->len = htons(8);
				ofp_experimenter->experimenter = htonl(p_action_experimenter->experimenter);

				act += 8;
				action_len += 8;
                break;
            }
            default:
                break;
        }
    }

    return action_len;
}

UINT2 CFlowMod::of13_add_instruction(UINT1 *buf, gn_flow_t *flow)
{
    UINT2 instruction_len = 0;
    UINT2 action_len = 0;
    UINT1 *ins = buf;
    gn_instruction_t *p_ins = flow->instructions;

    if(NULL == p_ins)
    {
        struct ofp_instruction_actions *oin = (struct ofp_instruction_actions *)ins;
        oin->type = ntohs(OFPIT_APPLY_ACTIONS);
        oin->pad[0] = 0;
        oin->pad[1] = 0;
        oin->pad[2] = 0;
        oin->pad[3] = 0;

        oin->len = ntohs(sizeof(struct ofp_instruction_actions));

        ins = ins + 8;
        instruction_len = instruction_len + 8;
        return instruction_len;
    }

    while(p_ins)
    {
        if((p_ins->type == OFPIT_APPLY_ACTIONS) || (p_ins->type == OFPIT_WRITE_ACTIONS))
        {

            gn_instruction_actions_t *p_ins_actions = (gn_instruction_actions_t *)p_ins;
            struct ofp_instruction_actions *oin = (struct ofp_instruction_actions *)ins;
            oin->type = ntohs(p_ins->type);
            oin->pad[0] = 0;
            oin->pad[1] = 0;
            oin->pad[2] = 0;
            oin->pad[3] = 0;

            action_len = of13_add_action((UINT1 *)(oin->actions), p_ins_actions->actions);
            oin->len = ntohs(sizeof(struct ofp_instruction_actions) + ALIGN_8(action_len));
//            printf("action len: %d, instruction len: %d\n", action_len, sizeof(struct ofp_instruction_actions) + action_len);

            ins = ins + 8 + action_len;
            instruction_len = instruction_len + 8 + action_len;
        }
        else if(p_ins->type == OFPIT_CLEAR_ACTIONS)
        {

            struct ofp_instruction_actions *oin = (struct ofp_instruction_actions *)ins;
            oin->type = ntohs(OFPIT_CLEAR_ACTIONS);
            oin->len = ntohs(8);
            oin->pad[0] = 0;
            oin->pad[1] = 0;
            oin->pad[2] = 0;
            oin->pad[3] = 0;

            ins += 8;
            instruction_len += 8;
        }
        else if(p_ins->type == OFPIT_GOTO_TABLE)
        {
//            printf("Instruct goto table\n");
            gn_instruction_goto_table_t *p_ins_goto_table = (gn_instruction_goto_table_t *)p_ins;
            struct ofp_instruction_goto_table *oi_table = (struct ofp_instruction_goto_table *)ins;
            oi_table->type = ntohs(OFPIT_GOTO_TABLE);
            oi_table->len  = ntohs(8);
            oi_table->table_id = p_ins_goto_table->table_id;
            memset(oi_table->pad, 0x0, 3);

            ins += 8;
            instruction_len += 8;
        }
        else if(p_ins->type == OFPIT_WRITE_METADATA)
        {
//            printf("Instruct metadata\n");
            gn_instruction_write_metadata_t *p_ins_metadata = (gn_instruction_write_metadata_t *)p_ins;
            struct ofp_instruction_write_metadata *oi_metadata = (struct ofp_instruction_write_metadata *)ins;
            oi_metadata->type = ntohs(OFPIT_WRITE_METADATA);
            oi_metadata->len  = ntohs(24);
            memset(oi_metadata->pad, 0x0, 4);
            oi_metadata->metadata = htonll(p_ins_metadata->metadata);
            oi_metadata->metadata_mask = htonll(p_ins_metadata->metadata_mask);

            ins += 24;
            instruction_len += 24;
        }
        else if(p_ins->type == OFPIT_METER)
        {
//            printf("Instruct meter\n");
            gn_instruction_meter_t *p_ins_meter = (gn_instruction_meter_t *)p_ins;
            struct ofp_instruction_meter *oi_meter = (struct ofp_instruction_meter *)ins;
            oi_meter->type = htons(OFPIT_METER);
            oi_meter->len = htons(8);
            oi_meter->meter_id = htonl(p_ins_meter->meter_id);

            ins += 8;
            instruction_len += 8;
        }
        else if(p_ins->type == OFPIT_EXPERIMENTER)
        {
//            printf("Instruct experimenter\n");
            gn_instruction_experimenter_t *p_ins_experimenter = (gn_instruction_experimenter_t *)p_ins;
            struct ofp_instruction_experimenter *oi_experimenter = (struct ofp_instruction_experimenter *)ins;
            oi_experimenter->type = htons(OFPIT_EXPERIMENTER);
            oi_experimenter->len = htons(8);
            oi_experimenter->experimenter = htonl(p_ins_experimenter->experimenter);

            ins += 8;
            instruction_len += 8;
        }

        p_ins = p_ins->next;
    }

    return instruction_len;
}


UINT2 CFlowMod::of13_add_match(struct ofpx_match *match, gn_match_t *gn_match)
{
    UINT2 oxm_len = 0;
    UINT2 match_len = 0;

    oxm_len = of13_add_oxm_field(match->oxm_fields, &(gn_match->oxm_fields));
    match_len = sizeof(struct ofpx_match) - 4 + oxm_len;
    return match_len;
}

//by:yhy 将oxm中的匹配值段的表现形式转化成flow_oxm中的匹配字段表现形式
void CFlowMod::set_flow_match(gn_oxm_t* flow_oxm, gn_oxm_t* oxm)
{
	UINT1 zero_array[24] = {0};
	if ((NULL == flow_oxm) || (NULL == oxm)) {
		return;
	}

	memcpy(flow_oxm, oxm, sizeof(gn_oxm_t));

	if (oxm->in_port) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IN_PORT);
	}
	if (oxm->in_phy_port) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IN_PHY_PORT);
	}
	if (oxm->metadata) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_METADATA);
	}
	if (memcmp(zero_array, oxm->eth_dst, 6)) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ETH_DST);
	}	
	if (memcmp(zero_array, oxm->eth_src, 6)) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ETH_SRC);
	}
	if (oxm->eth_type) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ETH_TYPE);
	}
	if (oxm->vlan_vid) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_VLAN_VID);
	}
	if (oxm->vlan_pcp) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_VLAN_PCP);
	}
	if (oxm->ip_dscp) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IP_DSCP);
	}
	if (oxm->ip_ecn) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IP_ECN);
	}
	if (oxm->ip_proto) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IP_PROTO);
	}
	if (oxm->ipv4_src) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IPV4_SRC);
	}
	if (oxm->ipv4_dst) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IPV4_DST);
	}
	if (oxm->tcp_src) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_TCP_SRC);
	}
	if (oxm->tcp_dst) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_TCP_DST);
	}
	if (oxm->udp_src) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_UDP_SRC);
	}
	if (oxm->udp_dst) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_UDP_DST);
	}
#if 0
	if (oxm->icmpv4_type) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ICMPV4_TYPE);
	}
	if (oxm->icmpv4_code) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ICMPV4_CODE);
	}
#else
	if (IPPROTO_ICMP == oxm->ip_proto) {
        if (oxm->icmpv4_type != 0xFF) {
            SET_MASK(flow_oxm->mask, OFPXMT_OFB_ICMPV4_TYPE);
        }
        if (oxm->icmpv4_code != 0xFF) {
            SET_MASK(flow_oxm->mask, OFPXMT_OFB_ICMPV4_CODE);
        }
	}
#endif
	if (oxm->arp_op) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ARP_OP);
	}
	if (oxm->arp_spa) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ARP_SPA);
	}
	if (oxm->arp_tpa) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ARP_TPA);
	}
	if (memcmp(zero_array, oxm->arp_sha, 6)) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ARP_SHA);
	}
	if (memcmp(zero_array, oxm->arp_tha, 6)) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_ARP_THA);
	}
	if (memcmp(zero_array, oxm->ipv6_src, 16)) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IPV6_SRC);
	}
	if (memcmp(zero_array, oxm->ipv6_dst, 16)) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IPV6_DST);
	}

	if (oxm->mpls_label) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_MPLS_LABEL);
	}
	if (oxm->tunnel_id) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_TUNNEL_ID);
	}
	if (oxm->ipv4_src_prefix) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IPV4_SRC_PREFIX);
	}
	if (oxm->ipv4_dst_prefix) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IPV4_DST_PREFIX);
	}
	if (oxm->ipv6_src_prefix) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IPV6_SRC_PREFIX);
	}
	if (oxm->ipv6_dst_prefix) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_IPV6_DST_PREFIX);
	}
	if (oxm->metadata_mask) {
        SET_MASK(flow_oxm->mask, OFPXMT_OFB_METADATA_MASK);
	}
}

void CFlowMod::set_security_match( gn_oxm_t* oxm, security_param_t* security_param)
{

	if ((NULL != security_param) && (security_param->ip_proto)) {
		oxm->ip_proto = security_param->ip_proto;
		oxm->icmpv4_code = security_param->icmp_code;
		oxm->icmpv4_type = security_param->imcp_type;
		oxm->tcp_src = security_param->tcp_port_num;
		oxm->udp_src = security_param->udp_port_num;
		if ((IPPROTO_ICMP == oxm->ip_proto) || (IPPROTO_TCP == oxm->ip_proto) || (IPPROTO_UDP == oxm->ip_proto)) 
			oxm->eth_type = ETHER_IP;
	}
}
//by:yhy 将instraction_param这里的instruction转化成flow_instruction中的instruction
//就是转换action的表现形式
gn_instruction_t* CFlowMod::set_flow_instruction(gn_instruction_t* flow_instruction, action_param_t* instraction_param)
{

	gn_instruction_t* instruction = flow_instruction;
	gn_instruction_goto_table_t* instruction_goto = NULL;
	gn_instruction_write_metadata_t* instruction_metadata = NULL;
	gn_instruction_actions_t* instruction_write = NULL;
	gn_instruction_actions_t* instruction_apply = NULL;
	gn_instruction_actions_t* instruction_clear = NULL;
	gn_instruction_meter_t* instrucion_meter = NULL;
	gn_instruction_experimenter_t* instruction_experimenter = NULL;

	while (NULL != instraction_param) {
		switch (instraction_param->type) {
		case OFPIT_GOTO_TABLE:
		{
			instruction_goto = (gn_instruction_goto_table_t*)bnc_malloc(sizeof(gn_instruction_goto_table_t));
			memset(instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
			instruction_goto->type = (UINT2)OFPIT_GOTO_TABLE;
			instruction_goto->table_id = *(UINT1*)instraction_param->param;
			break;
		}
		case OFPIT_WRITE_METADATA:
		{

			instruction_metadata = (gn_instruction_write_metadata_t*)bnc_malloc(sizeof(gn_instruction_write_metadata_t));
			memset(instruction_metadata, 0, sizeof(gn_instruction_write_metadata_t));
			instruction_metadata->type = (UINT2)OFPIT_WRITE_METADATA;
			instruction_metadata->metadata = *(UINT8*)instraction_param->param;
			instruction_metadata->metadata_mask = *((UINT8*)instraction_param->param + 1);

			break;
		}
		case OFPIT_WRITE_ACTIONS:
		{
			instruction_write = (gn_instruction_actions_t*)bnc_malloc(sizeof(gn_instruction_actions_t));
			memset(instruction_write, 0, sizeof(gn_instruction_actions_t));
			instruction_write->type = (UINT2)OFPIT_APPLY_ACTIONS;
			break;
		}
		case OFPIT_APPLY_ACTIONS:
		{
			// printf("***%s\n",FN);
			instruction_apply = (gn_instruction_actions_t*)bnc_malloc(sizeof(gn_instruction_actions_t));
			memset(instruction_apply, 0, sizeof(gn_instruction_actions_t));
			instruction_apply->type = (UINT2)OFPIT_APPLY_ACTIONS;
			break;
		}
		case OFPIT_CLEAR_ACTIONS:
		{
			instruction_clear = (gn_instruction_actions_t*)bnc_malloc(sizeof(gn_instruction_actions_t));
			memset(instruction_clear, 0, sizeof(gn_instruction_actions_t));
			instruction_clear->type = (UINT2)OFPIT_CLEAR_ACTIONS;
			break;
		}
		case OFPIT_METER:
		{
			instrucion_meter = (gn_instruction_meter_t*)bnc_malloc(sizeof(gn_instruction_meter_t));
			memset(instrucion_meter, 0, sizeof(gn_instruction_meter_t));
			instrucion_meter->type = (UINT2)OFPIT_METER;
			instrucion_meter->meter_id = *(UINT4*)instraction_param->param;
			break;
		}
		case OFPIT_EXPERIMENTER:
		{
			instruction_experimenter = (gn_instruction_experimenter_t*)bnc_malloc(sizeof(gn_instruction_experimenter_t));
			memset(instruction_experimenter, 0, sizeof(gn_instruction_experimenter_t));
			instruction_experimenter->type = (UINT2)OFPIT_EXPERIMENTER;
			instruction_experimenter->len = 4; //???
			instruction_experimenter->experimenter = *(UINT4*)instraction_param->param;
			break;
		}
		default:
			break;
		}
		instraction_param = instraction_param->next;
	}


	if (NULL != instruction_experimenter) {
		instruction_experimenter->next = instruction;
		instruction = (gn_instruction_t*)instruction_experimenter;
	}

	if (NULL != instrucion_meter) {
		instrucion_meter->next = instruction;
		instruction = (gn_instruction_t*)instrucion_meter;
	}
	if (NULL != instruction_apply) {
		instruction_apply->next = instruction;
		instruction = (gn_instruction_t*)instruction_apply;
	}
	if (NULL != instruction_clear) {
		instruction_clear->next = instruction;
		instruction = (gn_instruction_t*)instruction_clear;
	}
	if (NULL != instruction_write) {
		instruction_write->next = instruction;
		instruction = (gn_instruction_t*)instruction_write;
	}
	if (NULL != instruction_metadata) {
		instruction_metadata->next = instruction;
		instruction = (gn_instruction_t*)instruction_metadata;
	}
	if (NULL != instruction_goto) {
		instruction_goto->next = instruction;
		instruction = (gn_instruction_t*)instruction_goto;
	}

	return instruction;
}

void CFlowMod::set_flow_action(gn_instruction_t* flow_instruction, action_param_t* action_param, enum ofp_instruction_type instruction_type)
{
	gn_instruction_t* instruction_p = flow_instruction;
	if (NULL == instruction_p) {
		 //printf("%s instruction is NULL!\n", FN);
		 return;
	}

	while (NULL != instruction_p)
	{
		if (instruction_type == instruction_p->type) {
			break;
		}
		instruction_p = instruction_p->next;
	}

	if (NULL == instruction_p) {
		return ;
	}

	gn_instruction_actions_t* instruction = (gn_instruction_actions_t*)instruction_p;

	for (; NULL != action_param; action_param = action_param->next) {
		// printf("action type is %d,parameter is%d\n", action_param->type, *(UINT4*)action_param->param);
		switch (action_param->type) {
		case OFPAT13_OUTPUT:
		{
			gn_action_output_t* action_output = (gn_action_output_t*)bnc_malloc(sizeof(gn_action_output_t));
			memset(action_output, 0, sizeof(gn_action_t));
			action_output->port = *(UINT4*)action_param->param;
			action_output->type = OFPAT13_OUTPUT;
			action_output->next = instruction->actions;
			action_output->max_len = 0xffff;
			instruction->actions = (gn_action_t *)action_output;
			break;
		}
		case OFPAT13_COPY_TTL_OUT:
		{
			gn_action_t* act_copyTTLOut = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_copyTTLOut, 0, sizeof(gn_action_t));
			act_copyTTLOut->type = OFPAT13_COPY_TTL_OUT;
			act_copyTTLOut->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_copyTTLOut;
			break;
		}
		case OFPAT13_COPY_TTL_IN:
		{
			gn_action_t* act_copyTTLIn = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_copyTTLIn, 0, sizeof(gn_action_t));
			act_copyTTLIn->type = OFPAT13_COPY_TTL_IN;
			act_copyTTLIn->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_copyTTLIn;
			break;
		}
		case OFPAT13_MPLS_TTL:
		{
			gn_action_mpls_ttl_t* act_setMplsTtl = (gn_action_mpls_ttl_t*)bnc_malloc(sizeof(gn_action_mpls_ttl_t));
			memset(act_setMplsTtl, 0, sizeof(gn_action_mpls_ttl_t));
			act_setMplsTtl->type = OFPAT13_MPLS_TTL;
			act_setMplsTtl->mpls_tt = *(UINT1*)action_param->param;
			act_setMplsTtl->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_setMplsTtl;
			break;
		}
		case OFPAT13_DEC_MPLS_TTL:
		{
			gn_action_t* act_decMplsTtl = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_decMplsTtl, 0, sizeof(gn_action_t));
			act_decMplsTtl->type = OFPAT13_DEC_MPLS_TTL;
			act_decMplsTtl->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_decMplsTtl;
			break;
		}
		case OFPAT13_PUSH_VLAN:
		{
			gn_action_t* act_pushVlan = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_pushVlan, 0, sizeof(gn_action_t));
			act_pushVlan->type = OFPAT13_PUSH_VLAN;
			act_pushVlan->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_pushVlan;
			break;
		}
		case OFPAT13_POP_VLAN:
		{
			gn_action_t* act_popVlan = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_popVlan, 0, sizeof(gn_action_t));
			act_popVlan->type = OFPAT13_POP_VLAN;
			act_popVlan->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_popVlan;
			break;
		}
		case OFPAT13_PUSH_MPLS:
		{
			gn_action_t* act_pushMpls = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_pushMpls, 0, sizeof(gn_action_set_field_t));
			act_pushMpls->type = OFPAT13_PUSH_MPLS;
			act_pushMpls->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_pushMpls;
			break;
		}
		case OFPAT13_POP_MPLS:
		{
			gn_action_t* act_popMpls = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_popMpls, 0, sizeof(gn_action_t));
			act_popMpls->type = OFPAT13_POP_MPLS;
			act_popMpls->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_popMpls;
			break;
		}
		case OFPAT13_SET_QUEUE:
		{
			gn_action_set_queue_t* act_queue = (gn_action_set_queue_t*)bnc_malloc(sizeof(gn_action_set_queue_t));
			memset(act_queue, 0, sizeof(gn_action_group_t));
			act_queue->type = OFPAT13_SET_QUEUE;
			act_queue->queue_id = *(UINT4*)action_param->param;
			act_queue->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_queue;
			break;
		}
		case OFPAT13_GROUP:
		{
			gn_action_group_t* act_group = (gn_action_group_t*)bnc_malloc(sizeof(gn_action_group_t));
			memset(act_group, 0, sizeof(gn_action_group_t));
			act_group->type = OFPAT13_GROUP;
			act_group->group_id = *(UINT4*)action_param->param;
			act_group->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_group;
			break;
		}
		case OFPAT13_SET_NW_TTL:
		{
			gn_action_nw_ttl_t* act_setNwTTl = (gn_action_nw_ttl_t*)bnc_malloc(sizeof(gn_action_nw_ttl_t));
			memset(act_setNwTTl, 0, sizeof(gn_action_nw_ttl_t));
			act_setNwTTl->type = OFPAT13_SET_NW_TTL;
			act_setNwTTl->nw_tt = *(UINT1*)action_param->param;
			act_setNwTTl->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_setNwTTl;
			break;
		}
		case OFPAT13_DEC_NW_TTL:
		{
			gn_action_t* act_decNwTtl = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_decNwTtl, 0, sizeof(gn_action_t));
			act_decNwTtl->type = OFPAT13_DEC_NW_TTL;
			act_decNwTtl->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_decNwTtl;
			break;
		}
		case OFPAT13_SET_FIELD:
		{
			gn_action_set_field_t* act_set_field = (gn_action_set_field_t*)bnc_malloc(sizeof(gn_action_set_field_t));
			memset(act_set_field, 0, sizeof(gn_action_set_field_t));
			act_set_field->type = OFPAT13_SET_FIELD;
			set_flow_match(&act_set_field->oxm_fields, (gn_oxm_t*)action_param->param);
			act_set_field->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_set_field;
			break;
		}
		case OFPAT13_PUSH_PBB:
		{
			gn_action_t* act_pushPBB = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_pushPBB, 0, sizeof(gn_action_set_field_t));
			act_pushPBB->type = OFPAT13_PUSH_PBB;
			act_pushPBB->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_pushPBB;
			break;
		}
		case OFPAT13_POP_PBB:
		{
			gn_action_t* act_popPBB = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
			memset(act_popPBB, 0, sizeof(gn_action_t));
			act_popPBB->type = OFPAT13_POP_PBB;
			act_popPBB->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_popPBB;
			break;
		}
		case OFPAT13_EXPERIMENTER:
		{
			gn_action_experimenter_t* act_experimenter = (gn_action_experimenter_t*)bnc_malloc(sizeof(gn_action_experimenter_t));
			memset(act_experimenter, 0, sizeof(gn_action_experimenter_t));
			act_experimenter->type = OFPAT13_EXPERIMENTER;
			act_experimenter->experimenter = *(UINT4*)action_param->param;
			act_experimenter->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_experimenter;
			break;
		}
		default:
			break;
		}
	}
}

void CFlowMod::clear_instruction_data(gn_instruction_t* instruction)
{
	gn_instruction_t* temp_instruction = NULL;

	while (NULL != instruction) {
		temp_instruction = instruction;
		
		if ((OFPIT_APPLY_ACTIONS == temp_instruction->type)||(OFPIT_WRITE_ACTIONS == temp_instruction->type)) {
			gn_instruction_actions_t* action_instruction = (gn_instruction_actions_t*)temp_instruction;
			gn_action_t* action_p = action_instruction->actions;
			gn_action_t* temp_p = NULL;
			
			while(action_p)
			{
				temp_p = action_p;
				action_p = action_p->next;
				temp_p->next = NULL;
				bnc_free(temp_p);
				
			}
		}
		instruction = instruction->next;
		
		temp_instruction->next = NULL;
		bnc_free(temp_instruction);
	}
}

//by:yhy 将type和param初始化到action_param中,并将action_param->next指向自身
//by:yhy 在流表项内部添加一个action(动作)
void CFlowMod::add_action_param(action_param_t** action_param, INT4 type, void* param)
{
	action_param_t* new_param = (action_param_t*)bnc_malloc(sizeof(action_param_t));
	memset(new_param, 0, sizeof(action_param_t));
	new_param->type = type;
	new_param->param = param;
	new_param->next = *action_param;
	*action_param = new_param;
}
void CFlowMod::add_action_param_rear(action_param_t** action_param, INT4 type, void* param)
{
	action_param_t* new_param =  (action_param_t*)bnc_malloc(sizeof(action_param_t));
	memset(new_param, 0, sizeof(action_param_t));
	new_param->type = type;
	new_param->param = param;
	new_param->next = NULL;
	*action_param = new_param;
}

void CFlowMod::clear_action_param(action_param_t* action_param)
{
	action_param_t* temp = NULL;
	while (NULL != action_param) {
		temp = action_param;
		action_param = action_param->next;		
		bnc_free(temp);
	}
}


/*
 * 以上代码是从C版本DCFabric中拷贝过来的
 * 后续会修改, 现在只进行简单测试
 */


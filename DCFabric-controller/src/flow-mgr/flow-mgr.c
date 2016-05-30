/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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
*   File Name   : flow-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-2           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "flow-mgr.h"
#include "uuid.h"
#include "mem_pool.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"

const UINT8 MASK_SET = 1;
UINT4 g_max_flowentry = 100000;
void *g_gnflow_mempool_id = NULL;
void *g_gninstruction_mempool_id = NULL;
void *g_gnaction_mempool_id = NULL;

void gn_flow_free(gn_flow_t *flow)
{
    gn_instruction_t *p_ins = flow->instructions, *p_ins_next;
    gn_action_t *p_act = NULL, *p_act_next;

    while(p_ins)
    {
        p_ins_next = p_ins->next;
        if((p_ins->type == OFPIT_APPLY_ACTIONS) || (p_ins->type == OFPIT_WRITE_ACTIONS))
        {
            p_act = ((gn_instruction_actions_t *)p_ins)->actions;
            while(p_act)
            {
                p_act_next = p_act->next;
                mem_free(g_gnaction_mempool_id, (void *)p_act);
                p_act = p_act_next;
            }
        }

        mem_free(g_gninstruction_mempool_id, (void *)p_ins);
        p_ins = p_ins_next;
    }

    mem_free(g_gnflow_mempool_id, (void *)flow);
}

gn_flow_t * find_flow_entry_by_id(gn_switch_t *sw, gn_flow_t *flow)
{
    gn_flow_t *p_flow = sw->flow_entries;

    if((NULL == flow->uuid) || (sw->state == 0))
    {
        return NULL;
    }

    while(p_flow)
    {
//        if((p_flow->table_id == flow->table_id) && (0 == strcmp(p_flow->uuid, flow->uuid)))
    	if(0 == strcmp(p_flow->uuid, flow->uuid))
        {
            return p_flow;
        }

        p_flow = p_flow->next;
    }

    return NULL;
}

/*
void test_oxm_compare(gn_oxm_t *oxm_1, gn_oxm_t *oxm_2)
{
    printf("##### %s START\n", FN);
    if(oxm_1->mask != oxm_2->mask)
    {
        printf("  mask not equal: 1- %llu, 2- %llu\n", oxm_1->mask, oxm_2->mask);
    }

    if(oxm_1->in_port != oxm_2->in_port)
    {
        printf("  in_port not equal: 1- %d, 2- %d\n", oxm_1->in_port, oxm_2->in_port);
    }

    if(oxm_1->in_phy_port != oxm_2->in_phy_port)
    {
        printf("  in_phy_port not equal: 1- %d, 2- %d\n", oxm_1->in_phy_port, oxm_2->in_phy_port);
    }

    if(oxm_1->metadata != oxm_2->metadata)
    {
        printf("  metadata not equal: 1- %llu, 2- %llu\n", oxm_1->metadata, oxm_2->metadata);
    }

    if(memcmp(oxm_1->eth_dst , oxm_2->eth_dst, 6))
    {
        printf("  eth_dst not equal: \n");
        printf("%2x:%2x:%2x:%2x:%2x:%2x\n",oxm_1->eth_dst[0], oxm_1->eth_dst[1], oxm_1->eth_dst[2],
                oxm_1->eth_dst[3],oxm_1->eth_dst[4],oxm_1->eth_dst[5]);
        printf("%2x:%2x:%2x:%2x:%2x:%2x\n",oxm_2->eth_dst[0], oxm_2->eth_dst[1], oxm_2->eth_dst[2],
                oxm_2->eth_dst[3],oxm_2->eth_dst[4],oxm_2->eth_dst[5]);
    }

    if(memcmp(oxm_1->eth_src , oxm_2->eth_src, 6))
    {
        printf("  eth_src not equal: \n");
        printf("%2x:%2x:%2x:%2x:%2x:%2x\n",oxm_1->eth_src[0], oxm_1->eth_src[1], oxm_1->eth_src[2],
                oxm_1->eth_src[3],oxm_1->eth_src[4],oxm_1->eth_src[5]);
        printf("%2x:%2x:%2x:%2x:%2x:%2x\n",oxm_2->eth_src[0], oxm_2->eth_src[1], oxm_2->eth_src[2],
                oxm_2->eth_src[3],oxm_2->eth_src[4],oxm_2->eth_src[5]);
    }

    if(oxm_1->eth_type != oxm_2->eth_type)
    {
        printf("  eth_type not equal: 1- %d, 2- %d\n", oxm_1->eth_type, oxm_2->eth_type);
    }

    if(oxm_1->vlan_vid != oxm_2->vlan_vid)
    {
        printf("  vlan_vid not equal: 1- %d, 2- %d\n", oxm_1->vlan_vid, oxm_2->vlan_vid);
    }

    if(oxm_1->vlan_pcp != oxm_2->vlan_pcp)
    {
        printf("  vlan_pcp not equal: 1- %d, 2- %d\n", oxm_1->vlan_pcp, oxm_2->vlan_pcp);
    }

    if(oxm_1->ip_dscp != oxm_2->ip_dscp)
    {
        printf("  ip_dscp not equal: 1- %d, 2- %d\n", oxm_1->ip_dscp, oxm_2->ip_dscp);
    }

    if(oxm_1->ip_ecn != oxm_2->ip_ecn)
    {
        printf("  ip_ecn not equal: 1- %d, 2- %d\n", oxm_1->ip_ecn, oxm_2->ip_ecn);
    }

    if(oxm_1->eth_type != oxm_2->eth_type)
    {
        printf("  eth_type not equal: 1- %d, 2- %d\n", oxm_1->eth_type, oxm_2->eth_type);
    }

    if(oxm_1->ip_proto != oxm_2->ip_proto)
    {
        printf("  ip_proto not equal: 1- %d, 2- %d\n", oxm_1->ip_proto, oxm_2->ip_proto);
    }

    if(oxm_1->ipv4_src != oxm_2->ipv4_src)
    {
        printf("  ipv4_src not equal: 1- %d, 2- %d\n", oxm_1->ipv4_src, oxm_2->ipv4_src);
    }

    if(oxm_1->ipv4_dst != oxm_2->ipv4_dst)
    {
        printf("  ipv4_dst not equal: 1- %d, 2- %d\n", oxm_1->ipv4_dst, oxm_2->ipv4_dst);
    }

    if(oxm_1->tcp_src != oxm_2->tcp_src)
    {
        printf("  tcp_src not equal: 1- %d, 2- %d\n", oxm_1->tcp_src, oxm_2->tcp_src);
    }

    if(oxm_1->tcp_dst != oxm_2->tcp_dst)
    {
        printf("  tcp_dst not equal: 1- %d, 2- %d\n", oxm_1->tcp_dst, oxm_2->tcp_dst);
    }

    if(oxm_1->udp_src != oxm_2->udp_src)
    {
        printf("  udp_src not equal: 1- %d, 2- %d\n", oxm_1->udp_src, oxm_2->udp_src);
    }

    if(oxm_1->udp_dst != oxm_2->udp_dst)
    {
        printf("  udp_dst not equal: 1- %d, 2- %d\n", oxm_1->udp_dst, oxm_2->udp_dst);
    }

    if(oxm_1->icmpv4_type != oxm_2->icmpv4_type)
    {
        printf("  icmpv4_type not equal: 1- %d, 2- %d\n", oxm_1->icmpv4_type, oxm_2->icmpv4_type);
    }

    if(oxm_1->icmpv4_code != oxm_2->icmpv4_code)
    {
        printf("  icmpv4_code not equal: 1- %d, 2- %d\n", oxm_1->icmpv4_code, oxm_2->icmpv4_code);
    }

    if(oxm_1->arp_op != oxm_2->arp_op)
    {
        printf("  arp_op not equal: 1- %d, 2- %d\n", oxm_1->arp_op, oxm_2->arp_op);
    }

    if(oxm_1->arp_spa != oxm_2->arp_spa)
    {
        printf("  arp_spa not equal: 1- %d, 2- %d\n", oxm_1->arp_spa, oxm_2->arp_spa);
    }

    if(oxm_1->arp_tpa != oxm_2->arp_tpa)
    {
        printf("  arp_tpa not equal: 1- %d, 2- %d\n", oxm_1->arp_tpa, oxm_2->arp_tpa);
    }

    if(memcmp(oxm_1->arp_sha, oxm_2->arp_sha, 6))
    {
        printf("  arp_sha not equal: \n");
        printf("%2x:%2x:%2x:%2x:%2x:%2x\n",oxm_1->arp_sha[0], oxm_1->arp_sha[1], oxm_1->arp_sha[2],
                oxm_1->arp_sha[3],oxm_1->arp_sha[4],oxm_1->arp_sha[5]);
        printf("%2x:%2x:%2x:%2x:%2x:%2x\n",oxm_2->arp_sha[0], oxm_2->arp_sha[1], oxm_2->arp_sha[2],
                oxm_2->arp_sha[3],oxm_2->arp_sha[4],oxm_2->arp_sha[5]);
    }

    if(memcmp(oxm_1->arp_tha, oxm_2->arp_tha, 6))
    {
        printf("  arp_tha not equal: \n");
        printf("%2x:%2x:%2x:%2x:%2x:%2x\n",oxm_1->arp_tha[0], oxm_1->arp_tha[1], oxm_1->arp_tha[2],
                oxm_1->arp_tha[3],oxm_1->arp_tha[4],oxm_1->arp_tha[5]);
        printf("%2x:%2x:%2x:%2x:%2x:%2x\n",oxm_2->arp_tha[0], oxm_2->arp_tha[1], oxm_2->arp_tha[2],
                oxm_2->arp_tha[3],oxm_2->arp_tha[4],oxm_2->arp_tha[5]);
    }

    if(memcmp(oxm_1->ipv6_src, oxm_2->ipv6_src, 16))
    {
        printf("  ipv6_src not equal: 1- %llu, 2- %llu\n", oxm_1->ipv6_src, oxm_2->ipv6_src);
    }

    if(memcmp(oxm_1->ipv6_dst, oxm_2->ipv6_dst, 16))
    {
        printf("  ipv6_dst not equal: 1- %llu, 2- %llu\n", oxm_1->ipv6_dst, oxm_2->ipv6_dst);
    }

    if(oxm_1->mpls_label != oxm_2->mpls_label)
    {
        printf("  mpls_label not equal: 1- %d, 2- %d\n", oxm_1->mpls_label, oxm_2->mpls_label);
    }

    if(oxm_1->ipv4_src_prefix != oxm_2->ipv4_src_prefix)
    {
        printf("  ipv4_src_prefix not equal: 1- %d, 2- %d\n", oxm_1->ipv4_src_prefix, oxm_2->ipv4_src_prefix);
    }

    if(oxm_1->ipv4_dst_prefix != oxm_2->ipv4_dst_prefix)
    {
        printf("  ipv4_dst_prefix not equal: 1- %d, 2- %d\n", oxm_1->ipv4_dst_prefix, oxm_2->ipv4_dst_prefix);
    }

    if(oxm_1->ipv6_src_prefix != oxm_2->ipv6_src_prefix)
    {
        printf("  ipv6_src_prefix not equal: 1- %d, 2- %d\n", oxm_1->ipv6_src_prefix, oxm_2->ipv6_src_prefix);
    }

    if(oxm_1->ipv6_dst_prefix != oxm_2->ipv6_dst_prefix)
    {
        printf("  ipv6_dst_prefix not equal: 1- %d, 2- %d\n", oxm_1->ipv6_dst_prefix, oxm_2->ipv6_dst_prefix);
    }

    printf("##### END\n");
}
*/

//BOOL match_compare_strict(const gn_oxm_t *src_oxm_fields, const gn_oxm_t *target_oxm_fields)
BOOL match_compare_strict(const gn_match_t *src_match, const gn_match_t *target_match)
{
//    test_oxm_compare(&src_match->oxm_fields, &target_match->oxm_fields);
    if(0 == memcmp(&(src_match->oxm_fields), &(target_match->oxm_fields), sizeof(gn_oxm_t)))
    {
        return TRUE;
    }

    return FALSE;
}


BOOL match_compare_slack(const gn_match_t *src_match, const gn_match_t *target_match)
{
    gn_oxm_t *src_oxm_fields = (gn_oxm_t *)&(src_match->oxm_fields);
    gn_oxm_t *target_oxm_fields = (gn_oxm_t *)&(target_match->oxm_fields);
    if((src_oxm_fields->mask & target_oxm_fields->mask) != src_oxm_fields->mask)
    {
        printf("%s mask\n", FN);
        return FALSE;
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PORT))
    {
        if(src_oxm_fields->in_port != target_oxm_fields->in_port)
        {
            printf("%s inport\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PHY_PORT))
    {
        if(src_oxm_fields->in_phy_port != target_oxm_fields->in_phy_port)
        {
            printf("%s inport\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_METADATA))
    {
        if(src_oxm_fields->in_phy_port != target_oxm_fields->in_phy_port)
        {
            printf("%s in_phy_port\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_DST))
    {
        if(0 != memcmp(src_oxm_fields->eth_dst, target_oxm_fields->eth_dst, OFP_ETH_ALEN))
        {
            printf("%s eth_dst\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_SRC))
    {
        if(0 != memcmp(src_oxm_fields->eth_src, target_oxm_fields->eth_src, OFP_ETH_ALEN))
        {
            printf("%s eth_src\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_TYPE))
    {
        if(src_oxm_fields->eth_type != target_oxm_fields->eth_type)
        {
            printf("%s eth_type\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_VID))
    {
        if(src_oxm_fields->vlan_vid != target_oxm_fields->vlan_vid)
        {
            printf("%s vlan_vid\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_PCP))
    {
        if(src_oxm_fields->vlan_pcp != target_oxm_fields->vlan_pcp)
        {
            printf("%s vlan_pcp\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_DSCP))
    {
        if(src_oxm_fields->ip_dscp != target_oxm_fields->ip_dscp)
        {
            printf("%s ip_dscp\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_ECN))
    {
        if(src_oxm_fields->ip_ecn != target_oxm_fields->ip_ecn)
        {
            printf("%s ip_ecn\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_PROTO))
    {
        if(src_oxm_fields->ip_proto != target_oxm_fields->ip_proto)
        {
            printf("%s ip_proto\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC))
    {
        if (src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC_PREFIX))  //有掩码
        {
            if(src_oxm_fields->ipv4_src_prefix != target_oxm_fields->ipv4_src_prefix)
            {
                printf("%s ipv4_src_prefix\n", FN);
                return FALSE;
            }
        }

        if(src_oxm_fields->ipv4_src != target_oxm_fields->ipv4_src)
        {
            printf("%s ipv4_src\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST))
    {
        if (src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST_PREFIX))  //有掩码
        {
            if(src_oxm_fields->ipv4_dst_prefix != target_oxm_fields->ipv4_dst_prefix)
            {
                printf("%s ipv4_dst_prefix\n", FN);
                return FALSE;
            }
        }

        if(src_oxm_fields->ipv4_dst != target_oxm_fields->ipv4_dst)
        {
            printf("%s ipv4_dst\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_SRC))
    {
        if(src_oxm_fields->tcp_src != target_oxm_fields->tcp_src)
        {
            printf("%s tcp_src\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_DST))
    {
        if(src_oxm_fields->tcp_dst != target_oxm_fields->tcp_dst)
        {
            printf("%s tcp_dst\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_SRC))
    {
        if(src_oxm_fields->udp_src != target_oxm_fields->udp_src)
        {
            printf("%s udp_src\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_DST))
    {
        if(src_oxm_fields->udp_dst != target_oxm_fields->udp_dst)
        {
            printf("%s udp_dst\n", FN);
            return FALSE;
        }
    }

//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_SCTP_SRC))
//    {
//
//    }
//
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_SCTP_DST))
//    {
//
//    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ICMPV4_TYPE))
    {
        if(src_oxm_fields->icmpv4_type != target_oxm_fields->icmpv4_type)
        {
            printf("%s icmpv4_type\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ICMPV4_CODE))
    {
        if(src_oxm_fields->icmpv4_code != target_oxm_fields->icmpv4_code)
        {
            printf("%s icmpv4_code\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_OP))
    {
        if(src_oxm_fields->arp_op != target_oxm_fields->arp_op)
        {
            printf("%s arp_op\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_SPA))
    {
        if(src_oxm_fields->arp_spa != target_oxm_fields->arp_spa)
        {
            printf("%s arp_spa\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_TPA))
    {
        if(src_oxm_fields->arp_tpa != target_oxm_fields->arp_tpa)
        {
            printf("%s arp_tpa\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_SHA))
    {
        if(0 != memcpy(src_oxm_fields->arp_sha, target_oxm_fields->arp_sha, OFPXMT_OFB_ETH_SZ))
        {
            printf("%s arp_sha\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_THA))
    {
        if(0 != memcpy(src_oxm_fields->arp_tha, target_oxm_fields->arp_tha, OFPXMT_OFB_ETH_SZ))
        {
            printf("%s arp_tha\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_SRC))
    {
        if (src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_SRC_PREFIX))  //有掩码
        {
            if(src_oxm_fields->ipv6_src_prefix != target_oxm_fields->ipv6_src_prefix)
            {
                printf("%s ipv6_src_prefix\n", FN);
                return FALSE;
            }
        }

        if(0 != memcmp(src_oxm_fields->ipv6_src, target_oxm_fields->ipv6_src, OFPXMT_OFB_IPV6_SZ))
        {
            printf("%s ipv6_src\n", FN);
            return FALSE;
        }
    }

    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_DST))
    {
        if (src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_DST_PREFIX))  //有掩码
        {
            if(src_oxm_fields->ipv6_dst_prefix != target_oxm_fields->ipv6_dst_prefix)
            {
                printf("%s ipv6_dst_prefix\n", FN);
                return FALSE;
            }
        }

        if(0 != memcmp(src_oxm_fields->ipv6_dst, target_oxm_fields->ipv6_dst, OFPXMT_OFB_IPV6_SZ))
        {
            printf("%s ipv6_dst\n", FN);
            return FALSE;
        }
    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_FLABEL))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_ICMPV6_TYPE))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_ICMPV6_CODE))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_TARGET))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_SLL))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_TLL))
//    {
//    }
    if(src_oxm_fields->mask & (MASK_SET << OFPXMT_OFB_MPLS_LABEL))
    {
        if(src_oxm_fields->mpls_label != target_oxm_fields->mpls_label)
        {
            printf("%s mpls_label\n", FN);
            return FALSE;
        }
    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_MPLS_TC))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFP_MPLS_BOS))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_PBB_ISID))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_TUNNEL_ID))
//    {
//    }
//    if(src_oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_EXTHDR))
//    {
//    }

    return TRUE;
}

gn_flow_t * find_flow_entry(gn_switch_t *sw, gn_flow_t *flow)
{
    gn_flow_t *p_flow = sw->flow_entries;

    if((NULL == flow) || (sw->state == 0))
    {
        return NULL;
    }

    while(p_flow)
    {
        if(match_compare_strict(&(p_flow->match), &(flow->match)))
        {
            return p_flow;
        }

        p_flow = p_flow->next;
    }

    return NULL;
}

static INT4 send_flow_mod(gn_switch_t *sw, gn_flow_t *flow, UINT1 command)
{
    flow_mod_req_info_t flow_mod_req;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = command;
    flow_mod_req.flags = OFPFF_SEND_FLOW_REM;   //OFPFF13_SEND_FLOW_REM == OFPFF_SEND_FLOW_REM
    flow_mod_req.flow = flow;

    return sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
}
/*
INT4 add_flow_entry(gn_switch_t *sw, gn_flow_t *flow)
{
    uuid_t uuid_gen;
    if(sw->state == 0)
    {
        return GN_ERR;
    }

    pthread_mutex_lock(&sw->flow_entry_mutex);
    if(NULL == find_flow_entry(sw, flow))
    {
        uuid_generate_random(uuid_gen);
        uuid_unparse(uuid_gen, flow->uuid);

        if(sw->flow_entries)
        {
            gn_flow_t *p_flow = sw->flow_entries;
            while(p_flow->next)
            {
                p_flow = p_flow->next;
            };

            p_flow->next = flow;
        }
        else
        {
            flow->prev = NULL;
            flow->next = NULL;
            sw->flow_entries = flow;
        }
    }
    else
    {
        pthread_mutex_unlock(&sw->flow_entry_mutex);
        return GN_ERR;
    }

    flow->status = ENTRY_DISABLED;
    pthread_mutex_unlock(&sw->flow_entry_mutex);
    return GN_OK;
}
*/
gn_flow_t* copy_flow_entry(gn_flow_t *src)
{
	gn_instruction_t *p_ins_src,*p_ins_dst;
	gn_instruction_t *p_ins_src_next;
	gn_instruction_t *p_ins_dst_next;
    gn_action_t *p_act_src = NULL, *p_act_src_next = NULL;
    gn_action_t *p_act_dst = NULL, *p_act_dst_next = NULL;
    gn_instruction_actions_t* p_inact_src,* p_inact_dst;
    gn_flow_t *dest = NULL;

	
	dest =(gn_flow_t *)mem_get(g_gnflow_mempool_id);
	dest->prev = NULL;
	dest->next = NULL;
	memcpy(dest->uuid,src->uuid,sizeof(src->uuid));
	memcpy(dest->creater,src->creater,sizeof(src->creater));
	dest->create_time = src->create_time;
	dest->table_id = src->table_id;
	dest->status = src->status;
	dest->idle_timeout = src->idle_timeout;
	dest->hard_timeout = src->hard_timeout;
	dest->priority = src->priority;
	memcpy(&dest->stats,&src->stats,sizeof(src->stats));
	memcpy(&dest->match,&src->match,sizeof(src->match));
	
	if(src->instructions == NULL)
	{
		dest->instructions = NULL;
		return dest;
	}
	
	p_ins_src = src->instructions;

    while(p_ins_src)
    {
        p_ins_dst_next = mem_get(g_gninstruction_mempool_id);
        p_ins_dst_next->type = p_ins_src->type;   
        
        if((p_ins_src->type == OFPIT_APPLY_ACTIONS) || (p_ins_src->type == OFPIT_WRITE_ACTIONS))
        {
        	p_inact_src = (gn_instruction_actions_t *)p_ins_src;
        	p_inact_dst = (gn_instruction_actions_t *)p_ins_dst_next;
            p_act_src = p_inact_src->actions;
           
            while(p_act_src)
            {
            	p_act_dst_next = mem_get(g_gnaction_mempool_id);
            	memcpy(p_act_dst_next,p_act_src,sizeof(gn_action_t));
            	p_act_dst_next->next = NULL;
            	
            	if(p_inact_dst->actions == NULL)
            	{
            		p_inact_dst->actions = p_act_dst_next;
            	}
            	else
            	{
            		p_act_dst = p_inact_dst->actions;
            		while(p_act_dst->next)
            		{
            			p_act_dst = p_act_dst->next;
            		}
            		p_act_dst->next = p_act_dst_next;
            	}
                p_act_src_next = p_act_src->next; 
                p_act_src = p_act_src_next;
            }
        }

        if(dest->instructions == NULL)
        {
        	dest->instructions = p_ins_dst_next;
        	dest->instructions->next = NULL;
        }
        else
        {
			p_ins_dst = dest->instructions;
        	while(p_ins_dst->next)
    		{
    			p_ins_dst = p_ins_dst->next;
    		}
        	p_ins_dst->next = p_ins_dst_next;
        }
        
        p_ins_src_next = p_ins_src->next;        
        p_ins_src = p_ins_src_next;
    }

    return dest;
}

INT4 clean_flow_entry(gn_switch_t *sw, gn_flow_t *flow)
{
    gn_flow_t *p_flow = NULL;
    gn_flow_t *p_del_flow = NULL;

    if(sw->state == 0)
    {
        return EC_SW_STATE_ERR;
    }

    pthread_mutex_lock(&sw->flow_entry_mutex);
    p_flow = sw->flow_entries;
    while(p_flow)
    {
    	if(match_compare_strict(&p_flow->match,&flow->match))
    	{
    		//delete flow entry from list
		    if(p_flow->next)
		    {
		        p_flow->next->prev = p_flow->prev;
		    }

		    if(p_flow->prev)
		    {
		        p_flow->prev->next = p_flow->next;
		    }
		    else
		    {
		        sw->flow_entries = p_flow->next;
		    }
		    
			p_del_flow = p_flow;
			p_flow = p_flow->next;
    		//recycle the memory
    		gn_flow_free(p_del_flow);
    		continue;
    		
    	}
    	p_flow = p_flow->next;
    }

    pthread_mutex_unlock(&sw->flow_entry_mutex);
    return GN_OK;
}

INT4 clean_flow_entries(gn_switch_t *sw)
{
    gn_flow_t *p_flow_tmp, *p_flow = sw->flow_entries;
    
    if((NULL == sw) || (sw->state == 0))
    {
        return GN_ERR;
    }
    sw->flow_entries = NULL;
    while(p_flow)
    {
        p_flow_tmp = p_flow->next;

        //recycle the memory
        gn_flow_free(p_flow);

        p_flow = p_flow_tmp;
    }

    return GN_OK;
}

INT4 add_flow_entry(gn_switch_t *sw, gn_flow_t *flow)
{
    uuid_t uuid_gen;
    if(sw->state == 0)
    {
        return GN_ERR;
    }

    pthread_mutex_lock(&sw->flow_entry_mutex);
    if(NULL == find_flow_entry(sw, flow))
    {
        uuid_generate_random(uuid_gen);
        uuid_unparse(uuid_gen, flow->uuid);

        if(sw->flow_entries)
        {
            gn_flow_t *p_flow = sw->flow_entries;
            while(p_flow->next)
            {
                p_flow = p_flow->next;
            };
           p_flow->next = copy_flow_entry(flow);
           //p_flow->next = flow;
        }
        else
        {
            flow->prev = NULL;
            flow->next = NULL;
            sw->flow_entries = copy_flow_entry(flow);
            //sw->flow_entries = flow;
        }
    }
    else
    {
        pthread_mutex_unlock(&sw->flow_entry_mutex);
        return GN_OK;
    }

    flow->status = ENTRY_ENABLED;
    pthread_mutex_unlock(&sw->flow_entry_mutex);
    return GN_OK;
}


INT4 modify_flow_entry(gn_switch_t *sw, gn_flow_t *flow)
{
    INT4 ret = 0;
    gn_flow_t *p_flow = NULL;

    if(sw->state == 0)
    {
        return GN_ERR;
    }

    pthread_mutex_lock(&sw->flow_entry_mutex);
    p_flow = find_flow_entry_by_id(sw, flow);
    if(NULL == p_flow)
    {
        ret = EC_FLOW_NOT_EXIST;
        gn_flow_free(flow);

        pthread_mutex_unlock(&sw->flow_entry_mutex);
        return ret;
    }
    else
    {
        //modify flow entry from list
        gn_instruction_t *p_ins = p_flow->instructions;
        p_flow->instructions = flow->instructions;
        flow->instructions = p_ins;

        ret = send_flow_mod(sw, p_flow, OFPFC_MODIFY_STRICT);
        flow->status = ENTRY_ENABLED;
        pthread_mutex_unlock(&sw->flow_entry_mutex);
        return ret;
    }
}

INT4 enable_flow_entry(gn_switch_t *sw, gn_flow_t *flow)
{
    gn_flow_t *p_flow = NULL;
    INT4 ret = 0;

    if(sw->state == 0)
    {
        return GN_ERR;
    }

    pthread_mutex_lock(&sw->flow_entry_mutex);
    p_flow = find_flow_entry_by_id(sw, flow);
    if(NULL == p_flow)
    {
        p_flow = flow;
    }

    ret = send_flow_mod(sw, p_flow, OFPFC_ADD);
    if(GN_OK != ret)
    {
        pthread_mutex_unlock(&sw->flow_entry_mutex);
        return ret;
    }

    p_flow->status = ENTRY_ENABLED;
    pthread_mutex_unlock(&sw->flow_entry_mutex);
    return GN_OK;
}

INT4 disable_flow_entry(gn_switch_t *sw, gn_flow_t *flow)
{
    gn_flow_t *p_flow = NULL;

    if(sw->state == 0)
    {
        return GN_ERR;
    }

    pthread_mutex_lock(&sw->flow_entry_mutex);
    p_flow = find_flow_entry_by_id(sw, flow);
    if(NULL == p_flow)
    {
        pthread_mutex_unlock(&sw->flow_entry_mutex);
        goto ERROR;
    }

    send_flow_mod(sw, p_flow, OFPFC_DELETE_STRICT);
    flow->status = ENTRY_DISABLED;
    pthread_mutex_unlock(&sw->flow_entry_mutex);
    return GN_OK;

ERROR:
    pthread_mutex_unlock(&sw->flow_entry_mutex);
    return GN_ERR;
}

INT4 delete_flow_entry(gn_switch_t *sw, gn_flow_t *flow)
{
    gn_flow_t *p_flow = NULL;

    if(sw->state == 0)
    {
        return EC_SW_STATE_ERR;
    }

    pthread_mutex_lock(&sw->flow_entry_mutex);
    p_flow = find_flow_entry_by_id(sw, flow);
    if(NULL == p_flow)
    {
        pthread_mutex_unlock(&sw->flow_entry_mutex);
        return EC_FLOW_NOT_EXIST;
    }

    send_flow_mod(sw, p_flow, OFPFC_DELETE_STRICT);

    //delete flow entry from list
    if(p_flow->next)
    {
        p_flow->next->prev = p_flow->prev;
    }

    if(p_flow->prev)
    {
        p_flow->prev->next = p_flow->next;
    }
    else
    {
        sw->flow_entries = p_flow->next;
    }

    //recycle the memory
    gn_flow_free(p_flow);

    pthread_mutex_unlock(&sw->flow_entry_mutex);
    return GN_OK;
}

INT4 clear_flow_entries(gn_switch_t *sw)
{
    gn_flow_t flow;
    gn_flow_t *p_flow_tmp, *p_flow = sw->flow_entries;
    gn_instruction_actions_t instruction;
    gn_action_output_t action;

    if((NULL == sw) || (sw->state == 0))
    {
        return GN_ERR;
    }

    //删除所有流表
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.table_id = OFPTT_ALL;
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 0;
    flow.match.type = OFPMT_OXM;
    send_flow_mod(sw, &flow, OFPFC_DELETE);

    //下发table miss流表
    memset(&flow, 0, sizeof(gn_flow_t));
    flow.create_time = 0;
    flow.table_id = 0;
    flow.idle_timeout = 0;
    flow.hard_timeout = 0;
    flow.priority = 0;
    flow.match.type = OFPMT_OXM;

    memset(&instruction, 0, sizeof(gn_instruction_actions_t));
    instruction.type = OFPIT_APPLY_ACTIONS;
    instruction.next = flow.instructions;
    flow.instructions = (gn_instruction_t *)&instruction;

    memset(&action, 0, sizeof(gn_action_t));
    action.port = OFPP13_CONTROLLER;
    action.type = OFPAT13_OUTPUT;
    action.next = instruction.actions;
    action.max_len = 0xffff;
    instruction.actions = (gn_action_t *)&action;
    send_flow_mod(sw, &flow, OFPFC_ADD);

    sw->flow_entries = NULL;
    while(p_flow)
    {
        p_flow_tmp = p_flow->next;

        //recycle the memory
        gn_flow_free(p_flow);

        p_flow = p_flow_tmp;
    }

    return GN_OK;
}

INT4 flow_entry_timeout(gn_switch_t *sw, gn_flow_t *flow)
{
    pthread_mutex_lock(&sw->flow_entry_mutex);
    gn_flow_t *p_flow = find_flow_entry(sw, flow);
    if(p_flow)
    {
        p_flow->status = ENTRY_TIMEOUT;
        if(NULL == p_flow->prev)
        {
            sw->flow_entries = p_flow->next;
        }
        else
        {
            p_flow->prev = p_flow->next;
        }

        //delete flow entry from list
        if(p_flow->next)
        {
            p_flow->next->prev = p_flow->prev;
        }

        if(p_flow->prev)
        {
            p_flow->prev->next = p_flow->next;
        }
        else
        {
            sw->flow_entries = p_flow->next;
        }

        //recycle the memory
        gn_flow_free(p_flow);
    }
    pthread_mutex_unlock(&sw->flow_entry_mutex);
    return GN_OK;
}


INT4 init_flow_mgr()
{
    INT1 *value = NULL;

    value = get_value(g_controller_configure, "[controller]", "max_flowentry");
    g_max_flowentry = (value == NULL ? 100000 : atoll(value));

    //分配内存池
    g_gnflow_mempool_id = mem_create(sizeof(gn_flow_t), g_max_flowentry);
    if (NULL == g_gnflow_mempool_id)
    {
        return GN_ERR;
    }

    g_gninstruction_mempool_id = mem_create(sizeof(gn_instruction_write_metadata_t), g_max_flowentry);
    if (NULL == g_gninstruction_mempool_id)
    {
        return GN_ERR;
    }

    g_gnaction_mempool_id = mem_create(sizeof(gn_action_set_field_t), g_max_flowentry);
    if (NULL == g_gnaction_mempool_id)
    {
        return GN_ERR;
    }

    return GN_OK;
}

void fini_flow_mgr()
{
//    pthread_mutex_destroy(&sw->flow_entry_mutex);
    mem_destroy(g_gnflow_mempool_id);
    mem_destroy(g_gninstruction_mempool_id);
    mem_destroy(g_gnaction_mempool_id);
}

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
*   File Name   : CFlow.cpp                                                   *
*   Author      : bnc cyyang                                                  *
*   Create Date : 2017-12-21                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "bnc-type.h"
#include "comm-util.h"
#include "CFlow.h"

CFlow::CFlow():
    m_sw(NULL),
    m_create_time(0),
    m_table_id(0),
    m_idle_timeout(0),
    m_hard_timeout(0),
    m_priority(0)
{
    memset(&m_match, 0, sizeof(gn_match_t));
}

CFlow::~CFlow()
{
}

void CFlow::assign(gn_flow_t& flow)
{
    m_uuid         = flow.uuid;
    m_create_time  = flow.create_time;
    m_table_id     = flow.table_id;
    m_idle_timeout = flow.idle_timeout;
    m_hard_timeout = flow.hard_timeout;
    m_priority     = flow.priority;
    memcpy(&m_match, &(flow.match), sizeof(gn_match_t));

    for (gn_instruction_t* instruct = flow.instructions; NULL != instruct; instruct = instruct->next)
        addInstruction(instruct);
}

void CFlow::translate(gn_flow_t& flow)
{
    strncpy(flow.uuid, m_uuid.c_str(), UUID_LEN-1);
    flow.uuid[UUID_LEN-1] = '\0';
    flow.create_time  = m_create_time;
    flow.table_id     = m_table_id;
    flow.idle_timeout = m_idle_timeout;
    flow.hard_timeout = m_hard_timeout;
    flow.priority     = m_priority;
    memcpy(&flow.match, &m_match, sizeof(gn_match_t));

    flow.instructions = NULL;
    transInstructions(&(flow.instructions));
}

BOOL CFlow::match(CFlow& flow)
{
    if (m_sw != flow.m_sw)
        return FALSE;
    if (m_table_id != flow.m_table_id)
        return FALSE;

    gn_oxm_t& oxm = flow.getMatch().oxm_fields;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IN_PORT))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IN_PORT) &&
              (oxm.in_port == m_match.oxm_fields.in_port)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IN_PHY_PORT))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IN_PHY_PORT) &&
              (oxm.in_phy_port == m_match.oxm_fields.in_phy_port)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_METADATA))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_METADATA) &&
              (oxm.metadata == m_match.oxm_fields.metadata)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_DST))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ETH_DST) &&
              (0 == memcmp(oxm.eth_dst, m_match.oxm_fields.eth_dst, MAC_LEN))))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_SRC))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ETH_SRC) &&
              (0 == memcmp(oxm.eth_src, m_match.oxm_fields.eth_src, MAC_LEN))))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_TYPE))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ETH_TYPE) &&
              (oxm.eth_type == m_match.oxm_fields.eth_type)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_VLAN_VID))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_VLAN_VID) &&
              (oxm.vlan_vid == m_match.oxm_fields.vlan_vid)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_VLAN_PCP))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_VLAN_PCP) &&
              (oxm.vlan_pcp == m_match.oxm_fields.vlan_pcp)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_DSCP))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IP_DSCP) &&
              (oxm.ip_dscp == m_match.oxm_fields.ip_dscp)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_ECN))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IP_ECN) &&
              (oxm.ip_ecn == m_match.oxm_fields.ip_ecn)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_PROTO))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IP_PROTO) &&
              (oxm.ip_proto == m_match.oxm_fields.ip_proto)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_SRC))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IPV4_SRC) &&
              (oxm.ipv4_src == m_match.oxm_fields.ipv4_src)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_DST))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IPV4_DST) &&
              (oxm.ipv4_dst == m_match.oxm_fields.ipv4_dst)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TCP_SRC))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_TCP_SRC) &&
              (oxm.tcp_src == m_match.oxm_fields.tcp_src)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TCP_DST))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_TCP_DST) &&
              (oxm.tcp_dst == m_match.oxm_fields.tcp_dst)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_UDP_SRC))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_UDP_SRC) &&
              (oxm.udp_src == m_match.oxm_fields.udp_src)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_UDP_DST))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_UDP_DST) &&
              (oxm.udp_dst == m_match.oxm_fields.udp_dst)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ICMPV4_TYPE))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ICMPV4_TYPE) &&
              (oxm.icmpv4_type == m_match.oxm_fields.icmpv4_type)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ICMPV4_CODE))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ICMPV4_CODE) &&
              (oxm.icmpv4_code == m_match.oxm_fields.icmpv4_code)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_OP))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ARP_OP) &&
              (oxm.arp_op == m_match.oxm_fields.arp_op)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_SPA))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ARP_SPA) &&
              (oxm.arp_spa == m_match.oxm_fields.arp_spa)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_TPA))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ARP_TPA) &&
              (oxm.arp_tpa == m_match.oxm_fields.arp_tpa)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_SHA))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ARP_SHA) &&
              (oxm.arp_sha == m_match.oxm_fields.arp_sha)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_THA))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_ARP_THA) &&
              (oxm.arp_tha == m_match.oxm_fields.arp_tha)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_SRC))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IPV6_SRC) &&
              (0 == memcmp(oxm.ipv6_src, m_match.oxm_fields.ipv6_src, IPV6_LEN))))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_DST))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_IPV6_DST) &&
              (0 == memcmp(oxm.ipv6_dst, m_match.oxm_fields.ipv6_dst, IPV6_LEN))))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_MPLS_LABEL))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_MPLS_LABEL) &&
              (oxm.mpls_label == m_match.oxm_fields.mpls_label)))
            return FALSE;
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TUNNEL_ID))
        if (!(IS_MASK_SET(m_match.oxm_fields.mask, OFPXMT_OFB_TUNNEL_ID) &&
              (oxm.tunnel_id == m_match.oxm_fields.tunnel_id)))
            return FALSE;

    return TRUE;
}

BOOL CFlow::matchStrict(CFlow& flow)
{
    return ((m_sw == flow.m_sw) && 
            (m_idle_timeout == flow.m_idle_timeout) &&
            (m_hard_timeout == flow.m_hard_timeout) &&
            (m_priority == flow.m_priority) &&
            (m_table_id == flow.m_table_id) &&
            (0 == memcmp(&m_match, &flow.m_match, sizeof(gn_match_t))));
}

CFlow& CFlow::operator=(const CFlow& rhs)
{
    if (this != &rhs)
    {
        m_sw           = rhs.m_sw;
        m_uuid         = rhs.m_uuid;
        m_create_time  = rhs.m_create_time;
        m_table_id     = rhs.m_table_id;
        m_idle_timeout = rhs.m_idle_timeout;
        m_hard_timeout = rhs.m_hard_timeout;
        m_priority     = rhs.m_priority;
        m_instructions = rhs.m_instructions;
        memcpy(&m_match, &rhs.m_match, sizeof(gn_match_t));
    }

    return *this;
}

void CFlow::addInstruction(gn_instruction_t* instruction)
{
    CFlowInstruction flowInstruction(instruction->type);

    switch (instruction->type) 
    {
        case OFPIT_GOTO_TABLE:
        {
            gn_instruction_goto_table_t* gotoTable = (gn_instruction_goto_table_t*)instruction;
            flowInstruction.m_table_id = gotoTable->table_id;
            break;
        }
        case OFPIT_WRITE_METADATA:
        {
            gn_instruction_write_metadata_t* metadata = (gn_instruction_write_metadata_t*)instruction;
            flowInstruction.m_metadata = metadata->metadata;
            flowInstruction.m_metadata_mask = metadata->metadata_mask;
            break;
        }
        case OFPIT_WRITE_ACTIONS:
        {
            gn_instruction_actions_t* actions = (gn_instruction_actions_t*)instruction;
            for (gn_action_t* act = actions->actions; NULL != act; act = act->next)
            {
                flowInstruction.addAction(act);
            }
            break;
        }
        case OFPIT_APPLY_ACTIONS:
        {
            gn_instruction_actions_t* actions = (gn_instruction_actions_t*)instruction;
            for (gn_action_t* act = actions->actions; NULL != act; act = act->next)
            {
                flowInstruction.addAction(act);
            }
            break;
        }
        case OFPIT_CLEAR_ACTIONS:
            break;
        case OFPIT_METER:
        {
            gn_instruction_meter_t* meter = (gn_instruction_meter_t*)instruction;
            flowInstruction.m_meter_id = meter->meter_id;
            break;
        }
        case OFPIT_EXPERIMENTER:
        {
            gn_instruction_experimenter_t* experimenter = (gn_instruction_experimenter_t*)instruction;
            flowInstruction.m_experimenterLen = experimenter->len;
            flowInstruction.m_experimenter = experimenter->experimenter;
            break;
        }
        default:
            break;
    }

    m_instructions.insert(CFlowInstructionMap::value_type(flowInstruction.m_type, flowInstruction));
}

void CFlow::transInstructions(gn_instruction_t** instructions)
{
    STL_FOR_LOOP(m_instructions, it)
    {
        CFlowInstruction& instruction = it->second;
        switch (instruction.m_type)
        {
            case OFPIT_GOTO_TABLE:
            {
                gn_instruction_goto_table_t* instruct = (gn_instruction_goto_table_t*)bnc_malloc(sizeof(gn_instruction_goto_table_t));
                if (NULL != instruct)
                {
                    instruct->type = OFPIT_GOTO_TABLE;
                    instruct->table_id = instruction.m_table_id;
                    instruct->next = *instructions;
                    *instructions = (gn_instruction_t*)instruct;
                }
                break;
            }
            case OFPIT_WRITE_METADATA:
            {
                gn_instruction_write_metadata_t* instruct = (gn_instruction_write_metadata_t*)bnc_malloc(sizeof(gn_instruction_write_metadata_t));
                if (NULL != instruct)
                {
                    instruct->type = OFPIT_WRITE_METADATA;
                    instruct->metadata = instruction.m_metadata;
                    instruct->metadata_mask = instruction.m_metadata_mask;
                    instruct->next = *instructions;
                    *instructions = (gn_instruction_t*)instruct;
                }
                break;
            }
            case OFPIT_WRITE_ACTIONS:
            case OFPIT_APPLY_ACTIONS:
            {
                gn_instruction_actions_t* instruct = (gn_instruction_actions_t*)bnc_malloc(sizeof(gn_instruction_actions_t));
                if (NULL != instruct)
                {
                    instruct->type = instruction.m_type;
                    instruct->actions = NULL;
                    instruction.transActions(&(instruct->actions));
                    instruct->next = *instructions;
                    *instructions = (gn_instruction_t*)instruct;
                }
                break;
            }
            case OFPIT_CLEAR_ACTIONS:
            {
                gn_instruction_t* instruct = (gn_instruction_t*)bnc_malloc(sizeof(gn_instruction_t));
                if (NULL != instruct)
                {
                    instruct->type = OFPIT_CLEAR_ACTIONS;
                    instruct->next = *instructions;
                    *instructions = (gn_instruction_t*)instruct;
                }
                break;
            }
            case OFPIT_METER:
            {
                gn_instruction_meter_t* instruct = (gn_instruction_meter_t*)bnc_malloc(sizeof(gn_instruction_meter_t));
                if (NULL != instruct)
                {
                    instruct->type = OFPIT_METER;
                    instruct->meter_id = instruction.m_meter_id;
                    instruct->next = *instructions;
                    *instructions = (gn_instruction_t*)instruct;
                }
                break;
            }
            case OFPIT_EXPERIMENTER:
            {
                gn_instruction_experimenter_t* instruct = (gn_instruction_experimenter_t*)bnc_malloc(sizeof(gn_instruction_experimenter_t));
                if (NULL != instruct)
                {
                    instruct->type = OFPIT_EXPERIMENTER;
                    instruct->len = instruction.m_experimenterLen;
                    instruct->experimenter = instruction.m_experimenter;
                    instruct->next = *instructions;
                    *instructions = (gn_instruction_t*)instruct;
                }
                break;
            }
            default:
                break;
        }
    }
}


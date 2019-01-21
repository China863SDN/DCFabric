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
*   File Name   : CFlowInstruction.cpp                                        *
*   Author      : jiang bo                                                    *
*   Create Date : 2017-12-21                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "bnc-type.h"
#include "comm-util.h"
#include "CFlowInstruction.h"

void CFlowInstruction::addAction(gn_action_t* action)
{
    CFlowAction act(action->type);

    switch (action->type)
    {
        case OFPAT13_OUTPUT:
        {
            gn_action_output_t *ao = (gn_action_output_t *)action;
            act.m_port = ao->port;
            break;
        }
        case OFPAT13_COPY_TTL_OUT:
            break;
        case OFPAT13_COPY_TTL_IN:
            break;
        case OFPAT13_MPLS_TTL:
        {
            gn_action_mpls_ttl_t *mt = (gn_action_mpls_ttl_t *)action;
            act.m_mpls_tt = mt->mpls_tt;
            break;
        }
        case OFPAT13_DEC_MPLS_TTL:
            break;
        case OFPAT13_PUSH_VLAN:
            break;
        case OFPAT13_POP_VLAN:
            break;
        case OFPAT13_PUSH_MPLS:
            break;
        case OFPAT13_POP_MPLS:
            break;
        case OFPAT13_SET_QUEUE:
        {
            gn_action_set_queue_t *set = (gn_action_set_queue_t *)action;
            act.m_queue_id = set->queue_id;
            break;
        }
        case OFPAT13_GROUP:
        {
            gn_action_group_t *gp = (gn_action_group_t *)action;
            act.m_group_id = gp->group_id;
            break;
        }
        case OFPAT13_SET_NW_TTL:
        {
            gn_action_nw_ttl_t *nt = (gn_action_nw_ttl_t *)action;
            act.m_nw_tt = nt->nw_tt;
            break;
        }
        case OFPAT13_DEC_NW_TTL:
            break;
        case OFPAT13_SET_FIELD:
        {
            gn_action_set_field_t *sf = (gn_action_set_field_t *)action;
            memcpy(&(act.m_oxm_fields), &(sf->oxm_fields), sizeof(gn_oxm_t));
            break;
        }
        case OFPAT13_PUSH_PBB:
            break;
        case OFPAT13_POP_PBB:
            break;
        case OFPAT13_EXPERIMENTER:
        {
            gn_action_experimenter_t *exp = (gn_action_experimenter_t *)action;
            act.m_experimenter = exp->experimenter;
            break;
        }
        default:
            break;
    }

    m_actions.insert(CFlowActionMap::value_type(action->type, act));
}

void CFlowInstruction::transActions(gn_action_t** actions)
{
    STL_FOR_LOOP(m_actions, it)
    {
        CFlowAction& action = it->second;
        switch (action.m_type)
        {
            case OFPAT13_OUTPUT:
            {
                gn_action_output_t* act = (gn_action_output_t*)bnc_malloc(sizeof(gn_action_output_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_OUTPUT;
                    act->port = action.m_port;
                    act->max_len = (OFPP13_CONTROLLER==act->port)?0xffff:0;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_COPY_TTL_OUT:
                break;
            case OFPAT13_COPY_TTL_IN:
                break;
            case OFPAT13_MPLS_TTL:
            {
                gn_action_mpls_ttl_t* act = (gn_action_mpls_ttl_t*)bnc_malloc(sizeof(gn_action_mpls_ttl_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_MPLS_TTL;
                    act->mpls_tt = action.m_mpls_tt;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_DEC_MPLS_TTL:
                break;
            case OFPAT13_PUSH_VLAN:
            {
                gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_PUSH_VLAN;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_POP_VLAN:
            {
                gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_POP_VLAN;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_PUSH_MPLS:
            {
                gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_PUSH_MPLS;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_POP_MPLS:
            {
                gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_POP_MPLS;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_SET_QUEUE:
            {
                gn_action_set_queue_t* act = (gn_action_set_queue_t*)bnc_malloc(sizeof(gn_action_set_queue_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_SET_QUEUE;
                    act->queue_id = action.m_queue_id;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_GROUP:
            {
                gn_action_group_t* act = (gn_action_group_t*)bnc_malloc(sizeof(gn_action_group_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_GROUP;
                    act->group_id = action.m_group_id;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_SET_NW_TTL:
            {
                gn_action_nw_ttl_t* act = (gn_action_nw_ttl_t*)bnc_malloc(sizeof(gn_action_nw_ttl_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_SET_NW_TTL;
                    act->nw_tt = action.m_nw_tt;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_DEC_NW_TTL:
                break;
            case OFPAT13_SET_FIELD:
            {
                gn_action_set_field_t* act = (gn_action_set_field_t*)bnc_malloc(sizeof(gn_action_set_field_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_SET_FIELD;
                    memcpy(&(act->oxm_fields), &(action.m_oxm_fields), sizeof(gn_oxm_t));
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_PUSH_PBB:
            {
                gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_PUSH_PBB;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_POP_PBB:
            {
                gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_POP_PBB;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            case OFPAT13_EXPERIMENTER:
            {
                gn_action_experimenter_t* act = (gn_action_experimenter_t*)bnc_malloc(sizeof(gn_action_experimenter_t));
                if (NULL != act)
                {
                    act->type = OFPAT13_EXPERIMENTER;
                    act->experimenter = action.m_experimenter;
                    act->next = *actions;
                    *actions = (gn_action_t*)act;
                }
                break;
            }
            default:
                break;
        }
    }
}

CFlowInstruction& CFlowInstruction::operator=(const CFlowInstruction& rhs)
{
    if (this != &rhs)
    {
        m_type          = rhs.m_type;
        m_table_id      = rhs.m_table_id;
        m_metadata      = rhs.m_metadata;
        m_metadata_mask = rhs.m_metadata_mask;
        m_meter_id      = rhs.m_meter_id;
        m_experimenter  = rhs.m_experimenter;
        m_actions       = rhs.m_actions;
    }

    return *this;
}


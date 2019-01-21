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
*   File Name   : CFlowAction.h                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CFLOWACTION_H
#define _CFLOWACTION_H

#include "bnc-type.h"
#include "bnc-param.h"
#include "openflow-10.h"
#include "openflow-13.h"

#if 0
enum ofp13_action_type {
    OFPAT13_OUTPUT = 0, /* Output to switch port. */
    OFPAT13_COPY_TTL_OUT = 11, /* Copy TTL "outwards" -- from next-to-outermost to outermost */
    OFPAT13_COPY_TTL_IN = 12, /* Copy TTL "inwards" -- from outermost to next-to-outermost */
    OFPAT13_MPLS_TTL = 15, /* MPLS TTL */
    OFPAT13_DEC_MPLS_TTL = 16, /* Decrement MPLS TTL */
    OFPAT13_PUSH_VLAN = 17, /* Push a new VLAN tag */
    OFPAT13_POP_VLAN = 18, /* Pop the outer VLAN tag */
    OFPAT13_PUSH_MPLS = 19, /* Push a new MPLS tag */
    OFPAT13_POP_MPLS = 20, /* Pop the outer MPLS tag */
    OFPAT13_SET_QUEUE = 21, /* Set queue id when outputting to a port */
    OFPAT13_GROUP = 22, /* Apply group. */
    OFPAT13_SET_NW_TTL = 23, /* IP TTL. */
    OFPAT13_DEC_NW_TTL = 24, /* Decrement IP TTL. */
    OFPAT13_SET_FIELD = 25, /* Set a header field using OXM TLV format. */
    OFPAT13_PUSH_PBB = 26, /* Push a new PBB service tag (I-TAG) */
    OFPAT13_POP_PBB = 27, /* Pop the outer PBB service tag (I-TAG) */
    OFPAT13_EXPERIMENTER = 0xffff
};
#endif

class CFlowAction
{
public:
    CFlowAction():m_type(0),m_port(0),m_mpls_tt(0),m_queue_id(0),m_group_id(0),
        m_nw_tt(0),m_experimenter(0) {memset(&m_oxm_fields, 0, sizeof(gn_oxm_t));}
    CFlowAction(UINT2 type):m_type(type),m_port(0),m_mpls_tt(0),m_queue_id(0),m_group_id(0),
        m_nw_tt(0),m_experimenter(0) {memset(&m_oxm_fields, 0, sizeof(gn_oxm_t));}
    ~CFlowAction() {}

    INT4     m_type; //@ofp13_action_type
    UINT4    m_port;
    UINT1    m_mpls_tt;
    UINT4    m_queue_id;
    UINT4    m_group_id;
    UINT1    m_nw_tt;
    gn_oxm_t m_oxm_fields;
    UINT4    m_experimenter;

    CFlowAction& operator=(const CFlowAction& rhs)
    {
        if (this != &rhs)
        {
            m_type         = rhs.m_type;
            m_port         = rhs.m_port;
            m_mpls_tt      = rhs.m_mpls_tt;
            m_queue_id     = rhs.m_queue_id;
            m_group_id     = rhs.m_group_id;
            m_nw_tt        = rhs.m_nw_tt;
            m_experimenter = rhs.m_experimenter;
            memcpy(&m_oxm_fields, &rhs.m_oxm_fields, sizeof(gn_oxm_t));
        }

        return *this;
    }
};

#endif


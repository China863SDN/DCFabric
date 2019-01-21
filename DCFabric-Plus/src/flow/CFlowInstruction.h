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
*   File Name   : CFlowInstruction.h                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CFLOWINSTRUCTION_H
#define _CFLOWINSTRUCTION_H

#include "bnc-type.h"
#include "bnc-param.h"
#include "openflow-common.h"
#include "CFlowAction.h"

#if 0
enum ofp_instruction_type {
    OFPIT_GOTO_TABLE     = 1, /* Setup the next table in the lookup pipeline */
    OFPIT_WRITE_METADATA = 2, /* Setup the metadata field for use later in pipeline */
    OFPIT_WRITE_ACTIONS  = 3, /* Write the action(s) onto the datapath action set */
    OFPIT_APPLY_ACTIONS  = 4, /* Applies the action(s) immediately */
    OFPIT_CLEAR_ACTIONS  = 5, /* Clears all actions from the datapath action set */
    OFPIT_METER          = 6, /* Apply meter (rate limiter) */
    OFPIT_EXPERIMENTER   = 0xFFFF /* Experimenter instruction */
};
#endif

typedef std::map<INT4 /*ofp13_action_type*/, CFlowAction> CFlowActionMap;

class CFlowInstruction
{
public:
    CFlowInstruction():m_type(0),m_table_id(0),m_metadata(0),m_metadata_mask(0),m_meter_id(0),m_experimenterLen(0),m_experimenter(0) {}
    CFlowInstruction(INT4      type):m_type(type),m_table_id(0),m_metadata(0),m_metadata_mask(0),m_meter_id(0),m_experimenterLen(0),m_experimenter(0) {}
    ~CFlowInstruction() {}

    INT4           m_type; //@ofp_instruction_type
    UINT1          m_table_id;
    UINT8          m_metadata;
    UINT8          m_metadata_mask;
    UINT4          m_meter_id;
    UINT2          m_experimenterLen;
    UINT4          m_experimenter;
    CFlowActionMap m_actions;

    void addAction(gn_action_t* action);
    void transActions(gn_action_t** actions);

    CFlowInstruction& operator=(const CFlowInstruction& rhs);
};

#endif

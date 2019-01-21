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
*   File Name   : CFlow.h                                                     *
*   Author      : bnc cyyang                                                  *
*   Create Date : 2017-12-21                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CFLOW_H
#define _CFLOW_H

#include "CFlowInstruction.h"
#include "CSwitch.h"

typedef std::map<INT4 /*ofp_instruction_type*/, CFlowInstruction> CFlowInstructionMap;

class CFlow
{
public:
    CFlow();
    ~CFlow();

    void assign(gn_flow_t& flow);
    void translate(gn_flow_t& flow);

    CSmartPtr<CSwitch> getSw() {return m_sw;}
    std::string& getUuid() {return m_uuid;}
    UINT8 getCreateTime() {return m_create_time;}
    UINT1 getTableId() {return m_table_id;}
    UINT2 getIdleTimeout() {return m_idle_timeout;}
    UINT2 getHardTimeout() {return m_hard_timeout;}
    UINT2 getPriority() {return m_priority;}
    gn_match_t& getMatch() {return m_match;}
    CFlowInstructionMap& getInstructions() {return m_instructions;}

    void setSw(CSmartPtr<CSwitch> sw) {m_sw = sw;}
    void setUuid(std::string& uuid) {m_uuid = uuid;}
    void setCreateTime(UINT8 create_time) {m_create_time = create_time;}

    BOOL match(CFlow& flow);
    BOOL matchStrict(CFlow& flow);
    CFlow& operator=(const CFlow& rhs);

private:
    void addInstruction(gn_instruction_t* instruction);
    void transInstructions(gn_instruction_t** instructions);
    
private:
	CSmartPtr<CSwitch>  m_sw;
    std::string         m_uuid;
    UINT8               m_create_time;
    UINT1               m_table_id;
    UINT2               m_idle_timeout;
    UINT2               m_hard_timeout;
    UINT2               m_priority;
    gn_match_t          m_match;
    CFlowInstructionMap m_instructions;
};

#endif

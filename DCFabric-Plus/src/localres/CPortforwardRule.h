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
*   File Name   : CPortforwardRule.h                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CPORTFORWARDRULL_H
#define __CPORTFORWARDRULL_H

#include "bnc-param.h"
#include "CMutex.h"
#include "CRefObj.h"
#include "CRWLock.h"

class CPortforwardRule : public CRefObj
{
public:
    CPortforwardRule();
    CPortforwardRule(const CPortforwardRule& rule);
    ~CPortforwardRule();

	BOOL Compare(const CPortforwardRule& rule);

    void setEnabled(BOOL enabled) {m_enabled = enabled;}
    void setProtocol(const std::string& protocol);
    void setOutsideIp(UINT4 ip) {m_outIp = ip;}
    void setInsideIp(UINT4 ip) {m_inIp = ip;}
    void setOutsidePort(UINT2 start, UINT2 end) {m_outPortStart = start; m_outPortEnd = end;}
    void setInsidePort(UINT2 start, UINT2 end) {m_inPortStart = start; m_inPortEnd = end;}
    void setNetworkId(const std::string& networkId) {m_networkId = networkId;}
    void setSubnetId(const std::string& subnetId) {m_subnetId = subnetId;}

    BOOL getEnabled() const {return m_enabled;}
    INT4 getProtocol() const {return m_protocol;}
    UINT4 getOutsideIp() const {return m_outIp;}
    UINT4 getInsideIp() const {return m_inIp;}
    void getOutsidePort(UINT2& start, UINT2& end) const {start = m_outPortStart; end = m_outPortEnd;}
    void getInsidePort(UINT2& start, UINT2& end) const {start = m_inPortStart; end = m_inPortEnd;}
    const std::string& getNetworkId() const {return m_networkId;}
    const std::string& getSubnetId() const {return m_subnetId;}

private:
    BOOL        m_enabled;
    INT4        m_protocol;
    UINT4       m_outIp;
    UINT4       m_inIp;
    UINT2       m_outPortStart;
    UINT2       m_outPortEnd;
    UINT2       m_inPortStart;
    UINT2       m_inPortEnd;
    std::string m_networkId;
    std::string m_subnetId;
};

#endif

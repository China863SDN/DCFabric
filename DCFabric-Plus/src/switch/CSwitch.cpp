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
*   File Name   : CSwitch.cpp                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/

#include "bnc-error.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "CFlowMod.h"
#include "CConf.h"
#include "CSwitch.h"

CSwitch::CSwitch():
    m_state(SW_STATE_INIT),
    m_version(0),
    m_sockfd(-1),
    m_swIp(0),
    m_swPort(0),
    m_dpid(0),
    m_tag(0),
    m_nbuffers(0),
    m_ntables(0),
    m_nports(0),
    m_capabilities(0),
    m_heartbeatTimes(0),
    m_heartbeatInterim(0)
{
    memset(&m_swMac, 0, MAC_LEN);
    memset(&m_swDesc, 0, sizeof(switch_desc_t));
    memset(&m_loPort, 0, sizeof(gn_port_t));
}

CSwitch::CSwitch(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port):
    m_state(SW_STATE_INIT),
    m_version(0),
    m_sockfd(sockfd),
    m_swIp(ip),
    m_swPort(port),
    m_dpid(0),
    m_tag(0),
    m_nbuffers(0),
    m_ntables(0),
    m_nports(0),
    m_capabilities(0),
    m_heartbeatTimes(0),
    m_heartbeatInterim(0),
    m_manage_mode(0)
{
    memcpy(m_swMac, mac, MAC_LEN);
    memset(&m_swDesc, 0, sizeof(switch_desc_t));
    memset(&m_loPort, 0, sizeof(gn_port_t));
}

CSwitch::CSwitch(CSmartPtr<CSwitch>& sw):
    m_heartbeatTimes(0),
    m_heartbeatInterim(0)
{
    m_state = sw->getState();
    m_version = sw->getVersion();
    m_sockfd = sw->getSockfd();
    memcpy(m_swMac, sw->getSwMac(), MAC_LEN);
    m_swIp = sw->getSwIp();
    m_swPort = sw->getSwPort();
    m_dpid = sw->getDpid();
    m_tag = sw->getTag();
    m_nbuffers = sw->getNBuffers();
    m_ntables = sw->getNTables();
    m_nports = sw->getNPorts();
    m_capabilities = sw->getCapabilities();
	m_manage_mode = sw->getManageMode();
    memcpy(&m_swDesc, &(sw->getSwDesc()), sizeof(switch_desc_t));
    memcpy(&m_loPort, &(sw->getLoPort()), sizeof(gn_port_t));
    m_ports = sw->getPorts();
}

CSwitch::~CSwitch()
{
}

gn_port_t* CSwitch::getPort(UINT4 port_no)
{
    gn_port_t* port = NULL;

    //m_mutex.lock();
    m_lock.rlock();

    CPortMap::iterator it = m_ports.find(port_no);
    if (it != m_ports.end())
    {
        port = &(it->second);
    }

    //m_mutex.unlock();
    m_lock.unlock();

    return port;
}

void CSwitch::updatePort(gn_port_t& port)
{
    //m_mutex.lock();
    m_lock.wlock();

    CPortMap::iterator it = m_ports.find(port.port_no);
    if (it != m_ports.end())
    {
        //it->second = port;
        it->second.state = port.state;
        it->second.lldpDetectTimes = 0;
    }
    else
    {
        m_ports.insert(std::pair<UINT4, gn_port_t>(port.port_no, port));
    }

    //m_mutex.unlock();
    m_lock.unlock();
}

void CSwitch::deletePort(UINT4 port_no)
{
    //m_mutex.lock();
    m_lock.wlock();

    //m_ports.erase(port_no);
    CPortMap::iterator it = m_ports.find(port_no);
    if (it != m_ports.end())
    {
        it->second.state = PORT_STATE_DELETED;
        it->second.lldpDetectTimes = 0;
    }

    //m_mutex.unlock();
    m_lock.unlock();
}

void CSwitch::lockPorts()
{
    //m_mutex.lock();
    m_lock.rlock();
}

void CSwitch::unlockPorts()
{
    //m_mutex.unlock();
    m_lock.unlock();
}

INT4 CSwitch::toBeOverrided()
{
    return BNC_OK;
}
INT4 CSwitch::flowInstall( CSmartPtr<CSwitch > dst_sw, flow_param_t* &flow_param,  UINT2& goto_Table)
{
	if(dst_sw.isNull()||(NULL == flow_param))
	{
		return BNC_ERR;
	}
	CFlowMod::add_action_param(&flow_param->instruction_param,	OFPIT_GOTO_TABLE, (void*)&goto_Table);

	return BNC_OK;
}
INT4 CSwitch::flowAllocMemFree(void * mem)
{
	return (NULL == mem)? BNC_ERR: BNC_OK;
}
INT4  CSwitch::flow_fabric_nat_sameswitch_host2external(flow_param_t* &flow_param, gn_oxm_t &oxm)
{
	if(NULL == flow_param)
	{
		return BNC_ERR;
	}
	return BNC_OK;
}
INT4  CSwitch::flow_fabric_nat_sameswitch_external2host(flow_param_t* &flow_param, gn_oxm_t &oxm)
{
	if(NULL == flow_param)
	{
		return BNC_ERR;
	}
	return BNC_OK;
}


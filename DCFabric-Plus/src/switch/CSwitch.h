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
*   File Name   : CSwitch.h                                                   *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CSWITCH_H
#define __CSWITCH_H

#include "bnc-param.h"
#include "CMutex.h"
#include "CRefObj.h"
#include "CRWLock.h"

#define 	SWITCH_OVS			0
#define 	SWITCH_PHYSICAL 	1

typedef std::map<UINT4, gn_port_t> CPortMap;

enum switch_state_e
{
	SW_STATE_INIT,
	SW_STATE_NEW_ACCEPT,
	SW_STATE_CONNECTED,
	SW_STATE_STABLE,
	SW_STATE_UNREACHABLE,
	SW_STATE_DISCONNECTED,
	SW_STATE_POWER_OFF,
	SW_STATE_CLOSING,
	SW_STATE_CLOSED,
};

/*
 * OVS交换机基类
 */
class CSwitch : public CRefObj
{
public:
    CSwitch();
    CSwitch(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port);
    CSwitch(CSmartPtr<CSwitch>& sw);
    virtual ~CSwitch();

    INT4 getState() {return m_state;}
    void setState(INT4 state) {m_state = state;}

    UINT1 getVersion() {return m_version;}
    void setVersion(UINT1 version) {m_version = version;}

    INT4 getSockfd() {return m_sockfd;}
    void setSockfd(INT4 sockfd) {m_sockfd = sockfd;}

    const INT1* getSwMac() {return m_swMac;}
    void  setSwMac(INT1* mac) {memcpy(m_swMac, mac, 6);}

    UINT4 getSwIp() {return m_swIp;}
    void  setSwIp(UINT4 swIp) {m_swIp = swIp;}

    UINT2 getSwPort() {return m_swPort;}
    void  setSwPort(UINT2 swPort)    {m_swPort = swPort;}

    UINT8 getDpid() {return m_dpid;}
    void  setDpid(UINT8 dpid)    {m_dpid = dpid;}

    UINT4 getTag() {return m_tag;}
    void  setTag(UINT4 tag) {m_tag = tag;}

    UINT4 getNBuffers() {return m_nbuffers;}
    void  setNBuffers(UINT4 buffers) {m_nbuffers = buffers;}

    UINT4 getNTables() {return m_ntables;}
    void  setNTables(UINT4 tables) {m_ntables = tables;}

    UINT4 getNPorts() {return m_nports;}
    void  setNPorts(UINT4 ports) {m_nports = ports;}

	UINT1 getManageMode(){return m_manage_mode; }
	void  setManageMode(UINT1 manage_mode) {m_manage_mode = manage_mode; }

	UINT1 getSwType(){return m_swType;}
	void  setSwType(UINT1 sw_type){m_swType = sw_type;}

    UINT4 getCapabilities() {return m_capabilities;}
    void  setCapabilities(UINT4 capabilities) {m_capabilities = capabilities;}

    switch_desc_t& getSwDesc() {return m_swDesc;}
    void setSwDesc(switch_desc_t& swDesc) {m_swDesc = swDesc;}

    gn_port_t& getLoPort() {return m_loPort;}
    void setLoPort(gn_port_t& loPort) {m_loPort = loPort;}

    gn_port_t* getPort(UINT4 port_no);
    void updatePort(gn_port_t& loPort);
    void deletePort(UINT4 port_no);
    CPortMap& getPorts() {return m_ports;}
    void lockPorts();
    void unlockPorts();


	virtual INT4  flow_fabric_nat_sameswitch_host2external(flow_param_t* &flow_param, gn_oxm_t &oxm);
	virtual INT4  flow_fabric_nat_sameswitch_external2host(flow_param_t* &flow_param, gn_oxm_t &oxm);
	virtual INT4 flowInstall(CSmartPtr<CSwitch > dst_sw, flow_param_t* &flow_param,   UINT2& goto_Table);
	virtual INT4 flowAllocMemFree(void * mem);
    virtual INT4 toBeOverrided();

public:
    UINT4 m_heartbeatTimes;
    UINT4 m_heartbeatInterim;

private:
    INT4 m_state;
    UINT1 m_version;
    INT4 m_sockfd;
    INT1 m_swMac[MAC_LEN];
    UINT4 m_swIp;
    UINT2 m_swPort;
    UINT8 m_dpid;
    UINT4 m_tag;
    UINT4 m_nbuffers;
    UINT4 m_ntables;
    UINT4 m_nports;
    UINT4 m_capabilities;
	UINT1 m_manage_mode;
	UINT1 m_swType;
    switch_desc_t m_swDesc;
    gn_port_t m_loPort;
    CPortMap m_ports;
    //CMutex m_mutex;
    CRWLock m_lock;
};

#endif

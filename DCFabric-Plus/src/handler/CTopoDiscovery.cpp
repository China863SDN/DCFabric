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
*   File Name   : CTopoDiscovery.cpp                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CTopoDiscovery.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "COfMsgUtil.h"
#include "CControl.h"
#include "CServer.h"
#include "CRecvWorker.h"
#include "CClusterService.h"
#include "CConf.h"
#include "log.h"
#include "bnc-error.h"
#include "bnc-inet.h"
#include "comm-util.h"
#include "COfMsgUtil.h"
#include "COfConnectMgr.h"


//LLDP默认发送周期，单位秒
static const UINT4 LLDP_SEND_PERIOD_DEFAULT = 8;

//在拓扑边保活时，LLDP最大发送次数
static const UINT4 LLDP_DETECT_TIMES_MAX = 8;

//一次LLDP探测的交换机数目
static const UINT4 LLDP_DETECT_NUMBER_ONE_TIME = 100; //per switch

void CTopoDiscovery::floodLldp(void* param)
{
    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
        return;

    static UINT4 step = 0;
    UINT4 count = 0; //valid range is [step, step+1)*LLDP_DETECT_NUMBER_ONE_TIME
	
	LOG_INFO_FMT(">> floodLldp start: step[%u]", step);
    CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    if (map.size() <= step*LLDP_DETECT_NUMBER_ONE_TIME)
        step = 0;

    LOG_DEBUG_FMT(">> floodLldp start: step[%u]", step);
	LOG_INFO_FMT(">> floodLldp start: step[%u]", step);

    STL_FOR_LOOP(map, swit)
    {
		CSmartPtr<CSwitch> sw = swit->second;
		if (!sw.isNull() && 
            (SW_STATE_STABLE == sw->getState()) && 
            (OFP13_VERSION == sw->getVersion()) &&
            (0 != sw->getDpid()))
		{
            if (!((step*LLDP_DETECT_NUMBER_ONE_TIME <= count) && 
                  (count < (step+1)*LLDP_DETECT_NUMBER_ONE_TIME)))
            {
                ++count;
                continue;
            }
            ++count;

            INT4 sockfd = sw->getSockfd();
            
            LOG_DEBUG_FMT("floodLldp: CSwitch sockfd[%d] ...", sockfd);
            
            CPortMap& portmap = sw->getPorts();
            LOG_DEBUG_FMT("floodLldp: ports count[%lu] ...", portmap.size());
            
            STL_FOR_LOOP(portmap, portit)
            {
                gn_port_t& port = portit->second;
                if (PORT_STATE_UP == port.state)
                {
                    if (port.lldpDetectTimes >= LLDP_DETECT_TIMES_MAX)
                    {
                        BOOL isAlive = CControl::getInstance()->getTopoMgr().isLinkAlive(sw->getDpid(), port.port_no);
                        if (isAlive)
                        {
                            LOG_WARN_FMT("floodLldp: UN-ALIVE topo link switch[%llx]port[%u] !", sw->getDpid(), port.port_no);
                            CControl::getInstance()->getTopoMgr().unAliveLink(sw->getDpid(), port.port_no);
                        }
                        port.lldpDetectTimes = 0;
                    }

                    LOG_DEBUG_FMT("floodLldp: send OFPT13_PACKET_OUT|LLDP to switch[%llx] port[%u] with sockfd[%d] ...", 
                        sw->getDpid(), port.port_no, sockfd);
                    BOOL ret = COfMsgUtil::sendOfp13Lldp(sw, port);
                    if (!ret)
                    {
                        LOG_WARN_FMT("floodLldp: send OFPT13_PACKET_OUT|LLDP to switch[%llx] port[%u] with sockfd[%d] failed !", 
                            sw->getDpid(), port.port_no, sockfd);
                        COfConnectMgr::processPeerEnterUnreach(sw);
                        break;
                    }
                    port.lldpDetectTimes ++;
                }
            }
		}
    }

	CControl::getInstance()->getSwitchMgr().unlock();
    
    ++step;
    LOG_DEBUG_FMT("<< floodLldp end: step[%u], count[%u]", step, count);
}

CTopoDiscovery::CTopoDiscovery()
{
    init();
}

CTopoDiscovery::~CTopoDiscovery()
{
}

INT4 CTopoDiscovery::init()
{
    const INT1* pInterval = CConf::getInstance()->getConfig("controller", "topo_discover_interval");
    UINT4 interval = (pInterval == NULL) ? LLDP_SEND_PERIOD_DEFAULT : atol(pInterval);
    LOG_INFO_FMT("topo_discover_interval %us", interval);

    if (0 == interval)
        return BNC_OK;

    if (m_timer.schedule(interval, interval, CTopoDiscovery::floodLldp, NULL) != BNC_OK)
    {
        LOG_ERROR("CTopoDiscovery schedule floodLldp failed !");
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CTopoDiscovery::onregister()
{
    std::string path;
    if (CConf::getInstance()->getTopoDiscoverProtocol() == TOPO_DISCOVER_LLDP)
        path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_LLDP);
    else
        path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_IP, IP_INTERNAL);

    return CMsgHandler::onregister(path, 1/*, FALSE, THREAD_TYPE_OCCUPY_CORE*/);
}

void CTopoDiscovery::deregister()
{
    std::string path;
    if (CConf::getInstance()->getTopoDiscoverProtocol() == TOPO_DISCOVER_LLDP)
        path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_LLDP);
    else
        path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_IP, IP_INTERNAL);

    CMsgHandler::deregister(path);
}

INT4 CTopoDiscovery::handle(CSmartPtr<CMsgCommon> msg)
{
    CMsg* ofmsg = (CMsg*)msg.getPtr();

    LOG_DEBUG_FMT("%s[%p] handle LLDP msg of path[%s] from sockfd[%d] ...", 
        toString(), this, ofmsg->getPath().c_str(), ofmsg->getSockfd());

    INT4 sockfd = ofmsg->getSockfd();
    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (sw.isNull())
    {
        LOG_WARN_FMT("CSwitch not created for sockfd[%d] !", sockfd);
        COfConnectMgr::processSystemFailure(sockfd);
        return BNC_ERR;
    }

    INT1* data = ofmsg->getData();
    if (NULL == data)
    {
        LOG_WARN_FMT("%s[%p] received LLDP msg with no data from sockfd[%d] !", 
            toString(), this, sockfd);
        return BNC_ERR;
    }

    packet_in_info_t pktin = {0};
    COfMsgUtil::ofpPacketInConvertter(data, pktin);
	
    UINT8 neighDpid = 0;
    UINT4 neighPort = 0;

    if (CConf::getInstance()->getTopoDiscoverProtocol() == TOPO_DISCOVER_LLDP)
    {
        lldp_t* lldp = (lldp_t *)pktin.data;
        if ((lldp->chassis_tlv_subtype != LLDP_CHASSIS_ID_LOCALLY_ASSIGNED) ||
            (lldp->port_tlv_subtype != LLDP_PORT_ID_COMPONENT) ||
            (lldp->ttl_tlv_ttl != htons(120)))
        {
            LOG_WARN_FMT("%s[%p] received invalid LLDP msg from sockfd[%d] !", 
                toString(), this, sockfd);
            return BNC_ERR;
        }
        
        neighDpid = ntohll(lldp->chassis_tlv_id);
        neighPort = ntohs(lldp->port_tlv_id);
    }
    else
    {
        UINT1* ptr = pktin.data;
        ip_t* ip = (ip_t *)ptr;
        ptr += sizeof(ip_t);

        INT1 macStr[20] = {0}, ipStr[20] = {0};
        mac2str((UINT1*)sw->getSwMac(), macStr);
        number2ip(htonl(sw->getSwIp()), ipStr);
        LOG_WARN_FMT("%s[%p] received internal %u bytes IP msg(srcMac[%s]srcIp[%s]destMac[%s]destIp[%s]protocol[%u]) from sockfd[%d]", 
            toString(), this, htons(ip->len), mac2str(ip->eth_head.src, macStr), number2ip(ip->src, ipStr), 
            mac2str(ip->eth_head.dest, macStr), number2ip(ip->dest, ipStr), ip->proto, sockfd);

        if ((htons(ip->len) < (20+sizeof(UINT8)+sizeof(UINT4))) ||
            (ip->proto != IP_INTERNAL) ||
            (ip->src != ip->dest) ||
            (htonl(ip->src) != CServer::getInstance()->getControllerIp()))
        {
            LOG_WARN_FMT("%s[%p] received invalid internal IP msg from sockfd[%d] !", 
                toString(), this, sockfd);
            return BNC_ERR;
        }
        
        neighDpid = *(UINT8*)ptr;
        ptr += sizeof(UINT8);
        neighPort = *(UINT4*)ptr;
    }

    CSmartPtr<CSwitch> neighborSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(neighDpid);
    if (neighborSw.isNull())
    {
        LOG_WARN_FMT("CSwitch not created for neighbor switch[%llx], drop LLDP from switch[%llx] !", 
            neighDpid, sw->getDpid());
        return BNC_ERR;
    }

    LOG_INFO_FMT("%s[%p] discover new TOPO with dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u] ...", 
        toString(), this, sw->getDpid(), pktin.inport, neighDpid, neighPort);

    gn_port_t* port = sw->getPort(pktin.inport);
    if (NULL != port)
    {
        port->state = PORT_STATE_UP;
        port->type = PORT_TYPE_SWITCH;
        port->lldpDetectTimes = 0;
    }
    else
    {
        gn_port_t newPort = {0};
        newPort.port_no = pktin.inport;
        //can not get hw_addr from packet-in msg
        newPort.state = PORT_STATE_UP;
        newPort.type = PORT_TYPE_SWITCH;
        sw->updatePort(newPort);
    }

    port = neighborSw->getPort(neighPort);
    if (NULL != port)
    {
        port->state = PORT_STATE_UP;
        port->type = PORT_TYPE_SWITCH;
        port->lldpDetectTimes = 0;
    }
    else
    {
        gn_port_t newPort = {0};
        newPort.port_no = neighPort;
        ether_t* eth = (ether_t *)pktin.data;
        memcpy(newPort.hw_addr, eth->src, 6);
        newPort.state = PORT_STATE_UP;
        newPort.type = PORT_TYPE_SWITCH;
        neighborSw->updatePort(newPort);
    }

    CControl::getInstance()->getTopoMgr().updateLink(sw->getDpid(), pktin.inport, neighDpid, neighPort);
    CControl::getInstance()->getTopoMgr().updateLink(neighDpid, neighPort, sw->getDpid(), pktin.inport);

    return BNC_OK;
}


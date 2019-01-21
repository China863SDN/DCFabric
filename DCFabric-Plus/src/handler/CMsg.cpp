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
*   File Name   : CMsg.cpp                                                    *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CMsg.h"
#include "bnc-inet.h"
#include "bnc-error.h"
#include "log.h"
#include "CServer.h"
#include "COfMsgUtil.h"
#include "CConf.h"
#include "CControl.h"
#include "SwitchManageMode.h"
#include "SwitchManagePresistence.h"

CMsg::CMsg(INT4 sockfd, INT4 version, INT4 type, INT1* data, INT4 len):
    CMsgCommon(MSG_OPER_REALTIME),
    m_state(MSG_STATE_INIT),
    m_sockfd(sockfd),
    m_version(version),
    m_type(type),
    m_len(len),
    m_data(data)
{
}

CMsg::~CMsg()
{
    if (NULL != m_data)
    {
        CSmartPtr<COfRecvWorker> worker = CServer::getInstance()->mapOfRecvWorker(m_sockfd);
        if (worker.isNotNull())
            worker->getMemPool().release(m_data);
        else
            LOG_WARN_FMT("memory[%p] not released for none COfRecvWorker by sockfd[%d] !", m_data, m_sockfd);
        m_data = NULL;
    }
}

void CMsg::setPath()
{
    CMsgPath path;
    struct ofp_header *header = (struct ofp_header*)m_data;

    if ((OFP13_VERSION == header->version) && (OFPT13_PACKET_IN == header->type))
    {
        struct ofp13_packet_in* pktIn = (struct ofp13_packet_in *)m_data;
        UINT4 pktOfs = sizeof(struct ofp13_packet_in) + ALIGN_8(htons(pktIn->match.length)) - sizeof(struct ofpx_match) + 2;
        ether_t* eth = (ether_t*)(m_data + pktOfs);
        UINT2 ethType = ntohs(eth->proto);

		CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(m_sockfd) ;
		if((!sw.isNull())&&(SwitchManagePresistence::GetInstance()->isGlobalManageModeOn())&&(0 == sw->getManageMode()))
		{
			if(ETHER_LLDP != ethType)
			{
				path = COfMsgUtil::getMsgPath(0, header->type, ethType);
				CMsgCommon::setPath(path);
				return;
			}
		}

        if (ETHER_ARP == ethType)
        {
            arp_t* arp = (arp_t*)eth;
            path = COfMsgUtil::getMsgPath(header->version, header->type, ethType, ntohs(arp->opcode));
        }
        else if ((CConf::getInstance()->getTopoDiscoverProtocol() == TOPO_DISCOVER_IP) && (ETHER_IP == ethType))
        {
            ip_t* ip = (ip_t*)eth;
            path = COfMsgUtil::getMsgPath(header->version, header->type, ethType, ip->proto);
        }
        else
        {
            path = COfMsgUtil::getMsgPath(header->version, header->type, ethType);
        }
    }
    else
    {
        path = COfMsgUtil::getMsgPath(header->version, header->type);
    }

    CMsgCommon::setPath(path);
}

void CMsg::setKey()
{
    struct ofp_header *header = (struct ofp_header*)m_data;
    if ((OFP13_VERSION == header->version) && (OFPT13_PACKET_IN == header->type))
    {
        struct ofp13_packet_in* pktIn = (struct ofp13_packet_in *)m_data;
        UINT4 pktOfs = sizeof(struct ofp13_packet_in) + ALIGN_8(htons(pktIn->match.length)) - sizeof(struct ofpx_match) + 2;
        ether_t* eth = (ether_t*)(m_data + pktOfs);
    
        switch (ntohs(eth->proto))
        {
            case ETHER_ARP:
            {
                arp_t* arp = (arp_t*)eth;
                if (ntohs(arp->opcode) == 1)
                {
                    //OFPT13_PACKET_IN|ARP_REQUEST: sendip:targetip
                    CMsgPath key(to_string(arp->sendip));
                    key += ":";
                    key += to_string(arp->targetip);
                    CMsgCommon::setKey(key);
                }
            }
    
            case ETHER_IP:
            {
                //OFPT13_PACKET_IN|IP|ICMP: src:dest:proto:type
                //OFPT13_PACKET_IN|IP|TCP/UDP: src:dest:proto:sport:dport
                //OFPT13_PACKET_IN|IP: src:dest:proto
                ip_t* ip = (ip_t*)eth;
                CMsgPath key(to_string(ip->src));
                key += ":";
                key += to_string(ip->dest);
                key += ":";
                key += to_string(ip->proto);

                switch (ip->proto)
                {
                    case IP_ICMP:
                    {
                        icmp_t* icmp = (icmp_t*)(ip+1);
                        key += ":";
                        key += to_string(icmp->type);
                        break;
                    }
                    case IP_TCP:
                    {
                        tcp_t* tcp = (tcp_t*)(ip+1);
                        key += ":";
                        key += to_string(tcp->sport);
                        key += ":";
                        key += to_string(tcp->dport);
                        break;
                    }
                    case IP_UDP:
                    {
                        udp_t* udp = (udp_t*)(ip+1);
                        key += ":";
                        key += to_string(udp->sport);
                        key += ":";
                        key += to_string(udp->dport);
                        break;
                    }
                }
                
                CMsgCommon::setKey(key);
            }
    
            default:
                break;
        }
    }
}


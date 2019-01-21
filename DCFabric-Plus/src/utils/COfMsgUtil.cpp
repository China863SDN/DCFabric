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
*   File Name   : COfMsgUtil.cpp           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-6-22           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "log.h"
#include "bnc-error.h"
#include "bnc-param.h"
#include "bnc-type.h"
#include "bnc-inet.h"
#include "comm-util.h"
#include "CBuffer.h"
#include "CMemPool.h"
#include "COfMsgUtil.h"
#include "CControl.h"
#include "CServer.h"
#include "CSwitch.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "CFloatingIp.h"
#include "CFloatingIpMgr.h"
#include "COpenstackExternalMgr.h"
#include "CFlowDefine.h"
#include "CFlowMod.h"
#include "CFlowMgr.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "SwitchManagePresistence.h"
#include "CRouterGateMgr.h"
#include "CTopoMgr.h"
//#include "CSNatStream.h"
//#include "CSNatStreamMgr.h"
//#include "CNatHostConnMgr.h"
#include "CSNatConnMgr.h"
#include "CConf.h"

#define MAX_MSG_LEN  10240

UINT1 BROADCAST_MAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


BOOL COfMsgUtil::sendOfpMsg(INT4 fd, UINT1 version, UINT1 type, UINT4 xid)
{
    struct ofp_header ofMsg;
    ofMsg.version = version;
    ofMsg.type = type;
    ofMsg.length = htons(sizeof(ofMsg));
    ofMsg.xid = (xid != OF13_DEFAULT_XID) ? xid : randUint32();

    return sendMsgOut(fd, (INT1*)&ofMsg, sizeof(ofMsg));
}

BOOL COfMsgUtil::sendOfp13SetConfig(INT4 fd)
{
    struct ofp13_switch_config conf;
    conf.header.version = OFP13_VERSION;
    conf.header.type = OFPT13_SET_CONFIG;
    conf.header.length = htons(sizeof(conf));
    conf.header.xid = randUint32();
    conf.flags = htons(OFPC_FRAG_NORMAL);
    conf.miss_send_len = htons(1500);

    return sendMsgOut(fd, (INT1*)&conf, sizeof(conf));
}

BOOL COfMsgUtil::sendOfp13MultiPartRequest(INT4 fd, ofp_multipart_types type, void* data)
{
    INT1 buffer[MAX_MSG_LEN];
    struct ofp_multipart_request* ofp_mr = (struct ofp_multipart_request *)buffer;
    UINT2 total_len = sizeof(*ofp_mr);
    ofp_mr->header.version = OFP13_VERSION;
    ofp_mr->header.type = OFPT13_MULTIPART_REQUEST;
    //ofp_mr->header.length will be filled later
    ofp_mr->header.xid = randUint32();
    ofp_mr->type = htons(type);
    ofp_mr->flags = 0;

    switch (type)
    {
        case OFPMP_FLOW:
        {
            if (NULL == data)
            {
                LOG_WARN_FMT("sendOfp13MultiPartRequest OFPMP_FLOW, data is null !");
                break;
            }

            flow_stats_req_data_t *flow_stats_req_data = (flow_stats_req_data_t *)data;
            struct ofp13_flow_stats_request *ofp_fsr = (struct ofp13_flow_stats_request *)ofp_mr->body;
            ofp_fsr->table_id = flow_stats_req_data->table_id;
            ofp_fsr->out_port = htonl(flow_stats_req_data->out_port);
            ofp_fsr->out_group = htonl(flow_stats_req_data->out_group);
            ofp_fsr->cookie = 0x0;
            ofp_fsr->cookie_mask = 0x0;
            ofp_fsr->match.type = htons(OFPMT_OXM);
            ofp_fsr->match.length = htons(4);
            memset(ofp_fsr->pad, 0x0, 3);
            memset(ofp_fsr->pad2, 0x0, 4);

            total_len += sizeof(struct ofp13_flow_stats_request);
            break;
        }

        case OFPMP_PORT_STATS:
        {
            if (NULL == data)
            {
                LOG_WARN_FMT("sendOfp13MultiPartRequest OFPMP_PORT_STATS, data is null !");
                break;
            }

            port_stats_req_data_t *port_stats_req_data = (port_stats_req_data_t *)data;
            struct ofp13_port_stats_request *ofp_psr = (struct ofp13_port_stats_request *)ofp_mr->body;
            ofp_psr->port_no = htonl(port_stats_req_data->port_no);
            memset(ofp_psr->pad, 0x0, 4);

            total_len += sizeof(struct ofp13_port_stats_request);
            break;
        }

        default:
        {
             break;
        }
    }

    ofp_mr->header.length = htons(total_len);

    return sendMsgOut(fd, buffer, total_len);
}

BOOL COfMsgUtil::sendOfp13RoleRequest(INT4 fd, UINT4 role)
{
    struct ofp13_role_request rr = {0};
    rr.header.version = OFP13_VERSION;
    rr.header.type = OFPT13_ROLE_REQUEST;
    rr.header.length = htons(sizeof(rr));
    rr.header.xid = randUint32();
    rr.role = htonl(role);

    return sendMsgOut(fd, (INT1*)&rr, sizeof(rr));
}

void COfMsgUtil::ofp13PortConvertter(const struct ofp13_port *of_port, gn_port_t *new_sw_port)
{
    UINT4 state = ntohl(of_port->state);
    new_sw_port->state = 
        (0 == state) ? PORT_STATE_UP :
        (OFPPS13_LINK_DOWN == state) ? PORT_STATE_DOWN :
        (OFPPS13_BLOCKED == state) ? PORT_STATE_BLOCKED :
        (OFPPS13_LIVE == state) ? PORT_STATE_FAILOVER : PORT_STATE_DOWN;
    new_sw_port->port_no = ntohl(of_port->port_no);
    new_sw_port->curr = ntohl(of_port->curr);
    new_sw_port->advertised = ntohl(of_port->advertised);
    new_sw_port->supported = ntohl(of_port->supported);
    new_sw_port->peer = ntohl(of_port->peer);
    new_sw_port->config = ntohl(of_port->config);
    memcpy(new_sw_port->name, of_port->name, OFP_MAX_PORT_NAME_LEN);
    memcpy(new_sw_port->hw_addr, of_port->hw_addr, OFP_ETH_ALEN);
}

void COfMsgUtil::ofp13PortStatsConvertter(const struct ofp13_port_stats *net_stats, struct ofp13_port_stats *host_stats)
{
    host_stats->port_no = ntohl(net_stats->port_no);
    host_stats->rx_packets = ntohll(net_stats->rx_packets);
    host_stats->tx_packets = ntohll(net_stats->tx_packets);
    host_stats->rx_bytes = ntohll(net_stats->rx_bytes);
    host_stats->tx_bytes = ntohll(net_stats->tx_bytes);
    host_stats->rx_dropped = ntohll(net_stats->rx_dropped);
    host_stats->tx_dropped = ntohll(net_stats->tx_dropped);
    host_stats->rx_errors = ntohll(net_stats->rx_errors);
    host_stats->tx_errors = ntohll(net_stats->tx_errors);
    host_stats->rx_frame_err = ntohll(net_stats->rx_frame_err);
    host_stats->rx_over_err = ntohll(net_stats->rx_over_err);
    host_stats->rx_crc_err = ntohll(net_stats->rx_crc_err);
    host_stats->collisions = ntohll(net_stats->collisions);
    host_stats->duration_sec = ntohl(net_stats->duration_sec);
    host_stats->duration_nsec = ntohl(net_stats->duration_nsec);
}

void COfMsgUtil::ofpPacketInConvertter(const INT1* data, packet_in_info_t& pktin)
{
    struct ofp_header* header = (struct ofp_header*)data;
    if (OFPT13_PACKET_IN == header->type)
    {
        struct ofp13_packet_in* of_pkt = (struct ofp13_packet_in *)data;
        struct ofp_oxm_header* oxm_ptr = (struct ofp_oxm_header*)(of_pkt->match.oxm_fields);
        pktin.xid = ntohl(of_pkt->header.xid);
        pktin.buffer_id = ntohl(of_pkt->buffer_id);
        pktin.inport = ntohl(*(UINT4 *)(oxm_ptr->data));
        pktin.data_len = ntohs(of_pkt->header.length) - sizeof(struct ofp13_packet_in) -
            ALIGN_8(htons(of_pkt->match.length)) + sizeof(struct ofpx_match) - 2;
        pktin.data = getPktInData(of_pkt);
    }
    else if (OFPT_PACKET_IN == header->type)
    {
        struct ofp_packet_in* of_pkt = (struct ofp_packet_in*)data;
        pktin.xid = ntohl(of_pkt->header.xid);
        pktin.buffer_id = ntohl(of_pkt->buffer_id);
        pktin.inport = (UINT4)ntohs(of_pkt->in_port);
        pktin.data_len = ntohs(of_pkt->header.length) - offsetof(struct ofp_packet_in, data);
        pktin.data = of_pkt->data;
    }
}

BOOL COfMsgUtil::sendOfp13PacketOut(INT4 fd, packout_req_info_t& pktout)
{
    INT1 buffer[MAX_MSG_LEN];
    struct ofp13_packet_out* of13_out = (struct ofp13_packet_out*)buffer;
    memset(of13_out, 0, sizeof(struct ofp13_packet_out));
    of13_out->header.version = OFP13_VERSION;
    of13_out->header.type = OFPT13_PACKET_OUT;

    UINT2 total_len = sizeof(struct ofp13_packet_out) + sizeof(struct ofp13_action_output);
    if (OFP_NO_BUFFER == pktout.buffer_id)
        total_len += pktout.data_len;
    of13_out->header.length = htons(total_len);
    of13_out->header.xid = (pktout.xid != OF13_DEFAULT_XID) ? pktout.xid : randUint32();
    of13_out->buffer_id = htonl(pktout.buffer_id);
    of13_out->in_port = htonl(pktout.inport);
    of13_out->actions_len = htons(sizeof(struct ofp13_action_output));

    struct ofp13_action_output *ofp13_act = (struct ofp13_action_output *)of13_out->actions;
    ofp13_act->type = htons(OFPAT13_OUTPUT);
    ofp13_act->len = htons(sizeof(struct ofp13_action_output));
    ofp13_act->port = htonl(pktout.outport);
    ofp13_act->max_len = htons(pktout.max_len);
    memset(ofp13_act->pad, 0x0, 6);

    if ((OFP_NO_BUFFER == pktout.buffer_id) && (0 < pktout.data_len) && (NULL != pktout.data))
        memcpy(ofp13_act->pad+6, pktout.data, pktout.data_len);

    return sendMsgOut(fd, buffer, total_len);
}

BOOL COfMsgUtil::sendOfp13TableMiss(INT4 fd)
{
    INT1 buffer[MAX_MSG_LEN];
    UINT2 total_len = sizeof(struct ofp13_flow_mod) + ALIGN_8(sizeof(struct ofp_instruction)) + sizeof(struct ofp13_action_output);
    struct ofp13_flow_mod *ofm = (struct ofp13_flow_mod *)buffer;
    ofm->header.version = OFP13_VERSION;
    ofm->header.type = OFPT13_FLOW_MOD;
    ofm->header.length = htons(total_len);
    ofm->header.xid = randUint32();
    ofm->cookie = 0x0;
    ofm->cookie_mask = 0x0;
    ofm->table_id = 0x0;
    ofm->command = OFPFC_ADD;
    ofm->idle_timeout = htons(0x0);
    ofm->hard_timeout = htons(0x0);
    ofm->priority = htons(0x0);
    ofm->buffer_id = htonl(OFP_NO_BUFFER);
    ofm->out_port = htonl(0xffffffff);
    ofm->out_group = htonl(0xffffffff);
    ofm->flags = htons(0x0);
    ofm->pad[0] = 0x0;
    ofm->pad[1] = 0x0;

    ofm->match.type = htons(OFPMT_OXM);
    ofm->match.length = htons(4);

    struct ofp_instruction *oin = (struct ofp_instruction *)(ofm->match.oxm_fields + 4);
    oin->type = htons(OFPIT_APPLY_ACTIONS);
    oin->len  = ntohs(0x18);

    struct ofp13_action_output *ofp13_ao = (struct ofp13_action_output *)((UINT1 *)oin + ALIGN_8(sizeof(struct ofp_instruction)));
    ofp13_ao->type    = htons(OFPAT13_OUTPUT);
    ofp13_ao->len     = htons(16);
    ofp13_ao->port    = htonl(OFPP13_CONTROLLER);
    ofp13_ao->max_len = htons(0xffff);

    return sendMsgOut(fd, buffer, total_len);
}


BOOL COfMsgUtil::sendOfp13Lldp(CSmartPtr<CSwitch>& sw, const gn_port_t& port)
{
    if ((port.state != PORT_STATE_UP) || (sw->getDpid() == 0))
        return TRUE;

    packout_req_info_t packout_req_info;
    packout_req_info.buffer_id = OFP_NO_BUFFER;
    packout_req_info.inport = OFPP13_CONTROLLER;
    packout_req_info.outport = port.port_no;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = OF13_DEFAULT_XID;

    UINT1 data[1500];
    if (CConf::getInstance()->getTopoDiscoverProtocol() == TOPO_DISCOVER_LLDP)
    {
        lldp_t* lldp_pkt = (lldp_t*)data;
        packout_req_info.data_len = sizeof(lldp_t);
        packout_req_info.data = (UINT1*)lldp_pkt;
        fillLldpPkt(sw->getDpid(), port.hw_addr, port.port_no, lldp_pkt);
    }
    else
    {
        packout_req_info.data_len = sizeof(ip_t)+sizeof(UINT8)+sizeof(UINT4);
        packout_req_info.data = data;
        fillDiscoveryIpPkt(sw->getDpid(), port.port_no, data);
    }

    return COfMsgUtil::sendOfp13PacketOut(sw->getSockfd(), packout_req_info);
}

BOOL COfMsgUtil::sendOfp13ArpRequest(UINT4 srcIp, UINT4 destIp,const UINT1* srcMac)
{
    //UINT1 arp_zero_mac[] = {0x0,0x0,0x0,0x0,0x0,0x0};
    UINT1 arp_broadcat_mac[] = {0xff,0xff,0xff,0xff,0xff,0xff};

    arp_t arp_pkt;

    createArpPacket(&arp_pkt, 1, srcMac, arp_broadcat_mac, srcIp, destIp);
    ofp13floodInternal(sizeof(arp_pkt), &arp_pkt);
    return TRUE;
}

void COfMsgUtil::forward(const CSmartPtr<CSwitch> & sw, UINT4 port_no,
                                 packet_in_info_t* packetin)
{
	LOG_IF_VOID (sw.isNull() || (0 == port_no) || (NULL == packetin),
			     "fail to forward packet. input parameter invalid.");

	packout_req_info_t packout_req_info;

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.outport = port_no;
    packout_req_info.max_len = 0xff;
	packout_req_info.xid = 0;
	packout_req_info.data_len = packetin->data_len;
	packout_req_info.data = (UINT1 *)packetin->data;

	if (SW_STATE_STABLE ==sw->getState())
		COfMsgUtil::sendOfp13PacketOut(sw->getSockfd(), packout_req_info);
}

void COfMsgUtil::forward(const CSmartPtr<CSwitch> & sw, UINT4 port_no,
                                 INT4 data_len, void* data)
{
    LOG_IF_VOID (sw.isNull() || (0 == port_no),
                     "fail to forward packet. input parameter invalid.");

	packout_req_info_t packout_req_info;

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.outport = port_no;
    packout_req_info.max_len = 0xff;
	packout_req_info.xid = 0;
	packout_req_info.data_len = data_len + sizeof(packout_req_info_t);
	packout_req_info.data = (UINT1 *)data;

	if (SW_STATE_STABLE == sw->getState())
		COfMsgUtil::sendOfp13PacketOut(sw->getSockfd(), packout_req_info);
}
void COfMsgUtil::ofp13floodInternal(UINT4 datalen, void* data)
{
    LOG_IF_VOID (((NULL == data)||(0 == datalen)),
                        "fail to forward packet. input parameter invalid.");
    packout_req_info_t packout_req_info;

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = OFPP13_CONTROLLER;
    // packout_req_info.outport = port_no;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = 0;
    packout_req_info.data_len = datalen;
    packout_req_info.data = (UINT1 *)data;
	
	CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    STL_FOR_LOOP(map, iter)
    {
		CSmartPtr<CSwitch> sw = iter->second;
        if (sw.isNull())
            continue;

		UINT8 dpid = sw->getDpid();
		if ((CControl::getInstance()->isL3ModeOn())
			&& (COpenstackExternalMgr::getInstance()->isExternalSwitch(dpid)))
        {
            continue;
        }
					
        sw->lockPorts();
        CPortMap& ports = sw->getPorts();

        STL_FOR_LOOP(ports, portIter)
        {
            if((ports.end() != portIter)&&(PORT_TYPE_SWITCH == portIter->second.type))
            {
				BOOL  bMonitorFlag = FALSE;
				CSmartPtr<CSwitch> neighbor_sw(NULL);
            	neighbor_t* neigbor = CControl::getInstance()->getTopoMgr().getLink(sw->getDpid(), portIter->second.port_no);
				if(NULL != neigbor)
				{
					neighbor_sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(neigbor->dst_dpid);
					if(neighbor_sw.isNotNull()&&(1 == sw->getManageMode())&&(0 == neighbor_sw->getManageMode()))
					{
						bMonitorFlag = TRUE;
						
						//LOG_ERROR_FMT("sw port=%d  neigbor->src_port=%d",portIter->second.port_no,neigbor->src_port);
						//LOG_ERROR_FMT("sw dpid=0x%llx neigbor->srcdpid = 0x%llx neigbor->dstdpid = 0x%llx",
                         //   sw->getDpid(),neigbor->src_dpid, neigbor->dst_dpid);
					}
					if(neigbor->dst_dpid&&(neigbor->dst_dpid == SwitchManagePresistence::GetInstance()->getCommentSwitchDpid()))
					{
						LOG_ERROR_FMT("sw dpid=0x%llx neigbor->srcdpid = 0x%llx neigbor->dstdpid = 0x%llx",sw->getDpid(),neigbor->src_dpid, neigbor->dst_dpid);
						continue;
					}
				}
				if(FALSE  == bMonitorFlag)
                	continue;
            }

            

         //  if ((dpid == sw->getDpid())
          //      && (portIter->port_no == packetin->inport))
         //   {
         //       continue;
         //  }

            
			if(ports.end() != portIter)
			{
            	packout_req_info.outport = portIter->second.port_no;
			}
		    if (SW_STATE_STABLE == sw->getState())
			    COfMsgUtil::sendOfp13PacketOut(sw->getSockfd(), packout_req_info);
            //COfMsgUtil::ofMsgPacketOut(*iter, iter->getPtr()->getSockFd(),
             //                          (UINT1*)&packout_req_info);
        }

        sw->unlockPorts();
    }

	CControl::getInstance()->getSwitchMgr().unlock();
}

void COfMsgUtil::ofp13floodInternal(packet_in_info_t* packetin)
{
    LOG_IF_VOID ((NULL == packetin),
                        "fail to forward packet. input parameter invalid.");

	//LOG_ERROR_FMT("%s %d datalen=%d xid=%d",FN,LN,packetin->data_len, packetin->xid);
    packout_req_info_t packout_req_info;

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = OFPP13_CONTROLLER;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = 0;
    packout_req_info.data_len = packetin->data_len;
    packout_req_info.data = (UINT1 *)packetin->data;

	//arp_t* pkt = (arp_t*)packetin->data;
	//if(pkt)
	//	LOG_ERROR_FMT(" pkt->srcIp= 0x%x pkt->dstIp= 0x%x packetin->data_len=%d",pkt->sendip, pkt->targetip, packetin->data_len);
	
    CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    STL_FOR_LOOP(map, iter)
    {
        CSmartPtr<CSwitch> sw = iter->second;
        if (sw.isNull())
            continue;

        sw->lockPorts();
        CPortMap& ports = sw->getPorts();
		
		//LOG_ERROR_FMT("sw->sw_ip=0x%x ",sw->getSwIp());
        STL_FOR_LOOP(ports, portIter)
        {
            UINT8 dpid = sw->getDpid();

			if ((CControl::getInstance()->isL3ModeOn())
            	&& (COpenstackExternalMgr::getInstance()->isExternalSwitch(dpid)))
            {
                continue;
            }
				
            if((ports.end() != portIter)&&(PORT_TYPE_SWITCH == portIter->second.type))
            {
				BOOL  bMonitorFlag = FALSE;
				CSmartPtr<CSwitch> neighbor_sw(NULL);
            	neighbor_t* neigbor = CControl::getInstance()->getTopoMgr().getLink(sw->getDpid(), portIter->second.port_no);
				if(NULL != neigbor)
				{
					neighbor_sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(neigbor->dst_dpid);
					if((0 != sw->getManageMode())&&neighbor_sw.isNotNull()&&(0 == neighbor_sw->getManageMode()))
					{
						bMonitorFlag = TRUE;
						
						//LOG_ERROR_FMT("sw port=%d  neigbor->src_port=%d",portIter->second.port_no,neigbor->src_port);
						//LOG_ERROR_FMT("sw dpid=0x%x neigbor->srcdpid = 0x%x neigbor->dstdpid = 0x%x getCommentSwitchDpid=0x%x", 
						//	sw->getDpid(),neigbor->src_dpid, neigbor->dst_dpid,SwitchManagePresistence::GetInstance()->getCommentSwitchDpid());
					}
					if(neigbor->dst_dpid&&(neigbor->dst_dpid == SwitchManagePresistence::GetInstance()->getCommentSwitchDpid()))
					{
						continue;
					}
				}
				if(FALSE  == bMonitorFlag)
                	continue;
            }


         //  if ((dpid == sw->getDpid())
          //      && (portIter->port_no == packetin->inport))
         //   {
         //       continue;
         //  }

          
			if(ports.end() != portIter)
			{
            	packout_req_info.outport = portIter->second.port_no;
			}

			COfMsgUtil::sendOfp13PacketOut(sw->getSockfd(), packout_req_info);
            //COfMsgUtil::ofMsgPacketOut(*iter, iter->getPtr()->getSockFd(),
             //                          (UINT1*)&packout_req_info);
        }

        sw->unlockPorts();
    }

	CControl::getInstance()->getSwitchMgr().unlock();
}
BOOL COfMsgUtil::createArpPacket(arp_t* arp_pkt, UINT2 code,
		                               const UINT1* src_mac, const UINT1* dst_mac,
		                               UINT4 src_ip, UINT4 dst_ip)
{
	LOG_IF_RETURN ((NULL == arp_pkt) || (NULL == src_mac) || (NULL == dst_mac),
				   FALSE, "create arp packet failed. pointer is NULL.");

	memset(arp_pkt, sizeof(arp_t), 0);

	memcpy(arp_pkt->eth_head.src, src_mac, 6);
	memcpy(arp_pkt->eth_head.dest, dst_mac, 6);
	arp_pkt->eth_head.proto = htons(ETHER_ARP);
	arp_pkt->hardwaretype = htons(1);
	arp_pkt->prototype = htons(ETHER_IP);
	arp_pkt->hardwaresize = 0x6;
	arp_pkt->protocolsize = 0x4;
	arp_pkt->opcode = htons(code);
	arp_pkt->sendip = src_ip;
	arp_pkt->targetip= dst_ip;
	memcpy(arp_pkt->sendmac, src_mac, 6);
	memcpy(arp_pkt->targetmac, dst_mac, 6);

	return TRUE;
}

BOOL COfMsgUtil::createArpReplyPacket(arp_t* arp_pkt, CSmartPtr<CHost>& src, CSmartPtr<CHost>& dst)
{
	LOG_IF_RETURN ((NULL == arp_pkt) || (src.isNull()) || (dst.isNull()),
			       FALSE, "create arp reply packet failed. pointer is NULL.");
	CSmartPtr<CHost> srcGateway = CHostMgr::getInstance()->getHostGateway(src);
	CSmartPtr<CHost> dstGateway = CHostMgr::getInstance()->getHostGateway(dst);
	
	LOG_DEBUG_FMT("srcGateway=0x%p dstGateway=0x%p dstGateway->getMac()=0x%p dst->getIp()=0x%x CControl::getInstance()->isL3modeOn()=%d",
        srcGateway.getPtr(), dstGateway.getPtr(), dstGateway->getMac(),dst->getIp(),CControl::getInstance()->isL3ModeOn());
	if(CControl::getInstance()->isL3ModeOn())
	{
		if(srcGateway.isNotNull()&&(srcGateway != dstGateway))
		{
			createArpPacket(arp_pkt, 2,	srcGateway->getMac(), src->getMac(), dst->getIp(), src->getIp());
		}
		else
		{
			createArpPacket(arp_pkt, 2,	dst->getMac(), src->getMac(), dst->getIp(), src->getIp());
		}
	}
	else
	{
		createArpPacket(arp_pkt, 2,	dst->getMac(), src->getMac(), dst->getIp(), src->getIp());
	}

	return TRUE;
}

BOOL COfMsgUtil::createArpReplyPacket(arp_t* arp_pkt, CSmartPtr<CHost>& dst,arp_t* pkt)
{
    LOG_IF_RETURN ((NULL == arp_pkt) || (dst.isNull()) || (NULL == pkt),
                   FALSE, "create arp reply packet failed. pointer is NULL.");
    createArpPacket(arp_pkt, 2, dst->getMac(), pkt->sendmac, dst->getIp(), pkt->sendip);
    return TRUE;
}

BOOL COfMsgUtil::createArpRequestFloodPacket(arp_t* arp_pkt, CSmartPtr<CHost>& src, UINT4 dst_ip)
{
	LOG_IF_RETURN ((NULL == arp_pkt) || (src.isNull()),
			       FALSE, "create arp reply packet failed. pointer is NULL.");

	createArpPacket(arp_pkt, 1,	src->getMac(), BROADCAST_MAC, src->getIp(), dst_ip);

	return TRUE;
}

void COfMsgUtil::fillLldpPkt(UINT8 dpid, const UINT1 *mac, UINT4 port, lldp_t *lldp)
{
	//组播地址01 80 c2 00 00 0e
    UINT1 dest[6] = {0x00,0x80,0xc2,0x00,0x00,0x0e};
    memcpy(lldp->eth_head.dest, dest, 6);
    memcpy(lldp->eth_head.src, mac, 6);
    lldp->eth_head.proto = htons(ETHER_LLDP);

	//chassis_tlv
    lldp->chassis_tlv_type_and_len = htons(0x0209);
    lldp->chassis_tlv_subtype = LLDP_CHASSIS_ID_LOCALLY_ASSIGNED;
    lldp->chassis_tlv_id = htonll(dpid);   
	//port_tlv
    lldp->port_tlv_type_and_len    = htons(0x0403);
    lldp->port_tlv_subtype = LLDP_PORT_ID_COMPONENT;
    lldp->port_tlv_id = htons(port);        
	//lldp
    lldp->ttl_tlv_type_and_len = htons(0x0602);
    lldp->ttl_tlv_ttl = htons(120);

    lldp->endof_lldpdu_tlv_type_and_len = 0x00;
}

void COfMsgUtil::fillDiscoveryIpPkt(UINT8 dpid, UINT4 port, UINT1 *data)
{
    UINT1* ptr = data;

    ip_t* ip_pkt = (ip_t*)ptr;
    ptr += sizeof(ip_t);

	UINT1 dest[6] = {0x00,0x80,0xc2,0x00,0x00,0x0e};
	memset(ip_pkt, sizeof(ip_t), 0);
	memcpy(ip_pkt->eth_head.src, CServer::getInstance()->getControllerMac(), 6);
	//memcpy(ip_pkt->eth_head.dest, CServer::getInstance()->getControllerMac(), 6);
	
	memcpy(ip_pkt->eth_head.dest, dest, 6);
	ip_pkt->eth_head.proto = htons(ETHER_IP);
	ip_pkt->hlen = 0x45;
	ip_pkt->len = htons(sizeof(ip_t)+sizeof(UINT8)+sizeof(UINT4));
	ip_pkt->ttl = 255;
	ip_pkt->proto = IP_INTERNAL;
	ip_pkt->src = htonl(CServer::getInstance()->getControllerIp());
	ip_pkt->dest = htonl(CServer::getInstance()->getControllerIp());

    *(UINT8*)ptr = dpid;
    ptr += sizeof(UINT8);
    *(UINT4*)ptr = port;
}

UINT1* COfMsgUtil::getPktInData(struct ofp13_packet_in *pktIn)
{
    UINT4 matchLen = ALIGN_8(htons(pktIn->match.length));
    matchLen -= sizeof(pktIn->match);
    UINT4 pktOfs = sizeof(*pktIn) + matchLen + 2;
    return ((UINT1*)pktIn + pktOfs);
}


INT4 COfMsgUtil::remove_nat_flow(const CSmartPtr<CSwitch>& sw, UINT4 ip, UINT1* mac)
{	
	if(sw.isNull())
		return BNC_ERR;

	Base_External *baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(ip);
	if(NULL == baseExternalPort)
	{
		LOG_ERROR(" Can't get external port!!!");
		return BNC_ERR;
	}
	CSmartPtr<CSwitch> extSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
    if (extSw.isNull())
    {
        LOG_WARN_FMT("Can not find external switch via dpid[%llx] !", baseExternalPort->get_switch_DPID());
        return BNC_ERR;
    }

	CSNatConnsExtMap& extMap = CSNatConnMgr::getInstance()->getExtMap();
	STL_FOR_LOOP(extMap, extIt)
    {
        CSNatConnsExt& ext = extIt->second;
        CSNatConnsIpMap::iterator hostIt = ext.m_tcpHosts.find(ip);
        if (hostIt != ext.m_tcpHosts.end())
        {
            CSNatConnPortMap& portMap = hostIt->second;
            STL_FOR_LOOP(portMap, portIt)
            {
                CSmartPtr<CSNatConn> snatConn = portIt->second;
                if (snatConn.isNotNull())
                {
                    CFlowMgr::getInstance()->delete_fabric_input_flow_by_srcmac_proto(sw, mac, snatConn->getExternalIp(), snatConn->getInternalPort(), IPPROTO_TCP);
                    CFlowMgr::getInstance()->delete_fabric_input_flow_by_dstmac_proto(extSw, mac, snatConn->getExternalIp(), snatConn->getSNatPort(), IPPROTO_TCP);
                }
            }    
        }    

        hostIt = ext.m_udpHosts.find(ip);
        if (hostIt != ext.m_udpHosts.end())
        {
            CSNatConnPortMap& portMap = hostIt->second;
            STL_FOR_LOOP(portMap, portIt)
            {
                CSmartPtr<CSNatConn> snatConn = portIt->second;
                if (snatConn.isNotNull())
                {
                    CFlowMgr::getInstance()->delete_fabric_input_flow_by_srcmac_proto(sw, mac, snatConn->getExternalIp(), snatConn->getInternalPort(), IPPROTO_UDP);
                    CFlowMgr::getInstance()->delete_fabric_input_flow_by_dstmac_proto(extSw, mac, snatConn->getExternalIp(), snatConn->getSNatPort(), IPPROTO_UDP);
                }
            }    
        }    
    }

#if 0
	CSNatStreamMgr* natstreamMgr = CNatHostConnMgr::getInstance()->FindNatConnectStream(ip, IPPROTO_TCP);
	if(NULL == natstreamMgr)
	{
		return BNC_ERR;
	}
	STL_FOR_MAP(natstreamMgr->getNatStreamHead(), iter)
	{
		if(NULL != *iter)
		{
			CFlowMgr::getInstance()->delete_fabric_input_flow_by_srcmac_proto(sw, mac, (*iter)->getExternalIp(), (*iter)->getInternalPortNo(), IPPROTO_TCP);
			CFlowMgr::getInstance()->delete_fabric_input_flow_by_dstmac_proto(extSw, mac, (*iter)->getExternalIp(), (*iter)->getExternalPortNo(),IPPROTO_TCP);
		}
	}
	natstreamMgr = CNatHostConnMgr::getInstance()->FindNatConnectStream(ip, IPPROTO_UDP);
	if(NULL == natstreamMgr)
	{
		return BNC_ERR;
	}
	STL_FOR_MAP(natstreamMgr->getNatStreamHead(), iter)
	{
		if(NULL != *iter)
		{
			CFlowMgr::getInstance()->delete_fabric_input_flow_by_srcmac_proto(sw, mac, (*iter)->getExternalIp(), (*iter)->getInternalPortNo(), IPPROTO_UDP);
			CFlowMgr::getInstance()->delete_fabric_input_flow_by_dstmac_proto(extSw, mac, (*iter)->getExternalIp(), (*iter)->getExternalPortNo(),IPPROTO_UDP);
		}
	}
#endif

	return BNC_OK;
}

//by:根据sw,floating_ip,删除相关的流删除Pica8上的浮动IP有关流表)
INT4 COfMsgUtil::remove_floating_flow(UINT4 fixed_ip, UINT4 floating_ip, UINT1* mac)
{
	std::list<Base_External *>  baseExternalPortList;
	Base_External *baseExternalPort(NULL);

	baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(fixed_ip);
	if(NULL == baseExternalPort)
	{
		LOG_ERROR(" Can't get external port!!!");
		return BNC_ERR;
	}
	
	CSmartPtr<CSwitch> ext_sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
	CFlowMgr::getInstance()->delete_fabric_flow_by_ip(ext_sw, floating_ip, FABRIC_TABLE_FORWARD_INVM);
	CFlowMgr::getInstance()->delete_fabric_flow_by_mac(ext_sw, mac, FABRIC_TABLE_FORWARD_INVM);
	return BNC_OK;
}

INT4 COfMsgUtil::remove_host_output_flow_by_ip_mac(const CSmartPtr<CSwitch>& sw, UINT4 ip, UINT1* mac)
{
	LOG_INFO_FMT( "------------------ ip=0x%x",ip);
	if (0 != ip) 
	{
		CFlowMgr::getInstance()->delete_fabric_flow_by_ip(sw, ip, FABRIC_TABLE_FORWARD_INVM);
		//删除虚机内部IP的入口防火墙流表
		//install_remove_FirewallIn_flow(sw,ip);
		//删除虚机内部IP的出口防火墙流表
		//install_remove_FirewallOut_flow(sw,ip);
	}

	if (NULL != mac) 
	{
		
		CFlowMgr::getInstance()->delete_fabric_flow_by_mac(sw, mac, FABRIC_TABLE_INPUT);
		//CFlowMgr::getInstance()->delete_fabric_flow_by_mac(sw, mac, FABRIC_FQ_OUT_POST_PROCESS_TABLE);
		CFlowMgr::getInstance()->delete_fabric_flow_by_mac(sw, mac, FABRIC_TABLE_FORWARD_INVM);
	}
	return BNC_OK;
}


INT4 COfMsgUtil::remove_flows_by_sw_port(UINT8 sw_dpid, UINT4 port)
{
	
	LOG_INFO_FMT("sw_dpid=0x%llx port=%u",sw_dpid,port);
	CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByDpidAndPort(sw_dpid, port);
	if(host.isNull())
	{
		return BNC_ERR;
	}
	CSmartPtr<CSwitch> sw = host->getSw();
	
	//host->setSw(CSmartPtr<CSwitch>(NULL));
	host->setPortNo(0);
	
	CFloatingIp* efp = CFloatingIpMgr::getInstance()->findFloatingIpNodeByfixedIp(host->getfixIp());
	if(NULL != efp)
	{
		remove_floating_flow(efp->getFixedIp(), efp->getFloatingIp(), host->getMac());
		CFlowMgr::getInstance()->delete_fabric_flow_by_ip(sw, efp->getFloatingIp(), FABRIC_TABLE_FORWARD_INVM);
	}
	else
	{
		remove_nat_flow(sw, host->getfixIp(), host->getMac());
	}
	if (sw.isNotNull()) 
	{
		remove_host_output_flow_by_ip_mac(sw, host->getfixIp(), host->getMac());
		
	    //CFlowMgr::getInstance()->install_ip_controller_flow(sw, host->getfixIp(), FABRIC_TABLE_FORWARD_INVM);
	}
	return BNC_OK;
}


typedef struct MsgPath {
    INT4 msgType;
    CMsgPath path;
}MsgPath_t;
static const MsgPath_t g_of10MsgPath[] = {
    {OFPT_HELLO,                    "1,0"},
    {OFPT_ERROR,                    "1,1"},
    {OFPT_ECHO_REQUEST,             "1,2"},
    {OFPT_ECHO_REPLY,               "1,3"},
    {OFPT_VENDOR,                   "1,4"},
    {OFPT_FEATURES_REQUEST,         "1,5"},
    {OFPT_FEATURES_REPLY,           "1,6"},
    {OFPT_GET_CONFIG_REQUEST,       "1,7"},
    {OFPT_GET_CONFIG_REPLY,         "1,8"},
    {OFPT_SET_CONFIG,               "1,9"},
    {OFPT_PACKET_IN,                "1,10"},
    {OFPT_FLOW_REMOVED,             "1,11"},
    {OFPT_PORT_STATUS,              "1,12"},
    {OFPT_PACKET_OUT,               "1,13"},
    {OFPT_FLOW_MOD,                 "1,14"},
    {OFPT_PORT_MOD,                 "1,15"},
    {OFPT_STATS_REQUEST,            "1,16"},
    {OFPT_STATS_REPLY,              "1,17"},
    {OFPT_BARRIER_REQUEST,          "1,18"},
    {OFPT_BARRIER_REPLY,            "1,19"},
    {OFPT_QUEUE_GET_CONFIG_REQUEST, "1,20"},
    {OFPT_QUEUE_GET_CONFIG_REPLY,   "1,21"},
};
static const MsgPath_t g_of13MsgPath[] = {
    {OFPT13_HELLO,                    "4,0"},
    {OFPT13_ERROR,                    "4,1"},
    {OFPT13_ECHO_REQUEST,             "4,2"},
    {OFPT13_ECHO_REPLY,               "4,3"},
    {OFPT13_EXPERIMENTER,             "4,4"},
    {OFPT13_FEATURES_REQUEST,         "4,5"},
    {OFPT13_FEATURES_REPLY,           "4,6"},
    {OFPT13_GET_CONFIG_REQUEST,       "4,7"},
    {OFPT13_GET_CONFIG_REPLY,         "4,8"},
    {OFPT13_SET_CONFIG,               "4,9"},
    {OFPT13_PACKET_IN,                "4,10"},
    {OFPT13_FLOW_REMOVED,             "4,11"},
    {OFPT13_PORT_STATUS,              "4,12"},
    {OFPT13_PACKET_OUT,               "4,13"},
    {OFPT13_FLOW_MOD,                 "4,14"},
    {OFPT13_GROUP_MOD,                "4,15"},
    {OFPT13_PORT_MOD,                 "4,16"},
    {OFPT13_TABLE_MOD,                "4,17"},
    {OFPT13_MULTIPART_REQUEST,        "4,18"},
    {OFPT13_MULTIPART_REPLY,          "4,19"},
    {OFPT13_BARRIER_REQUEST,          "4,20"},
    {OFPT13_BARRIER_REPLY,            "4,21"},
    {OFPT13_QUEUE_GET_CONFIG_REQUEST, "4,22"},
    {OFPT13_QUEUE_GET_CONFIG_REPLY,   "4,23"},
    {OFPT13_ROLE_REQUEST,             "4,24"},
    {OFPT13_ROLE_REPLY,               "4,25"},
    {OFPT13_GET_ASYNC_REQUEST,        "4,26"},
    {OFPT13_GET_ASYNC_REPLY,          "4,27"},
    {OFPT13_SET_ASYNC,                "4,28"},
    {OFPT13_METER_MOD,                "4,29"},
};
static const INT4 g_of10MsgTypeSupported  = sizeof(g_of10MsgPath) / sizeof(g_of10MsgPath[0]);
static const INT4 g_of13MsgTypeSupported  = sizeof(g_of13MsgPath) / sizeof(g_of13MsgPath[0]);
const CMsgPath g_of13MsgPathLldp   = "4,10,88cc";
const CMsgPath g_of13MsgPathIpInternal = "4,10,0800,ff";
const CMsgPath g_of13MsgPathArpReq = "4,10,0806,1";
const CMsgPath g_of13MsgPathArpRsp = "4,10,0806,2";
const CMsgPath g_of13MsgPathIp     = "4,10,0800";
const CMsgPath g_of13MsgPathIpv6   = "4,10,86DD";
const CMsgPath g_of13MsgPathVlan   = "4,10,8100";
const CMsgPath g_of13MsgPathMpls   = "4,10,8847";

std::string COfMsgUtil::getMsgPath(INT4 version, INT4 msgType, INT4 ethType, INT4 subType)
{
    if (OFP10_VERSION == version)
    {
        if (msgType < g_of10MsgTypeSupported)
            return g_of10MsgPath[msgType].path;
    }
    else if (OFP13_VERSION == version)
    {
        if (msgType < g_of13MsgTypeSupported)
        {
            if (OFPT13_PACKET_IN != msgType)
                return g_of13MsgPath[msgType].path;
            else
                switch (ethType)
                {
                    case ETHER_LLDP:
                        if (CConf::getInstance()->getTopoDiscoverProtocol() == TOPO_DISCOVER_LLDP)
                            return g_of13MsgPathLldp;
                        break;
                    case ETHER_ARP:
                        return (1 == subType) ? g_of13MsgPathArpReq : g_of13MsgPathArpRsp;
                    case ETHER_IP:
                        if ((CConf::getInstance()->getTopoDiscoverProtocol() == TOPO_DISCOVER_IP)
                            && (IP_INTERNAL == subType))
                            return g_of13MsgPathIpInternal;
                        return g_of13MsgPathIp;
                    case ETHER_IPV6:
                        return g_of13MsgPathIpv6;
                    case ETHER_VLAN:
                        return g_of13MsgPathVlan;
                    case ETHER_MPLS:
                        return g_of13MsgPathMpls;
                    default:
                        break;
                }
        }
    }
    else
    {
        //LOG_WARN_FMT("only support OFP10_VERSION(1)|OFP13_VERSION(4), but version is %d!", version);
    }

    return "";
}


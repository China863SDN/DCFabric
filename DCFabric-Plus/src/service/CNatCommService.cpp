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
*   File Name   : CNatCommService.h           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "CControl.h"
#include "CFlowMgr.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "CRouter.h"
#include "CRouterGateMgr.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "CNatCommService.h"
#include "CSNatConnMgr.h"

INT4 CNatCommService::IpHandler(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
		 ip_t* pkt, packet_in_info_t* packetIn)
{
	UINT4 packetin_inport = packetIn->inport;
	ip_t* packetin_ip = (ip_t*)(packetIn->data);
	UINT4 packetin_src_ip = packetin_ip->src;
	UINT4 packetin_dst_ip = packetin_ip->dest;
	UINT2 packetin_src_port = 0;
	UINT2 packetin_dst_port = 0;
	UINT1 packetin_proto = 0;
	UINT1 packetin_src_mac[MAC_LEN] = {0};
    memcpy(packetin_src_mac, packetin_ip->eth_head.src, MAC_LEN);

	if (IPPROTO_TCP == packetin_ip->proto) {
		tcp_t* packetin_tcp = (tcp_t*)(packetin_ip->data);
		packetin_proto = IPPROTO_TCP;
		packetin_src_port = packetin_tcp->sport;
		packetin_dst_port = packetin_tcp->dport;
	}
	else if (IPPROTO_UDP == packetin_ip->proto)
	{
		udp_t* packetin_udp = (udp_t*)(packetin_ip->data);
		packetin_proto = IPPROTO_UDP;
		packetin_src_port = packetin_udp->sport;
		packetin_dst_port = packetin_udp->dport;
	}
	else 
    {
		return BNC_ERR;
	}
		
	BOOL from_inside = TRUE;
	if ((dstHost.isNotNull()&&(bnc::host::HOST_GATEWAY == dstHost->getHostType()))
		&&(srcHost.isNull()||(bnc::host::HOST_EXTERNAL== srcHost->getHostType())))
	{
		from_inside = FALSE;
	}

    INT1 str[48] = {0};
	UINT2 external_port_no = 0; 
	Base_External *baseExternalPort(NULL);
	CSmartPtr<CSwitch> gatewaysw(NULL);
	CSmartPtr<CSwitch> src_sw(srcSw);

	if (from_inside)
	{
        baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(packetin_src_ip);
        if(NULL == baseExternalPort)
        {
            LOG_WARN_FMT("SNAT: can't find baseExternalPort by host[%s] !", number2ip(packetin_src_ip, str));
            return BNC_ERR;
        }
        //LOG_WARN_FMT("SNAT: baseExternalPort->get_switch_DPID()=0x%llx!",baseExternalPort->get_switch_DPID());
        gatewaysw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(baseExternalPort->get_switch_DPID());
        if (gatewaysw.isNull())
        {
            LOG_WARN_FMT("SNAT: from inside to external gatewaysw[0x%llx] is NULL!", baseExternalPort->get_switch_DPID());
            return BNC_ERR;
        }

		CSmartPtr<CSNatConn> snatConn = CSNatConnMgr::getInstance()->findSNatConnByInt(
            packetin_dst_ip, packetin_src_ip, ntohs(packetin_src_port), packetin_proto);
		if (snatConn.isNull())
		{
            snatConn = CSNatConnMgr::getInstance()->createSNatConn(packetin_dst_ip, packetin_src_ip, ntohs(packetin_src_port), 
                packetin_proto, packetin_src_mac, baseExternalPort->get_switch_DPID(), srcSw->getDpid());
            if (snatConn.isNull())
        		return BNC_ERR;
        }
        external_port_no = snatConn->getSNatPort();
	}
	else
	{
		CSmartPtr<CSNatConn> snatConn = CSNatConnMgr::getInstance()->findSNatConnByExt(
            packetin_src_ip, packetin_dst_ip, ntohs(packetin_dst_port), packetin_proto);
		if (snatConn.isNull())
		{
			LOG_WARN("SNAT: from external to inside , can't find CSNatConn!");
			return BNC_ERR;
		}
		CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(snatConn->getInternalMac());
		if(host.isNull())
		{
			LOG_WARN_FMT("SNAT: host[%s] is NULL!", mac2str(snatConn->getInternalMac(), str));
			return BNC_ERR;
		}
		memcpy(packetin_src_mac, snatConn->getInternalMac(), MAC_LEN);
		packetin_src_ip = snatConn->getInternalIp();
		packetin_dst_ip = packetin_ip->src;
		packetin_dst_port = packetin_src_port;
		packetin_src_port = htons(snatConn->getInternalPort());
		external_port_no = snatConn->getSNatPort();

		packetin_inport = host->getPortNo();
		src_sw = host->getSw();

		if(src_sw.isNull()||(0 == packetin_inport))
		{
			CSmartPtr<CHost> gateway_p = CHostMgr::getInstance()->getHostGateway(host);
			if(gateway_p.isNotNull())
			{
				CArpFloodMgr::getInstance()->AddArpRequestNode(gateway_p->getIp(), host->getIp(), gateway_p->getMac());
			}
			return BNC_ERR;
		}
		
        baseExternalPort = G_ExternalMgr.getExternalPortByInternalIp(packetin_src_ip);
        if(NULL == baseExternalPort)
        {
            LOG_WARN_FMT("SNAT: can't find baseExternalPort by host[%s] !", number2ip(packetin_src_ip, str));
            return BNC_ERR;
        }
		gatewaysw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(snatConn->getGatewayDpid());
		if(gatewaysw.isNull())
		{
            LOG_WARN_FMT("SNAT: from external to inside gatewaysw[0x%llx] is NULL!", snatConn->getGatewayDpid());
			return BNC_ERR;
		}
 	}

    LOG_WARN_FMT("SNAT: external_port_no=%d",external_port_no);
	if (0 != external_port_no)
	{
        UINT4 src_vlan_vid = src_sw->getTag();
        UINT4 gateway_vlan_vid = gatewaysw->getTag();
        param_set_p param_set = new param_set_t();
        if (NULL == param_set) 
        {
            LOG_ERROR("SNAT: Can't create param list!");
            return BNC_ERR;
        }

		//UINT4 external_outer_interface_ip= 0xa334a8c0;
		//UINT1 external_outer_interface_mac[6] = {0xfa,0x16,0x3e, 0x32,0xcf, 0xc3};
		UINT4 external_outer_interface_ip= 0;
		UINT1 external_outer_interface_mac[6] = {0};
		CRouter* router = CRouterGateMgr::getInstance()->FindRouterNodeByHostMac(packetin_src_mac);
		if(NULL == router)
		{
			LOG_WARN_FMT("SNAT: Can't get router by host[%s] !!!", mac2str(packetin_src_mac, str));
			return BNC_ERR;
		}
		external_outer_interface_ip = router->getRouterIp();
		CSmartPtr<CHost> routerNode = CHostMgr::getInstance()->findHostByIp(external_outer_interface_ip);
		if(routerNode.isNull())
		{
			LOG_WARN_FMT("SNAT: Can't get router Node by router[%s] !!!", number2ip(external_outer_interface_ip, str));
            delete param_set;
			return BNC_ERR;
		}
		memcpy(external_outer_interface_mac, routerNode->getMac(), MAC_LEN);
		
		param_set->src_ip = packetin_src_ip;
		param_set->dst_ip = packetin_dst_ip;
		param_set->src_port_no = packetin_src_port;
		param_set->proto = packetin_proto;
		memcpy(param_set->src_mac, packetin_src_mac, MAC_LEN);
		param_set->outer_ip = external_outer_interface_ip;
		memcpy(param_set->outer_gateway_mac, baseExternalPort->get_gateway_MAC(), 6);
		memcpy(param_set->outer_mac, external_outer_interface_mac, 6);
		param_set->dst_port_no = external_port_no;
		param_set->dst_vlanid = gateway_vlan_vid;
		param_set->src_vlanid = src_vlan_vid;
		param_set->dst_inport = baseExternalPort->get_switch_port();
		param_set->src_sw = src_sw;
		param_set->dst_sw = gatewaysw;
		param_set->src_inport = packetin_inport;

		// get the output port from gateway to src
		UINT4 gateway_to_src_port_no = CControl::getInstance()->getTopoMgr().get_out_port_between_switch(gatewaysw, src_sw);
		if (0 == gateway_to_src_port_no) 
        {
			CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(packetin_src_mac);
			if((gatewaysw == src_sw)&&host.isNotNull()&&(gatewaysw == host->getSw()))
			{
				param_set->dst_gateway_output = baseExternalPort->get_switch_port();
			}
			else
			{
                INT1 gwMacStr[20] = {0}, swMacStr[20] = {0};
                mac2str((UINT1*)gatewaysw->getSwMac(), gwMacStr);
                mac2str((UINT1*)src_sw->getSwMac(), swMacStr);
                INT1 gwIpStr[20] = {0}, swIpStr[20] = {0};
                number2ip(htonl(gatewaysw->getSwIp()), gwIpStr);
                number2ip(htonl(src_sw->getSwIp()), swIpStr);
				LOG_WARN_FMT("SNAT: Can't find the output port from gateway switch[0x%llx-%s-%s] to source switch[0x%llx-%s-%s]!",
                    gatewaysw->getDpid(), gwMacStr, gwIpStr, src_sw->getDpid(), swMacStr, swIpStr);
                delete param_set;
                //CControl::getInstance()->getTopoMgr().printPaths();
				return BNC_ERR;
			}
		}
		else 
        {
			param_set->dst_gateway_output = gateway_to_src_port_no;
		}
		
		LOG_WARN_FMT("SNAT: param_set->dst_sw=%p param_set->src_sw=%p packetin_src_ip=0x%x packetin_dst_ip=0x%x",
    		(CSwitch*)gatewaysw.getPtr(), (CSwitch*)src_sw.getPtr(), packetin_src_ip, packetin_dst_ip);
		if(gatewaysw != src_sw)
		{
			CFlowMgr::getInstance()->install_fabric_nat_throughfirewall_from_inside_flow(
                param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto,
                param_set->src_mac,param_set->src_sw);
			CFlowMgr::getInstance()->install_fabric_nat_from_inside_flow(
                param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, 
                param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac, param_set->outer_mac, 
                param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->dst_inport,
                param_set->src_sw, param_set->dst_sw);
			CFlowMgr::getInstance()->install_fabric_nat_from_external_fabric_flow(
                param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto,
                param_set->src_mac, param_set->outer_ip, param_set->outer_mac, param_set->dst_port_no, param_set->src_vlanid,
                param_set->dst_sw, param_set->dst_gateway_output);	
			CFlowMgr::getInstance()->install_fabric_nat_from_external_fabric_host_flow(
                param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, 
                param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac, param_set->outer_mac, 
                param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->src_inport, 
                param_set->src_sw, param_set->dst_sw);
		}
		else
		{
			LOG_WARN_FMT("SNAT: gatewaysw == src_sw sw_ip=0x%x", gatewaysw->getSwIp());
			CFlowMgr::getInstance()->install_fabric_nat_sameswitch_external2host_flow(
                param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, 
                param_set->src_mac, param_set->outer_ip, param_set->outer_mac, param_set->dst_port_no, 
                param_set->src_inport, param_set->dst_sw);
			CFlowMgr::getInstance()->install_fabric_nat_sameswitch_host2external_flow(
                param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto,
                param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,param_set->outer_mac, 
                param_set->dst_port_no, param_set->dst_gateway_output,  param_set->src_sw);
		}
		CFlowMgr::getInstance()->install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);

        delete param_set;
	}

	COfMsgUtil::forward(srcSw, OFPP13_TABLE, packetIn);

	LOG_WARN_FMT("SNAT: from_inside= %d packetin_src_ip=0x%x packetin_dst_ip=0x%x",from_inside, packetin_src_ip, packetin_dst_ip)
	return BNC_OK;
}

INT4 CNatCommService::GetServiceType(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, UINT4 srcIp, UINT4 dstIp, UINT2 srcPort, UINT2 dstPort, UINT2 proto )
{
	if ((((srcHost.isNotNull()&&(bnc::host::HOST_NORMAL == srcHost->getHostType()))
            &&(dstHost.isNull()||(bnc::host::HOST_EXTERNAL == dstHost->getHostType())))
          ||((srcHost.isNull()||(bnc::host::HOST_EXTERNAL == srcHost->getHostType()))
              &&(dstHost.isNotNull()&&(bnc::host::HOST_GATEWAY == dstHost->getHostType()))))
    	 &&(IPPROTO_ICMP != proto)&&(dstIp != 0xffffffff)&&(srcIp != 0xffffffff))
	{
		return BNC_OK;
	}
	return BNC_ERR;
}



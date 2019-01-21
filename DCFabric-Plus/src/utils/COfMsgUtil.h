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
*   File Name   : COfMsgUtil.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-6-22           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef __COFMSGUTIL_H
#define __COFMSGUTIL_H
#include "bnc-type.h"
#include "bnc-param.h"
#include "bnc-inet.h"
#include "CSmartPtr.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "openflow-common.h"
#include "CSwitch.h"
#include "CMsg.h"
#include "CLoopBuffer.h"
#include "CHost.h"

extern const CMsgPath g_of13MsgPathArpReq;
extern const CMsgPath g_of13MsgPathArpRsp;
extern const CMsgPath g_of13MsgPathIp;

class COfMsgUtil
{
public:
    /*
     * 发送Openflow OFPT13_HELLO/OFPT13_ECHO_REQUEST/OFPT13_ECHO_REPLY/
     * OFPT13_FEATURES_REQUEST/OFPT13_GET_CONFIG_REQUEST/OFPT13_BARRIER_REQUEST消息
     */
    static BOOL sendOfpMsg(INT4 fd, UINT1 version, UINT1 type, UINT4 xid=OF13_DEFAULT_XID);

    /*
     * 发送Openflow OFPT13_SET_CONFIG消息
     */
    static BOOL sendOfp13SetConfig(INT4 fd);

    /*
     * 发送Openflow OFPT13_MULTIPART_REQUEST消息
     */
    static BOOL sendOfp13MultiPartRequest(INT4 fd, ofp_multipart_types type, void* data=NULL);

    /*
     * 发送Openflow OFPT13_ROLE_REQUEST消息
     */
    static BOOL sendOfp13RoleRequest(INT4 fd, UINT4 role);

    /*
     * 将struct ofp13_port转换为gn_port_t
     */
    static void ofp13PortConvertter(const struct ofp13_port *of_port, gn_port_t *new_sw_port);

    /*
     * 将struct ofp13_port_stats的网络序转换为主机序 
     */
    static void ofp13PortStatsConvertter(const struct ofp13_port_stats *net_stats, struct ofp13_port_stats *host_stats);

    /*
     * TBD: packet-in消息转化 
     */
    static void ofpPacketInConvertter(const INT1* data, packet_in_info_t& pktin);

    /*
     * 发送Openflow Packetout消息
     */
    static BOOL sendOfp13TableMiss(INT4 fd);


    /*
     * 发送用于拓扑发现的LLDP�?
     */
    static BOOL sendOfp13Lldp(CSmartPtr<CSwitch>& sw, const gn_port_t& port);

    /*
     * 发送用于探测主机的ARP广播�?
     */
    static BOOL sendOfp13ArpRequest(UINT4 srcIp, UINT4 destIp,const UINT1* srcMac);


	static void forward(const CSmartPtr<CSwitch> & sw, UINT4 port_no,packet_in_info_t* packetin);
	
	static void forward(const CSmartPtr<CSwitch> & sw, UINT4 port_no,INT4 data_len, void* data);
	 /*
     * 发送用于探测主机的ARP广播�?
     */
    static void ofp13floodInternal(UINT4 datalen, void* data);
	static void ofp13floodInternal( packet_in_info_t* packetin);
    /*
     * 获取消息路径
     *
     * @param: version       消息版本
     * @param: msgType       消息类型
     * @param: ethType       当为packet-in消息时的ethernet类型
     * @param: subType       当为packet-in消息时的ethernet子类�?
     *
     * ret: 成功 or 失败
     */
    static std::string getMsgPath(INT4 version, INT4 msgType, INT4 ethType=0, INT4 subType=0);

public:
	/*
     * 创建一个ARP�?
     *
     * @param: arp_pkt      ARP包指�?
     *
     * @param: code         ARP类型(0x01: Request, 0x02: Reply)
     * @param: src_mac      源主机MAC地址
     * @param: dst_mac      目的主机MAC地址
     * @param: src_ip       源主机IP地址
     * @param: dst_ip       目的主机IP地址
     *
     * @return: BOOL        TRUE: 成功; FALSE: 失败
     */
    static BOOL createArpPacket(arp_t* arp_pkt, UINT2 code,
    		                   const UINT1* src_mac, const UINT1* dst_mac,
    		                   UINT4 src_ip, UINT4 dst_ip);

    /*
     * 创建ARP Reply�?
     *
     * @param: arp_pkt      ARP包指�?
     * @param: src          源主�?
     * @param: dst          目的主机
     *
     * @return: BOOL        TRUE: 成功; FALSE:失败
     */
    static BOOL createArpReplyPacket(arp_t* arp_pkt, CSmartPtr<CHost>& src, CSmartPtr<CHost>& dst);

    /*
     * 创建ARP Reply�?
     *
     * @param: arp_pkt      ARP包指�?
     * @param: src          源主�?
     * @param: dst          目的主机
     *
     * @return: BOOL        TRUE: 成功; FALSE:失败
     */
    static BOOL createArpReplyPacket(arp_t* arp_pkt, CSmartPtr<CHost>& dst, arp_t* pkt);

    /*
     * 创建ARP广播�?
     *
     * @param: arp_pkt      ARP包指�?
     * @param: src          源主�?
     * @param: dst          目的主机
     *
     * @return: BOOL        TRUE: 成功; FALSE:失败
     */
    static BOOL createArpRequestFloodPacket(arp_t* arp_pkt, CSmartPtr<CHost>& src, UINT4 dst_ip);


	static INT4 remove_flows_by_sw_port(UINT8 sw_dpid, UINT4 port);
private:
    /*
     * 填充LLDP消息
     * param: dpid   src dpid
     * param: mac    src mac
     * param: port   src port
     * param: lldp   被填充的LLDP
     * ret: None
     */
    static void fillLldpPkt(UINT8 dpid, const UINT1 *mac, UINT4 port, lldp_t *lldp);

    /*
     * 填充拓扑发现IP消息
     * param: dpid   src dpid
     * param: mac    src mac
     * param: port   src port
     * param: data   被填充的IP
     * ret: None
     */
    static void fillDiscoveryIpPkt(UINT8 dpid, UINT4 port, UINT1 *data);

    /*
     * 获取packet in包中的data指针
     * param: pktIn   packet in�?
     * ret: data指针
     */
    static UINT1* getPktInData(struct ofp13_packet_in *pktIn);

    /*
     * 发送Openflow 1.3 Packetout的消�?
     */
    static BOOL sendOfp13PacketOut(INT4 fd, packout_req_info_t& pktout);


	
	static INT4 remove_floating_flow(UINT4 fixed_ip, UINT4 floating_ip, UINT1* mac);
	static INT4 remove_nat_flow(const CSmartPtr<CSwitch>& sw, UINT4 ip, UINT1* mac);
	static INT4 remove_host_output_flow_by_ip_mac(const CSmartPtr<CSwitch>& sw, UINT4 ip, UINT1* mac);

private:
    COfMsgUtil();
    ~COfMsgUtil();

};

#endif

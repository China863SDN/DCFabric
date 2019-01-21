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
*   File Name   : CWebRestApi.cpp                                             *
*   Author      : bnc xflu                                                    *
*   Create Date : 2016-8-31                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#include "CWebRestApi.h"
#include "CHost.h"
#include "CHostNormal.h"
#include "CHostMgr.h"
#include "BasePort.h"
#include "BasePortManager.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "COpenstackExternal.h"
#include "COpenstackExternalMgr.h"
#include "COpenstackMgr.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "CControl.h"
#include "comm-util.h"
#include "log.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "CFlowDefine.h"
#include "COfMsgUtil.h"
#include "CFlowMgr.h"
#include "CClusterService.h"

using namespace rapidjson;

static void writeMatch(Writer<StringBuffer>& writer, gn_oxm_t& oxm)
{
    INT1 json_tmp[1024] = {0};

    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IN_PORT))
    {
        writer.Key("inport");
        if (OFPP13_CONTROLLER == oxm.in_port)
            writer.String("controller");
        else
            writer.Uint(oxm.in_port);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IN_PHY_PORT))
    {
        writer.Key("inPhyPort");
        writer.Uint(oxm.in_phy_port);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_METADATA))
    {
        writer.Key("metadata");
        writer.Uint64(oxm.metadata);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_DST))
    {
        writer.Key("ethDst");
        writer.String(mac2str(oxm.eth_dst, json_tmp));
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_SRC))
    {
        writer.Key("ethSrc");
        writer.String(mac2str(oxm.eth_src, json_tmp));
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ETH_TYPE))
    {
        writer.Key("ethType");
        writer.String(
            (ETHER_LLDP == oxm.eth_type) ? "LLDP" :
            (ETHER_ARP == oxm.eth_type) ? "ARP" :
            (ETHER_IP == oxm.eth_type) ? "IPV4" :
            (ETHER_IPV6 == oxm.eth_type) ? "IPV6" :
            (ETHER_VLAN == oxm.eth_type) ? "VLAN" :
            (ETHER_MPLS == oxm.eth_type) ? "MPLS" : "UNKNOW");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_VLAN_VID))
    {
        writer.Key("vlanId");
        writer.Uint(oxm.vlan_vid);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_VLAN_PCP))
    {
        writer.Key("vlanPcp");
        writer.Uint(oxm.vlan_pcp);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_DSCP))
    {
        writer.Key("ipDscp");
        writer.Uint(oxm.ip_dscp);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_ECN))
    {
        writer.Key("ipEcn");
        writer.Uint(oxm.ip_ecn);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IP_PROTO))
    {
        writer.Key("ipProto");
        writer.String(
            (IPPROTO_ICMP == oxm.ip_proto) ? "ICMP" :
            (IPPROTO_TCP == oxm.ip_proto) ? "TCP" :
            (IPPROTO_UDP == oxm.ip_proto) ? "UDP" : "UNKNOW");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_SRC))
    {
        writer.Key("ipv4Src");
        if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_SRC_PREFIX))
            sprintf(json_tmp, "%s/%d", inet_htoa(oxm.ipv4_src), oxm.ipv4_src_prefix);
        else
            sprintf(json_tmp, "%s", inet_htoa(oxm.ipv4_src));
        writer.String(json_tmp);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_DST))
    {
        writer.Key("ipv4Dst");
        if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV4_DST_PREFIX))
            sprintf(json_tmp, "%s/%d", inet_htoa(oxm.ipv4_dst), oxm.ipv4_dst_prefix);
        else
            sprintf(json_tmp, "%s", inet_htoa(oxm.ipv4_dst));
        writer.String(json_tmp);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TCP_SRC))
    {
        writer.Key("tcpSrc");
        writer.Uint(oxm.tcp_src);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TCP_DST))
    {
        writer.Key("tcpDst");
        writer.Uint(oxm.tcp_dst);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_UDP_SRC))
    {
        writer.Key("udpSrc");
        writer.Uint(oxm.udp_src);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_UDP_DST))
    {
        writer.Key("udpDst");
        writer.Uint(oxm.udp_dst);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_SCTP_SRC))
    {
        writer.Key("sctpSrc");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_SCTP_DST))
    {
        writer.Key("sctpDst");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ICMPV4_TYPE))
    {
        writer.Key("icmpv4Type");
        writer.Uint(oxm.icmpv4_type);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ICMPV4_CODE))
    {
        writer.Key("icmpv4Code");
        writer.Uint(oxm.icmpv4_code);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_OP))
    {
        writer.Key("arpOp");
        //writer.String((1 == oxm.arp_op) ? "Request" : "Reply");
        writer.Uint(oxm.arp_op);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_SPA))
    {
        writer.Key("arpSpa");
        writer.String(inet_htoa(oxm.arp_spa));
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_TPA))
    {
        writer.Key("arpTpa");
        writer.String(inet_htoa(oxm.arp_tpa));
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_SHA))
    {
        writer.Key("arpSha");
        writer.String(mac2str(oxm.arp_sha, json_tmp));
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ARP_THA))
    {
        writer.Key("arpTha");
        writer.String(mac2str(oxm.arp_tha, json_tmp));
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_SRC))
    {
        writer.Key("ipv6Src");
        inet_ntop(AF_INET6, (char*)oxm.ipv6_src, json_tmp, 40);
        if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_SRC_PREFIX))
            sprintf(json_tmp, "%s/%d", json_tmp, oxm.ipv6_src_prefix);
        writer.String(json_tmp);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_DST))
    {
        writer.Key("ipv6Dst");
        inet_ntop(AF_INET6, (char*)oxm.ipv6_dst, json_tmp, 40);
        if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_DST_PREFIX))
            sprintf(json_tmp, "%s/%d", json_tmp, oxm.ipv6_dst_prefix);
        writer.String(json_tmp);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_FLABEL))
    {
        writer.Key("ipv6Flabel");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ICMPV6_TYPE))
    {
        writer.Key("icmpv6Type");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_ICMPV6_CODE))
    {
        writer.Key("icmpv6Code");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_ND_TARGET))
    {
        writer.Key("ipv6NdTarget");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_ND_SLL))
    {
        writer.Key("ipv6NdSll");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_ND_TLL))
    {
        writer.Key("ipv6NdTll");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_MPLS_LABEL))
    {
        writer.Key("mplsLabel");
        writer.Uint(oxm.mpls_label);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_MPLS_TC))
    {
        writer.Key("mplsTc");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFP_MPLS_BOS))
    {
        writer.Key("mplsBos");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_PBB_ISID))
    {
        writer.Key("pbbIsid");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_TUNNEL_ID))
    {
        writer.Key("tunnelId");
        writer.Uint(oxm.tunnel_id);
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IPV6_EXTHDR))
    {
        writer.Key("ipv6ExtHdr");
        writer.String("");
    }
    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_METADATA_MASK))
    {
        writer.Key("metadataMask");
        writer.Uint64(oxm.metadata_mask);
    }
}

static void writeAction(Writer<StringBuffer>& writer, CFlowAction& action)
{
    switch (action.m_type)
    {
        case OFPAT13_OUTPUT:
            writer.Key("output");
            if (OFPP13_CONTROLLER == action.m_port)
                writer.String("controller");
            else
                writer.Uint(action.m_port);
            break;
        case OFPAT13_COPY_TTL_OUT:
            writer.Key("copyTtlOut");
            writer.String("");
            break;
        case OFPAT13_COPY_TTL_IN:
            writer.Key("copyTtlIn");
            writer.String("");
            break;
        case OFPAT13_MPLS_TTL:
            writer.Key("mplsTtl");
            writer.Uint(action.m_mpls_tt);
            break;
        case OFPAT13_DEC_MPLS_TTL:
            writer.Key("decMplsTtl");
            writer.String("");
            break;
        case OFPAT13_PUSH_VLAN:
            writer.Key("pushVlan");
            writer.String("PUSH_VLAN");
            break;
        case OFPAT13_POP_VLAN:
            writer.Key("popVlan");
            writer.String("POP_VLAN");
            break;
        case OFPAT13_PUSH_MPLS:
            writer.Key("pushMpls");
            writer.String("PUSH_MPLS");
            break;
        case OFPAT13_POP_MPLS:
            writer.Key("popMpls");
            writer.String("POP_MPLS");
            break;
        case OFPAT13_SET_QUEUE:
            writer.Key("setQueue");
            writer.Uint(action.m_queue_id);
            break;
        case OFPAT13_GROUP:
            writer.Key("group");
            writer.Uint(action.m_group_id);
            break;
        case OFPAT13_SET_NW_TTL:
            writer.Key("setNwTtl");
            writer.Uint(action.m_nw_tt);
            break;
        case OFPAT13_DEC_NW_TTL:
            writer.Key("decNwTtl");
            writer.String("");
            break;
        case OFPAT13_SET_FIELD:
            writer.Key("setField");
            writer.StartObject();
            {
                writeMatch(writer, action.m_oxm_fields);
            }
            writer.EndObject();
            break;
        case OFPAT13_PUSH_PBB:
            writer.Key("pushPbb");
            writer.String("");
            break;
        case OFPAT13_POP_PBB:
            writer.Key("popPbb");
            writer.String("");
            break;
        case OFPAT13_EXPERIMENTER:
            writer.Key("experimenter");
            writer.Uint(action.m_experimenter);
            break;
        default:
            break;
    }
}

static void writeInstruction(Writer<StringBuffer>& writer, CFlowInstruction& instruction)
{
    INT1 json_tmp[1024] = {0};

    switch (instruction.m_type)
    {
        case OFPIT_GOTO_TABLE:
            writer.Key("gotoTable");
            writer.Uint(instruction.m_table_id);
            break;
        case OFPIT_WRITE_METADATA:
            writer.Key("writeMetadata");
            sprintf(json_tmp, "%llu/%llu", instruction.m_metadata, instruction.m_metadata_mask);
            writer.String(json_tmp);
            break;
        case OFPIT_WRITE_ACTIONS:
        case OFPIT_APPLY_ACTIONS:
            if (!instruction.m_actions.empty())
            {
                writer.Key((OFPIT_WRITE_ACTIONS==instruction.m_type)?"writeAction":"applyAction");
                writer.StartObject();
                {
                    STL_FOR_LOOP(instruction.m_actions, itAction)
                    {
                        writeAction(writer, itAction->second);
                    }
                }
                writer.EndObject();
            }
            break;
        case OFPIT_CLEAR_ACTIONS:
            writer.Key("clearAction");
            writer.String("");
            break;
        case OFPIT_METER:
            writer.Key("meter");
            writer.Uint(instruction.m_meter_id);
            break;
        case OFPIT_EXPERIMENTER:
            writer.Key("experimenter");
            sprintf(json_tmp, "%u:%u", instruction.m_experimenterLen, instruction.m_experimenter);
            writer.String(json_tmp);
            break;
    }
}

static void writeFlow(Writer<StringBuffer>& writer, CFlow& flow)
{
    writer.Key("uuid");
    writer.String(flow.getUuid().c_str());
    writer.Key("createTime");
    writer.Uint64(flow.getCreateTime());
    writer.Key("tableId");
    writer.Uint(flow.getTableId());
    writer.Key("idleTimeout");
    writer.Uint(flow.getIdleTimeout());
    writer.Key("hardTimeout");
    writer.Uint(flow.getHardTimeout());
    writer.Key("priority");
    writer.Uint(flow.getPriority());

    writer.Key("match");
    writer.StartObject();
    {
        writeMatch(writer, flow.getMatch().oxm_fields);
    }
    writer.EndObject();

    writer.Key("instruction");
    writer.StartObject();
    {
        CFlowInstructionMap& instructionMap = flow.getInstructions();
        STL_FOR_LOOP(instructionMap, itInstruct)
        {
            writeInstruction(writer, itInstruct->second);
        }
    }
    writer.EndObject();
}

static void writeSwitchFlows(Writer<StringBuffer>& writer, UINT8 dpid)
{
    CFlowCache& flowCache = CFlowMgr::getInstance()->getFlowCache();
    CTableIdFlowsMap* tableIdMap = flowCache.getTableIdFlowsMap(dpid);
    if (NULL != tableIdMap)
    {
        STL_FOR_LOOP(*tableIdMap, itTableId)
        {
            STL_FOR_LOOP(itTableId->second, itFlow)
            {
                writer.StartObject();
                {
                    writeFlow(writer, *itFlow);
                }
                writer.EndObject();
            }
        }
    }
}

static void parseMatch(Value& match, gn_oxm_t& oxm)
{
    oxm.mask = 0;

    if (match.HasMember("inport") && match["inport"].IsString())
    {
        Value& inport = match["inport"];
        oxm.in_port = atoi(inport.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_IN_PORT);
    }
    if (match.HasMember("inPhyPort") && match["inPhyPort"].IsString())
    {
        Value& inPhyPort = match["inPhyPort"];
        oxm.in_phy_port = atoi(inPhyPort.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_IN_PHY_PORT);
    }
    if (match.HasMember("metadata") && match["metadata"].IsString())
    {
        Value& metadata = match["metadata"];
        oxm.metadata = atoll(metadata.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_METADATA);
    }
    if (match.HasMember("ethDst") && match["ethDst"].IsString())
    {
        Value& ethDst = match["ethDst"];
        macstr2hex(ethDst.GetString(), oxm.eth_dst);
        SET_MASK(oxm.mask, OFPXMT_OFB_ETH_DST);
    }
    if (match.HasMember("ethSrc") && match["ethSrc"].IsString())
    {
        Value& ethSrc = match["ethSrc"];
        macstr2hex(ethSrc.GetString(), oxm.eth_src);
        SET_MASK(oxm.mask, OFPXMT_OFB_ETH_SRC);
    }
    if (match.HasMember("ethType") && match["ethType"].IsString())
    {
        Value& ethType = match["ethType"];
        oxm.eth_type = 
            (0==strncasecmp(ethType.GetString(), "LLDP", 4))?ETHER_LLDP:
            (0==strncasecmp(ethType.GetString(), "ARP", 3))?ETHER_ARP:
            (0==strncasecmp(ethType.GetString(), "IPV4", 4))?ETHER_IP:
            (0==strncasecmp(ethType.GetString(), "IPV6", 4))?ETHER_IPV6:
            (0==strncasecmp(ethType.GetString(), "MPLS", 4))?ETHER_MPLS:
            (0==strncasecmp(ethType.GetString(), "VLAN", 4))?ETHER_VLAN:0xffff;
        SET_MASK(oxm.mask, OFPXMT_OFB_ETH_TYPE);
    }
    if (match.HasMember("vlanId") && match["vlanId"].IsString())
    {
        Value& vlanId = match["vlanId"];
        oxm.vlan_vid = atoi(vlanId.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_VLAN_VID);
    }
    if (match.HasMember("vlanPcp") && match["vlanPcp"].IsString())
    {
        Value& vlanPcp = match["vlanPcp"];
        oxm.vlan_pcp = atoi(vlanPcp.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_VLAN_PCP);
    }
    if (match.HasMember("ipDscp") && match["ipDscp"].IsString())
    {
        Value& ipDscp = match["ipDscp"];
        oxm.ip_dscp = atoi(ipDscp.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_IP_DSCP);
    }
    if (match.HasMember("ipEcn") && match["ipEcn"].IsString())
    {
        Value& ipEcn = match["ipEcn"];
        oxm.ip_ecn = atoi(ipEcn.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_IP_ECN);
    }
    if (match.HasMember("ipProto") && match["ipProto"].IsString())
    {
        Value& ipProto = match["ipProto"];
        oxm.ip_proto = 
            (0==strncasecmp(ipProto.GetString(), "ICMP", 4))?IPPROTO_ICMP:
            (0==strncasecmp(ipProto.GetString(), "TCP", 3))?IPPROTO_TCP:
            (0==strncasecmp(ipProto.GetString(), "UDP", 3))?IPPROTO_UDP:0xff;
        SET_MASK(oxm.mask, OFPXMT_OFB_IP_PROTO);
    }
    if (match.HasMember("ipv4Src") && match["ipv4Src"].IsString())
    {
        Value& ipv4Src = match["ipv4Src"];
        INT1* ptr = (INT1*)strchr(ipv4Src.GetString(), '/');
        if (NULL != ptr)
        {
            *ptr = '\0';
            oxm.ipv4_src_prefix = atoi(ptr+1);
            SET_MASK(oxm.mask, OFPXMT_OFB_IPV4_SRC_PREFIX);
        }
        oxm.ipv4_src = ntohl(ip2number(ipv4Src.GetString()));
        SET_MASK(oxm.mask, OFPXMT_OFB_IPV4_SRC);
    }
    if (match.HasMember("ipv4Dst") && match["ipv4Dst"].IsString())
    {
        Value& ipv4Dst = match["ipv4Dst"];
        INT1* ptr = (INT1*)strchr(ipv4Dst.GetString(), '/');
        if (NULL != ptr)
        {
            *ptr = '\0';
            oxm.ipv4_dst_prefix = atoi(ptr+1);
            SET_MASK(oxm.mask, OFPXMT_OFB_IPV4_DST_PREFIX);
        }
        oxm.ipv4_dst = ntohl(ip2number(ipv4Dst.GetString()));
        SET_MASK(oxm.mask, OFPXMT_OFB_IPV4_DST);
    }
    if (match.HasMember("tcpSrc") && match["tcpSrc"].IsString())
    {
        Value& tcpSrc = match["tcpSrc"];
        oxm.tcp_src = atoi(tcpSrc.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_TCP_SRC);
    }
    if (match.HasMember("tcpDst") && match["tcpDst"].IsString())
    {
        Value& tcpDst = match["tcpDst"];
        oxm.tcp_dst = atoi(tcpDst.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_TCP_DST);
    }
    if (match.HasMember("udpSrc") && match["udpSrc"].IsString())
    {
        Value& udpSrc = match["udpSrc"];
        oxm.udp_src = atoi(udpSrc.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_UDP_SRC);
    }
    if (match.HasMember("udpDst") && match["udpDst"].IsString())
    {
        Value& udpDst = match["udpDst"];
        oxm.udp_dst = atoi(udpDst.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_UDP_DST);
    }
    if (match.HasMember("icmpv4Type") && match["icmpv4Type"].IsString())
    {
        Value& icmpv4Type = match["icmpv4Type"];
        oxm.icmpv4_type = atoi(icmpv4Type.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_ICMPV4_TYPE);
    }
    if (match.HasMember("icmpv4Code") && match["icmpv4Code"].IsString())
    {
        Value& icmpv4Code = match["icmpv4Code"];
        oxm.icmpv4_code = atoi(icmpv4Code.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_ICMPV4_CODE);
    }
    if (match.HasMember("arpOp") && match["arpOp"].IsString())
    {
        Value& arpOp = match["arpOp"];
        oxm.arp_op = atoi(arpOp.GetString());//(0==strncasecmp(arpOp.GetString(), "Request", 7))?1:2;
        SET_MASK(oxm.mask, OFPXMT_OFB_ARP_OP);
    }
    if (match.HasMember("arpSpa") && match["arpSpa"].IsString())
    {
        Value& arpSpa = match["arpSpa"];
        oxm.arp_spa = ntohl(ip2number(arpSpa.GetString()));
        SET_MASK(oxm.mask, OFPXMT_OFB_ARP_SPA);
    }
    if (match.HasMember("arpTpa") && match["arpTpa"].IsString())
    {
        Value& arpTpa = match["arpTpa"];
        oxm.arp_spa = ntohl(ip2number(arpTpa.GetString()));
        SET_MASK(oxm.mask, OFPXMT_OFB_ARP_TPA);
    }
    if (match.HasMember("arpSha") && match["arpSha"].IsString())
    {
        Value& arpSha = match["arpSha"];
        macstr2hex(arpSha.GetString(), oxm.arp_sha);
        SET_MASK(oxm.mask, OFPXMT_OFB_ARP_SHA);
    }
    if (match.HasMember("arpTha") && match["arpTha"].IsString())
    {
        Value& arpTha = match["arpTha"];
        macstr2hex(arpTha.GetString(), oxm.arp_tha);
        SET_MASK(oxm.mask, OFPXMT_OFB_ARP_THA);
    }
    if (match.HasMember("mplsLabel") && match["mplsLabel"].IsString())
    {
        Value& mplsLabel = match["mplsLabel"];
        oxm.mpls_label = atoi(mplsLabel.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_MPLS_LABEL);
    }
    if (match.HasMember("tunnelId") && match["tunnelId"].IsString())
    {
        Value& tunnelId = match["tunnelId"];
        oxm.tunnel_id = atoi(tunnelId.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_TUNNEL_ID);
    }
    if (match.HasMember("metadataMask") && match["metadataMask"].IsString())
    {
        Value& metadataMask = match["metadataMask"];
        oxm.metadata_mask = atoll(metadataMask.GetString());
        SET_MASK(oxm.mask, OFPXMT_OFB_METADATA_MASK);
    }
}

static void parseActions(Value& action, gn_action_t** actions)
{
    if (action.HasMember("output") && action["output"].IsString())
    {
        Value& output = action["output"];
        gn_action_output_t* act = (gn_action_output_t*)bnc_malloc(sizeof(gn_action_output_t));
        if (NULL != act)
        {
            act->type = OFPAT13_OUTPUT;
            act->port = (0==strcmp(output.GetString(), "controller"))?OFPP13_CONTROLLER:atoi(output.GetString());
            act->max_len = (OFPP13_CONTROLLER==act->port)?0xffff:0;
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("mplsTtl") && action["mplsTtl"].IsString())
    {
        Value& mplsTtl = action["mplsTtl"];
        gn_action_mpls_ttl_t* act = (gn_action_mpls_ttl_t*)bnc_malloc(sizeof(gn_action_mpls_ttl_t));
        if (NULL != act)
        {
            act->type = OFPAT13_MPLS_TTL;
            act->mpls_tt = atoi(mplsTtl.GetString());
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("pushVlan") && action["pushVlan"].IsString())
    {
        gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
        if (NULL != act)
        {
            act->type = OFPAT13_PUSH_VLAN;
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("popVlan") && action["popVlan"].IsString())
    {
        gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
        if (NULL != act)
        {
            act->type = OFPAT13_POP_VLAN;
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("pushMpls") && action["pushMpls"].IsString())
    {
        gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
        if (NULL != act)
        {
            act->type = OFPAT13_PUSH_MPLS;
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("popMpls") && action["popMpls"].IsString())
    {
        gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
        if (NULL != act)
        {
            act->type = OFPAT13_POP_MPLS;
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("setQueue") && action["setQueue"].IsString())
    {
        Value& setQueue = action["setQueue"];
        gn_action_set_queue_t* act = (gn_action_set_queue_t*)bnc_malloc(sizeof(gn_action_set_queue_t));
        if (NULL != act)
        {
            act->type = OFPAT13_SET_QUEUE;
            act->queue_id = atoi(setQueue.GetString());
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("group") && action["group"].IsString())
    {
        Value& group = action["group"];
        gn_action_group_t* act = (gn_action_group_t*)bnc_malloc(sizeof(gn_action_group_t));
        if (NULL != act)
        {
            act->type = OFPAT13_GROUP;
            act->group_id = atoi(group.GetString());
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("setNwTtl") && action["setNwTtl"].IsString())
    {
        Value& setNwTtl = action["setNwTtl"];
        gn_action_nw_ttl_t* act = (gn_action_nw_ttl_t*)bnc_malloc(sizeof(gn_action_nw_ttl_t));
        if (NULL != act)
        {
            act->type = OFPAT13_SET_NW_TTL;
            act->nw_tt = atoi(setNwTtl.GetString());
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("setField") && action["setField"].IsObject())
    {
        Value& setField = action["setField"];
        gn_action_set_field_t* act = (gn_action_set_field_t*)bnc_malloc(sizeof(gn_action_set_field_t));
        if (NULL != act)
        {
            act->type = OFPAT13_SET_FIELD;
            parseMatch(setField, act->oxm_fields);
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("pushPbb") && action["pushPbb"].IsString())
    {
        gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
        if (NULL != act)
        {
            act->type = OFPAT13_PUSH_PBB;
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("popPbb") && action["popPbb"].IsString())
    {
        gn_action_t* act = (gn_action_t*)bnc_malloc(sizeof(gn_action_t));
        if (NULL != act)
        {
            act->type = OFPAT13_POP_PBB;
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
    if (action.HasMember("experimenter") && action["experimenter"].IsString())
    {
        Value& experimenter = action["experimenter"];
        gn_action_experimenter_t* act = (gn_action_experimenter_t*)bnc_malloc(sizeof(gn_action_experimenter_t));
        if (NULL != act)
        {
            act->type = OFPAT13_EXPERIMENTER;
            act->experimenter = atoi(experimenter.GetString());
            act->next = *actions;
            *actions = (gn_action_t*)act;
        }
    }
}

static void parseInstructions(Value& instruction, gn_instruction_t** instructions)
{
    if (instruction.HasMember("gotoTable"))
    {
        Value& gotoTable = instruction["gotoTable"];
        gn_instruction_goto_table_t* instruct = (gn_instruction_goto_table_t*)bnc_malloc(sizeof(gn_instruction_goto_table_t));
        if (NULL != instruct)
        {
            instruct->type = OFPIT_GOTO_TABLE;
            instruct->table_id = gotoTable.IsString() ? atoi(gotoTable.GetString()) : gotoTable.GetInt();
            instruct->next = *instructions;
            *instructions = (gn_instruction_t*)instruct;
        }
    }
    if (instruction.HasMember("writeMetadata") && instruction["writeMetadata"].IsObject())
    {
        Value& writeMetadata = instruction["writeMetadata"];
        gn_instruction_write_metadata_t* instruct = (gn_instruction_write_metadata_t*)bnc_malloc(sizeof(gn_instruction_write_metadata_t));
        if (NULL != instruct)
        {
            instruct->type = OFPIT_WRITE_METADATA;
            if(writeMetadata.HasMember("metadata") && writeMetadata["metadata"].IsUint64())
            {
                Value& metadata = writeMetadata["metadata"];
                instruct->metadata = metadata.GetUint64();
            }
            if(writeMetadata.HasMember("metadataMask") && writeMetadata["metadataMask"].IsUint64())
            {
                Value& metadataMask = writeMetadata["metadataMask"];
                instruct->metadata_mask = metadataMask.GetUint64();
            }
            instruct->next = *instructions;
            *instructions = (gn_instruction_t*)instruct;
        }
    }
    if (instruction.HasMember("writeAction") && instruction["writeAction"].IsObject())
    {
        Value& writeAction = instruction["writeAction"];
        gn_instruction_actions_t* instruct = (gn_instruction_actions_t*)bnc_malloc(sizeof(gn_instruction_actions_t));
        if (NULL != instruct)
        {
            instruct->type = OFPIT_WRITE_ACTIONS;
            instruct->actions = NULL;
            parseActions(writeAction, &(instruct->actions));
            instruct->next = *instructions;
            *instructions = (gn_instruction_t*)instruct;
        }
    }
    if (instruction.HasMember("applyAction") && instruction["applyAction"].IsObject())
    {
        Value& applyAction = instruction["applyAction"];
        gn_instruction_actions_t* instruct = (gn_instruction_actions_t*)bnc_malloc(sizeof(gn_instruction_actions_t));
        if (NULL != instruct)
        {
            instruct->type = OFPIT_APPLY_ACTIONS;
            instruct->actions = NULL;
            parseActions(applyAction, &(instruct->actions));
            instruct->next = *instructions;
            *instructions = (gn_instruction_t*)instruct;
        }
    }
    if (instruction.HasMember("clearAction") && instruction["clearAction"].IsString())
    {
        gn_instruction_t* instruct = (gn_instruction_t*)bnc_malloc(sizeof(gn_instruction_t));
        if (NULL != instruct)
        {
            instruct->type = OFPIT_CLEAR_ACTIONS;
            instruct->next = *instructions;
            *instructions = (gn_instruction_t*)instruct;
        }
    }
    if (instruction.HasMember("meter") && instruction["meter"].IsUint())
    {
        Value& meter = instruction["meter"];
        gn_instruction_meter_t* instruct = (gn_instruction_meter_t*)bnc_malloc(sizeof(gn_instruction_meter_t));
        if (NULL != instruct)
        {
            instruct->type = OFPIT_METER;
            instruct->meter_id = meter.GetUint();
            instruct->next = *instructions;
            *instructions = (gn_instruction_t*)instruct;
        }
    }
    if (instruction.HasMember("experimenter") && instruction["experimenter"].IsUint())
    {
        Value& experimenter = instruction["experimenter"];
        gn_instruction_experimenter_t* instruct = (gn_instruction_experimenter_t*)bnc_malloc(sizeof(gn_instruction_experimenter_t));
        if (NULL != instruct)
        {
            instruct->type = OFPIT_EXPERIMENTER;
            instruct->len = 4;
            instruct->experimenter = experimenter.GetUint();
            instruct->next = *instructions;
            *instructions = (gn_instruction_t*)instruct;
        }
    }
}

static void parseFlow(Document& document, gn_flow_t& flow)
{
	if (document.HasMember("uuid") && document["uuid"].IsString())
	{
		Value& uuid = document["uuid"];
        strncpy(flow.uuid, uuid.GetString(), UUID_LEN-1);
        flow.uuid[UUID_LEN-1] = '\0';
	}
	if (document.HasMember("tableId") && document["tableId"].IsString())
	{
		Value& tableId = document["tableId"];
        flow.table_id = atoi(tableId.GetString());
	}
	if (document.HasMember("idleTimeout") && document["idleTimeout"].IsString())
	{
		Value& idleTimeout = document["idleTimeout"];
        flow.idle_timeout = atoi(idleTimeout.GetString());
	}
	if (document.HasMember("hardTimeout") && document["hardTimeout"].IsString())
	{
		Value& hardTimeout = document["hardTimeout"];
        flow.hard_timeout = atoi(hardTimeout.GetString());
	}
	if (document.HasMember("priority") && document["priority"].IsString())
	{
		Value& priority = document["priority"];
        flow.priority = atoi(priority.GetString());
	}
	if (document.HasMember("match") && document["match"].IsObject())
	{
        flow.match.type = OFPMT_OXM;
	    parseMatch(document["match"], flow.match.oxm_fields);
	}
	if (document.HasMember("instruction") && document["instruction"].IsObject())
	{
	    flow.instructions = NULL;
	    parseInstructions(document["instruction"], &(flow.instructions));
	}
}

void CWebRestApi::api_get_all_switch_info(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("switchInfo");
    writer.StartArray();

    CSwitchHMap& swMap = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    STL_FOR_LOOP(swMap, it)
    {
        CSmartPtr<CSwitch> sw = it->second;
        if (sw.isNull() || (sw->getState() >= SW_STATE_POWER_OFF))
            continue;

        writer.StartObject();

        writer.Key("state");
        INT4 swState = sw->getState();
        writer.String(
            (SW_STATE_NEW_ACCEPT==swState)?"NEW_ACCEPT":
            (SW_STATE_CONNECTED==swState)?"CONNECTED":
            (SW_STATE_STABLE==swState)?"STABLE":
            (SW_STATE_UNREACHABLE==swState)?"UNREACHABLE":
            (SW_STATE_DISCONNECTED==swState)?"DISCONNECTED":"UNKNOWN");

        writer.Key("DPID");
        writer.String(dpidUint8ToStr(sw->getDpid(), json_tmp));

        writer.Key("inetAddr");
        number2ip(htonl(sw->getSwIp()), json_tmp);
        sprintf(json_tmp, "%s:%d", json_tmp, ntohs(sw->getSwPort()));
        writer.String(json_tmp);

        writer.Key("mfrDesc");
        writer.String(sw->getSwDesc().mfr_desc);

        writer.Key("hwDesc");
        writer.String(sw->getSwDesc().hw_desc);

        writer.Key("swDesc");
        writer.String(sw->getSwDesc().sw_desc);

        writer.Key("serialNum");
        writer.String(sw->getSwDesc().serial_num);

        writer.Key("dpDesc");
        writer.String(sw->getSwDesc().dp_desc);

        writer.Key("buffers");
        writer.Int(sw->getNBuffers());

        writer.Key("ports");
        writer.StartArray();
        {
            sw->lockPorts();

            CPortMap portMap = sw->getPorts();
            for (CPortMap::iterator it = portMap.begin(); TRUE; ++it)
            {
                gn_port_t *port = NULL;
                if (it != portMap.end())
                {
                    port = &(it->second);
                    if (PORT_STATE_DELETED == port->state)
                        continue;
                }
                else
                {
                    port = &sw->getLoPort();
                }

                writer.StartObject();

                writer.Key("name");
                writer.String(port->name);

                writer.Key("type");
                writer.Int(port->type);

                writer.Key("state");
                writer.Int(port->state);

                writer.Key("hwAddr");
                writer.String(mac2str(port->hw_addr, json_tmp));

                writer.Key("portNo");
                sprintf(json_tmp, "%x", port->port_no);
                writer.String(json_tmp);

                writer.Key("config");
                writer.Int(port->config);

                writer.Key("currentFeatures");
                writer.Int(port->curr);

                writer.Key("advertisedFeatures");
                writer.Int(port->advertised);

                writer.Key("supportedFeatures");
                writer.Int(port->supported);

                writer.Key("peerFeatures");
                writer.Int(port->peer);

                writer.EndObject();

                if (it == portMap.end())
                    break;
            }
            sw->unlockPorts();
        }

        writer.EndArray();

        writer.Key("openflow");
        writer.String((sw->getVersion()==0x01)?"of1.0":"of1.3");

        writer.Key("connectedSince");
        writer.Uint(0);

        writer.EndObject();
    }

	CControl::getInstance()->getSwitchMgr().unlock();

    writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();

    body.append(strBuff.GetString());

    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_switch_info(CRestRequest* request, CRestResponse* response)
{
	std::string body;
    INT1 json_tmp[1024] = {0};

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();

	UINT8 dpid = 0;
	std::string pathHead = "/gn/switch/json/";
    std::string dpidStr = request->getPath().substr(pathHead.length());

	LOG_WARN_FMT("path: %s", request->getPath().c_str());
    dpidStr2Uint8(dpidStr.c_str(), &dpid);
    LOG_WARN_FMT("dpid: %s(0x%llx)", dpidStr.c_str(), dpid);

	if (0 != dpid)
	{
        CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
        if (sw.isNotNull())
        {
            writer.Key("DPID");
            writer.String(dpidUint8ToStr(dpid, json_tmp));
            
            writer.Key("inetAddr");
            number2ip(htonl(sw->getSwIp()), json_tmp);
            sprintf(json_tmp, "%s:%d", json_tmp, ntohs(sw->getSwPort()));
            writer.String(json_tmp);

            writer.Key("mfrDesc");
            writer.String(sw->getSwDesc().mfr_desc);

            writer.Key("hwDesc");
            writer.String(sw->getSwDesc().hw_desc);

            writer.Key("swDesc");
            writer.String(sw->getSwDesc().sw_desc);

            writer.Key("serialNum");
            writer.String(sw->getSwDesc().serial_num);

            writer.Key("dpDesc");
            writer.String(sw->getSwDesc().dp_desc);

            writer.Key("buffers");
            writer.Uint(sw->getNBuffers());

            writer.Key("openflow");
            writer.String((sw->getVersion()==0x01)?"of1.0":"of1.3");

            writer.Key("connectedSince");
            writer.Uint(0);
        }
	}

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

	writer.EndObject();

	body.append(strBuff.GetString());
	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}


void CWebRestApi::api_get_topo_link_info(CRestRequest* request, CRestResponse* response)
{
	std::string body;
    INT1 json_tmp[1024] = {0};
    UINT1 dpid[8] = {0};

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("linkTopo");
    writer.StartArray();

	CSwitchHMap& swMap = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    STL_FOR_LOOP(swMap, it)
    {
        CSmartPtr<CSwitch> sw = it->second;
        if (sw.isNull() || (sw->getState() >= SW_STATE_POWER_OFF))
            continue;

		 writer.StartObject();
         writer.Key("srcDPID");
         {
             memset(dpid, 0, 8);
             memset(json_tmp, 0, 1024);
             uint8ToStr(sw->getDpid(), dpid);
             sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                               dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                               dpid[7]);

         }
		 writer.String(json_tmp);
		 writer.Key("neighbors");
         writer.StartArray();
		 CNeighbors* neighLinks =  CControl::getInstance()->getTopoMgr().getLinks(sw->getDpid());
		 if (NULL != neighLinks)
	     {
	     	STL_FOR_LOOP(*neighLinks, iter)
     		{
     			//if(NULL != iter->second)
     			{
     				neighbor_t& neigh = iter->second;
					if (NEIGH_STATE_DELETED == neigh.state)
                   		continue;
                	if (!CControl::getInstance()->getTopoMgr().isLinkUp(neigh.src_dpid, neigh.src_port))
                    	continue;
					
					writer.StartObject();
					
					writer.Key("srcPort");
					writer.Int64(neigh.src_port);
	
					writer.Key("dstDPID");
					{
						memset(dpid, 0, 8);
						memset(json_tmp, 0, 1024);
						uint8ToStr(neigh.dst_dpid, dpid);
						sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
										   dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
										   dpid[7]);
	
					}
					writer.String(json_tmp);
	
					writer.Key("dstPort");
					writer.Int64(neigh.dst_port);
	
					writer.Key("state");
					writer.String(
						(NEIGH_STATE_ESTABLISHED==neigh.state)?"ESTABLISHED":
						(NEIGH_STATE_DELETED==neigh.state)?"DELETED":"UNALIVE");
	
					writer.EndObject();
     			}
				
     		}
	       
	     }
		 writer.EndArray();
         writer.EndObject();
		 
    }

	CControl::getInstance()->getSwitchMgr().unlock();

	writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();

    body.append(strBuff.GetString());

    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_topo_link(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};
    UINT1 dpid[8] = {0};

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("linkTopo");
    writer.StartArray();

    std::vector<CNeighborMap>& neighMapVec = CControl::getInstance()->getTopoMgr().getNeighMapVector();
    STL_FOR_LOOP(neighMapVec, neighMapIt)
    {
        STL_FOR_LOOP(*neighMapIt, dpidIt)
        {
            CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpidIt->first);
            if (sw.isNull() || (sw->getState() >= SW_STATE_POWER_OFF))
                continue;
            
            writer.StartObject();
            writer.Key("srcDPID");
            {
                memset(dpid, 0, 8);
                memset(json_tmp, 0, 1024);
                uint8ToStr(dpidIt->first, dpid);
                sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                                   dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                                   dpid[7]);

            }
            writer.String(json_tmp);

            writer.Key("neighbors");
            writer.StartArray();

            CNeighbors& portMap = dpidIt->second;
            STL_FOR_LOOP(portMap, portIt)
            {
                neighbor_t& neigh = portIt->second;
                if (NEIGH_STATE_DELETED == neigh.state)
                    continue;
                if (!CControl::getInstance()->getTopoMgr().isLinkUp(neigh.src_dpid, neigh.src_port))
                    continue;

                writer.StartObject();

                writer.Key("srcPort");
                writer.Int64(neigh.src_port);

                writer.Key("dstDPID");
                {
                    memset(dpid, 0, 8);
                    memset(json_tmp, 0, 1024);
                    uint8ToStr(neigh.dst_dpid, dpid);
                    sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                                       dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                                       dpid[7]);

                }
                writer.String(json_tmp);

                writer.Key("dstPort");
                writer.Int64(neigh.dst_port);

                writer.Key("state");
                writer.String(
                    (NEIGH_STATE_ESTABLISHED==neigh.state)?"ESTABLISHED":
                    (NEIGH_STATE_DELETED==neigh.state)?"DELETED":"UNALIVE");

                writer.EndObject();

            }
            writer.EndArray();
            writer.EndObject();
        }    
    }

    writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();

    body.append(strBuff.GetString());

    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_host_by_dpid(CRestRequest* request, CRestResponse* response)
{
	std::string body;
    INT1 json_tmp[1024] = {0};

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("host list");
	writer.StartArray();

	//get dpid

	std::string path = request->getPath();
	LOG_INFO_FMT("path:%s",path.c_str());
	std::string pathhead = "/dcf/get/host/json/";
	size_t headlen = pathhead.length();
	std::string dpidStr = path.substr(headlen);
	LOG_INFO_FMT("dpidStr:%s",dpidStr.c_str());

	UINT8 dpid;
	if (dpidStr2Uint8(dpidStr.c_str(), &dpid) == -1)
	{
		writer.String("invalid dpid");
	}
	else
	{
        CHostList& hostList = CHostMgr::getInstance()->getHostList();
		STL_FOR_LOOP(hostList, iter)
		{
            CSmartPtr<CHost> host = *iter;

			if (host.isNotNull()&&host->getSw().isNotNull()&&(host->getSw()->getDpid() == dpid))
			{
				writer.StartObject();
				if (CControl::getInstance()->isL3ModeOn())
				{
					switch (host->getHostType())
					{
					case bnc::host::HOST_NORMAL:
						sprintf(json_tmp,"%s","Host");
						break;
					case bnc::host::OPENSTACK_HOST:
						sprintf(json_tmp,"%s","Openstack Host");
						break;
					default:
						sprintf(json_tmp,"%s","unknown");
						break;

					}
					writer.Key("Type");
					writer.String(json_tmp);

					if (host->getSw().isNotNull())
					{
						writer.Key("SwIP");
						{
							memset(json_tmp,0,1024);
							number2ip(ntohl(host->getSw()->getSwIp()),json_tmp);
						}
						writer.String(json_tmp);
					}

					writer.Key("IPv4");
					{
						memset(json_tmp,0,1024);
						number2ip(host->getIp(),json_tmp);
					}
					writer.String(json_tmp);

					writer.Key("Mac");
					{
						memset(json_tmp,0,1024);
						mac2str(host->getMac(),json_tmp);
					}
					writer.String(json_tmp);

					writer.Key("port");
					sprintf(json_tmp,"%d",host->getPortNo());
					writer.String(json_tmp);


				}
				else
				{
					writer.Key("IPv4");
					{
						memset(json_tmp,0,1024);
						number2ip(host->getIp(),json_tmp);
					}
					writer.String(json_tmp);

					writer.Key("Mac");
					{
						memset(json_tmp,0,1024);
						mac2str(host->getMac(),json_tmp);
					}
					writer.String(json_tmp);

					writer.Key("port");
					sprintf(json_tmp,"%d",host->getPortNo());
					writer.String(json_tmp);
				}
				writer.EndObject();
			}
		}
    }
	writer.EndArray();

	writer.Key("retCode");
	writer.Int(0);

	writer.Key("retMsg");
	writer.String("OK");

	writer.EndObject();

	body.append(strBuff.GetString());
	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_qosrules(CRestRequest* request, CRestResponse* response)
{
	std::string body;
	INT1 json_tmp[1024] = {0};

	std::map<string, COpenstackQosRule* > qosRulesMap = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getOpenstackQosRuleList();

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("QosRules");
	writer.StartArray();

	STL_FOR_LOOP(qosRulesMap, iter)
	{
		writer.StartObject();

		COpenstackQosRule* qosRule = iter->second;

		writer.Key("qos_id");
		{
			memset(json_tmp, 0, 1024);
			memcpy(json_tmp,qosRule->GetQosId().c_str(),qosRule->GetQosId().size());
		}
		writer.String(json_tmp);


		writer.Key("rule_name");
		{
			memset(json_tmp,0,1024);
			memcpy(json_tmp,qosRule->GetRuleName().c_str(),qosRule->GetRuleName().size());
		}
		writer.String(json_tmp);

		writer.Key("descript");
		{
			memset(json_tmp,0,1024);
			memcpy(json_tmp,qosRule->GetDescript().c_str(),qosRule->GetDescript().size());
		}
		writer.String(json_tmp);

		writer.Key("tenant_id");
		{
			memset(json_tmp,0,1024);
			memcpy(json_tmp,qosRule->GetTenantId().c_str(),qosRule->GetTenantId().size());
		}
		writer.String(json_tmp);

		writer.Key("protocol");
		{
			memset(json_tmp,0,1024);
			memcpy(json_tmp,qosRule->GetProtocol().c_str(),qosRule->GetProtocol().size());
		}
		writer.String(json_tmp);

		writer.Key("type");
		{
			memset(json_tmp,0,1024);
			memcpy(json_tmp,qosRule->GetQosType().c_str(),qosRule->GetQosType().size());
		}
		writer.String(json_tmp);

		writer.Key("derection");
		{
			memset(json_tmp,0,1024);
			(TRUE == qosRule->GetDerection()) ? sprintf(json_tmp, "ingress"): sprintf(json_tmp, "egress");
		}
		writer.String(json_tmp);
		
		writer.Key("max_rate");
		{
			memset(json_tmp,0,1024);
			sprintf(json_tmp, "%llu", qosRule->GetMaxRate());
		}
		writer.String(json_tmp);


		writer.EndObject();
	}
	writer.EndArray();

	writer.Key("retCode");
	writer.Int(0);

	writer.Key("retMsg");
	writer.String("OK");

	writer.EndObject();
	body.append(strBuff.GetString());
	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_qosbind_ports(CRestRequest* request, CRestResponse* response)
{
	std::string body;
	INT1 json_tmp[1024] = {0};

	std::map<string, COpenstackQosBind* > qosBindMap = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getOpenstackQosBindList();

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("QosRules");
	writer.StartArray();

	STL_FOR_LOOP(qosBindMap, iter)
	{
		writer.StartObject();

		COpenstackQosBind* qosBind = iter->second;

		writer.Key("port_id");
		{
			memset(json_tmp, 0, 1024);
			memcpy(json_tmp,qosBind->GetPortId().c_str(),qosBind->GetPortId().size());
		}
		writer.String(json_tmp);


		writer.Key("ingress_id");
		{
			memset(json_tmp,0,1024);
			memcpy(json_tmp,qosBind->GetIngressQosId().c_str(),qosBind->GetIngressQosId().size());
		}
		writer.String(json_tmp);

		writer.Key("egress_id");
		{
			memset(json_tmp,0,1024);
			memcpy(json_tmp,qosBind->GetEgressQosId().c_str(),qosBind->GetEgressQosId().size());
		}
		writer.String(json_tmp);

		writer.Key("port_ip");
		{
			memset(json_tmp,0,1024);
			number2ip(qosBind->GetPortIp(), json_tmp);
		}
		writer.String(json_tmp);

		
		writer.EndObject();
	}
	writer.EndArray();

	writer.Key("retCode");
	writer.Int(0);

	writer.Key("retMsg");
	writer.String("OK");

	writer.EndObject();
	body.append(strBuff.GetString());
	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_setup_fabric_external(CRestRequest* request, CRestResponse* response)
{
    UINT4 gatwayip=0;
    UINT1 gateway_mac[6]={0};
    UINT4 outip=0;
    UINT1 outer_mac[6]={0};
    UINT8 dpid = 0;
    UINT4 port=0;
    std::string subnet_id;

    std::string body = request->getBody();
    LOG_INFO_FMT("body string is :%s",body.c_str());
    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        LOG_INFO("external api parse error");
        return ;
    }

    if (document.HasMember("bandDpid") && document["bandDpid"].IsString())
    {
        if (document["bandDpid"].GetString() != NULL)
            dpidStr2Uint8(document["bandDpid"].GetString(),&dpid);
    }

    if (document.HasMember("bandPort") && document["bandPort"].IsString())
    {
        port = (NULL == document["bandPort"].GetString()) ? 0 : atoi(document["bandPort"].GetString());
    }

    if (document.HasMember("gatwayip") && document["gatwayip"].IsString())
    {
        if (document["gatwayip"].GetString() != NULL)
            gatwayip = ip2number(document["gatwayip"].GetString());
    }

    if (document.HasMember("outer_interface_ip") && document["outer_interface_ip"].IsString())
    {
        if (document["outer_interface_ip"].GetString() != NULL)
            outip = ip2number(document["outer_interface_ip"].GetString());
    }

    if (document.HasMember("gatewaymac") && document["gatewaymac"].IsString())
    {
        if (document["gatewaymac"].GetString() != NULL)
            macstr2hex(document["gatewaymac"].GetString(), gateway_mac);
    }

    if (document.HasMember("mac") && document["mac"].IsString())
    {
        if (document["mac"].GetString() != NULL)
            macstr2hex(document["mac"].GetString(), outer_mac);
    }

    if (document.HasMember("subnetid") && document["subnetid"].IsString())
    {
        if (document["subnetid"].GetString() != NULL)
            subnet_id = document["subnetid"].GetString();
    }

    COpenstackExternalMgr::getInstance()->addOpenstackExternal(subnet_id,gatwayip,outip,gateway_mac,outer_mac,dpid,port);
    body.append("");
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);

}

void CWebRestApi::api_delete_fabric_external(CRestRequest* request, CRestResponse* response)
{
    UINT4 outip=0;
    std::string body = request->getBody();
    LOG_INFO_FMT("delete external body is :%s",body.c_str());
    Document document;

    if (document.Parse(body.c_str()).HasParseError())
    {
        LOG_INFO("external api parse error");
        return ;
    }

    if (document.HasMember("outer_interface_ip") && document["outer_interface_ip"].IsString())
    {
        if (document["outer_interface_ip"].GetString() != NULL)
            outip = ip2number(document["outer_interface_ip"].GetString());
    }

    COpenstackExternalMgr::getInstance()->removeOpenstackExternalByOuterIp(outip);
    body.append("");
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_fabric_external(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};

    std::list<COpenstackExternal* > externalList = COpenstackExternalMgr::getInstance()->getOpenstackExternalList();

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("external list");
    writer.StartArray();

    STL_FOR_LOOP(externalList, iter)
    {
        writer.StartObject();

        COpenstackExternal* external = *iter;

        writer.Key("bandDpid");
        {
            UINT1 dpid[8] = {0};
            memset(json_tmp, 0, 1024);

            uint8ToStr(external->getExternalDpid(), dpid);
            sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                               dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                               dpid[7]);
        }
        writer.String(json_tmp);

        writer.Key("bandPort");
        memset(json_tmp, 0, 1024);
        sprintf(json_tmp,"%d",external->getExternalPort());
        writer.String(json_tmp);

        writer.Key("gatwayip");
        {
            memset(json_tmp,0,1024);
            number2ip(external->getExternalGatewayIp(),json_tmp);
        }
        writer.String(json_tmp);

        writer.Key("outer_interface_ip");
        {
            memset(json_tmp,0,1024);
            number2ip(external->getExternalOuterInterfaceIp(),json_tmp);
        }
        writer.String(json_tmp);

        writer.Key("gatewaymac");
        {
            memset(json_tmp,0,1024);
            mac2str(external->getExternalGatewayMac(),json_tmp);
        }
        writer.String(json_tmp);

        writer.Key("mac");
        {
            memset(json_tmp,0,1024);
            mac2str(external->getExternalOuterInterfaceMac(),json_tmp);
        }
        writer.String(json_tmp);

        writer.Key("subnetid");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,external->getExternalSubnetId().c_str(),external->getExternalSubnetId().size());
        }
        writer.String(json_tmp);

        writer.EndObject();
    }

    writer.EndArray();



    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();
    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_fabric_floatingip(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};

    std::map<string, COpenstackFloatingip* > floatingipList = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getOpenstackFloatingipList();

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("floatingips");
    writer.StartArray();

    STL_FOR_LOOP(floatingipList, iter)
    {
        writer.StartObject();

        COpenstackFloatingip* floatingip = iter->second;

        writer.Key("port_id");
        {
            memset(json_tmp, 0, 1024);
            memcpy(json_tmp,floatingip->getPortId().c_str(),floatingip->getPortId().size());
        }
        writer.String(json_tmp);

        writer.Key("floating_ip_address");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,floatingip->getFloatingIp().c_str(),floatingip->getFloatingIp().size());
//            LOG_INFO_FMT("floating ip is: %d",ip2number(floatingip->getFloatingIp().c_str()));
        }
        writer.String(json_tmp);

        writer.Key("fixed_ip_address");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,floatingip->getFixedIp().c_str(),floatingip->getFixedIp().size());
//            LOG_INFO_FMT("fixed ip is: %d",ip2number(floatingip->getFixedIp().c_str()));
        }
        writer.String(json_tmp);

        writer.Key("router_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,floatingip->getRouterId().c_str(),floatingip->getRouterId().size());
        }
        writer.String(json_tmp);

        writer.Key("tenant_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,floatingip->getTenantId().c_str(),floatingip->getTenantId().size());
        }
        writer.String(json_tmp);

        writer.Key("floating_network_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,floatingip->getFloatingNetId().c_str(),floatingip->getFloatingNetId().size());
        }
        writer.String(json_tmp);

        writer.Key("id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,floatingip->getId().c_str(),floatingip->getId().size());
        }
        writer.String(json_tmp);

        writer.Key("status");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,floatingip->getStatus().c_str(),floatingip->getStatus().size());
        }
        writer.String(json_tmp);

        writer.Key("floating_mac");
        {
            memset(json_tmp,0,1024);
            CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByIp(ip2number(floatingip->getFloatingIp().c_str()));
			if(host.isNotNull())
			{
				mac2str(host->getMac(),json_tmp);
			}
        }
        writer.String(json_tmp);

        writer.Key("fixed_mac");
        {
            memset(json_tmp,0,1024);
            CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByIp(ip2number(floatingip->getFixedIp().c_str()));
			if(host.isNotNull())
			{
            	mac2str(host->getMac(),json_tmp);
			}
        }
        writer.String(json_tmp);

//        //测试�?
//        writer.Key("find floating ip test");
//        writer.Int(COpenstackMgr::getInstance()->getOpenstack()->getResource()->getFloatingipbyFixedIp(553648328));
//
//        writer.Key("find fixed ip test");
//        writer.Int(COpenstackMgr::getInstance()->getOpenstack()->getResource()->getFixedipByFloatingip(2016782528));

        writer.EndObject();
    }
    writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();
    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_fabric_security_group(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};

    const COpenstackSecurityGroupMap& securityGroupList = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getOpenstackSecurityGroupList();

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("security_group_rules");
    writer.StartArray();

    STL_FOR_LOOP(securityGroupList, iter)
    {
        writer.StartObject();

        CSmartPtr<COpenstackSecurityGroup> securityGroup = iter->second;

        writer.Key("direction");
        {
            memset(json_tmp, 0, 1024);
            memcpy(json_tmp,securityGroup->getDirection().c_str(),securityGroup->getDirection().size());
        }
        writer.String(json_tmp);

        writer.Key("ethertype");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,securityGroup->getEthertype().c_str(),securityGroup->getEthertype().size());
        }
        writer.String(json_tmp);

        writer.Key("id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,securityGroup->getId().c_str(),securityGroup->getId().size());
        }
        writer.String(json_tmp);

        writer.Key("port_range_max");
        writer.Int(securityGroup->getPortRangeMax());

        writer.Key("tenant_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,securityGroup->getTenantId().c_str(),securityGroup->getTenantId().size());
        }
        writer.String(json_tmp);

        writer.Key("port_range_min");
        writer.Int(securityGroup->getPortRangeMin());

        writer.Key("protocol");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,securityGroup->getProtocol().c_str(),securityGroup->getProtocol().size());
        }
        writer.String(json_tmp);

        writer.Key("remote_group_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,securityGroup->getRemoteGroupId().c_str(),securityGroup->getRemoteGroupId().size());
        }
        writer.String(json_tmp);

        writer.Key("remote_ip_prefix");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,securityGroup->getRemoteIpPrefix().c_str(),securityGroup->getRemoteIpPrefix().size());
        }
        writer.String(json_tmp);

        writer.Key("security_group_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,securityGroup->getSecurityGroupId().c_str(),securityGroup->getSecurityGroupId().size());
        }
        writer.String(json_tmp);

        writer.EndObject();
    }
    writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();
    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_fabric_network(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};

    std::map<string, COpenstackNetwork* > networkList = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getOpenstackNetworkList();

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("networks");
    writer.StartArray();

    STL_FOR_LOOP(networkList, iter)
    {
        writer.StartObject();

        COpenstackNetwork* network = iter->second;

        writer.Key("status");
        {
            memset(json_tmp, 0, 1024);
            memcpy(json_tmp,network->getStatus().c_str(),network->getStatus().size());
        }
        writer.String(json_tmp);

        writer.Key("name");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,network->getName().c_str(),network->getName().size());
        }
        writer.String(json_tmp);

        writer.Key("provider:physical_network");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,network->getProviderPhysicalNetwork().c_str(),network->getProviderPhysicalNetwork().size());
        }
        writer.String(json_tmp);

        writer.Key("admin_state_up");
        writer.Bool(network->getAdminStateUp());

        writer.Key("tenant_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,network->getTenantId().c_str(),network->getTenantId().size());
        }
        writer.String(json_tmp);

        writer.Key("qos_policy_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,network->getQosPolicyId().c_str(),network->getQosPolicyId().size());
        }
        writer.String(json_tmp);

        writer.Key("id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,network->getId().c_str(),network->getId().size());
        }
        writer.String(json_tmp);

        writer.Key("router:external");
        writer.Bool(network->getRouterExternal());

        writer.Key("provider:network_type");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,network->getProviderNetworkType().c_str(),network->getProviderNetworkType().size());
        }
        writer.String(json_tmp);

        writer.Key("port_security_enabled");
        writer.Bool(network->getPortSecurityEnabled());

        writer.Key("mtu");
        writer.Int(network->getMtu());

        writer.Key("shared");
        writer.Bool(network->getShared());

        writer.Key("provider:segmentation_id");
        writer.Int(network->getProviderSegmentationId());

        writer.Key("subnets");
        {
            writer.StartArray();
            std::list<std::string> subnetslist = network->getSubnetsList();
            STL_FOR_LOOP(subnetslist, iter)
            {
                writer.String(iter->c_str());
            }
            writer.EndArray();
        }
        writer.EndObject();
    }
    writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();
    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_get_fabric_subnet(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};

    std::map<string, COpenstackSubnet* > subnetList = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getOpenstackSubnetList();

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("subnets");
    writer.StartArray();

    STL_FOR_LOOP(subnetList, iter)
    {
        writer.StartObject();

        COpenstackSubnet* subnet = iter->second;

        writer.Key("enable_dhcp");
        writer.Bool(subnet->getEnableDhcp());

        writer.Key("name");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,subnet->getName().c_str(),subnet->getName().size());
        }
        writer.String(json_tmp);

        writer.Key("network_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,subnet->getNetworkId().c_str(),subnet->getNetworkId().size());
        }
        writer.String(json_tmp);

        writer.Key("tenant_id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,subnet->getTenantId().c_str(),subnet->getTenantId().size());
        }
        writer.String(json_tmp);

//        writer.Key("dns_nameservers");
//       {
//           writer.StartArray();
//           std::list<std::string> nameservers = subnet->getDnsNameserversList();
//           STL_FOR_LOOP(nameservers, iter)
//           {
//               writer.String(iter->c_str());
//           }
//           writer.EndArray();
//       }

        writer.Key("id");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,subnet->getId().c_str(),subnet->getId().size());
        }
        writer.String(json_tmp);

        writer.Key("ip_version");
        writer.Int(subnet->getIpVersion());

//        writer.Key("host_routes");
//        {
//           writer.StartArray();
//           std::list<std::string> host_routes = subnet->getHostRoutes();
//           STL_FOR_LOOP(host_routes, iter)
//           {
//               writer.String(iter->c_str());
//           }
//           writer.EndArray();
//       }

        writer.Key("gateway_ip");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,subnet->getGatewayIp().c_str(),subnet->getGatewayIp().size());
        }
        writer.String(json_tmp);

        writer.Key("cidr");
        {
            memset(json_tmp,0,1024);
            memcpy(json_tmp,subnet->getCidr().c_str(),subnet->getCidr().size());
        }
        writer.String(json_tmp);

        writer.Key("allocation_pools");
        {
            writer.StartArray();
            map<string,string> map_pools = subnet->getAlloctionPools();
            STL_FOR_LOOP(map_pools, iter)
            {
                writer.StartObject();
                writer.Key("start");
                writer.String((*iter).first.c_str());
                writer.Key("end");
                writer.String((*iter).second.c_str());
                writer.EndObject();
            }
            writer.EndArray();
        }
        writer.EndObject();

    }
    writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();
    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);

}

void CWebRestApi::api_extend_sws_get(CRestRequest* request, CRestResponse* response)
{
	string body;
	UINT1  mac[MAC_LEN] ={0};
	INT1 json_tmp[1024]={0};
	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("External_SwitchInfo");
	{
		writer.StartArray();
		{
            CSwitchHMap& swMap = CControl::getInstance()->getSwitchMgr().getSwitchMap();
            CControl::getInstance()->getSwitchMgr().lock();
            
			STL_FOR_LOOP(swMap, it)
			{
				CSmartPtr<CSwitch> sw = it->second;
		        if (sw.isNull() || (sw->getState() >= SW_STATE_POWER_OFF))
		            continue;

				writer.StartObject();
				{
//-------------------------------------------------------------------------------------------------------
					writer.Key("DPID");
					UINT1 dpid[8] = {0};
            		memset(json_tmp, 0, 1024);

            		uint8ToStr(sw->getDpid(), dpid);
            		sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                               dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                               dpid[7]);
					writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
					writer.Key("SW_IP");
					memset(json_tmp, 0, 1024);
					number2ip(htonl(sw->getSwIp()), json_tmp);
					sprintf(json_tmp, "%s", json_tmp);
					writer.String(json_tmp);
					
//-------------------------------------------------------------------------------------------------------
					writer.Key("SW_MAC");
					memset(json_tmp,0,1024);
					memcpy(&mac, sw->getSwMac(), MAC_LEN);
					sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
                    mac[0], mac[1],mac[2], mac[3],mac[4], mac[5]);
               	 	writer.String(json_tmp);
				
//-------------------------------------------------------------------------------------------------------
				}
				writer.EndObject();
			}

            CControl::getInstance()->getSwitchMgr().unlock();
		}
		writer.EndArray();

		writer.Key("retCode");
		writer.Int(0);
		writer.Key("retMsg");
		writer.String("OK");
	}
	writer.EndObject();
	
	body.append(strBuff.GetString());
	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_extend_sws_post(CRestRequest* request, CRestResponse* response)
{
	BOOL returnResult = BNC_ERR;
	UINT4 ip = 0;
	UINT1 mac[6] = {0};
	UINT8 switch_dpid = 0;
	UINT1 dpid_switch[8] = {0};
	string str_mac, str_ip, str_dpid;
	string body = request->getBody();

	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
	   LOG_ERROR_FMT("body.c_str()=%s", body.c_str());
	   returnResult = BNC_ERR  ;
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, body);
	   return;
	}
	if(document.HasMember("DPID")&& document["DPID"].IsString())
	{
		Value& s = document["DPID"];
		str_dpid = s.GetString();
		dpid_str_to_bin((char *)str_dpid.c_str(), dpid_switch);
		uc8_to_ulli64 ((const UINT1*)dpid_switch, &switch_dpid);
		//LOG_ERROR_FMT("dpid=%s", str_dpid);
	}
	if(document.HasMember("SW_MAC")&& document["SW_MAC"].IsString())
	{
		Value& s = document["SW_MAC"];
		str_mac = s.GetString();
		macstr2hex(str_mac.c_str(),mac);
		//LOG_ERROR_FMT("mac=%s", str_mac.c_str());
	}
	if(document.HasMember("SW_IP")&& document["SW_IP"].IsString())
	{
		Value& s = document["SW_IP"];
		str_ip = s.GetString();
		ip = ntohl(ip2number(str_ip.c_str()));
		//LOG_ERROR_FMT("ip=%s", str_ip);
	}
	LOG_ERROR_FMT("str_dpid=%s, str_mac=%s str_ip=%s",str_dpid.c_str(),str_mac.c_str(), str_ip.c_str());
	
    LOG_ERROR_FMT("mac[0]=0x%x mac[1]=0x%x mac[4]=0x%x mac[5]=0x%x ip=0x%x",mac[0],mac[1],mac[4],mac[5],ip);
	if(0 != ip)
	{
		CControl::getInstance()->getSwitchMgr().addSwitch(ip&0x00ff,(const INT1*)mac,ip, 0);
		CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByMac( (const INT1*)mac);
		if(sw.isNotNull())
		{
			sw->setDpid(switch_dpid);
			sw->setSwPort((UINT2)ip&&0xff);

			switch_desc_t swDesc = {0};
			strcpy(swDesc.mfr_desc, "OpenSwitch");
			strcpy(swDesc.hw_desc, "FD-1000");
			strcpy(swDesc.sw_desc, "FDOS 1.0.1");
			sw->setSwDesc(swDesc);
		}
		returnResult = BNC_OK;
	}
	else
	{
		body.assign("Switch ip can't be 0!!!");
	}
	body.append("");
	if(BNC_OK == returnResult)
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}

void CWebRestApi::api_extend_sws_delete(CRestRequest* request, CRestResponse* response)
{
	UINT8 switch_dpid = 0;
	string str_dpid;
	UINT1 dpid_switch[8] = {0};
	BOOL returnResult = BNC_OK;
	string body = request->getBody();

	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
	   LOG_ERROR_FMT("body.c_str()=%s", body.c_str());
	   returnResult = BNC_ERR  ;
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, body);
	   return;
	}
	if(document.HasMember("DPID")&& document["DPID"].IsString())
	{
		Value& s = document["DPID"];
		str_dpid = s.GetString();
		dpid_str_to_bin((char *)str_dpid.c_str(), dpid_switch);
		uc8_to_ulli64 ((const UINT1*)dpid_switch, &switch_dpid);
		//LOG_ERROR_FMT("dpid=%s", str_dpid);
	}
	CControl::getInstance()->getSwitchMgr().delSwitchByDpid(switch_dpid);
	
	body.append("");
	if(BNC_OK == returnResult)
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}

void CWebRestApi::api_extend_hosts_get(CRestRequest* request, CRestResponse* response)
{
	string body;
	UINT1  mac[MAC_LEN] ={0};
	INT1 json_tmp[1024]={0};
	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("External_HostInfo");
	{
		writer.StartArray();
		{
            CHostList& hostList = CHostMgr::getInstance()->getHostList();
			STL_FOR_LOOP(hostList, iter)
			{
				writer.StartObject();
				{
					CSmartPtr<CHost> host = *iter;
                    if (host.isNull())
                        continue;
					
					writer.Key("MAC");
					memset(json_tmp,0,1024);
					memcpy(&mac, host->getMac(), MAC_LEN);
					sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
					mac[0], mac[1],mac[2], mac[3],mac[4], mac[5]);
					writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------

					writer.Key("IP");
					memset(json_tmp, 0, 1024);
					number2ip(host->getfixIp(), json_tmp);
					sprintf(json_tmp, "%s", json_tmp);
					writer.String(json_tmp);


					writer.Key("DPID");
					UINT1 dpid[8] = {0};
					memset(json_tmp, 0, 1024);

					uint8ToStr(host->getDpid(), dpid);
					sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
							   dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
							   dpid[7]);
					writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
					writer.Key("TYPE");
					writer.Int(host->getHostType());
					
					writer.Key("PORT");
					writer.Int(host->getPortNo());


					writer.Key("TENANT_ID");
					writer.String(host->getTenantId().c_str());

					//writer.Key("NETWORK_ID");
					//writer.String(host->getTenantId());

					writer.Key("SUBNET_ID");
					writer.String(host->getSubnetId().c_str());

					//writer.Key("DEVICE_ID");
					//writer.String(host->getd());
					
			
//-------------------------------------------------------------------------------------------------------
				}
				writer.EndObject();
			}
		}
		writer.EndArray();

		writer.Key("retCode");
		writer.Int(0);
		writer.Key("retMsg");
		writer.String("OK");
	}
	writer.EndObject();
	LOG_ERROR_FMT("%s", strBuff.GetString());
	body.append(strBuff.GetString());
	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);

}

void CWebRestApi::api_extend_hosts_post(CRestRequest* request, CRestResponse* response)
{
	UINT1 type = 0;
	UINT2 port = 0;
	UINT4 ip = 0;
	UINT1 mac[6] = {0};
	UINT8 switch_dpid = 0;
	UINT1 dpid_switch[8] = {0};
	string str_mac, str_ip, str_dpid, str_type, str_port, str_networkid, str_subnetid, str_tenantid, str_name, str_deviceid ;
	BOOL returnResult = BNC_ERR;
	string body = request->getBody();

	LOG_ERROR_FMT("body.c_str()=%s", body.c_str());
	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
	   LOG_ERROR_FMT("body.c_str()=%s", body.c_str());
	   returnResult = BNC_ERR  ;
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, body);
	   return;
	}
	if(document.HasMember("MAC")&& document["MAC"].IsString())
	{
		Value& s = document["MAC"];
		str_mac = s.GetString();
		macstr2hex(str_mac.c_str(),mac);
		//LOG_ERROR_FMT("mac=%s", str_mac.c_str());
	}
	if(document.HasMember("IP")&& document["IP"].IsString())
	{
		Value& s = document["IP"];
		str_ip = s.GetString();
		ip = ip2number(str_ip.c_str());
		//LOG_ERROR_FMT("ip=%s", str_ip);
	}
	if(document.HasMember("DPID")&& document["DPID"].IsString())
	{
		Value& s = document["DPID"];
		str_dpid = s.GetString();
		dpid_str_to_bin((char *)str_dpid.c_str(), dpid_switch);
		uc8_to_ulli64 ((const UINT1*)dpid_switch, &switch_dpid);
		//LOG_ERROR_FMT("dpid=%s", str_dpid);
	}
	if(document.HasMember("TYPE")&& document["TYPE"].IsString())
	{
		Value& s = document["TYPE"];
		str_type = s.GetString();
		type = atoi(str_type.c_str());
		//LOG_ERROR_FMT("type=%s", s.GetString());
	}
	if(document.HasMember("PORT")&& document["PORT"].IsString())
	{
		Value& s = document["PORT"];
		str_port = s.GetString();
		port = atoi(str_port.c_str());
		//LOG_ERROR_FMT("port=%s", s.GetString());
	}
	if(document.HasMember("NAME")&& document["NAME"].IsString())
	{
		Value& s= document["NAME"];
		str_name = s.GetString();
	}
	if(document.HasMember("NETWORK_ID")&& document["NETWORK_ID"].IsString())
	{
		Value& s= document["NETWORK_ID"];
		str_networkid = s.GetString();
	}
	if(document.HasMember("SUBNET_ID")&& document["SUBNET_ID"].IsString())
	{
		Value& s=document["SUBNET_ID"];
		str_subnetid = s.GetString();
	}
	if(document.HasMember("TENANT_ID")&& document["TENANT_ID"].IsString())
	{
		Value& s = document["TENANT_ID"];
		str_tenantid = s.GetString();
	}

	if(document.HasMember("DEVICE_ID")&& document["DEVICE_ID"].IsString())
	{
		Value& s = document["DEVICE_ID"];
		str_deviceid = s.GetString();
	}

	if(0 != ip)
	{
		CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(switch_dpid);
		if(sw.isNotNull())
		{
			Base_Port* port_extend = new Base_Port(str_mac , str_name, str_tenantid, str_networkid, str_deviceid, "compute:nova",0, mac);
			port_extend->add_fixed_IP(ip,str_subnetid);
			G_PortMgr.insertNode_ByPort(port_extend);

			LOG_ERROR_FMT("host mac=%s",str_mac.c_str());
			CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(mac);
			if(host.isNotNull())
			{
				host->setDpid(switch_dpid);
				host->setPortNo(port);
				host->setSw( sw);
			}
			
			if(bnc::host::HOST_ROUTER == type)
			{
				Base_External* external_gateway = new Base_External(str_networkid, str_subnetid, ip);
				if(NULL != external_gateway)
				{
					external_gateway->set_gateway_MAC(mac);
					external_gateway->set_switch_DPID(switch_dpid);
					external_gateway->set_switch_port(port);
					G_ExternalMgr.insertNode_ByExternal(external_gateway);
				}
			}
		}
		returnResult = BNC_OK;

	}
	else
	{
		body.assign("Host ip can't be 0!!!");
	}

	body.append("");
    if(BNC_OK == returnResult)
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}

void CWebRestApi::api_extend_hosts_delete(CRestRequest* request, CRestResponse* response)
{
	UINT1 mac[6] = {0};
	string str_mac;
	BOOL returnResult = BNC_OK;
	string body = request->getBody();

	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
	   LOG_ERROR_FMT("body.c_str()=%s", body.c_str());
	   returnResult = BNC_ERR  ;
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, body);
	   return;
	}
	if(document.HasMember("MAC")&& document["MAC"].IsString())
	{
		Value& s = document["MAC"];
		str_mac = s.GetString();
		macstr2hex(str_mac.c_str(),mac);
		//LOG_ERROR_FMT("mac=%s", str_mac.c_str());
	}
	G_PortMgr.deleteNode_ByMac(mac);
	body.append("");
	if(BNC_OK == returnResult)
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}

void CWebRestApi::api_extend_links_get(CRestRequest* request, CRestResponse* response)
{
	std::string body;
	INT1 json_tmp[1024] = {0};
	UINT1 dpid[8] = {0};

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("Extend_LinkInfo");
	writer.StartArray();

	CSwitchHMap& swMap = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

	STL_FOR_LOOP(swMap, it)
	{
		CSmartPtr<CSwitch> sw = it->second;
		if (sw.isNull() || (sw->getState() >= SW_STATE_POWER_OFF))
			continue;

		 writer.StartObject();
		 writer.Key("srcDPID");
		 {
			 memset(dpid, 0, 8);
			 memset(json_tmp, 0, 1024);
			 uint8ToStr(sw->getDpid(), dpid);
			 sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
							   dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
							   dpid[7]);

		 }
		 writer.String(json_tmp);
		 writer.Key("neighbors");
		 writer.StartArray();
		 LOG_ERROR_FMT("switch dpid=%s",dpid);
		 CNeighbors* neighLinks =  CControl::getInstance()->getTopoMgr().getLinks(sw->getDpid());
		 if (NULL != neighLinks)
		 {
		    LOG_ERROR_FMT("switch-dpid=0x%llx",sw->getDpid());
			STL_FOR_LOOP(*neighLinks, iter)
			{
				//if(NULL != iter->second)
				{
					neighbor_t& neigh = iter->second;
					if (NEIGH_STATE_DELETED == neigh.state)
						continue;
					if (!CControl::getInstance()->getTopoMgr().isLinkUp(neigh.src_dpid, neigh.src_port))
						continue;
					
					writer.StartObject();
					
					writer.Key("srcPort");
					writer.Int64(neigh.src_port);
	
					writer.Key("dstDPID");
					{
						memset(dpid, 0, 8);
						memset(json_tmp, 0, 1024);
						uint8ToStr(neigh.dst_dpid, dpid);
						sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
										   dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
										   dpid[7]);
	
					}
					writer.String(json_tmp);
	
					writer.Key("dstPort");
					writer.Int64(neigh.dst_port);
	
					writer.Key("state");
					writer.String(
						(NEIGH_STATE_ESTABLISHED==neigh.state)?"ESTABLISHED":
						(NEIGH_STATE_DELETED==neigh.state)?"DELETED":"UNALIVE");
	
					writer.EndObject();
				}
				
			}
		   
		 }
		 writer.EndArray();
		 writer.EndObject();
		 
	}
    
    CControl::getInstance()->getSwitchMgr().unlock();

	writer.EndArray();

	writer.Key("retCode");
	writer.Int(0);

	writer.Key("retMsg");
	writer.String("OK");

	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_extend_links_post(CRestRequest* request, CRestResponse* response)
{
	BOOL returnResult = BNC_ERR;
	UINT2 src_port = 0, dst_port = 0;
	UINT8 switch_dpid_src = 0;
	UINT8 switch_dpid_dst = 0;
	UINT1 dpid_switch_src[8] = {0};
	UINT1 dpid_switch_dst[8] = {0};
	string str_port_src, str_dpid_src, str_port_dst, str_dpid_dst;
	string body = request->getBody();

	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
	   LOG_ERROR_FMT("body.c_str()=%s", body.c_str());
	   returnResult = BNC_ERR  ;
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, body);
	   return;
	}
	if(document.HasMember("srcDPID")&& document["srcDPID"].IsString())
	{
		Value& s = document["srcDPID"];
		str_dpid_src = s.GetString();
		dpid_str_to_bin((char *)str_dpid_src.c_str(), dpid_switch_src);
		uc8_to_ulli64 ((const UINT1*)dpid_switch_src, &switch_dpid_src);
		//LOG_ERROR_FMT("dpid=%s", str_dpid_src);
	}
	if(document.HasMember("srcPort")&& document["srcPort"].IsString())
	{
		Value& s = document["srcPort"];
		str_port_src = s.GetString();
		src_port = atoi(str_port_src.c_str());
	}
	if(document.HasMember("dstDPID")&& document["dstDPID"].IsString())
	{
		Value& s = document["dstDPID"];
		str_dpid_dst = s.GetString();
		dpid_str_to_bin((char *)str_dpid_dst.c_str(), dpid_switch_dst);
		uc8_to_ulli64 ((const UINT1*)dpid_switch_dst, &switch_dpid_dst);
		//LOG_ERROR_FMT("dpid=%s", str_dpid_dst);
	}
	if(document.HasMember("dstPort")&& document["dstPort"].IsString())
	{
		Value& s = document["dstPort"];
		str_port_dst  = s.GetString();
		dst_port = atoi(str_port_dst.c_str());
	}
	
	LOG_ERROR_FMT("str_dpid_src=%s,str_port_src=%s str_dpid_dst=%s, str_port_dst=%s ",str_dpid_src.c_str(),str_port_src.c_str(), str_dpid_dst.c_str(), str_port_dst.c_str());
	if((0 == switch_dpid_src)||(0 == switch_dpid_dst)||(0 == src_port)||(0 == dst_port))
	{
		body.assign("Switch dpid or link port can't be 0!!!");
	}
	else
	{
		CSmartPtr<CSwitch> sw(NULL);
		gn_port_t newPort = {0};
		newPort.state = PORT_STATE_UP;

		sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(switch_dpid_src);
		if(sw.isNotNull())
		{
			newPort.port_no = src_port;
			sw->updatePort(newPort);
		}
		
		sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(switch_dpid_dst);
		if(sw.isNotNull())
		{
			newPort.port_no = dst_port;
			sw->updatePort(newPort);
		}
		CControl::getInstance()->getTopoMgr().addLink(switch_dpid_src, src_port, switch_dpid_dst, dst_port);
		CControl::getInstance()->getTopoMgr().addLink(switch_dpid_dst, dst_port, switch_dpid_src, src_port);
		returnResult = BNC_OK;
	}

	body.append("");
	if(BNC_OK == returnResult)
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}

void CWebRestApi::api_extend_links_delete(CRestRequest* request, CRestResponse* response)
{
	UINT2 port = 0;
	UINT8 switch_dpid = 0;
	string str_dpid,str_port;
	UINT1 dpid_switch[8] = {0};
	BOOL returnResult = BNC_OK;
	string body = request->getBody();

	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
	   LOG_ERROR_FMT("body.c_str()=%s", body.c_str());
	   returnResult = BNC_ERR  ;
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, body);
	   return;
	}
	if(document.HasMember("SW_DPID")&& document["SW_DPID"].IsString())
	{
		Value& s = document["SW_DPID"];
		str_dpid = s.GetString();
		dpid_str_to_bin((char *)str_dpid.c_str(), dpid_switch);
		uc8_to_ulli64 ((const UINT1*)dpid_switch, &switch_dpid);
	}
	if(document.HasMember("SW_PORT")&& document["SW_PORT"].IsString())
	{
		Value& s = document["SW_PORT"];
		str_port  = s.GetString();
		port = atoi(str_port.c_str());
	}
	

	if((0 == switch_dpid)||(0 == port))
	{
		body.assign("Switch dpid or link port can't be 0!!!");
	}
	else
	{
		CControl::getInstance()->getTopoMgr().deleteLink(switch_dpid, port);
	}
	body.append("");
	if(BNC_OK == returnResult)
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}

void CWebRestApi::api_extend_mediaflow_delete(CRestRequest* request, CRestResponse* response)
{
	UINT1 mac[6] = {0};
	string str_mac,str_svr_ip, str_client_ip;
	UINT4 svr_ip = 0, client_ip = 0;
	BOOL returnResult = BNC_OK;
	string body = request->getBody();

	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
	   LOG_ERROR_FMT("body.c_str()=%s", body.c_str());
	   returnResult = BNC_ERR  ;
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, body);
	   return;
	}
	if(document.HasMember("SVR_MAC")&& document["SVR_MAC"].IsString())
	{
		Value& s = document["SVR_MAC"];
		str_mac = s.GetString();
		macstr2hex(str_mac.c_str(),mac);
		//LOG_ERROR_FMT("mac=%s", str_mac.c_str());
	}

	if(document.HasMember("SVR_IP")&& document["SVR_IP"].IsString())
	{
		Value& s = document["SVR_IP"];
		str_svr_ip = s.GetString();
		svr_ip = ip2number(str_svr_ip.c_str());
		//LOG_ERROR_FMT("mac=%s", str_mac.c_str());
	}
	if(document.HasMember("CLIENT_IP")&& document["CLIENT_IP"].IsString())
	{
		Value& s = document["CLIENT_IP"];
		str_client_ip = s.GetString();
		client_ip = ip2number(str_client_ip.c_str());
		//LOG_ERROR_FMT("mac=%s", str_mac.c_str());
	}
	CFlowMgr::getInstance()->install_delete_mediaflow(svr_ip,  client_ip);
	body.append("");
	if(BNC_OK == returnResult)
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}


void CWebRestApi::api_get_flow_entries(CRestRequest* request, CRestResponse* response)
{
	std::string body;
    INT1 json_tmp[1024] = {0};

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("switchflowentry");
	writer.StartArray();

	UINT8 dpid = 0;
	std::string pathHead = "/gn/flow/json/";
    std::string dpidStr = request->getPath().substr(pathHead.length());

	LOG_WARN_FMT("path: %s", request->getPath().c_str());
    dpidStr2Uint8(dpidStr.c_str(), &dpid);
    LOG_WARN_FMT("dpid: %s(0x%llx)", dpidStr.c_str(), dpid);

	if (0 != dpid)
	{
        CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
        if (sw.isNotNull())
        {
            writer.StartObject();
            writer.Key("DPID");
            writer.String(dpidUint8ToStr(dpid, json_tmp));
            
            writer.Key("flowEntries");
            writer.StartArray();
            writeSwitchFlows(writer, dpid);
            writer.EndArray();
            
            writer.EndObject();
        }
	}

	writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

	writer.EndObject();

	body.append(strBuff.GetString());
	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CWebRestApi::api_post_flow_entry(CRestRequest* request, CRestResponse* response)
{
	std::string body = request->getBody();

    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
    {
		std::string resp("operation is forbidened on slave !!!");
        response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, resp);
        return;
    }

	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
	   LOG_WARN_FMT("failed to parse body: %s", body.c_str());
       std::string resp("failed to parse body !!!");
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, resp);
	   return;
	}

	LOG_WARN_FMT("body: %s", body.c_str());

    UINT8 dpid = 0;
    
	if (document.HasMember("DPID") && document["DPID"].IsString())
	{
		Value& val = document["DPID"];
        dpidStr2Uint8(val.GetString(), &dpid);
	}

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
	if (sw.isNull())
	{
	   LOG_WARN_FMT("failed to get switch by dpid[0x%llx]", dpid);
       std::string resp("failed to get switch !!!");
	   response->setResponse(request->getVersion(), bnc::restful::STATUS_NOT_FOUND, resp);
	   return;
	}

    gn_flow_t flow = {0};
    strcpy(flow.creater, "Restful");

    parseFlow(document, flow);

    CFlowMgr::getInstance()->add_flow_entry(sw, flow);

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();

    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, strBuff.GetString());
}

void CWebRestApi::api_delete_flow_entry(CRestRequest* request, CRestResponse* response)
{
	std::string body = request->getBody();

    if (OFPCR_ROLE_SLAVE == CClusterService::getInstance()->getControllerRole())
    {
		std::string resp("operation is forbidened on slave !!!");
        response->setResponse(request->getVersion(), bnc::restful::STATUS_FORBIDDEN, resp);
        return;
    }

	Document document;
	if (document.Parse(body.c_str()).HasParseError())
	{
        LOG_WARN_FMT("failed to parse body: %s", body.c_str());
        std::string resp("failed to parse body !!!");
        response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, resp);
        return;
	}

	LOG_WARN_FMT("body: %s", body.c_str());

    UINT8 dpid = 0;
    
	if (document.HasMember("DPID") && document["DPID"].IsString())
	{
		Value& val = document["DPID"];
        dpidStr2Uint8(val.GetString(), &dpid);
	}

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
	if (sw.isNull())
	{
        LOG_WARN_FMT("failed to get switch by dpid[0x%llx]", dpid);
        std::string resp("failed to get switch !!!");
        response->setResponse(request->getVersion(), bnc::restful::STATUS_NOT_FOUND, resp);
        return;
	}

    INT1 uuid[UUID_LEN] = {0};
	if (document.HasMember("uuid") && document["uuid"].IsString())
	{
		Value& val = document["uuid"];
        strncpy(uuid, val.GetString(), UUID_LEN-1);
        uuid[UUID_LEN-1] = '\0';
	}
	if ('\0' == uuid[0])
	{
        LOG_WARN("failed to get uuid");
        std::string resp("failed to get uuid !!!");
        response->setResponse(request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, resp);
        return;
	}

    CFlow* cflow = CFlowMgr::getInstance()->getFlowCache().findFlow(dpid, uuid);
	if (NULL == cflow)
	{
        LOG_WARN_FMT("failed to get flow by dpid[0x%llx] and uuid[%s]", dpid, uuid);
        std::string resp("failed to get flow !!!");
        response->setResponse(request->getVersion(), bnc::restful::STATUS_NOT_FOUND, resp);
        return;
	}

    gn_flow_t flow = {0};
    cflow->translate(flow);

    CFlowMgr::getInstance()->del_flow_entry(sw, flow);

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();

    writer.Key("retCode");
    writer.Int(0);

    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();

    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, strBuff.GetString());
}




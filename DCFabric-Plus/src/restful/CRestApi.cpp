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
*   File Name   : CRestApi.cpp                                                *
*   Author      : bnc xflu           		                                  *
*   Create Date : 2016-7-21           		                                  *
*   Version     : 1.0           			                                  *
*   Function    : .           				                                  *
*                                                                             *
******************************************************************************/
#include "CRestApi.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "COpenStackHost.h"

#include "CNetwork.h"
#include "CNetworkMgr.h"
#include "CSubnet.h"
#include "CSubnetMgr.h"
#include "CFloatingIp.h"
#include "CFloatingIpMgr.h"
#include "CRouter.h"
#include "CGateway.h"
#include "CRouterGateMgr.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include <sstream>
#include "bnc-type.h"
#include "comm-util.h"
#include "CControl.h"
#include "CRestManager.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "CWebRestApi.h"
#include "CFirewallPolicy.h"
#include "CPortforwardPolicy.h"
#include "log.h"
//#include "CNatHostConnMgr.h"
#include "CSNatConnMgr.h"
#include "CClusterElect.h"
#include "CClusterService.h"
#include "CServer.h"
#include "CFlowDefine.h"
#include "CFlowMgr.h"

using namespace rapidjson;

static void writeMatch(Writer<StringBuffer>& writer, gn_oxm_t& oxm)
{
    INT1 json_tmp[1024] = {0};

    if (IS_MASK_SET(oxm.mask, OFPXMT_OFB_IN_PORT))
    {
        writer.Key("inport");
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
        writer.String((1 == oxm.arp_op) ? "Request" : "Reply");
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
            if (OFPP13_MAX > action.m_port)
                writer.Uint(action.m_port);
            else if (OFPP13_IN_PORT == action.m_port)
                writer.String("IN_PORT");
            else if (OFPP13_TABLE == action.m_port)
                writer.String("TABLE");
            else if (OFPP13_NORMAL == action.m_port)
                writer.String("NORMAL");
            else if (OFPP13_FLOOD == action.m_port)
                writer.String("FLOOD");
            else if (OFPP13_ALL == action.m_port)
                writer.String("ALL");
            else if (OFPP13_CONTROLLER == action.m_port)
                writer.String("CONTROLLER");
            else if (OFPP13_LOCAL == action.m_port)
                writer.String("LOCAL");
            else
                writer.String("UNKNOWN");
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
            writer.String("");
            break;
        case OFPAT13_POP_VLAN:
            writer.Key("popVlan");
            writer.String("");
            break;
        case OFPAT13_PUSH_MPLS:
            writer.Key("pushMpls");
            writer.String("");
            break;
        case OFPAT13_POP_MPLS:
            writer.Key("popMpls");
            writer.String("");
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
    writer.Key("tableId");
    writer.Uint(flow.getTableId());
    writer.Key("createTime");
    writer.Uint64(flow.getCreateTime());
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

static void writeSwitchFlows(Writer<StringBuffer>& writer, UINT8 dpid, UINT1 tableId)
{
    CFlowCache& flowCache = CFlowMgr::getInstance()->getFlowCache();
    if (OFPTT_ALL != tableId)
    {
        CFlowList* flowList = flowCache.getFlowList(dpid, tableId);
        if (NULL != flowList)
        {
            STL_FOR_LOOP(*flowList, itFlow)
            {
                writer.StartObject();
                {
                    writeFlow(writer, *itFlow);
                }
                writer.EndObject();
            }
        }
    }
    else
    {
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
}

void CRestApi::api_list(CRestRequest* request, CRestResponse* response)
{
	std::list<std::string> list;
	CRestManager::getInstance()->getRestApiMgr()->getRestApiPathList(list);

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("registered api list");
	writer.StartArray();

	STL_FOR_LOOP(list, iter)
	{
		writer.StartObject();

		writer.Key("Path: ");
		writer.String((*iter).c_str());

		writer.EndObject();
	}

	writer.EndArray();
	writer.EndObject();

	std::string body(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_list_detail(CRestRequest* request, CRestResponse* response)
{
	std::list<restVar> list;
	CRestManager::getInstance()->getRestApiMgr()->getRestApiPathList(list);

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("registered api list");
	writer.StartArray();

	STL_FOR_LOOP(list, iter)
	{
		writer.StartObject();

		restVar var = *iter;
		std::string str_method;
		CRestManager::getInstance()->getRestDefine()->getStrFromMethod(var.second, str_method);

		writer.Key("Method: ");
		writer.String(str_method.c_str());

		writer.Key("Path: ");
		writer.String(var.first.c_str());

		writer.EndObject();
	}

	writer.EndArray();
	writer.EndObject();

	std::string body(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}


void CRestApi::api_echo(CRestRequest* request, CRestResponse* response)
{
    std::string body;

    body.append("path is : " + request->getPath() + "\n");
    body.append("path with param is : " + request->getPathWithParam() + "\n");
    STL_FOR_LOOP(request->getPathParamList(), iter)
    {
        body.append("param meter is : " + *iter +"\n");
    }
    body.append("body is : " + request->getBody());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_default(CRestRequest* request, CRestResponse* response)
{
	std::string body("request: "  + request->getHeaderFirstLine() + "\n" +
			"not found, you can use method:GET and uri:/list to list all api");
	response->setResponse(request->getVersion(), bnc::restful::STATUS_NOT_FOUND, body);
}

void CRestApi::api_json_example(CRestRequest* request, CRestResponse* response)
{
	std::string body;

    const char json[] = " { \"hello\" : \"world\", \"t\" : true , \"f\" : false, \"n\": null, "
    		"\"i\":123, \"pi\": 3.1416, \"a\":[1, 2, 3, 4] } ";

	body.append("original json is ");
	body.append(json);
	body.append("\n");

	Document document;
	if (document.Parse(json).HasParseError())
	{
		LOG_INFO("json parse error");
		return ;
	}

	// 测试字符串查找和读取
	if (document.HasMember("hello") && document["hello"].IsString())
	{
		body.append("hello member is : ");
		body.append(document["hello"].GetString());
		body.append("\n");
	}

	// 测试文本查找和读�?
	if (document.HasMember("i") && document["i"].IsNumber())
	{
		std::stringstream stream;
		stream<<document["i"].GetInt();
		std::string str = stream.str();

		body.append("i member is : ");
		body.append(str);
		body.append("\n");
	}

	// 测试遍历
    static const char* kTypeNames[] = { "Null", "False", "True",
    		"Object", "Array", "String", "Number" };
    Value::ConstMemberIterator itr = document.MemberBegin();
    for (; itr != document.MemberEnd(); ++itr)
    {
    	char str_temp[300] = {0};
    	sprintf(str_temp, "Type of member %s is %s\n",
    			itr->name.GetString(), kTypeNames[itr->value.GetType()]);
    	body.append(str_temp);
    }

    // 测试修改
    document["i"]  = 100;
    if (document.HasMember("i") && document["i"].IsNumber())
	{

		std::stringstream stream;
		stream<<document["i"].GetInt();
		std::string str = stream.str();

		body.append("new i member is : ");
		body.append(str);
		body.append("\n");
	}

    // 测试在数组中增加新元�?
    Value& a = document["a"];   // This time we uses non-const reference.
    Document::AllocatorType& allocator = document.GetAllocator();
	for (int i = 5; i <= 10; i++)
	{
		a.PushBack(i, allocator);
	}

	// 另一种在数组中增加元素的方式
	a.PushBack("abc", allocator).PushBack("def", allocator);

	// 修改字符�?
	document["hello"] = "new world";

	// 增加新的member
	Value newmember;
	{
		char buffer2[20];
		int len = sprintf(buffer2, "%s %s", "new member", "new member");
		newmember.SetString(buffer2, static_cast<SizeType>(len), document.GetAllocator());

		memset(buffer2, 0, sizeof(buffer2));
	}
	document.AddMember("new member", newmember, document.GetAllocator());

	// 将dom转换成string
	StringBuffer sb;
	PrettyWriter<StringBuffer> writer(sb);
	document.Accept(writer);
	body.append(sb.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_switches(CRestRequest* request, CRestResponse* response)
{
	UINT1 dpid[8] = {0};
	INT1  str_temp[1024]={0};
	std::string body;

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("switch list");
	writer.StartArray();

	CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

	STL_FOR_LOOP(map, it)
	{
		CSmartPtr<CSwitch> sw = it->second;
        if (sw.isNull())
            continue;

		writer.StartObject();

        writer.Key("state");
        INT4 swState = sw->getState();
        writer.String(
            (SW_STATE_INIT==swState)?"INIT":
            (SW_STATE_NEW_ACCEPT==swState)?"NEW_ACCEPT":
            (SW_STATE_CONNECTED==swState)?"CONNECTED":
            (SW_STATE_STABLE==swState)?"STABLE":
            (SW_STATE_UNREACHABLE==swState)?"UNREACHABLE":
            (SW_STATE_DISCONNECTED==swState)?"DISCONNECTED":
            (SW_STATE_POWER_OFF==swState)?"POWER_OFF":
            (SW_STATE_CLOSING==swState)?"CLOSING":
            (SW_STATE_CLOSED==swState)?"CLOSED":
            "UNKNOWN");

		writer.Key("ip");
		INT1 ip[24] = {0};
		number2ip(htonl(sw->getSwIp()), ip);
		writer.String(ip);

		writer.Key("dpid");
		ulli64_to_uc8(sw->getDpid(), dpid);
		sprintf(str_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", 
            dpid[0], dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);
		writer.String(str_temp);

		writer.Key("tag");
		writer.Int64(sw->getTag());

		writer.EndObject();
	}

	CControl::getInstance()->getSwitchMgr().unlock();

	writer.EndArray();
	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_hosts(CRestRequest* request, CRestResponse* response)
{
    static const INT1* hostTypeStr[] = 
    {
        "UNKNOWN",
        "NORMAL",
        "ROUTER",
        "GATEWAY",
        "DHCP",
        "LOADBALANCE",
        "FLOATINGIP",
        "CLBHA",
        "CLBVIP",
        "EXTERNAL",
        "OPENSTACK HOST"
    };

	UINT1 dpid[8] = {0};
	INT1  str_temp[1024]={0};
	std::string body;
    UINT4 total[bnc::host::HOST_MAX] = {0};

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
	writer.Key("host list");
	writer.StartArray();

	CHostList& hostList = CHostMgr::getInstance()->getHostList();
	STL_FOR_LOOP(hostList, iter)
	{
        CSmartPtr<CHost> host = *iter;
        if (host.isNull())
            continue;

		INT1 strIP[32] = {0};
		number2ip(host->getIp(), strIP);

		writer.StartObject();

		writer.Key("ip");
		writer.String(strIP);

		writer.Key("dpid");
		ulli64_to_uc8(host->getDpid(), dpid);
		sprintf(str_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
		dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
		dpid[7]);
		writer.String(str_temp);
		
		writer.Key("inport");
		writer.Int(host->getPortNo());

	    if (host->getSw().isNotNull())
		{
		    memset(strIP, 0, 32);
		    number2ip(ntohl(host->getSw()->getSwIp()), strIP);
		    writer.Key("swip");
		    writer.String(strIP);
		}
		else
		{
			writer.Key("swip");
		    writer.String("");
		}

		memset(strIP, 0, 32);
		mac2str(host->getMac(), strIP);
		writer.Key("mac");
		writer.String(strIP);

		writer.Key("subnetid");
		writer.String(host->getSubnetId().c_str());

		writer.Key("tenantid");
		writer.String(host->getTenantId().c_str());
		
		writer.Key("is host");
		writer.Bool(host->isHost());

		writer.Key("hosttype");
        if (host->getHostType() < bnc::host::HOST_MAX)
        {
    		writer.String(hostTypeStr[host->getHostType()]);
            total[host->getHostType()]++;
        }
        else
        {
            writer.String("UNSUPPORTED");
        }
		
		/*
		if (bnc::host::OPENSTACK_HOST == (*iter)->getHostType())
		{
		    COpenStackHost* opHost = (COpenStackHost*)(*iter);

		    writer.Key("tenant id");
		    writer.String(opHost->getTenantId().c_str());

		    writer.Key("subnet id");
		    writer.String(opHost->getSubnetId().c_str());

		    writer.Key("network id");
		    writer.String(opHost->getNetworkId().c_str());

		    writer.Key("device type");
		    writer.Int(opHost->getDeviceType());
		}
		*/

		writer.EndObject();
	}

	writer.EndArray();

	writer.Key("total");
    writer.StartObject();
    
    for (INT4 i = bnc::host::HOST_UNKNOWN; i < bnc::host::HOST_MAX; ++i)
    {
        if (total[i] > 0)
        {
            writer.Key(hostTypeStr[i]);
            writer.Uint(total[i]);
        }
    }

	writer.EndObject();
	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_networks(CRestRequest* request, CRestResponse* response)
{
	std::string body;

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("network list");
	writer.StartArray();

	CNetworkMap networkmaplist = CNetworkMgr::getInstance()->getNetworkListMapHead();
	STL_FOR_LOOP(networkmaplist, iter)
	{
		CNetwork* networkNode = iter->second;
		writer.StartObject();
		writer.Key("networkid");
		writer.String(networkNode->get_networkid().c_str());

		writer.Key("tenantid");
		writer.String(networkNode->get_tenantid().c_str());
		
		writer.Key("is external");
		writer.Bool(networkNode->get_external());

		writer.Key("is shared");
		writer.Bool(networkNode->get_shared());

		writer.Key("check_status");
		writer.Int(networkNode->get_checkstatus());

		writer.EndObject();
	}

	writer.EndArray();
	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_subnets(CRestRequest* request, CRestResponse* response)
{
	std::string body;

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("subnet list");
	writer.StartArray();

	CSubnetMap& subnetmaplist = CSubnetMgr::getInstance()->getSubnetListMapHead();
	STL_FOR_LOOP(subnetmaplist, iter)
	{
		char str_temp[20] = {0};
		CSubnet* subnetNode = iter->second;
		writer.StartObject();
		writer.Key("subnetid");
		writer.String(subnetNode->get_subnetid().c_str());

		writer.Key("networkid");
		writer.String(subnetNode->get_networkid().c_str());

		writer.Key("tenantid");
	    writer.String(subnetNode->get_tenantid().c_str());
		
		writer.Key("gatewayIp");
		number2ip(subnetNode->get_subnetGateway(), str_temp);
		writer.String(str_temp);

		writer.Key("startIp");
		number2ip(subnetNode->get_subnetStartIp(), str_temp);
		writer.String(str_temp);

		writer.Key("endIp");
		number2ip(subnetNode->get_subnetEndIp(), str_temp);
		writer.String(str_temp);

		writer.Key("check_status");
		writer.Int(subnetNode->get_checkstatus());

		writer.EndObject();
	}

	writer.EndArray();
	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_floatingips(CRestRequest* request, CRestResponse* response)
{
	std::string body;

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("floatingip list");
	writer.StartArray();

	CFloatingIpMap floatingipmaplist = CFloatingIpMgr::getInstance()->getFloatingIpListMapHeader();
	STL_FOR_LOOP(floatingipmaplist, iter)
	{
		char str_temp[20] ;
		CFloatingIp* floatingipNode = iter->second;
		writer.StartObject();

		memset(str_temp, 0x00, sizeof(str_temp));
		writer.Key("fixedip");
		number2ip(floatingipNode->getFixedIp(), str_temp);
		writer.String(str_temp);
		
		memset(str_temp, 0x00, sizeof(str_temp));
		writer.Key("floatingip");
		number2ip(floatingipNode->getFloatingIp(), str_temp);
		writer.String(str_temp);

		writer.Key("flowinstall");
		writer.Int(floatingipNode->get_flowinstalled());

		writer.Key("check_status");
		writer.Int(floatingipNode->get_checkstatus());

		writer.EndObject();
	}

	writer.EndArray();
	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_routers(CRestRequest* request, CRestResponse* response)
{
	std::string body;

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("router list");
	writer.StartArray();

	CRouterMap routermaplist = CRouterGateMgr::getInstance()->getRouterMapHeader();
	CGatewayMap gatewaymaplist = CRouterGateMgr::getInstance()->getGatewayMapHeader();
	STL_FOR_LOOP(routermaplist, iter_router)
	{
		char str_temp[20] ;
		CRouter* routerNode = iter_router->second;
		writer.StartObject();

		writer.Key("deviceid");
		writer.String(routerNode->getDeviceid().c_str());

		writer.Key("networkid");
		writer.String(routerNode->getNetworkid().c_str());

		writer.Key("subnetid");
		writer.String(routerNode->getSubnetid().c_str());
		
		memset(str_temp, 0x00, sizeof(str_temp));
		writer.Key("routerip");
		number2ip(routerNode->getRouterIp(), str_temp);
		writer.String(str_temp);

		writer.Key("check_status");
		writer.Int(routerNode->getStatus());

		writer.Key("gateway list");
		writer.StartArray();
	
		STL_FOR_LOOP(gatewaymaplist, iter_gateway)
		{
			CGateway* gatewayNode = iter_gateway->second;
			if (routerNode->getDeviceid() == gatewayNode->getDeviceid())
			{
				writer.StartObject();
				writer.Key("deviceid");
				writer.String(gatewayNode->getDeviceid().c_str());
				
				writer.Key("networkid");
				writer.String(gatewayNode->getNetworkid().c_str());
		
				writer.Key("subnetid");
				writer.String(gatewayNode->getSubnetid().c_str());
				
				memset(str_temp, 0x00, sizeof(str_temp));
				writer.Key("gatewayip");
				number2ip(gatewayNode->getGatewayIp(), str_temp);
				writer.String(str_temp);
		
				writer.Key("check_status");
				writer.Int(gatewayNode->getStatus());
				
				writer.EndObject();
			}
		}
		
		writer.EndArray();

		writer.EndObject();
	}

	writer.EndArray();
	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_externalports(CRestRequest* request, CRestResponse* response)
{
	UINT1 dpid[8] = {0};
	INT1 mac[6] = {0};
	std::string body;

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);
	
	writer.StartObject();
	writer.Key("external_port list");
	writer.StartArray();

	std::list<Base_External *>& externalport_list =  G_ExternalMgr.getexternalListHead();
	STL_FOR_LOOP(externalport_list, iter_externalport)
	{
		char str_temp[20] ;
		Base_External * externalportNode = *iter_externalport;
		writer.StartObject();

		writer.Key("networkid");
		writer.String(externalportNode->get_network_ID().c_str());


		writer.Key("subnetid");
		writer.String(externalportNode->get_subnet_ID().c_str());
		
		memset(str_temp, 0x00, sizeof(str_temp));
		writer.Key("gatewayip");
		number2ip(externalportNode->get_gateway_IP(), str_temp);
		writer.String(str_temp);

		memset(str_temp, 0x00, sizeof(str_temp));
		writer.Key("gatewayMAC");
		mac2str(externalportNode->get_gateway_MAC(),mac);
		sprintf(str_temp, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		writer.String(str_temp);

		writer.Key("switch_DPID");
		ulli64_to_uc8(externalportNode->get_switch_DPID(), dpid);
		sprintf(str_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
		dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
		dpid[7]);
		writer.String(str_temp);
		
		
		writer.Key("switch_port");
		writer.Int64(externalportNode->get_switch_port());

		writer.EndObject();
	}

	writer.EndArray();
	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_manageswitches(CRestRequest* request, CRestResponse* response)
{
	UINT1 dpid[8] = {0};
	INT1  str_temp[1024]={0};
	std::string body;

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("manage-switch list");
	writer.StartArray();

	CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

	STL_FOR_LOOP(map, it)
	{
		CSmartPtr<CSwitch> sw = it->second;
		if (sw.isNull())
			continue;

		INT1 ip[24] = {0};
		number2ip(htonl(sw->getSwIp()), ip);

		writer.StartObject();

	    //LOG_ERROR_FMT("sw->getDpid()=0x%x sw->getManageMode()=%d", sw->getDpid(), sw->getManageMode());

		writer.Key("ip");
		writer.String(ip);
		writer.Key("dpid");
		ulli64_to_uc8(sw->getDpid(), dpid);
		sprintf(str_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
		dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
		dpid[7]);
		writer.String(str_temp);
		//writer.Int64(sw->getDpid());
		
		writer.Key("manage-mode");
		writer.Uint(sw->getManageMode());

		writer.EndObject();
	}

	CControl::getInstance()->getSwitchMgr().unlock();

	writer.EndArray();
	writer.EndObject();

	body.append(strBuff.GetString());

	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_topolinks(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};
    UINT1 dpid[8] = {0};

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("topo-link list");
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
    writer.EndObject();

    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_securitygroups(CRestRequest* request, CRestResponse* response)
{
    std::string body;

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("security-group list");
    writer.StartArray();

    CGroupHMap& groups = CFirewallPolicy::getInstance()->getGroups();
    STL_FOR_LOOP(groups, groupIt)
    {
        writer.StartObject();
        writer.Key("groupID");
        writer.String(groupIt->first.c_str());

        writer.Key("groups");
        writer.StartArray();

        STL_FOR_LOOP(groupIt->second, ruleIt)
        {
            CSmartPtr<CFirewallRule>& rule = ruleIt->second;
            if (rule.isNull())
                continue;

            writer.StartObject();

            writer.Key("enabled");
            writer.Bool(rule->getEnabled());

            writer.Key("id");
            writer.String(rule->getRuleId().c_str());

            writer.Key("direction");
            writer.String(rule->getDirection().c_str());

            writer.Key("ethertype");
            writer.String("IPv4");

            writer.Key("protocol");
            writer.String(
                (0==rule->getProtocol())?"none":
                (IP_ICMP==rule->getProtocol())?"icmp":
                (IP_TCP==rule->getProtocol())?"tcp":
                (IP_UDP==rule->getProtocol())?"udp":
                (IP_VRRP==rule->getProtocol())?"vrrp":"unknown");

            writer.Key("cidr");
            if ((rule->getRemoteIp() == 0) && (rule->getRemoteIpMask() == 0))
            {
                writer.String("none");
            }
            else
            {
                INT1 cidr[50] = {0};
                number2ip(htonl(rule->getRemoteIp()), cidr);
                strcat(cidr, "/");
                sprintf(cidr+strlen(cidr), "%u", rule->getRemoteIpMask());
                writer.String(cidr);
            }

            writer.Key("port_range_min");
            writer.Uint(rule->getPortMin());

            writer.Key("port_range_max");
            writer.Uint(rule->getPortMax());

            writer.Key("priority");
            writer.Uint(rule->getPriority());

            writer.EndObject();
        }

        writer.EndArray();
        writer.EndObject();
    }

    writer.EndArray();
    writer.EndObject();

    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_portforwards(CRestRequest* request, CRestResponse* response)
{
    std::string body;

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("portforward list");
    writer.StartArray();

    CPFRuleListHMap& ruleListHMap = CPortforwardPolicy::getInstance()->getOutPolicy();
    STL_FOR_LOOP(ruleListHMap, listIt)
    {
        CPFRuleList& ruleList = listIt->second;
        if (ruleList.empty())
            continue;

        CSmartPtr<CPortforwardRule>& front = ruleList.front();
        if (front.isNull())
            continue;

        writer.StartObject();
        writer.Key("network_id");
        writer.String(front->getNetworkId().c_str());

        writer.Key("subnet_id");
        writer.String(front->getSubnetId().c_str());

        writer.Key("outside_ip");
        INT1 ipStr[20] = {0};
        number2ip(htonl(front->getOutsideIp()), ipStr);
        writer.String(ipStr);

        writer.Key("portforwardings");
        writer.StartArray();

        STL_FOR_LOOP(ruleList, ruleIt)
        {
            CSmartPtr<CPortforwardRule>& rule = *ruleIt;
            if (rule.isNull())
                continue;

            writer.StartObject();

            writer.Key("status");
            writer.String(rule->getEnabled()?"ENABLE":"DISABLE");

            writer.Key("inside_addr");
            number2ip(htonl(rule->getInsideIp()), ipStr);
            writer.String(ipStr);

            writer.Key("protocol");
            writer.String(
                (rule->getProtocol()==0)?"none":
                (rule->getProtocol()==IP_ICMP)?"icmp":
                (rule->getProtocol()==IP_TCP)?"tcp":
                (rule->getProtocol()==IP_UDP)?"udp":
                (rule->getProtocol()==IP_VRRP)?"vrrp":"unknown");

            writer.Key("outside_port");
            UINT2 portStart = 0, portEnd = 0;
            rule->getOutsidePort(portStart, portEnd);
            INT1 portStr[20] = {0};
            if (portStart != portEnd)
                sprintf(portStr, "%u-%u", portStart, portEnd);
            else
                sprintf(portStr, "%u", portStart);
            writer.String(portStr);

            writer.Key("inside_port");
            rule->getInsidePort(portStart, portEnd);
            if (portStart != portEnd)
                sprintf(portStr, "%u-%u", portStart, portEnd);
            else
                sprintf(portStr, "%u", portStart);
            writer.String(portStr);

            writer.EndObject();
        }

        writer.EndArray();
        writer.EndObject();
    }

    writer.EndArray();
    writer.EndObject();

    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_qosrules(CRestRequest* request, CRestResponse* response)
{
	
}

void CRestApi::api_qosbindports(CRestRequest* request, CRestResponse* response)
{
	
}

void CRestApi::api_nathosts(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 str[48] = {0};

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("external hosts");
    writer.StartArray();

	CSNatConnsExtMap& extMap = CSNatConnMgr::getInstance()->getExtMap();
	STL_FOR_LOOP(extMap, extIt)
    {
        writer.StartObject();
        writer.Key("externalIp");
        writer.String(number2ip(extIt->first, str));
        
        writer.Key("tcp connections");
        writer.StartArray();
        
        CSNatConnsExt& ext = extIt->second;
        STL_FOR_LOOP(ext.m_tcpHosts, hostIt)
        {
            writer.StartObject();
            writer.Key("hostIp");
            writer.String(number2ip(hostIt->first, str));
            
            writer.Key("natConns");
            writer.StartArray();
            
            CSNatConnPortMap& portMap = hostIt->second;
            STL_FOR_LOOP(portMap, portIt)
            {
                CSmartPtr<CSNatConn> snatConn = portIt->second;
                if (snatConn.isNotNull())
                {
                    writer.StartObject();
                    
                    writer.Key("externalIp");
                    writer.String(number2ip(snatConn->getExternalIp(), str));
                    
                    writer.Key("snatIp");
                    writer.String(number2ip(snatConn->getSNatIp(), str));

                    writer.Key("snatPort");
                    writer.Uint(snatConn->getSNatPort());
                    
                    writer.Key("internalIp");
                    writer.String(number2ip(snatConn->getInternalIp(), str));
                    
                    writer.Key("internalPort");
                    writer.Uint(snatConn->getInternalPort());
                    
                    writer.Key("internalMac");
                    writer.String(mac2str(snatConn->getInternalMac(), str));
                    
                    writer.Key("gatewayDpid");
                    writer.String(dpidUint8ToStr(snatConn->getGatewayDpid(), str));
                    
                    writer.Key("switchDpid");
                    writer.String(dpidUint8ToStr(snatConn->getSwitchDpid(), str));
                    
                    writer.EndObject();
                }
            }    

            writer.EndArray();
            writer.EndObject();
        }    

        writer.EndArray();
        
        writer.Key("udp connections");
        writer.StartArray();
        
        STL_FOR_LOOP(ext.m_udpHosts, hostIt)
        {
            writer.StartObject();
            writer.Key("hostIp");
            writer.String(number2ip(hostIt->first, str));
            
            writer.Key("natConns");
            writer.StartArray();
            
            CSNatConnPortMap& portMap = hostIt->second;
            STL_FOR_LOOP(portMap, portIt)
            {
                CSmartPtr<CSNatConn> snatConn = portIt->second;
                if (snatConn.isNotNull())
                {
                    writer.StartObject();
                    
                    writer.Key("externalIp");
                    writer.String(number2ip(snatConn->getExternalIp(), str));
                    
                    writer.Key("snatIp");
                    writer.String(number2ip(snatConn->getSNatIp(), str));

                    writer.Key("snatPort");
                    writer.Uint(snatConn->getSNatPort());
                    
                    writer.Key("internalIp");
                    writer.String(number2ip(snatConn->getInternalIp(), str));
                    
                    writer.Key("internalPort");
                    writer.Uint(snatConn->getInternalPort());
                    
                    writer.Key("internalMac");
                    writer.String(mac2str(snatConn->getInternalMac(), str));
                    
                    writer.Key("gatewayDpid");
                    writer.String(dpidUint8ToStr(snatConn->getGatewayDpid(), str));
                    
                    writer.Key("switchDpid");
                    writer.String(dpidUint8ToStr(snatConn->getSwitchDpid(), str));
                    
                    writer.EndObject();
                }
            }    

            writer.EndArray();
            writer.EndObject();
        }    

        writer.EndArray();
        writer.EndObject();
    }

    writer.EndArray();
    writer.EndObject();

    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_cluster(CRestRequest* request, CRestResponse* response)
{
    std::string body;

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();

    writer.Key("cluster_on");
    writer.Bool(CClusterService::getInstance()->isClusterOn());

    if (!CClusterService::getInstance()->isClusterOn())
    {
        writer.EndObject();

        body.append(strBuff.GetString());
        response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
        return;
    }

    writer.Key("cluster");
    writer.StartArray();

    UINT4 master = 
        (CLUSTER_ELECTION_FINISHED == CClusterElection::getState())
        ? CClusterElection::getMasterIp() : 0;

    INT4 state = -1;
    if (CServer::getInstance()->getControllerIp() == master)
    {
        state = CLUSTER_CONTROLLER_CONNECTED;
    }
    else if (0 != master)
    {
        std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
        STL_FOR_LOOP(controllers, it)
        {
            cluster_controller_t& controller = *it;
            if (controller.ip == master)
            {
                state = controller.state;
                break;
            }
        }
    }

    writer.StartObject();
    writer.Key("master");
    writer.String((0 != master) ? inet_htoa(master) : "unknown");

    writer.Key("state");
    writer.String(
        (state == CLUSTER_CONTROLLER_INIT) ? "INIT" :
        (state == CLUSTER_CONTROLLER_CONNECTED) ? "CONNECTED" :
        (state == CLUSTER_CONTROLLER_NORESPONSE) ? "NORESPONSE" :
        (state == CLUSTER_CONTROLLER_UNREACHABLE) ? "UNREACHABLE" :
        (state == CLUSTER_CONTROLLER_DISCONNECTED) ? "DISCONNECTED" :
        "unknown");
    writer.EndObject();

    std::vector<cluster_controller_t>& controllers = CClusterService::getInstance()->getControllers();
    STL_FOR_LOOP(controllers, it)
    {
        cluster_controller_t& controller = *it;
        if (controller.ip != master)
        {
            writer.StartObject();
            writer.Key("slave");
            writer.String(inet_htoa(controller.ip));
            
            writer.Key("state");
            if (CServer::getInstance()->getControllerIp() == controller.ip)
            {
                writer.String("CONNECTED");
            }
            else
            {
                writer.String(
                    (controller.state == CLUSTER_CONTROLLER_INIT) ? "INIT" :
                    (controller.state == CLUSTER_CONTROLLER_CONNECTED) ? "CONNECTED" :
                    (controller.state == CLUSTER_CONTROLLER_NORESPONSE) ? "NORESPONSE" :
                    (controller.state == CLUSTER_CONTROLLER_UNREACHABLE) ? "UNREACHABLE" :
                    (controller.state == CLUSTER_CONTROLLER_DISCONNECTED) ? "DISCONNECTED" :
                    "unknown");
            }
            writer.EndObject();
        }
    }

    writer.EndArray();
    writer.EndObject();

    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}

void CRestApi::api_flowentries(CRestRequest* request, CRestResponse* response)
{
	std::string body;
    INT1 json_tmp[1024] = {0};
    const INT1* errMsg = NULL;
    bnc::restful::http_status status = bnc::restful::STATUS_BAD_REQUEST;

	StringBuffer strBuff;
	Writer<StringBuffer> writer(strBuff);

	writer.StartObject();
	writer.Key("flow entries");
	writer.StartArray();

	UINT8 dpid = 0;
    std::vector<UINT1> tableIdVec;

	LOG_WARN_FMT("path: %s", request->getPath().c_str());
	std::string pathHead = "/flowentries";

    if (request->getPath().length() > pathHead.length()+1)
    {
        std::string paramStr = request->getPath().substr(pathHead.length()+1);
        std::list<std::string> paramList;
        split(paramStr, "/", paramList);

        std::list<std::string>::iterator itParam = paramList.begin();
    	LOG_WARN_FMT("dpid: %s", (*itParam).c_str());
        if ((*itParam).compare("*") != 0)
        {
            if ((*itParam).empty() && (paramList.size() > 1))
            {
                errMsg = "[ERROR]:empty dpid in url!";
                status = bnc::restful::STATUS_BAD_REQUEST;
                goto FAILURE;
            }
            if (-1 == dpidStr2Uint8((*itParam).c_str(), &dpid))
            {
                errMsg = "[ERROR]:invalid dpid in url!";
                status = bnc::restful::STATUS_BAD_REQUEST;
                goto FAILURE;
            }
        }

        if (++itParam != paramList.end())
        {
        	LOG_WARN_FMT("table_id: %s", (*itParam).c_str());
            if (!(*itParam).empty() && ((*itParam).compare("*") != 0))
            {
                std::list<std::string> tableIdList;
                split(*itParam, ",", tableIdList);

                STL_FOR_LOOP(tableIdList, itTableId)
                {
                    if ((*itTableId).empty())
                        continue;
                    if ((*itTableId).compare("*") == 0)
                    {
                        LOG_WARN("table_id: all");
                        tableIdVec.clear();
                        break;
                    }

                    std::list<std::string> tableIdRange;
                    split(*itTableId, "-", tableIdRange);

                    UINT1 tableIdLeft = 0;
                    std::list<std::string>::iterator itRange = tableIdRange.begin();
                    if (!(*itRange).empty() && ((*itRange).compare("*") != 0))
                        tableIdLeft = atoi((*itRange).c_str());
                    if (tableIdLeft > FABRIC_TABLE_FORWARD_INVM)
                    {
                        errMsg = "[ERROR]:invalid table_id in url!";
                        status = bnc::restful::STATUS_BAD_REQUEST;
                        goto FAILURE;
                    }

                    if (++itRange == tableIdRange.end())
                    {
                        //LOG_WARN_FMT("push table_id: %u", tableIdLeft);
                        tableIdVec.push_back(tableIdLeft);
                    }
                    else
                    {
                        UINT1 tableIdRight = FABRIC_TABLE_FORWARD_INVM;
                        if (!(*itRange).empty() && ((*itRange).compare("*") != 0))
                            tableIdRight = atoi((*itRange).c_str());
                        if (tableIdRight > FABRIC_TABLE_FORWARD_INVM)
                        {
                            errMsg = "[ERROR]:invalid table_id in url!";
                            status = bnc::restful::STATUS_BAD_REQUEST;
                            goto FAILURE;
                        }

                        if (tableIdLeft <= tableIdRight)
                        {
                            if ((tableIdLeft == 0) && (tableIdRight == FABRIC_TABLE_FORWARD_INVM))
                            {
                                LOG_WARN("table_id: all");
                                tableIdVec.clear();
                                break;
                            }
                            for (INT4 i = tableIdLeft; i <= tableIdRight; i++)
                            {
                                //LOG_WARN_FMT("push table_id: %u", i);
                                tableIdVec.push_back(i);
                            }
                        }
                        else
                        {
                            if ((tableIdLeft == FABRIC_TABLE_FORWARD_INVM) && (tableIdRight == 0))
                            {
                                LOG_WARN("table_id: all in reverse order");
                                tableIdVec.clear();
                            }
                            for (INT4 i = tableIdLeft; i >= tableIdRight; i--)
                            {
                                //LOG_WARN_FMT("push table_id: %u", i);
                                tableIdVec.push_back(i);
                            }
                        }
                    }
                }
            }
        }
    }

	if (0 != dpid)
	{
        CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
        if (sw.isNull())
        {
            errMsg = "[ERROR]:can't find the switch!";
            status = bnc::restful::STATUS_NOT_FOUND;
            goto FAILURE;
        }

        switch (sw->getState())
        {
            case SW_STATE_UNREACHABLE:
                errMsg = "[ERROR]:switch is unreachable!";
                status = bnc::restful::STATUS_OTHER;
                goto FAILURE;
            case SW_STATE_DISCONNECTED:
                errMsg = "[ERROR]:switch is disconnected!";
                status = bnc::restful::STATUS_OTHER;
                goto FAILURE;
            case SW_STATE_POWER_OFF:
                errMsg = "[ERROR]:switch is power-off!";
                status = bnc::restful::STATUS_OTHER;
                goto FAILURE;
            case SW_STATE_CLOSING:
                errMsg = "[ERROR]:switch is closing!";
                status = bnc::restful::STATUS_OTHER;
                goto FAILURE;
            case SW_STATE_CLOSED:
                errMsg = "[ERROR]:switch is closed!";
                status = bnc::restful::STATUS_OTHER;
                goto FAILURE;
            default:
                break;
        }

        writer.StartObject();
        writer.Key("DPID");
        writer.String(dpidUint8ToStr(dpid, json_tmp));
        
        writer.Key("flows");
        writer.StartArray();
        if (tableIdVec.empty())
            writeSwitchFlows(writer, dpid, OFPTT_ALL);
        else
            STL_FOR_LOOP(tableIdVec, itTableId)
                writeSwitchFlows(writer, dpid, *itTableId);
    	writer.EndArray();

    	writer.EndObject();
	}
	else
	{
        CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
        CControl::getInstance()->getSwitchMgr().lock();

        STL_FOR_LOOP(map, it)
        {
            CSmartPtr<CSwitch> sw = it->second;
            if (sw.isNull())
                continue;
            
            if ((OFP13_VERSION == sw->getVersion()) && (SW_STATE_STABLE == sw->getState()))
            {
                writer.StartObject();
                writer.Key("DPID");
                writer.String(dpidUint8ToStr(sw->getDpid(), json_tmp));
                
                writer.Key("flows");
                writer.StartArray();
                if (tableIdVec.empty())
                    writeSwitchFlows(writer, sw->getDpid(), OFPTT_ALL);
                else
                    STL_FOR_LOOP(tableIdVec, itTableId)
                        writeSwitchFlows(writer, sw->getDpid(), *itTableId);
                writer.EndArray();
                
                writer.EndObject();
            }
        }

    	CControl::getInstance()->getSwitchMgr().unlock();
    }

	writer.EndArray();
	writer.EndObject();

	body.append(strBuff.GetString());
	response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);

    return;

FAILURE:
    
	writer.EndArray();
	writer.EndObject();

	body.append(errMsg);
	response->setResponse(request->getVersion(), status, body);
}

#if 0
void CRestApi::api_nathosts(CRestRequest* request, CRestResponse* response)
{
    std::string body;
    INT1 json_tmp[1024] = {0};
    INT1 str[48] = {0};

    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    writer.StartObject();
    writer.Key("nat-host list");
    writer.StartArray();

    writer.StartObject();
    writer.Key("tcp connections");
    writer.StartArray();

	CNatStreamMap& tcpMap = CNatHostConnMgr::getInstance()->getTcpNatStreamMap();
    STL_FOR_LOOP(tcpMap, tcpMapIt)
    {
        UINT4 hostIp = tcpMapIt->first;
        CSNatStreamMgr* streamMgr = tcpMapIt->second;
        if (NULL == streamMgr)
            continue;

        writer.StartObject();
        writer.Key("hostIp");
        writer.String(number2ip(hostIp, str));
        
        writer.Key("natStreams");
        writer.StartArray();
        
        CNatStreamList& streamList = streamMgr->getNatStreamHead();
        STL_FOR_LOOP(streamList, it)
        {
            CSNatStream* stream = *it;
            if (NULL == stream)
                continue;
            
            writer.StartObject();
            
            writer.Key("externalIp");
            writer.String(number2ip(stream->getExternalIp(), str));
            
            writer.Key("externalPort");
            writer.Uint(stream->getExternalPortNo());
            
            writer.Key("internalPort");
            writer.Uint(stream->getInternalPortNo());
            
            writer.Key("internalMac");
            writer.String(mac2str(stream->getInternalMac(), str));
            
            writer.Key("gatewayDpid");
            writer.String(dpidUint8ToStr(stream->getGatewayDpid(), str));
            
            writer.Key("srcDpid");
            writer.String(dpidUint8ToStr(stream->getSrcDpid(), str));

            writer.EndObject();
        }    

        writer.EndArray();
        writer.EndObject();
    }

    writer.EndArray();
    writer.EndObject();

    writer.StartObject();
    writer.Key("udp connections");
    writer.StartArray();

	CNatStreamMap& udpMap = CNatHostConnMgr::getInstance()->getUdpNatStreamMap();
    STL_FOR_LOOP(udpMap, udpMapIt)
    {
        UINT4 hostIp = udpMapIt->first;
        CSNatStreamMgr* streamMgr = udpMapIt->second;
        if (NULL == streamMgr)
            continue;

        writer.StartObject();
        writer.Key("hostIp");
        writer.String(number2ip(hostIp, str));
        
        writer.Key("natStreams");
        writer.StartArray();
        
        CNatStreamList& streamList = streamMgr->getNatStreamHead();
        STL_FOR_LOOP(streamList, it)
        {
            CSNatStream* stream = *it;
            if (NULL == stream)
                continue;
            
            writer.StartObject();
            
            writer.Key("externalIp");
            writer.String(number2ip(stream->getExternalIp(), str));
            
            writer.Key("externalPort");
            writer.Uint(stream->getExternalPortNo());
            
            writer.Key("internalPort");
            writer.Uint(stream->getInternalPortNo());
            
            writer.Key("internalMac");
            writer.String(mac2str(stream->getInternalMac(), str));
            
            writer.Key("gatewayDpid");
            writer.String(dpidUint8ToStr(stream->getGatewayDpid(), str));
            
            writer.Key("srcDpid");
            writer.String(dpidUint8ToStr(stream->getSrcDpid(), str));

            writer.EndObject();
        }    

        writer.EndArray();
        writer.EndObject();
    }

    writer.EndArray();
    writer.EndObject();

    writer.EndArray();
    writer.EndObject();

    body.append(strBuff.GetString());
    response->setResponse(request->getVersion(), bnc::restful::STATUS_OK, body);
}
#endif


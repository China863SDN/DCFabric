/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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
*   File Name   : forward-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "forward-mgr.h"
#include "l2.h"
#include "l3.h"
#include "fabric_arp.h"
#include "fabric_impl.h"
#include "fabric_openstack_arp.h"
#include "openstack_routers.h"
#include "openstack_portforward.h"
#include "../topo-mgr/topo-mgr.h"
#include "../user-mgr/user-mgr.h"
#include "../tenant-mgr/tenant-mgr.h"
#include "../overload-mgr/overload-mgr.h"

#include "gn_inet.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "mem_pool.h"
//by:yhy 其他处有申明及初始化
forward_handler_t g_default_forward_handler;
extern UINT4 g_openstack_on;
extern UINT1 g_fabric_start_flag;

#define PARAM_SET_MAX_COUNT	1000

//???该参数从头至尾未被赋值,却被取值,何用???
void *g_param_set_id = NULL;
//???该参数从头至尾未被赋值,却被取值,何用???
void *g_security_param_id = NULL;

//by:yhy 初始化参数集
void init_forward_param_list()
{
	if (NULL != g_param_set_id) {
		mem_destroy(g_param_set_id);
	}

	if (NULL != g_security_param_id) {
		mem_destroy(g_security_param_id);
	}

	g_param_set_id = mem_create(sizeof(param_set_t), PARAM_SET_MAX_COUNT);
	g_security_param_id = mem_create(sizeof(security_param_t), PARAM_SET_MAX_COUNT);
}

//by:yhy 不使用了
static INT4 arp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    return GN_OK;
}
//by:yhy ARP包处理
static INT4 fabric_arp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	LOG_PROC("HANDLER", "%s -- START",FN);
	//by:yhy <费解>这步操作用意
    if (0 == of131_fabric_impl_get_tag_sw(sw))
    {
		LOG_PROC("HANDLER", "%s -- 0 == of131_fabric_impl_get_tag_sw(sw)",FN);
        return GN_OK;
    }
    
    arp_t *arp = (arp_t *)(packet_in_info->data);
    p_fabric_host_node src_port =NULL;
    p_fabric_host_node dst_port=NULL;
	
	
	//by:yhy add,2017/01/12
	UINT1  SourceIP[15] = "";
	strcpy(SourceIP,inet_htoa(ntohl(arp->sendip)));
	UINT1  DestinationIP[15] = "";
	strcpy(DestinationIP,inet_htoa(ntohl(arp->targetip)));
	LOG_PROC("ARP_PACKET","Source IP:[%s] -> Destination IP:[%s]",SourceIP,DestinationIP);
	LOG_PROC("ARP_PACKET","Source MAC:[%02x:%02x:%02x:%02x:%02x:%02x] -> Destination MAC:[%02x:%02x:%02x:%02x:%02x:%02x]",
			 arp->sendmac[0],arp->sendmac[1],arp->sendmac[2],arp->sendmac[3],arp->sendmac[4],arp->sendmac[5],
			 arp->targetmac[0],arp->targetmac[1],arp->targetmac[2],arp->targetmac[3],arp->targetmac[4],arp->targetmac[5]
			);
	
	
	//by:yhy opcode 为1表示ARP请求,为2表示ARP应答
    if(arp->opcode == htons(1))
	{//by:yhy ARP请求包
		LOG_PROC("HANDLER", "%s -- ARP请求包 arp->opcode == htons(1)",FN);
        //by:yhy 将ARP包中的发送方MAC,IP,packet_in_info中的流入端口保存
        src_port = g_default_arp_handler.save_src_port(sw,arp->sendmac,arp->sendip,packet_in_info->inport);
        //get arp_request dst info
        dst_port = g_default_arp_handler.find_dst_port(src_port,arp->targetip);

		//by:yhy 源主机和目标主机均未知,则直接返回,不处理
        if(src_port==NULL && dst_port==NULL)
		{
			LOG_PROC("HANDLER", "%s -- src_port==NULL && dst_port==NULL",FN);
	        return GN_OK;
        }
		
        if(dst_port!=NULL)
		{//by:yhy 若找到目标主机
			LOG_PROC("HANDLER", "%s -- dst_port!=NULL",FN);
			//by:yhy 执行arp_reply
        	g_default_arp_handler.arp_reply(src_port,dst_port,arp->sendip,arp->targetip,packet_in_info);	
        }
        if(dst_port==NULL || dst_port->sw==NULL)
		{//by:yhy 若未找到目标主机,或者目标主机所连接的交换机未知
			LOG_PROC("HANDLER", "%s -- dst_port==NULL || dst_port->sw==NULL",FN);
			//by:yhy 执行arp_flood
        	g_default_arp_handler.arp_flood(src_port,arp->sendip,arp->targetip,packet_in_info);
        }
    }
	else
	{//by:yhy ARP应答包
		LOG_PROC("HANDLER", "%s -- ARP应答包 arp->opcode == htons(2)",FN);
    	//by:yhy 将ARP包中的发送方MAC,IP,packet_in_info中的流入端口保存
    	src_port = g_default_arp_handler.save_src_port(sw,arp->sendmac,arp->sendip,packet_in_info->inport);
    	//by:yhy 查找ARP应答包的目标主机
    	dst_port = g_default_arp_handler.find_dst_port(src_port,arp->targetip);
    	if(dst_port!=NULL)
		{//by:yhy 目标主机已知
			LOG_PROC("HANDLER", "%s -- dst_port!=NULL",FN);
			//by:yhy ARP应答包发送方的MAC,IP均已知,将其从flood列表中删除
    		g_default_arp_handler.arp_remove_ip_from_flood_list(arp->sendip);
			//by:yhy 输出ARP应答包
    		g_default_arp_handler.arp_reply_output(src_port,dst_port,arp->targetip,packet_in_info);
    	}
		else 
		{
			LOG_PROC("HANDLER", "%s -- dst_port==NULL",FN);
			//by:yhy ARP应答包发送方的MAC,IP均已知,将其从flood列表中删除
			g_default_arp_handler.arp_remove_ip_from_flood_list(arp->sendip);
		}
    }
	LOG_PROC("HANDLER", "%s -- STOP",FN);
    return GN_OK;
}

//by:yhy <费解> 创建一个参数集
param_set_p create_param_set()
{
	param_set_p g_param_set = (param_set_p)mem_get(g_param_set_id);
	security_param_p src_security = (security_param_p)mem_get(g_security_param_id);
	security_param_p dst_security = (security_param_p)mem_get(g_security_param_id);
	if ((NULL == g_param_set) || (NULL == src_security)|| (NULL == dst_security)) {
		return NULL;
	}
	memset(g_param_set, 0, sizeof(param_set_t));
	memset(src_security, 0, sizeof(security_param_t));
	memset(dst_security, 0, sizeof(security_param_t));
	g_param_set->src_security = src_security;
	g_param_set->dst_security = dst_security;

	return g_param_set;
}
//by:yhy 销毁一个参数集
void destory_param_set(param_set_p param)
{
	if (NULL != param) 
	{
		if (NULL != param->src_security) 
		{
			mem_free(g_security_param_id, param->src_security);
		}
		if (NULL != param->dst_security) 
		{
			mem_free(g_security_param_id, param->dst_security);
		}
		mem_free(g_param_set_id, param);
	}
}
//by:yhy ip包处理 
static INT4 fabric_ip_packet_handle(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	LOG_PROC("HANDLER", "%s -- START",FN);
    if (0 == of131_fabric_impl_get_tag_sw(sw))
    {
		LOG_PROC("HANDLER", "%s -- 0 == of131_fabric_impl_get_tag_sw(sw)",FN);
        return GN_OK;
    }

	//by:yhy 创建一个参数集
	param_set_p param = create_param_set();
	if (NULL == param) 
	{
		LOG_PROC("INFO", "Can't create param list!");
		return 0;
	}
	//by:yhy 提取packet_in包中的IP包信息
	ip_t *p_ip = (ip_t *)(packet_in_info->data);

	//by:yhy src_port,dst_port 源主机与目标主机
	p_fabric_host_node src_port =NULL;
	p_fabric_host_node dst_port=NULL;
	INT4 foward_type = IP_DROP;

	//by:yhy 保存源MAC,IP,交换机端口
	src_port = g_default_ip_handler.save_src_port_ip(sw,p_ip->eth_head.src,p_ip->src,packet_in_info->inport);

	//by:yhy 根据源端口,目标IP查找交换机的目标端口
	dst_port = g_default_ip_handler.find_dst_port_ip(src_port, p_ip->dest);

	//by:yhy add,2017/01/12
	UINT1  SourceIP[15] = "";
	strcpy(SourceIP,inet_htoa(ntohl(p_ip->src)));
	UINT1  DestinationIP[15] = "";
	strcpy(DestinationIP,inet_htoa(ntohl(p_ip->dest)));
	LOG_PROC("IP_PACKET","Source IP:[%s] -> Destination IP:[%s]",SourceIP,DestinationIP);
	
	
	if ((src_port == NULL && dst_port == NULL) && (-1 != p_ip->dest))
	{//by:yhy 源主机,目标主机均不存在,且目标IP不为广播地址
		// return GN_ERR;
		LOG_PROC("HANDLER", "%s -- (src_port == NULL && dst_port == NULL) && (-1 != p_ip->dest)",FN);
	}
	else 
	{
		//by:yhy 检查src_port与dst_port是否存在通路
		INT4 gn_access_result = g_default_ip_handler.ip_packet_check_access(src_port, dst_port, packet_in_info, param);

		if(gn_access_result == GN_ERR)
		{//by:yhy 如果不存在通路,则装载deny_flow
			g_default_ip_handler.ip_install_deny_flow(sw, p_ip);
			LOG_PROC("HANDLER", "%s -- gn_access_result == GN_ERR",FN);
		}
		else 
		{//by:yhy 存在通路
			param->src_port = src_port;
			param->dst_port = dst_port;

			//by:yhy 根据IP包内容判断下一步操作的类型
			foward_type = g_default_ip_handler.ip_packet_compute_src_dst_forward(src_port,dst_port,packet_in_info,param);
 
			//by:yhy 增加对应主机节点的消息计数值;发往外部的icmp消息不统计
            if (!(foward_type == CONTROLLER_FORWARD && IPPROTO_ICMP == p_ip->proto))
            {
                add_msg_counter(sw, p_ip->dest, p_ip->eth_head.dest);
				LOG_PROC("HANDLER", "%s -- !(foward_type == CONTROLLER_FORWARD && IPPROTO_ICMP == p_ip->proto)",FN);
            }
			
			//by:yhy 根据下一步操作类型,执行相关动作
			if(foward_type==CONTROLLER_FORWARD)
			{
				//by:yhy openflow输出一个packet_out
				fabric_openstack_packet_output(param->dst_sw, packet_in_info, param->dst_inport);
				LOG_PROC("HANDLER", "%s -- foward_type==CONTROLLER_FORWARD)",FN);
			}
			else if(foward_type==IP_FLOOD) 
			{
				//by:yhy ip flood
				g_default_ip_handler.ip_flood(src_port,param->src_ip, param->dst_ip, param->src_mac,packet_in_info);
				LOG_PROC("HANDLER", "%s -- foward_type==IP_FLOOD",FN);
			}
			else if(foward_type==BROADCAST_DHCP) 
			{
				//can access but don't know where is dst_port ->flood
				openstack_ip_p_broadcast(packet_in_info);
				LOG_PROC("HANDLER", "%s -- foward_type==BROADCAST_DHCP",FN);
				//DHCP handle
				//TODO
			}
			else if(foward_type==IP_HANDLE_ERR) 
			{//by:yhy TBD
				// printf(" something happened! ");
				// return GN_ERR;
			}
			else if(foward_type == IP_PACKET) 
			{//by:yhy TBD
				// printf(" packet out! \n");
			}
			else if(foward_type == IP_DROP) 
			{//by:yhy TBD
				// return GN_ERR;
			}
			else if((Internal_port_flow == foward_type) || (Internal_out_subnet_flow == foward_type)) 
			{
				g_default_ip_handler.ip_packet_install_flow(param, foward_type);
				fabric_openstack_packet_output(param->dst_port->sw,packet_in_info,param->dst_port->port);
				LOG_PROC("HANDLER", "%s -- (Internal_port_flow == foward_type) || (Internal_out_subnet_flow == foward_type)",FN);
				// fabric_openstack_packet_output(sw, packet_in_info, OFPP13_TABLE);
			}
			else
			{
				if (OFPP13_CONTROLLER != packet_in_info->inport)
				{
					//install other flows
					g_default_ip_handler.ip_packet_install_flow(param, foward_type);
					fabric_openstack_packet_output(sw, packet_in_info, OFPP13_TABLE);
					LOG_PROC("HANDLER", "%s -- OFPP13_CONTROLLER != packet_in_info->inport",FN);
				}
			}
		}
	}
	//by:yhy 销毁参数
	destory_param_set(param);
	
	LOG_PROC("HANDLER", "%s -- STOP",FN);
	return GN_OK;
}

/*Need to be deleted*/
//不使用
static INT4 ip_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    return GN_OK;
}

static INT4 ipv6_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	//by:yhy add 201701091313
	LOG_PROC("HANDLER", "ipv6_packet_handler - START");
#if 0
	if (0 == g_fabric_start_flag) {
		return 0;
	}

	UINT1 local_node[16] = {0};
	ipv6_str_to_number("FF01::1", local_node);
	UINT1 local_link[16] = {0};
	ipv6_str_to_number("FF02::1", local_link);
	UINT1 solicited_node[16] = {0};
	ipv6_str_to_number("FF02::1:FF00:0", solicited_node);
	UINT1 local_link_src[16] = {0};
	ipv6_str_to_number("fe80::", local_link_src);

	param_set_p param = create_param_set();
	if (NULL == param) {
		LOG_PROC("INFO", "Can't create param list!");
		return 0;
	}

	ipv6_t* p_ip = (ipv6_t*)(packet_in_info->data+14);

	p_fabric_host_node src_port = NULL;
	p_fabric_host_node dst_port= NULL;

	UINT1 src_mac[6] = {0};
	UINT1 dst_mac[6] = {0};

	memcpy(dst_mac, packet_in_info->data, 6);
	memcpy(src_mac, packet_in_info->data+6, 6);

//	nat_show_ipv6(p_ip->src);
//	nat_show_ipv6(p_ip->dest);
//	nat_show_ip(sw->sw_ip);
//	nat_show_mac(src_mac);
//	nat_show_mac(dst_mac);

	src_port = openstack_save_host_info_ipv6(sw, src_mac, p_ip->src, packet_in_info->inport);

	dst_port = openstack_find_ip_dst_port_ipv6(src_port, p_ip->dest);

	if (NULL != dst_port) {
		// printf("find dest!\n");
		fabric_openstack_install_fabric_flows_ipv6(src_port, dst_port, NULL, NULL);
	}

	if (0 == memcmp(src_mac+3, dst_mac+3, 3)) {
		// printf("same mac\n");
		destory_param_set(param);
		return 0;
	}

	if (((0 == memcmp(local_node, p_ip->dest, 16))
			|| (0 == memcmp(local_link, p_ip->dest, 16)))
			|| (0 == memcmp(solicited_node, p_ip->dest, 13))) {
		// printf("local link flood\n");
		openstack_ip_p_broadcast(packet_in_info);
	}

	destory_param_set(param);

#endif
    //by:yhy add 201701091313
	LOG_PROC("HANDLER", "ipv6_packet_handler - STOP");
	return GN_OK;
}
///Added by Xu yanwei in 2015-08-13
static INT4 vlan_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	LOG_PROC("HANDLER", "vlan_packet_handler - START");
	if(get_fabric_state())
	{
		fabric_vlan_handle(sw, packet_in_info);
		LOG_PROC("HANDLER", "vlan_packet_handler - STOP");
		return GN_OK;
	}
	LOG_PROC("HANDLER", "vlan_packet_handler - STOP");
    return GN_OK;
}

INT4 packet_in_process(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	ether_t *ether_header = (ether_t *)packet_in_info->data;
	//by:yhy add,2017/01/12
	UINT1  SwitchIP[15] = "";
	strcpy(SwitchIP,inet_htoa(ntohl(sw->sw_ip)));

	if(ether_header->proto == htons(ETHER_LLDP))
	{
		LOG_PROC("PACKET_IN", "ETHER_LLDP Switch_IP:%s,Switch_Port:  %d;",SwitchIP,ntohs(sw->sw_port));
		return g_default_forward_handler.lldp(sw, packet_in_info);
	}
	else if(ether_header->proto == htons(ETHER_ARP))
	{
		LOG_PROC("PACKET_IN", "ETHER_ARP  Switch_IP:%s,Switch_Port:  %d;",SwitchIP,ntohs(sw->sw_port));
		return g_default_forward_handler.arp(sw, packet_in_info);
	}
	else if(ether_header->proto == htons(ETHER_IP))
	{
		LOG_PROC("PACKET_IN", "ETHER_IP   Switch_IP:%s,Switch_Port:  %d;",SwitchIP,ntohs(sw->sw_port));
		return g_default_forward_handler.ip(sw, packet_in_info);
	}
	else if(ether_header->proto == htons(ETHER_IPV6))
	{
		LOG_PROC("PACKET_IN", "ETHER_IPV6 Switch_IP:%s,Switch_Port:  %d;",SwitchIP,ntohs(sw->sw_port));
		return g_default_forward_handler.ipv6(sw, packet_in_info);
	}
	else if(ether_header->proto == htons(ETHER_VLAN))
	{
		LOG_PROC("PACKET_IN", "ETHER_VLAN Switch_IP:%s,Switch_Port:  %d;",SwitchIP,ntohs(sw->sw_port));
		return g_default_forward_handler.vlan(sw, packet_in_info);
	}
	else
	{
		//by:yhy add 201701051305
		if(9984 != ether_header->proto && 400 != ether_header->proto)
		{
			//LOG_PROC("ERROR", "%s: ether type [%d] has no handler!", FN, ether_header->proto);
		}
		return GN_ERR;
	}
}

forward_handler_t g_default_forward_handler =
{
    .lldp = lldp_packet_handler,
    .arp = arp_packet_handler,
    .ip = ip_packet_handler,
    .ipv6 = ipv6_packet_handler,
	.vlan= vlan_packet_handler
};

arp_handler_t g_default_arp_handler =
{
    .save_src_port=fabric_save_host_info,
	.find_dst_port=fabric_find_dst_port,
	.arp_flood=fabric_arp_flood,
	.arp_reply=fabric_arp_reply,
	.arp_remove_ip_from_flood_list=fabric_arp_remove_ip_from_flood_list,
	.arp_reply_output=fabric_arp_reply_output
};
ip_handler_t g_default_ip_handler =
{
    .save_src_port_ip=fabric_save_host_info,
	.find_dst_port_ip=fabric_find_dst_port,
	.ip_packet_output=fabric_ip_packet_output,
	.ip_packet_check_access=fabric_ip_packet_check_access,
	.ip_flood=fabric_ip_p_flood,
	.ip_packet_install_flow=fabric_ip_p_install_flow,
	.ip_packet_compute_src_dst_forward=fabric_compute_src_dst_forward,
	.ip_install_deny_flow = fabric_ip_install_deny_flow,
};


//by:yhy 初始化 g_default_forward_handler,g_default_arp_handler,g_default_ip_handler
void init_handler()
{
	if(get_fabric_state())
	{
		init_forward_param_list();//by:yhy <费解>干什么用的

		g_default_forward_handler.ip=fabric_ip_packet_handle;
		g_default_forward_handler.arp=fabric_arp_packet_handler;
		if(g_openstack_on==0)
		{//by:yhy openstack OFF
			//by:yhy
			g_default_arp_handler.arp_flood=fabric_arp_flood;
			g_default_arp_handler.arp_remove_ip_from_flood_list=fabric_arp_remove_ip_from_flood_list;
			//by:yhy ARP响应
			g_default_arp_handler.arp_reply=fabric_arp_reply;
			g_default_arp_handler.arp_reply_output=fabric_arp_reply_output;
			g_default_arp_handler.find_dst_port=fabric_find_dst_port;
			g_default_arp_handler.save_src_port=fabric_save_host_info;
			//by:yhy 保存源MAC,IP,交换机端口
			g_default_ip_handler.save_src_port_ip=fabric_save_host_info;
			g_default_ip_handler.find_dst_port_ip=fabric_find_dst_port_ip;
			g_default_ip_handler.ip_packet_output=fabric_ip_packet_output;
			g_default_ip_handler.ip_flood=fabric_ip_p_flood;
			g_default_ip_handler.ip_packet_install_flow=fabric_ip_p_install_flow;
			//by:yhy 检查源和目标之间是否存在路径
			g_default_ip_handler.ip_packet_check_access=fabric_ip_packet_check_access;
			//by:yhy 根据给定参数,判断下一步操作的类型
			g_default_ip_handler.ip_packet_compute_src_dst_forward = fabric_compute_src_dst_forward;
			//by:yhy 如果不存在通路,则装载deny_flow
			g_default_ip_handler.ip_install_deny_flow = fabric_ip_install_deny_flow;
		}
		else
		{//by:yhy openstack ON  (注意以下函数调用处注意输出其返回值内容)
			g_default_arp_handler.arp_flood=openstack_arp_flood;
			g_default_arp_handler.arp_remove_ip_from_flood_list=openstack_arp_remove_ip_from_flood_list;
			g_default_arp_handler.arp_reply=openstack_arp_reply;
			g_default_arp_handler.arp_reply_output=openstack_arp_reply_output;
			g_default_arp_handler.find_dst_port=openstack_find_dst_port;
			g_default_arp_handler.save_src_port=openstack_save_host_info;

			//by:yhy 保存源MAC,IP,交换机端口
			g_default_ip_handler.save_src_port_ip=openstack_save_host_info;
			g_default_ip_handler.find_dst_port_ip=openstack_find_ip_dst_port;
			g_default_ip_handler.ip_packet_output=openstack_ip_packet_output;
			g_default_ip_handler.ip_flood=openstack_ip_p_flood;
			g_default_ip_handler.ip_packet_install_flow=openstack_ip_p_install_flow;
			g_default_ip_handler.ip_packet_check_access = openstack_ip_packet_check_access;
			g_default_ip_handler.ip_packet_compute_src_dst_forward = openstack_ip_packet_compute_src_dst_forward;
			g_default_ip_handler.ip_install_deny_flow = openstack_ip_install_deny_flow;
			// g_default_ip_handler.ip_broadcast = openstack_ip_p_broadcast;
		}
	}
}



void fini_forward_mgr()
{
	/*Need to be deleted
    fini_l2();
    fini_l3();
	*/
}




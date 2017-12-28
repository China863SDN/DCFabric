/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
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

/*
 * fabric_openstack_arp.c
 *
 *  Created on: Jun 19, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */

#include "fabric_openstack_arp.h"
#include "fabric_flows.h"
#include "fabric_impl.h"
#include "openstack_app.h"
#include "gn_inet.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "common.h"
#include "fabric_floating_ip.h"
#include "fabric_firewall.h"
#include "fabric_openstack_nat.h"
#include "fabric_openstack_external.h"
#include "fabric_openstack_gateway.h"
#include "../conn-svr/conn-svr.h"
#include "fabric_arp.h"
#include "openstack_security_app.h"
#include "openstack_lbaas_app.h"
#include "openstack_routers.h"
#include "openstack_portforward.h"
#include "openstack_lbaas_app.h"
#include "openstack_clbaas_app.h"

UINT4 g_openstack_dns_ip = 0x8080808;
const UINT1 arp_zero_mac[] = {0x0,0x0,0x0,0x0,0x0,0x0};
const UINT1 arp_broadcat_mac[] = {0xff,0xff,0xff,0xff,0xff,0xff};

extern UINT4 g_openstack_on;
extern UINT4 g_proactive_flow_flag;

extern UINT1 g_nat_physical_switch_flag;

UINT2  g_removeclbflow = 0;
/*****************************
 * local function
 *****************************/
UINT1 g_broad_mac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
UINT4 g_broad_ip = -1;  //by:yhy 255.255.255.255 广播IP

void fabric_openstack_ip_broadcast_handle(gn_switch_t *sw, packet_in_info_t *packet_in,ip_t *ip,p_fabric_host_node src_port);

void fabric_openstack_dhcp_request_handle(packet_in_info_t *packet_in, p_fabric_host_node src_port);
void fabric_openstack_dhcp_reply_handle(packet_in_info_t *packet_in, p_fabric_host_node dst_port);
void fabric_openstack_install_fabric_flows(p_fabric_host_node src_port,p_fabric_host_node dst_port,
										   security_param_p src_security, security_param_p dst_security);
void fabric_openstack_install_fabric_out_subnet_flows(p_fabric_host_node src_port,p_fabric_host_node src_gateway,
		p_fabric_host_node dst_port,p_fabric_host_node dst_gateway, security_param_p src_security, security_param_p dst_security);
int check_fabric_openstack_subnet_dhcp_gateway(p_fabric_host_node port,openstack_subnet_p subnet);
INT4 create_arp_flood_parameter(UINT4 dst_ip, p_fabric_host_node dst_port, param_set_p param);
/*****************************
 * local function : packet out
 *****************************/

void fabric_openstack_packet_flood(packet_in_info_t *packet_in_info);
void fabric_openstack_packet_flood_in_subnet(packet_in_info_t *packet_in_info,char* subnet_id,UINT4 subnet_port_num);
void fabric_openstack_create_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in_info);
void fabric_openstack_create_arp_reply_public(UINT1* srcMac, UINT4 srcIP, UINT1* dstMac, UINT4 dstIP, gn_switch_t* sw,
											UINT4 outPort, packet_in_info_t *packet_in_info);
void fabric_openstack_external_arp_mac();
//gn_switch_t* get_ext_sw_by_dpid(UINT8 dpid);

INT4 external_packet_out_compute_forward(p_fabric_host_node src_port,UINT4 sendip, UINT4 targetip, packet_in_info_t *packet_in, UINT1 proto, param_set_p param_set);
INT4 external_packet_in_compute_forward(p_fabric_host_node src_port, UINT4 src_ip, UINT4 targetip, packet_in_info_t* packet_in, UINT1 proto, param_set_p param_set);
INT4 internal_packet_compute_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip, param_set_p param_set, ip_t *ip);
void remove_host_output_flow_by_ip_mac(gn_switch_t* sw, UINT4 ip, UINT1* mac);
void remove_floating_flow(gn_switch_t* sw, UINT4 floating_ip, UINT1* mac);
void remove_nat_flow(gn_switch_t* sw, UINT4 ip, UINT1* src_mac);
INT4 internal_packet_compute_vip_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip, param_set_p param_set, ip_t *ip);
INT4 openstack_check_src_dst_is_controller(packet_in_info_t *packet_in);

void fabric_openstack_show_port(p_fabric_host_node port){
	struct in_addr addr;
	char temp[16] = {0};
	memcpy(&addr, &port->ip_list[0], 4);
	mac2str(port->mac, temp);
	openstack_port_p port_p = (openstack_port_p)port->data;
	LOG_PROC("INFO","Tenant: %s | Network: %s | Subnet: %s  | Port: %s | IP: %s  | MAC: %s  |\n",port_p->tenant_id,port_p->network_id,port_p->subnet_id,port_p->port_id,inet_ntoa(addr),temp);

	return;
}

void fabric_openstack_show_ip(UINT4 ip){
	struct in_addr addr;
	memcpy(&addr, &ip, 4);
	LOG_PROC("INFO","IP: %s  |",inet_ntoa(addr));
	return;
}
void fabric_openstack_show_mac(UINT1* mac){
	char temp[16] = {0};
	mac2str(mac, temp);
	LOG_PROC("INFO","MAC: %s  |",temp);
	return;
}
extern UINT4 g_openstack_fobidden_ip;

//by:yhy openstack保存主机相关信息(IP,sw,port)
p_fabric_host_node openstack_save_host_info(gn_switch_t *sw,UINT1* sendmac,UINT4 sendip,UINT4 targetip, UINT4 inport)
{
	int k = 0;
	UINT4 inside_vipIp = 0;
	p_fabric_host_node dst_node = NULL;
	p_fabric_host_node inside_viphost = NULL;
	p_fabric_host_node ext_viphost = NULL;
	external_port_p epp = NULL;
	gn_switch_t * external_sw = NULL;
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	
	LOG_PROC("HANDLER", "%s -- sendip=0x%x inport=%d ",FN,sendip, inport );
	p_fabric_host_node p_node =  get_fabric_host_from_list_by_mac(sendmac);
	if(p_node!=NULL)
	{//by:yhy 不处理

	}
	else
	{//by:yhy 如果不存在对应MAC的主机,则返回NULL
		return NULL;
	}
	
	if((NULL != p_node->sw)&&(OPENSTACK_PORT_TYPE_CLBLOADBALANCER == p_node->type)&&(sendip != targetip))
	{
		if((sw != p_node->sw)||((0 != p_node->port)&&(inport != p_node->port)))
		{
			lb_vipfloating = find_openstack_clbaas_vipfloatingpool_by_extip(sendip);
			if(lb_vipfloating&&(htonl(g_reserve_ip) == targetip))
			{
				
				LOG_PROC("INFO", "%s %d ################ targetip=0x%x sendip=0x%x g_reserve_ip=0x%x p_node->sw->sw_ip =0x%x sw_ip=0x%x",FN,LN, targetip,sendip,htonl(g_reserve_ip),p_node->sw->sw_ip,sw->sw_ip);
				inside_vipIp = lb_vipfloating->inside_ip;
				if(inside_vipIp)
				{
					LOG_PROC("INFO", "%s %d head->ip_list[0]=0x%x inside_vipIp=0x%x ",FN,LN,sendip, inside_vipIp);
					inside_viphost =  get_fabric_host_from_list_by_ip(inside_vipIp);

					if(inside_viphost->sw)
						LOG_PROC("INFO", "%s %d #################HA#################### inside_viphost->sw->sw_ip=0x%x p_node->sw->sw_ip=0x%x sendip=0x%x",FN,LN,inside_viphost->sw->sw_ip,p_node->sw->sw_ip,sendip);
					remove_openstack_clbaas_backend_internalflows_byMac(inside_viphost->sw, inside_viphost->mac);
					remove_openstack_clbaas_backend_internalflows_byIp(inside_vipIp);
					
					inside_viphost->sw = NULL;
					inside_viphost->port = 0;
				}
			
				epp  = get_external_port_by_hostip(sendip);
				if(epp)
				{
					external_sw = get_ext_sw_by_dpid(epp->external_dpid);
					if(external_sw)
					{
						
						LOG_PROC("INFO", "%s %d #################HA####################",FN,LN);
						if(p_node->sw)
						{
							remove_clbforward_flow_by_vmMac(p_node->sw, p_node->mac);
						}
						remove_clbforward_flow_by_SrcIp(external_sw, sendip);
						
					}
				}
				for( k = 0; k < g_server.max_switch; k++)
				{
					external_sw = &g_server.switches[k];
					if (external_sw&&(CONNECTED == external_sw->conn_state))
					{
						remove_clbforward_flow_by_SrcIp(external_sw, sendip);
					}
				}
			}
			else
			{
				if(NULL == lb_vipfloating)
				{
					
					remove_openstack_clbaas_backend_internalflows_byIp(sendip);	
					remove_openstack_clbaas_backend_internalflows_byMac(p_node->sw, p_node->mac);
					
					p_node->sw = NULL;
					p_node->port = 0;
					return p_node;
				}
			}
		}
		if((inport< MAX_PORTS)&&(inport > 0))
		{
			p_node->port = inport;
			p_node->sw=sw;
		}
		return p_node;
	}
	if ((NULL != p_node->sw) && (0 != p_node->port))
	{//by:yhy 返回 p_node

		
		if((sendip == targetip)&&(OPENSTACK_PORT_TYPE_CLBLOADBALANCER == p_node->type)/*&&((sw != p_node->sw)||(inport != p_node->port))*/) //HA切换
		{
			LOG_PROC("INFO", "%s %d sw->sw_ip=0x%x p_node_ip[0] = 0x%x sendip=0x%x targetip=0x%x inport=%d sendmac[4]=0x%x sendmac[5]=0x%x",FN,LN,sw->sw_ip,  p_node->ip_list[0],sendip,targetip,inport,sendmac[4],sendmac[5]);
			
			
			LOG_PROC("INFO", "%s %d p_node->sw->sw_ip=0x%x sw->sw_ip=0x%x inport=%d p_node->port=%d",FN,LN,p_node->sw->sw_ip, sw->sw_ip,inport, p_node->port);

			
			inside_vipIp = find_openstack_clbaas_vipfloatingpool_insideip_by_extip(sendip);
			if(inside_vipIp)
			{
				LOG_PROC("INFO", "%s %d head->ip_list[0]=0x%x inside_vipIp=0x%x",FN,LN,sendip, inside_vipIp);
				inside_viphost =  get_fabric_host_from_list_by_ip(inside_vipIp);

				if(inside_viphost->sw)
					LOG_PROC("INFO", "%s %d #################HA#################### inside_viphost->sw->sw_ip=0x%x p_node->sw->sw_ip=0x%x sendip=0x%x",FN,LN,inside_viphost->sw->sw_ip,p_node->sw->sw_ip,sendip);
				remove_openstack_clbaas_backend_internalflows_byMac(inside_viphost->sw, inside_viphost->mac);
				remove_openstack_clbaas_backend_internalflows_byIp(inside_vipIp);
				
				inside_viphost->sw = NULL;
				inside_viphost->port = 0;
				g_removeclbflow = 1;
			}
			
			if((OPENSTACK_PORT_TYPE_CLBLOADBALANCER == p_node->type)) // icmp flood
			{
				epp  = get_external_port_by_hostip(sendip);
				if(epp&&g_removeclbflow)
				{
					external_sw = get_ext_sw_by_dpid(epp->external_dpid);
					if(external_sw)
					{
						
						LOG_PROC("INFO", "%s %d #################HA####################",FN,LN);
						ext_viphost = get_fabric_host_from_list_by_ip(sendip);
						if(ext_viphost&&ext_viphost->sw)
						{
							remove_clbforward_flow_by_vmMac(ext_viphost->sw, ext_viphost->mac);
						}
						remove_clbforward_flow_by_SrcIp(external_sw, sendip);
						
					}
					g_removeclbflow = 0;
				}
				if(external_sw)
				{
					LOG_PROC("INFO","%s %d epp->ip=0x%x sendip=0x%x",FN,LN,external_sw->sw_ip,sendip);
				}
				for( k = 0; k < g_server.max_switch; k++)
				{
					external_sw = &g_server.switches[k];
					if (external_sw&&(CONNECTED == external_sw->conn_state))
					{
						remove_clbforward_flow_by_SrcIp(external_sw, sendip);
					}
				}
			}
		

			p_node->port = 0;
			p_node->sw=NULL;
		}

		
	
		return p_node;
	}
	
	if ((sendip) &&(0 == p_node->ip_list[0]))
	{
		p_node->ip_list[0] = sendip;
	}
	if((inport< MAX_PORTS)&&(inport > 0))
	{
		p_node->port = inport;
		p_node->sw=sw;
		
		sw->ports[inport].type = HOST_CON;
	}

	return p_node;
}
//by:yhy 根据源主机或者目标IP查找对应的目标主机
p_fabric_host_node openstack_find_dst_port(p_fabric_host_node src_node,UINT4 targetip)
{
	LOG_PROC("HANDLER", "%s ",FN);
	p_fabric_host_node dst_port=NULL;
	if(src_node==NULL)
	{//by:yhy 源主机未知
		external_floating_ip_p fip = find_external_floating_ip_by_floating_ip(targetip);
		if(fip != NULL)
		{
			dst_port = find_fabric_host_port_by_port_id(fip->port_id);
			
			LOG_PROC("HANDLER", "%s dst_port=0x%x",FN,dst_port);
		}
		else
		{
			dst_port = get_fabric_host_from_list_by_ip(targetip);
		}
	}
	else
	{//by:yhy 源主机已知
		openstack_port_p src_port_p = (openstack_port_p)src_node->data;
		// find dst_port by ip (because mac maybe is the 00:00:00:00:00:00)
		if (NULL != src_port_p)
		{
			dst_port = find_fabric_host_port_by_subnet_id(targetip,src_port_p->subnet_id);
		}
	}
	return dst_port;
}
//by:yhy 根据给定参数src_node或者targetip查找(p_fabric_host_node)目标主机(主机内存有对应的交换机上的端口)
p_fabric_host_node openstack_find_ip_dst_port(p_fabric_host_node src_node,UINT4 targetip)
{
	p_fabric_host_node dst_port=NULL;

	
	// find dst port
	if (src_node) 
	{//by:yhy 存在源主机
		dst_port = find_openstack_host_by_srcport_ip(src_node, targetip);
	}
	else 
	{//by:yhy 不存在源主机
		dst_port = get_fabric_host_from_list_by_ip(targetip);
	}
	
	LOG_PROC("HANDLER", "%s src_node=0x%x targetip=0x%x dst_port=0x%x",FN,src_node,targetip,dst_port);
	return dst_port;
}
//by:yhy openstack arp flood
INT4 openstack_arp_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in)
{
	LOG_PROC("HANDLER", "%s src_port=0x%x targetip=0x%x",FN,src_port, targetip);
	if ((NULL != src_port) && (NULL == find_fabric_host_port_by_subnet_id(targetip,"0"))) 
	{//by:yhy 源主机存在,且目标IP对应的主机存在且其subnet_ID为"0"
		LOG_PROC("HANDLER", "%s -- %d",FN, LN);
		//by:yhy 构建ARP包
		fabric_add_into_arp_request(src_port,sendip,targetip);
		//by:yhy 判断是否对targetip进行洪泛,若是则增加信号量
		fabric_push_arp_flood_queue(targetip,packet_in);
	}
	return GN_OK;
}
//by:yhy 根据sendip将p_fabric_arp_request_node从g_arp_request_list.list中移除
INT4 openstack_arp_remove_ip_from_flood_list(UINT4 sendip)
{

	p_fabric_arp_request_node temp_node = remove_fabric_arp_request_from_list_by_dstip(sendip);
	if(temp_node!=NULL)
	{//by:yhy 如果存在参数sendip对应的节点,则执行内存销毁工作
		LOG_PROC("HANDLER", "%s sendip=0x%x",FN,sendip);
		temp_node = delete_fabric_arp_request_list_node(temp_node);	
	}

	return GN_OK;
}
//by:yhy  
INT4 openstack_ip_p_install_flow(param_set_p param_set, INT4 foward_type)
{
	LOG_PROC("HANDLER", "%s param_set=0x%x foward_type=%d g_proactive_flow_flag=%d g_nat_physical_switch_flag=%d",FN,param_set,foward_type,g_proactive_flow_flag,g_nat_physical_switch_flag);
	if (NULL == param_set) 
	{
		return GN_OK;
	}

	//LOG_PROC("INFO","*************foward type is%d param_set->dst_sw=0x%x %s\n", foward_type,param_set->dst_sw->sw_ip,FN);

	if (Internal_port_flow == foward_type) 
	{
		fabric_openstack_install_fabric_flows(param_set->src_port, param_set->dst_port,
											  param_set->src_security, param_set->dst_security);
	}
	else if (Internal_out_subnet_flow == foward_type) 
	{
		fabric_openstack_install_fabric_out_subnet_flows(param_set->src_port, param_set->src_gateway,
														 param_set->dst_port, param_set->dst_gateway, param_set->src_security, param_set->dst_security);
	}
	else if (Floating_ip_flow == foward_type) 
	{
        if (0 == g_proactive_flow_flag) 
		{
            fabric_openstack_floating_ip_install_set_vlan_out_flow(param_set->src_sw, param_set->dst_ip, param_set->src_mac,
																   param_set->mod_src_ip, param_set->dst_gateway_mac, param_set->src_vlanid, param_set->src_security);
			
            fabric_openstack_floating_ip_install_set_vlan_in_flow(param_set->dst_sw, param_set->mod_src_ip, param_set->src_ip,
																  param_set->packet_src_mac, param_set->dst_vlanid, param_set->dst_inport);

            install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);
			install_add_FloatingIP_ToFixIP_OutputToHost_flow(param_set->mod_src_ip);
		
        }
	}
/*	else if(Portforward_ip_flow == foward_type)
	{
		
		fabric_openstack_portforward_ip_install_set_vlan_out_before_OutFirewallQOS_flow(param_set->src_sw, param_set->dst_ip,param_set->proto,  param_set->mod_dst_port_no,  param_set->src_port_no, param_set->src_mac, \
				param_set->mod_src_ip, param_set->mod_src_port_no, param_set->dst_gateway_mac, param_set->src_vlanid, param_set->src_security);
		
		fabric_openstack_portforward_ip_install_set_vlan_out_flow(param_set->src_sw, param_set->dst_ip,param_set->proto,  param_set->mod_dst_port_no,  param_set->src_mac,
															   param_set->mod_src_ip, param_set->mod_src_port_no, param_set->dst_gateway_mac, param_set->src_vlanid, param_set->src_security);
		
		fabric_openstack_portforward_ip_install_set_vlan_in_flow(param_set->dst_sw, param_set->mod_src_ip,param_set->proto, param_set->src_port_no,  param_set->mod_src_port_no,  param_set->dst_ip, param_set->src_ip, \
															  param_set->mod_dst_port_no, param_set->packet_src_mac, param_set->dst_vlanid, param_set->dst_inport);
		
		install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);

	}*/
	else if(Clb_forward_ip_flow == foward_type)
	{
		fabric_openstack_clbforward_ip_install_set_vlan_out_flow(param_set->src_sw, param_set->dst_ip, param_set->src_mac, param_set->dst_gateway_mac, param_set->src_vlanid, param_set->src_security);
		fabric_openstack_clbforward_ip_install_set_vlan_in_flow(param_set->dst_sw, param_set->src_ip, param_set->packet_src_mac,  param_set->dst_vlanid, param_set->dst_inport);
		fabric_openstack_clbforward_ip_install_set_vlan_in_pregototable(param_set->src_sw, param_set->src_ip, FABRIC_FIREWALL_IN_TABLE, FABRIC_QOS_IN_TABLE);
			//LOG_PROC("INFO", "%s %d-- param_set->src_sw->sw_ip=0x%x  param_set->src_sw->conn_state=%d\n",FN,LN,param_set->src_sw->sw_ip, param_set->src_sw->conn_state);
			
		//	LOG_PROC("INFO", "%s %d-- param_set->dst_sw->sw_ip=0x%x  param_set->dst_sw->conn_state=%d",FN,LN,param_set->dst_sw->sw_ip, param_set->dst_sw->conn_state);
		
		install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);
	}
	else if(Clb_HA_MULTICAST == foward_type)
	{
		fabric_openstack_clbforward_multicastip_install_flow(param_set->src_sw, param_set->dst_sw, param_set->src_ip, param_set->dst_ip, param_set->dst_mac, param_set->proto, param_set->dst_vlanid);
		fabric_openstack_clbforward_multicastip_pregototable(param_set->src_sw, param_set->src_mac,  FABRIC_FIREWALL_IN_TABLE, FABRIC_QOS_IN_TABLE);
		fabric_openstack_clbforward_multicastip_install_flow(param_set->dst_sw,param_set->src_sw, param_set->mod_dst_ip, param_set->dst_ip,  param_set->src_mac, param_set->proto, param_set->src_vlanid);
		fabric_openstack_clbforward_multicastip_pregototable(param_set->dst_sw, param_set->dst_mac,  FABRIC_FIREWALL_IN_TABLE, FABRIC_QOS_IN_TABLE);
		install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);
		install_fabric_output_flow(param_set->dst_sw, param_set->dst_mac, param_set->dst_inport);
		//LOG_PROC("INFO", "####### Clb_HA_MULTICAST %s %d-- param_set->dst_sw->sw_ip=0x%x  param_set->src_ip=0x%x  param_set->dst_ip=0x%x ",FN,LN,param_set->dst_sw->sw_ip, param_set->src_ip, param_set->dst_ip);
		
		//LOG_PROC("INFO", "########Clb_HA_MULTICAST %s %d-- param_set->src_sw->sw_ip=0x%x  param_set->src_vlanid=%d  param_set->dst_vlanid=%d ",FN,LN,param_set->src_sw->sw_ip, param_set->src_vlanid, param_set->dst_vlanid);
	}
	else if (Nat_ip_flow == foward_type) 
	{
		install_fabric_nat_throughfirewall_from_inside_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
											param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->dst_inport,param_set->src_sw, param_set->dst_sw, param_set->src_security);

		install_fabric_nat_from_inside_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
											param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->dst_inport,param_set->src_sw, param_set->dst_sw, param_set->src_security);

		if (0 == get_nat_physical_switch_flag()) 
		{
			install_fabric_nat_from_external_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
												  param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->dst_inport, param_set->src_sw, param_set->dst_sw);

			//install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);
		}
		else 
		{
			install_fabric_nat_from_external_fabric_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
					param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->dst_inport, param_set->src_sw, param_set->dst_sw, param_set->dst_gateway_output);

			install_fabric_nat_from_external_fabric_host_flow(param_set->src_ip, param_set->dst_ip, param_set->src_port_no, param_set->proto, param_set->src_mac, param_set->outer_ip, param_set->outer_gateway_mac,
					param_set->outer_mac, param_set->dst_port_no, param_set->dst_vlanid, param_set->src_vlanid, param_set->src_inport, param_set->src_sw, param_set->dst_sw);
			
		}
		
		install_fabric_output_flow(param_set->src_sw, param_set->src_mac, param_set->src_inport);
	}
	else if (Internal_vip_flow == foward_type) 
	{
		fabric_openstack_install_fabric_vip_flows(param_set->src_port, param_set->dst_port, param_set->proto, param_set->vip, param_set->vip_mac,
												  param_set->src_gateway, param_set->dst_gateway, param_set->src_port_no, param_set->vip_tcp_port_no, param_set->src_security, param_set->dst_security);
	}
	else if (External_vip_flow == foward_type) 
	{
		if (0 == g_proactive_flow_flag) 
		{
			fabric_openstack_install_fabric_vip_out_flows(param_set->src_sw, param_set->src_ip, param_set->src_mac, param_set->dst_ip,
				param_set->dst_mac, param_set->dst_port, param_set->proto, param_set->vip, param_set->vip_mac, param_set->src_gateway_mac,
				param_set->dst_gateway_mac, param_set->src_port_no, param_set->vip_tcp_port_no, param_set->outer_gateway_mac,
				param_set->src_security, param_set->dst_security);
		}
	}
	else if (Internal_floating_vip_flow == foward_type) 
	{
		fabric_openstack_install_fabric_floaing_vip_flows(param_set->src_port, param_set->dst_port, param_set->proto, param_set->vip, param_set->vip_mac, param_set->mod_dst_ip, param_set->packet_dst_mac,
														  param_set->src_gateway, param_set->dst_gateway, param_set->src_port_no, param_set->vip_tcp_port_no, param_set->src_security, param_set->dst_security);
	}
	else 
	{

	}
	return GN_OK;
}

//by:yhy 根据输入参数构建arp_reply
INT4 openstack_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in)
{
	
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	gn_switch_t * ext_sw = NULL;
	LOG_PROC("HANDLER", "%s src_port=0x%x sendip=0x%x targetip=0x%x",FN,src_port,sendip,targetip);
	if(src_port==NULL)
	{//by:yhy 源主机==NULL只有两种情况:1外部主机2浮动IP
		external_port_p ext_port=NULL;
		arp_t *arp = (arp_t *)(packet_in->data);
		if(NULL == dst_port)
		{
			return GN_ERR;
		}
		LOG_PROC("HANDLER", "%s dst_port->type=%d",FN,dst_port->type);
		if (OPENSTACK_PORT_TYPE_GATEWAY == dst_port->type)
		{//by:yhy 目标端口是网关端口(外部主机)
			//NAT
			ext_port = get_external_port_by_out_interface_ip(targetip);
			if(NULL != ext_port )
			{
				ext_sw = get_ext_sw_by_dpid(ext_port->external_dpid);
				if(NULL == ext_sw)
				{
					return GN_ERR;
				}
				fabric_openstack_create_arp_reply_public(ext_port->external_outer_interface_mac, ext_port->external_outer_interface_ip,
					                                 arp->sendmac, arp->sendip, ext_sw, ext_port->external_port, packet_in);
			}
		}
		else if (OPENSTACK_PORT_TYPE_CLBLOADBALANCER == dst_port->type)
		{
			
			lb_vipfloating = find_openstack_clbaas_vipfloatingpool_by_extip(targetip);
			if(NULL == lb_vipfloating)
			{
				LOG_PROC("INFO", "%s %d can't get clb host\n",FN,LN);
				return GN_ERR;
			}
			
			LOG_PROC("INFO", "%s lb_vipfloating->inside_ip=0x%x",FN,lb_vipfloating->inside_ip);
			ext_port= get_external_port_by_hostip(lb_vipfloating->inside_ip);
			
			if(NULL != ext_port )
			{
				ext_sw = get_ext_sw_by_dpid(ext_port->external_dpid);
				if(NULL == ext_sw)
				{
					
					LOG_PROC("ERROR", "%s_%d can't get ext sw",FN,LN);
					return GN_ERR;
				}
				fabric_openstack_create_arp_reply_public(dst_port->mac, targetip,arp->sendmac, arp->sendip, ext_sw, ext_port->external_port, packet_in);
			}
			else
			{
				LOG_PROC("ERROR", "%s %d  can't get external port",FN,LN);
			}
		}
		else
		{//by:yhy 浮动IP
			external_floating_ip_p fip = find_external_floating_ip_by_floating_ip(targetip);
			if (NULL != fip) 
			{
				external_port_p ext_port = get_external_port_by_floatip(fip->floating_ip);
				p_fabric_host_node float_port = find_fabric_host_port_by_port_id(fip->port_id);
			
				if (NULL != ext_port) 
				{
					ext_sw = get_ext_sw_by_dpid(ext_port->external_dpid);
					if(NULL == ext_sw)
					{
						return GN_ERR;
					}
					LOG_PROC("INFO", "%s %d-- arp->sendmac=0x%x arp->sendip=0x%x ext_sw->sw_ip=0x%x ext_port->external_port=%d targetip=0x%x",FN,LN, arp->sendmac,arp->sendip,ext_sw->sw_ip, ext_port->external_port,targetip);
					arp_t *arp = (arp_t *)(packet_in->data);
					fabric_openstack_create_arp_reply_public(float_port->mac, targetip,arp->sendmac,arp->sendip, ext_sw, ext_port->external_port, packet_in);
				}
			}
		}
	}
	else
	{//by:yhy 源主机已知
		if(dst_port!=NULL)
		{//by:yhy 目标主机已知

			LOG_PROC("INFO", "%s %d src_port->port=%d dst_port->ip_list[0]=0x%x src_port->ip_list[0]=0x%x",FN,LN,src_port->port,dst_port->ip_list[0],src_port->ip_list[0]);
			fabric_openstack_create_arp_reply(src_port,dst_port,packet_in);
		}
	}
	return GN_OK;
}
//by:yhy arp reply 输出(called by handler  init_handler)
INT4 openstack_arp_reply_output(UINT8 external_dpid,p_fabric_host_node src,p_fabric_host_node dst,UINT4 targetIP, packet_in_info_t *packet_in)
{
	arp_t *arp = (arp_t *)(packet_in->data);
	UINT1 arp_dst_mac[6] = {0};
	memcpy(arp_dst_mac, arp->eth_head.dest, 6);
	//fabric_push_flow_queue(src,arp->sendip, dst, targetIP);
	memcpy(arp->eth_head.dest,dst->mac, 6);
	arp->targetip = targetIP;
	memcpy(arp->targetmac,dst->mac, 6);

	
	LOG_PROC("HANDLER", "%s %d dst->sw=0x%x  dst->type=%d arp->sendip=0x%x arp->targetip=0x%x",FN,LN,dst->sw,  dst->type, arp->sendip, arp->targetip);
	if (NULL != dst->sw) 
	{	
		fabric_packet_output(dst->sw,packet_in,dst->port);
	}

	if( OPENSTACK_PORT_TYPE_GATEWAY == dst->type)
	{
		//by:yhy 更新对外网关 mac
		UINT4 S_ExternalPort =packet_in->inport;
		update_openstack_external_gateway_mac(external_dpid,arp->sendip, arp->sendmac, arp->targetip, arp_dst_mac,S_ExternalPort);
	}
	
	return GN_OK;
}
//by:yhy 根据给定参数进行洪泛
INT4 openstack_ip_p_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,UINT1* srcmac,packet_in_info_t *packet_in)
{
	LOG_PROC("HANDLER", "%s ",FN);
	fabric_opnestack_create_arp_flood(sendip, targetip, srcmac);

	return GN_OK;
}

INT4 openstack_ip_p_broadcast(packet_in_info_t *packet_in)
{
	p_fabric_host_node src_port = NULL;
	p_fabric_host_node dst_port = NULL;

	ip_t *p_ip = (ip_t *)(packet_in->data);
	if (NULL == p_ip) 
	{
		LOG_PROC("ERROR", "%s_%d NULL == p_ip",FN,LN);
		return GN_ERR;
	}

	src_port = get_fabric_host_from_list_by_mac(p_ip->eth_head.src);
	dst_port = get_fabric_host_from_list_by_mac(p_ip->eth_head.dest);
	
	if (IPPROTO_UDP == p_ip->proto)
	{
		udp_t* udp = (udp_t*)p_ip->data;
		
		// if dhcp request handler
		if (67 == ntohs(udp->dport)) {
			fabric_openstack_dhcp_request_handle(packet_in, src_port);
		}
		// if dhcp reply handler
		else if (68 == ntohs(udp->dport)) {
			fabric_openstack_dhcp_reply_handle(packet_in, dst_port);
		}
		// ohter case
		else {
			fabric_openstack_packet_flood(packet_in);
		}
	}
	else {
		fabric_openstack_packet_flood(packet_in);
	}
	return GN_OK;
}
//by:yhy why? 为何为空
INT4 openstack_ip_packet_output(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in)
{
	return GN_OK;
}

//by:yhy 根据dpid查找对应的对外sw
gn_switch_t* get_ext_sw_by_dpid(UINT8 dpid)
{
	gn_switch_t * ext_sw = NULL;
	ext_sw = find_sw_by_dpid(dpid);
	if (NULL == ext_sw) 
	{
		LOG_PROC("INFO", "%s dpid=0x%x gateway sw is NULL ", FN,dpid);
		return NULL;
	}
	return ext_sw;
}



/*
 * dhcp request
 */
void fabric_openstack_dhcp_request_handle(packet_in_info_t *packet_in, p_fabric_host_node src_port)
{
	p_fabric_host_node dhcp_port = NULL;
	openstack_port_p src_port_p = NULL;

	if ((NULL == src_port) || (NULL == src_port->data)) {
		return ;
	}

	src_port_p = (openstack_port_p)src_port->data;
	dhcp_port = find_openstack_app_dhcp_by_subnet_id(src_port_p->subnet_id);

	if ((NULL == dhcp_port) || (NULL == dhcp_port->sw) || (0 == dhcp_port->port)) {
		// if dhcp not exist, flood
		// printf("%s-flood\n", FN);
		fabric_openstack_packet_flood(packet_in);
	}
	else {
		// packet out
		// printf("%s-packetout\n", FN);
		fabric_openstack_packet_output(dhcp_port->sw, packet_in, dhcp_port->port);
	}
	return;
};
/*
 * dhcp reply
 */
void fabric_openstack_dhcp_reply_handle(packet_in_info_t *packet_in, p_fabric_host_node dst_port)
{
	if ((NULL == dst_port) || (NULL == dst_port->sw) || (0 == dst_port->port)) {
		// if dhcp not exist, flood
		fabric_openstack_packet_flood(packet_in);
	}
	else {
		// packet out
		fabric_openstack_packet_output(dst_port->sw, packet_in, dst_port->port);
	}
	return;
}



void fabric_openstack_install_fabric_flows(p_fabric_host_node src_port,p_fabric_host_node dst_port,
										   security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	// display port info
//	LOG_PROC("INFO","Sourt Port Info:");
//	fabric_openstack_show_port(src_port);
//	LOG_PROC("INFO","Destination Port Info:");
//	fabric_openstack_show_port(dst_port);
	if ((NULL== src_port) || (NULL == dst_port) || (NULL == src_port->sw) || (NULL == dst_port->sw) 
						  || (0 == src_port->ip_list[0]) || (0 == dst_port->ip_list[0])) 
	{
		return ;
	}

	src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);
	if(src_port->sw == dst_port->sw)
	{
		// printf("same switch\n");
		install_fabric_same_switch_security_before_OutFirewallQOS_flow(src_port->sw,src_port->mac,src_port->port, src_security);
		install_fabric_same_switch_security_before_OutFirewallQOS_flow(dst_port->sw,dst_port->mac,dst_port->port, dst_security);
		install_fabric_same_switch_security_flow(src_port->sw,src_port->mac,src_port->port, src_security);
		install_fabric_same_switch_security_flow(dst_port->sw,dst_port->mac,dst_port->port, dst_security);
	}
	else
	{
		// printf("different switch\n");
		//install_fabric_push_tag_security_flow(src_port->sw,dst_port->ip_list[0], dst_port->mac,dst_tag, src_security);
		//install_fabric_push_tag_security_flow(dst_port->sw,src_port->ip_list[0], src_port->mac,src_tag, dst_security);
		//install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		//install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
		
		//LOG_PROC("INFO","*******************%s %d src_port->ip_list[0]= 0x%x dst_port->ip_list[0]=0x%x",FN, LN,src_port->ip_list[0],dst_port->ip_list[0]);

		if((OPENSTACK_PORT_TYPE_CLBLOADBALANCER == src_port->type)||(OPENSTACK_PORT_TYPE_CLBLOADBALANCER_HA == src_port->type))
		{
			//LOG_PROC("INFO","*******************%s %d src_port->ip_list[0]= 0x%x dst_port->ip_list[0]=0x%x src_tag=%d dst_tag=%d",FN, LN,src_port->ip_list[0],dst_port->ip_list[0],src_tag, dst_tag);
			install_fabric_push_tag_security_AddLocalDstIp_pregototable(src_port->sw,src_port->ip_list[0], FABRIC_FIREWALL_IN_TABLE, FABRIC_QOS_IN_TABLE);
			install_fabric_push_tag_security_AddLocalSrc_postgototable(src_port->sw,src_port->ip_list[0], FABRIC_FIREWALL_OUT_TABLE, FABRIC_QOS_OUT_TABLE);
		}
		if((OPENSTACK_PORT_TYPE_CLBLOADBALANCER == dst_port->type)||(OPENSTACK_PORT_TYPE_CLBLOADBALANCER_HA == dst_port->type))
		{
			//LOG_PROC("INFO","*******************%s %d src_port->ip_list[0]= 0x%x dst_port->ip_list[0]=0x%x src_tag=%d dst_tag=%d",FN, LN,src_port->ip_list[0],dst_port->ip_list[0],src_tag, dst_tag);
			install_fabric_push_tag_security_AddLocalDstIp_pregototable(dst_port->sw,dst_port->ip_list[0], FABRIC_FIREWALL_IN_TABLE, FABRIC_QOS_IN_TABLE);
			install_fabric_push_tag_security_AddLocalSrc_postgototable(dst_port->sw,dst_port->ip_list[0], FABRIC_FIREWALL_OUT_TABLE, FABRIC_QOS_OUT_TABLE);
		}
		
		install_fabric_push_tag_security_flow_AddLocalSrcMAC(src_port->sw,dst_port->ip_list[0], src_port->mac,dst_port->mac,dst_tag, src_security);
		install_fabric_push_tag_security_flow_AddLocalSrcMAC(dst_port->sw,src_port->ip_list[0], dst_port->mac,src_port->mac,src_tag, dst_security);
	}
	install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
	install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);

	return;
};
void fabric_openstack_install_fabric_out_subnet_flows(p_fabric_host_node src_port,p_fabric_host_node src_gateway,
		p_fabric_host_node dst_port,p_fabric_host_node dst_gateway, security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	openstack_port_p srcport_p = NULL;
	openstack_port_p dstport_p = NULL;
	openstack_router_outerinterface_p srcnode_router_outerinterface = NULL;
	openstack_router_outerinterface_p dstnode_router_outerinterface = NULL;
	// display port info
//	LOG_PROC("INFO","Sourt Port Info:");
//	fabric_openstack_show_port(src_port);
//	LOG_PROC("INFO","Destination Port Info:");
//	fabric_openstack_show_port(dst_port);
	if ((NULL== src_port) || (NULL == dst_port) || (NULL == src_port->sw) || (NULL == dst_port->sw) 
						  || (0 == src_port->ip_list[0]) || (0 == dst_port->ip_list[0])) {
		return ;
	}
	srcport_p = (openstack_port_p)src_port->data;
	dstport_p = (openstack_port_p)dst_port->data;
	if((NULL == srcport_p)||(NULL ==  dstport_p))
	{
		return ;
	}
	srcnode_router_outerinterface = find_openstack_router_outerinterface_by_networkAndsubnetid(srcport_p->network_id, srcport_p->subnet_id);
	dstnode_router_outerinterface = find_openstack_router_outerinterface_by_networkAndsubnetid(dstport_p->network_id, dstport_p->subnet_id);

	if((NULL == srcnode_router_outerinterface)||(NULL == dstnode_router_outerinterface)||(srcnode_router_outerinterface != dstnode_router_outerinterface))
	{
		LOG_PROC("INFO","src_port 0x%x and  dst_port 0x%x are in different router",src_port->ip_list[0],dst_port->ip_list[0]);
		return ;
	}
	
	src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);
	if(src_port->sw == dst_port->sw){
		install_fabric_same_switch_out_subnet_flow(src_port->sw,src_gateway->mac,dst_port->mac,dst_port->ip_list[0],dst_port->port, src_security);
		install_fabric_same_switch_out_subnet_flow(dst_port->sw,dst_gateway->mac,src_port->mac,src_port->ip_list[0],src_port->port, dst_security);
	}else{

	//LOG_PROC("INFO","#############################%s %d src_port->ip_list[0]= 0x%x dst_port->ip_list[0]=0x%x",FN, LN,src_port->ip_list[0],dst_port->ip_list[0]);

		install_fabric_push_tag_out_subnet_flow(src_port->sw,src_gateway->mac,dst_port->mac,dst_port->ip_list[0],dst_tag, src_security);
		install_fabric_push_tag_out_subnet_flow(dst_port->sw,dst_gateway->mac,src_port->mac,src_port->ip_list[0],src_tag, dst_security);
		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);

	}
	return;
};

int check_fabric_openstack_subnet_dhcp_gateway(p_fabric_host_node port,openstack_subnet_p subnet){
	return ((port == subnet->dhcp_port) || (port == subnet->gateway_port))?1:0;
};
/*****************************
 * intern function: packet out
 *****************************/
/* by:yhy openflow输出一个packet_out
 * out put the packet
 */
void fabric_openstack_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport){
	if (NULL != sw) 
	{
		packout_req_info_t pakout_req;
		pakout_req.buffer_id = packet_in_info->buffer_id;
		pakout_req.inport = OFPP13_CONTROLLER;
		pakout_req.outport = outport;
		pakout_req.max_len = 0xff;
		pakout_req.xid = packet_in_info->xid;
		pakout_req.data_len = packet_in_info->data_len;
		pakout_req.data = packet_in_info->data;
		if(CONNECTED == sw->conn_state)
			sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
	}
};
/*
 * flood
 */
void fabric_openstack_packet_flood(packet_in_info_t *packet_in_info){
	packout_req_info_t pakout_req;
	gn_switch_t *sw = NULL;
	UINT2 i = 0,j=0;
	pakout_req.buffer_id = 0xffffffff;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;
//	pakout_req.outport = OFPP13_FLOOD;

	// find all switch
	for(i = 0; i < g_server.max_switch; i++){
		if (CONNECTED == g_server.switches[i].conn_state){
			sw = &g_server.switches[i];
//			sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
			// find switch's outter ports

			for(j=0; j<sw->n_ports; j++){
				// check port state is ok and also not connect other switch(neighbor)
				//if(sw->neighbor[j] == NULL){
				if(FALSE == sw->neighbor[j]->bValid){
					pakout_req.outport = sw->ports[j].port_no;
					sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
				}
			}
		}
	}
	return;
};
/*
 * flood in subnet
 */
void fabric_openstack_packet_flood_in_subnet(packet_in_info_t *packet_in_info,char* subnet_id,UINT4 subnet_port_num){
	p_fabric_host_node port_list[subnet_port_num+3];
	p_fabric_host_node temp = NULL;
	UINT4 i = 0,port_num = 0;
	packout_req_info_t pakout_req;
	gn_switch_t *sw = NULL;

	pakout_req.buffer_id = 0xffffffff;
	pakout_req.inport = OFPP13_CONTROLLER;
	pakout_req.max_len = 0xff;
	pakout_req.xid = packet_in_info->xid;
	pakout_req.data_len = packet_in_info->data_len;
	pakout_req.data = packet_in_info->data;


	port_num = find_fabric_host_ports_by_subnet_id(subnet_id,port_list);
	for(i = 0 ; i < port_num ; i++){
		temp = port_list[i];
		if(temp != NULL && temp->port != 0 && temp->sw != NULL){
			sw = temp->sw;
			if(CONNECTED == sw->conn_state)
				sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
		}
	}
	return;
};
//by:yhy 涉及到对openstack的内网端口进行arp_reply
//by:yhy 根据已知的源主机,目标主机,packet_in包来构建ARP reply包
void fabric_openstack_create_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in_info)
{
    packout_req_info_t packout_req_info;
    arp_t new_arp_pkt;
    arp_t *arp = (arp_t *)(packet_in_info->data);

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = OFPP13_CONTROLLER;
    packout_req_info.outport = src_port->port;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = packet_in_info->xid;
    packout_req_info.data_len = sizeof(arp_t);
    packout_req_info.data = (UINT1 *)&new_arp_pkt;

    memcpy(&new_arp_pkt, arp, sizeof(arp_t));
    memcpy(new_arp_pkt.eth_head.src, dst_port->mac, 6);
    memcpy(new_arp_pkt.eth_head.dest, src_port->mac, 6);
    new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
    new_arp_pkt.opcode = htons(2);
    new_arp_pkt.sendip = dst_port->ip_list[0];
    new_arp_pkt.targetip = src_port->ip_list[0];
    memcpy(new_arp_pkt.sendmac, dst_port->mac, 6);
    memcpy(new_arp_pkt.targetmac, src_port->mac, 6);
	if(CONNECTED == src_port->sw->conn_state)
    	src_port->sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](src_port->sw, (UINT1 *)&packout_req_info);
};
//by:yhy 涉及到对openstack的外网端口进行arp_reply
void fabric_openstack_create_arp_reply_public(UINT1* srcMac,UINT4 srcIP, UINT1* dstMac,UINT4 dstIP,gn_switch_t* sw, UINT4 outPort,packet_in_info_t *packet_in_info)
{
    packout_req_info_t packout_req_info;
    arp_t new_arp_pkt;
    arp_t *arp = (arp_t *)(packet_in_info->data);

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = OFPP13_CONTROLLER;
    packout_req_info.outport = outPort;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = packet_in_info->xid;
    packout_req_info.data_len = sizeof(arp_t);
    packout_req_info.data = (UINT1 *)&new_arp_pkt;

    memcpy(&new_arp_pkt, arp, sizeof(arp_t));
    memcpy(new_arp_pkt.eth_head.src,srcMac , 6);
    memcpy(new_arp_pkt.eth_head.dest,dstMac , 6);
    new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
    new_arp_pkt.opcode = htons(2);
    new_arp_pkt.sendip = srcIP;
    new_arp_pkt.targetip = dstIP;
    memcpy(new_arp_pkt.sendmac, srcMac, 6);
    memcpy(new_arp_pkt.targetmac,dstMac , 6);
	if(CONNECTED == sw->conn_state)
    	sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
};

void fabric_openstack_external_arp_mac(){
//	packout_req_info_t packout_req_info;
//    arp_t new_arp_pkt;
//    packout_req_info.buffer_id = 0xffffffff;
//	packout_req_info.inport = 0xfffffffd;
//	packout_req_info.outport = 0;
//	packout_req_info.max_len = 0xff;
//	packout_req_info.xid = 0;
//	packout_req_info.data_len = sizeof(arp_t);
//	packout_req_info.data = (UINT1 *)&new_arp_pkt;
//
//	memcpy(&new_arp_pkt, arp, sizeof(arp_t));
//	memcpy(new_arp_pkt.eth_head.src, g_controller_mac, 6);
//	memcpy(new_arp_pkt.eth_head.dest, arp->eth_head.src, 6);
//	new_arp_pkt.eth_head.proto = htons(ETHER_ARP);
//	new_arp_pkt.opcode = htons(2);
//	new_arp_pkt.sendip = arp->targetip;
//	new_arp_pkt.targetip = arp->sendip;
//	memcpy(new_arp_pkt.sendmac, g_controller_mac, 6);
//	memcpy(new_arp_pkt.targetmac, arp->sendmac, 6);
//
//	if(sw->ofp_version == OFP10_VERSION)
//	{
//		sw->msg_driver.msg_handler[OFPT_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
//	}
//	else if(sw->ofp_version == OFP13_VERSION)
//	{
//		sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
//	}
};

//by:yhy 根据给定参数进行洪泛(查找dst_ip主机的MAC,将结果返回给src_ip,src_mac对应的主机)
void fabric_opnestack_create_arp_flood(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac)
{
	packet_in_info_t packout_req_info;
	arp_t* new_arp_pkt = (arp_t*)gn_malloc(sizeof(arp_t));

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.xid = 0;
	packout_req_info.data_len = sizeof(arp_t);
	packout_req_info.data = (UINT1 *)new_arp_pkt;

	memcpy(new_arp_pkt->eth_head.src, src_mac, 6);
	memcpy(new_arp_pkt->eth_head.dest, arp_broadcat_mac, 6);
	new_arp_pkt->eth_head.proto = htons(ETHER_ARP);
	new_arp_pkt->hardwaretype = htons(1);
	new_arp_pkt->prototype = htons(ETHER_IP);
	new_arp_pkt->hardwaresize = 0x6;
	new_arp_pkt->protocolsize = 0x4;
	new_arp_pkt->opcode = htons(1);
	new_arp_pkt->sendip = src_ip;
	new_arp_pkt->targetip=dst_ip;

	memcpy(new_arp_pkt->sendmac, src_mac, 6);
	memcpy(new_arp_pkt->targetmac, arp_zero_mac, 6);

	p_fabric_host_node src_node = get_fabric_host_from_list_by_ip(src_ip);
	//by:yhy 标记src_ip,dst_ip之间已经有arp请求
	fabric_add_into_arp_request(src_node,src_ip,dst_ip);
	//by:yhy 将arp请求存入发送队列
	fabric_push_arp_flood_queue(dst_ip, &packout_req_info);

	
}
void fabric_opnestack_create_clb_arpflood(gn_switch_t* sw, UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT1* dst_mac)
{
	int j = 0;
	int outputno = 0;
	packet_in_info_t packout_req_info;
	arp_t* new_arp_pkt = (arp_t*)gn_malloc(sizeof(arp_t));

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.xid = 0;
	packout_req_info.data_len = sizeof(arp_t);
	packout_req_info.data = (UINT1 *)new_arp_pkt;

	memcpy(new_arp_pkt->eth_head.src, src_mac, 6);
	memcpy(new_arp_pkt->eth_head.dest, arp_broadcat_mac, 6);
	new_arp_pkt->eth_head.proto = htons(ETHER_ARP);
	new_arp_pkt->hardwaretype = htons(1);
	new_arp_pkt->prototype = htons(ETHER_IP);
	new_arp_pkt->hardwaresize = 0x6;
	new_arp_pkt->protocolsize = 0x4;
	new_arp_pkt->opcode = htons(1);
	new_arp_pkt->sendip = src_ip;
	new_arp_pkt->targetip=dst_ip;

	memcpy(new_arp_pkt->sendmac, src_mac, 6);
	memcpy(new_arp_pkt->targetmac, arp_zero_mac, 6);

	if (sw&&(CONNECTED == sw->conn_state))
	{
		//by:yhy 遍历该交换机的所有端口
		for(j=0; j<sw->n_ports; j++)
		{
			// check port state is ok and also not connect other switch(neighbor)
			//if(sw->ports[j].state == 0 && sw->neighbor[j] == NULL)
			if(sw->neighbor[j]&&(FALSE==sw->neighbor[j]->bValid)&&(SW_CON != sw->ports[j].type)&&(0 != sw->ports[j].state))
			{
				//LOG_PROC("INFO", "***************%s %d sw->sw_ip=0x%x port_no=%d src_ip=0x%x dst_ip=0x%x",FN,LN,sw->sw_ip, sw->ports[j].port_no, src_ip,dst_ip );
				outputno = sw->ports[j].port_no;
				//sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
				
				fabric_openstack_packet_output(sw, &packout_req_info, outputno );
			}
		}
	}
	if(NULL != new_arp_pkt)
	{
		gn_free(&new_arp_pkt);
	}
	

	
}

void fabric_opnestack_create_icmp_flood(gn_switch_t* sw, UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT1* dst_mac)
{
	int j = 0;
	int outputno = 0;
	packet_in_info_t packout_req_info;
	UINT1 data_len = 60+sizeof(ether_t);

	ip_t* new_ip = (ip_t*)gn_malloc(data_len);
	memset(new_ip, 0, data_len);
	icmp_t icmp_pkt;
	icmp_t* new_icmp = &icmp_pkt;
	memset(new_icmp, 0, sizeof(icmp_t));

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = -3;
	packout_req_info.xid = 0;
	packout_req_info.data_len = data_len ;
	packout_req_info.data = (UINT1 *)new_ip;

	new_icmp->type = 8;
	new_icmp->code = 0;
	new_icmp->id = 0;
	new_icmp->seq = 0;
	new_icmp->cksum = 0;
	new_icmp->cksum = calc_ip_checksum((UINT2*)&new_icmp->type, sizeof(icmp_t));

	memcpy(new_ip->eth_head.src, src_mac, 6);
	memcpy(new_ip->eth_head.dest, dst_mac, 6);
	new_ip->eth_head.proto = htons(0x0800);

	new_ip->hlen = 0x45;
	new_ip->ttl = 64;
	new_ip->len = htons(data_len - sizeof(ether_t));
	new_ip->src = src_ip;
	new_ip->dest = dst_ip;
	new_ip->ipid = htons(0x02);
	new_ip->fragoff = 0;
	new_ip->proto = IPPROTO_ICMP;
	new_ip->cksum = 0;
	new_ip->cksum = calc_ip_checksum((UINT2*)&new_ip->hlen, sizeof(ip_t)-sizeof(ether_t));
	memcpy(new_ip->data, new_icmp, sizeof(icmp_t));

	if (sw&&(CONNECTED == sw->conn_state))
	{
		//by:yhy 遍历该交换机的所有端口
		for(j=0; j<sw->n_ports; j++)
		{
			// check port state is ok and also not connect other switch(neighbor)
			//if(sw->ports[j].state == 0 && sw->neighbor[j] == NULL)
			if(sw->neighbor[j]&&(FALSE==sw->neighbor[j]->bValid)&&(SW_CON != sw->ports[j].type))
			{
				//LOG_PROC("INFO", "***************%s %d sw->sw_ip=0x%x port_no=%d src_ip=0x%x dst_ip=0x%x",FN,LN,sw->sw_ip, sw->ports[j].port_no, src_ip,dst_ip );
				outputno = sw->ports[j].port_no;
				//sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
				
				fabric_openstack_packet_output(sw, &packout_req_info, outputno );
			}
		}
	}
	if(NULL != new_ip)
	{
		gn_free(&new_ip);
	}
	
}

void fabric_opnestack_create_icmp_flood_allsw( UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT1* dst_mac)
{
	int i=0, j = 0;
	int outputno = 0;
	gn_switch_t* sw = NULL;
	packet_in_info_t packout_req_info;
	UINT1 data_len = 60+sizeof(ether_t);

	ip_t* new_ip = (ip_t*)gn_malloc(data_len);
	memset(new_ip, 0, data_len);
	icmp_t icmp_pkt;
	icmp_t* new_icmp = &icmp_pkt;
	memset(new_icmp, 0, sizeof(icmp_t));

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = -3;
	packout_req_info.xid = 0;
	packout_req_info.data_len = data_len ;
	packout_req_info.data = (UINT1 *)new_ip;

	new_icmp->type = 8;
	new_icmp->code = 0;
	new_icmp->id = 0;
	new_icmp->seq = 1;
	new_icmp->cksum = 0;
	new_icmp->cksum = calc_ip_checksum((UINT2*)&new_icmp->type, sizeof(icmp_t));

	memcpy(new_ip->eth_head.src, src_mac, 6);
	memcpy(new_ip->eth_head.dest, dst_mac, 6);
	new_ip->eth_head.proto = htons(0x0800);

	new_ip->hlen = 0x45;
	new_ip->ttl = 64;
	new_ip->len = htons(data_len - sizeof(ether_t));
	new_ip->src = src_ip;
	new_ip->dest = dst_ip;
	new_ip->ipid = htons(0x02);
	new_ip->fragoff = 0;
	new_ip->proto = IPPROTO_ICMP;
	new_ip->cksum = 0;
	new_ip->cksum = calc_ip_checksum((UINT2*)&new_ip->hlen, sizeof(ip_t)-sizeof(ether_t));
	memcpy(new_ip->data, new_icmp, sizeof(icmp_t));

	for( i = 0; i < g_server.max_switch; i++)
	{
		sw = &g_server.switches[i];
		if (sw&&(CONNECTED == sw->conn_state))
		{
			//by:yhy 遍历该交换机的所有端口
			for(j=0; j<sw->n_ports; j++)
			{
				// check port state is ok and also not connect other switch(neighbor)
				//if(sw->ports[j].state == 0 && sw->neighbor[j] == NULL)
				if(sw->neighbor[j]&&(FALSE==sw->neighbor[j]->bValid)&&(SW_CON != sw->ports[j].type))
				{
					//LOG_PROC("INFO", "***************%s %d sw->sw_ip=0x%x port_no=%d src_ip=0x%x dst_ip=0x%x",FN,LN,sw->sw_ip, sw->ports[j].port_no, src_ip,dst_ip );
					outputno = sw->ports[j].port_no;
					//sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
					
					fabric_openstack_packet_output(sw, &packout_req_info, outputno );
				}
			}
		}
	}
	if(NULL != new_ip)
	{
		gn_free(&new_ip);
	}
	
}


//by:yhy 根据IP包的内容决定下一步操作的类型
INT4 openstack_ip_packet_compute_src_dst_forward(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in, param_set_p param_set)
{
	INT4 foward_type = IP_DROP;
	UINT4 destport = 0;
	UINT4 srcport = 0;
	ip_t *ip = (ip_t *)(packet_in->data);

	if (IPPROTO_ICMP == ip->proto) 
	{
		icmp_t* icmp = (icmp_t*)ip->data;
		
	}
	else if (IPPROTO_TCP == ip->proto)
	{
		tcp_t* tcp = (tcp_t*)ip->data;
		destport = tcp->dport;
		srcport = tcp->sport;
	}
	else if (IPPROTO_UDP == ip->proto) 
	{
		udp_t* udp = (udp_t*)ip->data;
		destport = udp->dport;		
	}

	if ((NULL != dst_port) && (OPENSTACK_PORT_TYPE_GATEWAY == dst_port->type))
	{//by:yhy 对外
		dst_port = NULL;
	}
	
	if (NULL != get_external_floating_ip_by_floating_ip(ip->dest))
	{//by:yhy 目标IP是浮动IP
		dst_port = NULL;
	}
	
	if ((0 != destport)&&(NULL != get_internal_dnatip_by_external_ip(ip->dest, destport, ip->proto)))
	{
		dst_port = NULL;
	}
	
	LOG_PROC("HANDLER", "ip->dest=0x%x src_port=0x%x dst_port=0x%x ip->proto=0x%x",ip->dest,src_port ,dst_port,ip->proto);
	if ((ip->dest == ntohl(g_reserve_ip)) && (src_port)) 
	{//by:yhy 目标IP是控制器的IP
		if (IPPROTO_ICMP == ip->proto) 
		{
			icmp_t* icmp = (icmp_t*)ip->data;
		//	update_openstack_lbaas_listener_member_status(LBAAS_LISTENER_PING, src_port, ntohs(icmp->id), 0, 0);
		}
        else if (IPPROTO_TCP == ip->proto) 
		{
            tcp_t* tcp = (tcp_t*)ip->data;
            update_openstack_lbaas_listener_member_status(LBAAS_LISTENER_TCP, src_port, ntohl(tcp->ack), ntohs(tcp->sport), tcp->code);
        }
		return IP_DROP;
	}

	//LOG_PROC("INFO", "%s_%d",FN,LN);
	if ((NULL != src_port) && (NULL != dst_port))
	{//by:yhy 纯内网 internal network

		if (NULL != find_openstack_lbaas_pool_by_ip(ip->src) || (NULL != find_openstack_lbaas_pool_by_ip(ip->dest))) 
		{
			foward_type = internal_packet_compute_vip_forward(src_port, dst_port, ip->dest, param_set, ip);
		}
		else 
		{
			foward_type = internal_packet_compute_forward(src_port, dst_port, ip->dest, param_set, ip);
		}
		//LOG_PROC("INFO", "%s_%d",FN,LN);
	}
	else if ((NULL == src_port) && (g_broad_ip != ip->dest))
	{//by:yhy 外网到内网from external to internal
		foward_type = external_floatingip_dnat_packet_in_compute_forward(src_port, ip->src, ip->dest, destport ,packet_in, ip->proto, param_set);
	}
	else if (NULL == dst_port)
	{//by:yhy 内网到外网from internal to external
		// if dest ip is dns or fobidden ip
		if (ip->dest == g_openstack_fobidden_ip)
		{//by:yhy 目标IP是OpenStack的保留IP
			return IP_DROP;
		}
		else if(g_broad_ip == ip->dest)
		{//by:yhy 目标IP是广播IP
			return BROADCAST_DHCP;
		}
		else if(ip->proto == PROTO_VRRP)
		{	
			foward_type = multicast_packet_out_compute_forward(src_port, ip->src, ip->dest, packet_in, ip->proto, param_set);
		}
		else if(ip->proto == IPPROTO_UDP) 
		{//by:yhy UDP协议
			udp_t* udp = (udp_t*)(ip->data);
			if ((udp->sport == htons(68) && udp->dport == htons(67)) || (udp->sport == htons(67) && udp->dport == htons(68)))
			{//by:yhy DHCP采用UDP协议,使用67,68两个端口
				return BROADCAST_DHCP;
			}
			else 
			{
				foward_type = external_dnat_packet_out_compute_forward(src_port, ip->src, ip->dest, packet_in, ip->proto, param_set);
			}
		}
		else 
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			foward_type = external_dnat_packet_out_compute_forward(src_port, ip->src, ip->dest, packet_in, ip->proto, param_set);
		}
	}
	else
	{
		// do nothing
	}
	return foward_type;
}

INT4 internal_packet_compute_vip_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip, param_set_p param_set, ip_t *ip)
{
	// define the value
	UINT4 vip_tcp_port_no = 0;
	// p_fabric_host_node vip_dst_port = NULL;
	p_fabric_host_node vip_port = NULL;
	p_fabric_host_node src_gw = NULL;
	p_fabric_host_node dst_gw = NULL;
	p_fabric_host_node vip_gw = NULL;

	if (src_port) 
	{
		// if source is virtual ip
		if (find_openstack_lbaas_pool_by_ip(src_port->ip_list[0])) 
		{
			// printf("src is virtual ip\n", FN);
			return IP_DROP;
		}

		// if the src ip in the same virtual ip pool, drop
		if (find_openstack_lbaas_member_by_ip(dst_port->ip_list[0], src_port->ip_list[0])) 
		{
			// printf("src is inside ip\n", FN);
			return IP_DROP;
		}
	}


	// get fixed ip
	UINT4 lb_ip = get_openstack_lbaas_ip_by_ip_proto(targetip, ip->proto);

	// if exist, save the dest
	if (lb_ip) 
	{
		// printf("get member ip\n", FN);
		// nat_show_ip(lb_ip);

		vip_port = dst_port;
		param_set->vip = dst_port->ip_list[0];
		memcpy(param_set->vip_mac, dst_port->mac, 6);
	}
	else 
	{
		return IP_DROP;
	}

	// get the fixed port
	dst_port = get_fabric_host_from_list_by_ip(lb_ip);

	if (NULL == dst_port)
	{
		LOG_PROC("INFO", "load balance dst port is NULL");
		return IP_DROP;
	}

	// if src port and vip not in same pool
	if ((src_port) && (dst_port))
	{
		openstack_port_p src_port_p = (openstack_port_p)src_port->data;
		openstack_port_p dst_port_p = (openstack_port_p)dst_port->data;

		if (((src_port_p) && (dst_port_p)) &&  (0 == strcmp(src_port_p->tenant_id, dst_port_p->tenant_id)))
		{
			// do nothing
		}
		else 
		{
			return IP_DROP;
		}
	}

	// if dst port sw not exist, save and flood
	if ((NULL == dst_port->sw) || (0 == dst_port->port)) 
	{
		// printf("Can't find ip start flood %s\n", FN);
		if (src_port) 
		{
			param_set->src_ip = src_port->ip_list[0];
			memcpy(param_set->src_mac, src_port->mac, 6);
			param_set->dst_ip = dst_port->ip_list[0];
			return IP_FLOOD;
		}
		else 
		{
			return create_arp_flood_parameter(dst_port->ip_list[0], dst_port, param_set);
		}
	}
	else 
	{
		if (src_port)
			src_gw = find_openstack_app_gateway_by_host(src_port);

		if (dst_port)
			dst_gw = find_openstack_app_gateway_by_host(dst_port);

		if (vip_port)
			vip_gw = find_openstack_app_gateway_by_host(vip_port);

		param_set->src_gateway = (src_gw == vip_gw) ? NULL : src_gw;
		param_set->dst_gateway = (dst_gw == vip_gw) ? NULL : dst_gw;

		param_set->src_port = src_port;
		param_set->dst_port = dst_port;
		param_set->proto = ip->proto;

		if (ip->proto == IPPROTO_TCP)
		{
			tcp_t* tcp = (tcp_t*)ip->data;

			param_set->src_port_no = ntohs(tcp->sport);

			vip_tcp_port_no = create_openstack_lbaas_connect(ip->src, dst_port->ip_list[0], vip_port->ip_list[0], param_set->src_port_no);
			// printf("create vip port:%d", vip_tcp_port_no);

			param_set->vip_tcp_port_no = vip_tcp_port_no;

			return Internal_vip_flow;
		}
	}

	return IP_DROP;
}


INT4 external_packet_in_compute_vip_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip,
		param_set_p param_set, ip_t *ip, external_port_p epp, external_floating_ip_p fip)
{
	// if src port is external outer ip
		// get the inside ip
		// judge the ip is in the pool
		// port_p = find_nat_connect(packetin_src_ip, ntohs(packetin_dst_port), packetin_proto_type);

	INT4 return_value = internal_packet_compute_vip_forward(src_port, dst_port, targetip, param_set, ip);

	if (Internal_vip_flow == return_value) {
		param_set->src_sw = find_sw_by_dpid(epp->external_dpid);
		param_set->src_ip = ip->src;
		memcpy(param_set->src_mac, ip->eth_head.src, 6);
		param_set->dst_ip = ip->dest;
		memcpy(param_set->dst_mac, ip->eth_head.dest, 6);
		memcpy(param_set->src_gateway_mac, epp->external_gateway_mac, 6);
		memcpy(param_set->outer_gateway_mac, epp->external_gateway_mac, 6);
		return External_vip_flow;
	}
	else {
		return return_value;
	}
}

INT4 internal_packet_compute_floating_vip_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip,
		param_set_p param_set, ip_t *ip, external_port_p epp, external_floating_ip_p fip)
{
	p_fabric_host_node src_gw = find_openstack_app_gateway_by_host(src_port);
	if (NULL == src_gw) {
		return IP_DROP;
	}

	INT4 return_value = internal_packet_compute_vip_forward(src_port, dst_port, targetip, param_set, ip);

	if (Internal_vip_flow == return_value) {
		param_set->src_gateway = src_gw;
		param_set->mod_dst_ip = fip->floating_ip;
		return Internal_floating_vip_flow;
	}
	else {
		return return_value;
	}
}


INT4 internal_packet_compute_forward(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT4 targetip, param_set_p param_set, ip_t *ip)
{
	openstack_subnet_p src_subnet = NULL;
	openstack_subnet_p dst_subnet = NULL;
	p_fabric_host_node src_gateway = NULL;
	p_fabric_host_node dst_gateway = NULL;

	openstack_port_p src_port_p = (openstack_port_p)src_port->data;
	openstack_port_p dst_port_p = (openstack_port_p)dst_port->data;

	if ((NULL == src_port_p) || (NULL == dst_port_p)) 
	{
		return IP_DROP;
	}

	src_subnet = find_openstack_app_subnet_by_subnet_id(src_port_p->subnet_id);

	if (NULL == src_subnet) 
	{
		LOG_PROC("INFO", "Can't get src port subnet!");
		return IP_DROP;
	}

	// save the parameter
	param_set->src_port = src_port;
	param_set->dst_port = dst_port;
	param_set->dst_sw = dst_port->sw;
	param_set->dst_inport = dst_port->port;

	// if in the same subnet
	if (0 == strcmp(dst_port_p->subnet_id, src_port_p->subnet_id)) 
	{
		if(NULL == dst_port->sw) 
		{
			LOG_PROC("INFO", "**************%s %d dst_port sw not exist!!! ip->dest=0x%x ip->src=0x%x",FN,LN, ip->dest, ip->src);
			//flood
			return BROADCAST_DHCP;
		}
		else 
		{
			//setup flow
			// fobidden setup flows if it's gateway & dhcp port
			if( (0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,src_subnet))
					&& (0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,src_subnet))) 
			{
				
				// install flows
				return Internal_port_flow;
			}
			
			// packet out
			return CONTROLLER_FORWARD;
		}
	}
	// in the various subnet
	else 
	{
		openstack_network_p src_network = NULL;
		openstack_network_p dst_network = NULL;
		src_network =  find_openstack_app_network_by_network_id(src_port_p->network_id);
		dst_network =  find_openstack_app_network_by_network_id(dst_port_p->network_id);

		if((NULL == src_network)||(NULL == dst_network))
		{
			LOG_PROC("INFO", "**************%s %d can't find src_network or dst_network!!! ip->dest=0x%x ip->src=0x%x",FN,LN, ip->dest, ip->src);
			return IP_DROP;
		}
		if((0 !=  strcmp(src_port_p->tenant_id, dst_port_p->tenant_id))&&(0 == src_network->shared)&&(0 == dst_network->shared)) 
		{
			LOG_PROC("INFO", "**************%s %d there are different tenant and network not share!!! ip->dest=0x%x ip->src=0x%x",FN,LN, ip->dest, ip->src);
			return IP_DROP;
		}
		src_gateway = find_openstack_app_gateway_by_subnet_id(src_port_p->subnet_id);
		dst_gateway = find_openstack_app_gateway_by_subnet_id(dst_port_p->subnet_id);

		if (OPENSTACK_PORT_TYPE_DHCP != src_port->type)
		{
			if ((NULL == src_gateway) || (NULL == dst_gateway)) 
			{
				 LOG_PROC("ERROR","Src or Dst gateway is NULL!");
				return IP_DROP;
			}
			
			// modify the dst mac
			memcpy(ip->eth_head.dest,dst_port->mac,6);
		}

		if (dst_port->sw == NULL) 
		{
			
			LOG_PROC("INFO", "**************%s %d dst_port sw not exist!!! ip->dest=0x%x ip->src=0x%x",FN,LN, ip->dest, ip->src);
			//flood
			return BROADCAST_DHCP;
		}
		else 
		{
			dst_subnet = find_openstack_app_subnet_by_subnet_id(dst_port_p->subnet_id);
			// fobidden setup flows if it's gateway & dhcp port
			if( 0 == check_fabric_openstack_subnet_dhcp_gateway(src_port,src_subnet) && 0 == check_fabric_openstack_subnet_dhcp_gateway(dst_port,dst_subnet))
			{
				// save gateway info
				param_set->src_gateway = src_gateway;
				param_set->dst_gateway = dst_gateway;
				return Internal_out_subnet_flow;
			}
			//packet out
			return CONTROLLER_FORWARD;
		}
	}
	
	return IP_DROP;
}

INT4 external_packet_out_compute_forward(p_fabric_host_node src_port, UINT4 sendip, UINT4 targetip, packet_in_info_t *packet_in, UINT1 proto, param_set_p param_set)
{
	INT4 foward_type = IP_DROP;
	external_floating_ip_p fip_src = NULL;
	external_floating_ip_p fip_dst = NULL;
	p_fabric_host_node fixed_dst_port = NULL;

	// get floating ip
	fip_src = get_external_floating_ip_by_fixed_ip(sendip);
	fip_dst = get_external_floating_ip_by_floating_ip(targetip);

	// if source port is floating ip
	if(NULL != fip_src)
	{
		foward_type = fabric_openstack_floating_ip_packet_out_handle(src_port, packet_in, fip_src, param_set);
	}
	else if ((fip_dst) && (find_openstack_lbaas_pool_by_ip(fip_dst->fixed_ip))) {
		ip_t* ip = (ip_t*)packet_in->data;
		fixed_dst_port = get_fabric_host_from_list_by_ip(fip_dst->fixed_ip);
		if (fixed_dst_port)
			foward_type = internal_packet_compute_floating_vip_forward(src_port, fixed_dst_port, fip_dst->fixed_ip,
							param_set, ip, NULL, fip_dst);
	}
	else
	{
		if (IPPROTO_ICMP == proto) {
			foward_type = fabric_openstack_nat_icmp_comute_foward(src_port->sw, packet_in,TRUE,param_set);
		}
		else {
			foward_type = fabric_openstack_ip_nat_comute_foward(src_port->sw, packet_in, TRUE, param_set);
		}
	}
	return foward_type;
}

INT4 external_packet_in_compute_forward(p_fabric_host_node src_port, UINT4 src_ip, UINT4 targetip, packet_in_info_t* packet_in, UINT1 proto, param_set_p param_set)
{
	//outer -> inner
	external_floating_ip_p fip = NULL;
	p_fabric_host_node dst_port = NULL;
	INT4 foward_type = IP_DROP;

	fip = get_external_floating_ip_by_floating_ip(targetip);
	external_port_p epp = get_external_port_by_floatip(targetip);

	if ((NULL != fip) && (NULL != epp))
	{        
		if (g_proactive_flow_flag)
	        return IP_DROP;

        gn_switch_t * external_sw = get_ext_sw_by_dpid(epp->external_dpid);
		if (NULL == external_sw) {
			LOG_PROC("INFO", "Floating IP: Can't get external switch");
			return IP_DROP;
		}

		dst_port = get_fabric_host_from_list_by_ip(fip->fixed_ip);
		if (NULL == dst_port) {
			LOG_PROC("INFO", "Floating IP: Fixed ip is not exist");
			return IP_DROP;
		}

		if (NULL != find_openstack_lbaas_pool_by_ip(fip->fixed_ip)) {
			ip_t* ip = (ip_t*)packet_in->data;
			return external_packet_in_compute_vip_forward(src_port, dst_port, fip->fixed_ip, param_set, ip, epp, fip);
		}

		if (NULL == dst_port->sw) {
			LOG_PROC("INFO", "Floating IP: Fixed ip sw is NULL");
			return create_arp_flood_parameter(fip->fixed_ip, dst_port, param_set);
		}

		UINT4 out_port = get_out_port_between_switch(epp->external_dpid, dst_port->sw->dpid);
		if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) {
			param_set->dst_sw = external_sw;
			param_set->src_ip = dst_port->ip_list[0];
			memcpy(param_set->packet_src_mac, dst_port->mac, 6);
			param_set->dst_vlanid = of131_fabric_impl_get_tag_sw(dst_port->sw);
			param_set->dst_inport = out_port;

			param_set->src_sw = dst_port->sw;
			param_set->dst_ip = src_ip;
			memcpy(param_set->src_mac, dst_port->mac, 6);
			param_set->mod_src_ip = fip->floating_ip;
			memcpy(param_set->dst_gateway_mac, epp->external_gateway_mac, 6);
			param_set->src_vlanid = of131_fabric_impl_get_tag_sw(external_sw);
			param_set->src_inport = dst_port->port;

			return Floating_ip_flow;
		}
	}
	else
	{
		if (IPPROTO_ICMP == proto) {
			foward_type = fabric_openstack_nat_icmp_comute_foward(NULL, packet_in, FALSE, param_set);
		}
		else {
			foward_type = fabric_openstack_ip_nat_comute_foward(NULL, packet_in, FALSE, param_set);
		}
	}

	return foward_type;
}
//by:yhy 检查packet_in中ip_t包的源IP或者目的IP与控制器IP是否一致
//by:yhy 检查packet_in包中的IP包是不是由控制器发出的或者是发向控制器的
INT4 openstack_check_src_dst_is_controller(packet_in_info_t *packet_in)
{
	INT4 check_result = GN_ERR;

	if (packet_in) 
	{
		ip_t* ip = (ip_t*)packet_in->data;

		if ((ip) && ((ip->src == ntohl(g_reserve_ip)) || (ip->dest == ntohl(g_reserve_ip)))) 
		{
			check_result = GN_OK;
		}
	}
	return check_result;
}

//by:yhy  
INT4 openstack_ip_packet_check_access(gn_switch_t *sw,p_fabric_host_node src_port, p_fabric_host_node dst_port, packet_in_info_t *packet_in, param_set_p param)
{
	LOG_PROC("HANDLER", "%s -- %d",FN, LN);
	INT4 check_result = GN_ERR;

	check_result = openstack_check_src_dst_is_controller(packet_in);

	if (GN_OK == check_result) 
	{//by:yhy packet_in中ip_t包的源IP或者目的IP与控制器IP一致
		return SECURITY_CHECK_IPPACKET_NOT_CAMEFROM_SECURITY;
	}
	
	LOG_PROC("HANDLER", "%s -- %d",FN, LN);
	if((packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)||(packet_in->In_TableID == FABRIC_FIREWALL_OUT_TABLE))
	{//两个防火墙表packet_in消息
		ip_t *	IP_Header  	=NULL;
		tcp_t* 	TCP_Header 	=NULL;
		udp_t* 	UDP_Header 	=NULL;
		icmp_t* ICMP_Header	=NULL;
		char*	Direction 	=NULL;
		UINT4 	SrcIP 		=0;
		UINT4 	DstIP 		=0;
		UINT2 	SrcPort 	=0;
		UINT2 	DstPort 	=0;
		char*	Protocol 	=NULL;
		UINT1   FirewallCheckResult =0;
		
		//初始报文防火墙方向
		if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
		{	
			Direction =FIREWALL_DIRECTION_IN;
		}
		else
		{
			Direction =FIREWALL_DIRECTION_OUT;
		}
		
		IP_Header =(ip_t *)(packet_in->data);
		if(IP_Header)
		{//IP报文
			SrcIP =IP_Header->src;
			DstIP =IP_Header->dest;
			if(IP_PROTOCOL_ICMP == IP_Header->proto)
			{//icmp协议
				ICMP_Header =(icmp_t*)(IP_Header->data);
				SrcPort =ntohs(ICMP_Header->type);
				DstPort =ntohs(ICMP_Header->code);
				//LOG_PROC("INFO", "%s_%d_%d_%d",FN,LN,SrcPort,DstPort);
				Protocol="icmp";
				FirewallCheckResult = fabric_firewall_CheckFirewallAccessByDataPacketInfo(Direction,SrcIP,DstIP,SrcPort,DstPort,Protocol);
				if(FIREWALL_ACCESS_INNERIP_NOINCLUDE ==FirewallCheckResult)
				{
					LOG_PROC("INFO", "%s_%d",FN,LN);
					//下通过流表
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_ICMP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_ICMP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_THROUGH;
				}
				else if(FIREWALL_ACCESS_THROUGH ==FirewallCheckResult)
				{
					//下通过流表
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_ICMP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_ICMP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_THROUGH;
				}
				else if(FIREWALL_ACCESS_DENY ==FirewallCheckResult)
				{
					//下阻断流表
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificDeny_flow(sw, SrcIP, DstIP, IP_PROTOCOL_ICMP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificDeny_flow(sw, SrcIP, DstIP, IP_PROTOCOL_ICMP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_DENY;
				}	
			}
			else if(IP_PROTOCOL_TCP == IP_Header->proto)
			{//tcp协议
				TCP_Header =(tcp_t*)(IP_Header->data);
				SrcPort =ntohs(TCP_Header->sport);
				DstPort =ntohs(TCP_Header->dport);
				//LOG_PROC("INFO", "%s_%d_%d_%d",FN,LN,TCP_Header->sport,TCP_Header->dport);
				//LOG_PROC("INFO", "%s_%d_%d_%d",FN,LN,SrcPort,DstPort);
				Protocol="tcp";
				FirewallCheckResult = fabric_firewall_CheckFirewallAccessByDataPacketInfo(Direction,SrcIP,DstIP,SrcPort,DstPort,Protocol);
				if(FIREWALL_ACCESS_INNERIP_NOINCLUDE ==FirewallCheckResult)
				{
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_TCP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_TCP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_THROUGH;
				}
				else if(FIREWALL_ACCESS_THROUGH ==FirewallCheckResult)
				{
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_TCP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_TCP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_THROUGH;
				}
				else if(FIREWALL_ACCESS_DENY ==FirewallCheckResult)
				{
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificDeny_flow(sw, SrcIP, DstIP, IP_PROTOCOL_TCP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificDeny_flow(sw, SrcIP, DstIP, IP_PROTOCOL_TCP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_DENY;
				}	
				LOG_PROC("INFO", "%s_%d",FN,LN);
			}
			else if(IP_PROTOCOL_UDP == IP_Header->proto)
			{//udp协议
				UDP_Header =(udp_t*)(IP_Header->data);
				SrcPort =ntohs(UDP_Header->sport);
				DstPort =ntohs(UDP_Header->dport);
				LOG_PROC("INFO", "%s_%d_%d_%d",FN,LN,SrcPort,DstPort);
				Protocol="udp";
				FirewallCheckResult = fabric_firewall_CheckFirewallAccessByDataPacketInfo(Direction,SrcIP,DstIP,SrcPort,DstPort,Protocol);
				if(FIREWALL_ACCESS_INNERIP_NOINCLUDE ==FirewallCheckResult)
				{
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_UDP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_UDP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_THROUGH;
				}
				else if(FIREWALL_ACCESS_THROUGH ==FirewallCheckResult)
				{
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_UDP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificThrough_flow(sw, SrcIP, DstIP, IP_PROTOCOL_UDP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_THROUGH;
				}
				else if(FIREWALL_ACCESS_DENY ==FirewallCheckResult)
				{
					if(packet_in->In_TableID == FABRIC_FIREWALL_IN_TABLE)
					{
						install_add_FirewallIn_spicificDeny_flow(sw, SrcIP, DstIP, IP_PROTOCOL_UDP, SrcPort, DstPort);
					}
					else
					{
						install_add_FirewallOut_spicificDeny_flow(sw, SrcIP, DstIP, IP_PROTOCOL_UDP, SrcPort, DstPort);
					}
					return SECURITY_CHECK_IPPACKET_CAMEFROM_SECURITY_DENY;
				}
			}
			else
			{//其他协议
				return SECURITY_CHECK_IPPACKET_INVALID;
			}
		}
		else
		{
			return SECURITY_CHECK_IPPACKET_INVALID;
		}
	}
	else
	{
		return SECURITY_CHECK_IPPACKET_NOT_CAMEFROM_SECURITY;
	}
	//check_result = openstack_security_group_main_check(src_port, dst_port, packet_in, param->src_security, param->dst_security);
	LOG_PROC("HANDLER", "%s -- %d",FN,LN);
	return SECURITY_CHECK_IPPACKET_INVALID;
}

//by:yhy 根据交换机的DPID=sw_dpid,找到其port口上所接的host主机,删除与该主机相关的所有流表
void remove_flows_by_sw_port(UINT8 sw_dpid, UINT4 port)
{
	if (0 == g_openstack_on)
	{
		return ;
	}
	
	LOG_PROC("INFO", "------------------%s %d sw_dpid=0x%x port=%d",FN,LN,sw_dpid,port);
	p_fabric_host_node host = get_fabric_host_from_list_by_sw_port(sw_dpid, port);

	if (NULL != host) 
	{
		gn_switch_t* sw = host->sw;
		
		LOG_PROC("INFO", "##########------%s %d host->ip_list[0]=0x%x sw->sw_ip=0x%x port=%d",FN,LN,host->ip_list[0],sw->sw_ip, port);
		host->sw = NULL;
		host->port = 0;

		external_floating_ip_p fip = NULL;
		fip = get_external_floating_ip_by_fixed_ip(host->ip_list[0]);
		if (NULL != fip) 
		{
			LOG_PROC("INFO", "------------------%s %d host->ip_list[0]=0x%x fip->floating_ip=0x%x",FN,LN,host->ip_list[0],fip->floating_ip );
			remove_floating_flow(sw, fip->floating_ip, host->mac);
			//删除compute节点上的FABRIC_OUTPUT_TABLE上浮动IP到虚机的流表
			delete_fabric_flow_by_ip(sw, fip->floating_ip, FABRIC_OUTPUT_TABLE);
			//删除compute节点上出口QOS上的浮动IP的流表
			install_remove_QOS_jump_flow(sw,FABRIC_QOS_OUT_TABLE,fip->floating_ip);
			//删除compute节点上的浮动IP的入口防火墙流表
			install_remove_FirewallIn_flow(sw,fip->floating_ip);
			//删除compute节点上的浮动IP的出口防火墙流表
			install_remove_FirewallOut_flow(sw,fip->floating_ip);
			
			//pica8上防火墙及qos入出流表
			external_port_p S_TargetFloatingExternalPort =find_openstack_external_by_floating_ip(fip->floating_ip);
			if(S_TargetFloatingExternalPort)
			{
				gn_switch_t * S_TargetFloatingExternalSwitch =find_sw_by_dpid(S_TargetFloatingExternalPort->external_dpid);
				if(S_TargetFloatingExternalSwitch)
				{
					//pica8上浮动IP的入口防火墙流表
					install_remove_FirewallIn_flow(S_TargetFloatingExternalSwitch,fip->floating_ip);
					//pica8上浮动IP的出口防火墙流表
					install_remove_FirewallOut_flow(S_TargetFloatingExternalSwitch,fip->floating_ip);
					//pica8上出口QOS上的浮动IP的流表
					install_remove_QOS_jump_flow(S_TargetFloatingExternalSwitch,FABRIC_QOS_OUT_TABLE,fip->floating_ip);
				}
			}
			fip->flow_installed = 0;
		}
		else 
		{
			remove_nat_flow(sw, host->ip_list[0], host->mac);
		}

		if (NULL != sw) 
		{
			remove_host_output_flow_by_ip_mac(sw, host->ip_list[0], host->mac);
			
			install_ip_controller_flow(sw, host->ip_list[0]);
		}

#if 0
		p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(host);
		if (NULL != gateway_p) 
		{
			fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], host->ip_list[0], gateway_p->mac);
		}
#endif
	}
}

//by:yhy 根据sw,floating_ip,删除相关的流表(删除Pica8上的浮动IP有关流表)
void remove_floating_flow(gn_switch_t* sw, UINT4 floating_ip, UINT1* mac)
{
	external_port_p epp = get_external_port_by_floatip(floating_ip);
	if (NULL != epp) 
	{
	  gn_switch_t* sww = get_ext_sw_by_dpid(epp->external_dpid);
	  if (NULL != sww) 
	  {
		  delete_fabric_input_flow_by_ip(sww, floating_ip);
		  // delete_fabric_flow_by_mac(sw, mac, FABRIC_OUTPUT_TABLE);
	  }
	}
}
//by:yhy 
void remove_nat_flow(gn_switch_t* sw, UINT4 ip, UINT1* src_mac)
{
	UINT2 port_list[100];
	UINT4 externalip_list[100];
	UINT2 proto_list[100];
	UINT4 port_number;
	port_number = get_nat_connect_count_by_ip(ip, port_list, externalip_list, proto_list);
	external_port_p epp = get_external_port_by_host_mac(src_mac);
	if (NULL != epp) 
	{
		gn_switch_t* sww = get_ext_sw_by_dpid(epp->external_dpid);
		while (port_number)
		{
//			printf("port number:%d\n", port_list[port_number]);
//			printf("proto is %d", proto_list[port_number]);
//			nat_show_mac(epp->external_outer_interface_mac);
			delete_fabric_input_flow_by_mac_portno(sww, epp->external_outer_interface_mac, port_list[port_number], proto_list[port_number]);
//			delete_fabric_output_flow_by_ip_portno(sw, externalip_list[port_number], port_list[port_number], proto_list[port_number]);
//			delete_fabric_flow_by_ip(sw, externalip_list[port_number], FABRIC_PUSHTAG_TABLE);
			port_number--;
		}
	}
}
//by:yhy 删除table=3中的output流表
void remove_host_output_flow_by_ip_mac(gn_switch_t* sw, UINT4 ip, UINT1* mac)
{
	LOG_PROC("INFO", "------------------%s ip=0x%x",FN,ip);
	if (0 != ip) 
	{
		delete_fabric_flow_by_ip(sw, ip, FABRIC_OUTPUT_TABLE);
		//删除虚机内部IP的入口防火墙流表
		//install_remove_FirewallIn_flow(sw,ip);
		//删除虚机内部IP的出口防火墙流表
		//install_remove_FirewallOut_flow(sw,ip);
	}

	if (NULL != mac) 
	{
		
		delete_fabric_flow_by_mac(sw, mac, FABRIC_INPUT_TABLE);
		delete_fabric_flow_by_mac(sw, mac, FABRIC_FQ_OUT_POST_PROCESS_TABLE);
		delete_fabric_flow_by_mac(sw, mac, FABRIC_OUTPUT_TABLE);
	}
}

INT4 create_arp_flood_parameter(UINT4 dst_ip, p_fabric_host_node dst_port, param_set_p param)
{
	if (NULL != dst_port) {
		p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(dst_port);

		if (NULL != gateway_p) {
			// fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], fip->fixed_ip, gateway_p->mac, epp->external_dpid);
			param->src_ip = gateway_p->ip_list[0];
			param->dst_ip = dst_ip;
			memcpy(param->src_mac, gateway_p->mac, 6);
			return IP_FLOOD;
		}
	}

	return IP_DROP;
}
//by:yhy 根据给定参数构建ARP_Request包并通过ofpt_packet_out发送
void fabric_opnestack_create_arp_request(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, gn_switch_t* sw, UINT4 outPort)
{

	packet_in_info_t packout_req_info;
	arp_t* new_arp_pkt = (arp_t*)gn_malloc(sizeof(arp_t));

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.xid = 0;
	packout_req_info.data_len = sizeof(arp_t);
	packout_req_info.data = (UINT1 *)new_arp_pkt;

	memcpy(new_arp_pkt->eth_head.src, src_mac, 6);
	memcpy(new_arp_pkt->eth_head.dest, arp_broadcat_mac, 6);
	new_arp_pkt->eth_head.proto = htons(ETHER_ARP);
	new_arp_pkt->hardwaretype = htons(1);
	new_arp_pkt->prototype = htons(ETHER_IP);
	new_arp_pkt->hardwaresize = 0x6;
	new_arp_pkt->protocolsize = 0x4;
	new_arp_pkt->opcode = htons(1);
	new_arp_pkt->sendip = src_ip;
	new_arp_pkt->targetip=dst_ip;

	memcpy(new_arp_pkt->sendmac, src_mac, 6);
	memcpy(new_arp_pkt->targetmac, arp_zero_mac, 6);

    fabric_openstack_packet_output(sw, &packout_req_info, outPort);
	if(NULL != new_arp_pkt)
	{
		gn_free(&new_arp_pkt);
	}
};

//by:yhy 根据配置文件设置确定是否装载deny_flow
//by:yhy 仅在配置中security_drop_on为1时有效
INT4 openstack_ip_install_deny_flow(gn_switch_t* sw, ip_t* ip)
{
	INT4 return_value = GN_OK;
	INT1* value = get_value(g_controller_configure, "[openvstack_conf]", "security_drop_on");
	INT4 flag_security_drop_on = (NULL == value) ? 0: atoi(value);
	
	LOG_PROC("HANDLER", "%s  flag_security_drop_on=%d",FN, flag_security_drop_on);
	if (flag_security_drop_on)
	{
		return_value = fabric_ip_install_deny_flow(sw, ip);
	}
	return return_value;
}
//by:yhy 根据src_mac删除对应的deny_flow
INT4 openstack_ip_remove_deny_flow(UINT1* src_mac,UINT4 ip)
{
	INT4 return_value = GN_OK;
	INT1* value = get_value(g_controller_configure, "[openvstack_conf]", "security_drop_on");
	INT4 flag_security_drop_on = (NULL == value) ? 0: atoi(value);
	
	if (flag_security_drop_on) 
	{
		//LOG_PROC("INFO","%s-MAC:[%3d:%3d:%3d:%3d:%3d:%3d]:",FN,src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);
		return_value = fabric_ip_remove_deny_flow(src_mac,ip);
	}
	return return_value;
}

p_fabric_host_node openstack_save_host_info_ipv6(gn_switch_t *sw,UINT1* sendmac,UINT1* sendip,UINT4 inport)
{
	p_fabric_host_node p_node =  get_fabric_host_from_list_by_mac(sendmac);
	if(p_node!=NULL){
	}else{
		return NULL;
	}

	if ((NULL != p_node->sw) && (0 != p_node->port))
		return p_node;

	if (sendip)
		memcpy(p_node->ipv6[0], sendip, 16);
	p_node->port = inport;
	p_node->sw=sw;
	return p_node;
}

/*
 * temp added for ipv6
 * by lxf@2016.1.11
 */
#if 1
p_fabric_host_node openstack_find_ip_dst_port_ipv6(p_fabric_host_node src_node,UINT1* targetip)
{
	p_fabric_host_node dst_port=NULL;
	// find dst port
	dst_port = get_fabric_host_from_list_by_ipv6(targetip);

	return dst_port;
}

void fabric_openstack_install_fabric_flows_ipv6(p_fabric_host_node src_port,p_fabric_host_node dst_port,
										   security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	// display port info
//	LOG_PROC("INFO","Sourt Port Info:");
//	fabric_openstack_show_port(src_port);
//	LOG_PROC("INFO","Destination Port Info:");
//	fabric_openstack_show_port(dst_port);
	if ((NULL== src_port) || (NULL == dst_port) || (NULL == src_port->sw) || (NULL == dst_port->sw))
		return ;

	src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);
	if(src_port->sw == dst_port->sw){
		// printf("same switch\n");
		install_fabric_same_switch_security_before_OutFirewallQOS_flow(src_port->sw,src_port->mac,src_port->port, src_security);
		install_fabric_same_switch_security_before_OutFirewallQOS_flow(dst_port->sw,dst_port->mac,dst_port->port, dst_security);
		install_fabric_same_switch_security_flow(src_port->sw,src_port->mac,src_port->port, src_security);
		install_fabric_same_switch_security_flow(dst_port->sw,dst_port->mac,dst_port->port, dst_security);
	}else{
		// printf("different switch\n");
		install_fabric_push_tag_security_flow_ipv6(src_port->sw,dst_port->ipv6[0], dst_port->mac,dst_tag, src_security);
		install_fabric_push_tag_security_flow_ipv6(dst_port->sw,src_port->ipv6[0], src_port->mac,src_tag, src_security);
		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}

	return;
};
#endif

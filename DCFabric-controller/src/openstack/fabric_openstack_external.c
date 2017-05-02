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
 * fabric_openstack_external.c
 *
 *  Created on: sep 9, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: sep 9, 2015
 */
#include "gnflush-types.h"
#include "fabric_openstack_external.h"
#include "mem_pool.h"
#include "../restful-svr/openstack-server.h"
#include "../fabric/fabric_flows.h"
#include "openstack_host.h"
#include "../fabric/fabric_host.h"
#include "timer.h"
#include "fabric_openstack_arp.h"
#include "../cluster-mgr/redis_sync.h"
#include "openflow-common.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "openstack_app.h"
#include "fabric_floating_ip.h"

//by:yhy 这些全局变量具体用途存疑
void *g_openstack_external_id = NULL;
void *g_openstack_floating_id = NULL;
void *g_nat_icmp_iden_id=NULL;
void *g_openstack_external_node_id = NULL;
//by:yhy openstack外部网关列表
openstack_external_node_p g_openstack_external_list = NULL;
//by:yhy openstack浮动IP列表
openstack_external_node_p g_openstack_floating_list = NULL;
openstack_external_node_p g_nat_icmp_iden_list=NULL;
//by:yhy openstack的external是否已经初始化的标志
UINT1 g_openstack_external_init_flag = 0;
//by:yhy 这些全局变量具体用途<费解>
static const UINT4 external_min_seq = 1;
static const UINT4 external_max_seq = 20;

UINT4 g_external_check_on = 0;
UINT4 g_external_check_interval = 20;
void *g_external_check_timerid = NULL;
void *g_external_check_timer = NULL;

UINT4 g_external_install_flow_interval = 20;
void *g_external_install_flow_timerid = NULL;
void *g_external_install_flow_timer = NULL;


extern UINT4 g_openstack_on;
UINT1 ext_zero_mac[6] = {0};
extern UINT1 g_fabric_start_flag;

void install_external_flow(external_port_p epp);

external_port_p create_external_port(
        UINT4 external_gateway_ip,
		UINT1* external_gateway_mac,
        UINT4 external_outer_interface_ip,
		UINT1* external_outer_interface_mac,
		UINT8 external_dpid,
		UINT4 external_port,
		char* network_id);
external_floating_ip_p create_floating_ip_p(
		UINT4 fixed_ip,
		UINT4 floating_ip,
		char* port_id,
		char* router_id);
nat_icmp_iden_p create_nat_icmp_iden(
		UINT2 identifier,
		UINT4 host_ip,
		UINT1* host_mac,
		UINT8 sw_dpid,
		UINT4 inport);
void add_openstack_external_to_list(external_port_p epp);
void add_openstack_floating_to_list(external_floating_ip_p efp);
void add_nat_icmp_iden_to_list(nat_icmp_iden_p nii);
openstack_external_node_p create_openstack_external_node(UINT1* data);
void update_external_config(external_port_p epp);
external_port_p update_external_port(
        UINT4 external_gateway_ip,
		UINT1* external_gateway_mac,
        UINT4 external_outer_interface_ip,
		UINT1* external_outer_interface_mac,
		UINT8 external_dpid,
		UINT4 external_port,
		char* network_id);
external_floating_ip_p update_floating_ip_list(
		UINT4 fixed_ip,
		UINT4 floating_ip,
		char* port_id,
		char* router_id);
nat_icmp_iden_p update_nat_icmp_iden(
		UINT2 identifier,
		UINT4 host_ip,
		UINT1* host_mac,
		UINT8 sw_dpid,
		UINT4 inport);
void destory_openstack_external();
external_port_p find_openstack_external_by_outer_ip(UINT4 external_outer_interface_ip);

external_port_p find_openstack_external_by_gatway_ip(UINT4 external_gateway_ip);
external_port_p find_openstack_external_by_floating_ip(UINT4 external_floating_ip);
external_port_p find_openstack_external_by_network_id(char* network_id);

external_port_p find_openstack_external_by_outer_mac(UINT1* external_gateway_mac);
nat_icmp_iden_p find_nat_icmp_iden_by_host_ip(UINT4 host_ip);
nat_icmp_iden_p find_nat_icmp_iden_by_host_mac(UINT1* host_mac);
nat_icmp_iden_p find_nat_icmp_iden_by_identifier(UINT2 identifier);
external_floating_ip_p find_external_floating_ip_by_fixed_ip(UINT4 fixed_ip);
void get_sw_from_dpid(UINT8 dpid,gn_switch_t **sw);
void test(UINT1 type);

//by:yhy 初始化openstack_external用到的全局变量
void init_openstack_external()
{
	if(g_openstack_external_id != NULL){
		mem_destroy(g_openstack_external_id);
	}
	g_openstack_external_id = mem_create(sizeof(external_port_t), OPENSTACK_EXTERNAL_MAX_NUM);

	if(g_openstack_floating_id != NULL){
		mem_destroy(g_openstack_floating_id);
	}
	g_openstack_floating_id = mem_create(sizeof(external_floating_ip), OPENSTACK_FLOATING_MAX_NUM);

	if(g_nat_icmp_iden_id != NULL){
		mem_destroy(g_nat_icmp_iden_id);
	}
	g_nat_icmp_iden_id = mem_create(sizeof(nat_icmp_iden), OPENSTACK_NAT_ICMP_MAX_NUM);

	if(g_openstack_external_node_id != NULL){
		mem_destroy(g_openstack_external_node_id);
	}
	g_openstack_external_node_id = mem_create(sizeof(openstack_external_node), OPENSTACK_EXTERNAL_NODE_MAX_NUM);
	g_openstack_external_list=NULL;
	g_openstack_floating_list=NULL;
	g_nat_icmp_iden_list=NULL;
	return;
}
//by:yhy 通过对外接口IP查找外部网关端口结构体
external_port_p get_external_port_by_out_interface_ip(UINT4 external_outer_interface_ip) 
{
    external_port_p epp = find_openstack_external_by_outer_ip(external_outer_interface_ip);
	if (epp && (GN_OK == epp->status)) 
	{
		return epp;
	}
	return NULL;
}
//by:yhy 获取浮动Ip对应的外联口
external_port_p get_external_port_by_floatip(UINT4 external_floatip) 
{
	external_port_p epp = find_openstack_external_by_floating_ip(external_floatip);
	if (epp && (GN_OK == epp->status)) 
	{
		return epp;
	}
	return NULL;
}
//by:yhy 根据fixed_ip在g_openstack_floating_list查找对应的external_floating_ip_p
external_floating_ip_p get_external_floating_ip_by_fixed_ip(UINT4 fixed_ip)
{
	return find_external_floating_ip_by_fixed_ip(fixed_ip);
}
//by:yhy 通过输入IP查找是否是对外floating ip,是则返回external_floating_ip_p
external_floating_ip_p get_external_floating_ip_by_floating_ip(UINT4 floating_ip)
{
	return find_external_floating_ip_by_floating_ip(floating_ip);
}

nat_icmp_iden_p get_nat_icmp_iden_by_host_ip(UINT4 host_ip){
	return find_nat_icmp_iden_by_host_ip(host_ip);
}
nat_icmp_iden_p get_nat_icmp_iden_by_host_mac(UINT1* host_mac){
	return find_nat_icmp_iden_by_host_mac(host_mac);
}
nat_icmp_iden_p get_nat_icmp_iden_by_identifier(UINT2 identifier){
	return find_nat_icmp_iden_by_identifier(identifier);
}
//by:yhy 装载对外网关流表
void install_external_flow(external_port_p epp)
{
	if (g_controller_role == OFPCR_ROLE_SLAVE)
	{
		return;
	}

	gn_switch_t *sw = NULL;
	get_sw_from_dpid(epp->external_dpid, &sw);

	if (0 != g_fabric_start_flag)
	{
		if (sw)  
		{
			//by:yhy 装载外部网关MAC流表
			install_fabric_openstack_external_output_flow(sw,epp->external_port,epp->external_gateway_mac,epp->external_outer_interface_ip,1);
			//by:yhy 装载外部接口IP流表
			install_fabric_openstack_external_output_flow(sw,epp->external_port,epp->external_gateway_mac,epp->external_outer_interface_ip,2);
		}
		else 
		{
			INT1 str_dpid[48] = {0};
			dpidUint8ToStr(epp->external_dpid, str_dpid);
			LOG_PROC("ERROR", "can not find any sw match external_dpid:%s!\n", str_dpid);
		}
	}
}

/* by:yhy(待用)
 * 判断sw是否为外联交换机,若是则下发外联用流表
 */
void initiative_check_if_external_and_install_flow(gn_switch_t* sw)
{
	if (0 == g_openstack_on) 
	{
		return;
	}
	if (g_controller_role == OFPCR_ROLE_SLAVE)
	{
		return;
	}

	external_port_p epp = NULL;
	openstack_external_node_p node_p  = g_openstack_external_list;

	while (NULL != node_p)
	{
		epp = (external_port_p)node_p->data;
		if ((epp) && (GN_OK == epp->status)&&(epp->external_dpid == sw->dpid)) 
		{
			if (0 != g_fabric_start_flag)
			{
				if (sw)  
				{
					//by:yhy 装载外部网关MAC流表
					install_fabric_openstack_external_output_flow(sw,epp->external_port,epp->external_gateway_mac,epp->external_outer_interface_ip,1);
					//by:yhy 装载外部接口IP流表
					install_fabric_openstack_external_output_flow(sw,epp->external_port,epp->external_gateway_mac,epp->external_outer_interface_ip,2);
				}
				else 
				{
					INT1 str_dpid[48] = {0};
					dpidUint8ToStr(epp->external_dpid, str_dpid);
					LOG_PROC("ERROR", "can not find any sw match external_dpid:%s!\n", str_dpid);
				}
			}
		}
		node_p=node_p->next;
	}
}



//by:yhy   通过restAPI创建对外网关
void create_external_port_by_rest(
		UINT4 external_gateway_ip,
		UINT1* external_gateway_mac,
        UINT4 external_outer_interface_ip,
		UINT1* external_outer_interface_mac,
		UINT8 external_dpid,
		UINT4 external_port,
		char* network_id)
{
	external_port_p epp = update_external_port(
	        external_gateway_ip,
			external_gateway_mac,
	        external_outer_interface_ip,
			external_outer_interface_mac,
	        external_dpid,
	        external_port,
			network_id);
	
	if ((epp) && (GN_OK == epp->status))
	{
		//装载流表
	    install_external_flow(epp);
		//更新配置文件
		update_external_config(epp);
	}
}
//by:yhy 根据network_id匹配并删除g_openstack_external_list中的项
void remove_external_port_in_list_by_networkid(INT1* network_id)
{
	// remove external port by networkid
	openstack_external_node_p head_p = g_openstack_external_list;
	openstack_external_node_p prev_p = head_p;
	external_port_p external_p = NULL;

	if (NULL == head_p)
	{
		return ;
	}

	openstack_external_node_p next_p = prev_p->next;

	while (next_p) 
	{
		//by:yhy 遍历并删除
		external_p = (external_port_p)next_p->data;
		if ((external_p) && (0 == strcmp(external_p->network_id, network_id))) 
		{
			prev_p->next = next_p->next;
			mem_free(g_openstack_external_id, external_p);
			mem_free(g_openstack_external_node_id, next_p);
		}
		else 
		{
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	external_p = (external_port_p)head_p->data;
	if ((external_p) && (0 == strcmp(external_p->network_id, network_id))) 
	{
		//by:yhy 判断是否是头结点需要删除
		next_p = head_p->next;
		mem_free(g_openstack_external_id, head_p->data);
		mem_free(g_openstack_external_node_id, head_p);
		g_openstack_external_list = next_p;
	}	
}	
//by:yhy 根据network_id匹配并删除external_port
void remove_external_port_by_networkid(INT1* network_id)
{
	if (g_controller_role == OFPCR_ROLE_SLAVE)
	{
		return;
	}

	external_port_p external_p = find_openstack_external_by_network_id(network_id);

	if (external_p) 
	{
		gn_switch_t *sw = NULL;
		get_sw_from_dpid(external_p->external_dpid,&sw);
		//by:yhy 删除流表项
		if (sw) 
		{
			remove_fabric_openstack_external_output_flow(sw, external_p->external_gateway_mac, external_p->external_outer_interface_ip, 1);
			remove_fabric_openstack_external_output_flow(sw, external_p->external_gateway_mac, external_p->external_outer_interface_ip, 2);
		}

		//by:yhy 删除配置文件中的项
		INT1* selection = get_selection_by_name_value(g_controller_configure, "external_network_id", network_id);
		if (selection) 
		{
			INT4 return_value = remove_selection(g_controller_configure, selection);
			if (return_value)
			{
				g_controller_configure = save_ini(g_controller_configure,CONFIGURE_FILE);
			}
		}
		//by:yhy 删除g_openstack_external_list中的项
		remove_external_port_in_list_by_networkid(network_id);
	}
}
//by:yhy 初始化openstack的external功能(对Pica8下发流表)
void init_external_flows()
{
	INT1 *value = NULL;
	if (1 == g_openstack_on) 
	{
		value = get_value(g_controller_configure, "[openvstack_conf]", "external_check_internal");
		g_external_install_flow_interval = (NULL == value) ? 20: atoi(value);
		
		TimerFunction_init_external_flows();

		g_external_install_flow_timerid = timer_init(1);
		timer_creat(g_external_install_flow_timerid, 
					g_external_install_flow_interval, 
					NULL, 
					&g_external_install_flow_timer, 
					TimerFunction_init_external_flows);
	}
}
void TimerFunction_init_external_flows(void)
{
	if (0 == g_openstack_on) 
	{
		return;
	}
	LOG_PROC("INFO", "External:external flow installed!");
	//if (0 != g_openstack_external_init_flag) 
	//{
	//	LOG_PROC("INFO", "External:external flow has been initialized!");
	//	return;
	//}

	external_port_p epp = NULL;
	openstack_external_node_p node_p  = g_openstack_external_list;

	while (NULL != node_p)
	{
		epp = (external_port_p)node_p->data;
		if ((epp) && (GN_OK == epp->status)) 
		{
			install_external_flow(epp);
		}
		node_p=node_p->next;
	}
	//g_openstack_external_init_flag = 1;
}

//by:yhy 更新 对外网关 配置(刷新配置文件)
void update_external_config(external_port_p epp)
{

	UINT1 seq_num = external_min_seq;
	char para_title[48] = {0};
	char dpid_str[48] = {0};
	char ip_str[48] = {0};
	INT1 *value = NULL;

	number2ip(epp->external_outer_interface_ip, ip_str);
	INT1* selection = get_selection_by_name_value(g_controller_configure, "external_outer_interface_ip", ip_str);
	if (selection) 
	{
		remove_selection(g_controller_configure, selection);
	}

	for (; seq_num <= external_max_seq; seq_num++) 
	{
		sprintf(para_title, "[external_switch_%d]", seq_num);
		value = get_value(g_controller_configure, para_title, "external_gateway_ip");
		if (NULL == value)
		{
			break;
		}
	}

	if (seq_num > external_max_seq) 
	{
		LOG_PROC("INFO", "External: The external switch is max!");
		return;
	}

	dpidUint8ToStr(epp->external_dpid, dpid_str);
	
	// config
	set_value_ip(g_controller_configure, para_title, "external_gateway_ip", epp->external_gateway_ip);
	set_value_mac(g_controller_configure, para_title, "external_gateway_mac", epp->external_gateway_mac);
	set_value_ip(g_controller_configure, para_title, "external_outer_interface_ip", epp->external_outer_interface_ip);
	set_value(g_controller_configure, para_title, "external_dpid",dpid_str);
	set_value_int(g_controller_configure, para_title, "external_port", epp->external_port);
	g_controller_configure = save_ini(g_controller_configure,CONFIGURE_FILE);
}

external_floating_ip_p create_floatting_ip_by_rest(
		UINT4 fixed_ip,
        UINT4 floating_ip,
		char* port_id,
        char* router_id)
{
	external_floating_ip_p efp = update_floating_ip_list(
			fixed_ip,
			floating_ip,
			port_id,
			router_id);
	return efp;
}

nat_icmp_iden_p create_nat_imcp_iden_p(
		UINT2 identifier,
		UINT4 host_ip,
		UINT1* host_mac,
		UINT8 sw_dpid,
		UINT4 inport){
	return update_nat_icmp_iden(
			identifier,
			host_ip,
			host_mac,
			sw_dpid,
			inport);
}
//by:yhy 根据参数新建并初始化一个openstack的对外网关,并将其添加到g_openstack_external_list中
external_port_p create_external_port(
        UINT4 external_gateway_ip,
		UINT1* external_gateway_mac,
        UINT4 external_outer_interface_ip,
		UINT1* external_outer_interface_mac,
		UINT8 external_dpid,
		UINT4 external_port,
		char* network_id){
    external_port_p epp = NULL;
    epp = (external_port_p)mem_get(g_openstack_external_id);
	
	//addedby:yhy
	if(NULL == epp)
	{
		LOG_PROC("ERROR", "%s -- mem_get(g_openstack_external_id) Fail",FN);
		return NULL;
	}
	
    memset(epp,0,sizeof(external_port_t));
    if(external_gateway_ip && external_gateway_ip!=0){
    	epp->external_gateway_ip=external_gateway_ip;
    }
    if(external_outer_interface_ip && external_outer_interface_ip!=0){
    	epp->external_outer_interface_ip=external_outer_interface_ip;
    }
    if ((external_gateway_mac) && (0 != memcmp(ext_zero_mac, external_gateway_mac, 6))) {
    	memcpy(epp->external_gateway_mac, external_gateway_mac, 6);
    }
    if ((external_outer_interface_mac) && (0 != memcmp(ext_zero_mac, external_outer_interface_mac, 6))) {
    	memcpy(epp->external_outer_interface_mac, external_outer_interface_mac, 6);
    }
    if(external_dpid && external_dpid!=0){
    	epp->external_dpid=external_dpid;
    }
    if(external_port && external_port!=0){
    	epp->external_port=external_port;
    }
    if(network_id){
		strcpy(epp->network_id,network_id);
	}
    add_openstack_external_to_list(epp);
    test(1);
    return epp;
}

openstack_external_node_p remove_tail_nat_icmp_iden()
{
	openstack_external_node_p prev_p = g_nat_icmp_iden_list;
	if (NULL == prev_p) {
		return NULL;
	}

	openstack_external_node_p next_p = prev_p->next;
	if (NULL == next_p) {
		return NULL;
	}
	
	while (next_p->next) {
		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	// remove nat ident
	nat_icmp_iden_p nii = (nat_icmp_iden_p)next_p->data;
	if (nii) {
		mem_free(g_nat_icmp_iden_id, nii);
	}

	mem_free(g_openstack_external_node_id, next_p);
	prev_p->next = NULL;

	return prev_p;
}

nat_icmp_iden_p create_nat_icmp_iden(
		UINT2 identifier,
		UINT4 host_ip,
		UINT1* host_mac,
		UINT8 sw_dpid,
		UINT4 inport
		){
	nat_icmp_iden_p nii = NULL;
	nii = (nat_icmp_iden_p)mem_get(g_nat_icmp_iden_id);
	// if no memory
	if (NULL == nii) {
		if (NULL != remove_tail_nat_icmp_iden()) {
			nii = (nat_icmp_iden_p)mem_get(g_nat_icmp_iden_id);
		}
	}
	
	if (nii) {
		memset(nii,0,sizeof(nat_icmp_iden));
		if(identifier){
			nii->identifier=identifier;
		}
		if(host_ip){
			nii->host_ip=host_ip;
		}
		if(host_mac){
			memcpy(nii->host_mac, host_mac, 6);
		}
		if (sw_dpid) {
			nii->sw_dpid = sw_dpid;
		}
		if (inport) {
			nii->inport = inport;
		}
		add_nat_icmp_iden_to_list(nii);
		// test(3);
	}
	else {
		LOG_PROC("INFO", "NAT Icmp: Can't get memory.");
	}
	
	return nii;
}
//by:yhy 将epp添加到g_openstack_external_list中
void add_openstack_external_to_list(external_port_p epp)
{
	external_port_p epp_p = NULL;
    openstack_external_node_p node_p = NULL;
    if(epp == NULL)
	{
       return;
    }
    epp_p = find_openstack_external_by_outer_ip(epp->external_outer_interface_ip);
    if(epp_p != NULL)
	{
       return;
    }
    node_p = create_openstack_external_node((UINT1*)epp);
    node_p->next = g_openstack_external_list;
    g_openstack_external_list = node_p;
}
//by:yhy 将data构造成g_openstack_external_list所支持的节点结构
openstack_external_node_p create_openstack_external_node(UINT1* data)
{
	openstack_external_node_p ret = NULL;
	ret = (openstack_external_node_p)mem_get(g_openstack_external_node_id);
	
	//addedby:yhy
	if(NULL == ret)
	{
		LOG_PROC("ERROR", "%s -- mem_get(g_openstack_external_node_id) Fail",FN);
		return NULL;
	}
	
	memset(ret,0,sizeof(openstack_external_node));
	ret->data = data;
	return ret;
};

external_floating_ip_p create_floating_ip_p(
		UINT4 fixed_ip,
		UINT4 floating_ip,
		char* port_id,
		char* router_id){
	external_floating_ip_p efp = NULL;
	efp = (external_floating_ip_p)mem_get(g_openstack_floating_id);
	
	//addedby:yhy
	if(NULL == efp)
	{
		LOG_PROC("ERROR", "%s -- mem_get(g_openstack_floating_id) Fail",FN);
		return NULL;
	}
	
    memset(efp,0,sizeof(external_floating_ip));
    if(fixed_ip && fixed_ip!=0){
    	efp->fixed_ip = fixed_ip;
    }
    if(floating_ip && floating_ip!=0){
    	efp->floating_ip = floating_ip;
    }
    if(port_id){
    	strcpy(efp->port_id,port_id);
    }
    if(router_id){
    	strcpy(efp->router_id,router_id);
    }
    add_openstack_floating_to_list(efp);
    test(2);
    return efp;
}
void add_openstack_floating_to_list(external_floating_ip_p efp){
	external_floating_ip_p efp_p = NULL;
	openstack_external_node_p node_p = NULL;
	if(efp == NULL){
	   return;
	}
	efp_p = find_external_floating_ip_by_floating_ip(efp->floating_ip);
	if(efp_p != NULL){
	   return;
	}
	node_p = create_openstack_external_node((UINT1*)efp);
	node_p->next = g_openstack_floating_list;
	g_openstack_floating_list = node_p;
}
void add_nat_icmp_iden_to_list(nat_icmp_iden_p nii){
	nat_icmp_iden_p nii_p = NULL;
	openstack_external_node_p node_p = NULL;
	if(nii == NULL){
	   return;
	}
    nii_p = find_nat_icmp_iden_by_host_ip(nii->host_ip);
	if(nii_p != NULL){
	   return;
	}
	node_p = create_openstack_external_node((UINT1*)nii);
	node_p->next = g_nat_icmp_iden_list;
	g_nat_icmp_iden_list = node_p;
}
//by:yhy 检查openstack对外端口external_p的有效性
INT4 check_external_port_status(external_port_p external_p)
{
	if ((NULL != external_p)
		&& (external_p->external_gateway_ip)
		&& (external_p->external_gateway_mac)
		&& (0 != memcmp(ext_zero_mac, external_p->external_gateway_mac, 6))
		&& (external_p->external_outer_interface_ip)
		&& (external_p->external_outer_interface_mac)
		&& (0 != memcmp(ext_zero_mac, external_p->external_outer_interface_mac, 6))
		&& (external_p->external_dpid)
		&& (external_p->external_port)
		&& (external_p->network_id)
		&& (0 != strlen(external_p->network_id))
		&& (0 != strcmp("0", external_p->network_id))) {
		return GN_OK;
	}
	//by:yhy add 201701051305
	LOG_PROC("ERROR", "check_external_port_status -- Finall return GN_ERR");
	return GN_ERR;
}
//by:yhy 更新外部网关列表信息
external_port_p update_external_port(
        UINT4 external_gateway_ip,
		UINT1* external_gateway_mac,
	    UINT4 external_outer_interface_ip,
		UINT1* external_outer_interface_mac,
		UINT8 external_dpid,
		UINT4 external_port,
		char* network_id)
{
	char ip_str[48]={0};
    external_port_p epp = NULL;
	//by:jjq 根据interface_ip在外部的external结构体中找
    epp = find_openstack_external_by_outer_ip(external_outer_interface_ip);
	number2ip(external_outer_interface_ip, ip_str);
    if(epp == NULL)
	{
        epp = create_external_port(external_gateway_ip,external_gateway_mac,external_outer_interface_ip,
                                   external_outer_interface_mac,external_dpid,external_port,network_id);
    }
	else
	{
		//by:yhy 如果发生外部网关的变化(即原有的网关与现有网关不一致)
		if ((external_dpid) && (external_dpid != epp->external_dpid)) 
		{
			if (g_controller_role == OFPCR_ROLE_MASTER)
			{
				gn_switch_t *sw = NULL;
				//by:jjq 根据dpid从sw列表中找出对应的external_sw
				get_sw_from_dpid(epp->external_dpid, &sw);
				if (sw) 
				{
					//by:yhy 删除目标MAC是该MAC的流表
					remove_fabric_openstack_external_output_flow(sw, epp->external_gateway_mac, epp->external_outer_interface_ip, 1);
					//by:yhy 删除源IP是该IP的流表
					remove_fabric_openstack_external_output_flow(sw, epp->external_gateway_mac, epp->external_outer_interface_ip, 2);
				}
				
			}
			epp->external_dpid= external_dpid;
		}	
		//by:yhy 更新信息
        if(external_gateway_ip && external_gateway_ip!=0)
		{
        	epp->external_gateway_ip=external_gateway_ip;
        }
        if(external_outer_interface_ip && external_outer_interface_ip!=0)
		{
        	epp->external_outer_interface_ip=external_outer_interface_ip;
        }
        if ((external_gateway_mac) && (0 != memcmp(ext_zero_mac, external_gateway_mac, 6))) 
		{
			memcpy(epp->external_gateway_mac, external_gateway_mac, 6);
		}
        if ((external_outer_interface_mac) && (0 != memcmp(ext_zero_mac, external_outer_interface_mac, 6))) 
		{
        	memcpy(epp->external_outer_interface_mac, external_outer_interface_mac, 6);
        }
        if(external_dpid && external_dpid!=0)
		{
        	epp->external_dpid=external_dpid;
        }
        if(external_port && external_port!=0)
		{
        	epp->external_port=external_port;
        }
        if(network_id)
		{
			strcpy(epp->network_id,network_id);
		}
    }

	if (epp) 
	{
		epp->status = check_external_port_status(epp);
	}

	return epp;
}

INT4 compare_floating_ip_list(
		UINT4 fixed_ip,
		UINT4 floating_ip,
		char* port_id,
		char* router_id, 
		external_floating_ip_p efp) 
{
	if ((efp)
		&& (fixed_ip == efp->fixed_ip)
		&& (floating_ip == efp->floating_ip)
		&& compare_str(port_id, efp->port_id)
		&& compare_str(router_id, efp->router_id)) {
		return 1;
	}

	return 0;
}

external_floating_ip_p update_floating_ip_list(
		UINT4 fixed_ip,
		UINT4 floating_ip,
		char* port_id,
		char* router_id)
{
	external_floating_ip_p efp = NULL;
	efp = find_external_floating_ip_by_floating_ip(floating_ip);
	if(efp == NULL) {
		efp = create_floating_ip_p(fixed_ip,floating_ip,port_id,router_id);
		if (efp)
			efp->check_status = (UINT2)CHECK_CREATE;
	}
	else if (compare_floating_ip_list(fixed_ip,floating_ip,port_id,router_id,efp)) {
		efp->check_status = (UINT2)CHECK_LATEST;
	}
	else {
		if (efp->fixed_ip != fixed_ip) {
			remove_proactive_floating_flows_by_floating(efp);
		}
		
		efp->fixed_ip = fixed_ip;

		if(floating_ip && floating_ip!=0){
			efp->floating_ip = floating_ip;
		}
		if(port_id){
			strcpy(efp->port_id,port_id);
		}
		if(router_id){
			strcpy(efp->router_id,router_id);
		}
		
		efp->flow_installed = 0;
		efp->check_status= (UINT2)CHECK_UPDATE;
	}

	return efp;
}
nat_icmp_iden_p update_nat_icmp_iden(
		UINT2 identifier,
		UINT4 host_ip,
		UINT1* host_mac,
		UINT8 sw_dpid,
		UINT4 inport){
	nat_icmp_iden_p nii=NULL;
	nii = find_nat_icmp_iden_by_host_ip(host_ip);
	if(nii == NULL){
		nii=create_nat_icmp_iden(identifier,host_ip,host_mac,sw_dpid,inport);
	}else{
		if(identifier){
			nii->identifier=identifier;
		}
		if(host_ip){
			nii->host_ip=host_ip;
		}
		if(host_mac){
			memcpy(nii->host_mac, host_mac, 6);
		}
		if (sw_dpid) {
			nii->sw_dpid = sw_dpid;
		}
		if (inport) {
			nii->inport = inport;
		}
	}
    
    if (g_controller_role == OFPCR_ROLE_MASTER)
    {
        persist_fabric_nat_icmp_iden_list();
    }

	return nii;
}
void destory_openstack_external(){
    if(g_openstack_external_id != NULL){
        mem_destroy(g_openstack_external_id);
        g_openstack_external_id = NULL;
    }
    if(g_openstack_floating_id !=NULL){
        mem_destroy(g_openstack_floating_id);
        g_openstack_floating_id = NULL;
    }
    if(g_nat_icmp_iden_id !=NULL){
		mem_destroy(g_nat_icmp_iden_id);
		g_nat_icmp_iden_id = NULL;
	}
    if(g_openstack_external_node_id != NULL){
		mem_destroy(g_openstack_external_node_id);
		g_openstack_external_node_id = NULL;
	}
    g_openstack_external_list = NULL;
    g_openstack_floating_list = NULL;
    g_nat_icmp_iden_list=NULL;
}
//by:yhy 通过ip查找对应的openstack外部网关端口
external_port_p find_openstack_external_by_outer_ip(UINT4 external_outer_interface_ip)
{
	//by:yhy 外部网关端口结构体
    external_port_p epp = NULL;
    openstack_external_node_p node_p = g_openstack_external_list;
    while(node_p != NULL)
	{
        epp = (external_port_p)node_p->data;
        if(epp==NULL)
		{
        	return NULL;
        }
        if(epp->external_outer_interface_ip == external_outer_interface_ip)
		{
            return epp;
        }
        node_p = node_p->next;
    }
    return NULL;
};
//by:yhy why?这个函数有问题
external_port_p get_external_port_by_host_mac(UINT1* host_mac)
{
	external_port_p epp = NULL;
	openstack_external_node_p node_p = g_openstack_external_list;

	while(node_p != NULL )
	{
		epp = (external_port_p)node_p->data;
		if ((epp) && (GN_OK == epp->status)) 
		{
			return epp;
		}
		node_p=node_p->next;
	}
	return NULL;
}

external_port_p get_external_port(){
	external_port_p epp = NULL;
	openstack_external_node_p node_p = g_openstack_external_list;
	while(node_p != NULL ){
		epp = (external_port_p)node_p->data;
		if ((epp) && (GN_OK == epp->status)) {
			return epp;
		}
		node_p=node_p->next;
	}
	return NULL;
}
//by:yhy 根据fixed_ip在g_openstack_floating_list查找对应的external_floating_ip_p
external_floating_ip_p find_external_floating_ip_by_fixed_ip(UINT4 fixed_ip)
{
	external_floating_ip_p epp = NULL;
    openstack_external_node_p node_p = g_openstack_floating_list;
    while(node_p != NULL)
	{
        epp = (external_floating_ip_p)node_p->data;
        if(epp==NULL)
		{
        	return NULL;
        }
        if(epp->fixed_ip == fixed_ip)
		{
            return epp;
        }
        node_p = node_p->next;
    }
    return NULL;
};
//by:yhy 根据浮动IP在g_openstack_external_list中查找与之对应的external_port_p
external_port_p find_openstack_external_by_floating_ip(UINT4 external_floating_ip){
	external_port_p epp = NULL;
	openstack_external_node_p node_p = g_openstack_external_list;
    char network_id[48];
    find_fabric_network_by_floating_ip(external_floating_ip,network_id);
	while(node_p != NULL )
	{
		epp = (external_port_p)node_p->data;
		if(epp->external_dpid  && epp->external_port)
		{
			if((0 != strlen(network_id)) && (0 == strcmp(epp->network_id,network_id)))
			{
				return epp;
			}
		}
		node_p=node_p->next;
	}
	return NULL;
};
external_port_p find_openstack_external_by_gatway_ip(UINT4 external_gateway_ip){
    external_port_p epp = NULL;
    openstack_external_node_p node_p = g_openstack_external_list;
    while(node_p != NULL){
        epp = (external_port_p)node_p->data;
        if ((epp) && (epp->external_gateway_ip == external_gateway_ip)) {
            return epp;
        }
        node_p = node_p->next;
    }
    return NULL;
};
//by:yhy 根据network_id匹配并查找g_openstack_external_list中的项
external_port_p find_openstack_external_by_network_id(char* network_id)
{
    external_port_p epp = NULL;
    openstack_external_node_p node_p = g_openstack_external_list;
	
    while(node_p) 
	{
        epp = (external_port_p)node_p->data;
		
        if ((epp) && (0 != strlen(epp->network_id)) && (0 == strcmp(network_id, epp->network_id))) 
		{
            return epp;
        }
        node_p = node_p->next;
    }
    return NULL;
};
//by:yhy 通过输入IP查找是否是对外floating ip,是则返回external_floating_ip_p
external_floating_ip_p find_external_floating_ip_by_floating_ip(UINT4 floating_ip)
{
	external_floating_ip_p efp = NULL;
    openstack_external_node_p node_p = g_openstack_floating_list;
    while(node_p != NULL)
	{//by:yhy 遍历
    	efp = (external_floating_ip_p)node_p->data;
        if(efp==NULL)
		{
        	return NULL;
        }
        if(efp->floating_ip == floating_ip)
		{
            return efp;
        }
        node_p = node_p->next;
    }
    return NULL;
}
external_port_p find_openstack_external_by_outer_mac(UINT1* external_gateway_mac){
    external_port_p epp = NULL;
    openstack_external_node_p node_p = g_openstack_external_list;
    while(node_p != NULL){
        epp = (external_port_p)node_p->data;
        if ((epp) && (0 == memcmp(epp->external_gateway_mac, external_gateway_mac, 6))) {
            return epp;
        }
        node_p = node_p->next;
    }
    return NULL;
};

//by:yhy 通过ip和mac查找openstack对外网关端口
external_port_p find_openstack_external_by_ip_mac(UINT4 gateway_ip, UINT1* gateway_mac, UINT4 outer_ip, UINT1* outer_mac)
{
    external_port_p epp = NULL;
    openstack_external_node_p node_p = g_openstack_external_list;
    while(node_p != NULL)
	{
        epp = (external_port_p)node_p->data;
		if (epp) 
		{
			if ((epp->external_outer_interface_ip == outer_ip) && (epp->external_gateway_ip == gateway_ip) && (0 == memcmp(epp->external_outer_interface_mac, outer_mac, 6))) 
			{
				if (0 != memcmp(epp->external_gateway_mac, gateway_mac, 6)) 
				{
					return epp;
				}
				else 
				{
					return NULL;
				}
			}
		}		
        node_p = node_p->next;
    }
    return NULL;
};



nat_icmp_iden_p find_nat_icmp_iden_by_host_ip(UINT4 host_ip){
	nat_icmp_iden_p nii = NULL;
	openstack_external_node_p node_p = g_nat_icmp_iden_list;
	while(node_p != NULL){
		nii = (nat_icmp_iden_p)node_p->data;
		if(nii==NULL){
			return NULL;
		}
		if(nii->host_ip==host_ip){
			return nii;
	    }
		node_p = node_p->next;
	}
	return NULL;
}
nat_icmp_iden_p find_nat_icmp_iden_by_host_mac(UINT1* host_mac){
	nat_icmp_iden_p nii = NULL;
	openstack_external_node_p node_p = g_nat_icmp_iden_list;
	while(node_p != NULL){
		nii = (nat_icmp_iden_p)node_p->data;
		if(nii==NULL){
			return NULL;
		}
		node_p = node_p->next;
	}
	return NULL;
}
nat_icmp_iden_p find_nat_icmp_iden_by_identifier(UINT2 identifier){
	nat_icmp_iden_p nii = NULL;
	openstack_external_node_p node_p = g_nat_icmp_iden_list;
	while(node_p != NULL){
		nii = (nat_icmp_iden_p)node_p->data;
		if(nii==NULL){
			return NULL;
		}
		if(nii->identifier==identifier){
			return nii;
	    }
		node_p = node_p->next;
	}
	return NULL;
}


void test(UINT1 type)
{
	openstack_external_node_p node = NULL;
	UINT4 num = 0;
	if(type==1)
	{
		node = g_openstack_external_list;
		while(node != NULL)
		{
			external_port_p p = (external_port_p)(node->data);
			if(NULL!=p)
			{
				node = node->next;
				num++;
			}
			else
			{
	            printf("node data: %s \n",node->data);
	            node = node->next;
			}
		}
	}else if(type==2){
		node = g_openstack_floating_list;
		while(node != NULL){
			external_floating_ip_p p = (external_floating_ip_p)(node->data);
			if(NULL!=p){
				node = node->next;
				num++;
			}else{
	            printf("node data: %s \n",node->data);
	            node = node->next;
			}
		}
	}else if(type==3){
		node = g_nat_icmp_iden_list;
		while(node != NULL){
			nat_icmp_iden_p p = (nat_icmp_iden_p)(node->data);
			if(NULL!=p){
				node = node->next;
				// printf("g_nat_icmp_iden_list : id:[%d]  | ip:[%d]  | \n",p ->identifier,p->host_ip);
				num++;
			}else{
				printf("node data: %s \n",node->data);
				node = node->next;
			}
		}
	}

	LOG_PROC("INFO", "external number: %d",num);
	return;
}

void update_floating_ip_mem_info(){
	updateOpenstackFloating();
}
//by:yhy 根据dpid 查找对应的switch
void get_sw_from_dpid(UINT8 dpid,gn_switch_t **sw)
{
	UINT2 i = 0;
	for(i = 0; i < g_server.max_switch; i++)
	{
		if (g_server.switches[i].dpid==dpid)
		{
			*sw =   &g_server.switches[i];
			return;
		}
	}
}
//by:yhy 读取(并更新和持久化)openstack外部端口配置
void read_external_port_config()
{
    if (g_controller_role == OFPCR_ROLE_SLAVE)
    {
        return;
    }

	// LOG_PROC("INFO", "read external port config");
	// read the configure
	UINT4 seq_num = 0;

	for (seq_num = external_min_seq ; seq_num <= external_max_seq; seq_num++) 
	{
		char network_id[48];
		UINT4 external_gateway_ip = 0;
		UINT4 external_outer_interface_ip = 0;
		UINT1 external_outer_interface_mac[6] = {0};
		UINT1 external_gateway_mac[6] = {0};
		UINT8 external_dpid = 0;
		UINT4 external_port = 0;

		char para_title[48] = {0};
		sprintf(para_title, "[external_switch_%d]", seq_num);
		
		if (get_selection_by_selection(g_controller_configure, para_title)) 
		{
			//by:yhy 获取配置信息
			INT1 *value = NULL;
			value = get_value(g_controller_configure, para_title, "external_gateway_ip");
			external_gateway_ip = (NULL == value) ? 0: ip2number(value);

			value = get_value(g_controller_configure, para_title, "external_gateway_mac");
			memset(external_gateway_mac, 0, 6);
			macstr2hex(value, external_gateway_mac);

			value = get_value(g_controller_configure, para_title, "external_outer_interface_ip");
			external_outer_interface_ip = (NULL == value) ? 0: ip2number(value);

			// value = get_value(g_controller_configure, para_title, "external_outer_interface_mac");
			// memset(external_outer_interface_mac, 0, 6);
			// macstr2hex(value, external_outer_interface_mac);

			value = get_value(g_controller_configure, para_title, "external_dpid");
			UINT1 dpid_tmp[8] = {0};
			mac_str_to_bin(value, dpid_tmp);
			uc8_to_ulli64 (dpid_tmp, &external_dpid);

			value = get_value(g_controller_configure, para_title, "external_port");
			external_port = (NULL == value) ? 0: atoi(value);

			// value = get_value(g_controller_configure, para_title, "external_network_id");
			// (NULL == value) ? strncpy(network_id, "0", 48) : strncpy(network_id, value, 48);

			if (external_gateway_ip && external_outer_interface_ip && external_dpid && external_port) 
			{
				LOG_PROC("INFO", "External: read config of switch: %d!", seq_num);
				//by:yhy why? check 理解是否正确 检查openstack中的对外IP是否在控制器的主机列表中(why? 外连口IP怎么会在某台主机上?)
				p_fabric_host_node host = get_fabric_host_from_list_by_ip(external_outer_interface_ip);
				if ((host) && (host->data)) 
				{
					openstack_port_p port_p = (openstack_port_p)host->data;
					//by:yhy why?为什么要做下面的操作
					memcpy(external_outer_interface_mac, host->mac, 6);
					strcpy(network_id, port_p->network_id);
				}
				//by:yhy 更新
				update_external_port(external_gateway_ip, external_gateway_mac, external_outer_interface_ip, external_outer_interface_mac,
									 external_dpid, external_port, network_id);
			}
			else 
			{
				LOG_PROC("INFO", "External: wrong param in %s. Please check the configuration.", para_title);
			}
		}
	}

    if (g_controller_role == OFPCR_ROLE_MASTER)
    {
        persist_fabric_openstack_external_list();
    }
}

openstack_external_node_p get_floating_list()
{
	return g_openstack_floating_list;
}
//by:yhy 复位g_openstack_floating_list中external_floating_ip_p节点的check_status
void reset_floating_ip_flag()
{
	openstack_external_node_p node_p = g_openstack_floating_list;
	while (node_p) 
	{
		external_floating_ip_p epp = (external_floating_ip_p)node_p->data;
		if (epp)
		{
			epp->check_status = (UINT2)CHECK_UNCHECKED;
		}
		
		node_p = node_p->next;
	}
}
//by:yhy 复位g_openstack_floating_list->data->check_status
void cleare_floating_ip_unchecked()
{
	external_floating_ip_p epp = NULL;
	openstack_external_node_p head_p = g_openstack_floating_list;
	openstack_external_node_p prev_p = head_p;

	if (NULL == head_p)
	{
		return ;
	}

	openstack_external_node_p next_p = prev_p->next;

	while (next_p)
	{
		//by:yhy 遍历复位g_openstack_floating_list->data->check_status
		epp = (external_floating_ip_p)next_p->data;
		
		if ((epp) && (epp->check_status == (UINT2)CHECK_UNCHECKED)) 
		{
			remove_proactive_floating_flows_by_floating(epp);
			prev_p->next = next_p->next;
			mem_free(g_openstack_floating_id, epp);
			mem_free(g_openstack_external_node_id, next_p);
		}
		else 
		{
			prev_p = prev_p->next;
		}
		next_p = prev_p->next;
	}

	epp = (external_floating_ip_p)head_p->data;
	if ((epp) && (epp->check_status== (UINT2)CHECK_UNCHECKED)) 
	{
		//by:yhy 复位g_openstack_floating_list头结点->data->check_status
		remove_proactive_floating_flows_by_floating(epp);
		next_p = head_p->next;
		mem_free(g_openstack_floating_id, epp);
		mem_free(g_openstack_external_node_id, next_p);
		g_openstack_floating_list = next_p;
	}	
}
//by:yhy 刷新浮动IP
void reload_floating_ip()
{
	reset_floating_ip_flag();
	updateOpenstackFloating();
	cleare_floating_ip_unchecked();
}
//by:yhy 更新openstack 外部网关MAC
void update_openstack_external_gateway_mac(UINT4 gateway_ip, UINT1* gateway_mac, UINT4 outer_ip, UINT1* outer_mac)
{
	external_port_p epp = find_openstack_external_by_ip_mac(gateway_ip, gateway_mac, outer_ip, outer_mac);
	if (epp) 
	{
		UINT8 dpid = epp->external_dpid;
		UINT4 port = epp->external_port;
		char network_id[48] = {0};
		memcpy(network_id, epp->network_id, 48);
		create_external_port_by_rest(gateway_ip, gateway_mac, outer_ip, outer_mac, dpid, port, network_id);
	}
}
//by:yhy 根据对外接口信息更新对外网关端口
void update_openstack_external_by_outer_interface(UINT4 host_ip, UINT1* host_mac, char* network_id)
{
	if ((0 == host_ip) || (NULL == host_mac) || (NULL == network_id))
	{
		return ;
	}
	external_port_p external_p = find_openstack_external_by_outer_ip(host_ip);
	if ((external_p) && (external_p->external_outer_interface_ip == host_ip)&&(0 == memcmp(external_p->external_outer_interface_mac, host_mac, 6))&& (0 == strcmp(external_p->network_id, network_id))) 
	{
		return;
	}
	else
	{		
		create_external_port_by_rest(0, ext_zero_mac, host_ip, host_mac, 0, 0, network_id);
	}
}
//by:yhy 外部网关校验(对网关MAC发送ARP请求包)
void external_check_tx_timer(void *para, void *tid)
{
	LOG_PROC("TIMER", "external_check_tx_timer - START");
	if (0 == g_external_check_on) 
	{
		LOG_PROC("TIMER", "external_check_tx_timer - 0 == g_external_check_on");
		return ;
	}
	
	external_port_p epp = NULL;
    openstack_external_node_p node_p = g_openstack_external_list;
    while(node_p != NULL)
	{//by:yhy 遍历对外网关列表
        epp = (external_port_p)node_p->data;
		if ((epp) && (epp->external_dpid) && (epp->external_port) && (epp->external_outer_interface_ip)) 
		{
			gn_switch_t *sw = NULL;
			get_sw_from_dpid(epp->external_dpid, &sw);
			if (sw) 
			{
				fabric_opnestack_create_arp_request(epp->external_outer_interface_ip, 
													epp->external_gateway_ip, 
													epp->external_outer_interface_mac, 
													sw, 
													epp->external_port);
			}
		}
        node_p = node_p->next;
    }
	LOG_PROC("TIMER", "external_check_tx_timer - STOP");
}
//by:yhy 启动对外交换机检查
void start_external_mac_check(UINT4 check_on, UINT4 check_internal)
{
	INT1* value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	UINT4 flag_openstack_on = (NULL == value) ? 0: atoi(value);
	if (check_internal)
	{
		g_external_check_interval = check_internal;
	}
	g_external_check_on = check_on;

	if ((flag_openstack_on) && (g_external_check_on) && (g_external_check_interval)) 
	{
		//by:yhy 定时对对外交换机发送ARP请求
		external_check_tx_timer(NULL, NULL);

		g_external_check_timerid = timer_init(1);
		timer_creat(g_external_check_timerid, g_external_check_interval, NULL, &g_external_check_timer, external_check_tx_timer);
	}
}

void stop_external_mac_check()
{
	timer_kill(g_external_check_timerid, &g_external_check_timer);
	g_external_check_timerid = NULL;
	g_external_check_timer = NULL;
	g_external_check_on = 0;
}

//by:yhy 启动对外交换机检查(定期发送arp request)
void init_external_mac_check_mgr()
{
	UINT4 check_on = 0;
	UINT4 check_interval = 0;
	
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "external_check_on");
	check_on = (NULL == value) ? 0: atoi(value);
	value = get_value(g_controller_configure, "[openvstack_conf]", "external_check_internal");
	check_interval = (NULL == value) ? 20: atoi(value);

	start_external_mac_check(check_on, check_interval);
}



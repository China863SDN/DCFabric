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
 *  fabric OpenStack NAT module
 *
 *  Created on: September 6th, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */

#include "fabric_openstack_nat.h"
#include "mem_pool.h"
#include "gn_inet.h"
#include "fabric_flows.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "timer.h"
#include "../conn-svr/conn-svr.h"
#include "fabric_impl.h"
#include "fabric_flows.h"
#include "openstack_app.h"
#include "ini.h"
#include "fabric_openstack_arp.h"
#include "../cluster-mgr/hbase_sync.h"
#include "openflow-common.h"
#include "../cluster-mgr/cluster-mgr.h"


// 定义NAT最大连接数
#define NAT_HOST_MAX_NUM 		1000
#define NAT_PORT_MAX_VALUE		59200
#define NAT_PORT_MIN_VALUE		49200
#define NAT_PORT_MAX_COUNT		10000

// 定义flag: 是否使用物理交换机处理NAT流表的修改
UINT1 g_nat_physical_switch_flag = 1;

// 定义id, 用来区别mem_pool
void *g_nat_host_id = NULL;
void *g_nat_port_id = NULL;

// 定义全局的外部IPList
nat_host_p g_nat_host_list_head = NULL;
// 定义全局的port总数
UINT2 g_nat_port_total_count = 0;

// 初始化,分配内存池
void init_nat_mem_pool()
{
	// LOG_PROC("INFO", "NAT: Start create memory pool");

	// 判断存在
	if (NULL != g_nat_host_id) {
		mem_destroy(g_nat_host_id);
	}
	// 分配内存
	g_nat_host_id = mem_create(sizeof(nat_host), NAT_HOST_MAX_NUM);

	// 判断存在
	if (NULL != g_nat_port_id) {
		mem_destroy(g_nat_port_id);
	}
	// 分配内存
	g_nat_port_id = mem_create(sizeof(nat_port), NAT_PORT_MAX_COUNT);

	g_nat_host_list_head = NULL;
	g_nat_port_total_count = 0;

	// get openstack configure
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "use_physical_switch_modify_nat");
	g_nat_physical_switch_flag = ((NULL == value) ? 1 : atoi(value));

	// LOG_PROC("INFO", "NAT: Success create memory pool");
}

// 销毁内存池
void destroy_nat_mem_pool()
{
	// LOG_PROC("INFO", "NAT: Start destroy memory pool");

	// 判断存在
	if (NULL != g_nat_host_id) {
		// 销毁内存
		mem_destroy(g_nat_host_id);
		g_nat_host_id = NULL;
	}

	// 判断存在
	if (NULL != g_nat_port_id) {
		// 销毁内存
		mem_destroy(g_nat_port_id);
		g_nat_port_id = NULL;
	}

	g_nat_host_list_head = NULL;
	g_nat_port_total_count = 0;
	// LOG_PROC("INFO", "NAT: Success destroy memory pool");

	return;
}

// 创建nat_host头结点
nat_host_p init_nat_host()
{
	// LOG_PROC("INFO", "NAT: Start create host head node");

	// 定义空指针
	nat_host_p host_p = NULL;
	nat_port_p port_p = NULL;

	host_p = (nat_host_p)mem_get(g_nat_host_id);

	if (NULL == host_p) {
		// output log
		LOG_PROC("INFO", "NAT: get host memory failed!");
		return NULL;
	}

	// set host info
	host_p->host_ip = 0;
	host_p->next = NULL;

	// initialize TCP/UDP port list
	port_p = init_nat_port();
	host_p->tcp_port_list = port_p;

	port_p = init_nat_port();
	host_p->udp_port_list = port_p;

	// set the head
	g_nat_host_list_head = host_p;

	// LOG_PROC("INFO", "NAT: Start create host head node");

	return host_p;
}

// 创建nat_host条目, 此时内容为空
nat_host_p create_nat_host(UINT4 host_ip)
{
	// LOG_PROC("INFO", "NAT: Start create host ip:");
	// nat_show_ip(host_ip);

	// 如果已经达到创建的最大数
	if (g_nat_host_list_head->host_ip >= NAT_HOST_MAX_NUM -1 ) {
		// output log
		LOG_PROC("INFO", "NAT: Fail! create host number is max");
		return NULL;
	}

	// 定义指针
	nat_host_p prev_p = NULL;
	nat_host_p current_p = NULL;
	nat_host_p host_p = NULL;
	nat_port_p port_p = NULL;

	prev_p = g_nat_host_list_head;
	current_p = g_nat_host_list_head->next;

	// 循环判断
	while ((NULL != current_p) && (current_p->host_ip <= host_ip)) {
		if (current_p->host_ip == host_ip) {
			LOG_PROC("INFO", "NAT: Can't create host: IP exist!");
			return NULL;
		}

		prev_p = current_p;
		current_p = current_p->next;
	}

	// 分配内存
	host_p = (nat_host_p)mem_get(g_nat_host_id);

	if (NULL == host_p) {
		// output log
		LOG_PROC("INFO", "NAT: create host failed: Can't get memory");
		return NULL;
	}

	// 记录数据
	host_p->host_ip = host_ip;
	port_p = init_nat_port();
	if (NULL == port_p) {
		// output log
		LOG_PROC("INFO", "NAT: Stop create host: Can't create port node");
		mem_free(g_nat_host_id, host_p);
		return NULL;
	}

	host_p->tcp_port_list = port_p;
	port_p = init_nat_port();
	if (NULL == port_p) {
		// output log
		LOG_PROC("INFO", "NAT: Stop create host: Can't create port node");
		mem_free(g_nat_host_id, host_p);
		return NULL;
	}
	host_p->udp_port_list = port_p;

	prev_p->next = host_p;
	host_p->next = current_p;

	g_nat_host_list_head->host_ip++;

	// LOG_PROC("INFO", "NAT: Success create host");
	return host_p;
}

// 删除指定外部地址的nat_host条目
void remove_nat_host_by_ip(UINT4 host_ip)
{
	// LOG_PROC("INFO", "NAT: Start remove external ip ");
	// nat_show_ip(host_ip);

	nat_host_p prev_p = NULL;
	nat_host_p next_p = NULL;

	prev_p = g_nat_host_list_head;
	next_p = prev_p->next;

	// 循环判断
	while ((NULL != next_p) && (next_p->host_ip <= host_ip)) {
		if (next_p->host_ip == host_ip) {
			prev_p->next = next_p->next;
			next_p = NULL;
			g_nat_host_list_head->host_ip--;
			// 释放内存
			mem_free(g_nat_host_id, next_p);
			// LOG_PROC("INFO", "NAT: Success! remove ip total count is %d", g_nat_host_list_head->host_ip);
			// nat_show_ip(host_ip);
			return ;
		}
		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	// LOG_PROC("INFO", "NAT: Fail! remove ip is not exist");
}

// 查找指定外部地址的nat_host条目
nat_host_p find_nat_host_by_ip(UINT4 host_ip)
{
	// LOG_PROC("INFO", "NAT: Start find host ip");
	// nat_show_ip(host_ip);

	nat_host_p host_p = g_nat_host_list_head->next;

	// 循环判断
	while ((NULL != host_p) && (host_p->host_ip <= host_ip)) {
		if (host_p->host_ip == host_ip) {
			// LOG_PROC("INFO", "NAT: Success! find host ip");
			return host_p;
		}
		host_p = host_p->next;
	}

	// LOG_PROC("INFO", "NAT: Fail! host ip is not exist");
	return NULL;
}

// 创建nat_port头结点
nat_port_p init_nat_port()
{
	// LOG_PROC("INFO", "NAT: Start create port head node");

	// 定义空指针
	nat_port_p port_p = NULL;

	port_p = (nat_port_p)mem_get(g_nat_port_id);

	if (NULL == port_p) {
		// output error log
		LOG_PROC("INFO", "NAT: get memory failed");
		return NULL;
	}

	port_p->external_port_no = NAT_PORT_MIN_VALUE;
	port_p->next = NULL;

	g_nat_port_total_count++;
	// LOG_PROC("INFO", "NAT: Start create port_p head node");
	return port_p;
}

// 创建nat_host条目, 此时内容为空
nat_port_p create_nat_port(UINT4 internal_ip, UINT2 internal_port_no,
		UINT1* internal_mac, UINT8 external_gateway_dpid, nat_port_p port_head_p)
{
	// LOG_PROC("INFO", "NAT: Start create nat internal ip");
	// nat_show_ip(internal_ip);

	// 定义指针
	nat_port_p prev_p = NULL;
	nat_port_p current_p = NULL;
	nat_port_p port_p = NULL;
	UINT2 current_min_port_no = port_head_p->external_port_no ;
	prev_p = current_p = port_head_p;

	// if max value
	if (NAT_PORT_MAX_COUNT == g_nat_port_total_count) {
		LOG_PROC("INFO", "NAT: The Port total count is Max");
		return NULL;
	}

	while(NULL != current_p)
	{
		if(current_p->external_port_no > current_min_port_no + 1)
			break;
		else
		{
			current_min_port_no = current_p->external_port_no;
			prev_p = current_p;
			current_p = current_p->next;
		}
	}

	if (current_min_port_no == NAT_PORT_MAX_VALUE) {
		// output log
		LOG_PROC("INFO", "NAT: The Port value is Max");
		return NULL;
	}

	// 分配内存
	port_p = (nat_port_p)mem_get(g_nat_port_id);

	if (NULL == port_p) {
		// output log
		LOG_PROC("INFO", "NAT: set port failed to get memory");
		return NULL;
	}

	// 记录数据
	port_p->external_port_no = current_min_port_no + 1;
	port_p->internal_ip = internal_ip;
	port_p->internal_port_no = internal_port_no;
	memset(port_p->internal_mac, 0, 6);
	memcpy(port_p->internal_mac, internal_mac, 6);
	port_p->gateway_dpid = external_gateway_dpid;

	prev_p->next = port_p;
	port_p->next = current_p;

	g_nat_port_total_count++;
	// LOG_PROC("INFO", "NAT: Success create port, total count is %d", port_head_p->external_port_no);
	return port_p;
}

// 删除指定List所有nat_port条目
void remove_nat_all_port(nat_port_p head_p)
{
	// LOG_PROC("INFO", "NAT: Remove all port");

	// 定义指针
	nat_port_p port_p = head_p;
	nat_port_p temp_p = NULL;

	// 判断是否存在
	if (NULL == port_p) {
		// output error log
		LOG_PROC("INFO", "NAT: Remove port is NULL");
		return ;
	}

	// 循环判断
	while (NULL != port_p) {
		temp_p = port_p->next;
		// // LOG_PROC("INFO", "NAT: Remove port id :%d", port_p->external_port_no);
		g_nat_port_total_count--;
		mem_free(g_nat_port_id, port_p);
		port_p = temp_p;
	}
	// LOG_PROC("INFO", "NAT: Success Remove all port");
}

// 根据port number删除条目
void remove_nat_port_by_port_no(UINT2 external_port_no, nat_port_p head_p)
{
	// LOG_PROC("INFO", "NAT: Remove port by id:%d", external_port_no);

	// 判断List是否为空
	if (NULL == head_p) {
		// output error log
		// LOG_PROC("INFO", "NAT: head pointer is NULL");
		return ;
	}

	// 定义指针
	nat_port_p prev_p = head_p;
	nat_port_p next_p = prev_p->next;

	// 循环判断
	while ((NULL != next_p) && (next_p->external_port_no <= external_port_no)) {
		if (next_p->external_port_no == external_port_no) {
			prev_p->next = next_p->next;
			next_p = NULL;
			// 释放内存
			mem_free(g_nat_host_id, next_p);
			g_nat_port_total_count--;
			// LOG_PROC("INFO", "NAT: Success! remove id %d", external_port_no);
			return ;
		}
		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	// LOG_PROC("INFO", "NAT: Fail to find Remove port by id");
}


// 根据port number删除条目
void remove_nat_port_by_mac_and_port(UINT8 sw_dpid, UINT1* internal_mac, UINT2 internal_port_no, nat_port_p head_p)
{
	// LOG_PROC("INFO", "NAT: Start Remove port by id:%d", internal_port_no);

	// 判断List是否为空
	if (NULL == head_p) {
		// output error log
		LOG_PROC("INFO", "NAT: head pointer is NULL");
		return ;
	}

	// 定义指针
	nat_port_p prev_p = head_p;
	nat_port_p next_p = prev_p->next;

	// 循环判断
	while (NULL != next_p) {

		if ((0 == memcmp(next_p->internal_mac, internal_mac, 6))
				&&(next_p->internal_port_no == internal_port_no)
				&&(next_p->src_dpid == sw_dpid))
		{
			prev_p->next = next_p->next;
			next_p = NULL;
			// 释放内存
			mem_free(g_nat_host_id, next_p);
			g_nat_port_total_count--;
			// LOG_PROC("INFO", "NAT: Success! remove id %d", internal_port_no);
			return ;
		}
		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	// LOG_PROC("INFO", "NAT: Fail! can't find id %d", internal_port_no);
}

// 根据port id查找条目
nat_port_p find_nat_port_by_port_no(UINT2 external_port_no, nat_port_p head_p)
{
	// LOG_PROC("INFO", "NAT: start find port by id:%d", external_port_no);

	// 判断List是否为空
	if (NULL == head_p) {
		// output error log
		// LOG_PROC("INFO", "NAT: head pointer is NULL");
		return NULL;
	}

	// 定义指针
	nat_port_p prev_p = head_p;
	nat_port_p next_p = prev_p->next;

	// 循环判断
	while ((NULL != next_p) && (next_p->external_port_no <= external_port_no)) {
		if (next_p->external_port_no == external_port_no) {
			// LOG_PROC("INFO", "NAT: success to find port by id:%d!", external_port_no);
			return next_p;
		}
		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	// LOG_PROC("INFO", "NAT: Fail to find port by id");
	return NULL;
}

// 根据ip信息查找条目
nat_port_p find_nat_port_by_ip_and_port(UINT4 internal_ip, UINT2 internal_port_no, nat_port_p head_p)
{
	// LOG_PROC("INFO", "NAT: Find port by internal_ip and port| internal_port:%d", internal_port_no);
	// nat_show_ip(internal_ip);

	// 定义指针
	nat_port_p prev_p = head_p;
	nat_port_p next_p = prev_p->next;

	// 循环判断
	while (NULL != next_p) {
		if ((next_p->internal_ip == internal_ip)
				&&(next_p->internal_port_no == internal_port_no))
		{
			// LOG_PROC("INFO", "NAT: Success to find port by ip and port");
			return next_p;
		}
		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	// LOG_PROC("INFO", "NAT: Fail to find port by ip and port");
	return NULL;
}

UINT4 find_nat_port_by_internal_ip(UINT4 internal_ip, UINT2* port_list, UINT4* externalip_list,
		UINT2* proto_list, UINT4 externalip, UINT4 port_number, UINT2 proto, nat_port_p head_p)
{
	// 定义指针
	nat_port_p prev_p = head_p;
	nat_port_p next_p = prev_p->next;

	// 循环判断
	while (NULL != next_p) {
		if (next_p->internal_ip == internal_ip)
		{
			// LOG_PROC("INFO", "NAT: Success to find port by ip and port");
			port_number++;
			port_list[port_number] = next_p->external_port_no;
			proto_list[port_number] = proto;
			externalip_list[port_number] = externalip;
		}
		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	return port_number;
}

// 根据ip信息查找条目
nat_port_p find_nat_port_by_mac_and_port(UINT1* internal_mac, UINT2 internal_port_no, nat_port_p head_p)
{
	// LOG_PROC("INFO", "NAT: Find port by internal_mac and port| internal_port:%d", internal_port_no);
	// nat_show_ip(internal_ip);

	// 定义指针
	nat_port_p prev_p = head_p;
	nat_port_p next_p = prev_p->next;

	// 循环判断
	while (NULL != next_p) {
		if (memcmp(next_p->internal_mac, internal_mac, 6)
				&&(next_p->internal_port_no == internal_port_no))
		{
			// LOG_PROC("INFO", "NAT: Success to find port by ip and port");
			return next_p;
		}
		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	// LOG_PROC("INFO", "NAT: Fail to find port by ip and port");
	return NULL;
}

// 根据{内部IP,外部IP,内部端口号,协议类型}创建新的可以使用的nat端口号,并保存相应数据
UINT2 create_nat_connect(UINT4 internal_ip, UINT4 host_ip, UINT2 internal_port_no,
		UINT1 proto_type, UINT1* internal_mac, UINT8 gateway_dpid, UINT8 src_dpid)
{
	// LOG_PROC("INFO", "NAT: create NAT connect!");

	// 定义返回值
	UINT2 external_port_no = 0;

	// 定义指针
	nat_host_p host_p  = NULL;
	nat_port_p port_p = NULL;
	nat_port_p head_p = NULL;

	// 判断host条目(以外部IP为key)存在
	host_p = find_nat_host_by_ip(host_ip);

	// 如果host条目不存在
	if (NULL == host_p) {
		// 创建对应的条目
		host_p = create_nat_host(host_ip);
	}

	if (NULL == host_p) {
		// output log
		LOG_PROC("INFO", "NAT: create NAT connect: create external Failed!");
		return 0;
	}

	// 如果TCP协议
	if (IPPROTO_TCP == proto_type) {
		// 查找对应的port(以外部端口为Key)存在
		head_p = host_p->tcp_port_list;
	}
	// 如果UDP协议
	else if (IPPROTO_UDP == proto_type) {
		// 查找对应的port(以外部端口为Key)存在
		head_p = host_p->udp_port_list;
	}
	else {
		// output log
		LOG_PROC("INFO", "NAT: remove NAT Node  failed! Not TCP/UDP!");
		return 0;
	}

	// 判断port条目存在
	port_p = find_nat_port_by_ip_and_port(internal_ip, internal_port_no, head_p);

	if (NULL == port_p) {
		// create nat port
		port_p = create_nat_port(internal_ip, internal_port_no, internal_mac, gateway_dpid, head_p);
	}

	// 如果不为空
	if (NULL != port_p) {
		// 创建成功
		external_port_no = port_p->external_port_no;
		port_p->src_dpid = src_dpid;
	}
	else {
		// output log
		LOG_PROC("INFO", "NAT: create NAT connect: create port Failed!");
		return 0;
	}

	// LOG_PROC("INFO", "NAT: Success create NAT connect!");

    if (g_controller_role == OFPCR_ROLE_MASTER)
    {
        persist_fabric_nat_host_single(OPERATE_ADD, host_p);
    }

	// 返回
	return external_port_no;
}

// 根据{外部IP,转换后的端口号,协议类型}查找并且删除该条目
void destroy_nat_connect(UINT4 host_ip, UINT2 external_port_no, UINT1 proto_type)
{
	// LOG_PROC("INFO", "NAT: Start destroy NAT connect!");

	// 定义指针
	nat_host_p host_p  = NULL;

	// 判断host条目(以外部IP为key)存在
	host_p = find_nat_host_by_ip(host_ip);

	// 如果host条目不存在
	if (NULL == host_p) {
		LOG_PROC("INFO", "NAT: destroy NAT connect failed! ip not exist!");
		return ;
	}

	// 如果TCP协议
	if (IPPROTO_TCP == proto_type) {
		// 删除对应的port(以外部端口号为Key)存在
		remove_nat_port_by_port_no(external_port_no, host_p->tcp_port_list);
	}
	// 如果UDP协议
	else if (IPPROTO_UDP == proto_type) {
		// 删除对应的port(以外部端口号为Key)存在
		remove_nat_port_by_port_no(external_port_no, host_p->udp_port_list);
	}
	else {
		// output log
		LOG_PROC("INFO", "NAT: destroy NAT Node failed! Not TCP/UDP!");
		return ;
	}

	// 如果port已经全部删除
	if ((NULL == host_p->tcp_port_list->next)
			&&(NULL == host_p->udp_port_list->next)){
        if (g_controller_role == OFPCR_ROLE_MASTER)
        {
            persist_fabric_nat_host_single(OPERATE_DEL, host_p);
        }

		// 删除该host
		remove_nat_host_by_ip(host_p->host_ip);
        return;
	}

    if (g_controller_role == OFPCR_ROLE_MASTER)
    {
        persist_fabric_nat_host_single(OPERATE_ADD, host_p);
    }
}

void destroy_nat_connect_by_mac_and_port(gn_switch_t* sw, UINT4 host_ip, UINT1* internal_mac, UINT2 internal_port_no, UINT1 proto_type)
{
	// LOG_PROC("INFO", "NAT: Start destroy NAT connect!");

	// 定义指针
	nat_host_p host_p  = NULL;

	// 判断host条目(以外部IP为key)存在
	host_p = find_nat_host_by_ip(host_ip);

	// 如果host条目不存在
	if (NULL == host_p) {
		LOG_PROC("INFO", "NAT: destroy NAT connect failed! ip not exist!");
		return ;
	}

	// 如果TCP协议
	if (IPPROTO_TCP == proto_type) {
		// 删除对应的port(以外部端口号为Key)存在
		remove_nat_port_by_mac_and_port(sw->dpid, internal_mac, internal_port_no, host_p->tcp_port_list);
	}
	// 如果UDP协议
	else if (IPPROTO_UDP == proto_type) {
		// 删除对应的port(以外部端口号为Key)存在
		remove_nat_port_by_mac_and_port(sw->dpid, internal_mac, internal_port_no, host_p->udp_port_list);
	}
	else {
		// output log
		LOG_PROC("INFO", "NAT: destroy NAT Node failed! Not TCP/UDP!");
		return ;
	}

	// 如果port已经全部删除
	if ((NULL == host_p->tcp_port_list->next)
			&&(NULL == host_p->udp_port_list->next))
    {   
        if (g_controller_role == OFPCR_ROLE_MASTER)
        {
            persist_fabric_nat_host_single(OPERATE_DEL, host_p);
        }
		// 删除该host
		remove_nat_host_by_ip(host_p->host_ip);
        return;
	}

    if (g_controller_role == OFPCR_ROLE_MASTER)
    {
        persist_fabric_nat_host_single(OPERATE_ADD, host_p);
    }
}

nat_port_p find_nat_connect(UINT4 external_ip, UINT2 external_port_no, UINT1 proto_type)
{
	// LOG_PROC("INFO", "NAT: Start find NAT connect!");

	nat_host_p host_p = NULL;
	nat_port_p port_p = NULL;
	nat_port_p head_p = NULL;

	// judge the external_port_no range
	if ((NAT_PORT_MIN_VALUE > external_port_no) || (NAT_PORT_MAX_VALUE < external_port_no)) {
		// LOG_PROC("INFO", "NAT: the external port no is out of range");
		return NULL;
	}

	host_p = find_nat_host_by_ip(external_ip);

	if (NULL != host_p) {
		if (IPPROTO_TCP == proto_type) {
			head_p = host_p->tcp_port_list;
		}
		else if (IPPROTO_UDP == proto_type) {
			head_p = host_p->udp_port_list;
		}
		else {
			return NULL;
		}

		port_p = find_nat_port_by_port_no(external_port_no, head_p);
	}

	return port_p;
	// LOG_PROC("INFO", "NAT: Success find NAT connect!");
}


UINT4 get_nat_connect_count_by_ip(UINT4 internal_ip, UINT2* port_list, UINT4* externalip_list, UINT2* proto_list)
{
	// LOG_PROC("INFO", "NAT: Start find NAT connect!");
	nat_host_p host_p = NULL;
	nat_port_p head_p = NULL;
	UINT4 port_number = 0;

	host_p = g_nat_host_list_head;

	while (NULL != host_p) {
		head_p = host_p->tcp_port_list;
		port_number = find_nat_port_by_internal_ip(internal_ip, port_list, externalip_list, proto_list, host_p->host_ip, port_number, IPPROTO_TCP, head_p);

		head_p = host_p->udp_port_list;
		port_number = find_nat_port_by_internal_ip(internal_ip, port_list, externalip_list, proto_list, host_p->host_ip, port_number, IPPROTO_UDP, head_p);

		host_p = host_p->next;
	}

	return port_number;
}


// 处理NAT相关的包
INT4 fabric_openstack_ip_nat_comute_foward(gn_switch_t *sw, packet_in_info_t *packet_in, UINT1 from_inside, param_set_p param_set)
{
	// LOG_PROC("INFO", "NAT: Start fabric_openstack_ip_nat_handle");
	// printf("%s\n", FN);
	// 取得packet_in的信息
	if (NULL == packet_in) {
		// output error log "packet in is NULL!"
		LOG_PROC("INFO", "NAT: Handle IP NAT packet failed! packet is NULL!");
		return IP_DROP;
	}

	// 取得需要的数据
	ip_t* packetin_ip = (ip_t*)(packet_in->data);
	tcp_t* packetin_tcp = NULL;
	udp_t* packetin_udp = NULL;

	UINT4 packetin_src_ip = packetin_ip->src;
	UINT4 packetin_dst_ip = packetin_ip->dest;
	UINT2 packetin_src_port = 0;
	UINT2 packetin_dst_port = 0;
	UINT1 packetin_proto_type;
	UINT4 packetin_inport = packet_in->inport;

	UINT1 packetin_src_mac[6];
	memcpy(packetin_src_mac, packetin_ip->eth_head.src, 6);

	gn_switch_t* src_sw = sw;
	gn_switch_t* gateway_sw = NULL;
	external_port_p export_p = NULL;

	// 判断TCP/UDP
	if (IPPROTO_TCP == packetin_ip->proto) {
		packetin_tcp = (tcp_t*)(packetin_ip->data);
		packetin_proto_type = IPPROTO_TCP;
		packetin_src_port = packetin_tcp->sport;
		packetin_dst_port = packetin_tcp->dport;
	}
	else if (IPPROTO_UDP == packetin_ip->proto)
	{
		packetin_udp = (udp_t*)(packetin_ip->data);
		packetin_proto_type = IPPROTO_UDP;
		packetin_src_port = packetin_udp->sport;
		packetin_dst_port = packetin_udp->dport;
	}
	else {
		return IP_DROP;
	}

	// 取得外部网关
	export_p = get_external_port_by_host_mac(packetin_src_mac);
	if (NULL == export_p) {
		// 输出错误信息, 取得网关信息失败
		LOG_PROC("INFO", "fabric_openstack_ip_nat_handle: fail to get external port!");
		return IP_DROP;
	}

	// 取得外部网关交换机
	gateway_sw = find_sw_by_dpid(export_p->external_dpid);

	if (NULL == gateway_sw) {
		LOG_PROC("INFO", "NAT: Fail to get external gateway Fail!");
		return IP_DROP;
	}

	// define external port number
	UINT2 external_port_no = 0;

	if (TRUE == from_inside) {
		// 调用create_nat_connect, 返回值为0是表示创建失败, 其他则为port_no
		external_port_no = create_nat_connect(packetin_src_ip, packetin_dst_ip,
				packetin_src_port, packetin_proto_type, packetin_src_mac, export_p->external_dpid, sw->dpid);
	}
	else {
		// judge nat connect exist
		nat_port_p port_p = NULL;
		port_p = find_nat_connect(packetin_src_ip, ntohs(packetin_dst_port), packetin_proto_type);

		// if exist
		if (NULL != port_p) {
			memcpy(packetin_src_mac, port_p->internal_mac, 6);
			packetin_src_ip = port_p->internal_ip;
			packetin_dst_ip = packetin_ip->src;
			packetin_src_port = port_p->internal_port_no;
			packetin_dst_port = ntohs(packetin_src_port);
			external_port_no = port_p->external_port_no;
			p_fabric_host_node openstack_p = get_fabric_host_from_list_by_mac(packetin_src_mac);

			if (NULL == openstack_p) {
				LOG_PROC("INFO", "openstack port not exist");
				return IP_DROP;
			}

			packetin_inport = openstack_p->port;
			src_sw = openstack_p->sw;

			if ((NULL == src_sw) || (0 == packetin_inport))
			{
				p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(openstack_p);
				fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], openstack_p->ip_list[0], gateway_p->mac);
				return IP_DROP;
			}

			gateway_sw = find_sw_by_dpid(export_p->external_dpid);

			if (NULL == gateway_sw) {
				LOG_PROC("INFO", "NAT: Fail to get external gateway!");
				return IP_DROP;
			}

			sw = gateway_sw;
			port_p->src_dpid = src_sw->dpid;
		}
	}

	// get vlan id
	UINT4 src_vlan_vid = of131_fabric_impl_get_tag_sw(src_sw);
	UINT4 gateway_vlan_vid = of131_fabric_impl_get_tag_sw(gateway_sw);

	// LOG_PROC("INFO", "NAT: create nat port id %d", external_port_no);

	// 如果创建成功
	if (0 != external_port_no) {

		// 从主机所在交换机到网关的流表
//		install_fabric_nat_from_inside_flow(packetin_src_ip, packetin_dst_ip, packetin_src_port, packetin_proto_type,
//				packetin_src_mac, export_p->external_outer_interface_ip, export_p->external_gateway_mac, export_p->external_outer_interface_mac,
//				external_port_no, gateway_vlan_vid, src_vlan_vid, export_p->external_port, src_sw, gateway_sw);

		param_set->src_ip = packetin_src_ip;
		param_set->dst_ip = packetin_dst_ip;
		param_set->src_port_no = packetin_src_port;
		param_set->proto = packetin_proto_type;
		memcpy(param_set->src_mac, packetin_src_mac, 6);
		param_set->outer_ip = export_p->external_outer_interface_ip;
		memcpy(param_set->outer_gateway_mac, export_p->external_gateway_mac, 6);
		memcpy(param_set->outer_mac, export_p->external_outer_interface_mac, 6);
		param_set->dst_port_no = external_port_no;
		param_set->dst_vlanid = gateway_vlan_vid;
		param_set->src_vlanid = src_vlan_vid;
		param_set->dst_inport = export_p->external_port;
		param_set->src_sw = src_sw;
		param_set->dst_sw = gateway_sw;
		param_set->src_inport = packetin_inport;


		// 不使用物理交换机
		if (0 == get_nat_physical_switch_flag())
		{
			// 从网关到主机所在交换机的流表
//			install_fabric_nat_from_external_flow(packetin_src_ip, packetin_dst_ip, packetin_src_port, packetin_proto_type,
//					packetin_src_mac, export_p->external_outer_interface_ip, export_p->external_gateway_mac, export_p->external_outer_interface_mac,
//					external_port_no, gateway_vlan_vid, src_vlan_vid, export_p->external_port, src_sw, gateway_sw);

			// 主机所在交换机的流表
			// install_fabric_output_flow(src_sw, packetin_src_mac, packetin_inport);
			// param_set->flow_type = Nat_ip_flow;
		}
		// 使用物理交换机
		else
		{
			// get the output port from gateway to src
			UINT4 gateway_to_src_port_no = get_out_port_between_switch(gateway_sw->dpid, src_sw->dpid);

			if (0 == gateway_to_src_port_no) {
				LOG_PROC("ERROR", "NAT: Can't find the output port from gateway switch to source switch!");
				return IP_DROP;
			}
			else {
				// 从网关到主机所在交换机的流表
//				install_fabric_nat_from_external_fabric_flow(packetin_src_ip, packetin_dst_ip, packetin_src_port, packetin_proto_type,
//						packetin_src_mac, export_p->external_outer_interface_ip, export_p->external_gateway_mac, export_p->external_outer_interface_mac,
//						external_port_no, gateway_vlan_vid, src_vlan_vid, export_p->external_port, src_sw, gateway_sw, gateway_to_src_port_no);

				// 主机所在交换机的流表
//				install_fabric_nat_from_external_fabric_host_flow(packetin_src_ip, packetin_dst_ip, packetin_src_port, packetin_proto_type,
//						packetin_src_mac, export_p->external_outer_interface_ip, export_p->external_gateway_mac, export_p->external_outer_interface_mac,
//						external_port_no, gateway_vlan_vid, src_vlan_vid, packetin_inport, src_sw, gateway_sw);

				param_set->dst_gateway_output = gateway_to_src_port_no;
				// param_set->flow_type = Nat_ip_flow;
			}

		}

		// 重新处理第一个包
		// fabric_openstack_nat_packet_output(sw, packet_in, OFPP13_TABLE);
		return Nat_ip_flow;
	}
	// 如果创建失败
	else {

		// output error log "create NAT tuple node failed!"
//		LOG_PROC("INFO", "NAT: Handle IP NAT packet failed! create NAT node failed!");
//		nat_host_p host_p = g_nat_host_list_head;
//		while (NULL != host_p) {
//			nat_show_ip(host_p->host_ip);
//			nat_port_p tcp_p = host_p->tcp_port_list;
//			nat_port_p udp_p = host_p->udp_port_list;
//			while (NULL != tcp_p) {
//				printf("tcp:%d ", tcp_p->external_port_no);
//				tcp_p = tcp_p->next;
//			}
//			printf("\n");
//			while (NULL != udp_p) {
//				printf("udp:%d ", udp_p->external_port_no);
//				udp_p = udp_p->next;
//			}
//			printf("\n");
//			host_p = host_p->next;
//		}

		return IP_DROP;
	}
}

// deal with packet out action
//void fabric_openstack_nat_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport)
//{
//	// LOG_PROC("INFO", "NAT: call function fabric_openstack_nat_packet_output");
//	packout_req_info_t pakout_req;
//	pakout_req.buffer_id = packet_in_info->buffer_id;
//	pakout_req.inport = OFPP13_CONTROLLER;
//	pakout_req.outport = outport;
//	pakout_req.max_len = 0xff;
//	pakout_req.xid = packet_in_info->xid;
//	pakout_req.data_len = packet_in_info->data_len;
//	pakout_req.data = packet_in_info->data;
//	sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
//};

INT4 fabric_openstack_nat_icmp_comute_foward(gn_switch_t *sw, packet_in_info_t *packet_in, UINT1 from_inside, param_set_p param_set)
{
	// LOG_PROC("INFO", "NAT: call function fabric_openstack_nat_icmp_handler");
	// printf("%s\n", FN);
	nat_icmp_iden_p nat_icmp_p = NULL;
	external_port_p epp = NULL;

	ip_t* ipt = (ip_t*)(packet_in->data);
	if (NULL == ipt) {
		LOG_PROC("INFO", "NAT: icmp: no ip data");
		return IP_DROP;
	}

	// get icmp data
	icmp_t* packetin_icmp = (icmp_t*)(ipt->data);
	nat_icmp_p = get_nat_icmp_iden_by_identifier(packetin_icmp->id);

	// from indise to outside
	if (TRUE == from_inside) {
		// get external port info
		epp = get_external_port_by_host_mac(ipt->eth_head.src);
		if(NULL == epp) {
			 LOG_PROC("INFO", "NAT: icmp: no valid external port!");
			 return IP_DROP;
		}
		// get gateway info
		gn_switch_t *gateway_sw = find_sw_by_dpid(epp->external_dpid);
		if (NULL == gateway_sw) {
			LOG_PROC("INFO", "NAT: icmp: gateway switch is NULL!");
			return IP_DROP;
		}

		// judge nat_icmp create
		if (NULL == nat_icmp_p) {
			// if not created, create nat_icmp
			nat_icmp_p = create_nat_imcp_iden_p(packetin_icmp->id, ipt->src, ipt->eth_head.src, sw->dpid, packet_in->inport);
		}

		if (NULL == nat_icmp_p) {
			return IP_DROP;
		}
		// modify src_ip, src_mac, dst_mac
		ipt->src = epp->external_outer_interface_ip;
		memcpy(ipt->eth_head.src, epp->external_outer_interface_mac, 6);
		memcpy(ipt->eth_head.dest,epp->external_gateway_mac, 6);

		// calculate checksum
		ipt->cksum = 0;
		ipt->cksum = calc_ip_checksum((UINT2*)&ipt->hlen, 20);

		// packet out data to external
		param_set->dst_sw = gateway_sw;
		param_set->dst_inport = epp->external_port;
		return CONTROLLER_FORWARD;
	}
	// if from external
	else {
		// judge nat_icmp create
		if (NULL == nat_icmp_p) {
			// do nothing
			// LOG_PROC("INFO", "NAT ICMP not created!");
			return IP_DROP;
		}
		else {
			// modify dst_ip, dst_mac
			ipt->dest = nat_icmp_p->host_ip;
			memcpy(ipt->eth_head.dest, nat_icmp_p->host_mac, 6);

			// calculate checksum
			ipt->cksum = 0;
			ipt->cksum = calc_ip_checksum((UINT2*)&ipt->hlen,20);

			gn_switch_t* output_sw = find_sw_by_dpid(nat_icmp_p->sw_dpid);
			if (NULL == output_sw) {
				LOG_PROC("INFO", "NAT: output switch is NULL");
				return IP_DROP;
			}

			// packet out the data to the host
			// fabric_openstack_nat_packet_output(output_sw, packet_in, nat_icmp_p->inport);
			p_fabric_host_node host = get_fabric_host_from_list_by_mac(nat_icmp_p->host_mac);

			if (NULL == host) {
				return IP_DROP;
			}

			if ((NULL == host->sw) || (0 == host->port))
			{
				p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(host);
				fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], host->ip_list[0], gateway_p->mac);
				return IP_DROP;
			}

			param_set->dst_sw = host->sw;
			param_set->dst_inport = host->port;

//			param_set->dst_sw = output_sw;
//			param_set->dst_inport = nat_icmp_p->inport;
			return CONTROLLER_FORWARD;
		}
	}
}

// update natphysical switch flag
void update_nat_physical_switch_flag(UINT1 flag)
{
	if (flag == g_nat_physical_switch_flag)
	{
		// do nothing
	}
	else {
		LOG_PROC("INFO", "NAT: configure: use_physical_switch_modify_nat: %d", flag);
		g_nat_physical_switch_flag = flag;
		set_value_int(g_controller_configure, "[openvstack_conf]", "use_physical_switch_modify_nat", flag);
		g_controller_configure = save_ini(g_controller_configure, CONFIGURE_FILE);
	}
}

// get nat physical switch flag
UINT1 get_nat_physical_switch_flag()
{
	return g_nat_physical_switch_flag;
}


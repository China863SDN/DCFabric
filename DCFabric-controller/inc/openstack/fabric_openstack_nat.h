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
 * fabric_openstack_arp.h
 *
 *  Created on: September 9th, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */

#ifndef INC_FABRIC_FABRIC_OPENSTACK_NAT_H_
#define INC_FABRIC_FABRIC_OPENSTACK_NAT_H_

#include "common.h"
#include "gnflush-types.h"
#include "fabric_openstack_external.h"
#include "openstack_host.h"
#include "forward-mgr.h"

/*
 * 定义结构体nat_port
 * 这个结构体中用来记录某个外部IP对应的连接
 * 以external_port_no为Key值有序排列
 * external_port_no是在nat创建的时候动态创建的ID值,保证唯一性
 * 该结构体为带头结点的单链表,首项存储的是当前所有连接数
 */
typedef struct _nat_port {
	UINT2 external_port_no;				///< 转换后的端口号
	UINT4 internal_ip;					///< 内部IP
	UINT2 internal_port_no;				///< 内部端口号
	UINT1 internal_mac[6];				///< 内部mac
	UINT8 gateway_dpid;					///< 网关的交换机的dpid
	UINT8 src_dpid;						///< src dpid
	struct _nat_port* next;				///< 指向下一个结点的指针
} nat_port, *nat_port_p;

/*
 * 定义结构体nat_host
 * 用来记录某个外部IP的连接
 * tcp_port_list/udp_port_list是外部IP地址对应的tcp/udp连接端口列表
 * 这两个list均以external_port_no为Key值,有序排列
 * 两个list均为带头结点的单链表,首项存储为当前IP数
 */
typedef struct _nat_host {
	UINT4 host_ip;						///< 外部IP地址
	nat_port_p tcp_port_list;			///< TCP端口List
	nat_port_p udp_port_list;			///< UDP端口list
	struct _nat_host* next;				///< 指向下一个结点的指针
} nat_host, *nat_host_p;

extern nat_host_p g_nat_host_list_head;

/*
 * 初始化,分配内存池
 */
void init_nat_mem_pool();

/*
 * 销毁内存池
 */
void destroy_nat_mem_pool();

///< 定义NAT host相关的函数
/*
 * 创建nat_host头结点
 * @return nat_host_p 	: 创建的结点(如果为空,表示创建失败)
 */
nat_host_p init_nat_host();

/*
 * 创建nat_host条目
 * @param host_ip 		: 外部地址
 * @return nat_host_p 	: 创建的结点(如果为空,表示创建失败)
 */
nat_host_p create_nat_host(UINT4 host_ip);

/*
 * 删除指定外部地址的nat_host条目
 * @param host_ip 		: 外部地址
 */
void remove_nat_host_by_ip(UINT4 host_ip);

/*
 * 查找指定外部地址的nat_host条目
 * @param host_ip 		: 外部地址
 * @return nat_host_p 	: 查找到的结点(如果为空,表示查找失败)
 */
nat_host_p find_nat_host_by_ip(UINT4 host_ip);

///< 定义NAT port相关的函数
/*
 * 创建nat_port头结点
 * @return nat_port_p 	: 创建的结点(如果为空,表示创建失败)
 */
nat_port_p init_nat_port();

/*
 * 创建nat_port条目
 * @param internal_ip 			: 内部IP
 * @param internal_port_no 		: 内部端口号
 * @param internal_mac 			: 内部mac
 * @param external_gateway_dpid : 网关的交换机的dpid
 * @param port_head_p			: 当前list的头结点地址
 * @return nat_port_p 			: 当前list的头结点
 */
nat_port_p create_nat_port(UINT4 internal_ip, UINT2 internal_port_no,
		UINT1* internal_mac, UINT8 external_gateway_dpid, nat_port_p port_head_p);

/*
 * 删除指定List所有nat_port条目
 * @param head_p 			: List头结点的地址
 */
void remove_nat_all_port(nat_port_p head_p);

/*
 * 根据port id删除条目
 * @param external_port_no		: port id
 * @param head_p 				: List头结点的地址
 */
void remove_nat_port_by_port_no(UINT2 external_port_no, nat_port_p head_p);

/*
 * 根据port id查找条目
 * @param external_port_no			: port id
 * @param head_p 					: List头结点的地址
 */
nat_port_p find_nat_port_by_port_no(UINT2 external_port_no, nat_port_p head_p);

/*
 * 根据ip信息查找条目
 * @param internal_ip 			: 内部IP,内部网络中的源IP
 * @param internal_port 		: 内部端口号
 * @param head_p 				: List头结点的地址
 */
nat_port_p find_nat_port_by_ip_and_port(UINT4 internal_ip, UINT2 internal_port_no, nat_port_p head_p);

/*
 * 根据ip信息查找条目
 * @param internal_mac			: 内部IP,内部网络中的源MAC
 * @param internal_port 		: 内部端口号
 * @param head_p 				: List头结点的地址
 */
nat_port_p find_nat_port_by_mac_and_port(UINT1* internal_mac, UINT2 internal_port_no, nat_port_p head_p);

/*
 * 创建NAT条目
 * @param internal_ip 		: 内部IP,内部网络中的源IP
 * @param host_ip 			: 外部IP,外部网络中的目标IP
 * @param internal_ip 		: 内部端口号(现在只在TCP/UDP协议时中有效)
 * @param proto_type  		: 协议类型
 * @param internal_mac		: MAC
 * @param gateway_dpid		: 网关dpid
 *
 * @return 创建的NAT映射条目(如果为空,表示创建失败)
 */
UINT2 create_nat_connect(UINT4 internal_ip, UINT4 host_ip, UINT2 internal_port_no,
		UINT1 proto_type, UINT1* internal_mac, UINT8 gateway_dpid, UINT8 src_dpid);

/*
 * 根据{外部IP,转换后的端口号,协议类型}查找并且删除该条目
 * @param host_ip 			: 外部IP,外部网络中的目标IP
 * @param external_port_no	: 转换后的端口号
 * @param proto_type  		: 协议类型
 */
void destroy_nat_connect(UINT4 host_ip, UINT2 external_port_no, UINT1 proto_type);

/*
 * 根据{外部IP,转换后的端口号,协议类型}查找并且删除该条目
 * @param host_ip 			: 外部IP,外部网络中的目标IP
 * @param external_port_no	: 转换后的端口号
 * @param proto_type  		: 协议类型
 */
void destroy_nat_connect_by_mac_and_port(gn_switch_t* sw, UINT4 host_ip, UINT1* internal_mac, UINT2 internal_port_no, UINT1 proto_type);

/*
 * 根据{外部IP,转换后的端口号,协议类型}查找条目
 * @param host_ip 			: 外部IP,外部网络中的目标IP
 * @param external_port_no	: 转换后的端口号
 * @param proto_type  		: 协议类型
 *
 * @return 查找的NAT映射条目(如果为空,表示查找失败)
 */
nat_port_p find_nat_connect(UINT4 external_ip, UINT2 external_port_no, UINT1 proto_type);

/*
 * 处理NAT相关的包
 * @param sw 				: 指定的交换机
 * @param packet_in			: 指定的包
 * @param from_inside		: TRUE:从内网到外网; FALSE:从外网到内网
 */
INT4 fabric_openstack_ip_nat_comute_foward(gn_switch_t *sw, packet_in_info_t *packet_in, UINT1 from_inside, param_set_p param_set);

/*
 * 处理ouput包
 * @param sw 				: 指定的交换机
 * @param packet_in			: 指定的包
 * @param outport			: 指定的port
 */
// void fabric_openstack_nat_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info, UINT4 outport);

/*
 * 处理NAT icmp相关的包
 * @param sw 				: 指定的交换机
 * @param packet_in			: 指定的包
 * @param from_inside		: TRUE:从内网到外网; FALSE:从外网到内网
 */
INT4 fabric_openstack_nat_icmp_comute_foward(gn_switch_t *sw, packet_in_info_t *packet_in, UINT1 from_inside, param_set_p param_set);

/*
 * 设置是否使用物理交换机的flag
 */
void update_nat_physical_switch_flag(UINT1 flag);

/*
 * 取得是否使用物理交换机的flag
 */
UINT1 get_nat_physical_switch_flag();


UINT4 get_nat_connect_count_by_ip(UINT4 internal_ip, UINT2* port_list, UINT4* externalip_list, UINT2* proto_list);

#endif /* INC_FABRIC_FABRIC_OPENSTACK_NAT_H_ */

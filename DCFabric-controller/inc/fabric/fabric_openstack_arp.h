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
 *  Created on: Jun 19, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */

#ifndef INC_FABRIC_FABRIC_OPENSTACK_ARP_H_
#define INC_FABRIC_FABRIC_OPENSTACK_ARP_H_
#include "gnflush-types.h"
#include "fabric_host.h"
#include "forward-mgr.h"
// void fabric_openstack_arp_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
//void fabric_openstack_ip_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
p_fabric_host_node openstack_save_host_info(gn_switch_t *sw,UINT1* sendmac,UINT4 sendip,UINT4 inport);
p_fabric_host_node openstack_find_dst_port(p_fabric_host_node src_node,UINT4 targetip);
p_fabric_host_node openstack_find_ip_dst_port(p_fabric_host_node src_node,UINT4 targetip);
INT4 openstack_arp_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
INT4 openstack_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
INT4 openstack_arp_remove_ip_from_flood_list(UINT4 sendip);
INT4 openstack_arp_reply_output(p_fabric_host_node src,p_fabric_host_node dst,UINT4 targetIP, packet_in_info_t *packet_in);
INT4 openstack_ip_p_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,UINT1* src_mac,packet_in_info_t *packet_in);
INT4 openstack_ip_packet_output(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
INT4 openstack_ip_p_install_flow(param_set_p param_set, INT4 foward_type);
void fabric_opnestack_create_arp_flood(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac);
INT4 openstack_ip_packet_compute_src_dst_forward(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in, param_set_p param_set);
INT4 openstack_ip_p_broadcast(packet_in_info_t *packet_in);
INT4 openstack_ip_packet_check_access(p_fabric_host_node src_port, p_fabric_host_node dst_port, packet_in_info_t *packet_in, param_set_p param);
void remove_flows_by_sw_port(UINT8 sw_dpid, UINT4 port);
void fabric_openstack_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport);
void fabric_opnestack_create_arp_request(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, gn_switch_t* sw, UINT4 outPort);

INT4 openstack_ip_install_deny_flow(gn_switch_t* sw, ip_t* ip);
INT4 openstack_ip_remove_deny_flow(UINT1* src_mac);

/*
 * temp added for ipv6
 * by lxf@2016.1.11
 */
#if 1
p_fabric_host_node openstack_find_ip_dst_port_ipv6(p_fabric_host_node src_node,UINT1* targetip);
p_fabric_host_node openstack_save_host_info_ipv6(gn_switch_t *sw,UINT1* sendmac,UINT1* sendip,UINT4 inport);
void fabric_openstack_install_fabric_flows_ipv6(p_fabric_host_node src_port,p_fabric_host_node dst_port,
										   security_param_p src_security, security_param_p dst_security);
#endif
#endif /* INC_FABRIC_FABRIC_OPENSTACK_ARP_H_ */

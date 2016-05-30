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
 * fabric_arp.h
 *
 *  Created on: Apr 2, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#include "fabric_host.h"
#include "forward-mgr.h"

#ifndef INC_FABRIC_FABRIC_ARP_H_
#define INC_FABRIC_FABRIC_ARP_H_

p_fabric_host_node fabric_save_host_info(gn_switch_t *sw,UINT1* sendmac,UINT4 sendip,UINT4 inport);
p_fabric_host_node fabric_find_dst_port(p_fabric_host_node src_node,UINT4 targetip);
INT4 fabric_arp_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
INT4 fabric_arp_reply(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
INT4 fabric_arp_remove_ip_from_flood_list(UINT4 sendip);
INT4 fabric_ip_p_flood(p_fabric_host_node src_port,UINT4 sendip,UINT4 targetip,UINT1* srcmac,packet_in_info_t *packet_in);
INT4 fabric_ip_p_install_flow(param_set_p param, INT4 forward_type);
INT4 fabric_ip_packet_output(p_fabric_host_node src_port,p_fabric_host_node dst_port,UINT4 sendip,UINT4 targetip,packet_in_info_t *packet_in);
INT4 fabric_arp_reply_output(p_fabric_host_node src,p_fabric_host_node dst,UINT4 targetIP, packet_in_info_t *packet_in);
//void fabric_arp_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
INT4 fabric_ip_packet_check_access(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in, param_set_p param_set);
INT4 fabric_compute_src_dst_forward(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in, param_set_p param_set);
void fabric_ip_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
void fabric_vlan_handle(gn_switch_t *sw, packet_in_info_t *packet_in);

void fabric_push_arp_flood_queue(UINT4 targetIP,packet_in_info_t *packet_in);
void fabric_push_flow_queue(p_fabric_host_node src,UINT4 src_IP,p_fabric_host_node dst,UINT4 dst_IP);
void fabric_create_arp_reply(p_fabric_host_node src,p_fabric_host_node dst,packet_in_info_t *packet_in_info);
void fabric_packet_output(gn_switch_t *sw, packet_in_info_t *packet_in_info,UINT4 outport);
p_fabric_host_node fabric_find_dst_port_ip(p_fabric_host_node src_node,UINT4 targetip);

INT4 fabric_ip_install_deny_flow(gn_switch_t *sw, ip_t* ip);
INT4 fabric_ip_remove_deny_flow(UINT1* src_mac);

#endif /* INC_FABRIC_FABRIC_ARP_H_ */

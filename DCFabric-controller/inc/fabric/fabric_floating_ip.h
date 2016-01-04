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
 * fabric_floating_ip.h
 *
 *  Created on: Sep 8, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: Sep 8, 2015
 */

#ifndef INC_FABRIC_FABRIC_FLOATING_IP_H_
#define INC_FABRIC_FABRIC_FLOATING_IP_H_

#include "gnflush-types.h"
#include "fabric_openstack_external.h"
#include "openstack_host.h"
#include "fabric_host.h"
#include "forward-mgr.h"

INT4 fabric_openstack_floating_ip_packet_out_handle(p_fabric_host_node src_port, packet_in_info_t *packet_in, external_floating_ip_p fip, param_set_p param_set);
//void fabric_openstack_floating_ip_packet_in_handle(external_port_p epp, packet_in_info_t *packet_in, external_floating_ip_p fip);
//void fabric_openstack_floating_ip_arp_request_handle(gn_switch_t *sw, external_floating_ip_p fip, packet_in_info_t *packet_in);
//void fabric_openstack_floating_ip_arp_reply_handle(gn_switch_t *sw, external_floating_ip_p fip, packet_in_info_t *packet_in);
//void fabric_opnestack_floating_flood_inside(UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT8 ext_dpid);
//void fabric_openstack_packet_flood_inside(packet_in_info_t *packet_in_info, UINT8 ext_dpid);
void init_floating_mgr();
void floating_tx_timer(void *para, void *tid);

#endif /* INC_FABRIC_FABRIC_FLOATING_IP_H_ */

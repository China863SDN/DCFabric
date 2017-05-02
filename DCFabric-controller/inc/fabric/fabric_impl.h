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
 * fabric_impl.h
 *
 *  Created on: Apr 3, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */

#ifndef INC_FABRIC_FABRIC_IMPL_H_
#define INC_FABRIC_FABRIC_IMPL_H_

//#include "gnflush-types.h"
#include "fabric_path.h"

#define FABRIC_START_TAG 4
extern t_fabric_sw_list g_fabric_sw_list;

void of131_fabric_impl_setup_by_dpids(UINT8* dpids,UINT4 len);
void of131_fabric_impl_setup();
void of131_fabric_impl_delete();

void of131_fabric_impl_set_dpids(UINT8* dpids, UINT4 len);
void of131_fabric_impl_unset_dpids();

void of131_fabric_impl_remove_sws(gn_switch_t** swList, UINT4 num);
void of131_fabric_impl_add_sws(gn_switch_t** swList, UINT4 num);

UINT4 of131_fabric_impl_get_tag_sw(gn_switch_t *sw);
UINT4 of131_fabric_impl_get_tag_dpid(UINT8 dpid);
gn_switch_t* of131_fabric_impl_get_sw_tag(UINT4 tag);
UINT8 of131_fabric_impl_get_dpid_tag(UINT4 tag);

UINT1 get_fabric_state();

p_fabric_path of131_fabric_get_path(UINT8 src_dpid,UINT8 dst_dpid);

UINT4 get_out_port_between_switch(UINT8 src_dpid, UINT8 dst_dpid);

/*
 * get port number between switch and ip
 *
 * @brief: this function is used to get port between switch and host ip
 *
 * @param: sw			the source switch
 * @param: dst_ip			the host ip
 *
 * @return: UINT4			0: fail; other: port number
 */
UINT4 get_port_no_between_sw_ip(gn_switch_t* sw, UINT4 dst_ip);

void of131_test_update();

p_fabric_sw_node get_fabric_sw_node_by_dpid(UINT8 dpid);
UINT4 adjust_fabric_sw_node_list(UINT8 pre_dpid, UINT8 dpid);

#endif /* INC_FABRIC_FABRIC_IMPL_H_ */

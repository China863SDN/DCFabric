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
 *   File Name   : group-mgr.h           *
 *   Author      : greenet Administrator           *
 *   Create Date : 2015-3-10           *
 *   Version     : 1.0           *
 *   Function    : .           *
 *                                                                             *
 ******************************************************************************/

#ifndef GROUP_MGR_H_
#define GROUP_MGR_H_

#include "gnflush-types.h"

INT4 add_group_entry(gn_switch_t *sw, gn_group_t *group);
INT4 modify_group_entry(gn_switch_t *sw, gn_group_t *group);
INT4 delete_group_entry(gn_switch_t *sw, gn_group_t *group);
gn_group_t *find_group_by_id(gn_switch_t *sw, UINT4 group_id);

void clear_group_entries(gn_switch_t *sw);
void gn_group_free(gn_group_t *group);

group_bucket_t* create_lbaas_group_bucket(UINT4 between_portno, UINT1* host_mac, UINT4 host_ip, UINT2 host_tcp_dst,
            UINT2 host_vlan_id, UINT2 weight);


INT4 init_group_mgr();
void fini_group_mgr();
#endif /* GROUP_MGR_H_ */

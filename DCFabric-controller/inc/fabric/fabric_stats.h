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
 * fabric_state.h
 *
 *  Created on: Jan 18, 2016
 *  Author: BNC zgzhao
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 */

#ifndef INC_FABRIC_FABRIC_STATE_H_
#define INC_FABRIC_FABRIC_STATE_H_
#include "gnflush-types.h"


typedef struct stats_fabric_flow {
    gn_switch_t *sw;
    gn_flow_t *flow;

    struct stats_fabric_flow *next;
} stats_fabric_flow_t;


//interface
void init_fabric_stats();
void fini_fabric_stats();
stats_fabric_flow_t *query_fabric_all_flow_entries();
stats_fabric_flow_t *query_fabric_flow_entries_by_switch(gn_switch_t *sw, UINT4 group, UINT4 port, UINT1 id);
void update_fabric_flow_entries(gn_switch_t *sw, UINT1 *entry, UINT2 length, UINT2 flag, UINT4 xid);
void clear_fabric_stats();


#endif

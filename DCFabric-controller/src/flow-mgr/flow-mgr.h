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
*   File Name   : flow-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-2           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef FLOW_MGR_H_
#define FLOW_MGR_H_

#include "gnflush-types.h"

extern void *g_gnflow_mempool_id;
extern void *g_gninstruction_mempool_id;
extern void *g_gnaction_mempool_id;

//根据uuid查找流表
gn_flow_t * find_flow_entry_by_id(gn_switch_t *sw, gn_flow_t *flow);

//根据流表信息查找流表
gn_flow_t * find_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//新增下发一条流表
INT4 add_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

INT4 clean_flow_entry(gn_switch_t *sw, gn_flow_t *flow);


INT4 clean_flow_entries(gn_switch_t *sw);

//修改一条流表
INT4 modify_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//使流表生效(下发到交换机)
INT4 enable_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//是流表失效(从交换机上删除,控制器上保留)
INT4 disable_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//删除一条流表(从交换机和控制器上删除)
INT4 delete_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//清空所有流表(从交换机和控制器上删除所有流表)
INT4 clear_flow_entries(gn_switch_t *sw);

//下发table_miss流表
INT4 flow_mod_table_miss(gn_switch_t *sw);

//流表超时
INT4 flow_entry_timeout(gn_switch_t *sw, gn_flow_t *flow);

//释放flow_entry内存空间
void gn_flow_free(gn_flow_t *flow);

INT4 init_flow_mgr();

//compare the match
BOOL match_compare_strict(const gn_match_t *src_match, const gn_match_t *target_match);
#endif /* FLOW_MGR_H_ */

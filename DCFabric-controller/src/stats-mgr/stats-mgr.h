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
*   File Name   : stats-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef STATS_MGR_H_
#define STATS_MGR_H_

#include "gnflush-types.h"

#define MAX_RECORED 10

extern UINT4 g_switch_bandwidth;
extern UINT4 g_stats_mgr_interval;

void of10_proc_port_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts);
void of13_proc_port_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts);

void of10_proc_flow_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts);
void of13_proc_flow_stats(gn_switch_t *sw, UINT1 *stats, UINT2 length);

INT4 init_stats_mgr();
void fini_stats_mgr();

#endif /* STATS_MGR_H_ */

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
*   File Name   : conn-svr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef CONN_SVR_H_
#define CONN_SVR_H_

#include "msg_handler.h"
#include "gnflush-types.h"

INT4 init_conn_svr();
void fini_conn_svr();
INT4 start_openflow_server();

gn_switch_t *find_sw_by_dpid(UINT8 dpid);
void free_switch(gn_switch_t *sw);
UINT1 *init_sendbuff(gn_switch_t *sw, UINT1 of_version, UINT1 type, UINT2 buf_len, UINT4 transaction_id);
INT4 send_of_msg(gn_switch_t *sw, UINT4 total_len);
gn_switch_t* find_sw_by_port_physical_mac(UINT1* mac);

#endif /* CONN_SVR_H_ */

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
*   File Name   : cluster-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef CLUSTER_MGR_H_
#define CLUSTER_MGR_H_

#include "gnflush-types.h"

#define MAX_CONTROLLER 8

enum CLUSTER_STATE
{
	CLUSTER_STOP = 0,
	CLUSTER_SETUP = 1
};

#pragma pack(1)
typedef struct cluster_node
{
    UINT4 cluster_id;
    INT1 controller_ip[36];
}cluster_node_t;
#pragma pack()

extern UINT1 g_controller_role;
extern UINT8 g_election_generation_id;
extern cluster_node_t g_controller_cluster[];   //控制器集群管理结构
extern UINT8 g_master_id; 

INT4 update_role(UINT4 role);
INT4 get_controller_status(UINT4 tmp_id);
UINT4 set_cluster_onoff(UINT4 onoff);
UINT4 set_cluster_role(UINT8 controller_id);
int update_controllers_role(UINT4 controller_id, UINT1 mode);
INT4 init_cluster_mgr();
void fini_cluster_mgr();

#endif /* CLUSTER_MGR_H_ */

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
*   File Name   : topo-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef TOPO_MGR_H_
#define TOPO_MGR_H_

#include "gnflush-types.h"
#include "forward-mgr.h"

#define NO_PATH -1

//by:yhy 邻接矩阵表示的图
typedef struct adac_matrix
{
    UINT1 *V;              //顶点存储空间
    UINT4 **src_port;      //哪个端口与下一个节点相连//by:yhy ;Time:201612131128;Context:交换机的那个端口与下一交换机相连接.和邻接矩阵相似
    UINT4 **A;             //邻接矩阵
    gn_switch_t ***sw;	   //by:yhy 此处为指向交换机指针的二维数组的头指针
}adac_matrix_t;

extern adac_matrix_t g_adac_matrix;
extern UINT4 **g_short_path;     //v到各顶点的最短路径向量
extern UINT4 **g_short_weight;   //v到各顶点最短路径长度向量

int mapping_new_neighbor(gn_switch_t *src_sw, UINT4 rx_port, UINT8 neighbor_dpid, UINT4 tx_port);
INT4 lldp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info);
INT4 init_topo_mgr();
void lldp_tx(gn_switch_t *sw, UINT4 port_no, UINT1 *src_addr);
void lldp_tx_timer();
#endif /* TOPO_MGR_H_ */

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
*   File Name   : topo-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*   Modify Date : 2015-5-19
*                                                                             *
******************************************************************************/

#include "topo-mgr.h"
#include "gn_inet.h"
#include "timer.h"
#include "../conn-svr/conn-svr.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../event/event_service.h"

void *g_lldp_timerid = NULL;
UINT4 g_lldp_interval = 5;
void *g_lldp_timer = NULL;

void *g_spath_timerid = NULL;
void *g_spath_timer = NULL;

adac_matrix_t g_adac_matrix;
UINT4 **g_short_path;
UINT4 **g_short_weight;

int mapping_new_neighbor(gn_switch_t *src_sw, UINT4 rx_port, UINT8 neighbor_dpid, UINT4 tx_port)
{
    gn_switch_t  *neigh_sw   = NULL;
    int neigh_port_seq = 0;   //neigh's port_no's Sequence
    int own_port_seq = 0;   //our port_no's Sequence

    //通过邻居的dpid找到邻居
    neigh_sw = find_sw_by_dpid(neighbor_dpid);
    if(neigh_sw)
    {
        for(neigh_port_seq=0; neigh_port_seq<neigh_sw->n_ports; neigh_port_seq++)
        {
            //if(neigh_sw->sw_ports[neigh_port_seq])
            {
                if(neigh_sw->ports[neigh_port_seq].port_no == tx_port)

                {
                    break;
                }
            }
        }

        g_adac_matrix.A[src_sw->index][neigh_sw->index] = 1;
        g_adac_matrix.src_port[src_sw->index][neigh_sw->index] = rx_port;
        g_adac_matrix.sw[src_sw->index][neigh_sw->index] = src_sw;
    }
	else {
		return GN_ERR;
	}

    //没有找到邻居端口
    if (neigh_port_seq >= neigh_sw->n_ports)
    {
        return GN_ERR;
    }


    //通过交换机的port_no，找到端口序号
    for(own_port_seq=0; own_port_seq<src_sw->n_ports; own_port_seq++)
    {
        //if(sw->sw_ports[own_port_seq])
        {
            if(src_sw->ports[own_port_seq].port_no == rx_port)
            {
                break;
            }
        }
    }

    //没有找到本交换机端口
    if (own_port_seq >= src_sw->n_ports)
    {
        return GN_ERR;
    }
    

    if (!(src_sw->neighbor[own_port_seq]))
    {
        src_sw->neighbor[own_port_seq] = (neighbor_t *)gn_malloc(sizeof(neighbor_t));
    }

    if (!(neigh_sw->neighbor[neigh_port_seq]))
    {
        neigh_sw->neighbor[neigh_port_seq] = (neighbor_t *)gn_malloc(sizeof(neighbor_t));
    }

    if (neigh_sw->neighbor[neigh_port_seq]->sw != src_sw || src_sw->neighbor[own_port_seq]->sw != neigh_sw)
    {
        event_add_switch_port_on(src_sw,rx_port);
		event_add_switch_port_on(neigh_sw,tx_port);
    }

    src_sw->neighbor[own_port_seq]->sw   = neigh_sw;  //该端口序号连接的sw
    src_sw->neighbor[own_port_seq]->port = &(neigh_sw->ports[neigh_port_seq]); //该端口与哪个端口相连
    neigh_sw->neighbor[neigh_port_seq]->sw   = src_sw;  //该端口序号连接的sw
    neigh_sw->neighbor[neigh_port_seq]->port = &(src_sw->ports[own_port_seq]); //该端口与哪个端口相连

    return 1;
}

INT4 lldp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    UINT8 sender_id;        //neigh's dpid
    UINT4 sender_port;      //neigh's port_no

    lldp_t *pkt = (lldp_t *)packet_in_info->data;
    if (pkt->chassis_tlv_subtype != LLDP_CHASSIS_ID_LOCALLY_ASSIGNED||
        pkt->port_tlv_subtype != LLDP_PORT_ID_COMPONENT ||
        pkt->ttl_tlv_ttl != htons(120))
    {
        return GN_ERR;
    }

    sender_id = gn_ntohll(pkt->chassis_tlv_id);
    sender_port = ntohs(pkt->port_tlv_id);

    return mapping_new_neighbor(sw, packet_in_info->inport, sender_id, sender_port);
}

static void create_lldp_pkt(void *src_addr, UINT8 id, UINT2 port, lldp_t *buffer)
{
    UINT1 dest_addr[6] = {0x01,0x80,0xc2,0x00,0x00,0x0e};

    memcpy(buffer->eth_head.dest,dest_addr,6);
    memcpy(buffer->eth_head.src, src_addr, 6);
    buffer->eth_head.proto = htons(0x88cc);

    buffer->chassis_tlv_type_and_len = htons(0x0209);
    buffer->chassis_tlv_subtype = LLDP_CHASSIS_ID_LOCALLY_ASSIGNED;
    buffer->chassis_tlv_id = gn_htonll(id);   //datapath id

    buffer->port_tlv_type_and_len    = htons(0x0403);
    buffer->port_tlv_subtype = LLDP_PORT_ID_COMPONENT;
    buffer->port_tlv_id = htons(port);         //send port

    buffer->ttl_tlv_type_and_len = htons(0x0602);
    buffer->ttl_tlv_ttl = htons(120);

/*
    buffer->orgspe_tlv_type_and_len = htons(0xfe0c);
    buffer->unique_code[0]          = 0x00;
    buffer->unique_code[1]          = 0x26;
    buffer->unique_code[2]          = 0xe1;
    memcpy(buffer->sub_content, sub_content, 9);

    buffer->unknow1_tlv_type_and_len = htons(0x1808);
    buffer->unknow1_code             = htonll(0x0baa92b47d87cbba);

    buffer->unknow2_tlv_type_and_len = htons(0xe601);
    buffer->unknow2_code             = 0x01;
*/

    buffer->endof_lldpdu_tlv_type_and_len = 0x00;
}

static void lldp_tx(gn_switch_t *sw, UINT4 port_no, UINT1 *src_addr)
{
    packout_req_info_t packout_req_info;
    lldp_t lldp_pkt;

    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = 0xfffffffd;
    packout_req_info.outport = port_no;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = 0;
    packout_req_info.data_len = sizeof(lldp_t);
    packout_req_info.data = (UINT1 *)&lldp_pkt;

    /* create lldp packet */
    create_lldp_pkt(src_addr, sw->dpid, port_no, &lldp_pkt);
    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&packout_req_info);
    }
}

//定时发送lldp
static void lldp_tx_timer(void *para, void *tid)
{
    UINT4 num  = 0;
    UINT4 port = 0;
    gn_switch_t *sw = NULL;

    for(; num < g_server.max_switch; num++)
    {
        if(g_server.switches[num].state)
        {
            sw = &g_server.switches[num];
            if(sw->state == 1)
            {
                for (port=0; port<sw->n_ports; port++)
                {
                    lldp_tx(sw, sw->ports[port].port_no, sw->ports[port].hw_addr);
                }
            }
        }
    }

    return;
}

//Floyd算法
void path_calc(adac_matrix_t *G, UINT4 **path, UINT4 **D, UINT4 n)
{
    UINT4 i,j,k;
    for(i=0;i<n;i++)
    {
        //初始化
        for(j=0;j<n;j++)
        {
            if(G->A[i][j] < 65535)
            {
                path[i][j]=j;
            }
            else
            {
                path[i][j] = NO_PATH;
            }
            D[i][j]=G->A[i][j];
        }
    }

    for(k=0;k<n;k++)
    {
        //进行n次试探
        for(i=0;i<n;i++)
        {
            for(j=0;j<n;j++)
            {
                if(D[i][j]>D[i][k]+D[k][j])
                {
                    D[i][j]=D[i][k]+D[k][j];    //取小者
                    path[i][j]=path[i][k];      //改Vi的后继
                }
            }
        }
    }
}

//定时计算最短路径
static void spath_tx_timer(void *para, void *tid)
{
    path_calc(&g_adac_matrix, g_short_path, g_short_weight, g_server.max_switch);
}

INT4 init_topo_mgr()
{
	UINT4 i, j;

    g_adac_matrix.V = (UINT1 *)gn_malloc(g_server.max_switch * sizeof(UINT1));
    g_adac_matrix.A = (UINT4 **)gn_malloc(g_server.max_switch * sizeof(UINT4 *));
    g_adac_matrix.src_port = (UINT4 **)gn_malloc(g_server.max_switch * sizeof(UINT4 *));
    g_adac_matrix.sw = (gn_switch_t ***)gn_malloc(g_server.max_switch * sizeof(gn_switch_t **));
    g_short_path = (UINT4 **)gn_malloc(g_server.max_switch * sizeof(UINT4 *));
    g_short_weight = (UINT4 **)gn_malloc(g_server.max_switch * sizeof(UINT4 *));

    for(i = 0; i < g_server.max_switch; i++)
    {
        g_adac_matrix.A[i] = (UINT4 *)gn_malloc(g_server.max_switch * sizeof(UINT4));
        g_adac_matrix.src_port[i] = (UINT4 *)gn_malloc(g_server.max_switch * sizeof(UINT4));
        g_adac_matrix.sw[i] = (gn_switch_t **)gn_malloc(g_server.max_switch * sizeof(gn_switch_t *));
        g_short_path[i] = (UINT4 *)gn_malloc(g_server.max_switch * sizeof(UINT4));
        g_short_weight[i] = (UINT4 *)gn_malloc(g_server.max_switch * sizeof(UINT4));
    }

    for(i=0; i < g_server.max_switch; i++)
    {
        for(j=0; j < g_server.max_switch; j++)
        {
            if (i == j)
            {
                g_adac_matrix.A[i][j] = 0;
            }
            else
            {
               g_adac_matrix.A[i][j] = 65535;
            }

            g_adac_matrix.src_port[i][j] = -1;

            g_short_path[i][j] = NO_PATH;
            g_short_weight[i][j] = 0;
        }
    }

    //定时发现拓扑
    g_lldp_timerid = timer_init(1);
    timer_creat(g_lldp_timerid, g_lldp_interval, NULL, &g_lldp_timer, lldp_tx_timer);

    //定时最短路径选路
    g_spath_timerid = timer_init(1);
    timer_creat(g_spath_timerid, g_lldp_interval, NULL, &g_spath_timer, spath_tx_timer);

    return GN_OK;
}

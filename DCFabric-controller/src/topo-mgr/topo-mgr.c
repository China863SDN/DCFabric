/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2015, BNC
 *
 * This file is part of the BNC SDN Controller. BNC SDN
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
*   Author      : BNC Administrator           *
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
#include "../stats-mgr/stats-mgr.h"
#include <sys/prctl.h>   

pthread_t g_timer_task_thread;

void *g_lldp_timerid = NULL;
UINT4 g_lldp_interval = 3;
void *g_lldp_timer = NULL;

void *g_spath_timerid = NULL;
void *g_spath_timer = NULL;

adac_matrix_t g_adac_matrix;
UINT4 **g_short_path;			//by:yhy 路径 二维数组
UINT4 **g_short_weight;			//by:yhy 路径权重 二维数组

/* by:yhy
 * (1)src_sw:接收到LLDP包的交换机;(2)rx_port:接收到LLDP包的端口;(3)neighbor_dpid发送LLDP包交换机的dpid;(4)tx_port:发送LLDP包的端口
 */
int mapping_new_neighbor(gn_switch_t *src_sw, UINT4 rx_port, UINT8 neighbor_dpid, UINT4 tx_port)
{
    gn_switch_t  *neigh_sw   = NULL;
    int neigh_port_seq = 0;   //neigh's port_no's Sequence
    int own_port_seq = 0;     //our port_no's Sequence

	BOOL bSwNodeEdgeUp = FALSE;
	BOOL bNeigborChange = FALSE;
	
    neigh_sw = find_sw_by_dpid(neighbor_dpid);
    if(neigh_sw)
    {//by:yhy 源交换机存在
        for(neigh_port_seq=0; neigh_port_seq<neigh_sw->n_ports; neigh_port_seq++)
        {//by:yhy 通过发送交换机的port_no，找到端口序号
			if(neigh_sw->ports[neigh_port_seq].port_no == tx_port)
			{
				neigh_sw->ports[neigh_port_seq].type = SW_CON;
				break;
			}
        }
		//by:yhy 考虑删除
        g_adac_matrix.A[src_sw->index][neigh_sw->index] = 1;
        g_adac_matrix.src_port[src_sw->index][neigh_sw->index] = rx_port;
        g_adac_matrix.sw[src_sw->index][neigh_sw->index] = src_sw;
    }
	else 
	{
		LOG_PROC("LLDP_HANDLER", "lldp_packet_handler - find_sw_by_dpid(neighbor_dpid) return GN_ERR");
		return GN_ERR;
	}

    //by:yhy 发送时的端口在发送时的交换机中不存在
    if (neigh_port_seq >= neigh_sw->n_ports)
    {
		LOG_PROC("LLDP_HANDLER", "lldp_packet_handler - neigh_port_seq >= neigh_sw->n_ports return GN_ERR");
        return GN_ERR;
    }

    //by:yhy 通过接收交换机的port_no，找到端口序号
    for(own_port_seq=0; own_port_seq<src_sw->n_ports; own_port_seq++)
    {
		if(src_sw->ports[own_port_seq].port_no == rx_port)
		{
			src_sw->ports[own_port_seq].type = SW_CON;
			break;
		}
    }

    //by:yhy 接收时的端口在发送时的交换机中不存在
    if (own_port_seq >= src_sw->n_ports)
    {
		LOG_PROC("LLDP_HANDLER", "lldp_packet_handler - own_port_seq >= src_sw->n_ports return GN_ERR");
        return GN_ERR;
    }
    
	if(!src_sw->neighbor[own_port_seq] || !neigh_sw->neighbor[neigh_port_seq]) //YCY
	{
		return GN_ERR;
	}

    if (!(src_sw->neighbor[own_port_seq]->bValid))
    {//by:yhy 接收交换机
       src_sw->neighbor[own_port_seq]->bValid =TRUE;
	   bSwNodeEdgeUp = TRUE;
    }

	if (!(neigh_sw->neighbor[neigh_port_seq]->bValid))
    {//by:yhy 发送交换机
        neigh_sw->neighbor[neigh_port_seq]->bValid = TRUE;
	    bSwNodeEdgeUp = TRUE;
    }

	if ((neigh_sw->neighbor[neigh_port_seq]->sw != src_sw || src_sw->neighbor[own_port_seq]->sw != neigh_sw)||(TRUE == bSwNodeEdgeUp ))
    {//by:yhy 拓补有变,发送信号,激活topo_change_thread
		bNeigborChange = TRUE;
    }
	//by:yhy 更新接收交换机,发送交换机双方的邻居表
    src_sw->neighbor[own_port_seq]->sw   = neigh_sw;  							
    src_sw->neighbor[own_port_seq]->port = &(neigh_sw->ports[neigh_port_seq]); 	
    neigh_sw->neighbor[neigh_port_seq]->sw   = src_sw;  						
    neigh_sw->neighbor[neigh_port_seq]->port = &(src_sw->ports[own_port_seq]);

	if(bNeigborChange)
	{
		if(src_sw&&neigh_sw)
		{
			LOG_PROC("INFO", "%s %d src_sw->sw_ip=0x%x neigh_sw->sw_ip=0x%x rx_port=%d tx_port=%d",FN,LN,src_sw->sw_ip,neigh_sw->sw_ip, rx_port, tx_port );
		}
		event_add_switch_port_on(src_sw,rx_port);
		event_add_switch_port_on(neigh_sw,tx_port);
	}

	LOG_PROC("LLDP_HANDLER", "lldp_packet_handler - STOP");
    return 1;
}
//by:yhy lldp包处理
INT4 lldp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
	LOG_PROC("LLDP_HANDLER", "%s - START",FN);
	
    UINT8 sender_id;        //发送方的dpid
    UINT4 sender_port;      //发送方的port_no

    lldp_t *pkt = (lldp_t *)packet_in_info->data;
    if (pkt->chassis_tlv_subtype != LLDP_CHASSIS_ID_LOCALLY_ASSIGNED||pkt->port_tlv_subtype != LLDP_PORT_ID_COMPONENT ||pkt->ttl_tlv_ttl != htons(120))
    {//by:yhy 判断是否为控制器构造的LLDP包
		return GN_ERR;
    }

	//by:yhy 提取发送时放入的dpid和端口号
    sender_id = gn_ntohll(pkt->chassis_tlv_id);
    sender_port = ntohs(pkt->port_tlv_id);
	LOG_PROC("LLDP_HANDLER", "%s - sender_id:%llu ,sender_port:%u",FN,sender_id,sender_port);
    
    return mapping_new_neighbor(sw, packet_in_info->inport, sender_id, sender_port);
}
//by:yhy 构建LLDP包 将src_addr,id,port赋给buffer
static void create_lldp_pkt(void *src_addr, UINT8 id, UINT2 port, lldp_t *buffer)
{
	//by:yhy 组播地址01 80 c2 00 00 0e
    UINT1 dest_addr[6] = {0x01,0x80,0xc2,0x00,0x00,0x0e};

    memcpy(buffer->eth_head.dest,dest_addr,6);
    memcpy(buffer->eth_head.src, src_addr, 6);
	//by:yhy lldp帧 固定的帧类型描述 0x88cc
    buffer->eth_head.proto = htons(0x88cc);

	//by:yhy 初始化lldp包中必选的三个TLV
	//by:yhy chassis_tlv
    buffer->chassis_tlv_type_and_len = htons(0x0209);					//by:yhy 为什么长度是9,因为1字节的tlv_subtype,8字节的tlv_id(此处赋dpid值是8字节)
    buffer->chassis_tlv_subtype = LLDP_CHASSIS_ID_LOCALLY_ASSIGNED;     //by:yhy 此处选择这个subtype,表示后续的tlv_id是本地自定义的
    buffer->chassis_tlv_id = gn_htonll(id);   
	//by:yhy port_tlv
    buffer->port_tlv_type_and_len    = htons(0x0403);
    buffer->port_tlv_subtype = LLDP_PORT_ID_COMPONENT;
    buffer->port_tlv_id = htons(port);        
	//by:yhy ttl_tlv
    buffer->ttl_tlv_type_and_len = htons(0x0602);
    buffer->ttl_tlv_ttl = htons(120);

    buffer->endof_lldpdu_tlv_type_and_len = 0x00;
}


//by:yhy 发送LLDP包
void lldp_tx(gn_switch_t *sw, UINT4 port_no, UINT1 *src_addr)
{
    packout_req_info_t packout_req_info;
    lldp_t lldp_pkt;
	//by:yhy packout包头
    packout_req_info.buffer_id = 0xffffffff;
    packout_req_info.inport = 0xfffffffd;
    packout_req_info.outport = port_no;
    packout_req_info.max_len = 0xff;
    packout_req_info.xid = 0;
    packout_req_info.data_len = sizeof(lldp_t);
    packout_req_info.data = (UINT1 *)&lldp_pkt;

    //by:yhy 构建lldp包
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

//by:yhy 定时发送lldp
void lldp_tx_timer()
{
    UINT4 num  = 0;
	UINT4 port  = 0;
    gn_switch_t *sw = NULL;
	p_Queue_node pNode  = NULL;
	gst_msgsock_t newsockmsgProc= {0};	
	INT4  iSockFd = 0;
	
    for(; num < g_server.max_switch; num++)
    {

        sw = &g_server.switches[num];
        if(/*(sw->ports[port].state == 0) &&*/(CONNECTED == sw->conn_state))
        {	
			for (port=0; port<sw->n_ports; port++)
			{
				if(sw->ports[port].state == 0)
				{
					lldp_tx(sw, sw->ports[port].port_no, sw->ports[port].hw_addr);
				}
			}
        }
    }

    return;
}

//by:yhy Floyd算法,最短路径计算方法
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

//by:yhy 定时计算最短路径
static void spath_tx_timer(void *para, void *tid)
{
	LOG_PROC("TIMER", "spath_tx_timer - START");
    path_calc(&g_adac_matrix, g_short_path, g_short_weight, g_server.max_switch);
	LOG_PROC("TIMER", "spath_tx_timer - STOP");
}

INT4 init_topo_mgr()
{
	UINT4 i, j;

	//by:yhy 网络拓补图的顶点数据即为配置中定义的最大交换机数量
	//by:yhy 分配内存空间
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

    //by:yhy 邻接矩阵内部路径的初始化
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

    //by:yhy 定时发现拓扑
	//by:yhy 定时g_lldp_interval发送LLDP包去执行链路发现
    //g_lldp_timerid = timer_init(1);
    //timer_creat(g_lldp_timerid, g_lldp_interval, NULL, &g_lldp_timer, lldp_tx_timer);

    //by:yhy 定时g_lldp_interval最短路径选路
    //g_spath_timerid = timer_init(1);
    //timer_creat(g_spath_timerid, g_lldp_interval, NULL, &g_spath_timer, spath_tx_timer);

    return GN_OK;
}


static void timer_task(void *para, void *tid)
{
	UINT4 CurrentRunning_second =1;
	p_Queue_node pNode  = NULL;
	gst_msgsock_t newsockmsgProc= {0};	
	INT4  iSockFd = 0;
	
	prctl(PR_SET_NAME, (unsigned long) "SysTimerTaskThread" ) ;  
    while(1)
    {
		CurrentRunning_second ++;
		
		if(g_lldp_interval&&((CurrentRunning_second % g_lldp_interval)==0))
		{
			push_MsgAndEvent(EVENT_PROC, pNode, &g_tProc_queue, newsockmsgProc,LLDP, iSockFd, 0 );
		}
		if(g_heartbeat_interval&&((CurrentRunning_second % g_heartbeat_interval)==0))
		{
			push_MsgAndEvent(EVENT_PROC, pNode, &g_tProc_queue, newsockmsgProc,HEARTBEAT, iSockFd, 0);
		}
		if((CurrentRunning_second % g_stats_mgr_interval)==0)
		{
			push_MsgAndEvent(EVENT_PROC, pNode, &g_tProc_queue, newsockmsgProc,GETPORTSTATE, iSockFd, 0);
		}
        MsecSleep(1000);
    }
    return ;
}


INT4 init_timer_task(void)
{
	if(pthread_create(&g_timer_task_thread, NULL, timer_task,NULL) != 0 )
    {
        return GN_ERR;
    }
	return GN_OK;
}





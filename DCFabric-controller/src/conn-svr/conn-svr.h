/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2015, BNC <ywxu@bnc.org.cn>
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
*   File Name   : conn-svr.h           *
*   Author      : BNC Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef CONN_SVR_H_
#define CONN_SVR_H_

#include "msg_handler.h"
#include "gnflush-types.h"
#include "../inc/openflow/openflow-common.h"


#define Event_QUEUE_MAX_NUM  65535*6
#define PIPEEVENT_QUEUE_NUM	65535


#define 	EVENT_RECV			0
#define	 	EVENT_PROC			1
#define 	EVENT_SEND			2

#define push_MsgAndEvent(EventNo, pNode,pQueue, stMsgSock_t,MsgType, iSockFd, swIndex ) \
	pNode = get_MsgSock_Node(g_pMsgNodeBuff); \
	if(NULL != pNode)			\
	{							\
		stMsgSock_t.uiMsgType = MsgType;		\
		stMsgSock_t.iSockFd =  iSockFd;		\
		stMsgSock_t.uiSw_Index = swIndex;	\
		stMsgSock_t.uiUsedFlag = 1;			\
		push_MsgSock_into_queue( pQueue, pNode, &stMsgSock_t);	\
		Write_Event(EventNo);			\
	}	\
	else	\
	{		\
		LOG_PROC("ERROR","Memory alloc error : %s, %d",FN, LN);	\
	}


enum msgpipe_type
{
	INITPIPE,
	ADDSOCK,
	DELSOCK,
	FREESOCK
};

typedef struct Event_SockFd
{
	UINT4   uiUsed;
	UINT4 	uiType;
	INT4    iSockFd;
	void  * funcptr;   
}stEvent_SockFd;

extern t_MultiPurpose_queue g_tProc_queue ;
extern UINT4 g_heartbeat_interval;
extern p_Queue_node		 g_pMsgNodeBuff ;
INT4 init_conn_svr();
void fini_conn_svr();
INT4 start_openflow_server();


gn_switch_t *find_sw_by_dpid(UINT8 dpid);
void free_switch(gn_switch_t *sw);
INT4 new_switch(UINT4 switch_sock, struct sockaddr_in addr);
UINT1 *init_sendbuff(gn_switch_t *sw, UINT1 of_version, UINT1 type, UINT2 buf_len, UINT4 transaction_id);
INT4 send_of_msg(gn_switch_t *sw, UINT4 total_len);
gn_switch_t* find_sw_by_port_physical_mac(UINT1* mac);

INT4 send_packet(gn_switch_t *sw, INT1* pBuf, UINT4 nLen);
void init_header(struct ofp_header *p_header,UINT1 of_version, UINT1 type, UINT2 len, UINT4 transaction_id);

void heartbeat_tx_timer();

/**************************************************************************/
//Queue related start
p_Queue_node create_MsgSock_TotalNode();
p_Queue_node get_MsgSock_Node(p_Queue_node totalnode);
INT4 free_MsgSock_Node(p_Queue_node ptotalnode, p_Queue_node pnode);

INT4 push_MsgSock_into_queue( p_MultiPurpose_queue para_queue, p_Queue_node node, gst_msgsock_t *new_msg_sock);
INT4 pop_MsgSock_from_queue( p_MultiPurpose_queue para_queue, p_Queue_node ptotalnode,  gst_msgsock_t *new_msg_sock);

//Queue related stop
/**************************************************************************/



#endif /* CONN_SVR_H_ */

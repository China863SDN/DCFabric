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
*   File Name   : conn-svr.c           *
*   Author      : BNC Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*   Modify      : 2015-5-19 by bnc
*                                                                             *
******************************************************************************/
#include <errno.h>
#include <sys/prctl.h>   
#include "conn-svr.h"
#include "timer.h"
#include "msg_handler.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../user-mgr/user-mgr.h"
#include "../event/event_service.h"
#include "fabric_impl.h"
#include "../flow-mgr/flow-mgr.h"
#include "../topo-mgr/topo-mgr.h"
#include "epoll_svr.h"
#include "event.h"
#include "openstack_routers.h"

void *g_heartbeat_timer = NULL;
void *g_heartbeat_timerid = NULL;
UINT4 g_heartbeat_interval = 5;
UINT4 g_heartbeat_times = 3;			//从配置文件中读取出的心跳次数
UINT4 g_heartbeat_count = 0;
UINT1 g_heartbeat_flag = 0;				//是否首次启动的标志

gn_server_t g_server;					//by:yhy即本控制器相关基础信息	


t_MultiPurpose_queue g_tRecv_queue ;
t_MultiPurpose_queue g_tProc_queue ;
t_MultiPurpose_queue g_tSend_queue ;
p_Queue_node		 g_pMsgNodeBuff = NULL;
void*		 		 g_pMsgNodeDataBuff = NULL;

INT4				 g_iConnEpollFd = 0;
INT4 				 g_iConnPipeFd[2];  //g_iConnPipeFd[0]---read  g_iConnPipeFd[1] --- write
stEvent_SockFd		 g_stPipeEvent_SockFdQueue[PIPEEVENT_QUEUE_NUM] ;
pthread_mutex_t 	 g_PipeEventMutex;
UINT4                g_uiPipeEventWrOffset = 0;
UINT4                g_uiPipeEventRdOffset = 0;



UINT4 g_sendbuf_len = 81920;
UINT4 g_controller_south_port = 0;

void proc_port_forward( port_forward_proc_p p_forward_proc);

/**********************************************************************/
//申请消息节点总的缓冲区
p_Queue_node create_MsgSock_TotalNode()
{
	p_Queue_node  ret	= NULL;
	ret = (p_Queue_node) mem_create(sizeof(t_Queue_node),Event_QUEUE_MAX_NUM);
	return ret;
}
//取出单个消息缓冲区
p_Queue_node get_MsgSock_Node(p_Queue_node totalnode)
{
	p_Queue_node  ret	= NULL;
	if(NULL == totalnode)
		return NULL;
	ret = (p_Queue_node) mem_get( totalnode );
	return ret;
}

//释放单个消息缓冲区
INT4 free_MsgSock_Node(p_Queue_node ptotalnode, p_Queue_node pnode)
{
	
	if((NULL == ptotalnode)||(NULL == pnode))
		return GN_ERR;

	return mem_free(ptotalnode, pnode);
}

//将消息压到线程对应的队列
INT4 push_MsgSock_into_queue( p_MultiPurpose_queue para_queue, p_Queue_node node, gst_msgsock_t *new_msg_sock)
{
	gst_msgsock_t *pmsgsock = NULL;
	if((NULL == para_queue)||(NULL == node)||(NULL == new_msg_sock))
	{
		LOG_PROC("ERROR", "Error %s",FN);
		return GN_ERR;
	}
	
	pmsgsock = mem_get(g_pMsgNodeDataBuff);
	if(NULL == pmsgsock)
	{
		LOG_PROC("ERROR", "Error %s",FN);
		return GN_ERR;
	}

	memcpy(pmsgsock, new_msg_sock , sizeof(gst_msgsock_t));
	node->nodeData = pmsgsock;
	
	push_node_into_queue(para_queue,node);
	return GN_OK;
}
//从对应队列取出消息节点，并释放该消息节点内存
INT4 pop_MsgSock_from_queue( p_MultiPurpose_queue para_queue, p_Queue_node ptotalnode, gst_msgsock_t *new_msg_sock)
{
	p_Queue_node  pNode	= NULL;
	gst_msgsock_t *pmsgsock = NULL;

	pNode = get_head_node_from_queue(para_queue);
	if( NULL != pNode)
	{
		pmsgsock = (gst_msgsock_t *)pNode->nodeData;
		pmsgsock->uiUsedFlag = 0;
		memcpy(new_msg_sock,pmsgsock, sizeof(gst_msgsock_t));
	
		mem_free(g_pMsgNodeDataBuff, pmsgsock);
		pop_node_from_queue(para_queue);
		free_MsgSock_Node(ptotalnode, pNode);
		return GN_OK;
	}
	return GN_ERR;
}
/*************************************************************************************************/

void of13_delete_line2(gn_switch_t* sw,UINT4 port_index)
{
	gn_switch_t* n_sw = NULL;
	UINT4 port = 0, n_port = 0,n_port_index = 0;
	// event delete line between 2 ports
	if(sw->neighbor[port_index]->bValid )//if(sw->neighbor[port_index] != NULL)
	{
		n_sw = sw->neighbor[port_index]->sw;
		port = sw->ports[port_index].port_no;
		// if neighbor is alived, find neighbor, delete port
		if(n_sw->conn_state != INITSTATE)//(n_sw->state != 0)
		{
			n_port = sw->neighbor[port_index]->port->port_no;
			// get neighbor sw's neighbor
			for(n_port_index=0; n_port_index < n_sw->n_ports; n_port_index++)
			{
				if(n_sw->ports[n_port_index].port_no == n_port)
				{
					if(n_sw->neighbor[n_port_index] != NULL)
					{
			           // free(n_sw->neighbor[n_port_index]);
			           // n_sw->neighbor[n_port_index] = NULL;
			           n_sw->neighbor[n_port_index]->bValid = FALSE;
			            // delete neighbor
						event_delete_switch_port_on(n_sw,n_port);
					}
					break;
				}
			}
		}
	    //free(sw->neighbor[port_index]);
	    //sw->neighbor[port_index] = NULL;
	    sw->neighbor[port_index]->bValid = FALSE;
		event_delete_switch_port_on(sw,port);
	}
	// event end
	return;
}

//by:ycy 根据sockfd在g_server中查找对应的gn_switch_t
gn_switch_t *find_sw_by_sockfd(INT4 iSockFd)
{
    UINT4 num;
    gn_switch_t  *sw = NULL;

    for(num=0; num < g_server.max_switch; num++)
    {
        sw = &g_server.switches[num];
        if(sw && (INITSTATE != sw->conn_state))
        {
            if(sw->sock_fd == iSockFd)
            {
                return sw;
            }
        }
    }

    return NULL;
}

//by:yhy 根据dpid在g_server中查找对应的gn_switch_t
gn_switch_t *find_sw_by_dpid(UINT8 dpid)
{
    UINT4 num;
    gn_switch_t  *sw = NULL;

    for(num=0; num < g_server.max_switch; num++)
    {
        sw = &g_server.switches[num];
        if(sw&&(INITSTATE != sw->conn_state))
        {
            if(sw->dpid == dpid)
            {
                return sw;
            }
        }
    }

    return NULL;
}

//by:yhy 根据物理机的MAC查找其所连接的交换机
gn_switch_t* find_sw_by_port_physical_mac(UINT1* mac)
{
	gn_switch_t* sw = NULL;
	UINT4 i_sw = 0;
	UINT4 i_port = 0;
	gn_port_t *sw_port = NULL;
	
	for (; i_sw < g_server.max_switch; i_sw++)
    {
        sw = &g_server.switches[i_sw];
        if (INITSTATE != sw->conn_state ) 
		{
			for (i_port = 0; i_port < sw->n_ports; i_port++)
            {
                sw_port = &(sw->ports[i_port]);
                if (i_port == sw->n_ports)
                {
                    sw_port = &sw->lo_port;
                }

				if (0 == memcmp(sw_port->hw_addr, mac, 6)) 
				{
					return sw;
				}
			 }
        }
	}

	return NULL;
}

void registerEpoll_OpSock(UINT4 uiOpType, INT4 iSockFd, void *funcptr)
{
	char wrbuf = 1;
	
	//send wait_close to epoll by pipe
    pthread_mutex_lock(&g_PipeEventMutex);
	g_stPipeEvent_SockFdQueue[g_uiPipeEventWrOffset].uiType = uiOpType;
	g_stPipeEvent_SockFdQueue[g_uiPipeEventWrOffset].iSockFd = iSockFd;
	g_stPipeEvent_SockFdQueue[g_uiPipeEventWrOffset].funcptr = funcptr;
	g_stPipeEvent_SockFdQueue[g_uiPipeEventWrOffset].uiUsed = 1;
	g_uiPipeEventWrOffset = (g_uiPipeEventWrOffset +1 )%PIPEEVENT_QUEUE_NUM;
	pthread_mutex_unlock(&g_PipeEventMutex);
	write(g_iConnPipeFd[1],&wrbuf, sizeof(UINT1));

	return ;
}
//创建TCP server
static INT4 create_tcp_server(UINT4 ip, UINT2 port)
{
    INT4 sockfd;
    INT4 ret;
    INT4 sockopt = 1;
    struct sockaddr_in my_addr;

    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sockopt, sizeof(sockopt));
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&sockopt, sizeof(sockopt));
    if(sockfd < 0 )
    {
        LOG_PROC("ERROR", "Controller service start failed, create TCP socket failed.");
        return GN_ERR;
    }

    memset(&my_addr , 0 , sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = 0;
//    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr));
    if( ret < 0  )
    {
        LOG_PROC("ERROR", "Controller service start failed, bind TCP failed. --errno %d", errno);
        close(sockfd);
        return GN_ERR;
    }

    if (listen(sockfd, g_server.max_switch) == -1)
    {
        LOG_PROC("ERROR", "Controller service start failed.");
        close(sockfd);
        return GN_ERR;
    }
	//LOG_PROC("INFO","|------------------------------------------------------------------------|");
    LOG_PROC("INFO", "Controller service started successfully, listening at [tcp:%s:%d]", inet_htoa(g_server.ip), g_server.port);
	//LOG_PROC("INFO","|------------------------------------------------------------------------|");
    g_server.state = TRUE;
    g_server.sock_fd = sockfd;
    return GN_OK;
}

//by:yhy 交换机消息接收并存入缓存空间
void msg_recv(gn_switch_t *sw)
{
    INT4 iRet =0;
    UINT4 head = 0;
    UINT4 tail = 0;
    INT4 len = 0;
	INT4 RecvLen = 0;

	INT4  iSockFd = 0;
	UINT4 uiMsgType = 0;
	UINT4 uiSwIndex = 0;
	gst_msgsock_t newsockmsgRecv = {0};
	gst_msgsock_t newsockmsgProc = {0};
	p_Queue_node pNode  = NULL;
	char Recv_Tempbuff[65535]= {0};
	int nErr= 0;
	gn_switch_t * switch_gw = NULL;

	
	while(1)
	{
		iRet = Read_Event(EVENT_RECV);
		if(GN_OK != iRet) 
		{
			LOG_PROC("ERROR", "Error: %s! %d",FN, LN);
			return ;
		}
		while(GN_OK == pop_MsgSock_from_queue( &g_tRecv_queue, g_pMsgNodeBuff, &newsockmsgRecv))
		{
	
			iSockFd = newsockmsgRecv.iSockFd;
			uiMsgType = newsockmsgRecv.uiMsgType;
			// LOG_PROC("INFO","%s %d uiMsgType=%d",FN, LN,uiMsgType);
			switch_gw = find_sw_by_sockfd(iSockFd);
			if (NULL == switch_gw) 
			{
				LOG_PROC("ERROR", "sock fd: %d switch is NULL!",iSockFd);
				continue;
			}
			if(CLOSE_ACT == uiMsgType)
			{
				//uiSwIndex = newsockmsgRecv.uiSw_Index;
				push_MsgAndEvent(EVENT_PROC, pNode, &g_tProc_queue, newsockmsgProc, CLOSE_ACT, iSockFd, switch_gw->index );
				continue;
			}
			if(WAITCLOSE == uiMsgType)
			{
				if((CONNECTED == switch_gw->conn_state)||(NEWACCEPT == switch_gw->conn_state))
				{
					switch_gw->conn_state = WAITCLOSE;
					push_MsgAndEvent(EVENT_PROC, pNode, &g_tProc_queue, newsockmsgProc, WAITCLOSE, iSockFd, switch_gw->index );
				}
				
	            continue;
			}
			else if(CONNECTED == uiMsgType)
			{
				RecvLen = 0;
				// find sw index by sock fd
				while((CONNECTED == switch_gw->conn_state)||(NEWACCEPT == switch_gw->conn_state))
				{   
					len = recv(switch_gw->sock_fd,Recv_Tempbuff, sizeof(Recv_Tempbuff), 0);				
					if(len <= 0)
					{//by:yhy recv返回0即连接关闭,小于0即出错
						nErr= errno;
						//by:yhy added 20170214
						//LOG_PROC("DEBUG", "%s : len=%d nErr=%d",FN,len, nErr);
						if(EAGAIN == nErr) // 缓冲区数据读完
						{
							break;
						}
							
						if(EINTR == nErr)
						{
							continue;
						}
					   
						//send wait_close to epoll by pipe
						registerEpoll_OpSock(DELSOCK, switch_gw->sock_fd , switch_gw->data);
						break;
					}
					else
					{//by:yhy recv返回大于0即接收长度
						RecvLen += len;
						//LOG_PROC("INFO", "switch_gw: %d ,buffer->cur_len: %d, WriteLen: %ld",switch_gw->index,switch_gw->recv_loop_buffer->cur_len,len);
						//LOG_PROC("INFO", "Recv: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",Recv_Tempbuff[0],Recv_Tempbuff[1],Recv_Tempbuff[2],Recv_Tempbuff[3],Recv_Tempbuff[4],Recv_Tempbuff[5],Recv_Tempbuff[6],Recv_Tempbuff[7],Recv_Tempbuff[8],Recv_Tempbuff[9],Recv_Tempbuff[10],Recv_Tempbuff[11],Recv_Tempbuff[12],Recv_Tempbuff[13],Recv_Tempbuff[14],Recv_Tempbuff[15]);
						buffer_write(switch_gw->recv_loop_buffer,Recv_Tempbuff,len);						
						//push_MsgAndEvent(EVENT_PROC, pNode, &g_tProc_queue, newsockmsgProc,CONNECTED, iSockFd, switch_gw->index );
					}
			    }	
				
				push_MsgAndEvent(EVENT_PROC, pNode, &g_tProc_queue, newsockmsgProc,CONNECTED, iSockFd, switch_gw->index );
			}
		}
	}
}

//by:yhy 校验交换机发送上来的消息是否合法
static INT4 msg_check(struct ofp_header *oh)
{
    if(oh->version == OFP10_VERSION)
    {
        if(oh->type >= OFP10_MAX_MSG)
        {
            printf("Bad Message type: %d.\n", oh->type);
            return GN_ERR;
        }
    }
    else if(oh->version == OFP13_VERSION)
    {
        if(oh->type >= OFP13_MAX_MSG)
        {
            printf("Bad Message type: %d.\n", oh->type);
            return GN_ERR;
        }
    }
    else
    {
        printf("Bad OpenFlow version: %d.\n", oh->version);
        return GN_ERR;
    }

    return GN_OK;
}
INT4 msglen_check(UINT4 length)
{
    if(length < sizeof(struct ofp_header))
    {
        return GN_ERR;
    }
	
    return GN_OK;
}

//by:yhy 消息处理(callby:msg_proc_thread)
void msg_process(gn_switch_t *sw)
{
    unsigned char *of_msg = NULL;
    struct ofp_header * header =NULL;
	INT1 Recv_Tempbuff[65535] ={0};
    while((CONNECTED ==  sw->conn_state)||(NEWACCEPT ==  sw->conn_state))
    {
        if(buffer_read((sw->recv_loop_buffer),(char *)Recv_Tempbuff,8,SW_READ_BUFFER_PEEK))
		{
			header = (struct ofp_header *)Recv_Tempbuff;
			header->length = ntohs(header->length);
			//LOG_PROC("INFO", "msg_process : sw->index: %d sw->buff->curlen:%d version:%d type:%d lenth:%ld xid:%ld",sw->index ,sw->recv_loop_buffer->cur_len,header->version,header->type,ntohs(header->length),header->xid)
			//by:yhy 校验交换机发送上来的消息是否在可支持的openflow协议范围内
			if( GN_OK != msglen_check(header->length))
			{
				break ;
			}
			if(msg_check(header) !=GN_OK)
			{//
				if(buffer_read((sw->recv_loop_buffer),(char *)Recv_Tempbuff,header->length,SW_READ_BUFFER_UNPEEK))
				{
					continue;
				}
				else
				{
					break;
				}
			}
			//by:yhy 处理openflow消息
			//LOG_PROC("INFO", "bef read sw buffer head :%p ,cur len:%ld",sw->recv_loop_buffer->head,sw->recv_loop_buffer->cur_len);
			if(buffer_read((sw->recv_loop_buffer),(char *)Recv_Tempbuff,header->length,SW_READ_BUFFER_UNPEEK))
			{
				//LOG_PROC("INFO", "after read sw buffer head :%p ,cur len:%ld",sw->recv_loop_buffer->head,sw->recv_loop_buffer->cur_len);
				message_process(sw, Recv_Tempbuff);
			}
			else
			{
				break;
			}
		}
		else
		{
			return ;
		}
    }
}

//by:yhy 交换机信息接收线程
void *msg_recv_thread(void *index)
{
    gn_switch_t *sw = NULL;

	prctl(PR_SET_NAME, (unsigned long) "Msg_Recv_Thread" ) ;  

	msg_recv(sw);
    return NULL;
}

//by:yhy 交换机信息处理线程
void *msg_proc_thread(void *index)
{
    gn_switch_t *sw = NULL;
        INT4 iRet = 0;
        UINT4 head = 0;
        UINT4 tail = 0;
        UINT4 uiMsgType = 0;
        UINT4 uiswIndex = 0;
        INT4  iSockFd = 0;
        INT4  num = 0;
        INT4 switch_idx = -1;
        socklen_t addrlen;
        struct sockaddr_in addr;
        
        p_Queue_node pNode  = NULL;
        
        gst_msgsock_t newsockmsgProc= {0};	
        gst_msgsock_t newsockmsgSend= {0};
        
        prctl(PR_SET_NAME, (unsigned long) "Msg_Proc_Thread" ) ;  
        
        addrlen = sizeof(struct sockaddr);
        while(1)
        {
            iRet = Read_Event(EVENT_PROC);
            if(GN_OK != iRet) 
            {
                LOG_PROC("ERROR", "Error: %s! %d",FN, LN);
                    return NULL;
            }

            while(GN_OK == pop_MsgSock_from_queue( &g_tProc_queue,g_pMsgNodeBuff, &newsockmsgProc))
        	{
        		
        		//continue;

				/*
				num++;

				if((!(num%1000))||(g_tProc_queue.QueueLength>500))
				{
					LOG_PROC("ERROR", "Error: %s! %d &g_tSend_queue.QueueLength = %d",FN,LN, g_tProc_queue.QueueLength );
					num=0;
				}
				*/
			uiMsgType =  newsockmsgProc.uiMsgType;
			uiswIndex = newsockmsgProc.uiSw_Index ;
			iSockFd = newsockmsgProc.iSockFd;

                switch(uiMsgType)
                {
                    case NEWACCEPT:
                        {
                            if(!getpeername(iSockFd, (struct sockaddr *)&addr, &addrlen))
                            {
                                //printf( "Peer IP：%s ", inet_ntoa(addr.sin_addr));
                                //printf( "Peer PORT：%d ", ntohs(addr.sin_port));
                                switch_idx = new_switch(iSockFd, addr);
	                            if(switch_idx == -1 )
	                            {
	                                LOG_PROC("ERROR", "Accept a new switch failed, max switch count is reached.");
	                                close(iSockFd);
	                            }
                            }
							  break;
                            
                        }
                    case CONNECTED:
                        {
                            sw = &g_server.switches[uiswIndex];
                            msg_process(sw);
                            break;
                        }
                    case WAITCLOSE:
                        {
							sw = &g_server.switches[uiswIndex];
                            if(WAITCLOSE == sw->conn_state)
                            {
                                push_MsgAndEvent(EVENT_SEND, pNode, &g_tSend_queue, newsockmsgSend,WAITCLOSE, iSockFd, sw->index );
                            }

                            break;
                        }
                    case CLOSE_ACT:
                        {
                            sw = &g_server.switches[uiswIndex];
                            free_switch(sw);
                            break;
                        }
						case LLDP:
						{
							lldp_tx_timer();
							break;
						}
						case HEARTBEAT:
						{
							heartbeat_tx_timer();
							break;
						}
						case GETPORTSTATE:
						{
							get_throughput();
							break;
						}
						case PORT_FORWARD:
								  {
                                                           proc_port_forward((port_forward_proc_p) newsockmsgProc.param);
									  break;
								  }
                };
            
        }
    }
    return NULL;
}

void *msg_send_thread(void *index)
{
	UINT4 uiMsgType = 0;
	UINT4 uiswIndex = 0;
    INT4  iSockFd = 0;
	INT4  iErrno =0;
	INT4  iWriteLen = 0;
	INT4  iRet = 0;
	INT4  num = 0;
	
	
	gn_switch_t *sw = NULL;
	gst_msgsock_t newsockmsgSend = {0};
	
	
	prctl(PR_SET_NAME, (unsigned long) "Msg_Send_Thread" ) ; 
	
	while(1)
	{
		iRet = Read_Event(EVENT_SEND);
		if(GN_OK != iRet) 
		{
			LOG_PROC("ERROR", "Error: %s! %d",FN,LN);
			return NULL;
		}
		while(GN_OK == pop_MsgSock_from_queue( &g_tSend_queue, g_pMsgNodeBuff,&newsockmsgSend))
		{
			/*
			num++;

			if(!(num%1000))
			{
				LOG_PROC("ERROR", "Error: %s! %d &g_tSend_queue.QueueLength = %d",FN,LN, g_tSend_queue.QueueLength );
				num=0;
			}
			*/
			//continue;
			
			uiMsgType =  newsockmsgSend.uiMsgType;
			uiswIndex = newsockmsgSend.uiSw_Index ;
			iSockFd =  newsockmsgSend.iSockFd;
			
			//LOG_PROC("INFO","%s %d uiMsgType=%d",FN, LN,uiMsgType);
			sw = &g_server.switches[uiswIndex];
	
		if(CONNECTED ==  uiMsgType)
		{
			//by:yhy 根据发送长度判断是否有数据发送
		    if(sw->send_len)
		    {
		    	iWriteLen = 0;
		        pthread_mutex_lock(&sw->send_buffer_mutex);
				while(iWriteLen < sw->send_len)				//modify by ycy
				{
		        	iRet = write(sw->sock_fd, sw->send_buffer+iWriteLen, sw->send_len-iWriteLen);
			
					if(iRet <= 0)
					{
						iErrno = errno;
					
						
						if(EAGAIN == iErrno)
						{
							break;
						}
						if(EINTR == iErrno)
						{
							usleep(5);
							continue;
						}
							LOG_PROC("DEBUG", "%s : len=%d nErr=%d sw->sock_fd=%d uiswIndex=%d sw->conn_state=%d ",FN,iRet, iErrno,sw->sock_fd, uiswIndex,sw->conn_state);
						if((CONNECTED == sw->conn_state)||(NEWACCEPT ==  uiMsgType))
						{
							//send wait_close to epoll by pipe
							registerEpoll_OpSock(DELSOCK, sw->sock_fd , sw->data);
						}

							break;
						}
						else
						{
							iWriteLen += iRet;
						}
					}
					if(iWriteLen > 0)
					{
						//by:yhy 一次性全部清空
				        // memset(sw->send_buffer, 0, g_sendbuf_len);
				        sw->send_len -= iWriteLen;
						if(sw->send_len > 0)
						{
							memcpy(sw->send_buffer, sw->send_buffer+iWriteLen, sw->send_len);
						}
					}
			        pthread_mutex_unlock(&sw->send_buffer_mutex);
			    }
			}
			else if(WAITCLOSE ==  uiMsgType)
			{	
				registerEpoll_OpSock(FREESOCK, sw->sock_fd , sw->data);
			}
		}
	
	}
	return NULL;
}

//by:yhy 向sw的send_buffer内按当前参数填入要发送的openflow数据包
UINT1 *init_sendbuff(gn_switch_t *sw, UINT1 of_version, UINT1 type, UINT2 buf_len, UINT4 transaction_id)
{
    UINT1 *b = NULL;
    struct ofp_header *h;

    pthread_mutex_lock(&sw->send_buffer_mutex);
    if(sw->send_len < g_sendbuf_len)
    {
        b = sw->send_buffer + sw->send_len;
    }
    else
    {
        printf("........\n");
        write(sw->sock_fd, sw->send_buffer, sw->send_len);
        memset(sw->send_buffer, 0, g_sendbuf_len);
        sw->send_len = 0;
        b = sw->send_buffer;
    }

    h = (struct ofp_header *)b;
    h->version = of_version;
    h->type    = type;
    h->length  = htons(buf_len);

    if (transaction_id)
    {
        h->xid = transaction_id;
    }
    else
    {
        h->xid = random_uint32();
    }

    return b;
}

//by:yhy 发送openflow消息(解锁switch的发送缓存,具体发送在发送线程中执行)
INT4 send_of_msg(gn_switch_t *sw, UINT4 total_len)
{
	INT4 iSockFd = 0;
	INT4 iSwIndex = 0;
	gst_msgsock_t newsockmsgSend= {0};
	p_Queue_node pNode  = NULL;
	
    sw->send_len += total_len;
    pthread_mutex_unlock(&sw->send_buffer_mutex);

	iSockFd = sw->sock_fd;
	iSwIndex= sw->index;
	push_MsgAndEvent(EVENT_SEND, pNode, &g_tSend_queue, newsockmsgSend, CONNECTED, iSockFd, iSwIndex );
    return GN_OK;
}

INT4 send_packet(gn_switch_t *sw, INT1* pBuf, UINT4 nLen)
{
	INT4 iSockFd = 0;
	INT4 iSwIndex = 0;
	gst_msgsock_t newsockmsgSend= {0};
	p_Queue_node pNode  = NULL;
        BOOL bResult = TRUE;
    pthread_mutex_lock(&sw->send_buffer_mutex);
   
    if(sw->send_buffer + sw->send_len + nLen > sw->send_buffer + g_sendbuf_len)
    {
        bResult = FALSE; 
    }
    else
    {
      memcpy(sw->send_buffer + sw->send_len, pBuf, nLen);
      sw->send_len += nLen;
    }
    
    pthread_mutex_unlock(&sw->send_buffer_mutex);

	iSockFd = sw->sock_fd;
	iSwIndex= sw->index;

       if(bResult)
       {
	push_MsgAndEvent(EVENT_SEND, pNode, &g_tSend_queue, newsockmsgSend, CONNECTED, iSockFd, iSwIndex );
       }
       else
       {
	  registerEpoll_OpSock(DELSOCK, iSockFd ,0);
       }

       return GN_OK;
}


void init_header(struct ofp_header *p_header,UINT1 of_version, UINT1 type, UINT2 len, UINT4 transaction_id)
{
	p_header->version = of_version;
	p_header->type = type;
	p_header->length = htons(len);
    if (transaction_id)
    {
        p_header->xid = transaction_id;
    }
    else
    {
        p_header->xid = random_uint32();
    }
}

//by:yhy 有新的交换机连入则将其初始化入g_server.switches,并发送ofpt_hello
INT4 new_switch(UINT4 switch_sock, struct sockaddr_in addr)
{
    if(g_server.cur_switch < g_server.max_switch)
    {
        UINT idx = 0;
        for(; idx < g_server.max_switch; idx++)
        {
            if(INITSTATE == g_server.switches[idx].conn_state)
            {//by:yhy 找到g_sever中交换机数组中最近的未使用的那一项
                g_server.switches[idx].sock_fd = switch_sock;
                g_server.switches[idx].msg_driver.msg_handler = of_message_handler;
                g_server.switches[idx].sw_ip = *(UINT4 *)&addr.sin_addr;
                g_server.switches[idx].sw_port = *(UINT2 *)&addr.sin_port;
                g_server.switches[idx].recv_buffer.head = 0;
                g_server.switches[idx].recv_buffer.tail = 0;
                g_server.switches[idx].send_len = 0;
                memset(g_server.switches[idx].send_buffer, 0, g_sendbuf_len);
                //g_server.switches[idx].state = 1;
                g_server.switches[idx].sock_state = 0;
				g_server.switches[idx].conn_state = NEWACCEPT;
                pthread_mutex_lock(&g_server.cur_switch_mutex);
				g_server.cur_switch++;
				pthread_mutex_unlock(&g_server.cur_switch_mutex);
                // printf("version:%d, ip:%d\n", g_server.switches[idx].ofp_version, g_server.switches[idx].sw_ip);
                of13_msg_hello(&g_server.switches[idx], NULL);

				registerEpoll_OpSock(ADDSOCK, switch_sock, g_server.switches[idx].data);
                return idx;
            }
        }
    }
    return -1;
}
//by:yhy 释放交换机结构体
void free_switch(gn_switch_t *sw)
{
    UINT4 port_idx;
    UINT1 dpid[8];
    ulli64_to_uc8(sw->dpid, dpid);
    /*LOG_PROC("WARNING", "Switch [%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x] disconnected", dpid[0],
            dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);   
	
	 LOG_PROC("INFO","%s %d sockfd=%d",FN, LN,sw->sock_fd );*/

	if((0 == sw->sw_ip)||(INITSTATE == sw->conn_state))
	{
		//abort();
		return;
	}
   
    //reset driver
    sw->sw_ip = 0;
    sw->sw_port = 0;
    sw->recv_buffer.head = 0;
    sw->recv_buffer.tail = 0;
 


    reset_loop_buffer(sw->recv_loop_buffer);	
    
    if((sw->msg_driver.msg_handler != of10_message_handler)
       && (sw->msg_driver.msg_handler != of13_message_handler)
       && (sw->msg_driver.msg_handler != of_message_handler))
    {
        //gn_free((void **)&(sw->msg_driver.msg_handler));
    }
    sw->msg_driver.msg_handler = of_message_handler;
    sw->send_len = 0;
    
    //by:yhy 触发删除交换机的相关操作
   // event_delete_switch_on(sw);

   

    clean_flow_entries(sw);

    //reset neighbor
    for (port_idx = 0; port_idx < MAX_PORTS; port_idx++)
    {
        //if (sw->neighbor[port_idx])
        if (sw->neighbor[port_idx]->bValid)
        {
        	of13_delete_line2(sw,port_idx);
        }
    }
    memset(sw->ports, 0, sizeof(gn_port_t) * MAX_PORTS);

    //reset user
	/*Need to be deleted
    for (hash_idx = 0; hash_idx < g_macuser_table.macuser_hsize; hash_idx++)
    {
        p_macuser = sw->users[hash_idx];
        del_mac_user(p_macuser);
    }
    memset(sw->users, 0, g_macuser_table.macuser_hsize * sizeof(mac_user_t *));
	*/

	//by:yhy 触发删除交换机的相关操作
    event_delete_switch_on(sw);

	pthread_mutex_lock(&g_server.cur_switch_mutex);
	g_server.cur_switch--;
	pthread_mutex_unlock(&g_server.cur_switch_mutex);
	
	if(0 != sw->sock_fd)
	{
		close(sw->sock_fd);
	
    	sw->sock_fd = 0;
	}
	sw->sock_state = 0;
	  // sw->state = 0;
	//sw->conn_state = INITSTATE;
	
}

INT4 client_read_func(INT4 iSockFd)
{
	gst_msgsock_t newsockmsgRecv = {0};
	p_Queue_node pNode  = NULL;


	if(g_server.cur_switch >g_server.max_switch)
	{
		LOG_PROC("ERROR","func %s g_server.cur_switch=%d", FN, g_server.cur_switch);
		return;
	}
	//LOG_PROC("INFO","func %s iSockFd=%d", FN, iSockFd);

	push_MsgAndEvent(EVENT_RECV, pNode, &g_tRecv_queue, newsockmsgRecv,CONNECTED, iSockFd, 0 );
	return GN_OK;
}

INT4 client_write_func(INT4 iSockFd)
{

	//LOG_PROC("INFO","func %s iSockFd=%d", FN,iSockFd);
	return GN_OK;
}
INT4 error_proc_func(INT4 iSockFd)
{
	gst_msgsock_t newsockmsgRecv = {0};
	p_Queue_node pNode  = NULL;

	//LOG_PROC("INFO","func %s %d", FN, iSockFd);
	EpollSvr_DelSockFd( g_iConnEpollFd, iSockFd, TRUE ); //del from epoll poll

	//EPOLL ERR --> SEND WAITCLOSE TO RECV THREAD

	push_MsgAndEvent(EVENT_RECV, pNode, &g_tRecv_queue, newsockmsgRecv, WAITCLOSE, iSockFd, 0 );
	return GN_OK;
}

INT4 svr_read_func(INT4 iSockFd)
{
	INT4 switch_sock = -1;
	INT4 iErrno = 0;
	socklen_t addrlen;
    struct sockaddr_in addr;
	p_Queue_node pNode  = NULL;
	gst_msgsock_t newsockmsgProc = {0};

	
	addrlen = sizeof(struct sockaddr);

	
	//LOG_PROC("INFO","func %s %d", FN, iSockFd);
	if(iSockFd < 0)
		return GN_ERR;
	
	while(1)
	{
		switch_sock = accept(iSockFd, (struct sockaddr *)&addr, &addrlen);
		if(switch_sock <= 0)
		{
			iErrno = errno;
			if(EINTR == iErrno)
			{
				continue;
			}
			break;
		}
		if(*(UINT4 *)&addr.sin_addr == 0)
	    {//by:yhy 校验连入交换机客户端IP
	        LOG_PROC("ERROR", "Accept a new switch connection failed, socket address error");
	        close(switch_sock);
	        return GN_ERR;
	    }

		//push_MsgAndEvent(EVENT_PROC, pNode, &g_tProc_queue, newsockmsgProc, NEWACCEPT , switch_sock, 0 );
		pNode = get_MsgSock_Node(g_pMsgNodeBuff); 
		if(NULL != pNode)			
		{							
			newsockmsgProc.uiMsgType = NEWACCEPT;		
			newsockmsgProc.iSockFd =  switch_sock;		
			newsockmsgProc.uiSw_Index = 0;	
			newsockmsgProc.uiUsedFlag = 1;			
			push_MsgSock_into_queue( &g_tProc_queue, pNode, &newsockmsgProc);	
			Write_Event(EVENT_PROC);			
		}	
	}
	return GN_OK;
}
INT4 pipe_MsgProc(UINT4 uiOffsetNum)
{
	INT4 i = 0;
	UINT4 uiReadPos = 0;
	INT4 iSockFd = 0;
	UINT4 uiMsgType = 0;
	gst_msgsock_t newsockmsgRecv = {0};
	p_Queue_node pNode  = NULL;
	stEpollsvr_Sockfuncptr *pstEpollsvr_SockfuncAddr = NULL;

	for(i=0; i < uiOffsetNum ; i++)
	{
		uiReadPos = g_uiPipeEventRdOffset;
		uiMsgType = g_stPipeEvent_SockFdQueue[uiReadPos].uiType;
		iSockFd = g_stPipeEvent_SockFdQueue[uiReadPos].iSockFd;
		pstEpollsvr_SockfuncAddr = g_stPipeEvent_SockFdQueue[uiReadPos].funcptr;
		g_stPipeEvent_SockFdQueue[uiReadPos].uiUsed = 0;
		g_uiPipeEventRdOffset = (g_uiPipeEventRdOffset +1 )%PIPEEVENT_QUEUE_NUM;

		//LOG_PROC("DEBUG","func %s sockfd= %d uiMsgType=%d", FN, iSockFd,uiMsgType);
		switch(uiMsgType)
		{
			case ADDSOCK:
				EpollSvr_AddSockFd( g_iConnEpollFd, iSockFd, TRUE,  pstEpollsvr_SockfuncAddr );
				break;
			case DELSOCK: 
				//EpollSvr_DelSockFd( g_iConnEpollFd, iSockFd, TRUE ); //del from epoll poll
				//EPOLL ERR --> SEND WAITCLOSE TO RECV THREAD
				push_MsgAndEvent(EVENT_RECV, pNode, &g_tRecv_queue, newsockmsgRecv, WAITCLOSE, iSockFd, 0 );
				break;
			case FREESOCK:

				push_MsgAndEvent(EVENT_RECV, pNode, &g_tRecv_queue, newsockmsgRecv, CLOSE_ACT, iSockFd, 0 );
				break;
			default:
				return GN_ERR;
		}
	}
	return GN_OK;
}
INT4 pipe_read_func(INT4 iSockFd)
{
	INT4 ret = 0;
	UINT4 uiReadLen = 0;
	INT4 iErrno = 0;
	char readbuf[1024] = {0};
	

	//LOG_PROC("INFO","func %s %d", FN, iSockFd);
	while(1)
	{
		ret = read(g_iConnPipeFd[0],readbuf,1024);
		if(ret <= 0)
		{
			iErrno = errno;
			if(EINTR == iErrno)
			{
				continue;
			}
			break;
		}
		uiReadLen += ret;
	}
	if(uiReadLen>0)
	{
		pipe_MsgProc(uiReadLen);
	}
	
	return GN_OK;
}

//by:yhy 启动openflow端口,等待交换机接入
INT4 start_openflow_server()
{
	INT4  ret = 0;
	stEpollsvr_Sockfuncptr *pstEpollsvr_SockfuncAddr = NULL;
	
	ret = create_tcp_server(g_server.ip, g_server.port);
	if (GN_OK != ret) 
	{
		return GN_ERR;
	}
	

	ret = pipe(g_iConnPipeFd); //0 --success -1---failed
	ret += EpollSvr_Create(&g_iConnEpollFd);
	if (GN_OK != ret) 
	{
		return GN_ERR;
	}
	
	pstEpollsvr_SockfuncAddr =  register_EpollSock_Handle(pipe_read_func, NULL, error_proc_func);
	if (NULL == pstEpollsvr_SockfuncAddr) 
	{
		return GN_ERR;
	}
	fcntl(g_iConnPipeFd[1], F_SETFL, fcntl(g_iConnPipeFd[1], F_GETFD, 0)| O_NONBLOCK);
	EpollSvr_AddSockFd( g_iConnEpollFd, g_iConnPipeFd[0], TRUE,  pstEpollsvr_SockfuncAddr );
	
	pstEpollsvr_SockfuncAddr =  register_EpollSock_Handle(svr_read_func, NULL, error_proc_func);
	if (NULL == pstEpollsvr_SockfuncAddr) 
	{
		return GN_ERR;
	}
	EpollSvr_AddSockFd( g_iConnEpollFd, g_server.sock_fd, TRUE,  pstEpollsvr_SockfuncAddr );

	EpollSvr_Loop(g_iConnEpollFd, g_server.state);

    return GN_OK;
}

//by:yhy 初始化交换机结构体
static INT4  init_switch(gn_switch_t *sw)
{
    UINT4 j = 0;
    UINT1 *recv_buffer = NULL;
	sw->recv_loop_buffer =init_loop_buffer(SW_DEFAULT_RECV_BUFFER_LENGTH);
    if(NULL == sw->recv_loop_buffer)
	{	
        LOG_PROC("ERROR", "init_loop_buffer error");
        return GN_ERR;
    }

	
	//by:yhy 接收缓存分配空间
    sw->recv_buffer.buff_arr = (buffer_cache_t *)gn_malloc(g_server.buff_num * sizeof(buffer_cache_t));
    if(NULL == sw->recv_buffer.buff_arr)
    {
		//by:yhy add 201701051031
		LOG_PROC("ERROR", "init_switch -- sw->recv_buffer.buff_arr gn_malloc  Finall return GN_ERR");
        return GN_ERR;
    }
	//by:yhy why? 为何数量要多一个
    recv_buffer = (UINT1 *)gn_malloc((g_server.buff_num + 1) * g_server.buff_len);
    if(NULL == recv_buffer)
    {
        free(sw->recv_buffer.buff_arr);
		//by:yhy add 201701051031
		LOG_PROC("ERROR", "init_switch -- recv_buffer gn_malloc  Finall return GN_ERR");
        return GN_ERR;
    }
	//by:yhy  指针偏移,此处将多分配的那块内存遗弃
    recv_buffer += g_server.buff_len;
    for(j = 0; j < g_server.buff_num; j++)
    {//by:yhy 给每个w->recv_buffer.buff_arr[j].buff分配空间
        sw->recv_buffer.buff_arr[j].buff = recv_buffer + g_server.buff_len * j;
    }
	
	//by:yhy 发送缓存分配空间
    sw->send_len = 0;
    sw->send_buffer = (UINT1 *)gn_malloc(g_sendbuf_len);
    if(NULL == sw->send_buffer)
    {//by:yhy 分配内存失败,释放buffer
        free(sw->recv_buffer.buff_arr[0].buff);
        free(sw->recv_buffer.buff_arr);
		
		//by:yhy add 201701051031
		LOG_PROC("ERROR", "init_switch -- sw->send_buffer gn_malloc  Finall return GN_ERR");
        return GN_ERR;
    }
	 for(j = 0; j < MAX_PORTS; j++)
	 {
		sw->neighbor[j] = (neighbor_t *)gn_malloc(sizeof(neighbor_t));
		if(NULL == sw->neighbor[j])
		{
			free(sw->recv_buffer.buff_arr[0].buff);
        	free(sw->recv_buffer.buff_arr);
			free(sw->send_buffer);
			//by:yhy add 201701051031
			LOG_PROC("ERROR", "init_switch -- sw->neighbor gn_malloc  Finall return GN_ERR");
        	return GN_ERR;
		}
	 }
	
    return GN_OK;
}

//控制器与交换机之间的心跳检
void heartbeat_tx_timer()
{
	LOG_PROC("TIMER", "heartbeat_tx_timer - START");
    
	//不建议变量后补初始化
    UINT4 num  = 0;
    gn_switch_t *sw = NULL;
	
	
    for(; num < g_server.max_switch; num++)
    {
		sw = &g_server.switches[num];
		if(CONNECTED == sw->conn_state)
		{
			if (OFP10_VERSION == sw->ofp_version) 
			{
				of10_msg_echo_request(sw, NULL);
			}
			else
			{ 
				of13_msg_echo_request(sw, NULL);
			}
		}
    }

    if (g_heartbeat_count < g_heartbeat_times) 
    {
        g_heartbeat_count++;
    }
    else
    {
        g_heartbeat_count = 0;
    }
	LOG_PROC("TIMER", "heartbeat_tx_timer - STOP");
}

//by:yhy
INT4 init_conn_svr()
{
    INT4 ret = 0;
    UINT4 i = 0;
    INT1 *value = NULL;
	pthread_t swPktRecv_thread;
	pthread_t swPktProc_thread;
	pthread_t swPktSend_thread;
	
	ret  = Create_Event(EVENT_RECV);
	ret += Create_Event(EVENT_PROC);
	ret += Create_Event(EVENT_SEND);
	if(GN_OK != ret) 
		return GN_ERR;
	
	g_pMsgNodeBuff = create_MsgSock_TotalNode();
	if(NULL == g_pMsgNodeBuff)
		return GN_ERR;
	g_pMsgNodeDataBuff =  mem_create(sizeof(gst_msgsock_t),Event_QUEUE_MAX_NUM);
	if(NULL == g_pMsgNodeDataBuff)
	{
		return GN_ERR;
	}
		
	init_queue(&g_tRecv_queue);
	init_queue(&g_tProc_queue);
	init_queue(&g_tSend_queue);
	memset(&g_stPipeEvent_SockFdQueue, 0, PIPEEVENT_QUEUE_NUM*sizeof(stEvent_SockFd));
	pthread_mutex_init(&g_PipeEventMutex,NULL);
	
    memset(&g_server, 0, sizeof(gn_server_t));

    g_server.ip = g_controller_ip;

    value = get_value(g_controller_configure, "[controller]", "openflow_service_port");
    g_server.port = ((NULL == value) ? 6633 : atoi(value));
    g_controller_south_port = g_server.port;

    value = get_value(g_controller_configure, "[controller]", "max_switch");
    g_server.max_switch = ((NULL == value) ? 7 : atoi(value));

    value = get_value(g_controller_configure, "[controller]", "buff_num");
    g_server.buff_num = ((NULL == value) ? 1000 : atoi(value));

    value = get_value(g_controller_configure, "[controller]", "buff_len");
    g_server.buff_len = ((NULL == value) ? 20480 : atoi(value));

    value = get_value(g_controller_configure, "[heartbeat_conf]", "heartbeat_interval");
    g_heartbeat_interval= ((NULL == value) ? 5 : atoll(value));

    value = get_value(g_controller_configure, "[heartbeat_conf]", "heartbeat_times");
    g_heartbeat_times= ((NULL == value) ? 3 : atoll(value));
	

	//by:yhy 获取CPU核心数
    g_server.cpu_num = get_total_cpu();
	//by:yhy 分配交换机列表内存空间
    g_server.switches = (gn_switch_t *)gn_malloc(g_server.max_switch * sizeof(gn_switch_t));
    if(NULL == g_server.switches)
    {
		//by:yhy add 201701051031
		LOG_PROC("ERROR", "init_conn_svr -- g_server.switches  gn_malloc  Finall return GN_ERR");
        return GN_ERR;
    }

	
	stEpollsvr_Sockfuncptr *pstEpollsvr_SockfuncAddr = NULL;
	for(i = 0; i < g_server.max_switch; i++)
	{
		pstEpollsvr_SockfuncAddr =  register_EpollSock_Handle(client_read_func, client_write_func, error_proc_func);
		if (NULL == pstEpollsvr_SockfuncAddr) 
		{
			free(g_server.switches );
			LOG_PROC("ERROR", "init_switch -- register_EpollSock_Handle gn_malloc  Finall return GN_ERR");
			return GN_ERR;
		}
		g_server.switches[i].data = pstEpollsvr_SockfuncAddr;
	}
	
	pthread_mutex_init(&g_server.cur_switch_mutex, NULL);
    //by:yhy 初始化交换机各功能的多线程互斥锁
    for(i = 0; i < g_server.max_switch; i++)
    {
        pthread_mutex_init(&g_server.switches[i].send_buffer_mutex, NULL);
        pthread_mutex_init(&g_server.switches[i].flow_entry_mutex, NULL);
        pthread_mutex_init(&g_server.switches[i].meter_entry_mutex, NULL);
        pthread_mutex_init(&g_server.switches[i].group_entry_mutex, NULL);
        pthread_mutex_init(&g_server.switches[i].sock_state_mutex, NULL);
        g_server.switches[i].index = i;
		//by:yhy 初始化g_server下每个交换机
        if(GN_OK != init_switch(&(g_server.switches[i])))
        {
            return GN_ERR;
        }
    }
	
	pthread_create(&swPktRecv_thread, NULL, msg_recv_thread, NULL);
	pthread_create(&swPktProc_thread, NULL, msg_proc_thread, NULL);
	pthread_create(&swPktSend_thread, NULL, msg_send_thread, NULL);
	
    //by:yhy 控制器与交换机之间的心跳包发送
    //g_heartbeat_timerid = timer_init(1);
    //timer_creat(g_heartbeat_timerid, g_heartbeat_interval, NULL, &g_heartbeat_timer, heartbeat_tx_timer);
    
	ret = GN_OK;
    return ret;
}

void fini_conn_svr()
{
    g_server.state = FALSE;
}

void print_port_forward(openstack_port_forward_p p_forward_proc_list)
{
	openstack_port_forward_p  port_forward_templist = NULL;
	if(NULL == p_forward_proc_list) 
	{
		return;
	}
	port_forward_templist = p_forward_proc_list;
	while(port_forward_templist)
	{
		LOG_PROC("INFO","p_forward_proc_list proto=%d state=%d src_port=%d dst_port=%d",port_forward_templist->proto, port_forward_templist->state, port_forward_templist->src_port, port_forward_templist->dst_port);
		
		LOG_PROC("INFO","p_forward_proc_list n_src_ip=0x%x n_dst_ip=0x%x src_ip=%s dst_ip=%s",port_forward_templist->n_src_ip, port_forward_templist->n_dst_ip, port_forward_templist->src_ip, port_forward_templist->dst_ip);
		
		LOG_PROC("INFO","p_forward_proc_list network_id=%s",port_forward_templist->network_id);
		port_forward_templist = port_forward_templist->next;
	}
	
	
}
void proc_port_forward( port_forward_proc_p p_forward_proc)
{ 
	if(NULL == p_forward_proc) 
	{
		return;
	}
	destroy_portforward_old_flows(p_forward_proc->old_list);
	//print_port_forward(p_forward_proc->old_list);
	destory_port_forward_list(p_forward_proc->old_list);
	destory_port_forward_list(g_openstack_forward_list);
        
    g_openstack_forward_list = p_forward_proc->copy_list;
	//print_port_forward(g_openstack_forward_list);
    gn_free(&p_forward_proc);
}

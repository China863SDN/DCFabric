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
*   File Name   : conn-svr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*   Modify      : 2015-5-19 by bnc
*                                                                             *
******************************************************************************/

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

void *g_heartbeat_timer = NULL;
void *g_heartbeat_timerid = NULL;
UINT4 g_heartbeat_interval = 5;
UINT4 g_heartbeat_times = 3;
UINT4 g_heartbeat_count = 0;
UINT1 g_heartbeat_flag = 0;

gn_server_t g_server;
UINT4 g_sendbuf_len = 81920;
UINT4 g_controller_south_port = 0;
void of13_delete_line2(gn_switch_t* sw,UINT4 port_index){
	gn_switch_t* n_sw = NULL;
	UINT4 port = 0, n_port = 0,n_port_index = 0;
	// event delete line between 2 ports
	if(sw->neighbor[port_index] != NULL){
		n_sw = sw->neighbor[port_index]->sw;
		port = sw->ports[port_index].port_no;
		// if neighbor is alived, find neighbor, delete port
		if(n_sw->state != 0){
			n_port = sw->neighbor[port_index]->port->port_no;
			// get neighbor sw's neighbor
			for(n_port_index=0; n_port_index < n_sw->n_ports; n_port_index++){
				if(n_sw->ports[n_port_index].port_no == n_port){
					if(n_sw->neighbor[n_port_index] != NULL){
			            free(n_sw->neighbor[n_port_index]);
			            n_sw->neighbor[n_port_index] = NULL;
			            // delete neighbor
						event_delete_switch_port_on(n_sw,n_port);
					}
					break;
				}
			}
		}
	    free(sw->neighbor[port_index]);
	    sw->neighbor[port_index] = NULL;
		event_delete_switch_port_on(sw,port);
	}
	// event end
    //
	return;
};
gn_switch_t *find_sw_by_dpid(UINT8 dpid)
{
    UINT4 num;
    gn_switch_t  *sw = NULL;

    for(num=0; num < g_server.max_switch; num++)
    {
        if ((g_server.switches) && (g_server.switches[num].state))
        {
            sw = &g_server.switches[num];
            if(sw)
            {
                if(sw->dpid == dpid)
                {
                    return sw;
                }
            }
        }
    }

    return NULL;
}

gn_switch_t* find_sw_by_port_physical_mac(UINT1* mac)
{
	gn_switch_t* sw = NULL;
	UINT4 i_sw = 0;
	UINT4 i_port = 0;
	gn_port_t *sw_port = NULL;
	
	for (; i_sw < g_server.max_switch; i_sw++)
    {
        sw = &g_server.switches[i_sw];
		
        if (sw->state == 1) {
			for (i_port = 0; i_port <= sw->n_ports; i_port++)
            {
                sw_port = &(sw->ports[i_port]);
                if (i_port == sw->n_ports)
                {
                    sw_port = &sw->lo_port;
                }

				if (0 == memcmp(sw_port->hw_addr, mac, 6)) {
					return sw;
				}
			 }
        }
	}

	return NULL;
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
//    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&sockopt, sizeof(sockopt));
//    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&sockopt, sizeof(sockopt));
//    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(char *)&wait, sizeof(wait));
//    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&sockopt, sizeof(sockopt));

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

    LOG_PROC("INFO", "Controller service started successfully, listening at [tcp:%s:%d]", inet_htoa(g_server.ip), g_server.port);

    g_server.state = TRUE;
    g_server.sock_fd = sockfd;
    return GN_OK;
}

void msg_recv(gn_switch_t *sw)
{
    fd_set rfds;
    INT4 ret;
    UINT4 head = 0;
    UINT4 tail = 0;
    UINT4 len = 0;
    struct timeval tv;

    while(sw->state)
    {
        FD_ZERO(&rfds);
        FD_SET(sw->sock_fd, &rfds);
        tv.tv_sec = 6;
        tv.tv_usec = 0;
        ret = select(sw->sock_fd + 1, &rfds, NULL, NULL, &tv);
        if(ret <= 0)
        {
            continue;
        }
        else
        {
            if(FD_ISSET(sw->sock_fd, &rfds))
            {
                head = sw->recv_buffer.head;
                tail = sw->recv_buffer.tail;
                if( (tail+1)%g_server.buff_num != head )
                {
                    len = recv(sw->sock_fd, sw->recv_buffer.buff_arr[tail].buff, g_server.buff_len, 0);
                    if(len <= 0)
                    {
                        //exit(-1);
                        free_switch(sw);
                        break;
                    }
                    else
                    {
                        sw->recv_buffer.buff_arr[tail].len = len;
                        sw->recv_buffer.tail = (tail+1)%g_server.buff_num;

//                        if(DEBUG)
//                            printf("-- RECV head[%d], tail[%d], len[%d]\n", sw->recv_buffer.head, sw->recv_buffer.tail, sw->recv_buffer.buff_arr[tail].len);
                    }
                }
//                else
//                {
//                    printf("#### Buffer is full %llu.\n", g_cur_sys_time.tv_sec);
//                }
            }
        }
    }
}

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

void msg_process(gn_switch_t *sw)
{
    unsigned char *of_msg = NULL;
    struct ofp_header *header = NULL;

    INT4 offset = 0;        //偏移量
    INT4 left_len = 0;      //剩余处理长度
    INT4 pkt_len = 0;       //包总长度
    INT4 next_head = 0;     //下一个buffer
    INT4 curr_head = sw->recv_buffer.head;
    buffer_cache_t *buff_arr = sw->recv_buffer.buff_arr;
    unsigned char *buffer = buff_arr[curr_head].buff - buff_arr[curr_head].bak_len;
    left_len = buff_arr[curr_head].len + buff_arr[curr_head].bak_len;

//    if(DEBUG)
//            printf("-- PROC head[%d], totlen[%d], recvlen[%d], baklen[%d]\n", curr_head, left_len, buff_arr[curr_head].len, buff_arr[curr_head].bak_len);

    next_head = (curr_head + 1) % g_server.buff_num;
    buff_arr[next_head].bak_len = 0;

    while(sw->sock_fd && left_len > 0)
    {
        of_msg = buffer + offset;
        header = (struct ofp_header*)of_msg;
        pkt_len = ntohs(header->length);
        if((left_len < sizeof(struct ofp_header)) || (pkt_len > left_len))
        {
            buff_arr[next_head].bak_len = left_len;
            if((buff_arr[curr_head].len != g_server.buff_len) || (next_head == 0))
            {
            	memmove(buff_arr[next_head].buff - left_len, of_msg, left_len);
                break;
            }
        }
        msg_check(header);
//        if(msg_check(header))
//        {
//            offset++;
//            continue;
//        }

        offset += pkt_len;
        left_len -= pkt_len;

        //处理openflow消息
        message_process(sw, of_msg);
    }
}

void *msg_recv_thread(void *index)
{
    gn_switch_t *sw = NULL;
    UINT4 idx = *(int *)index;
    UINT4 cpu_id = 0;

    sw = &g_server.switches[idx];
    cpu_id = idx % g_server.cpu_num;
    set_cpu_affinity(cpu_id);

    signal(SIGPIPE, SIG_IGN);
    while(1)
    {
        if( sw->state == 0 )
        {
            usleep(1000);
            continue;
        }
        else
        {
            msg_recv(sw);
        }
    }

    return NULL;
}

void *msg_proc_thread(void *index)
{
    gn_switch_t *sw = NULL;
    UINT4 idx = *(UINT4 *)index;
    UINT4 cpu_id = 0;
    UINT4 head = 0;
    UINT4 tail = 0;

    sw = &g_server.switches[idx];
    cpu_id = idx % g_server.cpu_num;
    set_cpu_affinity(cpu_id);

    signal(SIGPIPE, SIG_IGN);
    while(1)
    {
        head = sw->recv_buffer.head;
        tail = sw->recv_buffer.tail;
        if(head != tail )
        {
//            printf("#### Proc new buffer %d.\n", g_cur_sys_time.tv_sec);
            msg_process(sw);
            sw->recv_buffer.head =(head + 1) % g_server.buff_num;
//            printf("#### Proc new over %d.\n", g_cur_sys_time.tv_sec);
        }
        else
        {
//            printf("#### Buffer is empty.\n");
            usleep(1000);
        }

        if(sw->send_len)
        {
//            printf("START  %d\n", sw->send_len);
            pthread_mutex_lock(&sw->send_buffer_mutex);
            write(sw->sock_fd, sw->send_buffer, sw->send_len);
            memset(sw->send_buffer, 0, g_sendbuf_len);
            sw->send_len = 0;
            pthread_mutex_unlock(&sw->send_buffer_mutex);
//            printf("END\n");
        }
    }

    return NULL;
}

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

//发送openflow消息
INT4 send_of_msg(gn_switch_t *sw, UINT4 total_len)
{
//    INT4 ret = 0;

    sw->send_len += total_len;

    pthread_mutex_unlock(&sw->send_buffer_mutex);

//    if(ret != total_len)
//    {
//        printf("socket send error, --errno[%d], --socketfd[%d]\n", errno, sw->sock_fd);
//        return EC_SW_SEND_MSG_ERR;
//    }
//
//    if(1)
//    {
//        struct ofp_header* header = (struct ofp_header*)sw->send_buffer;
//        printf("@@@@ Send msg type[%d], total_len[%d]\n", header->type, total_len);
//        printf("<=- UNLOCK [%d]\n", sw->index);
//    }
    return GN_OK;
}

static INT4 new_switch(UINT4 switch_sock, struct sockaddr_in addr)
{
    if(g_server.cur_switch < g_server.max_switch)
    {
        UINT idx = 0;
        for(; idx < g_server.max_switch; idx++)
        {
            if(0 == g_server.switches[idx].state)
            {
                g_server.cur_switch++;
                g_server.switches[idx].sock_fd = switch_sock;
                g_server.switches[idx].msg_driver.msg_handler = of_message_handler;
                g_server.switches[idx].sw_ip = *(UINT4 *)&addr.sin_addr;
                g_server.switches[idx].sw_port = *(UINT2 *)&addr.sin_port;
                g_server.switches[idx].recv_buffer.head = 0;
                g_server.switches[idx].recv_buffer.tail = 0;
                g_server.switches[idx].send_len = 0;
                memset(g_server.switches[idx].send_buffer, 0, g_sendbuf_len);
                g_server.switches[idx].state = 1;
                g_server.switches[idx].sock_state = 0;

                // printf("version:%d, ip:%d\n", g_server.switches[idx].ofp_version, g_server.switches[idx].sw_ip);
                of13_msg_hello(&g_server.switches[idx], NULL);

                return idx;
            }
        }
    }

    return -1;
}

void free_switch(gn_switch_t *sw)
{
    UINT4 port_idx;
    UINT4 hash_idx;
    mac_user_t *p_macuser;
    UINT1 dpid[8];
    ulli64_to_uc8(sw->dpid, dpid);
    LOG_PROC("WARNING", "Switch [%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x] disconnected", dpid[0],
            dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);
    g_server.cur_switch--;
    
    sw->sock_fd = 0;
    sw->sock_state = 0;
    //reset driver
    sw->sw_ip = 0;
    sw->sw_port = 0;
    sw->recv_buffer.head = 0;
    sw->recv_buffer.tail = 0;
    sw->state = 0;
    
    if((sw->msg_driver.msg_handler != of10_message_handler)
            && (sw->msg_driver.msg_handler != of13_message_handler)
            && (sw->msg_driver.msg_handler != of_message_handler))
    {
        gn_free((void **)&(sw->msg_driver.msg_handler));
    }
    sw->msg_driver.msg_handler = of_message_handler;
    sw->send_len = 0;

    // event delete switch
    event_delete_switch_on(sw);
    // event end

    clean_flow_entries(sw);

    //reset neighbor
    for (port_idx = 0; port_idx < MAX_PORTS; port_idx++)
    {
        if (sw->neighbor[port_idx])
        {
        	of13_delete_line2(sw,port_idx);
        }
    }
    memset(sw->ports, 0, sizeof(gn_port_t) * MAX_PORTS);

    //reset user
    for (hash_idx = 0; hash_idx < g_macuser_table.macuser_hsize; hash_idx++)
    {
        p_macuser = sw->users[hash_idx];
        del_mac_user(p_macuser);
    }
    memset(sw->users, 0, g_macuser_table.macuser_hsize * sizeof(mac_user_t *));
}

INT4 start_openflow_server()
{
	// printf("%s: %d\n", FN, g_cur_sys_time.tv_sec);

	INT4 ret = create_tcp_server(g_server.ip, g_server.port);

	if (GN_OK != ret) {
		return ret;
	}

    socklen_t addrlen;
    struct sockaddr_in addr;
    INT4 switch_sock = -1;
    INT4 switch_idx = -1;

    while(g_server.state)
    {
        addrlen = sizeof(struct sockaddr);
        switch_sock = accept(g_server.sock_fd, (struct sockaddr *)&addr, &addrlen);
        if (switch_sock <= -1)
        {
            LOG_PROC("ERROR", "Accept a new switch connection failed.");
            return GN_ERR;
        }

        if(*(UINT4 *)&addr.sin_addr == 0)
        {
            LOG_PROC("ERROR", "Accept a new switch connection failed, socket address error");
            close(switch_sock);
            continue;
        }

        switch_idx = new_switch(switch_sock, addr);
        if(switch_idx == -1 )
        {
            LOG_PROC("ERROR", "Accept a new switch failed, max switch count is reached.");
            close(switch_sock);
            continue;
        }
    }

    return GN_OK;
}

static INT4 init_switch(gn_switch_t *sw)
{
    UINT4 j = 0;
    UINT1 *recv_buffer = NULL;

    sw->recv_buffer.buff_arr = (buffer_cache_t *)gn_malloc(g_server.buff_num * sizeof(buffer_cache_t));
    if(NULL == sw->recv_buffer.buff_arr)
    {
        return GN_ERR;
    }

    recv_buffer = (UINT1 *)gn_malloc((g_server.buff_num + 1) * g_server.buff_len);
    if(NULL == recv_buffer)
    {
        free(sw->recv_buffer.buff_arr);
        return GN_ERR;
    }

    recv_buffer += g_server.buff_len;
    for(j = 0; j < g_server.buff_num; j++)
    {
        sw->recv_buffer.buff_arr[j].buff = recv_buffer + g_server.buff_len * j;
    }

    sw->send_len = 0;
    sw->send_buffer = (UINT1 *)gn_malloc(g_sendbuf_len);
    if(NULL == sw->send_buffer)
    {
        free(sw->recv_buffer.buff_arr[0].buff);
        free(sw->recv_buffer.buff_arr);
        return GN_ERR;
    }

    pthread_create(&(sw->pid_recv), NULL, msg_recv_thread, (void *)&(sw->index));
    pthread_create(&(sw->pid_proc), NULL, msg_proc_thread, (void *)&(sw->index));
    return GN_OK;
}


//控制器与交换机之间的心跳检测
static void heartbeat_tx_timer(void *para, void *tid)
{
    //延迟30s，确保控制器启动完毕
    if (0 == g_heartbeat_flag)
    {
        g_heartbeat_flag = 1;
        sleep(30);
        return;
    }
    
    UINT4 num  = 0;
    gn_switch_t *sw = NULL;
    for(; num < g_server.max_switch; num++)
    {
        if(g_server.switches[num].state)
        {
            sw = &g_server.switches[num];
            if(1 == sw->state)
            {
                //超过g_heartbeat_threshold次没收到响应，断开与该交换机的连接
                if (g_heartbeat_count >= g_heartbeat_times) 
                {
                    if (1 != sw->sock_state)
                    {
                        //关闭连接
                        if (0 != sw->sock_fd)
                        {
                            close(sw->sock_fd);
                        }

                        //删除交换机
                        free_switch(sw);
                    }

                    pthread_mutex_lock(&sw->sock_state_mutex);
                    sw->sock_state = 0;
                    pthread_mutex_unlock(&sw->sock_state_mutex);
                }

                //发送心跳消息echo
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
    }

    if (g_heartbeat_count < g_heartbeat_times) 
    {
        g_heartbeat_count++;
    }
    else
    {
        g_heartbeat_count = 0;
    }
}


INT4 init_conn_svr()
{
    INT4 ret = 0;
    UINT4 i = 0;
    INT1 *value = NULL;

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

    g_server.cpu_num = get_total_cpu();
    g_server.switches = (gn_switch_t *)gn_malloc(g_server.max_switch * sizeof(gn_switch_t));
    if(NULL == g_server.switches)
    {
        return GN_ERR;
    }

    //初始化交换机
    for(; i < g_server.max_switch; i++)
    {
        pthread_mutex_init(&g_server.switches[i].send_buffer_mutex, NULL);
        pthread_mutex_init(&g_server.switches[i].flow_entry_mutex, NULL);
        pthread_mutex_init(&g_server.switches[i].meter_entry_mutex, NULL);
        pthread_mutex_init(&g_server.switches[i].group_entry_mutex, NULL);
        pthread_mutex_init(&g_server.switches[i].sock_state_mutex, NULL);
        g_server.switches[i].index = i;
        init_switch(&(g_server.switches[i]));
    }

    // ret = create_tcp_server(g_server.ip, g_server.port);

    //控制器与交换机之间的心跳检测
    g_heartbeat_timerid = timer_init(1);
    timer_creat(g_heartbeat_timerid, g_heartbeat_interval, NULL, &g_heartbeat_timer, heartbeat_tx_timer);
    

	ret = GN_OK;
    return ret;
}

void fini_conn_svr()
{
    g_server.state = FALSE;
}

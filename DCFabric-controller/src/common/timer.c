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
*   File Name   : timer.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "timer.h"

UINT1 RUN_FLAG = 0;
struct timeval g_cur_sys_time;
pthread_t g_datetime_thread;

typedef struct _timer_node
{
    UINT1       is_used;
    UINT4       timeout;            //依次会递减
    UINT4       times;              //之前保存的值

    void        *para;              //指向自己的参数
    void        *timer_id;          //指向自己的结构体

    void        (*fun)(void *para,  void *timer_id);
}Timer_Node;

typedef struct _timer_list
{
    UINT4 total_len;          //总共的单元个数
    UINT4 used_len;
    Timer_Node **arr;
    void *mutex;
    void *pool;              //节点首地址的
    pthread_t pthread_id;    //定时器的“时钟”线程ID
}Timer_List;

void *timer_thread(void *_timer_hdl)
{
    Timer_List *timer_hdl;
    UINT4       i;
    Timer_Node  *node;
    timer_hdl = (Timer_List *)_timer_hdl;

    while(1)
    {
        for(i=0;i<timer_hdl->total_len;i++)
        {
            node = timer_hdl->arr[i];
            if(node && node->is_used)
            {
                if((node->times == 0) && (node->fun))
                {
                    node->fun(node->para ,node->timer_id);
                    node->times = node->timeout;
                }
                else
                {
                    node->times--;
                }
            }
        }
        sleep(1);
    }
}

UINT4 timer_num(void *_timer_hdl)
{
    Timer_List *timer_hdl;
    timer_hdl = (Timer_List *)_timer_hdl;
    return mem_num(timer_hdl->pool);
}

void *timer_init(UINT4  len)
{
    Timer_List *timer_hdl;
    Timer_Node *tmp;
    UINT4 i;
    timer_hdl = (Timer_List *)malloc(sizeof(Timer_List));
    if(timer_hdl == NULL)
    {
        return NULL;
    }
    memset(timer_hdl, 0, sizeof(Timer_List));

    timer_hdl->arr = (Timer_Node **)malloc(sizeof(Timer_Node *) * len);
    if( timer_hdl->arr == NULL  )
    {
        return NULL;
    }
    memset(timer_hdl->arr, 0, sizeof(sizeof(Timer_Node *) * len));

    timer_hdl->mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if(timer_hdl->mutex == NULL)   return NULL;
    pthread_mutex_init(timer_hdl->mutex , NULL);

    timer_hdl->total_len = len;
    timer_hdl->pool = mem_create( sizeof(Timer_Node),len);
    if(timer_hdl->pool == NULL)
    {
        return NULL;
    }
    for(i=0; i< len ;i++)
    {
        tmp = (Timer_Node *)mem_get(timer_hdl->pool);

        tmp->is_used = 0;
        timer_hdl->arr[i] = tmp;
        mem_free(timer_hdl->pool, tmp);
    }
    if( pthread_create(&(timer_hdl->pthread_id),NULL,timer_thread,(void *)timer_hdl) != 0 )
    {
        return NULL;
    }
    return (void *)timer_hdl;
}

void *timer_creat(void *_timer_hdl ,UINT4 timeout,void *para,void **timer_id, void (*fun)(void *para ,void *timer_id) )
{
    Timer_List *timer_hdl;
    Timer_Node *tmp;
    timer_hdl = (Timer_List *)_timer_hdl;
    if( timer_hdl == NULL )
    {
        return NULL;
    }

    tmp = (Timer_Node *)mem_get(timer_hdl->pool);
    if(tmp == NULL)
    {
        return NULL;
    }

    tmp->timeout = timeout;
    tmp->times =    timeout;
    tmp->is_used = 1;
    tmp->fun = fun;
    tmp->para = para;
    tmp->timer_id = tmp;        //全都指向用户空间
    *timer_id = tmp;

    return _timer_hdl;
}
void *timer_kill(void *_timer_hdl ,void **timer_id)
{
    Timer_List *timer_hdl;
    Timer_Node *tmp;
    timer_hdl = (Timer_List *)_timer_hdl;
    tmp       = (Timer_Node *)*timer_id;
    if(!timer_hdl || !tmp )
    {
        return NULL;
    }

    tmp->is_used = 0;
    mem_free(timer_hdl->pool,tmp);

    *timer_id = NULL;
    return _timer_hdl;
}

void *timer_destroy(void **_timer_hdl)
{
    Timer_List *timer_hdl;
    if(NULL == _timer_hdl)
    {
        return NULL;
    }

    timer_hdl = (Timer_List *)(*_timer_hdl);
    mem_destroy(timer_hdl->pool);
    pthread_cancel(timer_hdl->pthread_id);
    pthread_mutex_destroy(timer_hdl->mutex);
    free(timer_hdl->mutex);
    free(timer_hdl->arr);

    *_timer_hdl = NULL;
    return _timer_hdl;
}

void *sys_time()
{
    while(RUN_FLAG)
    {
        gettimeofday(&g_cur_sys_time, NULL);
        sleep(1);
    }

    return NULL;
}

int init_sys_time()
{
    RUN_FLAG = 1;
    if(pthread_create(&g_datetime_thread, NULL, sys_time,NULL) != 0 )
    {
        return -1;
    }
    return 0;
}

void fini_sys_time()
{
    RUN_FLAG = 0;
}

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
*   File Name   : mem_pool.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "mem_pool.h"

//内存池的操作
typedef struct _Queue_List      //每一个用户都从这里面取Nat单元
{
    UINT4       total_len;          //总共的单元个数
    UINT4       usable_len;         //可用的
    void        *data;
    void        **arr;
    UINT4       head;
    UINT4       tail;
    void        *mutex;
    char        *states;            //防止重复释放,0:没有占用  1:已经占用
    UINT2       block;
}Queue_List;

//归返的,先加在返
static int Queue_In(void *_q_list, void *data)
{
    Queue_List *q_list;
    UINT4   tail;
//  UINT4   head;
    int     pos;
//  void *unit=NULL;
    q_list = (Queue_List *)(_q_list);

    if(data == NULL)
    {
        return -1;
    }

    //必须为0
    if( (data - q_list->data) %  q_list->block )
    {
        printf("free add error,%p\n",data);
        return -1;
    }
    pos = (data - q_list->data) /  q_list->block ;      //节点的位置
    if(pos <0 || pos >= q_list->total_len )
    {
        printf("free add error over %d\n",pos);
        return -1;
    }
    if( *(q_list->states + pos ) == 1 )                 //说明重复释放了内存
    {
        printf("alread free \n");
        return -1;
    }

    pthread_mutex_lock(q_list->mutex );
    {
        tail = q_list->tail;
        q_list->arr[tail] = data;
        *(q_list->states + pos ) = 1;
        q_list->tail = ( tail + 1 ) % q_list->total_len;
        q_list->usable_len++;
    }
    pthread_mutex_unlock(q_list->mutex );

    return 1;
}



//拿来用的，先出在减
static void *Queue_Out(void *_q_list)      //把队列
{
    Queue_List *q_list;
    int head;
//  int tail;
    int pos;
    void *unit=NULL;
    q_list = (Queue_List *)(_q_list);

    pthread_mutex_lock(q_list->mutex );
    {
        if( q_list->usable_len >0 )
        {
            head = q_list->head;
            unit = q_list->arr[head];
            pos = (unit - q_list->data)  /  q_list->block;          //确定节点地址
            *(q_list->states + pos) = 0;                            //没有占用
            q_list->usable_len--;
            q_list->head = (head+1)%q_list->total_len;
            memset(unit, 0, q_list->block);
        }
    }
    pthread_mutex_unlock(q_list->mutex );
    return unit;
}

static void *Queue_Init(UINT4 block ,UINT4 len)
{
	UINT8 i;
    char    *data;
    Queue_List *q_list;

    q_list = (Queue_List *)malloc(sizeof(Queue_List));
    if(q_list == NULL ) return q_list;
    memset(q_list, 0, sizeof(Queue_List));

    //锁
    q_list->mutex = (pthread_mutex_t *)malloc( sizeof(pthread_mutex_t) );
    if(q_list->mutex == NULL)   return NULL;
    pthread_mutex_init(q_list->mutex , NULL);

    q_list->data = malloc(    block * len );  //首地址
    if( q_list->data == NULL )  return NULL;
    memset(q_list->data, 0, block * len);

    q_list->arr = (void **)malloc( len * sizeof(void *) );
    if( q_list->arr == NULL )   return NULL;
    memset(q_list->arr, 0, len * sizeof(void *));

    q_list->states = (char *)malloc( len * sizeof(char) );
    if( q_list->states == NULL )    return NULL;
    memset(q_list->states, 0, len * sizeof(char));


    q_list->head = 0;
    q_list->tail = 0;               //都指向0位置处
    q_list->usable_len = 0;         //能用的个数
    q_list->block = block;
    q_list->total_len = len;


    for(i=0; i < len ; i++)
    {
        data = q_list->data + i *block ;
        Queue_In(q_list, data);
    }
    return (void *)q_list;

}


void *mem_create(UINT4 block , UINT4 len)   //快大小 ，节点个数
{
    void *pool;
    pool = Queue_Init(block,len);
    return  (void *)pool;
}

void *mem_get(void *pool)
{
    void *data;
    data = Queue_Out(pool);
    return data;
}

int mem_free(void *pool ,void *data)    //禁止同一内存释放两次
{
    return Queue_In(pool , data);
}

void mem_destroy(void *pool)
{
    Queue_List *q_list = (Queue_List *)pool;
    free(q_list->arr);
    free(q_list->data);
    free(q_list->states);
    if(q_list->mutex)
    {
        pthread_mutex_destroy(q_list->mutex);
        free(q_list->mutex);
    }

    free(q_list);
}

UINT4 mem_num(void *_pool)              //可用节点的个数
{
    Queue_List *pool;
    pool = (Queue_List *)(_pool);
    return (pool->total_len - pool->usable_len);
}

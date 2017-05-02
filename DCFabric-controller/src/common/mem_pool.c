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

//by:yhy 内存池结构体
typedef struct _Queue_List      
{
    UINT4       total_len;          //by:yhy 内存池总共的单元个数
    UINT4       usable_len;         //by:yhy 内存池可用的单元个数
    void        *data;				//by:yhy 内存池数据区
    void        **arr;				//by:yhy 内存池存储数据区各区块头指针的一个数组(该内存池是向上增长的)
    UINT4       head;				//by:yhy 内存池数据区头指针索引(该内存池是向上增长的)即当前未使用的区块头指针的索引
    UINT4       tail;
    void        *mutex;
    char        *states;            //防止重复释放,1:没有占用  0:已经占用
    UINT2       block;
}Queue_List;

//by:yhy 在_q_list中插入data
//by:yhy 与其说是将data插入q_list不如说是将data所占空间还给内存池q_list
static int Queue_In(void *_q_list, void *data)
{
    Queue_List *q_list;
    UINT4   tail;
    int     pos;
    q_list = (Queue_List *)(_q_list);

    if(data == NULL)
    {
        return -1;
    }

    //by:yhy 判断数据是否是块的整数倍
    if( (data - q_list->data) %  q_list->block )
    {
        printf("free add error,%p\n",data);
        return -1;
    }
	//by:yhy 节点的位置及合法性校验
    pos = (data - q_list->data) /  q_list->block ;      
    if(pos <0 || pos >= q_list->total_len )
    {
        printf("free add error over %d\n",pos);
        return -1;
    }
	
	//by:yhy 多线程安全访问
    pthread_mutex_lock(q_list->mutex );
    {
		//by:yhy 说明重复释放了内存
		if( *(q_list->states + pos ) == 1 )                 
		{
			pthread_mutex_unlock(q_list->mutex );
			LOG_PROC("ERROR", "%s : already free",FN);
			Debug_PrintTrace();
			return -1;
		}
		
        tail = q_list->tail;
        q_list->arr[tail] = data;
        *(q_list->states + pos ) = 1;
        q_list->tail = ( tail + 1 ) % q_list->total_len;
        q_list->usable_len++;
    }
    pthread_mutex_unlock(q_list->mutex );

    return 1;
}

//by:yhy 从queue中取出一元素(更像是取出一元素的空间并初始化0)
static void *Queue_Out(void *_q_list)     
{
    Queue_List *q_list;
    int head;
    int pos;
    void *unit=NULL;
    q_list = (Queue_List *)(_q_list);

    pthread_mutex_lock(q_list->mutex );
    {
        if( q_list->usable_len >0 )
        {//by:yhy 可用数量大于0
            head = q_list->head;
            unit = q_list->arr[head];
            pos = (unit - q_list->data)  /  q_list->block;          //确定节点地址
            *(q_list->states + pos) = 0;                            //没有占用
            q_list->usable_len--;
            q_list->head = (head+1)%q_list->total_len;
			//by:yhy  清零初始化
            memset(unit, 0, q_list->block);
        }
    }
    pthread_mutex_unlock(q_list->mutex );
    return unit;
}
//by:yhy 取得block*len大小的队列
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

//by:yhy 取得lock*len大小的队列(内存池)
void *mem_create(UINT4 block , UINT4 len)   //块大小,块数量
{
    void *pool;
    pool = Queue_Init(block,len);
    return  (void *)pool;
}
//by:yhy  queue获取队列首部一元素地址(该元素已赋0,即初始化)
//        为何清零参见mem_free的注释
//!!!注意:mem_get可能会存在获取不到内存空间的时候.即有可能返回NULL指针
void *mem_get(void *pool)
{
    void *data;
    data = Queue_Out(pool);
    return data;
}
//by:yhy 此处的free操作其实就是把已经利用结束的内存区域还给内存池.供以后需要再利用时通过mem_get获得.这也是为什么mem_get时需要清零的原因
int mem_free(void *pool ,void *data)    //禁止同一内存释放两次
{
    return Queue_In(pool , data);
}
//by:yhy 内存池销毁
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
//by:yhy 返回内存池可用节点个数
UINT4 mem_num(void *_pool)              //可用节点的个数
{
    Queue_List *pool;
    pool = (Queue_List *)(_pool);
    return (pool->total_len - pool->usable_len);
}

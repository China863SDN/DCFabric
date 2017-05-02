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
#include "ini.h"
#include <sys/prctl.h>   

extern ini_file_t *g_controller_configure;

UINT1 g_log_debug = 0;
//
UINT1 RUN_FLAG = 0;
//by:yhy 全局 系统当前时间(每秒刷新一次)
struct timeval g_cur_sys_time;
pthread_t g_datetime_thread;

//时间节点
typedef struct _timer_node
{
    UINT1       is_used;								//by:yhy 该node是否启用
    UINT4       timeout;            					//by:yhy 该node超时时间
    UINT4       times;              					//by:yhy 该node剩余时间
    void        *para;              					//by:yhy 参数
    void        *timer_id;          					//by:yhy 参数
    void        (*fun)(void *para,  void *timer_id);	//by:yhy 当times为零时执行的功能函数
}Timer_Node;



//一个与时间(定时器)相关的多线程安全的时间列表
typedef struct _timer_list
{
    UINT4 total_len;          	//
	UINT4 used_len;				//
    Timer_Node **arr;			//
    void *mutex;				//
    void *pool;      			//
	pthread_t pthread_id;    	//ID
}Timer_List;

//by:yhy 时间线程:在超时时间到达后执行_timer_hdl->arr[i]->fun这个具体的功能
void *timer_thread(void *_timer_hdl)
{
    Timer_List *timer_hdl;
    UINT4       i;
    Timer_Node  *node;
    timer_hdl = (Timer_List *)_timer_hdl;
	
	prctl(PR_SET_NAME, (unsigned long) "TimerThread" ) ;  
    while(1)
    {
        for(i=0;i<timer_hdl->total_len;i++)
        {
            node = timer_hdl->arr[i];
            if(node && node->is_used)
            {
                if((node->times == 0) && (node->fun))
                {//by:yhy 到达计时时间触发函数功能
                    node->fun(node->para ,node->timer_id);
                    node->times = node->timeout;
                }
                else
                {//by:yhy 倒计时自减
                    node->times--;
                }
            }
        }
        //sleep(1);
        MsecSleep(1000);
    }
}

UINT4 timer_num(void *_timer_hdl)
{
    Timer_List *timer_hdl;
    timer_hdl = (Timer_List *)_timer_hdl;
    return mem_num(timer_hdl->pool);
}
//by:yhy 构建一个长度为len的时间池
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
    
    memset(timer_hdl->arr, 0, sizeof(Timer_Node *) * len);

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
		if(NULL!= tmp)
		{
			tmp->is_used = 0;
			timer_hdl->arr[i] = tmp;
			mem_free(timer_hdl->pool, tmp);
		}
		else
		{
			LOG_PROC("ERROR", "%s -- mem_get(timer_hdl->pool) Fail",FN);
		}
    }
    if( pthread_create(&(timer_hdl->pthread_id),NULL,timer_thread,(void *)timer_hdl) != 0 )
    {
        return NULL;
    }
    return (void *)timer_hdl;
}
//by:yhy 用timeout,para,fun去初始化_timer_hdl,timer_id
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
		LOG_PROC("ERROR", "%s -- mem_get Fail",FN);
        return NULL;
    }

    tmp->timeout = timeout;
    tmp->times =    timeout;
    tmp->is_used = 1;
    tmp->fun = fun;
    tmp->para = para;
    tmp->timer_id = tmp;       //全都指向用户空间 
    *timer_id = tmp;

    return _timer_hdl;
}
//by:yhy
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

//by:yhy 刷新当前系统时间
void *sys_time()
{
	UINT4 iCount = 0;
	INT1* value = NULL;
	prctl(PR_SET_NAME, (unsigned long) "SysTimeGetThread" ) ;  
    while(RUN_FLAG)
    {
		//by:yhy add 201701091313
		//LOG_PROC("TIME", "sys_time - gettimeofday");
        gettimeofday(&g_cur_sys_time, NULL);
		#if 0
		LocalTime = localtime((time_t*)(&g_cur_sys_time));
		//by:yhy
		LocalYear  = LocalTime->tm_year+1900;
		LocalMonth = LocalTime->tm_mon+1;
		LOG_PROC("TIME", "Day: %d/%d/%d  Time: %d:%d:%d",
				 LocalYear,
				 LocalMonth,
				 LocalTime->tm_mday,
				 LocalTime->tm_hour,
				 LocalTime->tm_min,
				 LocalTime->tm_sec);
		#endif
        iCount++;
		if(!(iCount%10))
		{
			value = get_value(g_controller_configure, "[controller]", "log_debug");
			g_log_debug = ((NULL == value) ? 0 : atoll(value));;
		}
        MsecSleep(1000);
    }

    return NULL;
}
//
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

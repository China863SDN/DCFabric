/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
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


/*
 * event_service.h
 *
 *  Created on: May 14, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#include <sys/prctl.h>   
#include "event_service.h"
#include "gnflush-types.h"
////////////////////////////////////////////////////////////
event_add_switch_fun g_event_add_switch_fun[EVENT_MAX_FUNCTION_NUM];
event_delete_switch_fun g_event_delete_switch_fun[EVENT_MAX_FUNCTION_NUM];
event_add_switch_port_fun g_event_add_switch_port_fun[EVENT_MAX_FUNCTION_NUM];
event_delete_switch_port_fun g_event_delete_switch_port_fun[EVENT_MAX_FUNCTION_NUM];

UINT2 g_event_add_switch_fun_num = 0;
//by:yhy 等待被删除的交换机的数量
UINT2 g_event_delete_switch_fun_num = 0;
UINT2 g_event_add_switch_port_fun_num = 0;
UINT2 g_event_delete_switch_port_fun_num = 0;

gn_switch_t* g_event_add_switch_list[EVENT_MAX_CHANGED_SWITCH_NUM];
//by:yhy 等待删除的交换机的列表集
gn_switch_t* g_event_delete_switch_list[EVENT_MAX_CHANGED_SWITCH_NUM];
t_event_sw_port g_event_add_switch_port_list[EVENT_MAX_CHANGED_SWITCH_PORT_NUM];
t_event_sw_port g_event_delete_switch_port_list[EVENT_MAX_CHANGED_SWITCH_PORT_NUM];

UINT4 g_event_add_switch_list_num = 0;
//by:yhy 等待删除的交换机数量
UINT4 g_event_delete_switch_list_num = 0;
UINT4 g_event_add_switch_port_list_num = 0;
UINT4 g_event_delete_switch_port_list_num = 0;

static pthread_mutex_t g_event_add_switch_list_mutex = PTHREAD_MUTEX_INITIALIZER;
//by:yhy 用于删除交换机操作的线程锁
static pthread_mutex_t g_event_delete_switch_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_event_add_switch_port_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_event_delete_switch_port_list_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t topo_change_thread_id;
//by:yhy 拓补发生改变的信号量
sem_t topo_change_thread_sem;
static UINT1 g_topo_change_thread_flag;

extern void init_openstack_fabric_auto_start_delay();

////////////////////////////////////////////////////////////
void *topo_change_event_thread();
void call_add_switch_fun();
void call_delete_switch_fun();
void call_add_switch_port_fun();
void call_delete_switch_port_fun();
////////////////////////////////////////////////////////////

//by:yhy 拓补变动的扫描线程启动
void thread_topo_change_start()
{
	LOG_PROC("INFO", "event_topo_change_thread_start");
	g_topo_change_thread_flag = 1;
	//初始化多线程之间的未命名信号量
	sem_init(&topo_change_thread_sem,0,0);
	pthread_create(&topo_change_thread_id,NULL,topo_change_event_thread,NULL);
	LOG_PROC("INFO", "event_topo_change_thread_start finish");
	return;
}
void thread_topo_change_stop(){
	g_topo_change_thread_flag = 0;
	sem_post(&topo_change_thread_sem);
	if(topo_change_thread_id)
	    pthread_detach(topo_change_thread_id);
	return;
}

// callback functions register & unregister


void register_add_switch_function(event_add_switch_fun method){
	pthread_mutex_lock(&g_event_add_switch_list_mutex);
	if(g_event_add_switch_fun_num < EVENT_MAX_FUNCTION_NUM){
		g_event_add_switch_fun[g_event_add_switch_fun_num] = method;
		g_event_add_switch_fun_num++;
	}
	pthread_mutex_unlock(&g_event_add_switch_list_mutex);
	return;
};
//by:yhy 给交换机绑定删除函数
void register_delete_switch_function(event_delete_switch_fun method)
{
	pthread_mutex_lock(&g_event_delete_switch_list_mutex);
	if(g_event_delete_switch_fun_num < EVENT_MAX_FUNCTION_NUM)
	{
		g_event_delete_switch_fun[g_event_delete_switch_fun_num] = method;
		g_event_delete_switch_fun_num++;
	}
	pthread_mutex_unlock(&g_event_delete_switch_list_mutex);
	return;
};

void register_add_switch_port_function(event_add_switch_port_fun method){
	pthread_mutex_lock(&g_event_add_switch_port_list_mutex);
	if(g_event_add_switch_port_fun_num < EVENT_MAX_FUNCTION_NUM){
		g_event_add_switch_port_fun[g_event_add_switch_port_fun_num] = method;
		g_event_add_switch_port_fun_num++;
	}
	pthread_mutex_unlock(&g_event_add_switch_port_list_mutex);
	return;
};
void register_delete_switch_port_function(event_delete_switch_port_fun method){
	pthread_mutex_lock(&g_event_delete_switch_port_list_mutex);
	if(g_event_delete_switch_port_fun_num < EVENT_MAX_FUNCTION_NUM){
		g_event_delete_switch_port_fun[g_event_delete_switch_port_fun_num] = method;
		g_event_delete_switch_port_fun_num++;
	}
	pthread_mutex_unlock(&g_event_delete_switch_port_list_mutex);
	return;
};

void unregister_add_switch_function(event_add_switch_fun method){
	pthread_mutex_lock(&g_event_add_switch_list_mutex);
	UINT2 i = 0,j = 0;
	while( i < g_event_add_switch_fun_num ){
		if(g_event_add_switch_fun[i] == method){
			g_event_add_switch_fun[i] = NULL;
			i++;
		}else{
			g_event_add_switch_fun[j] = g_event_add_switch_fun[i];
			i++;
			j++;
		}
	}
	pthread_mutex_unlock(&g_event_add_switch_list_mutex);
	return;
};
void unregister_delete_switch_function(event_delete_switch_fun method){
	pthread_mutex_lock(&g_event_delete_switch_list_mutex);
	UINT2 i = 0,j = 0;
	while( i < g_event_delete_switch_fun_num ){
		if(g_event_delete_switch_fun[i] == method){
			g_event_delete_switch_fun[i] = NULL;
			i++;
		}else{
			g_event_delete_switch_fun[j] = g_event_delete_switch_fun[i];
			i++;
			j++;
		}
	}
	pthread_mutex_unlock(&g_event_delete_switch_list_mutex);
	return;
};
void unregister_add_switch_port_function(event_add_switch_port_fun method){
	pthread_mutex_lock(&g_event_add_switch_port_list_mutex);
	UINT2 i = 0,j = 0;
	while( i < g_event_add_switch_port_fun_num ){
		if(g_event_add_switch_port_fun[i] == method){
			g_event_add_switch_port_fun[i] = NULL;
			i++;
		}else{
			g_event_add_switch_port_fun[j] = g_event_add_switch_port_fun[i];
			i++;
			j++;
		}
	}
	pthread_mutex_unlock(&g_event_add_switch_port_list_mutex);
	return;
};
void unregister_delete_switch_port_function(event_delete_switch_port_fun method){
	pthread_mutex_lock(&g_event_delete_switch_port_list_mutex);
	UINT2 i = 0,j = 0;
	while( i < g_event_delete_switch_port_fun_num ){
		if(g_event_delete_switch_port_fun[i] == method){
			g_event_delete_switch_port_fun[i] = NULL;
			i++;
		}else{
			g_event_delete_switch_port_fun[j] = g_event_delete_switch_port_fun[i];
			i++;
			j++;
		}
	}
	pthread_mutex_unlock(&g_event_delete_switch_port_list_mutex);
	return;
};

// events create
//by:yhy g_event_add_switch_list中将当前交换机结构体装入,并更新拓补
void event_add_switch_on(gn_switch_t* sw){
	pthread_mutex_lock(&g_event_add_switch_list_mutex);
	//LOG_PROC("INFO", "event_add_switch_on");
	g_event_add_switch_list[g_event_add_switch_list_num] = sw;
	g_event_add_switch_list_num++;
	pthread_mutex_unlock(&g_event_add_switch_list_mutex);
	sem_post(&topo_change_thread_sem);
	return;
};
//by:yhy 触发交换机删除事件
void event_delete_switch_on(gn_switch_t* sw)
{
	pthread_mutex_lock(&g_event_delete_switch_list_mutex);
	//LOG_PROC("INFO", "event_delete_switch_on");
	g_event_delete_switch_list[g_event_delete_switch_list_num] = sw;
	g_event_delete_switch_list_num++;
	pthread_mutex_unlock(&g_event_delete_switch_list_mutex);
	//by:yhy 增加信号量,触发操作
	sem_post(&topo_change_thread_sem);
	return;
};
//by:yhy 网络拓补发生改变
void event_add_switch_port_on(gn_switch_t* sw,UINT4 port_no)
{
	pthread_mutex_lock(&g_event_add_switch_port_list_mutex);
	//LOG_PROC("INFO", "event_add_switch_port_on sw-dpid:%d",sw->dpid);
	g_event_add_switch_port_list[g_event_add_switch_port_list_num].sw = sw;
	g_event_add_switch_port_list[g_event_add_switch_port_list_num].port_no = port_no;
	g_event_add_switch_port_list_num++;
	pthread_mutex_unlock(&g_event_add_switch_port_list_mutex);
	sem_post(&topo_change_thread_sem);
};
void event_delete_switch_port_on(gn_switch_t* sw,UINT4 port_no){
	pthread_mutex_lock(&g_event_delete_switch_port_list_mutex);
	g_event_delete_switch_port_list[g_event_delete_switch_port_list_num].sw = sw;
	g_event_delete_switch_port_list[g_event_delete_switch_port_list_num].port_no = port_no;
	g_event_delete_switch_port_list_num++;
	pthread_mutex_unlock(&g_event_delete_switch_port_list_mutex);
	sem_post(&topo_change_thread_sem);
};
///////////////////////////////////////////////////
//by:yhy 拓补变动响应线程  响应交换机的增加删除,交换机端口的增加删除
void *topo_change_event_thread()
{
	//UINT4 sleep_times = 0;
	UINT4 last_add_sw_num = 0;
	UINT4 last_delete_sw_num = 0;
	UINT4 last_add_sw_port_num = 0;
	UINT4 last_delete_sw_port_num = 0;
	INT4 iRet = 0;
	//INT4 num = 0;
	prctl(PR_SET_NAME, (unsigned long) "TopoEventThread" ) ;  
	while(g_topo_change_thread_flag)
	{
		//sleep_times = 0;
		//by:yhy 等待信号量  
		iRet = sem_wait(&topo_change_thread_sem);
		if(0 != iRet)
		{
			int ierr= errno;
			LOG_PROC("ERROR", "Error: %s! %d errno=%d",FN, LN, ierr);
		}
		/*
		sem_getvalue(&topo_change_thread_sem, &num);
		if(num<500)
		{
			LOG_PROC("ERROR", "Error: %s! %d num=%d g_event_delete_switch_list_num=%d",FN, LN, num,g_event_delete_switch_list_num);
		}
		*/
		//why? 这个函数存疑
		init_openstack_fabric_auto_start_delay();
		////why? 为何等待存疑(目测此段代码是错误的)
		if(g_event_delete_switch_list_num)
		{//by:yhy 有交换机需要删除
			do
			{
				//by:yhy 下面几个操作赋值都是赋的对应操作的全局变量,此处为什么不是g_event_delete_switch_list_num
				last_delete_sw_num = g_event_delete_switch_list_num;
				//sleep(EVENT_SLEEP_DEFAULT_TIME);
				MsecSleep(EVENT_SLEEP_DEFAULT_TIME*100);
			}
			while(last_delete_sw_num < g_event_delete_switch_list_num );
			call_delete_switch_fun();
		}
		if(g_event_add_switch_list_num)
		{//by:yhy 有交换机增加
			do
			{
				last_add_sw_num = g_event_add_switch_list_num;
				//sleep(EVENT_SLEEP_DEFAULT_TIME);	
				MsecSleep(EVENT_SLEEP_DEFAULT_TIME*100);
				
			}
			while(last_add_sw_num < g_event_add_switch_list_num );
			//sleep_times = 5>g_event_add_switch_list_num?5:g_event_add_switch_list_num;
			call_add_switch_fun();
		}
		if(g_event_delete_switch_port_list_num)
		{//by:yhy 有交换机端口需要删除
			do
			{
				last_delete_sw_port_num = g_event_delete_switch_port_list_num;
				//sleep(EVENT_SLEEP_DEFAULT_TIME);
				
				MsecSleep(EVENT_SLEEP_DEFAULT_TIME*100);
			}
			while(last_delete_sw_port_num < g_event_delete_switch_port_list_num );
			call_delete_switch_port_fun();
		}
		if(g_event_add_switch_port_list_num)
		{//by:yhy 有交换机端口需要增加
			do
			{
				last_add_sw_port_num = g_event_add_switch_port_list_num;
				//sleep(EVENT_SLEEP_DEFAULT_TIME);
				
				MsecSleep(EVENT_SLEEP_DEFAULT_TIME*100);
			}
			while(last_add_sw_port_num < g_event_add_switch_port_list_num );
			//sleep(sleep_times);
			call_add_switch_port_fun();
		}
		last_delete_sw_num = 0;
		last_add_sw_num = 0;
		last_delete_sw_port_num = 0;
		last_add_sw_port_num = 0;
		
    }
	
	return NULL;
}

void of131_fabric_impl_remove_sws_state(gn_switch_t** swList, UINT4 num)
{
	UINT4 i = 0;
	gn_switch_t* sw = NULL;
	for(i = 0 ; i < num; i++)
	{
		sw = swList[i];
		
		sw->conn_state = INITSTATE;
	}
	return ;
}
//by:yhy 调用of131_fabric_impl_add_sws
//       添加交换机操作
void call_add_switch_fun()
{
	UINT2 i = 0;
	pthread_mutex_lock(&g_event_add_switch_list_mutex);
	//LOG_PROC("INFO", "call_add_switch_fun, sw num:%d",g_event_add_switch_list_num);
	// call function
	for(i=0;i < g_event_add_switch_fun_num;i++)
	{
		g_event_add_switch_fun[i](g_event_add_switch_list,g_event_add_switch_list_num);
	}
	// clear data
	memset(g_event_add_switch_list, 0, sizeof(gn_switch_t*)*EVENT_MAX_CHANGED_SWITCH_NUM);
	g_event_add_switch_list_num = 0;
	pthread_mutex_unlock(&g_event_add_switch_list_mutex);
	return;
};
//by:yhy 调用of131_fabric_impl_remove_sws
//       调用删除交换机的相关动作
void call_delete_switch_fun()
{
	UINT2 i = 0;
	pthread_mutex_lock(&g_event_delete_switch_list_mutex);
	//LOG_PROC("INFO", "call_delete_switch_fun, sw num:%d",g_event_delete_switch_list_num);
	// call function why? g_event_delete_switch_fun_num有可能大于g_event_delete_switch_fun[i]中i的最大值
	for(i=0;i < g_event_delete_switch_fun_num;i++)
	{//by:yhy 执行删除交换的相关动作
		g_event_delete_switch_fun[i](g_event_delete_switch_list,g_event_delete_switch_list_num);
	}
	of131_fabric_impl_remove_sws_state(g_event_delete_switch_list, g_event_delete_switch_list_num);
	// clear data
	memset(g_event_delete_switch_list, 0, sizeof(gn_switch_t*)*EVENT_MAX_CHANGED_SWITCH_NUM);
	g_event_delete_switch_list_num = 0;
	pthread_mutex_unlock(&g_event_delete_switch_list_mutex);
	return;
};
//by:yhy 调用of131_fabric_impl_add_sw_ports  
void call_add_switch_port_fun()
{
	UINT2 i = 0;
	//LOG_PROC("INFO", "call_add_switch_port_fun");
	// call function
	for(i=0;i < g_event_add_switch_port_fun_num;i++)
	{
		g_event_add_switch_port_fun[i](g_event_add_switch_port_list,g_event_add_switch_port_list_num);
	}
	
	pthread_mutex_lock(&g_event_add_switch_port_list_mutex);
	// clear data
	memset(g_event_add_switch_port_list, 0, sizeof(gn_switch_t*)*EVENT_MAX_CHANGED_SWITCH_NUM);
	g_event_add_switch_port_list_num = 0;
	pthread_mutex_unlock(&g_event_add_switch_port_list_mutex);
	return;
};
//by:yhy 调用of131_fabric_impl_delete_sw_ports
void call_delete_switch_port_fun()
{
	UINT2 i = 0;
	//LOG_PROC("INFO", "call_delete_switch_port_fun");
	// call function
	for(i=0;i < g_event_delete_switch_port_fun_num;i++)
	{
		g_event_delete_switch_port_fun[i](g_event_delete_switch_port_list,g_event_delete_switch_port_list_num);
	}
	pthread_mutex_lock(&g_event_delete_switch_port_list_mutex);
	// clear data
	memset(g_event_delete_switch_port_list, 0, sizeof(gn_switch_t*)*EVENT_MAX_CHANGED_SWITCH_NUM);
	g_event_delete_switch_port_list_num = 0;
	pthread_mutex_unlock(&g_event_delete_switch_port_list_mutex);
	return;
};

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

#include "event_service.h"
#include "gnflush-types.h"
////////////////////////////////////////////////////////////
event_add_switch_fun g_event_add_switch_fun[EVENT_MAX_FUNCTION_NUM];
event_delete_switch_fun g_event_delete_switch_fun[EVENT_MAX_FUNCTION_NUM];
event_add_switch_port_fun g_event_add_switch_port_fun[EVENT_MAX_FUNCTION_NUM];
event_delete_switch_port_fun g_event_delete_switch_port_fun[EVENT_MAX_FUNCTION_NUM];

UINT2 g_event_add_switch_fun_num = 0;
UINT2 g_event_delete_switch_fun_num = 0;
UINT2 g_event_add_switch_port_fun_num = 0;
UINT2 g_event_delete_switch_port_fun_num = 0;

gn_switch_t* g_event_add_switch_list[EVENT_MAX_CHANGED_SWITCH_NUM];
gn_switch_t* g_event_delete_switch_list[EVENT_MAX_CHANGED_SWITCH_NUM];
t_event_sw_port g_event_add_switch_port_list[EVENT_MAX_CHANGED_SWITCH_PORT_NUM];
t_event_sw_port g_event_delete_switch_port_list[EVENT_MAX_CHANGED_SWITCH_PORT_NUM];

UINT4 g_event_add_switch_list_num = 0;
UINT4 g_event_delete_switch_list_num = 0;
UINT4 g_event_add_switch_port_list_num = 0;
UINT4 g_event_delete_switch_port_list_num = 0;

static pthread_mutex_t g_event_add_switch_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_event_delete_switch_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_event_add_switch_port_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_event_delete_switch_port_list_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t topo_change_thread_id;
sem_t topo_change_thread_sem;
static UINT1 g_topo_change_thread_flag;

////////////////////////////////////////////////////////////
void *topo_change_event_thread();
void call_add_switch_fun();
void call_delete_switch_fun();
void call_add_switch_port_fun();
void call_delete_switch_port_fun();
////////////////////////////////////////////////////////////

// event thread start

void thread_topo_change_start(){
	LOG_PROC("INFO", "event_topo_change_thread_start");
	g_topo_change_thread_flag = 1;
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

void register_delete_switch_function(event_delete_switch_fun method){
	pthread_mutex_lock(&g_event_delete_switch_list_mutex);
	if(g_event_delete_switch_fun_num < EVENT_MAX_FUNCTION_NUM){
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

void event_add_switch_on(gn_switch_t* sw){
	pthread_mutex_lock(&g_event_add_switch_list_mutex);
	//LOG_PROC("INFO", "event_add_switch_on");
	g_event_add_switch_list[g_event_add_switch_list_num] = sw;
	g_event_add_switch_list_num++;
	pthread_mutex_unlock(&g_event_add_switch_list_mutex);
	sem_post(&topo_change_thread_sem);
	return;
};
void event_delete_switch_on(gn_switch_t* sw){
	pthread_mutex_lock(&g_event_delete_switch_list_mutex);
	//LOG_PROC("INFO", "event_delete_switch_on");
	g_event_delete_switch_list[g_event_delete_switch_list_num] = sw;
	g_event_delete_switch_list_num++;
	pthread_mutex_unlock(&g_event_delete_switch_list_mutex);
	sem_post(&topo_change_thread_sem);
	return;
};
void event_add_switch_port_on(gn_switch_t* sw,UINT4 port_no){
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
void *topo_change_event_thread(){
	//UINT4 sleep_times = 0;
	UINT4 last_add_sw_num = 0;
	UINT4 last_delete_sw_num = 0;
	UINT4 last_add_sw_port_num = 0;
	UINT4 last_delete_sw_port_num = 0;
	while(g_topo_change_thread_flag){
		//sleep_times = 0;
		sem_wait(&topo_change_thread_sem);
		LOG_PROC("INFO", "topo_change_event_thread start");
		// init rate

		if(g_event_delete_switch_list_num){
			do{
				last_delete_sw_num = g_event_add_switch_list_num;
				sleep(EVENT_SLEEP_DEFAULT_TIME);
			}while(last_delete_sw_num < g_event_add_switch_list_num );
			call_delete_switch_fun();
		}

		if(g_event_add_switch_list_num){
			do{
				last_add_sw_num = g_event_add_switch_list_num;
				sleep(EVENT_SLEEP_DEFAULT_TIME);
			}while(last_add_sw_num < g_event_add_switch_list_num );
			//sleep_times = 5>g_event_add_switch_list_num?5:g_event_add_switch_list_num;
			call_add_switch_fun();
		}

		if(g_event_delete_switch_port_list_num){
			do{
				last_delete_sw_port_num = g_event_delete_switch_port_list_num;
				sleep(EVENT_SLEEP_DEFAULT_TIME);
			}while(last_delete_sw_port_num < g_event_delete_switch_port_list_num );
			call_delete_switch_port_fun();
		}

		if(g_event_add_switch_port_list_num){
			do{
				last_add_sw_port_num = g_event_add_switch_port_list_num;
				sleep(EVENT_SLEEP_DEFAULT_TIME);
			}while(last_add_sw_port_num < g_event_add_switch_port_list_num );
			//sleep(sleep_times);
			call_add_switch_port_fun();
		}
    }
	return NULL;
}

void call_add_switch_fun(){
	UINT2 i = 0;
	pthread_mutex_lock(&g_event_add_switch_list_mutex);
	LOG_PROC("INFO", "call_add_switch_fun, sw num:%d",g_event_add_switch_list_num);
	// call function
	for(i=0;i < g_event_add_switch_fun_num;i++){
		g_event_add_switch_fun[i](g_event_add_switch_list,g_event_add_switch_list_num);
	}
	// clear data
	memset(g_event_add_switch_list, 0, sizeof(gn_switch_t*)*EVENT_MAX_CHANGED_SWITCH_NUM);
	g_event_add_switch_list_num = 0;
	pthread_mutex_unlock(&g_event_add_switch_list_mutex);
	return;
};
void call_delete_switch_fun(){
	UINT2 i = 0;
	pthread_mutex_lock(&g_event_delete_switch_list_mutex);
	LOG_PROC("INFO", "call_delete_switch_fun, sw num:%d",g_event_delete_switch_list_num);
	// call function
	for(i=0;i < g_event_delete_switch_fun_num;i++){
		g_event_delete_switch_fun[i](g_event_delete_switch_list,g_event_delete_switch_list_num);
	}
	// clear data
	memset(g_event_delete_switch_list, 0, sizeof(gn_switch_t*)*EVENT_MAX_CHANGED_SWITCH_NUM);
	g_event_delete_switch_list_num = 0;
	pthread_mutex_unlock(&g_event_delete_switch_list_mutex);
	return;
};
void call_add_switch_port_fun(){
	UINT2 i = 0;
	pthread_mutex_lock(&g_event_add_switch_port_list_mutex);
	LOG_PROC("INFO", "call_add_switch_port_fun");
	// call function
	for(i=0;i < g_event_add_switch_port_fun_num;i++){
		g_event_add_switch_port_fun[i](g_event_add_switch_port_list,g_event_add_switch_port_list_num);
	}
	// clear data
	memset(g_event_add_switch_port_list, 0, sizeof(gn_switch_t*)*EVENT_MAX_CHANGED_SWITCH_NUM);
	g_event_add_switch_port_list_num = 0;
	pthread_mutex_unlock(&g_event_add_switch_port_list_mutex);
	return;
};
void call_delete_switch_port_fun(){
	UINT2 i = 0;
	pthread_mutex_lock(&g_event_delete_switch_port_list_mutex);
	LOG_PROC("INFO", "call_delete_switch_port_fun");
	// call function
	for(i=0;i < g_event_delete_switch_port_fun_num;i++){
		g_event_delete_switch_port_fun[i](g_event_delete_switch_port_list,g_event_delete_switch_port_list_num);
	}
	// clear data
	memset(g_event_delete_switch_port_list, 0, sizeof(gn_switch_t*)*EVENT_MAX_CHANGED_SWITCH_NUM);
	g_event_delete_switch_port_list_num = 0;
	pthread_mutex_unlock(&g_event_delete_switch_port_list_mutex);
	return;
};

/******************************************************************************
*                                                                             *
*   File Name   : event-service.c           *
*   Author      :            *
*   Create Date : 2015-4-29           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "event_service.h"
#include "gnflush-types.h"

void fabric_add_sw();
void fabric_del_sw();
void fabric_port_add();
void fabric_del_port();

foo foo1;

gn_switch_t** fabric_sw_add_array;
gn_switch_t** fabric_sw_del_array;
gn_switch_t** fabric_port_add_array;
gn_switch_t** fabric_port_del_array;

void fabric_register_add_sw_function(void *method)
{
	  foo1= method;
}
void fabric_thread_start(){
      LOG_PROC("INFO", "fabric_thread_start");
	  pthread_t mh;
	  pthread_create(&mh,NULL,fabric_thread_foo,NULL);
	  LOG_PROC("INFO", "fabric_thread_start_finish");
}
void *fabric_thread_foo(void *arg){
    while(1){
	   if(foo1){
         fabric_add_sw();
         fabric_del_sw();
		 fabric_del_port();
         fabric_port_add();
		 LOG_PROC("INFO", "fabric_thread_foo 1 times done. no error");
        }else{
		 LOG_PROC("INFO", "fabric_thread_foo 1 times done. no foo1 register");
		}
		sleep(5);
    }
}
void fabric_add_sw(){
	UINT4 i = 0;
	int len = sizeof(fabric_sw_add_array);
    for(i=0;i<len;i++){
      (*foo1)(fabric_sw_add_array[i],1);
    }
	if(len){
	    free(fabric_sw_add_array);
	}
	i = 0;
	void * recalloc(gn_switch_t* fabric_sw_add_array,unsigned int i);
}
void fabric_del_sw(){
	UINT4 i = 0;
	int len = sizeof(fabric_sw_del_array);
    for(i=0;i<len;i++){
      (*foo1)(fabric_sw_del_array[i],1);
    }
	if(len){
	    free(fabric_sw_del_array);
	}
	i = 0;
	void * recalloc(gn_switch_t* fabric_sw_del_array,unsigned int i);
}

void fabric_port_add(){
	UINT4 i = 0;
	int len = sizeof(fabric_port_add_array);
    for(i=0;i<len;i++){
      (*foo1)(fabric_port_add_array[i],1);
    }
	if(len){
	    free(fabric_port_add_array);
	}
	i = 0;
	void * recalloc(gn_switch_t* fabric_port_add_array,unsigned int i);
}
void fabric_del_port(){
	UINT4 i = 0;
	int len = sizeof(fabric_port_del_array);
    for(i=0;i<len;i++){
      (*foo1)(fabric_port_del_array[i],1);
    }
	if(len){
	    free(fabric_port_del_array);
	}
	i = 0;
	void * recalloc(gn_switch_t* fabric_port_del_array,unsigned int i);
}
void put_into_array(gn_switch_t* p){
	int len = sizeof(fabric_sw_add_array)/sizeof(unsigned int)+1;
	void * recalloc(gn_switch_t* fabric_sw_add_array,unsigned int len);
	fabric_sw_add_array[len] = p;
}
void delete_from_array(gn_switch_t* p){
	int len = sizeof(fabric_sw_del_array)/sizeof(unsigned int)+1;
	void * recalloc(gn_switch_t* fabric_sw_del_array,unsigned int len);
	fabric_sw_del_array[len] = p;
}
void put_into_port_array(gn_switch_t* p){
	int len = sizeof(fabric_port_add_array)/sizeof(unsigned int)+1;
	void * recalloc(gn_switch_t* fabric_port_add_array,unsigned int len);
	fabric_port_add_array[len] = p;
}
void delete_from_port_array(gn_switch_t* p){
	int len = sizeof(fabric_port_del_array)/sizeof(unsigned int)+1;
	void * recalloc(gn_switch_t* fabric_port_del_array,unsigned int len);
	fabric_port_del_array[len] = p;
}

/*
 * event_service.h
 *
 *  Created on: May 14, 2015
 *      Author: joe
 *  Modify Jonathan
 */

#ifndef SRC_EVENT_EVENT_SERVICE_H_
#define SRC_EVENT_EVENT_SERVICE_H_

#include "gnflush-types.h"

#define EVENT_MAX_FUNCTION_NUM 4
#define EVENT_MAX_CHANGED_SWITCH_NUM 1024
#define EVENT_MAX_CHANGED_SWITCH_PORT_NUM 10240

typedef struct event_sw_port{
	gn_switch_t* sw;
	UINT4 port_no;
}t_event_sw_port,*p_event_sw_port;

typedef void (*event_add_switch_fun)(gn_switch_t** swList,UINT4 num);
typedef void (*event_delete_switch_fun)(gn_switch_t** swList,UINT4 num);
typedef void (*event_add_switch_port_fun)(p_event_sw_port sw_portList,UINT4 num);
typedef void (*event_delete_switch_port_fun)(p_event_sw_port sw_portList,UINT4 num);

void thread_topo_change_start();
void thread_topo_change_stop();

void register_add_switch_function(event_add_switch_fun method);
void register_delete_switch_function(event_delete_switch_fun method);
void register_add_switch_port_function(event_add_switch_port_fun method);
void register_delete_switch_port_function(event_delete_switch_port_fun method);
void unregister_add_switch_function(event_add_switch_fun method);
void unregister_delete_switch_function(event_delete_switch_fun method);
void unregister_add_switch_port_function(event_add_switch_port_fun method);
void unregister_delete_switch_port_function(event_delete_switch_port_fun method);

void event_add_switch_on(gn_switch_t* sw);
void event_delete_switch_on(gn_switch_t* sw);
void event_add_switch_port_on(gn_switch_t* sw,UINT4 port_no);
void event_delete_switch_port_on(gn_switch_t* sw,UINT4 port_no);
#endif /* SRC_EVENT_EVENT_SERVICE_H_ */

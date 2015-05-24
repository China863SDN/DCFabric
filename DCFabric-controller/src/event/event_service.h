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

#ifndef SRC_EVENT_EVENT_SERVICE_H_
#define SRC_EVENT_EVENT_SERVICE_H_

#include "gnflush-types.h"

#define EVENT_MAX_FUNCTION_NUM 4
#define EVENT_MAX_CHANGED_SWITCH_NUM 1024
#define EVENT_MAX_CHANGED_SWITCH_PORT_NUM 10240
#define EVENT_SLEEP_DEFAULT_TIME 1

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

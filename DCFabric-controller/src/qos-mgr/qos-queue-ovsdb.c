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
*   File Name   : qos-mgr.c           *
*   Author      : bnc Administrator           *
*   Create Date : 2016-05-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "qos-queue-ovsdb.h"
#include "gnflush-types.h"
#include "../conn-svr/conn-svr.h"
#include "fabric_impl.h"
#include "../ovsdb/ovsdb.h"

/* global define */
#define STR_LEN_MAX 500
#define STR_LEN_EXTERNAL 50

extern t_fabric_sw_list g_fabric_sw_list;

/* internal function */

/*
 * this function is used to send query to ovsdb
 *
 * @brief: this function is used to send query to ovsdb,
 *            this function will connect the input string with the header contains id header.
 *
 * @param: sw				the switch
 * @param: content			query content
 * @param: id					query id 
 *
 * @return: INT4				GN_OK: success; GN_ERR: fail
 */
INT4 send_query(gn_switch_t* sw, INT1* content, INT1* id);

/*
 * set queue uuid
 *
 * @brief: this function is used to set queue uuid
 *
 * @param: sw				the switch
 * @param: port_no			the port no
 * @param: queue_id			the queue id
 * @param: uuid				the uuid of queue
 *
 * @return: INT4				GN_OK: success; GN_ERR: fail
 */
INT4 set_queue_uuid(gn_switch_t* sw, UINT4 port_no, UINT4 queue_id, INT1* uuid);

/*
 * set qos uuid
 *
 * @brief: this function is used to set qos uuid
 *
 * @param: sw				the switch
 * @param: port_no			the port no
 * @param: qos_uuid			the uuid of qos
 *
 * @return: INT4				GN_OK:success; GN_ERR: fail
 */
INT4 set_qos_uuid(gn_switch_t* sw, UINT4 port_no, INT1* qos_uuid);

/*
 * set interface uuid
 *
 * @breif: this function is used to set interface uuid
 *
 * @param: sw				the switch
 * @param: port_no			the port no
 * @param: interface_uuid		the uuid of interface
 * 
 * @return: INT4				GN_OK:success; GN_ERR: fail
 */
INT4 set_interface_uuid(gn_switch_t* sw, UINT4 port_no, INT1* interface_uuid);

/*
 * set port uuid
 *
 * @brief: this function is used to set port uuid
 *
 * @param: port_uuid			the uuid of port
 * @param: interface_uuid		the uuid of interface
 *
 * @return: INT4				GN_OK: success; GN_ERR: fail
 */
INT4 set_port_uuid(INT1* port_uuid, INT1* interface_uuid);

/* function body */

// this function is used to send query to ovsdb
INT4 send_query(gn_switch_t* sw, INT1* content, INT1* id)
{  
	if ((NULL == sw) || (NULL == content) || (NULL == id)) {
		return GN_ERR;
	}

	INT4 conn_fd = get_conn_fd_by_sw_ip(sw->sw_ip);
	
	if (0 == conn_fd) {
		LOG_PROC("INFO", "Fail to find ip:%s ovsdb port", inet_ntoa(*(struct in_addr*)&sw->sw_ip));
		return GN_ERR;
	}

	INT1 pre_transact[] = {"{\"id\":\"%s\",\"method\":\"transact\",\"params\":%s}"};

	INT1 query_str[STR_LEN_MAX];
		
	sprintf(query_str, pre_transact, id, content); 			

	// printf("%s\n", query_str);
	send(conn_fd, query_str, strlen(query_str), 0);
	
	return GN_OK;
}

INT4 send_query_fd(INT4 conn_fd, INT1* content, INT1* id)
{
	INT1 pre_transact[] = {"{\"id\":\"%s\",\"method\":\"transact\",\"params\":%s}"};

	INT1 query_str[STR_LEN_MAX];
		
	sprintf(query_str, pre_transact, id, content); 			

	// printf("%s\n", query_str);
	
	send(conn_fd, query_str, strlen(query_str), 0);
	
	return GN_OK;
}


// this function is used to create queue table
INT4 add_queue_in_queue_table(gn_switch_t* sw, gn_queue_t* queue)
{
	if ((NULL == sw) || (NULL == queue)) {
		return GN_ERR;
	}
	
	INT1 query_str[STR_LEN_MAX];

	INT1 dpid_str[48];
	dpidUint8ToStr(sw->dpid, dpid_str);

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"insert\",\"table\":\"Queue\","
		"\"row\":{\"dscp\":[\"set\",[]],"
		"\"external_ids\":[\"map\",[[\"sw-dpid\",\"%s\"],[\"port-no\",\"%d\"],[\"queue-id\",\"%d\"]]],"
		"\"other_config\":[\"map\",[[\"min-rate\",\"%llu\"],[\"max-rate\",\"%llu\"],[\"priority\",\"%d\"],"
		"[\"burst\",\"%llu\"]]]}}]"};
		
	sprintf(query_str, text, dpid_str, queue->port_no, queue->queue_id, queue->min_rate, queue->max_rate, queue->priority, queue->burst); 

	send_query(sw, query_str, "08");
	
	queue->next = sw->queue_entries;
	sw->queue_entries = queue;

	return GN_OK;
}

// create default queue in queue table
INT4 create_default_queue_in_queue_table(gn_switch_t* sw, UINT4 port_no)
{
	if ((NULL == sw) || (0 == port_no)) {
		return GN_ERR;
	}
	
	gn_queue_t* queue = (gn_queue_t*)gn_malloc(sizeof(gn_queue_t));

	if (NULL == queue) {
		return GN_ERR;
	}

	queue->queue_id = 0;
	queue->min_rate = 0;
	queue->max_rate = 10000000000;
	queue->burst = queue->max_rate;
	queue->priority = 1;
	queue->port_no = port_no;

	add_queue_in_queue_table(sw, queue);

	return GN_OK;
}

// update queue in queue table
INT4 update_queue_in_queue_table(gn_switch_t* sw, gn_queue_t* queue)
{
	if ((NULL == sw) || (NULL == queue)) {
		LOG_PROC("INFO", "Fail to update queue in queue table.please check sw and queue.");
		return GN_ERR;
	}


	INT1 query_str[STR_LEN_MAX];

	INT1 text[] = {"[\"Open_vSwitch\","
		"{\"op\":\"update\",\"table\":\"Queue\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]],"
		"\"row\":{\"other_config\":[\"map\",[[\"min-rate\",\"%llu\"],[\"max-rate\",\"%llu\"],"
		"[\"priority\",\"%d\"],[\"burst\",\"%llu\"]]]}}]"};

	sprintf(query_str, text, queue->queue_uuid, queue->min_rate, queue->max_rate, queue->priority, queue->burst); 

	send_query(sw, query_str, "08");

	return GN_OK;

}

// this function is used to delete queue in queue table
INT4 delete_queue_in_queue_table(gn_switch_t* sw, gn_queue_t* queue)
{
	/* 
	 * example 
	 * ["Open_vSwitch",{"op":"delete","table":"Queue","where":[["_uuid","==",["uuid","e4c0989b-95b2-4efe-83f5-cda733f3c354"]]]}]
	 */
	 
	if ((NULL == sw) || (0 == queue->queue_uuid)) {
		LOG_PROC("INFO", "Fail to delete queue in queue table. please check sw and queue id.");
		return GN_ERR;
	}

	INT1 query_str[STR_LEN_MAX];

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"delete\",\"table\":\"Queue\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]]}]"};

	sprintf(query_str, text, queue->queue_uuid);

	send_query(sw, query_str, "08");

	return GN_OK;
}

// clear qos in qos table
INT4 clear_queue_in_queue_table(INT4 conn_fd)
{	
	if (0 == conn_fd) {
		return GN_ERR;
	}

	INT1 query_str[STR_LEN_MAX];

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"delete\",\"table\":\"Queue\",\"where\":[]}]"};

	strcpy(query_str, text);

	send_query_fd(conn_fd, query_str, "08");	
	
	return GN_OK;
}

// add queue to qos table
INT4 add_queue_to_qos_table(gn_switch_t* sw, INT1* qos_uuid, UINT4 queue_id, INT1* queue_uuid)
{
	/*
	  * example
	  * ["Open_vSwitch",{"op":"mutate","table":"QoS","where":[["_uuid","==",["uuid","a86f68bc-334f-411c-a44e-6b4e9ea24f8d"]]]
	  * ,"mutations":[["queues","insert",["map",[[1,["uuid","1ba592f7-6845-4a2e-8127-3aa1426805ca"]]]]]]}]
	  */
	  
	if ((NULL == sw) || (NULL == qos_uuid) || (NULL == queue_uuid) || (0 == strlen(qos_uuid)) 
		|| (0 == strlen(queue_uuid))) {
		return GN_ERR;
	}

	INT1 query_str[STR_LEN_MAX];

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"mutate\",\"table\":\"QoS\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]],"
		"\"mutations\":[[\"queues\",\"insert\",[\"map\",[[%d,[\"uuid\",\"%s\"]]]]]]}]"};

	sprintf(query_str, text, qos_uuid, queue_id, queue_uuid);

	send_query(sw, query_str, "08");

	return GN_OK;
}


//this function is used to delete queue in qos table
INT4 delete_queue_in_qos_table(gn_switch_t* sw, INT1* qos_uuid, gn_queue_t* queue)
{
	/*
	 * example
	 *  ["Open_vSwitch",{"op":"mutate","table":"QoS","where":[["_uuid","==",["uuid","a86f68bc-334f-411c-a44e-6b4e9ea24f8d"]]],
	 * "mutations":[["queues","delete",["map",[[0,["uuid","1ba592f7-6845-4a2e-8127-3aa1426805ca"]]]]]]}]
	 */
	 
	if ((NULL == sw) || (0 == strlen(qos_uuid)) || (NULL == queue)) {
		LOG_PROC("INFO", "Fail to delete queue in qos table. Please check sw and queue.");
		return GN_ERR;
	}

	INT1 query_str[STR_LEN_MAX];

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"mutate\",\"table\":\"QoS\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]],"
		"\"mutations\":[[\"queues\",\"delete\",[\"map\",[[%d,[\"uuid\",\"%s\"]]]]]]}]"};

	sprintf(query_str, text, qos_uuid, queue->queue_id, queue->queue_uuid);

	send_query(sw, query_str, "08");

	return GN_OK;
}


// this function is used to create qos in qos table
INT4 add_qos_in_qos_table(gn_switch_t* sw, gn_qos_t* qos)
{
	INT1 query_str[STR_LEN_MAX];		

	INT1 dpid_str[48];
	dpidUint8ToStr(sw->dpid, dpid_str);	

	INT1 text[] = {"[\"Open_vSwitch\","
	"{\"op\":\"insert\",\"table\":\"QoS\",\"row\":"
	"{\"external_ids\":[\"map\",[[\"sw-dpid\",\"%s\"],[\"port-no\",\"%d\"]]],"
	"\"other_config\":[\"map\",[]],\"queues\":[\"map\",[]],\"type\":[\"set\",[\"linux-htb\"]]}}]"};

	sprintf(query_str, text, dpid_str, qos->port_no); 	

	send_query(sw, query_str, "08");

	qos->next = sw->qos_entries;
	sw->qos_entries = qos;

	return GN_OK;
}

// clear qos in qos table
INT4 clear_qos_in_qos_table(INT4 conn_fd)
{	
	if (0 == conn_fd) {
		return GN_ERR;
	}

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"delete\",\"table\":\"QoS\",\"where\":[]}]"};

	send_query_fd(conn_fd, text, "08");	
	
	return GN_OK;
}


// this function is used to add qos to port table
INT4 add_qos_to_port_table(gn_switch_t* sw, INT1* port_uuid, INT1* qos_uuid)
{
	/*
	 * example
	 * [\"Open_vSwitch\",{\"op\":\"update\",\"table\":\"Port\",\"where\":[[\"_uuid\",\"==
	 * \",[\"uuid\",\"917d741f-f0c9-4cc0-a94c-7536e48689fe\"]]],\"row\":{\"qos\":[\"set\",
	 * [[\"uuid\",\"7a61aacf-d7e8-423b-917a-3f83011561d0\"]]]}}]
	 */
	 
	if ((NULL == sw) || (NULL == port_uuid) || (NULL == qos_uuid)
		|| (0 == strlen(port_uuid)) || (0 == strlen(qos_uuid))) {
		return GN_ERR;
	}

	INT1 query_str[STR_LEN_MAX];
	
	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"update\",\"table\":\"Port\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]],"
		"\"row\":{\"qos\":[\"set\",[[\"uuid\",\"%s\"]]]}}]"};

	sprintf(query_str, text, port_uuid, qos_uuid);

	send_query(sw, query_str, "08");

	return GN_OK;
}

// clear qos in port table
INT4 clear_qos_in_port_table(INT4 conn_fd, INT1* port_uuid)
{
	if ((0 == conn_fd) || (NULL == port_uuid)) {
		return GN_ERR;
	}

	INT1 query_str[STR_LEN_MAX];

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"update\",\"table\":\"Port\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]],"
		"\"row\":{\"qos\":[\"set\",[]]}}]"};
	

	sprintf(query_str, text, port_uuid);
	
	send_query_fd(conn_fd, query_str, "08");	
	
	return GN_OK;
}


// search interface by port no
INT4 search_interface_by_port_no(gn_switch_t* sw, UINT4 port_no)
{
	/* 
	 * example
	*  [\"Open_vSwitch\",{\"op\":\"select\",\"table\":\"Interface\",\"where\":[[\"ofport\",\"==\",25]],\"columns\":[\"_uuid\",\"ofport\"]}]
	 */
	if ((NULL == sw) || (0 == port_no)) {
		return GN_ERR;
	}

	INT1 query_str[STR_LEN_MAX];

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"select\",\"table\":\"Interface\","
		"\"where\":[[\"ofport\",\"==\",%d]],\"columns\":[\"_uuid\",\"ofport\",\"mac_in_use\"]}]"};

	sprintf(query_str, text, port_no);

	send_query(sw, query_str, "09");

	return GN_OK;
}

// search host by interace uuid
INT4 search_port_by_interface(gn_switch_t* sw, INT1* interface_uuid)
{
	/*
	 * example
	 * [\"Open_vSwitch\",{\"op\":\"select\",\"table\":\"Port\",\"where\":[[\"interfaces\",\"==\",
	 * [\"uuid\",\"10f326ab-dae4-4acc-8136-24036f26e807\"]]],\"columns\":[\"_uuid\",\"interfaces\"]}]
	 */
	if ((NULL == sw) || (NULL == interface_uuid) || (0 == strlen(interface_uuid))) {
		return GN_ERR;
	}

	INT1 query_str[STR_LEN_MAX];

	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"select\",\"table\":\"Port\","
		"\"where\":[[\"interfaces\",\"==\",[\"uuid\",\"%s\"]]],\"columns\":[\"_uuid\",\"interfaces\"]}]"};

	sprintf(query_str, text, interface_uuid);

	send_query(sw, query_str, "10");
	
	return GN_OK;
}

// receive queue uuid notify
INT4 notify_recevice_queue_uuid(json_t* queue)
{
	if ((NULL == queue) || (NULL == queue->child) || (NULL == queue->child->child) 
		|| (NULL == queue->child->child->text) || (NULL == queue->child->child->child->child) 
		|| (NULL == queue->child->child->child->child->child)) {
		return 0;
	}
	
	json_t* queue_json = json_find_first_label(queue->child->child->child->child->child, "external_ids");

	if ((NULL == queue_json) || (NULL == queue_json->child) || (NULL == queue_json->child->child)
		|| (NULL == queue_json->child->child->next) || (NULL == queue_json->child->child->next->child)
		|| (NULL == queue_json->child->child->next->child->child)) {
		return 0;
	}

	// port_no json
	json_t* port_json = queue_json->child->child->next->child->child;

	if ((NULL == port_json) || (NULL ==  port_json->next) || (NULL == port_json->next->text)) {
		return 0;
	}

	// queue id json
	json_t* id_json = queue_json->child->child->next->child->next->child;

	if ((NULL == id_json) || (NULL ==  id_json->next) || (NULL == id_json->next->text)) {
		return 0;
	}

	// sw dpid json
	json_t* sw_json = queue_json->child->child->next->child->next->next->child;

	if ((NULL == sw_json) || (NULL == sw_json->text) || (NULL == sw_json->next->text)) {
		return 0;
	}

	// queue_id
	// printf("%s : %s\n", port_json->text, port_json->next->text);
	
	// queue_id
	// printf("%s : %s\n", id_json->text, id_json->next->text);

	// sw dpid
	// printf("%s : %s\n", sw_json->text, sw_json->next->text);

	// queue uuid
	// printf("uuid: %s\n", queue->child->child->text);

	UINT8 sw_dpid;
	dpidStr2Uint8(sw_json->next->text, &sw_dpid);	

	gn_switch_t* sw = find_sw_by_dpid(sw_dpid);

	UINT4 queue_id = atoi(id_json->next->text);
	
	UINT4 port_no = atoi(port_json->next->text);
	
	set_queue_uuid(sw, port_no, queue_id, queue->child->child->text);
	
	return GN_OK;
}

// receive qos uuid
INT4 notify_recevice_qos_uuid(json_t* qos)
{
	if ((NULL == qos) || (NULL == qos->child) || (NULL == qos->child->child) 
		|| (NULL == qos->child->child->text) || (NULL == qos->child->child->child->child) 
		|| (NULL == qos->child->child->child->child->child)) {
		return 0;
	}
	
	json_t* qos_json = json_find_first_label(qos->child->child->child->child->child, "external_ids");

	if ((NULL == qos_json) || (NULL == qos_json->child) || (NULL == qos_json->child->child)
		|| (NULL == qos_json->child->child->next) || (NULL == qos_json->child->child->next->child)
		|| (NULL == qos_json->child->child->next->child->child)) {
		return 0;
	}

	// port no
	json_t* port_json = qos_json->child->child->next->child->child;

	if ((NULL == port_json) || (NULL ==  port_json->next) || (NULL == port_json->next->text)) {
		return 0;
	}

	// sw dpid
	json_t* sw_json = qos_json->child->child->next->child->next->child;

	if ((NULL == sw_json) || (NULL == sw_json->text) || (NULL == sw_json->next->text)) {
		return 0;
	}
	
	// port no
	// printf("%s : %s\n", port_json->text, port_json->next->text);

	// sw dpid
	// printf("%s : %s\n", sw_json->text, sw_json->next->text);

	// qos uuid
	// printf("uuid: %s\n", qos->child->child->text);

	UINT8 sw_dpid;
	dpidStr2Uint8(sw_json->next->text, &sw_dpid);	

	gn_switch_t* sw = find_sw_by_dpid(sw_dpid);

	UINT4 port_no = atoi(port_json->next->text);

	if (sw) {
		set_qos_uuid(sw, port_no, qos->child->child->text);
	}

	return GN_OK;
}

// receive interface uuid
INT4 notify_receive_interface_uuid(json_t* interface)
{
	if ((NULL == interface)) {
		return GN_ERR;
	}

	json_t* tmp, *row;
	if ((interface) && (interface->child) && (interface->child->child)) {
			tmp = interface->child->child;
			row = json_find_first_label(tmp, "rows");
		}

	// find interface 
	if ((row) && (row->child) && (row->child->child))
	{	
		tmp = row->child->child;
		if ((tmp) && (tmp->child) && (tmp->child->next) && (tmp->child->next->child) 
			&& (tmp->child->next->child->text) && (tmp->child->child->child->next->text)){
			// uuid
			// printf("%s\n", tmp->child->child->child->next->text);
			// port no
			// printf("%s\n", tmp->child->next->child->text);
			// mac in use
			// printf("%s\n", tmp->child->next->next->child->text);

			UINT1 phy_mac[6] = {0};
			macstr2hex(tmp->child->next->next->child->text, phy_mac);

			gn_switch_t* sw = find_sw_by_port_physical_mac(phy_mac);

			UINT4 port_no = atoi(tmp->child->next->child->text);
			
			set_interface_uuid(sw, port_no, tmp->child->child->child->next->text);
		}
	}

	return GN_OK;
}

// recieve port uuid
INT4 notify_receive_port_uuid(json_t* port)
{
	if ((NULL == port)) {
		return GN_ERR;
	}

	json_t* tmp, *row;
	if ((port) && (port->child) && (port->child->child)) {
			tmp = port->child->child;
			row = json_find_first_label(tmp, "rows");
		}

	// find interface 
	if ((row) && (row->child) && (row->child->child))
	{	
		tmp = row->child->child;
		if (tmp) {
			// uuid
			// printf("%s\n", tmp->child->child->child->next->text);
			// interface uuid
			// printf("%s\n", tmp->child->next->child->child->next->text);
			// set port uuid

			set_port_uuid(tmp->child->child->child->next->text, tmp->child->next->child->child->next->text);
		}
	}

	return GN_OK;
}


// set the queue uuid
INT4 set_queue_uuid(gn_switch_t* sw, UINT4 port_no, UINT4 queue_id, INT1* uuid)
{
	if ((NULL == sw) || (0 == port_no) || (NULL == uuid) || (0 == strlen(uuid))) {
		return GN_ERR;
	}

	INT1* qos_uuid = NULL;
	
	gn_qos_t* qos = sw->qos_entries;

	while (qos) {
		if (qos->port_no == port_no)
			qos_uuid = qos->qos_uuid;

		qos = qos->next;
	}

	// printf("%d, %d, %s, %s\n", port_no, queue_id, qos_uuid, uuid);

	gn_queue_t* queue = sw->queue_entries;

	while (queue) {
		if ((queue->port_no == port_no) && (queue->queue_id == queue_id)) {
			strcpy(queue->queue_uuid, uuid);
			
			add_queue_to_qos_table(sw, qos_uuid, queue->queue_id, queue->queue_uuid);
		}

		queue = queue->next;
	}

	return GN_OK;
}

// set qos uuid
INT4 set_qos_uuid(gn_switch_t* sw, UINT4 port_no, INT1* qos_uuid)
{
	if ((NULL == sw) || (0 == port_no) || (NULL == qos_uuid) || (0 == strlen(qos_uuid))) {
		return GN_ERR;
	}

	gn_qos_t* qos = sw->qos_entries;

	while (qos) {
		if (qos->port_no == port_no) {
			strcpy(qos->qos_uuid, qos_uuid);

			if (0 == strlen(qos->interface_uuid)) {
				search_interface_by_port_no(sw, port_no);
			}
		}
		qos = qos->next;
	}

	gn_queue_t* queue = sw->queue_entries;

	while (queue) {
		if (queue->port_no == port_no) {
			add_queue_to_qos_table(sw, qos_uuid, queue->queue_id, queue->queue_uuid);
		}
		
		queue = queue->next;
	}

	return GN_OK;
}

// set interface uuid
INT4 set_interface_uuid(gn_switch_t* sw, UINT4 port_no, INT1* interface_uuid)
{
	if ((NULL == sw) || (0 == port_no) || (NULL == interface_uuid) || (0 == strlen(interface_uuid))) {
		return GN_ERR;
	}
	
	gn_qos_t* qos = sw->qos_entries;

	while (qos) {
		if (qos->port_no == port_no) {
			strcpy(qos->interface_uuid, interface_uuid);

			if (0 == strlen(qos->port_uuid)) {
				search_port_by_interface(sw, interface_uuid);
			}
		}
		qos = qos->next;
	}

	return GN_OK;
}

// set port uuid
INT4 set_port_uuid(INT1* port_uuid, INT1* interface_uuid)
{
	if ((NULL == port_uuid) || (NULL == interface_uuid)) {
		return GN_ERR;
	}

	p_fabric_sw_list list = NULL;
	list = &g_fabric_sw_list;

	p_fabric_sw_node sw_node = list->node_list;

	while (sw_node) {
		gn_switch_t* sw = sw_node->sw;

		if ((sw) && (sw->qos_entries)) {
			if (0 == strcmp(interface_uuid, sw->qos_entries->interface_uuid)) {
				strcpy(sw->qos_entries->port_uuid, port_uuid);

				if ((sw) && (strlen(port_uuid)) && (strlen(sw->qos_entries->qos_uuid))) {
					sw->qos_entries->status = 1;
					add_qos_to_port_table(sw, port_uuid, sw->qos_entries->qos_uuid);
				}
			}
		}
		
		sw_node = sw_node->next;
	}

	return GN_OK;
}

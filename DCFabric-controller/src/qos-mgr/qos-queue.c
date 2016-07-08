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
*   File Name   : qos-queue.c           *
*   Author      : bnc Administrator           *
*   Create Date : 2016-05-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "qos-queue.h"
#include "mem_pool.h"
#include "gnflush-types.h"
#include "qos-queue-ovsdb.h"
#include "fabric_impl.h"

/* global defines */
void *g_qos_queue_id = NULL;
INT1 g_queue_id[1000] = {0};

/* internal functions */

/* 
 * gen qos meter id
 *
 * @brief: this function is used to generate id for qos meter
 *
 * @param: sw			the switch which qos meter installed
 * @param: port_no		the port no
 *
 * @param: INT4			0: fail; other: generated qos meter id
 */
INT4 gen_qos_queue_id(gn_switch_t* sw, UINT4 port_no);

/*
 * create qos queue
 *
 * @brief: this function is used to create qos queue
 *
 * @param: policy_p			qos policy
 *
 * @return: qos_meter_p		NULL: fail; other: qos queue pointer
 */
gn_queue_t* create_qos_policy_queue(qos_policy_p policy_p);

/*
 * update qos queue
 *
 * @brief: this function is used to update qos queue
 *
 * @param: policy_p			qos policy
 *
 * @return: qos_meter_p		qos queue
 */
gn_queue_t* update_qos_policy_queue(qos_policy_p policy_p, gn_queue_t* queue);

/*
 * find qos by queue
 *
 * @brief: this function is used to find the qos by queue
 *
 * @param: sw				the switch queue installed
 * @param: queue				the pointer of queue
 *
 * @return gn_qos_t*			the pointer of qos
 */
gn_qos_t* find_qos_by_queue(gn_switch_t* sw, gn_queue_t* queue);

/*
 * find qos by port no
 *
 * @brief: this function is used to find the qos by queue
 *
 * @param: sw				the switch queue installed
 * @param: port_no			the port_no
 *
 * @return gn_qos_t*			the pointer of qos
 */
gn_qos_t* find_qos_by_port_no(gn_switch_t* sw, UINT4 port_no);

/*
 * this function is used to add qos entry
 * 
 * @brief: this function is used to add qos entry with sw and port no
 *            each port has only one qos.
 *
 * @param: sw				the switch qos installed
 * @param: port_no			the port_no of port
 *
 * @return: INT4				GN_OK:success; GN_ERR: fail
 */
INT4 add_qos_entery(gn_switch_t* sw, UINT4 port_no);



/* function body */

// generate qos policy id
INT4 gen_qos_queue_id(gn_switch_t* sw, UINT4 port_no)
{
	if ((NULL == sw) || (0 == port_no)) {
		return 0;
	}

	gn_port_t* port_p = NULL;

	INT4 idx = 0;

	for (; idx < MAX_PORTS; idx++) {
		port_p = &sw->ports[idx];
		if (port_p->port_no == port_no) {
			break;
		}
	}

	if (NULL == port_p) {
		for (idx = 0; idx < MAX_PORTS; idx++) {
			neighbor_t* nei = sw->neighbor[idx];
			if ((nei) && (nei->port) && (nei->port->port_no == port_no)) {
				port_p = nei->port;
				break;
			}
		}
	}

	if ((NULL == port_p) || (port_p->port_no != port_no)){
		return 0;
	}

	INT4 seq = 1;

	port_p->queue_ids[1] = 1;
	

	for (; seq < MAX_QUEUE_ID; seq++) {
		if (0 == port_p->queue_ids[seq]) {
			port_p->queue_ids[seq] = 1;
	
			return seq;	
		}
	}

	return 0;
}

// initiazlie qos policy queue
INT4 init_qos_policy_queue()
{
	return GN_OK;
}

// destroy qos policy queue
INT4 destory_qos_policy_queue()
{
	return GN_OK;
}

// find qos queue by policy
gn_queue_t* find_qos_policy_queue(qos_policy_p policy_p)
{
	if ((NULL == policy_p) || (NULL == policy_p->sw)) {
		return NULL;
	}

	gn_queue_t* queue = policy_p->sw->queue_entries;
	gn_queue_t* policy_queue = (gn_queue_t*)policy_p->qos_service;

	if ((NULL == queue) || (NULL == policy_queue)) {
		return NULL;
	}

	while (queue)
	{
		if ((queue->queue_id == policy_queue->queue_id)) {
			return queue;
		}
	
		queue = queue->next;
	}

	return NULL;
}

// create qos policy queue
gn_queue_t* create_qos_policy_queue(qos_policy_p policy_p)
{
	if ((NULL == policy_p) || (NULL == policy_p->sw)) {
		return NULL;
	}

	UINT4 port_no = get_port_no_between_sw_ip(policy_p->sw, policy_p->dst_ip);
	if (0 == port_no) {
		return NULL;
	}

	UINT4 queue_id = gen_qos_queue_id(policy_p->sw, port_no);
		
	if (0 == queue_id) {
		LOG_PROC("INFO", "Fail to generate queue id.");
		return NULL;
	}

	gn_queue_t* queue = (gn_queue_t*)gn_malloc(sizeof(gn_queue_t));

	if (NULL == queue) {
		LOG_PROC("INFO", "Create qos policy queue failed.");
		return NULL;
	}	
	
	queue->queue_id = queue_id;	
	queue->min_rate = policy_p->min_speed * 1024;
	queue->max_rate = policy_p->max_speed * 1024;
	queue->burst = policy_p->burst_size * 1024;
	queue->priority = policy_p->priority;
	queue->port_no = port_no;

	add_qos_entery(policy_p->sw, port_no);
	add_queue_in_queue_table(policy_p->sw, queue);

	return queue;
}

// update qos policy queue
gn_queue_t* update_qos_policy_queue(qos_policy_p policy_p, gn_queue_t* queue)
{
	if ((NULL == policy_p) || (NULL == queue)) {
		return NULL;
	}

	// update qos policy 
	queue->min_rate = policy_p->min_speed * 1024;
	queue->max_rate = policy_p->max_speed * 1024;
	queue->burst = policy_p->burst_size * 1024;
	queue->priority = policy_p->priority;

	update_queue_in_queue_table(policy_p->sw, queue);

	return queue;
}

// add qos policy queue
gn_queue_t* add_qos_policy_queue(qos_policy_p policy_p)
{		
	gn_queue_t* queue_p = find_qos_policy_queue(policy_p);

	if (queue_p) {
		// update data
		// printf("update policy queue\n");
		queue_p = update_qos_policy_queue(policy_p, queue_p);
	}
	else {
		// create data		
		// printf("creates policy queue\n");
		queue_p = create_qos_policy_queue(policy_p);
	}

	return queue_p;
}

// find qos by queue
gn_qos_t* find_qos_by_queue(gn_switch_t* sw, gn_queue_t* queue) 
{
	gn_qos_t* qos = sw->qos_entries;

	while (qos) {
		if (qos->port_no == queue->port_no) {
			return qos;
		}
		
		qos = qos->next;
	}

	return NULL;
}

// add qos entry
INT4 add_qos_entery(gn_switch_t* sw, UINT4 port_no)
{
	// if qos entry not exist
	if (NULL == find_qos_by_port_no(sw, port_no)) {
		// add entry
		gn_qos_t* qos = (gn_qos_t*)gn_malloc(sizeof(gn_qos_t));

		if (NULL == qos) {
			LOG_PROC("INFO", "Fail to malloc qos add qos policy.");
			return GN_ERR;
		}

		qos->port_no = port_no;

		add_qos_in_qos_table(sw, qos);

		create_default_queue_in_queue_table(sw, port_no);
	}

	return GN_OK;
}

// find qos by port no
gn_qos_t* find_qos_by_port_no(gn_switch_t* sw, UINT4 port_no)
{
	gn_qos_t* qos = sw->qos_entries;

	while (qos) {
		if (qos->port_no == port_no) {
			return qos;
		}
	}
	
	return NULL;
}




// delete qos policy queue
INT4 delete_qos_policy_queue(gn_switch_t* sw, gn_queue_t* queue)
{
	gn_qos_t* qos = find_qos_by_queue(sw, queue);

	if (qos) {
		delete_queue_in_qos_table(sw, qos->qos_uuid, queue);
	}

	delete_queue_in_queue_table(sw, queue);
	
	return GN_OK;
}


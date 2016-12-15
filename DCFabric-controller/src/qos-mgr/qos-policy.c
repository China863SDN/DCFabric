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
#include "qos-policy.h"
#include "mem_pool.h"
#include "common.h"
#include "fabric_openstack_external.h"
#include "ini.h"
#include "../conn-svr/conn-svr.h"
#include "qos-meter.h"
#include "qos-policy-file.h"
#include "qos-mgr.h"

/* global definition */
INT1 g_qos_id[QOS_POLICY_MAX_NUM] = {0};

void *g_qos_policy_id = NULL;
qos_policy_p g_qos_policy_list = NULL;

/* internal function */

/*
 * set qos policy id
 *
 * @brief: this function is used to update the status of qos id
 * 
 * @param: id				qos policy id
 * @param: status			qos policy status
 * 
 * @return INT4			GN_OK: success; GN_ERR: fail
 */
static INT4 set_qos_policy_id(INT4 id, INT4 status);

/*
 * get qos id status
 * 
 * @brief: this function is used to get the status of specific qos id
 *
 * @param: id				qos policy id
 *
 * @return: INT4			0: unused; 1: used
 */
INT4 get_qos_policy_id(INT4 id);

/*
  * create qos policy
  *
  * @brief: this function is used to create qos policy
  *
  * @param: name				qos policy name
  * @param: id				qos id
  * @param: sw				qos which switch installed
  * @param: dst_ip				*one of the match condition
  * @param: min_speed			qos min speed
  * @param: max_speed		qos max speed
  * @param: burst				qos burst speed
  * @param: priority			qos priority
  *
  * @return: qos_policy_p		NULL: create failed; other: created qos policy
  */
qos_policy_p create_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, 
								  UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority);


/* function body */

// initialize qos policy
INT4 init_qos_policy()
{
	destroy_qos_policy();
	
	g_qos_policy_id = mem_create(sizeof(qos_policy), QOS_POLICY_MAX_NUM);

	if (NULL == g_qos_policy_id) {
		LOG_PROC("ERROR", "Create qos policy failed.");

		return GN_ERR;
	}

	return GN_OK;
}

// destroy qos policy
INT4 destroy_qos_policy()
{
	if (g_qos_policy_id != NULL) {
		mem_destroy(g_qos_policy_id);
		g_qos_policy_id = NULL;
	}

	g_qos_policy_list = NULL;

	return GN_OK;
}

// generate qos policy id
INT4 gen_qos_policy_id(gn_switch_t* sw, UINT4 dst_ip)
{
	INT4 seq = 1;

	for (; seq < QOS_POLICY_MAX_NUM; seq++) {
		if (0 == g_qos_id[seq]) {
			g_qos_id[seq] = 1;
	
			return seq;	
		}
	}

	return 0;
}

// set policy id status
static INT4 set_qos_policy_id(INT4 id, INT4 status)
{
	g_qos_id[id] = status;

	return 0;
}

// get qos id status
INT4 get_qos_policy_id(INT4 id)
{
	return g_qos_id[id];
}

// create qos policy
qos_policy_p create_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority)
{
	qos_policy_p policy_p = (qos_policy_p)mem_get(g_qos_policy_id);
	
	if (NULL == policy_p) {
		LOG_PROC("INFO", "Fail to create qos policy, can't get memory");
		return NULL;
	}

	INT4 qos_policy_id = (0 == id) ? gen_qos_policy_id(sw, dst_ip) : id;

	if (0 == qos_policy_id) {
		LOG_PROC("INFO", "Fail to generate qos policy id.");
		return NULL;
	}
	else {
		strcpy(policy_p->qos_policy_name, name);
		policy_p->qos_policy_id = qos_policy_id;
		policy_p->sw= sw;
		policy_p->dst_ip = dst_ip;
		policy_p->min_speed = min_speed;
		policy_p->max_speed = max_speed;
		policy_p->burst_size = burst;
		policy_p->priority = priority;
	}
	
	return policy_p;
}

// add qos policy
qos_policy_p add_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, 
							    UINT8 max_speed, UINT8 burst, UINT4 priority)
{
	qos_policy_p policy_p = find_qos_policy_by_sw_dstip(sw, dst_ip);

	if (NULL == policy_p) {
		policy_p = create_qos_policy(name, id, sw, dst_ip, min_speed, max_speed, burst, priority);
		
		if (policy_p) {
			policy_p->next = g_qos_policy_list;
			g_qos_policy_list = policy_p;
		}
	}
	else {
		update_qos_policy(name, id, sw, dst_ip, min_speed, max_speed, burst, priority, policy_p);
	}

	return policy_p;
}

// update qos policy
INT4 update_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, 
						   UINT8 max_speed, UINT8 burst, UINT4 priority, qos_policy_p policy_p)
{
	set_qos_policy_id(id, 1);

	if (strlen(name))
		strcpy(policy_p->qos_policy_name, name);
	policy_p->sw= sw;
	policy_p->dst_ip = dst_ip;
	if (min_speed)
		policy_p->min_speed = min_speed;
	if (max_speed)
		policy_p->max_speed = max_speed;
	if (priority)
		policy_p->priority = priority;
	if (id)
		policy_p->qos_policy_id= id;
	if (burst)
		policy_p->burst_size = burst;

	return GN_OK;
}



// find qos policy
qos_policy_p find_qos_policy_by_sw_dstip(gn_switch_t* sw, UINT4 dst_ip)
{
	qos_policy_p list = g_qos_policy_list;

	while (list) {
		if ((list->sw == sw) && (list->dst_ip == dst_ip)) {
			return list;
		}

		list = list->next;
	}

	return NULL;
}

// delete qos policy
INT4 delete_qos_policy_by_sw_dstip(gn_switch_t* sw, UINT4 dst_ip)
{
	if (NULL == sw) {
		return GN_ERR;
	}

	UINT4 id = 0;

	qos_policy_p prev_p = g_qos_policy_list;

	if (NULL == prev_p) {
		return id;
	}
	
	qos_policy_p next_p = prev_p->next;

	if ((prev_p->sw == sw) && (prev_p->dst_ip == dst_ip)) {

		id = prev_p->qos_policy_id;
		g_qos_policy_list = prev_p->next;
		set_qos_policy_id(prev_p->qos_policy_id, 0);
		remove_qos_policy_from_file(prev_p);
		mem_free(g_qos_policy_id, prev_p);
	}

	while ((prev_p) && (next_p)) {
		if ((next_p->sw == sw) && (next_p->dst_ip = dst_ip)) {
			
			id = next_p->qos_policy_id;
			prev_p->next = next_p->next;
			set_qos_policy_id(next_p->qos_policy_id, 0);
			remove_qos_policy_from_file(next_p);
			mem_free(g_qos_policy_id, next_p);
		}

		prev_p = prev_p->next;
		next_p = prev_p->next;
	}

	return id;
}

// this function is used to get qos policy list
qos_policy_p get_qos_policy_list()
{
	return g_qos_policy_list;
}

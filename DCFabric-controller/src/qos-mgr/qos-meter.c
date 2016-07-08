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
*   File Name   : qos-meter.c           *
*   Author      : bnc Administrator           *
*   Create Date : 2016-05-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "qos-meter.h"
#include "mem_pool.h"
#include "openflow-common.h"
#include "fabric_impl.h"
#include "qos-mgr.h"
#include "../meter-mgr/meter-mgr.h"

/* define global values */
#define QOS_METER_LIST_MAX_NUM	40960
#define QOS_SWTICH_MAX_TAG  4096

void *g_qos_meter_id = NULL;
qos_meter_p g_qos_meter_list = NULL;

INT4 g_meter_id[QOS_SWTICH_MAX_TAG][QOS_METER_LIST_MAX_NUM];

/* internal functions */

/* 
 * gen qos meter id
 *
 * @brief: this function is used to generate id for qos meter
 *
 * @param: sw			the switch which qos meter installed
 *
 * @param: INT4			0: fail; other: generated qos meter id
 */
INT4 gen_qos_meter_id(gn_switch_t* sw);

/*
 * create qos meter
 *
 * @brief: this function is used to create qos meter
 *
 * @param: policy_p			qos policy
 *
 * @return: qos_meter_p		NULL: fail; other: qos meter pointer
 */
qos_meter_p create_qos_policy_meter(qos_policy_p policy_p);

/*
 * update qos meter
 *
 * @brief: this function is used to update qos meter
 *
 * @param: policy_p			qos policy
 *
 * @return: qos_meter_p		qos meter
 */
qos_meter_p update_qos_policy_meter(qos_policy_p policy_p);

/* function body */

// generate qos policy id
INT4 gen_qos_meter_id(gn_switch_t* sw)
{
	UINT4 tag = of131_fabric_impl_get_tag_dpid(sw->dpid);

	if (0 == tag) {
		return GN_ERR;
	}
	
	INT4 seq = 1;

	for (; seq < QOS_METER_LIST_MAX_NUM; seq++) {
		if (0 == g_meter_id[tag][seq]) {
			g_meter_id[tag][seq] = 1;
			return seq;	
		}
	}

	return 0;
}

// init qos meter
INT4 init_qos_policy_meter()
{
	// destory all qos meter
	destory_qos_policy_meter();

	// malloc the memory for qos meter list
	g_qos_meter_id = mem_create(sizeof(qos_meter), QOS_METER_LIST_MAX_NUM);

	if (NULL == g_qos_meter_id) {
		LOG_PROC("ERROR", "Can't initialize qos meter.");
		return GN_ERR;
	}

	return GN_OK;
}

// destroy qos meter
INT4 destory_qos_policy_meter()
{
	if (NULL != g_qos_meter_id) {
		mem_destroy(g_qos_meter_id);
		g_qos_meter_id = NULL;
	}

	g_qos_meter_list = NULL;

	return GN_OK;
}

// find qos meter by qos policy
INT4 find_qos_policy_meter(qos_policy_p policy_p)
{
	if (NULL == policy_p) {
		return 0;
	}

	qos_meter_p list_p = g_qos_meter_list;
	qos_meter_p qos_service = (qos_meter_p)policy_p->qos_service;

	if ((NULL == qos_service) || (0 == qos_service->meter_id)) {
		return 0;
	}

	// loop to find the qos meter
	while (list_p) {
		if (qos_service->meter_id == list_p->meter_id) {
			return qos_service->meter_id;
		}
		
		list_p = list_p->next;
	}

	return 0;
}

// create qos meter
qos_meter_p create_qos_policy_meter(qos_policy_p policy_p)
{
	qos_meter_p meter_p = (qos_meter_p)mem_get(g_qos_meter_id);

	if (NULL == meter_p) {
		LOG_PROC("INFO", "Fail to create qos policy meter.");
		return NULL;
	}

	meter_p->meter_id = gen_qos_meter_id(policy_p->sw);

	if (0 == meter_p->meter_id) {
		return NULL;
	}

	gn_meter_t* meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));
	
	meter->meter_id = meter_p->meter_id;
	meter->burst_size = policy_p->burst_size;
	meter->flags = OFPMF_KBPS;
	meter->rate = policy_p->max_speed;
	meter->type = OFPMBT_DROP;
	
 	add_meter_entry(policy_p->sw, meter);
	
	return meter_p;
}

// add qos policy
qos_meter_p add_qos_policy_meter(qos_policy_p policy_p)
{	
	if (NULL == policy_p) {
		return NULL;
	}

	qos_meter_p meter_p = NULL;

	if (find_qos_policy_meter(policy_p)) {
		// update data
		// printf("update qos policy meter\n");
		meter_p = update_qos_policy_meter(policy_p);
	}
	else {
		// create data
		// printf("create qos policy meter\n");
		meter_p = create_qos_policy_meter(policy_p);
		if (NULL != meter_p) {
			meter_p->next = g_qos_meter_list;
			g_qos_meter_list = meter_p;
		}
	}

	return meter_p;
}

// update qos meter
qos_meter_p update_qos_policy_meter(qos_policy_p policy_p)
{	
	if ((NULL == policy_p) || (NULL == policy_p->qos_service)) {
		return NULL;
	}
		
	// update qos policy 
	qos_meter_p meter_p = (qos_meter_p)policy_p->qos_service;

	gn_meter_t* meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));

	if (NULL == meter) {
		LOG_PROC("INFO", "Fail to malloc gn_meter_t.");
		return NULL;
	}

	// printf("%s\n, meter: %d", FN, meter_p->meter_id);
		
	meter->meter_id = meter_p->meter_id;
	meter->burst_size = policy_p->burst_size;
	meter->flags = OFPMF_KBPS;
	meter->rate = policy_p->max_speed;
	meter->type = OFPMBT_DROP;
	
	modify_meter_entry(policy_p->sw, meter);

	return meter_p;
}

// delete qos meter
INT4 delete_qos_policy_meter(gn_switch_t* sw, UINT4 meter_id)
{
	gn_meter_t* meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));

	if (NULL == meter) {
		LOG_PROC("INFO", "Failed to malloc gn_mter_t.");
		return GN_ERR;
	}
	
	meter->meter_id = meter_id;
	delete_meter_entry(sw, meter);
	
	return GN_OK;
}



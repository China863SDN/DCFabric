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
*   File Name   : overload-mgr.h           *
*   Author      : bnc Administrator           *
*   Create Date : 2016-05-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef QOS_POLICY_H_
#define QOS_POLICY_H_

#include "common.h"
#include "gnflush-types.h"

/*

typedef struct _qos_match_filed
{
	UINT4 dst_ip;
}qos_match_field, *p_qos_match_field;


typedef struct _qos_target
{
	UINT8 sw_dpid;
	UINT4 sw_vlan_id;
}qos_target, *p_qos_target;

typedef struct _qos_service
{
	UINT4 tos;
	UINT4 min_speed;
	UINT4 max_speed;
	UINT4 meter_id;
}
*/

#define QOS_POLICY_MAX_NUM		10000
#define QOS_PORT_MAX_NUM		1000
#define QOS_POLICY_NAME_LENGTH 	64

enum SW_PARAM_TYPE
{
	SW_INVALID = 0,			// invalid
	SW_DPID = 1, 			// dpid
	SW_ALL = 2, 			// all sw
	SW_EXTERNAL = 3			// external sw
};

typedef struct _qos_policy 
{
	// describe status
	INT4 qos_status;		// 0: not installed

	// policy name
	INT1 qos_policy_name[QOS_POLICY_NAME_LENGTH];

	// policy id
	INT4 qos_policy_id;

	// Target sw
	gn_switch_t* sw;
	
	// Match filed
	UINT4 dst_ip;
	
	// dscp (queue support)
	UINT8 min_speed;
	UINT8 max_speed;
	UINT4 priority;

	// dscp (meter support)
	UINT8 burst_size;

	// service
	void* qos_service;

	// next
	struct _qos_policy* next;
}qos_policy, *qos_policy_p;

/*
  * initialize qos policy
  *
  * @brief: this function is called to initiazlie the qos policy
  *
  * @param: none
  *
  * @return 					GN_OK: success; GN_ERR failed
  */
INT4 init_qos_policy();

/*
  * delete qos policy
  *
  * @brief: this function is called to destory qos policy
  *
  * @param:none
  *
  * @return					GN_OK:success; GN_ERR failed
  */
INT4 destroy_qos_policy();


/*
  * generate qos policy id
  *
  * @brief: this function is used to genrate queue_id/meter_id
  *
  * @param: sw				the switch qos installed
  * @param: dst_ip				
  */
INT4 gen_qos_policy_id(gn_switch_t* sw, UINT4 port_no);

/*
  * add qos policy
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
  * @return: qos_policy_p		NULL: add failed; other: added qos policy
  */
qos_policy_p add_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, 
					   		   UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority);

/*
  * update qos policy
  * 
  * @brief: this function is used to update qos policy
  *
  * @param: name				qos policy name
  * @param: id				qos id
  * @param: sw				qos which switch installed
  * @param: dst_ip				*one of the match condition
  * @param: min_speed			qos min speed
  * @param: max_speed		qos max speed
  * @param: burst				qos burst speed
  * @param: priority			qos priority
  * @param: policy_p			old qos policy
  *
  * @return: qos_policy_p		NULL: add failed; other: added qos policy
  */

INT4 update_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, 
						   UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority, qos_policy_p policy_p);

/*
  * find qos policy
  *
  * @brief: this function is used to find qos policy by dst_ip
  *
  * @param: sw				the switch qos installed
  * @param: dst_ip				the dest ip of qos policy
  *
  * @return: qos_policy_p		NULL: find failed; other: finded qos policy
  */
qos_policy_p find_qos_policy_by_sw_dstip(gn_switch_t* sw, UINT4 dst_ip);


/*
  * delete qos policy
  *
  * @brief: this function is used to delete qos policy by dst_ip
  *
  * @param: sw				the switch qos installed
  * @param: dst_ip				the dest ip of qos policy
  *
  * @return: qos_policy_p		NULL: find failed; other: finded qos policy
  */

INT4 delete_qos_policy_by_sw_dstip(gn_switch_t* sw, UINT4 dst_ip);


/*
 * this function is used to get qos policy list
 *
 * @brief: this function is used get qos policy list
 *
 * @return qos_policy_p			the qos policy list
 */
qos_policy_p get_qos_policy_list();
#endif


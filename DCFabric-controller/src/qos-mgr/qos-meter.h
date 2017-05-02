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
*   File Name   : qos-meter.h           *
*   Author      : bnc Administrator           *
*   Create Date : 2016-05-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef QOS_METER_H_
#define QOS_METER_H_

#include "qos-mgr.h"
#include "qos-policy.h"

// define the qos meter struct
typedef struct _qos_meter 
{
	UINT4 meter_id;
	
	struct _qos_meter* next;
}qos_meter, *qos_meter_p;

/*
 * init qos meter
 *
 * @brief: this function is used to initialize qos meter
 *
 * @param: none
 * 
 * @return: INT4				GN_OK: success; GN_ERR: fail
 */
INT4 init_qos_policy_meter();

/*
 * destory qos meter
 *
 * @brief: this function is used to destory qos meter
 *
 * @param: none
 *
 * @return: INT4				GN_OK: success; GN_ERR: fail
 */
INT4 destory_qos_policy_meter();

/*
 * find qos meter by qos policy
 *
 * @brief: this function is used to find qos meter by qos policy
 *
 * @param: policy_p			qos policy
 *
 * @return: INT4				GN_ERR: fail; other: qos meter id
 */
INT4 find_qos_policy_meter(qos_policy_p policy_p);

/*
 * add qos meter
 * 
 * @brief: this function is used to add qos meter
 * 
 * @param: policy_p 			qos policy
 *
 * @return: qos_meter_p		qos meter
 */
qos_meter_p add_qos_policy_meter(qos_policy_p policy_p);


/*
 * delete qos policy meter
 *
 * @brief: this function is used to delete qos meter
 * 
 * @param: sw				the switch
 * @param: meter_id			the qos meter id
 *
 * @return: INT4				GN_OK:success; GN_ERR:fail
 */
INT4 delete_qos_policy_meter(gn_switch_t* sw, UINT4 meter_id);
#endif


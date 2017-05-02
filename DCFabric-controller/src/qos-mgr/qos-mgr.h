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
*   File Name    : qos-mgr.h           *
*   Author         : bnc Administrator           *
*   Create Date : 2016-05-19           *
*   Version        : 1.0           *
*   Function       : .           *
*                                                                             *
******************************************************************************/
#ifndef QOS_MGR_H_
#define QOS_MGR_H_

#include "common.h"
#include "gnflush-types.h"

/*
  * define the qos type
  */
enum QOS_TYPE
{
	QOS_TYPE_OFF = 0,		///< do not use qos 
	QOS_TYPE_AUTO = 1,		///< according to swich's support, decide the qos type
	QOS_TYPE_METER = 2,		///< use meter
	QOS_TYPE_QUEUE = 3		///< use queue
};

/*
  * initialize qos manager paramer
  * 
  * @brief: this function is used to initiazlie the qos manager
  *
  * @param: none
  *
  * @return: none
  */
void init_qos_mgr();

/*
  * start qos manager paramer
  * 
  * @brief: this function is used to start the qos manager
  *
  * @param: none
  *
  * @return: none
  */

void start_qos_mgr();

/*
  * uninitialize qos manager parameter
  *
  * @brief: this function is called when destroy the qos manager
  * 
  * @param: none
  *
  * @return: none
  */
void destroy_qos_mgr();

/*
  * query qos on/off status
  * 
  * @brief: this function is used to get the qos on/off status
  *
  * @param: none
  *
  * @return		0: qos off; 1: qos on
  */
INT4 get_qos_mgr_status();

/*
  * query qos on/off status
  * 
  * @brief: this function is used to get the qos type
  *
  * @param: none
  *
  * @return		0:QOS_TYPE_OFF; 1: QOS_TYPE_AUTO; 2: QOS_TYPE_METER; 3: QOS_TYPE_QUEUE
  */
INT4 get_qos_mgr_type();

/*
  * add qos rule by rest api
  *
  * @brief: this function is called to add qos policy through rest api
  *
  * @param: name			qos name
  * @param: sw			the switch where qos installed
  * @param: dst_ip			dest ip
  * @param: min_speed		mininum speed 
  * @param: max_speed	maxinum speed
  * @param: burst			burst speed
  * @param: priority		qos priority
  *
  * @return				GN_OK: add success; GN_ERR: add failed
  */
INT4 add_qos_rule_by_rest_api(INT1* name, INT4 id, INT4 sw_type, INT1* sw_param, UINT4 dst_ip, 
								   UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority);

/*
  * delete qos rule by rest api
  *
  * @brief: this function is used to delete qos rule through rest api
  *
  * @param: sw		 	the switch where qos installed
  * @param: dst_ip			dest ip
  * 
  * @return				GN_OK: delete success; GN_ERR: delete fail
  */
INT4 delete_qos_rule_by_rest_api(INT4 sw_type, INT1* sw_param, UINT4 dst_ip);

/*
  * init qos manager timer
  *
  * @brief: this function is used to initialize qos manager timer.  It's designed for accomplishing asynchronous tasks.
  *            For example, in queue, we can't get responce after send json data. So we call the thread timely to san changes.
  *		  
  * @param: none
  *
  * @return				GN_OK: init success; GN_ERR: init failed
  */
INT4 init_qos_mgr_timer();


/*
 * this function is used to initialize siwth qos type
 *
 * @brief: this function is used to initialize switch qos type
 *
 * @param: sw			the switch
 *
 * @return: INT4			enum QOS_TYPE
 */
INT4 init_sw_qos_type(gn_switch_t* sw);

#endif


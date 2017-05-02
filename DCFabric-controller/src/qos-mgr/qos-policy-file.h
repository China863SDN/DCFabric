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
*   File Name   : qos-policy-file.h           *
*   Author      : bnc Administrator           *
*   Create Date : 2016-05-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef QOS_POLICY_FILE_H_
#define QOS_POLICY_FILE_H_

#include "qos-policy.h"

/*
  * initi qos policy file
  *
  * @brief: this function is used to initialize qos policy file
  *
  * @param: none
  *
  * @return: none
  */
void init_qos_policy_file();

/*
 * save qos policy to file
 *
 * @brief: this function is used to save qos policy to local file
 *
 * @param: none
 *
 * @return: none
 */
INT4 save_qos_policy_to_file();

/*
  * remove qos policy from file
  *
  * @brief: this function is used to remove qos policy from local file
  *
  * @param: policy_p			the qos policy object
  *
  * @return: INT4				GN_OK: success; GN_ERR: fail			
  */
INT4 remove_qos_policy_from_file(qos_policy_p policy_p);

/*
  * read qos policy from file
  * 
  * @brief: this function is used to get qos policy from local file
  *
  * @param: none
  *
  * @return: INT4				GN_OK: success; GN_ERR:fail
  */
INT4 read_qos_policy_from_file();


#endif


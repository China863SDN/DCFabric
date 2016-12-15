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
#ifndef QOS_QUEUE_OVSDB_H_
#define QOS_QUEUE_OVSDB_H_

#include "qos-mgr.h"


/*
 * create queue in queue table
 *
 * @brief: this function is used to add queue to queue table
 * 
 * @param sw					the switch
 * @param: queue				the queue struct
 *
 * @return INT4				GN_OK: success; GN_ERR: fail
 */
INT4 add_queue_in_queue_table(gn_switch_t* sw, gn_queue_t* queue);

/*
 * create defalut queue in queue table
 *
 * @brief: this function is used to add default queue to queue table
 * 
 * @param sw					the switch
 * @param: port_no			the port no
 *
 * @return INT4				GN_OK: success; GN_ERR: fail
 */
INT4 create_default_queue_in_queue_table(gn_switch_t* sw, UINT4 port_no);


/*
 * update queue in queue table
 *
 * @brief: this function is used to update queue to queue table
 * 
 * @param sw					the switch
 * @param: queue				the queue struct
 *
 * @return INT4				GN_OK: success; GN_ERR: fail
 */
INT4 update_queue_in_queue_table(gn_switch_t* sw, gn_queue_t* queue);

/*
  * delete queue in queue table
  *
  * @brief: this function is used to delete queue in queue table
  *
  * @param: sw 				the switch
  * @param: queue				the struct of queue
  *
  * @return: INT4				GN_OK: success; GN_ERR: faild
  */
INT4 delete_queue_in_queue_table(gn_switch_t* sw, gn_queue_t* queue);

/*
  * delete all queue in queue table
  *
  * @brief: this function is used to delete all queues
  *
  * @param: conn_fd 			the connect socket
  *
  * @return: INT4				GN_OK: success; GN_ERR: faild
  */
INT4 clear_queue_in_queue_table(INT4 conn_fd);


/*
 * add queue to qos table
 *
 * @brief: this function is used to add queue to qos table
 *            each qos contains various queues, and each queue has his own queue id.
 *            each qos must contain a default queue whose queue is 0.
 *		 and this function is used to add the exist queue into the qos table.
 *
 * @param: sw				the switch 
 * @param: qos_uuid			the uuid of qos
 * @param: queue_id			the id of queue
 * @param: queue_uuid			the uuid of queue
 *
 * @return INT4				GN_OK: success; GN_ERR: fail
 */
INT4 add_queue_to_qos_table(gn_switch_t* sw, INT1* qos_uuid, UINT4 queue_id, INT1* queue_uuid);

/*
  * delete queue in qos table
  *
  * @brief: this function is used to delete queue in qos table
  *
  * @param: sw 				the switch
  * @param: qos_uuid			the uuid of qos
  * @param: queue				the struct of queue
  *
  * @return: INT4				GN_OK: success; GN_ERR: faild
  */
INT4 delete_queue_in_qos_table(gn_switch_t* sw, INT1* qos_uuid, gn_queue_t* queue);

/*
  * create qos in qos table
  *
  * @brief: this function is used to add qos  in qos table
  *
  * @param: sw 				the switch
  * @param: qos				the struct of qos
  *
  * @return: INT4				GN_OK: success; GN_ERR: faild
  */
INT4 add_qos_in_qos_table(gn_switch_t* sw, gn_qos_t* qos);

/*
  * delete all qos in qos table
  *
  * @brief: this function is used to delete all qos  in qos table
  *
  * @param: conn_fd			the connect socket  
  *
  * @return: INT4				GN_OK: success; GN_ERR: faild
  */
INT4 clear_qos_in_qos_table(INT4 conn_fd);

/*
 * add qos to port table
 *
 * @brief: this function is used to qos to port table, each port has only one qos.
 *
 * @param: sw				the switch
 * @param: port_uuid			the uuid of port
 * @param: qos_uuid			the uuid of qos
 *
 * @return INT4				GN_OK: success; GN_ERR: fail
 */
INT4 add_qos_to_port_table(gn_switch_t* sw, INT1* port_uuid, INT1* qos_uuid);

/*
  * delete all qos in port table
  *
  * @brief: this function is used to delete all qos  in port table
  *
  * @param: conn_fd			the connect socket  
  *
  * @return: INT4				GN_OK: success; GN_ERR: faild
  */

INT4 clear_qos_in_port_table(INT4 conn_fd, INT1* port_uuid);

/*
 * search interface by port no
 *
 * @brief: this function is used to search interface by port no
 *
 * @param: sw				the switch
 * @param: port_no			the port no
 *
 * @return INT4				GN_OK: success; GN_ERR: fail
 */
INT4 search_interface_by_port_no(gn_switch_t* sw, UINT4 port_no);

/*
 * search port by interface uuid
 *
 * @brief: this function is used to search port by interface uuid
 *
 * @param: sw				the switch
 * @param: interface_uuid		the uuid of interface
 *
 * @return: INT4				GN_OK: success; GN_ERR:fail
 */
INT4 search_port_by_interface(gn_switch_t* sw, INT1* interface_uuid);

/*
 * receive queue uuid notify
 *
 * @brief: this function is used to receive queue uuid notify
 *            after create queue, the uuid won't be reply synchronously
 *            so this function is used to receive the reply.
 *
 * @param: queueu			the queue json, include uuid, sw and port_no info
 *
 * @return: INT4				GN_OK: success; GN_ERR: faild
 */
INT4 notify_recevice_queue_uuid(json_t* queue);

/*
 * receive qos uuid notify
 *
 * @brief: this function is used to receive qos uuid notify
 *            after create qos, the uuid won't be reply synchronously
 *            so this function is used to receive the reply.
 *
 * @param: qos				the qos json, include uuid, sw and port_no info
 *
 * @return: INT4				GN_OK: success; GN_ERR: faild
 */
INT4 notify_recevice_qos_uuid(json_t* qos);

/*
 * receive interface uuid notify
 *
 * @brief: this function is used to receive interface uuid notify
 *
 * @param: interface			the interface json, include uuid, port and mac(use to find the sw) info
 *
 * @return: INT4				GN_OK: success; GN_ERR: faild
 */
INT4 notify_receive_interface_uuid(json_t* interface);

/*
 * receive port uuid notify
 *
 * @brief: this function is used to receive port uuid notify
 *
 * @param: interface			the port json, include uuid and interface uuid
 *
 * @return: INT4				GN_OK: success; GN_ERR: faild
 */
INT4 notify_receive_port_uuid(json_t* port);


#endif


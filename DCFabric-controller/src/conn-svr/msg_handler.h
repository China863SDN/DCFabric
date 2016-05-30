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
*   File Name   : msg_handler.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-13           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef MSG_HANDLER_H_
#define MSG_HANDLER_H_

#include "gnflush-types.h"

extern UINT4 DEFAULT_TRANSACTION_XID;

extern convertter_t of10_convertter;
extern convertter_t of13_convertter;
extern msg_handler_t of_message_handler[];
extern msg_handler_t of10_message_handler[];
extern msg_handler_t of13_message_handler[];

void message_process(gn_switch_t *sw, UINT1 *ofmsg);
INT4 of13_msg_hello(gn_switch_t *sw, UINT1 *of_msg);
INT4 of13_msg_echo_request(gn_switch_t *sw, UINT1 *of_msg);
INT4 of10_msg_echo_request(gn_switch_t *sw, UINT1 *of_msg);


#endif /* MSG_HANDLER_H_ */

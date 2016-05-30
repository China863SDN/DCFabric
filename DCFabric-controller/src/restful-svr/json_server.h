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
*   File Name   : json_server.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef JSON_SERVER_H_
#define JSON_SERVER_H_

#include "restful-svr.h"
#include "app_impl.h"


//max length of param that pasted by rest client
#define REST_MAX_PARAM_LEN 1024
#define REST_MAX_ACTION_NUM 256

INT4 init_json_server();
INT1 *json_to_reply(json_t *obj, INT4 code);
INT1 *json_to_reply_desc(json_t *obj, INT4 code, const INT1 *desc);
INT1 *proc_restful_request(UINT1 type, const INT1 *url, json_t *root);

#endif /* JSON_SERVER_H_ */

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
*   File Name   : restful-svr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef RESTFUL_SVR_H_
#define RESTFUL_SVR_H_

#include "microhttpd.h"
#include "gnflush-types.h"

#define REST_CAPACITY 30
#define POSTBUFFERSIZE 512
#define REST_BUFF_LEN 100   //55 * 1024 = 56320

extern UINT4 g_rest_port;

extern restful_handles_t g_restful_get_handles[];
extern restful_handles_t g_restful_post_handles[];
extern restful_handles_t g_restful_put_handles[];
extern restful_handles_t g_restful_delete_handles[];


INT4 init_restful_svr();

#endif /* RESTFUL_SVR_H_ */

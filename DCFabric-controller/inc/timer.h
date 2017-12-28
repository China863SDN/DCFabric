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
*   File Name   : timer.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef TIMER_H_
#define TIMER_H_

#include "mem_pool.h"


//时间节点
typedef struct _timer_node
{
    UINT1       is_used;								//by:yhy 该node是否启用
    UINT4       timeout;            					//by:yhy 该node超时时间
    UINT4       times;              					//by:yhy 该node剩余时间
    void        *para;              					//by:yhy 参数
    void        *timer_id;          					//by:yhy 参数
    void        (*fun)(void *para,  void *timer_id);	//by:yhy 当times为零时执行的功能函数
}Timer_Node;


typedef int(*Fun)(void *data);
extern struct timeval g_cur_sys_time;

UINT4 timer_num(void *_timer_hdl);
void *timer_init(UINT4  len);
void *timer_creat(void *_timer_hdl ,UINT4 timeout,void *para,void **timer_id, void (*fun)(void *para ,void *timer_id) );
void *timer_kill(void *_timer_hdl ,void **timer_id);
void *timer_destroy(void **_timer_hdl);
int init_sys_time();
void fini_sys_time();

#endif /* TIMER_H_ */

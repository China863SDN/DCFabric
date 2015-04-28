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

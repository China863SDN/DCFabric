/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2016, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the BNC SDN Controller. BNC SDN
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
*   File Name   : CTimer.h                                                    *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CTIMER_H
#define __CTIMER_H

#include "bnc-type.h"
#include "CThread.h"

typedef enum
{
    TIMER_STATE_INIT = 0,
    TIMER_STATE_START,
    TIMER_STATE_DELAY,
    TIMER_STATE_PERIOD,
    TIMER_STATE_STOP
}timer_state_e;

typedef void (*RUNNABLE)(void*);

/*
 * 定时器类
 */
class CTimer
{
public:
    CTimer(INT4 type=THREAD_TYPE_NORMAL);
    ~CTimer();

    /*
     * 执行定时任务
     * param delay: 延时执行时间，单位ms
     * param period: 执行周期，单位ms
     * param runnable: 回调函数指针 void (*RUNNABLE)();
     * param param: 回调函数所需要的参数;
     * ret: 成功 or 失败
     */
    INT4 schedule(INT4 delay, INT4 period, RUNNABLE runnable, void* param);

    /*
     * 获取延迟时间
     * ret: 返回延迟时间，单位ms
     */
    INT4 getDelay() {return m_delay;}

    /*
     * 获取执行周期
     * ret: 返回执行周期，单位ms
     */
    INT4 getPeriod() {return m_period;}

    /*
     * 获取回调函数指针
     * ret: 返回回调函数指针
     */
    RUNNABLE getRunnable() {return m_runnable;}

    /*
     * 获取回调函数的参数
     * ret: 返回回调函数参数指针
     */
    void* getParam() {return m_param;}

    /*
     * 判断定时器是否已停止
     * ret: 停止 TRUE，未停止 FALSE
     */
    BOOL isStop() {return m_stop;}

    /*
     * 停止定时器
     */
    void stop() {m_stop = TRUE;}

private:
    volatile BOOL m_stop;
    INT4          m_delay;
    INT4          m_period;
    RUNNABLE      m_runnable;
    void*         m_param;
    CThread       m_thread;

public:
    volatile INT4 m_state;
    volatile INT4 m_delayTimes;
    volatile INT4 m_periodTimes;
};

#endif

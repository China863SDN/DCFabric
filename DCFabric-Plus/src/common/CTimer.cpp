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
*   File Name   : CTimer.cpp                                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "log.h"
#include "comm-util.h"
#include "bnc-error.h"
#include "CTimer.h"

#define EXIT_TIMER_ROUTINE(timer)         \
    {                                     \
        timer->stop();                    \
        timer->m_state = TIMER_STATE_STOP;\
        timer->m_delayTimes = 0;          \
        timer->m_periodTimes = 0;         \
        return NULL;                      \
    }
#define EXEC_TIMER_ROUTINE(timer)                   \
    {                                               \
        try {                                       \
            timer->getRunnable()(timer->getParam());\
        } catch (...) {                             \
            LOG_ERROR("catch exception !");         \
        }                                           \
        INT4 period = timer->getPeriod();           \
        if (period > 0) {                           \
            timer->m_state = TIMER_STATE_PERIOD;    \
            timer->m_periodTimes = period * 10;     \
        } else {                                    \
            EXIT_TIMER_ROUTINE(timer);              \
        }                                           \
    }

static void* run(void* param)
{
	prctl(PR_SET_NAME, (unsigned long)"CTimer");  

    CTimer* timer = (CTimer*)param;

    while (1)
    {
        if (timer->isStop())
            EXIT_TIMER_ROUTINE(timer);

        switch (timer->m_state)
        {
            case TIMER_STATE_START:
            {
                INT4 delay = timer->getDelay();
                if (delay > 0)
                {
                    timer->m_state = TIMER_STATE_DELAY;
                    timer->m_delayTimes = delay * 10;
                }
                else
                {
                    EXEC_TIMER_ROUTINE(timer);
                }
                break;
            }

            case TIMER_STATE_DELAY:
            {
                if (timer->m_delayTimes > 0)
                    timer->m_delayTimes --;
                if (timer->m_delayTimes <= 0)
                    EXEC_TIMER_ROUTINE(timer);
                break;
            }
            
            case TIMER_STATE_PERIOD:
            {
                if (timer->m_periodTimes > 0)
                    timer->m_periodTimes --;
                if (timer->m_periodTimes <= 0)
                    EXEC_TIMER_ROUTINE(timer);
                break;
            }

            case TIMER_STATE_INIT:
            case TIMER_STATE_STOP:
            default:
                EXIT_TIMER_ROUTINE(timer);
        }

        usleep(100*1000); //100ms
    }

    EXIT_TIMER_ROUTINE(timer);
}

CTimer::CTimer(INT4 type):
    m_stop(FALSE),
    m_delay(0),
    m_period(0),
    m_runnable(NULL),
    m_param(NULL),
    m_thread(type),
    m_state(TIMER_STATE_INIT),
    m_delayTimes(0),
    m_periodTimes(0)
{
}

CTimer::~CTimer()
{
}

INT4 CTimer::schedule(INT4 delay, INT4 period, RUNNABLE runnable, void* param)
{
    m_stop = FALSE;
    m_delay = delay;
    m_period = period;
    m_runnable = runnable;
    m_param = param;

    INT4 curState = m_state;
    m_state = TIMER_STATE_START;

    if ((TIMER_STATE_INIT == curState) || (TIMER_STATE_STOP == curState))
    {
        if (m_thread.init(run, (void*)this, "CTimer") != BNC_OK)
        {
            LOG_ERROR_FMT("CTimer init CThread failed[%d] !", errno);
            return BNC_ERR;
        }
    }

    return BNC_OK;
}


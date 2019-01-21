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
#include "CTimer.h"
#include "bnc-error.h"
#include <sys/prctl.h> 

static void* run(void* param)
{
    CTimer* timer = (CTimer*)param;
    if (timer->isStop())
    {
        return NULL;
    }

	prctl(PR_SET_NAME, (unsigned long)"CTimer");  

    INT4 delay = timer->getDelay();
    if (delay > 0)
        sleep(delay);

    while (1)
    {
        if (timer->isStop())
        {
            return NULL;
        }

        try
        {
            timer->getRunnable()(timer->getParam());
        }
        catch (...)
        {
            printf("catch exception !");
        }

        INT4 period = timer->getPeriod();
        if (period <= 0)
            break;
        sleep(period);
    }

    return NULL;
}

CTimer::CTimer(INT4 type):m_stop(TRUE),m_delay(0),m_period(0),m_runnable(NULL),m_param(NULL),m_thread(type)
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

    if (m_thread.init(run, (void*)this, "CTimer") != BNC_OK)
    {
        printf("CTimer init CThread failed[%d] !", errno);
        return BNC_ERR;
    }

    return BNC_OK;
}


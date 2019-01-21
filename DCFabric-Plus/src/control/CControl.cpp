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
*   File Name   : CControl.cpp                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "log.h"
#include "bnc-error.h"
#include "CTimer.h"
#include "CConf.h"
#include "CControl.h"

CControl* CControl::m_pInstance = NULL;

static void* signal_proc(void* param)
{
	sigset_t set;
    INT4 iSigno=0;
	INT4 iRet = 0; 
	
	prctl(PR_SET_NAME, (unsigned long) "SignalProc");  

	sigemptyset(&set);
	sigfillset(&set);
	pthread_sigmask(SIG_SETMASK, &set, NULL);
	
	while(1)
	{
        iRet = sigwait(&set, &iSigno);
		if(0 != iRet)
		{
			LOG_INFO("Get signal failed !");
		}
		LOG_INFO_FMT("Recv signal %d !",iSigno);
		switch(iSigno)
		{
			case SIGPIPE:  break;
			case SIGINT:   
				{
					// process by main thread function wait_exit
					break;
				} 
			default: break;
		}

			
	}
	return NULL;
}

static void delay_set(void* param)
{
	CControl::getInstance()->setDelayTimeReach(TRUE);
}

CControl* CControl::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CControl();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return (m_pInstance);
}

CControl::CControl(): m_L3modeon(FALSE),m_DelayFlag(FALSE)
{
	 // 读取L3 MODE启动信息
    const INT1* result =  CConf::getInstance()->getConfig("controller", "L3mode_on");
    m_L3modeon = (result == NULL) ? FALSE : ( atoi(result) != 0);
	
	result =  CConf::getInstance()->getConfig("controller", "Delay_time_ms");
	
	INT4 delaytime = (result == NULL)? DELAY_TIMEOUT_SECOND:((atoi(result)> DELAY_TIMEOUT_SECOND)? atoi(result) : DELAY_TIMEOUT_SECOND);
	m_timer.schedule(delaytime, 0, delay_set, NULL);
	
	init_signal();
	set_fileandcore();
}

CControl::~CControl()
{
}

INT4 CControl::init_signal()
{
	pthread_t signal_thread;
	
	sigset_t set;
	sigemptyset(&set);
	sigfillset(&set);
	sigdelset(&set,  SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL); //主线程屏蔽信号集处理
	
	if(pthread_create(&signal_thread, NULL, signal_proc,NULL) != 0 )
    {
        return BNC_ERR;
    }
	return BNC_OK;
}

INT4 CControl::set_fileandcore()
{
    //设置进程允许打开最大文件数
    struct rlimit rt;
    rt.rlim_max = rt.rlim_cur = m_maxFileNo;
    if (setrlimit(RLIMIT_NOFILE, &rt) < 0) 
    {
        LOG_ERROR("setrlimit file error");
        return BNC_ERR;
    }
	rt.rlim_cur = rt.rlim_max = RLIM_INFINITY;
	if (setrlimit(RLIMIT_CORE, &rt)< 0) 
	{
		LOG_ERROR("setrlimit core error");
        return BNC_ERR;
    }
    return BNC_OK;
}


INT4 CControl::init()
{
    LOG_INFO("init CMsgTree...");
    if (BNC_OK != m_msgTree.init())
    {
        LOG_ERROR("init CMsgTree failed!");
        return BNC_ERR;
    }

    LOG_INFO("init CMsgHandlerMgr...");
    if (BNC_OK != m_handlerMgr.init())
    {
        LOG_ERROR("init CMsgHandlerMgr failed!");
        return BNC_ERR;
    }

    LOG_INFO("init CSwitchMgr...");
    if (BNC_OK != m_switchMgr.init())
    {
        LOG_ERROR("init CSwitchMgr failed!");
        return BNC_ERR;
    }

    LOG_INFO("init CTopoMgr...");
    if (BNC_OK != m_topoMgr.init())
    {
        LOG_ERROR("init CTopoMgr failed!");
        return BNC_ERR;
    }

    LOG_INFO("init CTagMgr...");
    if (BNC_OK != m_tagMgr.init())
    {
        LOG_ERROR("init CTagMgr failed!");
        return BNC_ERR;
    }

    LOG_INFO("init CControl success");
    return BNC_OK;
}

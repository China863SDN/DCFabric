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
*   File Name   : CMsgHandler.cpp                                             *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "CMsgHandler.h"
#include "comm-util.h"
#include "bnc-param.h"
#include "bnc-error.h"
#include "log.h"
#include "CControl.h"
#include "CCpuCtrl.h"

class CHandlerParam {
public:
    CHandlerParam(CMsgHandler* handler, CMsgPath& path, CSemaphore& sem):
        m_handler(handler),m_path(path),m_sem(sem) {}
    ~CHandlerParam() {}
        
    CMsgHandler* m_handler;
    CMsgPath m_path;
    CSemaphore& m_sem;
};

static void* work(void* param)
{
    if (NULL == param)
    {
        LOG_ERROR("handler thread start failed, null param!");
        return NULL;
    }

    CHandlerParam* hdparam = (CHandlerParam*)param;
    UINT8 threadId = (UINT8)pthread_self();

    LOG_WARN_FMT("%s[%p] start thread with pid[%d]threadId[%llu] on path[%s] ...", 
        hdparam->m_handler->toString(), hdparam->m_handler, getpid(), threadId, 
        hdparam->m_path.c_str());

	prctl(PR_SET_NAME, (unsigned long)hdparam->m_handler->toString());  

    while (1)
    {
        try
        {
            if (hdparam->m_sem.wait() == 0)
            {
                hdparam->m_handler->handle(hdparam->m_path);
            }
            else
            {
                LOG_ERROR_FMT("%s sem_wait on path %s failed[%d]!", 
                    hdparam->m_handler->toString(), hdparam->m_path.c_str(), errno);
            }
        }
        catch (...)
        {
            LOG_ERROR_FMT("%s[%p] catch exception !", hdparam->m_handler->toString(), hdparam->m_handler);
        }
    }

    delete hdparam;

    LOG_DEBUG_FMT("%s thread stop, threadId:%llu, path:%s", 
        hdparam->m_handler->toString(), threadId, hdparam->m_path.c_str());
    return NULL;
}

CMsgHandler::CMsgHandler():CMsgOperator(MSG_OPERATOR_CONSUMER),m_count(0)
{
}

CMsgHandler::~CMsgHandler()
{
    CMsgOperator::deregister(m_firstPath);
    STL_FOR_LOOP(m_morePaths, it)
    {
        CMsgPath path = it->first;
        CMsgOperator::deregister(path);
    }
}

INT4 CMsgHandler::onregister(CMsgPath& path, INT4 threadNum, BOOL integrated, INT4 type)
{
    LOG_INFO_FMT("%s[%p] register to path[%s] ...", toString(), this, path.c_str());

    CSemThreads* sem = NULL;
    CSemThreads semThreads(integrated, type);

    if (0 == m_count)
    {
        m_firstPath = path;
        m_firstSem = semThreads;
        sem = &m_firstSem;
    }
    else
    {
        std::pair<std::map<CMsgPath, CSemThreads>::iterator, bool> ret =
            m_morePaths.insert(std::pair<CMsgPath, CSemThreads>(path, semThreads));
        if (!ret.second)
        {
            LOG_WARN_FMT("%s[%p] insert map failed when register to path[%s] !", toString(), this, path.c_str());
            return BNC_ERR;
        }
        sem = &(ret.first->second);
    }
    ++ m_count;

    INT4 quota = CCpuCtrl::getInstance()->getQuota(toString());
    //if ((quota > 0) && (quota < threadNum))
    //    threadNum = quota;
    INT4 ttype = type;

    INT4 i = 0;
    for (; i < threadNum; i++)
    {
        CHandlerParam* param = new CHandlerParam(this, path, sem->m_sem); 
        if (NULL == param)
        {
            LOG_ERROR_FMT("%s new CHandlerParam failed!", toString());
            break;
        }

        if (type == THREAD_TYPE_OCCUPY_CORE)
            if (!((quota > 0) && (i < quota)))
                ttype = THREAD_TYPE_NORMAL;

        sem->m_threads.push_back(CThread(ttype));
        //sem->m_threads.push_back(CThread(type));
        if (sem->m_threads[i].init(work, param, toString()) != BNC_OK)
        {
            LOG_ERROR_FMT("%s init CThread failed !", toString());
            delete param;
            break;
        }
    }

    if (0 == i)
    {
        LOG_WARN_FMT("%s start thread failed!", toString());
        return BNC_ERR;
    }

    return CMsgOperator::onregister(path);
}

void CMsgHandler::deregister(CMsgPath& path)
{
    CMsgOperator::deregister(path);

    if (0 == m_count)
        return;
    
    if (m_firstPath.compare(path) == 0)
    {
        m_firstSem.~CSemThreads();
        if (--m_count > 0)
        {
            std::map<CMsgPath, CSemThreads>::iterator it = m_morePaths.begin();
            if (it != m_morePaths.end())
            {
                m_firstPath = it->first;
                m_firstSem = it->second;
                m_morePaths.erase(it);
            }
            else
            {
                m_count = 0;
            }
        }
    }
    else if (m_count > 1)
    {
        std::map<CMsgPath, CSemThreads>::iterator it = m_morePaths.find(path);
        if (it != m_morePaths.end())
        {
            m_morePaths.erase(it);
            -- m_count;
        }
    }
}

INT4 CMsgHandler::onregister(CMsgPath& path, INT4 state, INT4 threadNum, BOOL integrated, INT4 type)
{
    LOG_INFO_FMT("%s register handler[%p] to path[%s]state[%d] ...", 
        toString(), this, path.c_str(), state);

    CMsgPath pathState(path);
    pathState += ":";
    pathState += to_string(state);

    return onregister(pathState, threadNum, integrated, type);
}

void CMsgHandler::deregister(CMsgPath& path, INT4 state)
{
    CMsgPath pathState(path);
    pathState += ":";
    pathState += to_string(state);

    deregister(pathState);
}

void CMsgHandler::notify(CMsgPath& path)
{
    CSemThreads* semThreads = getSemThreads(path);
    if (NULL != semThreads)
    {
        semThreads->m_sem.post();
        LOG_INFO_FMT("notified %s[%p] with path[%s]", toString(), this, path.c_str());
    }
    else
    {
        LOG_WARN_FMT("%s[%p] not register to path[%s] !", toString(), this, path.c_str());
    }
}

void CMsgHandler::cancel(CMsgPath& path)
{
    CSemThreads* semThreads = getSemThreads(path);
    if (NULL != semThreads)
        semThreads->m_sem.trywait();
}

INT4 CMsgHandler::handle(CMsgPath& path)
{
    CSemThreads* semThreads = getSemThreads(path);
    if (NULL == semThreads)
    {
        LOG_WARN_FMT("%s[%p] not register to path[%s] !", toString(), this, path.c_str());
        return BNC_ERR;
    }

    if (!semThreads->m_integrated)
    {
        LOG_INFO_FMT("%s[%p] pop one msg with path[%s] ...", toString(), this, path.c_str());
        CSmartPtr<CMsgCommon> msg = CControl::getInstance()->getMsgTree().popMsg(path, this);
        if (msg.isNotNull())
            return handle(msg);
    }
    else
    {
        LOG_INFO_FMT("%s[%p] pop msg queue with path[%s] ...", toString(), this, path.c_str());
        CSmartPtr<CMsgQueue> queue = CControl::getInstance()->getMsgTree().popMsgQueue(path, this);
        if (queue.isNotNull())
            return handle(queue);
    }

    return BNC_ERR;
}

INT4 CMsgHandler::handle(CSmartPtr<CMsgQueue> queue)
{
    LOG_INFO("CMsgHandler handle msg queue");
    return BNC_OK;
}

INT4 CMsgHandler::handle(CSmartPtr<CMsgCommon> msg)
{
    LOG_INFO("CMsgHandler handle single msg");
    return BNC_OK;
}

CSemThreads* CMsgHandler::getSemThreads(CMsgPath& path)
{
    CSemThreads* ret = NULL;

    if (m_count > 0)
    {
        if (m_firstPath.compare(path) == 0)
        {
            ret = &m_firstSem;
        }
        else if (m_count > 1)
        {
            std::map<CMsgPath, CSemThreads>::iterator it = m_morePaths.find(path);
            if (it != m_morePaths.end())
                ret = &(it->second);
        }
    }

    return ret;
}


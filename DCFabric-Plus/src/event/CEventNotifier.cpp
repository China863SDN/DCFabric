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
*   File Name   : CEventNotifier.cpp                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "CEventNotifier.h"
#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CControl.h"

static void* work(void* param)
{
    if (NULL == param)
    {
        LOG_ERROR("CEventConsumer start thread failed, null param!");
        return NULL;
    }

    CSemThread* semThread = (CSemThread*)param;
    UINT8 threadId = (UINT8)pthread_self();

    LOG_WARN_FMT("%s start thread with pid[%d]threadId[%llu] on path[%s] ...", 
        semThread->m_notifier->toString(), getpid(), threadId, semThread->m_path.c_str());

	prctl(PR_SET_NAME, (unsigned long)semThread->m_notifier->toString());  

    while (1)
    {
        try
        {
            if (semThread->m_sem.wait() == 0)
            {
                semThread->m_notifier->consume(semThread->m_path);
            }
            else
            {
                LOG_ERROR_FMT("%s sem_wait on path %s failed[%d]!", 
                    semThread->m_notifier->toString(), semThread->m_path.c_str(), errno);
            }
        }
        catch (...)
        {
            LOG_ERROR("catch exception !");
        }
    }

    LOG_DEBUG_FMT("%s thread stop, threadId:%llu, path:%s", 
        semThread->m_notifier->toString(), threadId, semThread->m_path.c_str());
    return NULL;
}

CEventNotifier::CEventNotifier():CMsgOperator(MSG_OPERATOR_CONSUMER)
{
}

CEventNotifier::~CEventNotifier()
{
    CMsgOperator::deregister(m_firstPath);
    STL_FOR_LOOP(m_morePaths, it)
    {
        CMsgPath path = it->first;
        CMsgOperator::deregister(path);
    }
}

INT4 CEventNotifier::onregister(CMsgPath& path, BOOL integrated)
{
    LOG_INFO_FMT("%s[%p] register to path[%s] ...", toString(), this, path.c_str());

    CSemThread* sem = NULL;
    CSemThread semThread(path, this, integrated);

    m_lock.wlock();

    if (0 == m_count)
    {
        m_firstPath = path;
        m_firstSem = semThread;
        sem = &m_firstSem;
    }
    else
    {
        std::pair<std::map<CMsgPath, CSemThread>::iterator, bool> ret =
            m_morePaths.insert(std::pair<CMsgPath, CSemThread>(path, semThread));
        if (!ret.second)
        {
            LOG_WARN_FMT("%s[%p] insert map failed when register to path[%s] !", toString(), this, path.c_str());
            m_lock.unlock();
			return BNC_ERR;
        }
        sem = &(ret.first->second);
    }
    ++ m_count;

    m_lock.unlock();

    if (sem->m_thread.init(work, sem, toString()) != BNC_OK)
    {
        LOG_ERROR_FMT("%s init CThread failed !", toString());
        return BNC_ERR;
    }

    return CMsgOperator::onregister(path);
}

void CEventNotifier::deregister(CMsgPath& path)
{
    CMsgOperator::deregister(path);

    m_lock.wlock();

    if (0 == m_count)
	{
	    m_lock.unlock();
        return;
	}
    
    if (m_firstPath.compare(path) == 0)
    {
        m_firstSem.~CSemThread();
        if (--m_count > 0)
        {
            std::map<CMsgPath, CSemThread>::iterator it = m_morePaths.begin();
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
        std::map<CMsgPath, CSemThread>::iterator it = m_morePaths.find(path);
        if (it != m_morePaths.end())
        {
            m_morePaths.erase(it);
            -- m_count;
        }
    }

    m_lock.unlock();
}

INT4 CEventNotifier::consume(CMsgPath& path)
{
    CSemThread* semThread = getSemThread(path);
    if (NULL == semThread)
	{
        LOG_WARN_FMT("%s[%p] not register to path[%s] !", toString(), this, path.c_str());
        return BNC_ERR;
    }
    
    if (!semThread->m_integrated)
    {
        LOG_INFO_FMT("%s[%p] pop single event with path[%s] ...", toString(), this, path.c_str());
        CSmartPtr<CMsgCommon> evt = CControl::getInstance()->getMsgTree().popMsg(path, this);
        if (evt.isNotNull())
            return consume(evt);
    }
    else
    {
        LOG_INFO_FMT("%s[%p] pop event queue with path[%s] ...", toString(), this, path.c_str());
        CSmartPtr<CMsgQueue> queue = CControl::getInstance()->getMsgTree().popMsgQueue(path, this);
        if (queue.isNotNull())
            return consume(queue);
    }

    return BNC_ERR;
}

void CEventNotifier::notify(CMsgPath& path)
{
    CSemThread* semThread = getSemThread(path);
    if (NULL != semThread)
    {
        semThread->m_sem.post();
        LOG_INFO_FMT("notified %s[%p] with path[%s]", toString(), this, path.c_str());
    }
    else
    {
        LOG_WARN_FMT("%s[%p] not register to path[%s] !", toString(), this, path.c_str());
    }
}

void CEventNotifier::cancel(CMsgPath& path)
{
    CSemThread* semThread = getSemThread(path);
    if (NULL != semThread)
        semThread->m_sem.trywait();
}

INT4 CEventNotifier::consume(CSmartPtr<CMsgQueue> queue)
{
    LOG_INFO("CEventConsumer consume event queue");
    return BNC_OK;
}

INT4 CEventNotifier::consume(CSmartPtr<CMsgCommon> evt)
{
    LOG_INFO("CEventConsumer consume single event");
    return BNC_OK;
}

CSemThread* CEventNotifier::getSemThread(CMsgPath& path)
{
    CSemThread* ret = NULL;

    m_lock.rlock();

    if (m_count > 0)
    {
        if (m_firstPath.compare(path) == 0)
        {
            ret = &m_firstSem;
        }
        else if (m_count > 1)
        {
            std::map<CMsgPath, CSemThread>::iterator it = m_morePaths.find(path);
            if (it != m_morePaths.end())
                ret = &(it->second);
        }
    }

    m_lock.unlock();

    return ret;
}



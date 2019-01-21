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
*   File Name   : CEventNotifier.h                                            *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CEVENTNOTIFIER_H
#define _CEVENTNOTIFIER_H

#include "CMsgOperator.h"
#include "CMsgTreeNode.h"
#include "CSemaphore.h"
#include "CThread.h"
#include "CRWLock.h"

class CEventNotifier;

class CSemThread 
{
public:
    CSemThread():m_notifier(NULL),m_integrated(FALSE) {}
    CSemThread(CMsgPath& path, CEventNotifier* notifier, BOOL integrated):
        m_path(path),m_notifier(notifier),m_integrated(integrated) {}
    ~CSemThread() {}

    CSemaphore m_sem;
    CThread m_thread;
    CMsgPath m_path;
    CEventNotifier* m_notifier;
    BOOL m_integrated; //when integrated enabled, msg queue will be processed one time
};

/*
 * 事件被动消费者基类
 */
class CEventNotifier : public CMsgOperator
{
public:
    CEventNotifier();
    virtual ~CEventNotifier();

    virtual INT4 onregister() = 0;
    virtual void deregister() = 0;
    INT4 onregister(CMsgPath& path, BOOL integrated=FALSE);
    void deregister(CMsgPath& path);

    void notify(CMsgPath& path);
    void cancel(CMsgPath& path);
    INT4 consume(CMsgPath& path);

    virtual const char* toString() {return "CEventConsumer";}

private:
    virtual INT4 consume(CSmartPtr<CMsgQueue> queue);
    virtual INT4 consume(CSmartPtr<CMsgCommon> evt);
    CSemThread* getSemThread(CMsgPath& path);

private:
    //std::map<CMsgPath, CSemThread> m_paths;
    UINT4 m_count;
    CMsgPath m_firstPath;
    CSemThread m_firstSem;
    std::map<CMsgPath, CSemThread> m_morePaths;
    CRWLock m_lock;
};


#endif

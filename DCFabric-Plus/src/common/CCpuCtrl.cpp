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
*   File Name   : CCpuCtrl.cpp                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CCpuCtrl.h"
#include "comm-util.h"
#include "bnc-error.h"
#include "log.h"

CCpuCtrl* CCpuCtrl::m_instance = NULL;

CCpuCtrl* CCpuCtrl::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CCpuCtrl();
        if (NULL == m_instance)
        {
            exit(-1);
        }

        m_instance->init();
    }

    return (m_instance);
}

CCpuCtrl::CCpuCtrl():m_cpuId(0),m_cpuTotal(getTotalCpu())
{
}

CCpuCtrl::~CCpuCtrl()
{
}

INT4 CCpuCtrl::init()
{
    //m_quotas.insert(std::pair<std::string, INT4>("COfTcpListener", 1));
    //m_quotas.insert(std::pair<std::string, INT4>("COfRecvWorker", m_cpuTotal/8));
    m_quotas.insert(std::pair<std::string, INT4>("CEtherIPHandler", m_cpuTotal/12));
    m_quotas.insert(std::pair<std::string, INT4>("CEtherArpHandler", m_cpuTotal/12));
    //m_quotas.insert(std::pair<std::string, INT4>("COfHelloHandler", 1));
    //m_quotas.insert(std::pair<std::string, INT4>("CTopoDiscovery", 1));
    //m_quotas.insert(std::pair<std::string, INT4>("CTopoMgr", 1));

    return BNC_OK;
}

INT4 CCpuCtrl::getQuota(std::string owner)
{
    INT4 quota = 0;

    std::map<std::string, INT4>::iterator it = m_quotas.find(owner);
    if (it != m_quotas.end())
        quota = it->second;

    return quota;
}

INT4 CCpuCtrl::occupyCpu(std::string owner, pthread_t tid)
{
    if (getQuota(owner) <= 0)
    {
        LOG_WARN_FMT("%s not allowed to occupy CPU", owner.c_str());
        return BNC_ERR;
    }

    INT4 cpuId = getCpuId();
    if ((cpuId >= 0) && (cpuId < m_cpuTotal/2))
    {
        LOG_WARN_FMT("%s bind thread[%llu] to CPU[%d], total CPU[%d] ...", 
            owner.c_str(), (UINT8)tid, cpuId, m_cpuTotal);
        setCpuAffinity(tid, cpuId);

        std::map<std::string, std::vector<INT4> >::iterator it = m_occupies.find(owner);
        if (it == m_occupies.end())
            m_occupies.insert(std::pair<std::string, std::vector<INT4> >(owner, std::vector<INT4>(1, cpuId)));
        else
            it->second.push_back(cpuId);

        return BNC_OK;
    }

    return BNC_ERR;
}

INT4 CCpuCtrl::getCpuId()
{
    return (m_cpuTotal/2 > m_cpuId) ? m_cpuId++ : -1;
}


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
*   File Name   : CNotifyMgr.cpp            *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-13                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "CNotifyMgr.h"
#include "bnc-type.h"
#include "CNotificationCenter.h"
#include "log.h"

CNotifyMgr* CNotifyMgr::m_pInstance = 0;

CNotifyMgr::CNotifyMgr()
{

}

CNotifyMgr::~CNotifyMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

CNotifyMgr* CNotifyMgr::getInstance()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new CNotifyMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
        m_pInstance->init();
    }

    return m_pInstance;
}

void CNotifyMgr::init()
{

    LOG_INFO("Start notification manager.");

    m_center = new CNotificationCenter();

    m_notifyOpenstack = new CNotifyOpenstack(m_center);
    m_notifyHost = new CNotifyHost(m_center);
	m_notifyNetwork = new CNotifyNetwork(m_center);
	m_notifySubnet = new CNotifySubnet(m_center);
	m_notifyFloatingIp = new CNotifyFloatingIp(m_center);
		
    m_notifyPath = new CNotifyPath(m_center);
    m_notifyFlow = new CNotifyFlow(m_center);
    m_notifyTopo = new CNotifyTopo(m_center);
    m_notifyExternal = new CNotifyExternal(m_center);



    m_center->addNotifyHandler(m_notifyOpenstack, m_notifyHost);
    m_center->addNotifyHandler(m_notifyOpenstack, m_notifyExternal);
	m_center->addNotifyHandler(m_notifyOpenstack, m_notifyNetwork);
    m_center->addNotifyHandler(m_notifyOpenstack, m_notifySubnet);
	m_center->addNotifyHandler(m_notifyOpenstack, m_notifyFloatingIp);

	
	m_notifyBaseNetworking = new CNotifyBaseNetworking(m_center);
	m_center->addNotifyHandler(m_notifyBaseNetworking, m_notifyHost);
    //m_center->addNotifyHandler(m_notifyBaseNetworking, m_notifyExternal);
	m_center->addNotifyHandler(m_notifyBaseNetworking, m_notifyNetwork);
    m_center->addNotifyHandler(m_notifyBaseNetworking, m_notifySubnet);
	m_center->addNotifyHandler(m_notifyBaseNetworking, m_notifyFloatingIp);

    m_center->addNotifyHandler(m_notifyPath, m_notifyFlow);
    m_center->addNotifyHandler(m_notifyPath, m_notifyTopo);
}



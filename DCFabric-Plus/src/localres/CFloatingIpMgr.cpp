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
*   File Name   : CFloatingIpMgr.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "bnc-type.h"
#include "comm-util.h"
#include "log.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "CSubnet.h"
#include "CSubnetMgr.h"
#include "CFloatingFlow.h"
#include "CFloatingIp.h"
#include "CFloatingIpMgr.h"
#include "CFloatingFlow.h"

CFloatingIpMgr* CFloatingIpMgr::m_pInstance = NULL;

CFloatingIpMgr::CFloatingIpMgr()
{
	
}

CFloatingIpMgr::~CFloatingIpMgr()
{
	if(NULL != m_pInstance)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

CFloatingIpMgr* CFloatingIpMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CFloatingIpMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
		m_pInstance->init();
		CFloatingFlowInstall::getInstance();
	}
	return m_pInstance;
}
BOOL  CFloatingIpMgr::addFloatingIpNode(CFloatingIp * floatingipNode)
{
	std::pair< CFloatingIpMap::iterator,bool > ret;
	if(NULL == floatingipNode)
	{
		return FALSE;
	}
	ret = m_pFloatingIpList.insert(std::make_pair(floatingipNode->getFloatingIp(), floatingipNode));

	return ret.second;
}

BOOL  CFloatingIpMgr::delFloatingIpNode(UINT4 uifloatingip)
{
	CFloatingIp* fip = findFloatingIpNodeByfloatingIp(uifloatingip);
	if(NULL != fip)
	{
		CFloatingFlowInstall::getInstance()->remove_proactive_floating_flows_by_floating(fip);
	}
	m_pFloatingIpList.erase(uifloatingip);
	return TRUE;
}

CFloatingIp*  CFloatingIpMgr::findFloatingIpNodeByfixedIp(UINT4 uifixedip)
{
 	STL_FOR_LOOP(m_pFloatingIpList, iter)
    {
        if ((NULL != iter->second)&&(uifixedip == iter->second->getFixedIp()))
        {
            return iter->second;
        }

    }
    return NULL;
}

CFloatingIp*  CFloatingIpMgr::findFloatingIpNodeByfloatingIp(UINT4 uifloatingip)
{
   CFloatingIpMap::iterator iter;
   
   iter = m_pFloatingIpList.find(uifloatingip);
   if(m_pFloatingIpList.end() == iter)
   {
		return NULL;
   }
   return iter->second;
}



INT4 CFloatingIpMgr::judge_sw_in_use(UINT4 fix_ip)
{
	CSmartPtr<CHost> host_p = CHostMgr::getInstance()->findHostByIp(fix_ip);
	if((host_p.isNull())||(host_p->getSw().isNull())||(bnc::host::HOST_NORMAL != host_p->getHostType()))
	{
		return  0;
	}
	STL_FOR_LOOP(m_pFloatingIpList, iter)
	{
		if((NULL != iter->second)&&(host_p->getfixIp() == iter->second->getFixedIp()))
		{
			return 1;
		}
	}
	return 0;
}


CFloatingIpMap& CFloatingIpMgr::getFloatingIpListMapHeader()
{
	return m_pFloatingIpList;
}





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
*   File Name   : CNetworkMgr.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "bnc-type.h"
#include "CNetwork.h"
#include "CNetworkMgr.h"

CNetworkMgr* CNetworkMgr::m_pInstance = NULL;

CNetworkMgr::CNetworkMgr()
{
}

CNetworkMgr::~CNetworkMgr()
{
	
}

CNetworkMgr* CNetworkMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CNetworkMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
		m_pInstance->init();
	}
	return m_pInstance;
}


BOOL CNetworkMgr::addNetworkNode(CNetwork * networkNode)
{
	std::pair< CNetworkMap::iterator,bool > ret;
	ret = m_pNetworkList.insert(std::make_pair(networkNode->get_networkid(), networkNode));
	return ret.second;
}

BOOL CNetworkMgr::delNetworkNode( const std::string& networkid)
{
	m_pNetworkList.erase(networkid);
	return TRUE;
}

BOOL CNetworkMgr::updateNetworkNode(CNetwork* networkNode)
{
	
	return TRUE;
}

CNetwork*  CNetworkMgr::findNetworkById(const std::string& networkid)
{
   CNetworkMap::iterator iter;
   iter = m_pNetworkList.find(networkid);
   if(m_pNetworkList.end() == iter)
   {
		return NULL;
   }
   return iter->second ;
}


CNetworkMap CNetworkMgr::getNetworkListMapHead()
{
	return m_pNetworkList;
}


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
*   File Name   : CSubnetMgr.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "bnc-type.h"
#include "comm-util.h"
#include "CSubnet.h"
#include "CSubnetMgr.h"

CSubnetMgr* CSubnetMgr::m_pInstance = NULL;
CSubnetMgr::CSubnetMgr()
{
	
}

CSubnetMgr::~CSubnetMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

CSubnetMgr* CSubnetMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CSubnetMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
}

BOOL CSubnetMgr::addSubnetNode(CSubnet* subnetNode)
{
	std::pair< std::map<std::string ,CSubnet* >::iterator,bool > ret;
	ret = m_pSubnetList.insert(std::make_pair(subnetNode->get_subnetid(), subnetNode));
	return ret.second;
}

BOOL CSubnetMgr::delSubnetNode(const std::string& subnetid)
{
	m_pSubnetList.erase(subnetid);
	return TRUE;
}

BOOL CSubnetMgr::updateSubnetNode(CSubnet* subnetNode)
{
	return TRUE;
}
CSubnet*  CSubnetMgr::findSubnetById(const std::string& subnetid)
{
   std::map<std::string ,CSubnet* >::iterator iter;
   iter = m_pSubnetList.find(subnetid);
   if(m_pSubnetList.end() == iter)
   {
		return NULL;
   }
   return iter->second ;
}

std::string CSubnetMgr::findNetworkIdById(const std::string& subnetid) 
{
	CSubnet* CSubnetNode = findSubnetById(subnetid);
	if(NULL == CSubnetNode)
	{
		return "";
	}
	return CSubnetNode->get_networkid();
}
UINT4  CSubnetMgr::findSubnetDhcpIpById(const std::string& subnetid) 
{
	CSubnet* CSubnetNode = findSubnetById(subnetid);
	if(NULL == CSubnetNode)
	{
		return 0;
	}
	return 0;
}

UINT4  CSubnetMgr::findSubnetGatewayIpById(const std::string& subnetid)
{
	CSubnet* CSubnetNode = findSubnetById(subnetid);
	if(NULL == CSubnetNode)
	{
		return 0;
	}
	return CSubnetNode->get_subnetGateway();
}

CSubnet* CSubnetMgr::findSubnetByIp(UINT4 ip)
{
	if(0 == ip)
	{
		return NULL;
	}
	STL_FOR_LOOP(m_pSubnetList,iter)
	{
		if(iter->second &&((TRUE == iter->second->isInSubnetRangeByIp(ip))||(ip == iter->second->get_subnetGateway())))
		{
			return iter->second;
		}
	}
	return NULL;
}
CSubnet* CSubnetMgr::findSubnetByIp(UINT4 ip, UINT2*  count)
{
	if(0 == ip)
	{
		return NULL;
	}
	STL_FOR_LOOP(m_pSubnetList,iter)
	{
		if(iter->second &&(TRUE == iter->second->isInSubnetRangeByIp(ip)))
		{
			return iter->second;
		}
	}
	return NULL;
}

std::map<std::string, CSubnet* >& CSubnetMgr::getSubnetListMapHead()
{
	return m_pSubnetList;
}
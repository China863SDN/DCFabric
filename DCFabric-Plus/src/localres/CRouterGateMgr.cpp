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
*   File Name   : CRouterGateMgr.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include    "log.h"
#include    "bnc-error.h"
#include    "bnc-param.h"
#include    "CHostDefine.h"
#include    "CHost.h"
#include    "CHostMgr.h"
#include    "CRouterGateMgr.h"

CRouterGateMgr*      CRouterGateMgr::m_pInstance = NULL;

CRouterGateMgr::CRouterGateMgr()
{
	
}
CRouterGateMgr::~CRouterGateMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

CRouterGateMgr*   CRouterGateMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CRouterGateMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
}

INT4   CRouterGateMgr::AddRouterNode(CRouter*  routerNode)
{
	if(NULL == routerNode)
	{
		return BNC_ERR;
	}
	CRouterMap::iterator it = m_routerMap.find(routerNode->getDeviceid());
	if((m_routerMap.end() != it)&&it->second)
	{
		it->second->setRouterIp(routerNode->getRouterIp());
		it->second->setNetworkid(routerNode->getSubnetid());
		it->second->setSubnetid(routerNode->getSubnetid());
		it->second->setStatus(CHECK_UPDATE);
	}
	else
	{
		m_routerMap.insert(std::make_pair(routerNode->getSubnetid(), routerNode));
	}
	return BNC_OK;
}

INT4   CRouterGateMgr::AddRouterNode(const std::string & deviceid, const std::string & networkid, const std::string & subnetid, UINT4 ip)
{
	CRouterMap::iterator it = m_routerMap.find(deviceid);
	if((m_routerMap.end() != it)&&it->second)
	{
		it->second->setRouterIp(ip);
		it->second->setNetworkid(networkid);
		it->second->setSubnetid(subnetid);
		it->second->setStatus(CHECK_UPDATE);
	}
	else
	{
		CRouter*  routerNode = new CRouter(deviceid, networkid, subnetid, ip, CHECK_CREATE);
		if(NULL != routerNode)
		{
			m_routerMap.insert(std::make_pair(routerNode->getDeviceid(), routerNode));
		}
	}
	return BNC_OK;
}


INT4   CRouterGateMgr::DelRouterNode(const std::string & deviceid)
{
	CRouterMap::iterator it = m_routerMap.find(deviceid);
	if(m_routerMap.end() != it)
	{
		m_routerMap.erase(it);
		return BNC_OK;
		
	}
	return BNC_ERR;
}
CRouter*   CRouterGateMgr::FindRouterNodeByDeviceId(const std::string & deviceid)
{
	CRouterMap::iterator it = m_routerMap.find(deviceid);
	if(m_routerMap.end() == it)
	{
		LOG_ERROR_FMT("can't find router by device id[%s] !!!",deviceid.c_str());
		return NULL;
	}
	return it->second;
}

CRouter*   CRouterGateMgr::FindRouterNodeByHostIp(UINT4 HostIp)
{
	CGateway* gateway = FindGatewayNodeByHostIp( HostIp);
	if(NULL == gateway)
	{
		return NULL;
	}
	return FindRouterNodeByDeviceId(gateway->getDeviceid());
}
CRouter*   CRouterGateMgr::FindRouterNodeByHostMac(UINT1* mac)
{
	CGateway* gateway = FindGatewayNodeByHostMac(mac);
	if(NULL == gateway)
	{
        INT1 macStr[20] = {0};
        mac2str(mac, macStr);
		LOG_WARN_FMT("can't find gateway by mac[%s] !!!", macStr);
		return NULL;
	}
	return FindRouterNodeByDeviceId(gateway->getDeviceid());
}


INT4   CRouterGateMgr::AddGatewayNode(CGateway*  GatewayNode)
{
	if(NULL == GatewayNode)
	{
		return BNC_ERR;
	}
	CGatewayMap::iterator it = m_gatewayMap.find(GatewayNode->getSubnetid());
	if((m_gatewayMap.end() != it)&&it->second)
	{
		it->second->setGatewayIp(GatewayNode->getGatewayIp());
		it->second->setNetworkid(GatewayNode->getNetworkid());
		it->second->setDeviceid(GatewayNode->getDeviceid());
		it->second->setStatus(CHECK_UPDATE);
	}
	else
	{
		m_gatewayMap.insert(std::make_pair(GatewayNode->getSubnetid(), GatewayNode));
	}
	return BNC_OK;
}

INT4   CRouterGateMgr::AddGatewayNode(const std::string & deviceid, const std::string & networkid,const std::string & subnetid, UINT4 ip)
{
	CGatewayMap::iterator it = m_gatewayMap.find(subnetid);
	if((m_gatewayMap.end() != it)&&it->second)
	{
		it->second->setGatewayIp(ip);
		it->second->setNetworkid(networkid);
		it->second->setDeviceid(deviceid);
		it->second->setStatus(CHECK_UPDATE);
	}
	else
	{
		CGateway*  gatewayNode = new CGateway(deviceid, networkid, subnetid, ip, CHECK_CREATE);
		if(NULL != gatewayNode)
		{
			m_gatewayMap.insert(std::make_pair(gatewayNode->getSubnetid(), gatewayNode));
		}
	}
	return BNC_OK;
}

INT4   CRouterGateMgr::DelGatewayNode(const std::string & subnetid)
{
	CGatewayMap::iterator it = m_gatewayMap.find(subnetid);
	if(m_gatewayMap.end() != it)
	{
		m_gatewayMap.erase(it);
		return BNC_OK;
		
	}
	return BNC_ERR;
}
CGateway*   CRouterGateMgr::FindGatewayNodeBySubnetId(const std::string & subnetid)
{
#if 0
    LOG_WARN_FMT("### searching subnetid[%s]", subnetid.c_str());
    STL_FOR_LOOP(m_gatewayMap, it)
    {
        INT1 ipStr[20] = {0};
        number2ip(it->second->getGatewayIp(), ipStr);
        LOG_WARN_FMT("### key[%s]: networkid[%s], subnetid[%s], gateway[%s]", 
            it->first.c_str(), it->second->getNetworkid().c_str(), it->second->getSubnetid().c_str(), ipStr);
    }
#endif
	CGatewayMap::iterator it = m_gatewayMap.find(subnetid);
	if(m_gatewayMap.end() == it)
	{
		LOG_WARN_FMT("can't find gateway by subnetid[%s] !!!", subnetid.c_str());
		return NULL;
	}
	return it->second;
}

CGateway*   CRouterGateMgr::FindGatewayNodeByHostIp(UINT4 hostip)
{
	CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByIp(hostip);
	if(host.isNull())
	{
		return NULL;
	}
	return FindGatewayNodeBySubnetId(host->getSubnetId());
}

CGateway*   CRouterGateMgr::FindGatewayNodeByHostMac(UINT1* mac)
{
	CSmartPtr<CHost> host = CHostMgr::getInstance()->findHostByMac(mac);
	if(host.isNull())
	{
        INT1 macStr[20] = {0};
        mac2str(mac, macStr);
		LOG_WARN_FMT("can't find host by mac[%s] !!!", macStr);
		return NULL;
	}
	return FindGatewayNodeBySubnetId(host->getSubnetId());
}

INT4   CRouterGateMgr::AddNode(const std::string & deviceid,const std::string & networkid,const std::string & subnetid,UINT4 ip,UINT1 type)
{
	INT4 ret= BNC_ERR;

	
	switch(type)
	{
		case bnc::host::HOST_ROUTER:
			ret = AddGatewayNode( deviceid, networkid, subnetid, ip);
			break;
		case bnc::host::HOST_GATEWAY:
			ret = AddRouterNode(  deviceid, networkid, subnetid, ip);
			break;
		default:
			break;
	}
	return ret;
}

INT4   CRouterGateMgr::DelNode(const std::string & deviceid,const std::string & networkid,const std::string & subnetid,UINT4 ip,UINT1 type)
{
	INT4 ret= BNC_ERR;

	switch(type)
	{
		case bnc::host::HOST_ROUTER:
			ret = DelGatewayNode( subnetid);
			break;
		case bnc::host::HOST_GATEWAY:
			ret = DelRouterNode(  deviceid);
			break;
		default:
			break;
	}
	return ret;
}
CRouterMap  CRouterGateMgr::getRouterMapHeader()
{
	return m_routerMap;
}
CGatewayMap CRouterGateMgr::getGatewayMapHeader()
{
	return m_gatewayMap;
}


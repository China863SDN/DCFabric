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
*   File Name   : CHostMgr.cpp		*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-27         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#include "CMutex.h"
#include "CHostNormal.h"
#include "CHostFloating.h"
#include "CHostMgr.h"
#include "CControl.h"
#include "CFloatingIp.h"
#include "CFloatingIpMgr.h"
#include "COpenstackMgr.h"
#include "COpenStackHost.h"
#include "COpenstackExternal.h"
#include "COpenstackExternalMgr.h"
#include "CSubnet.h"
#include "CSubnetMgr.h"
#include "CNetwork.h"
#include "CNetworkMgr.h"
#include "CHostTrack.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "CFirewallPolicy.h"
#include "CPortforwardPolicy.h"
#include "log.h"

CHostMgr* CHostMgr::m_instance = NULL;
CHostMgr* CHostMgr::getInstance()
{
	if (NULL == m_instance) 
    {
		m_instance = new CHostMgr();
		if (NULL == m_instance)
		{
			exit(-1);
		}
	}

	return (m_instance);
}

CHostMgr::CHostMgr()
{
}

CHostMgr::~CHostMgr()
{
}

INT4 CHostMgr::init()
{
    return m_track.init();
}

CSmartPtr<CHost> CHostMgr::findHostByMac(const UINT1* mac)
{
    CSmartPtr<CHost> host(NULL);

    if (NULL == mac)
    {
        return host;
    }

    m_lock.rlock();

	STL_FOR_LOOP(m_list, iter)
	{
        CSmartPtr<CHost> curr = *iter;
		if (curr.isNotNull() && (0 == memcmp(mac, curr->getMac(), 6)))
		{
			host = curr;
            break;
		}
	}

    m_lock.unlock();

	return host;
}

CSmartPtr<CHost> CHostMgr::findHostByIp(UINT4 ip)
{
    CSmartPtr<CHost> host(NULL);

    if (0 == ip)
    {
        return host;
    }

    m_lock.rlock();

	STL_FOR_LOOP(m_list, iter)
	{
        CSmartPtr<CHost> curr = *iter;
		if (curr.isNotNull() && (curr->getIp() == ip))
		{
			host = curr;
            break;
		}
	}

    m_lock.unlock();

	return host;
}

CSmartPtr<CHost> CHostMgr::findHostByIPAndMac(const UINT4 ip, const UINT1* mac)
{
    CSmartPtr<CHost> host(NULL);

    if ((0 == ip) || (NULL == mac))
    {
        return host;
    }

    m_lock.rlock();

	STL_FOR_LOOP(m_list, iter)
	{
        CSmartPtr<CHost> curr = *iter;
		if (curr.isNotNull() && (curr->getIp() == ip) && (0 == memcmp(mac, curr->getMac(), 6)))
		{
			host = curr;
            break;
		}
	}

    m_lock.unlock();

	return host;
}

CSmartPtr<CHost> CHostMgr::findHostByDpidAndPort(const UINT8 dpid, const UINT4 port)
{
    CSmartPtr<CHost> host(NULL);

    if (0 == dpid)
    {
        return host;
    }

    m_lock.rlock();

	STL_FOR_LOOP(m_list, iter)
	{
        CSmartPtr<CHost> curr = *iter;
		if (curr.isNotNull() && (curr->getDpid() == dpid) && (curr->getPortNo() == port))
		{
			host = curr;
            break;
		}
	}

    m_lock.unlock();

	return host;
}

CSmartPtr<CHost> CHostMgr::addHost(CSmartPtr<CSwitch> sw, 
                                   UINT4 portNo, 
                                   const UINT1* mac, 
                                   UINT4 ip, 
                                   UINT8 dpid)
{
    CSmartPtr<CHost> host(NULL);

	CSubnet* subnet = CSubnetMgr::getInstance()->findSubnetByIp(ip);
	if ((NULL == subnet) && CControl::getInstance()->isL3ModeOn())
	{
		return host;
	}

	if (NULL != subnet)
	{
		CNetwork* network =  CNetworkMgr::getInstance()->findNetworkById(subnet->get_networkid());
		//LOG_ERROR_FMT(" dstSubnet->get_networkid()=%s networknode=0x%x ",dstSubnet->get_networkid().c_str(),networknode);
		if ((NULL != network) && network->get_external())
		{
			LOG_DEBUG_FMT("network is external!!!");
			return host;
		}
	}

	host = findHostByMac(mac);
	LOG_DEBUG_FMT("%s:%d host=0x%p ip=0x%x",FN, LN, host.getPtr(), ip);

	if (host.isNull() && (0 != ip))
	{
	    // 在使用Openstack的情况下, 只能通过读取Openstack资源创建主机
        CHost* hostNew = new CHostNormal(sw, dpid, portNo, mac, ip);
        if (NULL == hostNew)
        {
            LOG_ERROR("new CHostNormal failed!!!");
            return host;
        }

        m_lock.wlock();
        
        host = CSmartPtr<CHost>(hostNew);
        m_list.push_back(host);

        m_lock.unlock();
        
	    if (CControl::getInstance()->isL3ModeOn())
	    {
             LOG_DEBUG_FMT("Add host. ip: 0x%x subnetid(): %s", ip, subnet->get_subnetid().c_str());

             host->setSubnetId(subnet->get_subnetid());
             host->setTenantId(subnet->get_tenantid());
             if (ip == subnet->get_subnetGateway())
             {
                 host->setHostType(bnc::host::HOST_ROUTER);
             }
	    }
	}
	else
	{
	    if (host.isNotNull() && ((host->getSw().isNull()) || (0 == host->getPortNo())))
	    {
	        host->setSw(sw);
	        host->setPortNo(portNo);
			host->setDpid(dpid);
            
            CFirewallPolicy::getInstance()->apply(mac);
            CPortforwardPolicy::getInstance()->apply(ntohl(ip));
	    }
	}

	return host;
}

CSmartPtr<CHost> CHostMgr::addHost(INT4 type,
                                   const UINT1* mac,
                                   UINT4 ip,
                                   const std::string & tenant_id,
                                   const std::string & network_id,
                                   const std::string & subnet_id,
                                   const std::string & port_id)
{
    CSmartPtr<CHost> host = findHostByMac(mac);
	if (host.isNotNull())
	{
		host->setHostType(bnc::host::host_type(type));
		host->setfixIp(ip);
		host->setSubnetId(subnet_id);
		host->setTenantId(tenant_id);

	    return host;
	}

	LOG_INFO_FMT("add openstack host. ip: %d, type: %d, port id: %s", ip, type, port_id.c_str());

    CHost* hostNew = NULL;

	if (bnc::openstack::device_floatingip == type)
	{
		hostNew = new CHostFloating(mac, ip, bnc::host::host_type(type), subnet_id, tenant_id);
        if (NULL == hostNew)
        {
            LOG_ERROR("new CHostFloating failed!!!");
            return host;
        }

		CFloatingIp* floatingHost  = CFloatingIpMgr::getInstance()->findFloatingIpNodeByfloatingIp(ip);
		if (NULL != floatingHost)
		{
			hostNew->setfixIp(floatingHost->getFixedIp());
		}
	}
	else
	{
        hostNew = new CHostNormal(mac, ip, bnc::host::host_type(type), subnet_id, tenant_id);
        if (NULL == hostNew)
        {
            LOG_ERROR("new CHostNormal failed!!!");
            return host;
        }
	}
	
    m_lock.wlock();

    host = CSmartPtr<CHost>(hostNew);
    m_list.push_back(host);

    m_lock.unlock();

	return host;
}

CSmartPtr<CHost> CHostMgr::addHost(INT4 type,
                                   const UINT1* mac,
                                   const std::vector<UINT4> & ipList,
                                   //UINT4 ip,
                                   UINT8 dpid,
                                   UINT4 port,
                                   const std::string & subnetId,
                                   const std::string & tenantId,
                                   UINT4 fixedIp)
{
    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);

    CSmartPtr<CHost> host = findHostByMac(mac);
	if (host.isNotNull())
	{
        if (host->getHostType() == (bnc::host::host_type)type)
        {
            host->setSw(sw);
            host->setDpid(dpid);
            host->setPortNo(port);
            host->setIpList(ipList);
            //host->setIp(ip);
            if (bnc::openstack::device_floatingip == type)
                host->setfixIp(fixedIp);
            host->setSubnetId(subnetId);
            host->setTenantId(tenantId);

            return host;
        }
        else
        {
            delHostByMac(mac);
        }
	}

#if 0
    INT1 macStr[20] = {0};
    mac2str(mac, macStr);
    INT1 ipStr[20] = {0}, fixedIpStr[20] = {0};
    number2ip(ipList[0], ipStr);
    number2ip(fixedIp, fixedIpStr);
    LOG_WARN_FMT("### CREATE host: type[%d]sw[0x%p]dpid[0x%llu]port[%u]mac[%s]ip[%s]fixedIp[%s]subnetId[%s]tenantId[%s]", 
        type, sw.getPtr(), dpid, port, macStr, ipStr, fixedIpStr, subnetId.c_str(), tenantId.c_str());
#endif

    CHost* hostNew = NULL;

	if (bnc::openstack::device_floatingip == type)
	{
		hostNew = new CHostFloating(sw, dpid, port, mac, ipList[0], (bnc::host::host_type)type, subnetId, tenantId);
        if (NULL == hostNew)
        {
            LOG_ERROR("new CHostFloating failed!!!");
            return host;
        }
        hostNew->setfixIp(fixedIp);
	}
	else
	{
        hostNew = new CHostNormal(sw, dpid, port, mac, ipList[0], (bnc::host::host_type)type, subnetId, tenantId);
        if (NULL == hostNew)
        {
            LOG_ERROR("new CHostNormal failed!!!");
            return host;
        }
	}
    hostNew->setIpList(ipList);

    m_lock.wlock();

    host = CSmartPtr<CHost>(hostNew);
    m_list.push_back(host);

    m_lock.unlock();

	return host;
}

BOOL CHostMgr::delHostByMac(const   UINT1* mac) 
{
    BOOL ret = FALSE;

	if (NULL == mac)
	{
	   return ret;
	}

    m_lock.wlock();

	STL_FOR_LOOP(m_list, iter)
	{
       CSmartPtr<CHost> curr = *iter;
       if (curr.isNotNull() && (0 == memcmp(mac, curr->getMac(), 6)))
	   {
		   m_list.erase(iter);
		   ret = TRUE;
           break;
	   }
	}

    m_lock.unlock();

	return ret;
}

BOOL CHostMgr::delHostBySw(const CSmartPtr<CSwitch> sw)
{
    BOOL ret = FALSE;

	if (sw.isNull())
	{
	   return ret;
	}

    m_lock.wlock();

    CHostList::iterator iter = m_list.begin();
	while (iter != m_list.end())
	{
        CSmartPtr<CHost> host = *iter;
        if (host.isNotNull() && host->getSw().isNotNull() && (host->getSw()->getDpid() == sw->getDpid()))
	    {
            CHostList::iterator curr = iter++;
		    m_list.erase(curr);
		    ret = TRUE;
	    }
        else
        {
            iter++;
        }
	}

	m_lock.unlock();

	return ret;
}

CSmartPtr<CHost> CHostMgr::getHostGateway(const CSmartPtr<CHost>& host) 
{
    CSmartPtr<CHost> ret(NULL);

    if (host.isNull())
    {
        return ret;
    }

    m_lock.rlock();

	STL_FOR_LOOP(m_list, iter)
	{
        CSmartPtr<CHost> curr = *iter;
        if (curr.isNotNull() && curr->isGateway() && curr->isSameSubnet(*host))
		{
			ret = curr;
            break;
		}
	}

    m_lock.unlock();

	return ret;
}

CSmartPtr<CHost> CHostMgr::getHostGatewayByHostIp(const UINT4 ip) 
{
    CSmartPtr<CHost> ret(NULL);

    if (0 == ip)
    {
        return ret;
    }

	CSubnet* SubnetNode = CSubnetMgr::getInstance()->findSubnetByIp(ip);
	if (NULL == SubnetNode)
	{
        return ret;
	}
	
    m_lock.rlock();

	STL_FOR_LOOP(m_list, iter)
	{
        CSmartPtr<CHost> curr = *iter;
		if (curr.isNotNull() && curr->isGateway() && (SubnetNode->get_subnetid() == curr->getSubnetId()))
		{
			ret = curr;
            break;
		}
	}

    m_lock.unlock();

	return ret;
}

CSmartPtr<CHost> CHostMgr::getHostDhcp(const CSmartPtr<CHost>& host) 
{
    CSmartPtr<CHost> ret(NULL);

    if (host.isNull())
    {
        return ret;
    }

    m_lock.rlock();

	STL_FOR_LOOP(m_list, iter)
	{
        CSmartPtr<CHost> curr = *iter;
		if (curr.isNotNull() && curr->isDhcp() && curr->isSameSubnet(*host))
		{
			ret = curr;
            break;
		}
	}

    m_lock.unlock();

	return ret;
}


UINT4 CHostMgr::getExternalHostPort(const CSmartPtr<CHost>& host) const
{
    UINT4 ret = 0;
    
    if (host.isNull())
    {
        return ret;
    }

    COpenStackHost* openstackhost = (COpenStackHost*)host.getPtr();
    COpenstackExternal* external = COpenstackExternalMgr::getInstance()->findOpenstackExternalBySubnetId(openstackhost->getSubnetId());
    if (external)
    {
        ret = external->getExternalPort();
    }

	return ret;
}

CSmartPtr<CSwitch> CHostMgr::getExternalHostSw(const CSmartPtr<CHost>& host) const
{
    CSmartPtr<CSwitch> ret(NULL);

    if (host.isNull())
    {
        return ret;
    }

    COpenStackHost* openstackhost = (COpenStackHost*)host.getPtr();
    COpenstackExternal* external = COpenstackExternalMgr::getInstance()->findOpenstackExternalBySubnetId(openstackhost->getSubnetId());
    if (external)
    {
        ret = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(external->getExternalDpid());
    }

    return ret;
}


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
*   File Name   : CSwitchMgr.cpp                                              *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CSwitchMgr.h"
#include "log.h"
#include "bnc-type.h"
#include "bnc-error.h"
#include "CControl.h"
#include "CHostMgr.h"
#include "comm-util.h"
#include "CControl.h"
#include "COfConnectMgr.h"
#include "CTagMgr.h"
#include "CHostMgr.h"
#include "CMemPool.h"
#include "CClusterService.h"
#include "openflow-common.h"
#include "CConf.h"

//#define USING_SWITCH_MEM_POOL 1

#ifdef USING_SWITCH_MEM_POOL
static void deleteSwitch(CSwitch* sw)
{
    if (NULL != sw)
        CControl::getInstance()->getSwitchMgr().releaseSwitch(sw);
}
#endif

CSwitchMgr::CSwitchMgr():
    m_sws(hash_bucket_number),
    m_macMap(hash_bucket_number),
    m_ipMap(hash_bucket_number),
    m_dpidMap(hash_bucket_number),
    m_tagMap(hash_bucket_number),
    m_swPool(TRUE)
{
}

CSwitchMgr::~CSwitchMgr()
{
}

INT4 CSwitchMgr::init()
{
#ifdef USING_SWITCH_MEM_POOL
    m_swPool.init(sizeof(CSwitch), switch_mem_node_size, "CSwitchMgr");
#endif
    
    return BNC_OK;
}

INT4 CSwitchMgr::addSwitch(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port)
{
    CSmartPtr<CSwitch> sw;
    
    INT1 macStr[20] = {0};
    LOG_DEBUG_FMT("addSwitch with sockfd[%d]mac[%s]ip[%s]port[%u]", 
        sockfd, mac2str((UINT1*)mac, macStr), inet_htoa(ip), port);

    if (isValidMac(mac))
    {
        CSmartPtr<CSwitch> swOld = getSwitchByMac(mac);
        if (swOld.isNotNull())
        {
            LOG_DEBUG_FMT("getSwitchByMac[%s] returns sockfd[%d]mac[%s]ip[%s]port[%u]", 
                mac2str((UINT1*)mac, macStr), swOld->getSockfd(), 
                mac2str((UINT1*)swOld->getSwMac(), macStr), inet_htoa(swOld->getSwIp()), swOld->getSwPort());
            
            m_rwlock.wlock();
            delMacMap(mac);
            delIpMap(swOld->getSwIp());
            delDpidMap(swOld->getDpid());
            delTagMap(swOld->getTag());
            delSwitchMap(swOld->getSockfd());
            m_rwlock.unlock();

            switch_dupl_conn_t duplConn = {swOld->getSwIp(), swOld->getSwPort(), swOld->getSockfd(), swOld->getState()};

            sw = swOld;
            sw->setSockfd(sockfd);
            sw->setSwIp(ip);
            sw->setSwPort(port);
            sw->m_heartbeatTimes = 0;
            sw->m_heartbeatInterim = 0;

            COfConnectMgr::processPeerReconn(sw, duplConn);
        }
        else
        {
#ifdef USING_SWITCH_MEM_POOL
            CSwitch* swNew = allocSwitch();
            if (NULL == swNew)
            {
                LOG_ERROR_FMT("allocSwitch failed !");
                return BNC_ERR;
            }
            new(swNew) CSwitch(sockfd, mac, ip, port);
            sw = CSmartPtr<CSwitch>(swNew, deleteSwitch);
#else
            CSwitch* swNew = new CSwitch(sockfd, mac, ip, port);
            if (NULL == swNew)
            {
                LOG_ERROR_FMT("new CSwitch failed !");
                return BNC_ERR;
            }
            sw = CSmartPtr<CSwitch>(swNew);
#endif
            
            COfConnectMgr::processPeerConnect(sw);
        }

        sw->setState(SW_STATE_NEW_ACCEPT);

        m_rwlock.wlock();
        addSwitchMap(sockfd, sw);
        addMacMap(mac, sw);
        addIpMap(ip, sw);
        m_rwlock.unlock();
    }
    else
    {
        LOG_DEBUG_FMT("addSwitch with sockfd[%d]invalid mac[%s]ip[%s]port[%u]", 
            sockfd, mac2str((UINT1*)mac, macStr), inet_htoa(ip), port);

        //seems mininet sws have all zero mac and same ip
#ifdef USING_SWITCH_MEM_POOL
        CSwitch* swNew = allocSwitch();
        if (NULL == swNew)
        {
            LOG_ERROR_FMT("allocSwitch failed !");
            return BNC_ERR;
        }
        new(swNew) CSwitch(sockfd, mac, ip, port);
        sw = CSmartPtr<CSwitch>(swNew, deleteSwitch);
#else
        CSwitch* swNew = new CSwitch(sockfd, mac, ip, port);
        if (NULL == swNew)
        {
            LOG_ERROR_FMT("new CSwitch failed !");
            return BNC_ERR;
        }
        sw = CSmartPtr<CSwitch>(swNew);
#endif

        COfConnectMgr::processPeerConnect(sw);
        
        sw->setState(SW_STATE_NEW_ACCEPT);
        if (CConf::getInstance()->isMininet(ip))
            sw->setState(SW_STATE_STABLE);

        m_rwlock.wlock();
        addSwitchMap(sockfd, sw);
        m_rwlock.unlock();
    }

    if (OFPCR_ROLE_SLAVE != CClusterService::getInstance()->getControllerRole())
    {
        UINT4 tag = CControl::getInstance()->getTagMgr().alloc();
        if (CTagMgr::invalid_tag != tag)
        {
            sw->setTag(tag);
            addTagMap(tag, sw);
        }
    }

    return BNC_OK;
}

INT4 CSwitchMgr::addSwitch(CSmartPtr<CSwitch> sw)
{
    if (sw.isNull())
        return BNC_ERR;

    m_rwlock.wlock();
    addSwitchMap(sw->getSockfd(), sw);
    addMacMap(sw->getSwMac(), sw);
    addIpMap(sw->getSwIp(), sw);
    if (sw->getDpid() != 0)
        addDpidMap(sw->getDpid(), sw);
    m_rwlock.unlock();

    return BNC_OK;
}

void CSwitchMgr::delSwitch(INT4 sockfd)
{
    LOG_INFO_FMT("before delete switch with sockfd[%d], switch map size[%lu] ...", 
        sockfd, m_sws.size());

    m_rwlock.wlock();

    CSmartPtr<CSwitch> sw = getSwitch(sockfd);
    if (sw.isNotNull())
    {
        sw->setState(SW_STATE_CLOSED);

        CControl::getInstance()->getTopoMgr().deleteSwitch(sw->getDpid());

		CHostMgr::getInstance()->delHostBySw(sw);

        delTagMap(sw->getTag());
		CControl::getInstance()->getTagMgr().release(sw->getTag());

        if (isValidMac(sw->getSwMac()))
        {
            delMacMap(sw->getSwMac());
            delIpMap(sw->getSwIp());
        }
        delDpidMap(sw->getDpid());
        delSwitchMap(sockfd);
    }

    m_rwlock.unlock();

    LOG_INFO_FMT("after delete switch with sockfd[%d], switch map size[%lu] ...", 
        sockfd, m_sws.size());
}
void CSwitchMgr::delSwitchByDpid(UINT8 dpid)
{
	LOG_INFO_FMT("before delete switch with dpid[0x%llx], switch map size[%lu] ...", 
		dpid, m_sws.size());

    m_rwlock.wlock();

	CSmartPtr<CSwitch> sw = getSwitchByDpid(dpid);
	if (sw.isNotNull())
	{
		sw->setState(SW_STATE_CLOSED);

		CControl::getInstance()->getTopoMgr().deleteSwitch(sw->getDpid());

		CHostMgr::getInstance()->delHostBySw(sw);

        delTagMap(sw->getTag());
		CControl::getInstance()->getTagMgr().release(sw->getTag());

		if (isValidMac(sw->getSwMac()))
		{
			delMacMap(sw->getSwMac());
			delIpMap(sw->getSwIp());
		}
		delDpidMap(sw->getDpid());
		delSwitchMap(sw->getSockfd());
	}

    m_rwlock.unlock();

	LOG_INFO_FMT("after delete switch with dpid[0x%llx], switch map size[%lu] ...", 
		dpid, m_sws.size());
}

void CSwitchMgr::updSwitch(CSmartPtr<CSwitch> swOld, INT4 sockfd, CSmartPtr<CSwitch> swNew)
{
    m_rwlock.wlock();

    delSwitchMap(swOld->getSockfd());
    delMacMap(swOld->getSwMac());
    delIpMap(swOld->getSwIp());
    delDpidMap(swOld->getDpid());
    delTagMap(swOld->getTag());

    addSwitchMap(sockfd, swNew);
    addMacMap(swNew->getSwMac(), swNew);
    addIpMap(swNew->getSwIp(), swNew);
    addDpidMap(swNew->getDpid(), swNew);
    addTagMap(swNew->getTag(), swNew);

    m_rwlock.unlock();
}

CSmartPtr<CSwitch> CSwitchMgr::getSwitch(INT4 sockfd)
{
    CSmartPtr<CSwitch> sw;

#if 0
    //m_mutex.lock();
    m_rwlock.rlock();

    CSwitchMap::iterator it = m_sws.find(sockfd);
    if (it != m_sws.end())
    {
        sw = it->second;
    }

    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    CSwitchHMap::CPair* item = NULL;
    if (m_sws.find(sockfd, &item))
        sw = item->second;
#endif

    return sw;
}

CSmartPtr<CSwitch> CSwitchMgr::getSwitchByMac(const INT1* mac)
{
    CSmartPtr<CSwitch> sw(NULL);

    if (NULL == mac)
        return sw;

    INT1 macStr[20] = {0};
    mac2str((UINT1*)mac, macStr);

    CMacHMap::CPair* item = NULL;
    if (m_macMap.find(macStr, &item))
        sw = item->second;

    return sw;
}

CSmartPtr<CSwitch> CSwitchMgr::getSwitchByIp(UINT4 ip)
{
    CSmartPtr<CSwitch> sw(NULL);

    CIpHMap::CPair* item = NULL;
    if (m_ipMap.find(ip, &item))
        sw = item->second;

    return sw;
}

CSmartPtr<CSwitch> CSwitchMgr::getSwitchByDpid(UINT8 dpid)
{
    CSmartPtr<CSwitch> sw(NULL);

    CDpidHMap::CPair* item = NULL;
    if (m_dpidMap.find(dpid, &item))
        sw = item->second;

    return sw;
}

CSmartPtr<CSwitch> CSwitchMgr::getSwitchByTag(UINT4 tag)
{
    CSmartPtr<CSwitch> sw(NULL);

    CTagHMap::CPair* item = NULL;
    if (m_tagMap.find(tag, &item))
        sw = item->second;

    return sw;
}

void CSwitchMgr::addSwitchMap(INT4 sockfd, CSmartPtr<CSwitch>& sw)
{
    LOG_INFO_FMT("before add switch with sockfd[%d]ip[%x]port[%u], switch map size[%lu] ...", 
        sockfd, sw->getSwIp(), sw->getSwPort(), m_sws.size());

#if 0
    //m_mutex.lock();
    m_rwlock.wlock();
    m_sws.insert(CSwitchMap::value_type(sockfd, sw));
    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    if (!m_sws.insert(sockfd, sw))
        LOG_WARN_FMT("add mapping sockfd[%d] to CSwitch failed !", sockfd);
#endif

    LOG_INFO_FMT("after add switch with sockfd[%d]ip[%x]port[%u], switch map size[%lu] ...", 
        sockfd, sw->getSwIp(), sw->getSwPort(), m_sws.size());
}

void CSwitchMgr::addMacMap(const INT1* mac, CSmartPtr<CSwitch>& sw)
{
    if (NULL == mac)
        return;

    INT1 macStr[20] = {0};
    mac2str((UINT1*)mac, macStr);
    
    LOG_DEBUG_FMT("before add mapping mac[%s], m_macMap size[%lu] ...", macStr, m_macMap.size());

#if 0
    //m_mutex.lock();
    m_rwlock.wlock();
#if 1
    m_macSockfdMap.insert(CMacSockfdMap::value_type(macStr, sockfd));
#else
    m_macSockfdMap.insert(macStr, sockfd);
#endif
    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    if (!m_macMap.insert(macStr, sw))
        LOG_WARN_FMT("add mapping mac[%s] failed !", macStr);
#endif

    LOG_DEBUG_FMT("after add mapping mac[%s], m_macMap size[%lu] ...", macStr, m_macMap.size());
}

void CSwitchMgr::addIpMap(UINT4 ip, CSmartPtr<CSwitch>& sw)
{
    LOG_INFO_FMT("before add mapping ip[%x], m_ipMap size[%lu] ...", ip, m_ipMap.size());

#if 0
    //m_mutex.lock();
    m_rwlock.wlock();
#if 0
    m_ipSockfdMap.insert(CIpSockfdMap::value_type(ip, sockfd));
#else
    m_ipSockfdMap.insert(ip, sockfd);
#endif
    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    if (!m_ipMap.insert(ip, sw))
        LOG_WARN_FMT("add mapping ip[%x] failed !", ip);
#endif

    LOG_INFO_FMT("after add mapping ip[%x], m_ipMap size[%lu] ...", ip, m_ipMap.size());
}

void CSwitchMgr::addDpidMap(UINT8 dpid, CSmartPtr<CSwitch>& sw)
{
    LOG_INFO_FMT("before add mapping dpid[%llx], m_dpidMap size[%lu] ...", dpid, m_dpidMap.size());

#if 0
    //m_mutex.lock();
    m_rwlock.wlock();
#if 0
    m_dpidSockfdMap.insert(CDpidSockfdMap::value_type(dpid, sockfd));
#else
    m_dpidSockfdMap.insert(dpid, sockfd);
#endif
    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    if (!m_dpidMap.insert(dpid, sw))
        LOG_WARN_FMT("add mapping dpid[%llx] failed !", dpid);
#endif

    LOG_INFO_FMT("after add mapping dpid[%llx], m_dpidMap size[%lu] ...", dpid, m_dpidMap.size());
}

void CSwitchMgr::addTagMap(UINT4 tag, CSmartPtr<CSwitch>& sw)
{
    LOG_INFO_FMT("before add mapping tag[%u], m_tagMap size[%lu] ...", tag, m_tagMap.size());

    if (!m_tagMap.insert(tag, sw))
        LOG_WARN_FMT("add mapping tag[%u] failed !", tag);

    LOG_INFO_FMT("after add mapping tag[%u], m_tagMap size[%lu] ...", tag, m_tagMap.size());
}

void CSwitchMgr::delSwitchMap(INT4 sockfd)
{
    LOG_INFO_FMT("before delete switch with sockfd[%d], switch map size[%lu] ...", 
        sockfd, m_sws.size());

#if 0
    //m_mutex.lock();
    m_rwlock.wlock();
    m_sws.erase(sockfd);
    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    m_sws.erase(sockfd);
#endif

    LOG_INFO_FMT("after delete switch with sockfd[%d], switch map size[%lu] ...", 
        sockfd, m_sws.size());
}

void CSwitchMgr::delMacMap(const INT1* mac)
{
    if (NULL == mac)
        return;

    INT1 macStr[20] = {0};
    mac2str((UINT1*)mac, macStr);
    
    LOG_DEBUG_FMT("before delete mapping mac[%s], m_macMap size[%lu] ...", macStr, m_macMap.size());

#if 0
    //m_mutex.lock();
    m_rwlock.wlock();
#if 1
    m_macSockfdMap.erase(macStr);
#else
    m_macSockfdMap.remove(macStr);
#endif
    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    m_macMap.erase(macStr);
#endif

    LOG_DEBUG_FMT("after delete mapping mac[%s], m_macMap size[%lu] ...", macStr, m_macMap.size());
}

void CSwitchMgr::delIpMap(UINT4 ip)
{
    LOG_INFO_FMT("before delete mapping ip[%x], m_ipMap size[%lu] ...", ip, m_ipMap.size());

#if 0
    //m_mutex.lock();
    m_rwlock.wlock();
#if 0
    m_ipSockfdMap.erase(ip);
#else
    m_ipSockfdMap.remove(ip);
#endif
    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    m_ipMap.erase(ip);
#endif

    LOG_INFO_FMT("after delete mapping ip[%x], m_ipMap size[%lu] ...", ip, m_ipMap.size());
}

void CSwitchMgr::delDpidMap(UINT8 dpid)
{
    LOG_INFO_FMT("before delete mapping dpid[%llx], m_dpidMap size[%lu] ...", dpid, m_dpidMap.size());

#if 0
    //m_mutex.lock();
    m_rwlock.wlock();
#if 0
    m_dpidSockfdMap.erase(dpid);
#else
    m_dpidSockfdMap.remove(dpid);
#endif
    //m_mutex.unlock();
    m_rwlock.unlock();
#else
    m_dpidMap.erase(dpid);
#endif

    LOG_INFO_FMT("after delete mapping dpid[%llx], m_dpidMap size[%lu] ...", dpid, m_dpidMap.size());
}

void CSwitchMgr::delTagMap(UINT4 tag)
{
    LOG_INFO_FMT("before delete mapping tag[%u], m_tagMap size[%lu] ...", tag, m_tagMap.size());

    m_tagMap.erase(tag);

    LOG_INFO_FMT("after delete mapping tag[%u], m_tagMap size[%lu] ...", tag, m_tagMap.size());
}

CSwitch* CSwitchMgr::allocSwitch()
{
    CSwitch* sw = NULL;

    p_mem_node_t node = m_swPool.alloc();
    if (NULL != node)
        sw = (CSwitch*)node->data;

    return sw;
}

void CSwitchMgr::releaseSwitch(CSwitch* sw)
{
    if (NULL != sw)
    {
        p_mem_node_t node = (p_mem_node_t)((INT1*)sw - sizeof(mem_node_t));
        m_swPool.release(node);
    }
}

void CSwitchMgr::lock()
{
    m_rwlock.rlock();
}

void CSwitchMgr::unlock()
{
    m_rwlock.unlock();
}


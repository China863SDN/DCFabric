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
*   File Name   : CTopoMgr.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-7-21           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "CTopoMgr.h"
#include "CServer.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "CSwitch.h"
#include "COfMsgUtil.h"
#include "comm-util.h"
#include "log.h"
#include "bnc-error.h"
#include "CConf.h"
#include "CControl.h"
#include "CLinkEventReportor.h"
#include "CNotifyMgr.h"
#include "CFlowMgr.h"
#include "CEvent.h"
#include "CLinkEventNotifier.h"
#include "CPortEventNotifier.h"
#include "CTagFlowEventReportor.h"
#include "CClusterSync.h"


//路径默认更新周期，单位秒
static const UINT4 PATH_CHANGE_INTERVAL_DEFAULT = 5;

#if 0
static void update(void* param)
{
    if (NULL == param)
        return;

    CTopoMgr* topoMgr = (CTopoMgr*)param;
    if (TRUE == topoMgr->getPathChangeFlag())
    {
		topoMgr->updatePath();
		topoMgr->setPathChangeFlag(FALSE);
	}
}
#endif

static void* update(void* param)
{
	BOOL   updateflag = FALSE;
	UINT4  last_change_link_num = 0;
	
    if (NULL == param)
        return NULL;

	prctl(PR_SET_NAME, (unsigned long)"TopoUpdate");  
	usleep(5*1000*1000);
    CTopoMgr* topoMgr = (CTopoMgr*)param;
    while(1)
    {
        try
        {
			topoMgr->getLinkEventSem().wait();
		
			if(topoMgr->getLinkEventList().size() > 0 )
			{
				while(1)
				{
					while(last_change_link_num < topoMgr->getLinkEventList().size())
					{
						LOG_WARN_FMT("0 CTopoMgr update last_change_link_num=%d size=%lu !", last_change_link_num,topoMgr->getLinkEventList().size() );
						last_change_link_num = topoMgr->getLinkEventList().size();
					
						LOG_WARN_FMT("1 CTopoMgr update last_change_link_num=%d size=%lu !", last_change_link_num,topoMgr->getLinkEventList().size() );
						usleep(50000);
						updateflag = TRUE;
					}
					usleep(1000000);
					if(last_change_link_num == topoMgr->getLinkEventList().size())
					{
						break;
					}
				}
				LOG_WARN_FMT("2 CTopoMgr update last_change_link_num=%d size=%lu updateflag=%d  !", last_change_link_num,topoMgr->getLinkEventList().size() ,updateflag);
				if(updateflag)
				{
					int time1 = time(NULL);
					topoMgr->popLinkEventList();
					topoMgr->updatePath();
					updateflag = FALSE;
					last_change_link_num =0 ;
					int time2 = time(NULL);
				
					LOG_WARN_FMT("##########CTopoMgr update last_change_link_num=%d size=%lu time2-time1=%d!##############", last_change_link_num,topoMgr->getLinkEventList().size(),time2-time1 );
    			}
    		}

            //following will happen in slave
            if (topoMgr->checkPathChanged())
            {
                topoMgr->updatePath();
                topoMgr->updatePathChanged(FALSE);
            }
        }
        catch (...)
        {
            LOG_WARN("TopoUpdate catch exception !");
        }
	}
}

CTopoMgr::CTopoMgr():
    m_neighbors(neighbor_map_granularity),
    m_neighMutexes(neighbor_map_granularity),
    m_pathChanged(FALSE),
    m_thread(THREAD_TYPE_OCCUPY_CORE),
    m_linkeventNotify(NULL),
    m_porteventNotify(NULL)
{
}

CTopoMgr::~CTopoMgr()
{
	if(NULL != m_linkeventNotify)
	{
		delete m_linkeventNotify;
		m_linkeventNotify = NULL;
	}
	if(NULL != m_porteventNotify)
	{
		delete m_porteventNotify;
		m_porteventNotify = NULL;
	}
}

INT4 CTopoMgr::init()
{
    const INT1* pInterval = CConf::getInstance()->getConfig("controller", "path_change_interval");
    UINT4 interval = (pInterval == NULL) ? PATH_CHANGE_INTERVAL_DEFAULT : atol(pInterval);
    LOG_INFO_FMT("path_change_interval %us", interval);

    if (0 == interval)
    {
    	LOG_ERROR("interval can't be  0!!!");
        return BNC_OK;
    }

	if (m_thread.init(update, this, "CTopoMgr") != BNC_OK)
    {
        LOG_ERROR("init thread for CTopoMgr failed !");
        return BNC_ERR;
    }
	
	m_linkeventNotify = new CLinkEventNotifier();
	m_linkeventNotify->onregister();

	m_porteventNotify = new CPortEventNotifier();
	m_porteventNotify->onregister();

    return BNC_OK;
}

INT4 CTopoMgr::addSwitch(UINT8 dpid)
{
    if (0 == dpid)
    {
        LOG_WARN_FMT("add switch with invalid dpid[%llx] ...", dpid);
        return BNC_ERR;
    }

    UINT4 mapPos = (dpid & 0xFFFF) % neighbor_map_granularity;

    LOG_INFO_FMT(">> before add switch with dpid[%llx], map[%u] has %lu neighbors ...", 
        dpid, mapPos, m_neighbors[mapPos].size());

    std::map<UINT4, neighbor_t> portMap;

    m_neighMutexes[mapPos].lock();

    CNeighborMap::iterator find = m_neighbors[mapPos].find(dpid);
    if (find == m_neighbors[mapPos].end())
    {
        m_neighbors[mapPos].insert(std::pair<UINT8, std::map<UINT4, neighbor_t> >(dpid, portMap));
    }
    else
    {
        LOG_INFO_FMT("switch with dpid[%llx] is already in map[%u] ...", dpid, mapPos);
    }

    m_neighMutexes[mapPos].unlock();

    LOG_INFO_FMT("<< after add switch with dpid[%llx], map[%u] has %lu neighbors ...", 
        dpid, mapPos, m_neighbors[mapPos].size());
    return BNC_OK;
}

void CTopoMgr::deleteSwitch(UINT8 dpid)
{
    if (0 == dpid)
    {
        return;
    }

    UINT4 mapPos = (dpid & 0xFFFF) % neighbor_map_granularity;

    LOG_INFO_FMT(">> before delete switch with dpid[%llx], map[%u] has %lu neighbors ...", 
        dpid, mapPos, m_neighbors[mapPos].size());

    m_neighMutexes[mapPos].lock();

    CNeighborMap::iterator dpidIt = m_neighbors[mapPos].find(dpid);
    if (dpidIt != m_neighbors[mapPos].end())
    {
        CNeighbors& portMap = dpidIt->second;
        for (CNeighbors::iterator portIt = portMap.begin(); portIt != portMap.end(); portIt++)
        {
            neighbor_t& neighbor = portIt->second;
            if (NEIGH_STATE_DELETED != neighbor.state)
            {
                LOG_INFO_FMT("map[%u] delete line: dpid[%llx]port[%u]neighborDpid[%llx]neighborPort[%u]", 
                    mapPos, neighbor.src_dpid, neighbor.src_port, neighbor.dst_dpid, neighbor.dst_port);
                neighbor.state = NEIGH_STATE_DELETED;

                //m_pathChanged = TRUE;
                CLinkEventReportor::getInstance()->report(EVENT_TYPE_TOPO_LINK_DELETE, EVENT_REASON_SWITCH_POWER_OFF, 
                    neighbor.src_dpid, neighbor.src_port, neighbor.dst_dpid, neighbor.dst_port);
                CClusterSync::syncTopoLink(neighbor);
            }
        }

        m_neighbors[mapPos].erase(dpidIt);
    }

    m_neighMutexes[mapPos].unlock();

    LOG_INFO_FMT("<< after delete switch with dpid[%llx], map[%u] has %lu neighbors ...", 
        dpid, mapPos, m_neighbors[mapPos].size());
}

INT4 CTopoMgr::updateLink(UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort)
{
    if ((0 == dpid) || (0 == neighDpid))
    {
        LOG_WARN_FMT("updateLink with invalid dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u] ...", 
            dpid, port, neighDpid, neighPort);
        return BNC_ERR;
    }

    UINT4 mapPos = (dpid & 0xFFFF) % neighbor_map_granularity;

    LOG_INFO_FMT(">> before updateLink with dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u], map[%u] has %lu neighbors ...", 
        dpid, port, neighDpid, neighPort, mapPos, m_neighbors[mapPos].size());
    
    BOOL pathChanged = FALSE;
    INT4 event = EVENT_TYPE_NONE;
    INT4 reason = EVENT_REASON_NONE;

    neighbor_t neighInsert = {NEIGH_STATE_ESTABLISHED, dpid, port, neighDpid, neighPort, 0};
    CNeighbors portMapInsert;
    portMapInsert.insert(std::pair<UINT4, neighbor_t>(port, neighInsert));

    m_neighMutexes[mapPos].lock();

    CNeighborMap::iterator dpidIt = m_neighbors[mapPos].find(dpid);
    if (dpidIt == m_neighbors[mapPos].end())
    {
        m_neighbors[mapPos].insert(std::pair<UINT8, CNeighbors>(dpid, portMapInsert));

        LOG_INFO_FMT("after create neighbor map, switch with dpid[%llx] have one neighbor with dpid[%llx]port[%u]",
            dpid, neighDpid, neighPort);
		updateSwPortType(dpid,port, neighDpid, neighPort);

        pathChanged = TRUE;
        event = EVENT_TYPE_TOPO_LINK_ESTABLISH;
        reason = EVENT_REASON_TOPO_LINK_ESTABLISH;
    }
    else
    {
        std::map<UINT4, neighbor_t>& portMap = dpidIt->second;

        LOG_INFO_FMT("before update neighbor map, switch with dpid[%llx] have %lu neighbors",
            dpid, portMap.size());

        std::map<UINT4, neighbor_t>::iterator portIt = portMap.find(port);
        if (portIt != portMap.end())
        {
            neighbor_t& neighbor = portIt->second;
            if ((neighDpid != neighbor.dst_dpid) || (neighPort != neighbor.dst_port))
            {
                neighbor.state = NEIGH_STATE_ESTABLISHED;
                neighbor.src_dpid = dpid;
                neighbor.src_port = port;
                neighbor.dst_dpid = neighDpid;
                neighbor.dst_port = neighPort;

                pathChanged = TRUE;
                event = EVENT_TYPE_TOPO_LINK_ESTABLISH;
                reason = EVENT_REASON_TOPO_LINK_ESTABLISH;
            }
            else if (NEIGH_STATE_UNALIVE == neighbor.state)
            {
                neighbor.state = NEIGH_STATE_ESTABLISHED;

                pathChanged = TRUE;
                event = EVENT_TYPE_TOPO_LINK_RECOVER;
                reason = EVENT_REASON_TOPO_LINK_RECOVER;
            }
            else if (NEIGH_STATE_DELETED == neighbor.state)
            {
                neighbor.state = NEIGH_STATE_ESTABLISHED;

                pathChanged = TRUE;
                event = EVENT_TYPE_TOPO_LINK_ESTABLISH;
                reason = EVENT_REASON_TOPO_LINK_ESTABLISH;
            }
        }
        else
        {
            portMap.insert(std::pair<UINT4, neighbor_t>(port, neighInsert));
            updateSwPortType(dpid,port, neighDpid, neighPort);

            pathChanged = TRUE;
            event = EVENT_TYPE_TOPO_LINK_ESTABLISH;
            reason = EVENT_REASON_TOPO_LINK_ESTABLISH;
        }
        
        LOG_INFO_FMT("after update neighbor map, switch with dpid[%llx] have %lu neighbors",
            dpid, portMap.size());
    }

    m_neighMutexes[mapPos].unlock();

    LOG_INFO_FMT("<< after updateLink with dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u], map[%u] has %lu neighbors ...", 
        dpid, port, neighDpid, neighPort, mapPos, m_neighbors[mapPos].size());

    if (pathChanged)
    {
        //m_pathChanged = TRUE;
        CLinkEventReportor::getInstance()->report(event, reason, dpid, port, neighDpid, neighPort);
        CClusterSync::syncTopoLink(neighInsert);
    }

    return BNC_OK;
}

void CTopoMgr::deleteLink(UINT8 dpid, UINT4 port)
{
    if (0 == dpid)
    {
        return;
    }

    UINT4 mapPos = (dpid & 0xFFFF) % neighbor_map_granularity;

    LOG_INFO_FMT(">> before delete link with dpid[%llx], map[%u] has %lu neighbors ...", 
        dpid, mapPos, m_neighbors[mapPos].size());

    m_neighMutexes[mapPos].lock();

    CNeighborMap::iterator dpidIt = m_neighbors[mapPos].find(dpid);
    if (dpidIt != m_neighbors[mapPos].end())
    {
        CNeighbors& portMap = dpidIt->second;
        CNeighbors::iterator portIt = portMap.find(port);
        if (portIt != portMap.end())
        {
            neighbor_t& neighbor = portIt->second;
            if (NEIGH_STATE_DELETED != neighbor.state)
            {
                LOG_INFO_FMT("map[%u] delete link: dpid[%llx]port[%u]neighborDpid[%llx]neighborPort[%u]", 
                    mapPos, dpid, port, neighbor.dst_dpid, neighbor.dst_port);
                neighbor.state = NEIGH_STATE_DELETED;

                //m_pathChanged = true;                
                CLinkEventReportor::getInstance()->report(EVENT_TYPE_TOPO_LINK_DELETE, EVENT_REASON_PORT_DOWN, dpid, port, neighbor.dst_dpid, neighbor.dst_port);
                CClusterSync::syncTopoLink(neighbor);
            }
        }
    }

    m_neighMutexes[mapPos].unlock();

    LOG_INFO_FMT("<< after delete link with dpid[%llx], map[%u] has %lu neighbors ...", 
        dpid, mapPos, m_neighbors[mapPos].size());
}

BOOL CTopoMgr::isLinkUp(UINT8 dpid, UINT4 port)
{
    neighbor_t* link = getLink(dpid, port);
    if ((NULL == link) || (NEIGH_STATE_DELETED == link->state))
        return FALSE;

    neighbor_t* neighLink = getLink(link->dst_dpid, link->dst_port);
    if ((NULL == neighLink) || (NEIGH_STATE_DELETED == neighLink->state))
        return FALSE;

    if ((link->src_dpid != neighLink->dst_dpid) || (link->src_port != neighLink->dst_port))
        return FALSE;

    CSmartPtr<CSwitch> neighSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(link->dst_dpid);
    if (neighSw.isNull() || (neighSw->getState() >= SW_STATE_POWER_OFF))
        return FALSE;

    gn_port_t* neighPort = neighSw->getPort(link->dst_port);
    if ((NULL == neighPort) || (PORT_STATE_UP != neighPort->state))
        return FALSE;

    return TRUE;
}

BOOL CTopoMgr::isLinkAlive(UINT8 dpid, UINT4 port)
{
    neighbor_t* link = getLink(dpid, port);
    if (NULL != link)
        return (NEIGH_STATE_ESTABLISHED == link->state);

    return FALSE;
}

void CTopoMgr::unAliveLink(UINT8 dpid, UINT4 port)
{
    neighbor_t* link = getLink(dpid, port);
    if (NULL != link)
    {
        link->state = NEIGH_STATE_UNALIVE;
        
        CLinkEventReportor::getInstance()->report(EVENT_TYPE_TOPO_LINK_UNALIVE, EVENT_REASON_TOPO_LINK_UNALIVE, dpid, port, link->dst_dpid, link->dst_port);
    }
}

INT4 CTopoMgr::recoverLink(neighbor_t& neigh)
{
    if ((0 == neigh.src_dpid) || (0 == neigh.dst_dpid))
    {
        LOG_WARN_FMT("recoverLink with invalid dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u] ...", 
            neigh.src_dpid, neigh.src_port, neigh.dst_dpid, neigh.dst_port);
        return BNC_ERR;
    }

    UINT4 mapPos = (neigh.src_dpid & 0xFFFF) % neighbor_map_granularity;

    LOG_INFO_FMT(">> before recoverLink with dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u], map[%u] has %lu neighbors ...", 
        neigh.src_dpid, neigh.src_port, neigh.dst_dpid, neigh.dst_port, mapPos, m_neighbors[mapPos].size());

    m_neighMutexes[mapPos].lock();

    CNeighborMap::iterator dpidIt = m_neighbors[mapPos].find(neigh.src_dpid);
    if (dpidIt == m_neighbors[mapPos].end())
    {
        CNeighbors portMapInsert;
        portMapInsert.insert(std::pair<UINT4, neighbor_t>(neigh.src_port, neigh));
        m_neighbors[mapPos].insert(std::pair<UINT8, CNeighbors>(neigh.src_dpid, portMapInsert));

        LOG_INFO_FMT("after create neighbor map, switch with dpid[%llx] have one neighbor with dpid[%llx]port[%u]",
            neigh.src_dpid, neigh.dst_dpid, neigh.dst_port);
    }
    else
    {
        std::map<UINT4, neighbor_t>& portMap = dpidIt->second;

        LOG_INFO_FMT("before update neighbor map, switch with dpid[%llx] have %lu neighbors",
            neigh.src_dpid, portMap.size());

        std::map<UINT4, neighbor_t>::iterator portIt = portMap.find(neigh.src_port);
        if (portIt != portMap.end())
        {
            neighbor_t& neighbor = portIt->second;
            memcpy(&neighbor, &neigh, sizeof(neighbor_t));
        }
        else
        {
            portMap.insert(std::pair<UINT4, neighbor_t>(neigh.src_port, neigh));
        }
        
        LOG_INFO_FMT("after update neighbor map, switch with dpid[%llx] have %lu neighbors",
            neigh.src_dpid, portMap.size());
    }

    m_neighMutexes[mapPos].unlock();

    LOG_INFO_FMT("<< after recoverLink with dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u], map[%u] has %lu neighbors ...", 
        neigh.src_dpid, neigh.src_port, neigh.dst_dpid, neigh.dst_port, mapPos, m_neighbors[mapPos].size());
    return BNC_OK;
}

CNeighbors* CTopoMgr::getLinks(UINT8 dpid)
{
    UINT4 mapPos = (dpid & 0xFFFF) % neighbor_map_granularity;

    CNeighborMap::iterator it = m_neighbors[mapPos].find(dpid);
    if (it != m_neighbors[mapPos].end())
        return &(it->second);

    return NULL;
}

neighbor_t* CTopoMgr::getLink(UINT8 dpid, UINT4 port)
{
    CNeighbors* links = getLinks(dpid);
    if (NULL != links)
    {
        CNeighbors::iterator it = links->find(port);
        if (it != links->end())
            return &(it->second);
    }

    return NULL;
}


INT4 CTopoMgr::addLink(UINT8 srcDpid, UINT4 srcPort, UINT8 dstDpid, UINT4 dstPort)
{
	CNeighbors* links = getLinks(srcDpid);
	if(NULL != links)
	{
		CNeighbors::iterator it = links->find(srcPort);
        if (it != links->end())
    	{
    		it->second.dst_dpid = dstDpid;
			it->second.dst_port = dstPort;
			return BNC_OK;
    	}
	}
	neighbor_t neighInsert = {NEIGH_STATE_ESTABLISHED, srcDpid, srcPort, dstDpid, dstPort, 0};
    CNeighbors portMapInsert;
    portMapInsert.insert(std::pair<UINT4, neighbor_t>(srcPort, neighInsert));
	
	UINT4 mapPos = (srcDpid & 0xFFFF) % neighbor_map_granularity;
	
	m_neighMutexes[mapPos].lock();

    CNeighborMap::iterator dpidIt = m_neighbors[mapPos].find(srcDpid);
    if (dpidIt == m_neighbors[mapPos].end())
    {
        m_neighbors[mapPos].insert(std::pair<UINT8, CNeighbors>(srcDpid, portMapInsert));

        LOG_INFO_FMT("after create neighbor map, switch with dpid[%llx] have one neighbor with dpid[%llx]port[%u]",
            srcDpid, dstDpid, dstPort);
		updateSwPortType(srcDpid, srcPort, dstDpid, dstPort);

        //m_pathChanged = true;
        CLinkEventReportor::getInstance()->report(EVENT_TYPE_TOPO_LINK_ESTABLISH, EVENT_REASON_TOPO_LINK_ESTABLISH, srcDpid, srcPort, dstDpid, dstPort);
        CClusterSync::syncTopoLink(neighInsert);    	
    }
    else
    {
        std::map<UINT4, neighbor_t>& portMap = dpidIt->second;

        LOG_INFO_FMT("before update neighbor map, switch with dpid[%llx] have %lu neighbors",
            srcDpid, portMap.size());

        std::map<UINT4, neighbor_t>::iterator portIt = portMap.find(srcPort);
        if (portIt == portMap.end())
        {
            portMap.insert(std::pair<UINT4, neighbor_t>(srcPort, neighInsert));
            updateSwPortType(srcDpid, srcPort, dstDpid, dstPort);

            //m_pathChanged = true;
            CLinkEventReportor::getInstance()->report(EVENT_TYPE_TOPO_LINK_ESTABLISH, EVENT_REASON_TOPO_LINK_ESTABLISH, srcDpid, srcPort, dstDpid, dstPort);
            CClusterSync::syncTopoLink(neighInsert);    	
        }
        
        LOG_INFO_FMT("after update neighbor map, switch with dpid[%llx] have %lu neighbors",
            srcDpid, portMap.size());
    }

    m_neighMutexes[mapPos].unlock();

    LOG_INFO_FMT("<< after addLink with dpid[%llx]port[%u]neighDpid[%llx]neighPort[%u], map[%u] has %lu neighbors ...", 
        srcDpid, srcPort, dstDpid, dstPort, mapPos, m_neighbors[mapPos].size());

	return BNC_OK;
}

void CTopoMgr::updatePath()
{
#if 1
	LOG_INFO("topo changed start.");

	m_pathMutex.lock();

	//清空旧数�?
	clearPaths();

	//生成所有路�?
    CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();

    STL_FOR_LOOP(map, swit)
    {
        CSmartPtr<CSwitch> sw = swit->second;
		
		if (sw.isNotNull() && 
            (SW_STATE_STABLE == sw->getState()) && 
            (OFP13_VERSION == sw->getVersion()) &&
            (0 != sw->getDpid()))
        {
            sw_path_list_t* pPathList = updateOneSwPath(sw->getDpid());
            if (NULL != pPathList)
            {
               m_pathLists.push_back(pPathList);
            }
        }
    }

	m_pathMutex.unlock();

	//printPaths();
	LOG_INFO("topo changed end.");
#else
    m_pce.generate();
#endif
}

void CTopoMgr::updateSwPortType(UINT8 srcDpid,UINT4 srcPort,UINT8 dstDpid,UINT4 dstPort)
{
	CSmartPtr<CSwitch> srcSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(srcDpid);
	CSmartPtr<CSwitch> dstSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dstDpid);
	if(srcSw.isNull()|| dstSw.isNull())
	{
		return;
	}
	srcSw->lockPorts();
	CPortMap& srcPorts = srcSw->getPorts();
	STL_FOR_LOOP(srcPorts, iter)
	{
		if(iter->second.port_no == srcPort)
		{
			iter->second.type = PORT_TYPE_SWITCH;
			break;
		}
	}
	srcSw->unlockPorts();

	dstSw->lockPorts();
	CPortMap& dstPorts = dstSw->getPorts();
	
	STL_FOR_LOOP(dstPorts, iter)
	{
		if(iter->second.port_no == dstPort)
		{
			iter->second.type = PORT_TYPE_SWITCH;
			break;
		}
	}
	dstSw->unlockPorts();
}
sw_path_list_t* CTopoMgr::updateOneSwPath(UINT8 dpid)
{
	//判断是否合法
	CSmartPtr<CSwitch> dst_sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
	if (dst_sw.isNull())
	{
		return NULL;
	}

	//添加自己到自己的路径
	sw_path_node_t* pSwPathNode = new sw_path_node_t();
	
	pSwPathNode->sw_dpid = dpid;
	pSwPathNode->port_no = 0;

	sw_path_t* pSwPath = new sw_path_t();

	pSwPath->src_dpid = dpid;
	pSwPath->dst_dpid = dpid;
	pSwPath->node_list.push_back(pSwPathNode);

	sw_path_list_t* pSwPathList =  new sw_path_list_t();
	
	pSwPathList->dst_dpid = dpid;
	
	
	std::map<UINT8, UINT8>   swUsedMap;

	swUsedMap.insert(std::make_pair(dpid,dpid));

	//建立路径�?
	//std::stack<sw_path_t*> pathStack;  //为复用路径，深度优先DFS
	std::queue<sw_path_t*> pathStack;  //为复用路径，深度优先DFS
	pathStack.push(pSwPath);

	std::list<sw_tagflow_t*> tagflowList;
	INT4 event = EVENT_TYPE_TAG_FLOW_ADD;
	INT4 reason = EVENT_REASON_TAG_FLOW_ADD;
	
	//添加相邻节点到该节点路径
	while (pathStack.size())
	{
		//弹出一条路径，获取源sw
		//sw_path_t* curPath = pathStack.top();
		//pathStack.pop();

		sw_path_t* curPath = pathStack.front();
		pathStack.pop();
	
		UINT8 & src_dpid = curPath->src_dpid;
		//寻找srcSw的所有相邻节�?
		addNeighborsPath(src_dpid, dst_sw, swUsedMap, pathStack, curPath, tagflowList);

		//路径加入list
		pSwPathList->path_list.push_back(curPath);
	}

	CTagFlowEventReportor::getInstance()->report(event, reason, tagflowList);
	
	return pSwPathList;
}

//void CTopoMgr::addNeighborsPath(UINT8 dpid,CSwitchMgr& swUsed,std::stack<sw_path_t*>& pathStack,sw_path_t* curPath)

void CTopoMgr::addNeighborsPath(UINT8 dpid, CSmartPtr<CSwitch>& dst_sw, std::map<UINT8, UINT8> & swUsed,std::queue<sw_path_t*>& pathStack,sw_path_t* curPath, std::list<sw_tagflow_t*>& tagflowList)
{
	UINT4 mapPos = (dpid & 0xFFFF) % neighbor_map_granularity;

	CNeighborMap::iterator iterNeighs = m_neighbors[mapPos].find(dpid);
	if (iterNeighs == m_neighbors[mapPos].end())
	{
		return;
	}
	CNeighbors& neighbors = iterNeighs->second;
	if (neighbors.empty())
	{
		return;
	}
	STL_FOR_LOOP(neighbors, iter)
	{
		neighbor_t& neigh = iter->second;
		if (NEIGH_STATE_ESTABLISHED != neigh.state) 
		{
			continue;
		}

		CSmartPtr<CSwitch> neighSw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(neigh.dst_dpid);
		if (neighSw.isNull()) 
		{
			continue;
		}

		if(swUsed.end() != swUsed.find(neigh.dst_dpid))
		{
			continue;
		}

		//新建路径并入�?
		sw_path_node_t* node = new sw_path_node_t(); 
		
		node->sw_dpid = neigh.dst_dpid;
		//node->port_no = neigh->dst_port_no;
		node->port_no = neigh.dst_port;
		sw_path_t* newPath = copyPath(curPath);
		newPath->node_list.push_front(node);
	
		newPath->src_dpid = neigh.dst_dpid;
		pathStack.push(newPath);
		
		CSmartPtr<CSwitch>  src_sw =  CControl::getInstance()->getSwitchMgr().getSwitchByDpid(newPath->src_dpid);


		sw_tagflow_t* tagflow_node = new sw_tagflow_t();
		tagflow_node->dst_sw = dst_sw;
		tagflow_node->src_sw = src_sw;
		tagflow_node->outport = node->port_no;

		tagflowList.push_back(tagflow_node);
		
		// 通知发生了变�?
		//CNotifyMgr::getInstance()->getNotifyPath()->notifyAddPath(src_sw, dst_sw, node->port_no);
		//标记为已使用
		//swUsed.addSwitch(neighSw);
		swUsed.insert(std::make_pair(neigh.dst_dpid, neigh.dst_dpid));
	}
}

sw_path_t* CTopoMgr::copyPath(const sw_path_t* path)
{
	sw_path_t* newPath = new sw_path_t();


	newPath->src_dpid = path->src_dpid;
	newPath->dst_dpid = path->dst_dpid;

	std::list<sw_path_node_t*>::const_iterator iterNode = path->node_list.begin();
	for (; iterNode != path->node_list.end(); ++iterNode)
	{
		sw_path_node_t* node = new sw_path_node_t();
	
		node->sw_dpid = (*iterNode)->sw_dpid;
		node->port_no = (*iterNode)->port_no;
		newPath->node_list.push_back(node);
	}

	return newPath;
}

void CTopoMgr::clearPaths()
{
	std::vector<sw_path_list_t*>::iterator iterPathList = m_pathLists.begin();
	for (; iterPathList != m_pathLists.end(); ++iterPathList)
	{
		sw_path_list_t* pPathList = *iterPathList;
		if (NULL == pPathList)
		{
			continue;
		}

		std::vector<sw_path_t*>::iterator iterPath = pPathList->path_list.begin();
		for (; iterPath != pPathList->path_list.end(); ++iterPath)
		{
			sw_path_t* pPath = *iterPath;
			if (NULL == pPath)
			{
				continue;
			}

			std::list<sw_path_node_t*>::iterator iterNode = pPath->node_list.begin();
			for (; iterNode != pPath->node_list.end(); ++iterNode)
			{
				if (NULL == *iterNode)
				{
					delete *iterNode;
				}
			}
			pPath->node_list.clear();
			delete pPath;
		}
		pPathList->path_list.clear();
		delete pPathList;
	}
	m_pathLists.clear();
}

void CTopoMgr::printPaths()
{
#if 0
	LOG_WARN_FMT("print path");

    INT1 srcMacStr[20] = {0}, dstMacStr[20] = {0};
    INT1 srcIpStr[20] = {0}, dstIpStr[20] = {0};

	m_pathMutex.lock();
	std::list<sw_path_list_t*>::iterator iterPathList = m_pathLists.begin();
	for (; iterPathList != m_pathLists.end(); ++iterPathList)
	{
		sw_path_list_t* pPathList = *iterPathList;
		if (NULL == pPathList)
		{
			continue;
		}
		LOG_WARN_FMT("---------------PathList: dst sw[0x%llx-%s-%s]---------------", 
    		pPathList->dst_sw->getDpid(),
		    mac2str((UINT1*)pPathList->dst_sw->getSwMac(), dstMacStr), 
		    number2ip(htonl(pPathList->dst_sw->getSwIp()), dstIpStr));

		std::list<sw_path_t*>::iterator iterPath = pPathList->path_list.begin();
		for (; iterPath != pPathList->path_list.end(); ++iterPath)
		{
			sw_path_t* pPath = *iterPath;
			if (NULL == pPath)
			{
				continue;
			}

            LOG_WARN_FMT(">>>>>>>>>>>>>>>>>>>Path: src sw[0x%llx-%s-%s]==>dst sw[0x%llx-%s-%s]", 
    		    pPath->src_sw->getDpid(),
    		    mac2str((UINT1*)pPath->src_sw->getSwMac(), srcMacStr), 
    		    number2ip(htonl(pPath->src_sw->getSwIp()), srcIpStr), 
    		    pPath->dst_sw->getDpid(),
    		    mac2str((UINT1*)pPath->dst_sw->getSwMac(), srcMacStr), 
    		    number2ip(htonl(pPath->dst_sw->getSwIp()), srcIpStr));

			std::list<sw_path_node_t*>::iterator iterNode = pPath->node_list.begin();
			for (; iterNode != pPath->node_list.end(); ++iterNode)
			{
				if (NULL != *iterNode)
				{
                    LOG_WARN_FMT(">>>>>>>>>>>>>>>>>>>>>>>PathNode: sw[0x%llx-%s-%s-%u]", 
                        (*iterNode)->sw->getDpid(),
                        mac2str((UINT1*)(*iterNode)->sw->getSwMac(), srcMacStr), 
                        number2ip(htonl((*iterNode)->sw->getSwIp()), srcIpStr),
                        (*iterNode)->port_no);
				}
			}
		}
	}
	m_pathMutex.unlock();
#endif
}

void CTopoMgr::pushLinkEventAndNotify(CSmartPtr<CMsgCommon> evt)
{
	if( evt.isNull())
	{
		return;
	}
	m_linkEventList.push_back(evt);
	m_linkSem.post();
}

void CTopoMgr::popLinkEventList()
{
	m_linkEventList.clear();
}

void CTopoMgr::notifyPathChanged()
{
    m_pathChanged = TRUE;
	m_linkSem.post();
}

//by:根据src_dpid与dst_dpid查找之间的路�?
sw_path_t* CTopoMgr::of131_fabric_get_path(UINT8 src_dpid,UINT8 dst_dpid)
{
	sw_path_t* ret(NULL);

	std::vector<sw_path_list_t*>::iterator iter = m_pathLists.begin();
	while(m_pathLists.end() != iter)
	{
		if(dst_dpid  == (*iter)->dst_dpid)
		{
			STL_FOR_LOOP((*iter)->path_list, path_iter)
			{
				if((*path_iter)&&(dst_dpid == (*path_iter)->dst_dpid)&&(src_dpid == (*path_iter)->src_dpid))
				{
					ret = *path_iter;
					break;
				}
			}
			if(NULL != ret)
			{
				break;
			}
		}
		iter++;
	}
	return ret;
}
sw_path_t* CTopoMgr::of131_fabric_get_path(CSmartPtr<CSwitch> srcSw, CSmartPtr<CSwitch> dstSw)
{
	sw_path_t* ret(NULL);
	if(srcSw.isNull()||dstSw.isNull())
	{
		return NULL;
	}
	std::vector<sw_path_list_t*>::iterator iter = m_pathLists.begin();
	while(m_pathLists.end() != iter)
	{
		if(dstSw->getDpid()  == (*iter)->dst_dpid)
		{
			STL_FOR_LOOP((*iter)->path_list, path_iter)
			{
				if((*path_iter)&&(dstSw->getDpid() == (*path_iter)->dst_dpid)&&(srcSw->getDpid() == (*path_iter)->src_dpid))
				{
					ret = *path_iter;
					break;
				}
			}
			if(NULL != ret)
			{
				break;
			}
		}
		iter++;
	}
	return ret;
}
	

UINT4 CTopoMgr::get_out_port_between_switch(UINT8 src_dpid, UINT8 dst_dpid)
{
#if 1
	INT1 src_str[48] = {0};
	INT1 dst_str[48] = {0}; 
		
	sw_path_t* path(NULL);
	sw_path_node_t* path_node(NULL);
	//by:yhy 根据src_dpid, dst_dpid查找路径
	path = of131_fabric_get_path(src_dpid, dst_dpid);
	//by:yhy dpid转字符串
	dpidUint8ToStr(src_dpid, src_str);
	dpidUint8ToStr(dst_dpid, dst_str);

	if ((0 == strlen(src_str)) || (0 == strlen(dst_str))) 
	{
		LOG_ERROR_FMT("Fail to get path. Src dpid or Dst dpid is emtpy src_str=%s dst_str=%s",src_str, dst_str);
		return 0;
	}

	if (NULL == path) 
	{
		LOG_ERROR_FMT("The path from %s to %s is not exist", src_str, dst_str);
		return 0;
	}

	path_node = path->node_list.front();
	if (NULL == path_node) 
	{
		LOG_ERROR_FMT("The path from %s to %s has no node", src_str, dst_str);
		return 0;
	}

	return path_node->port_no;
#else
    return m_pce.getOutPort(src_dpid, dst_dpid);
#endif
}
UINT4 CTopoMgr::get_out_port_between_switch(CSmartPtr<CSwitch> srcSw, CSmartPtr<CSwitch> dstSw)
{
	if(srcSw.isNull()||dstSw.isNull())
	{
		return 0;
	}
	return get_out_port_between_switch(srcSw->getDpid(), dstSw->getDpid());
}


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
#ifndef __CTOPOMGR_H
#define __CTOPOMGR_H

#include "CTimer.h"
#include "bnc-param.h"
#include "bnc-inet.h"
#include "CRefObj.h"
#include "CMutex.h"
#include "CEvent.h"
#include "CSemaphore.h"
#include "CEvent.h"
#include "CLinkEventNotifier.h"
#include "CPortEventNotifier.h"
#include "CPCE.h"

class CSwitch;
class CSwitchMgr;

typedef std::map<UINT4/*port*/, neighbor_t> CNeighbors;
typedef std::map<UINT8/*dpid*/, CNeighbors> CNeighborMap;

/*
 * 交换机拓扑管理类
 */
class CTopoMgr
{
public:
    CTopoMgr();
    ~CTopoMgr();

    /*
     * 初始化拓扑管理
     * ret: 成功 or 失败
     */
    INT4 init();

    /*
     * 将指定交换机加入拓扑中
     * param dpid:         交换机dpid
     * ret: 成功 or 失败
     */
    INT4 addSwitch(UINT8 dpid);

    /*
     * 将指定交换机从拓扑中删除
     * param dpid: 交换机dpid
     */
    void deleteSwitch(UINT8 dpid);

    /*
     * 更新拓扑边
     * param dpid:      交换机dpid
     * param port:      交换机端口
     * param neighDpid: 邻居交换机dpid
     * param neighPort: 邻居交换机端口
     * ret: 成功 or 失败
     */
    INT4 updateLink(UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort);

    /*
     * 删除指定交换机的指定端口与邻居节点的连接
     * param dpid: 交换机dpid
     * param port: 交换机端口
     */
    void deleteLink(UINT8 dpid, UINT4 port);

    /*
     * 检查指定交换机的指定端口的拓扑连接状况
     * param dpid: 交换机dpid
     * param port: 交换机端口
     * ret: UP or DOWN
     */
    BOOL isLinkUp(UINT8 dpid, UINT4 port);

    /*
     * 获取指定交换机的指定端口的拓扑连接保活性
     * param dpid: 交换机dpid
     * param port: 交换机端口
     * ret: TRUE or FALSE
     */
    BOOL isLinkAlive(UINT8 dpid, UINT4 port);

    /*
     * 指定交换机的指定端口的拓扑连接保活失败
     * param dpid: 交换机dpid
     * param port: 交换机端口
     */
    void unAliveLink(UINT8 dpid, UINT4 port);

    /*
     * 背板恢复指定拓扑连接
     * param neigh: 待恢复的拓扑连接
     */
    INT4 recoverLink(neighbor_t& neigh);

    /*
     * 获取所有交换机的邻居信息
     */
    std::vector<CNeighborMap>& getNeighMapVector() { return m_neighbors; }

	 /*
     * 获取交换机的邻居信息
     */
    CNeighbors* getLinks(UINT8 dpid);

     /*
      * 获取指定交换机的指定端口的拓扑连接信息
      */
     neighbor_t* getLink(UINT8 dpid, UINT4 port);

	 /*
     * 增加交换机的邻居信息
     */
	INT4 addLink(UINT8 srcDpid, UINT4 srcPort, UINT8 dstDpid, UINT4 dstPort);

    /*
     * 更新Path
     */
    void updatePath();

	void updateSwPortType(UINT8 srcDpid,UINT4 srcPort,UINT8 dstDpid,UINT4 dstPort);
	
	sw_path_list_t* updateOneSwPath(UINT8 dpid);
	void addNeighborsPath(UINT8 dpid, CSmartPtr<CSwitch>& dst_sw, std::map<UINT8, UINT8> & swUsed,std::queue<sw_path_t*>& pathStack,sw_path_t* curPath, std::list<sw_tagflow_t*>& tagflowList);
	sw_path_t* copyPath(const sw_path_t* path);
	void clearPaths();
	void printPaths();

	CSemaphore&  getLinkEventSem(){return m_linkSem; }
	std::list<CSmartPtr<CMsgCommon> >& getLinkEventList(){return m_linkEventList;}

	void popLinkEventList();
	void pushLinkEventAndNotify(CSmartPtr<CMsgCommon> evt);

	void notifyPathChanged();
    BOOL checkPathChanged() {return m_pathChanged;}
	void updatePathChanged(BOOL changed) {m_pathChanged = changed;}
	
	sw_path_t* of131_fabric_get_path(CSmartPtr<CSwitch> srcSw, CSmartPtr<CSwitch> dstSw);
	sw_path_t* of131_fabric_get_path(UINT8 src_dpid,UINT8 dst_dpid);
	UINT4 get_out_port_between_switch(UINT8 src_dpid, UINT8 dst_dpid);
	UINT4 get_out_port_between_switch(CSmartPtr<CSwitch> srcSw, CSmartPtr<CSwitch> dstSw);

private:
    static const UINT4 neighbor_map_granularity = 8;

    std::vector<CNeighborMap> m_neighbors;
    std::vector<CMutex> m_neighMutexes;

	std::vector<sw_path_list_t*> m_pathLists;
    CMutex m_pathMutex;
    CTimer m_timer;     //path更新timer
    BOOL m_pathChanged; //是否需要更新path

    CThread 			  m_thread;
	CSemaphore 			  m_linkSem;
	std::list<CSmartPtr<CMsgCommon> >   m_linkEventList;

	CLinkEventNotifier* m_linkeventNotify;
	CPortEventNotifier* m_porteventNotify;

    CPCE m_pce;
};

#endif

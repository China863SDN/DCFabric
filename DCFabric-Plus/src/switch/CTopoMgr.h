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
 * ���������˹�����
 */
class CTopoMgr
{
public:
    CTopoMgr();
    ~CTopoMgr();

    /*
     * ��ʼ�����˹���
     * ret: �ɹ� or ʧ��
     */
    INT4 init();

    /*
     * ��ָ������������������
     * param dpid:         ������dpid
     * ret: �ɹ� or ʧ��
     */
    INT4 addSwitch(UINT8 dpid);

    /*
     * ��ָ����������������ɾ��
     * param dpid: ������dpid
     */
    void deleteSwitch(UINT8 dpid);

    /*
     * �������˱�
     * param dpid:      ������dpid
     * param port:      �������˿�
     * param neighDpid: �ھӽ�����dpid
     * param neighPort: �ھӽ������˿�
     * ret: �ɹ� or ʧ��
     */
    INT4 updateLink(UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort);

    /*
     * ɾ��ָ����������ָ���˿����ھӽڵ������
     * param dpid: ������dpid
     * param port: �������˿�
     */
    void deleteLink(UINT8 dpid, UINT4 port);

    /*
     * ���ָ����������ָ���˿ڵ���������״��
     * param dpid: ������dpid
     * param port: �������˿�
     * ret: UP or DOWN
     */
    BOOL isLinkUp(UINT8 dpid, UINT4 port);

    /*
     * ��ȡָ����������ָ���˿ڵ��������ӱ�����
     * param dpid: ������dpid
     * param port: �������˿�
     * ret: TRUE or FALSE
     */
    BOOL isLinkAlive(UINT8 dpid, UINT4 port);

    /*
     * ָ����������ָ���˿ڵ��������ӱ���ʧ��
     * param dpid: ������dpid
     * param port: �������˿�
     */
    void unAliveLink(UINT8 dpid, UINT4 port);

    /*
     * ����ָ�ָ����������
     * param neigh: ���ָ�����������
     */
    INT4 recoverLink(neighbor_t& neigh);

    /*
     * ��ȡ���н��������ھ���Ϣ
     */
    std::vector<CNeighborMap>& getNeighMapVector() { return m_neighbors; }

	 /*
     * ��ȡ���������ھ���Ϣ
     */
    CNeighbors* getLinks(UINT8 dpid);

     /*
      * ��ȡָ����������ָ���˿ڵ�����������Ϣ
      */
     neighbor_t* getLink(UINT8 dpid, UINT4 port);

	 /*
     * ���ӽ��������ھ���Ϣ
     */
	INT4 addLink(UINT8 srcDpid, UINT4 srcPort, UINT8 dstDpid, UINT4 dstPort);

    /*
     * ����Path
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
    CTimer m_timer;     //path����timer
    BOOL m_pathChanged; //�Ƿ���Ҫ����path

    CThread 			  m_thread;
	CSemaphore 			  m_linkSem;
	std::list<CSmartPtr<CMsgCommon> >   m_linkEventList;

	CLinkEventNotifier* m_linkeventNotify;
	CPortEventNotifier* m_porteventNotify;

    CPCE m_pce;
};

#endif

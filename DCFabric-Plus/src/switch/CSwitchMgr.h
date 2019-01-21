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
*   File Name   : CSwitchMgr.h                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CSWITCHMGR_H
#define __CSWITCHMGR_H

#include "CSwitch.h"
#include "CHashMap.h"
//#include "CRWLock.h"
//#include "CMutex.h"
#include "CMemPool.h"
#include "CLFHashMap.h"

//typedef std::map<INT4/*sockfd*/, CSmartPtr<CSwitch> > CSwitchMap;
typedef CUint32LFHashMap<CSmartPtr<CSwitch> > CSwitchHMap;
//typedef std::map<std::string/*mac*/, INT4/*sockfd*/> CMacSockfdMap;
//typedef CStringHashMap<INT4/*sockfd*/> CMacSockfdHMap;
typedef CStringLFHashMap<CSmartPtr<CSwitch> > CMacHMap;
//typedef std::map<UINT4/*ip*/, INT4/*sockfd*/> CIpSockfdMap;
//typedef CUint32HashMap<INT4/*sockfd*/> CIpSockfdHMap;
typedef CUint32LFHashMap<CSmartPtr<CSwitch> > CIpHMap;
//typedef std::map<UINT8/*dpid*/, INT4/*sockfd*/> CDpidSockfdMap;
//typedef CUint64HashMap<INT4/*sockfd*/> CDpidSockfdHMap;
typedef CUint64LFHashMap<CSmartPtr<CSwitch> > CDpidHMap;
typedef CUint32LFHashMap<CSmartPtr<CSwitch> > CTagHMap;

/*
 * 交换机管理类
 */
class CSwitchMgr
{
public:
    static const UINT4 hash_bucket_number = 10240;

public:
    CSwitchMgr();
    ~CSwitchMgr();

    INT4 init();

    INT4 addSwitch(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port);
    INT4 addSwitch(CSmartPtr<CSwitch> sw);
    void delSwitch(INT4 sockfd);
	void delSwitchByDpid(UINT8 dpid);
    void updSwitch(CSmartPtr<CSwitch> swOld, INT4 sockfd, CSmartPtr<CSwitch> swNew);
    CSmartPtr<CSwitch> getSwitch(INT4 sockfd);
    CSmartPtr<CSwitch> getSwitchByMac(const INT1* mac);
    CSmartPtr<CSwitch> getSwitchByIp(UINT4 ip);
    CSmartPtr<CSwitch> getSwitchByDpid(UINT8 dpid);
    CSmartPtr<CSwitch> getSwitchByTag(UINT4 tag);

    CSwitchHMap& getSwitchMap() {return m_sws;}

    void addDpidMap(UINT8 dpid, CSmartPtr<CSwitch>& sw);

    CSwitch* allocSwitch();
    void releaseSwitch(CSwitch* sw);

    void lock();
    void unlock();

private:
    void addSwitchMap(INT4 sockfd, CSmartPtr<CSwitch>& sw);
    void addMacMap(const INT1* mac, CSmartPtr<CSwitch>& sw);
    void addIpMap(UINT4 ip, CSmartPtr<CSwitch>& sw);
    void addTagMap(UINT4 tag, CSmartPtr<CSwitch>& sw);
    void delSwitchMap(INT4 sockfd);
    void delMacMap(const INT1* mac);
    void delIpMap(UINT4 ip);
    void delDpidMap(UINT8 dpid);
    void delTagMap(UINT4 tag);

private:
    static const UINT4 switch_mem_node_size = 10240;

    CSwitchHMap     m_sws;
    CMacHMap        m_macMap;
    CIpHMap         m_ipMap;
    CDpidHMap       m_dpidMap;
    CTagHMap        m_tagMap;
    CRWLock         m_rwlock;
    //CMutex          m_mutex;
    CMemEntry       m_swPool;
};

#endif

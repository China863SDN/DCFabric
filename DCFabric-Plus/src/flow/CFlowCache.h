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
*   File Name   : CFlowCache.h                                                *
*   Author      : jiang bo                                                    *
*   Create Date : 2017-12-21                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CFLOWCACHE_H
#define _CFLOWCACHE_H

#include "CFlow.h"
#include "CEventNotifier.h"
#include "CLFHashMap.h"

typedef std::list<CFlow> CFlowList;
typedef std::map<UINT1 /*table id*/, CFlowList> CTableIdFlowsMap;
typedef CUint64LFHashMap<CTableIdFlowsMap> CSwitchFlowsMap;

class CFlowCache : public CEventNotifier
{
public:
    CFlowCache();
    ~CFlowCache();
    void clear();

    INT4 onregister();
    void deregister();

    INT4 addFlow(CFlow& flow);
    INT4 modFlow(CFlow& flow);
    INT4 modFlowStrict(CFlow& flow);
    void delFlow(CFlow& flow);
    void delFlowStrict(CFlow& flow);

    CFlow* findFlow(UINT8 dpid, const INT1* uuid);
    CTableIdFlowsMap* getTableIdFlowsMap(UINT8 dpid);
    CFlowList* getFlowList(UINT8 dpid, UINT1 tableId);
    CSwitchFlowsMap& getFlowsMap() {return m_map;}

    INT4 recoverFlow(CFlow& flow);

    const char* toString() {return "CFlowCache";}

private:
    INT4 consume(CSmartPtr<CMsgCommon> evt);

private:
    static const UINT4 hash_bucket_number = 10240;

    CSwitchFlowsMap m_map;
};

#endif

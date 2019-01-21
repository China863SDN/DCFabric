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
*   File Name   : CFlowCache.cpp                                              *
*   Author      : jiang bo                                                    *
*   Create Date : 2017-12-21                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "bnc-type.h"
#include "comm-util.h"
#include "log.h"
#include "CFlowCache.h"
#include "CFlowDefine.h"
#include "CFlowTableEvent.h"
#include "CEventReportor.h"
#include "CClusterSync.h"
#include "uuid.h"

CFlowCache::CFlowCache():m_map(hash_bucket_number)
{
}

CFlowCache::~CFlowCache() 
{
    deregister();
}

void CFlowCache::clear()
{
    m_map.clear();
}

INT4 CFlowCache::onregister()
{
    CMsgPath path(g_flowTableEventPath[0]);
    return CEventNotifier::onregister(path);
}

void CFlowCache::deregister()
{
    CMsgPath path(g_flowTableEventPath[0]);
    CEventNotifier::deregister(path);
}

INT4 CFlowCache::consume(CSmartPtr<CMsgCommon> evt)
{
    INT4 ret = BNC_ERR;
    
    if (evt.isNull())
        return ret;

    CFlowTableEvent* pEvt = (CFlowTableEvent*)evt.getPtr();
    //LOG_WARN(pEvt->getDesc().c_str());

    switch (pEvt->getEvent())
    {
        case EVENT_TYPE_FLOW_TABLE_ADD:
            //LOG_WARN("EVENT_TYPE_FLOW_TABLE_ADD");
            ret = addFlow(pEvt->getFlow());
            break;
        case EVENT_TYPE_FLOW_TABLE_MOD:
            //LOG_WARN("EVENT_TYPE_FLOW_TABLE_MOD");
            ret = modFlow(pEvt->getFlow());
            break;
        case EVENT_TYPE_FLOW_TABLE_MOD_STRICT:
            //LOG_WARN("EVENT_TYPE_FLOW_TABLE_MOD_STRICT");
            ret = modFlowStrict(pEvt->getFlow());
            break;
        case EVENT_TYPE_FLOW_TABLE_DEL:
            //LOG_WARN("EVENT_TYPE_FLOW_TABLE_DEL");
            delFlow(pEvt->getFlow());
            ret = BNC_OK;
            break;
        case EVENT_TYPE_FLOW_TABLE_DEL_STRICT:
            //LOG_WARN("EVENT_TYPE_FLOW_TABLE_DEL_STRICT");
            delFlowStrict(pEvt->getFlow());
            ret = BNC_OK;
            break;
        case EVENT_TYPE_FLOW_TABLE_RECOVER:
            //LOG_WARN("EVENT_TYPE_FLOW_TABLE_RECOVER");
            ret = recoverFlow(pEvt->getFlow());
            break;
        default:
            LOG_WARN_FMT("%s consume invalid event[%d] !", toString(), pEvt->getEvent());
            break;
    }

    return ret;
}

INT4 CFlowCache::addFlow(CFlow& flow)
{
    UINT8 dpid = flow.getSw()->getDpid();
    CSwitchFlowsMap::CPair* item = NULL;
    if (!m_map.find(dpid, &item))
    {
        if (!m_map.insert(dpid, CTableIdFlowsMap(), &item))
        {
            LOG_WARN_FMT("CSwitchFlowsMap insert CTableIdFlowsMap for sw[0x%llx] failed !", dpid);
            return BNC_ERR;
        }
    }

    CTableIdFlowsMap& tableIdFlowsMap = item->second;
    CTableIdFlowsMap::iterator itTableId = tableIdFlowsMap.find(flow.getTableId());
    if (itTableId == tableIdFlowsMap.end())
    {
        std::pair<CTableIdFlowsMap::iterator, bool> result =
            tableIdFlowsMap.insert(CTableIdFlowsMap::value_type(flow.getTableId(), CFlowList()));
        if (!result.second)
        {
            LOG_WARN_FMT("CTableIdFlowsMap insert CFlowList for sw[0x%llx]tableId[%u] failed !", 
                dpid, flow.getTableId());
            return BNC_ERR;
        }
        itTableId = result.first;
    }

    CFlowList& flowList = itTableId->second;
    STL_FOR_LOOP(flowList, itFlow)
    {
        if ((*itFlow).matchStrict(flow))
        {
            flow.setUuid((*itFlow).getUuid());
            flow.setCreateTime((*itFlow).getCreateTime());
            *itFlow = flow;
            return BNC_OK;
        }
    }

    uuid_t gen = {0};
    uuid_generate_random(gen);
    char uuid[UUID_LEN] = {0};
    uuid_unparse(gen, uuid);
    std::string uuidStr(uuid);
    flow.setUuid(uuidStr);
    flow.setCreateTime(time(NULL));

    flowList.push_back(flow);

    CClusterSync::syncAddFlowEntry(dpid, uuid);

    return BNC_OK;
}

INT4 CFlowCache::modFlow(CFlow& flow)
{
    CFlowList* flowList = getFlowList(flow.getSw()->getDpid(), flow.getTableId());
    if (NULL == flowList)
        return BNC_ERR;

    STL_FOR_LOOP(*flowList, itFlow)
    {
        if ((*itFlow).match(flow))
        {
            (*itFlow).getInstructions() = flow.getInstructions();
        }
    }

    return BNC_OK;
}

INT4 CFlowCache::modFlowStrict(CFlow& flow)
{
    CFlowList* flowList = getFlowList(flow.getSw()->getDpid(), flow.getTableId());
    if (NULL == flowList)
        return BNC_ERR;

    STL_FOR_LOOP(*flowList, itFlow)
    {
        if ((*itFlow).matchStrict(flow))
        {
            flow.setUuid((*itFlow).getUuid());
            flow.setCreateTime((*itFlow).getCreateTime());

            *itFlow = flow;
            return BNC_OK;
        }
    }

    return BNC_ERR;
}

void CFlowCache::delFlow(CFlow& flow)
{
    CFlowList* flowList = getFlowList(flow.getSw()->getDpid(), flow.getTableId());
    if (NULL == flowList)
        return;

    CFlowList::iterator itFlow = flowList->begin();
    while (itFlow != flowList->end())
    {
        if ((*itFlow).match(flow))
        {
            CFlowList::iterator itRm = itFlow++;

            CClusterSync::syncDelFlowEntry(flow.getSw()->getDpid(), (*itRm).getUuid().c_str());

            uuid_t uuid = {0};
            uuid_parse((*itRm).getUuid().c_str(), uuid);
            uuid_clear(uuid);

            flowList->erase(itRm);
        }
        else
        {
            ++ itFlow;
        }
    }
}

void CFlowCache::delFlowStrict(CFlow& flow)
{
    if (flow.getUuid().empty())
    {
        CFlowList* flowList = getFlowList(flow.getSw()->getDpid(), flow.getTableId());
        if (NULL == flowList)
            return;

        STL_FOR_LOOP(*flowList, itFlow)
        {
            if ((*itFlow).matchStrict(flow))
            {
                CClusterSync::syncDelFlowEntry(flow.getSw()->getDpid(), (*itFlow).getUuid().c_str());
                
                uuid_t uuid;
                uuid_parse((*itFlow).getUuid().c_str(), uuid);
                uuid_clear(uuid);

                flowList->erase(itFlow);
                return;
            }
        }
    }
    else
    {
        CTableIdFlowsMap* tableIdMap = getTableIdFlowsMap(flow.getSw()->getDpid());
        if (NULL != tableIdMap)
        {
            STL_FOR_LOOP(*tableIdMap, itTableId)
            {
                CFlowList& flowList = itTableId->second;
                STL_FOR_LOOP(flowList, itFlow)
                {
                    if ((*itFlow).getUuid().compare(flow.getUuid()) == 0)
                    {
                        CClusterSync::syncDelFlowEntry(flow.getSw()->getDpid(), flow.getUuid().c_str());
                        
                        flowList.erase(itFlow);
                        return;
                    }
                }
            }
        }
    }
}

CFlow* CFlowCache::findFlow(UINT8 dpid, const INT1* uuid)
{
    CTableIdFlowsMap* tableIdMap = getTableIdFlowsMap(dpid);
    if (NULL != tableIdMap)
    {
        STL_FOR_LOOP(*tableIdMap, itTableId)
        {
            CFlowList& flowList = itTableId->second;
            STL_FOR_LOOP(flowList, itFlow)
            {
                CFlow& flow = *itFlow;
                if (flow.getUuid().compare(uuid) == 0)
                    return &flow;
            }
        }
    }

    return NULL;
}

CTableIdFlowsMap* CFlowCache::getTableIdFlowsMap(UINT8 dpid)
{
    CTableIdFlowsMap* ret = NULL;

    CSwitchFlowsMap::CPair* item = NULL;
    if (m_map.find(dpid, &item))
        ret = &(item->second);

    return ret;
}

CFlowList* CFlowCache::getFlowList(UINT8 dpid, UINT1 tableId)
{
    CTableIdFlowsMap* tableIdFlowsMap = getTableIdFlowsMap(dpid);
    if (NULL == tableIdFlowsMap)
        return NULL;

    CTableIdFlowsMap::iterator itTableId = tableIdFlowsMap->find(tableId);
    if (itTableId == tableIdFlowsMap->end())
    {
        //LOG_WARN_FMT("can not find CFlowList for sw[0x%llx]tableId[%u] !", dpid, tableId);
        return NULL;
    }

    return &(itTableId->second);
}

INT4 CFlowCache::recoverFlow(CFlow& flow)
{
    if (flow.getUuid().empty())
    {
        LOG_WARN("recovered flow table has empty uuid !");
        return BNC_ERR;
    }

    UINT8 dpid = flow.getSw()->getDpid();
    CSwitchFlowsMap::CPair* item = NULL;
    if (!m_map.find(dpid, &item))
    {
        if (!m_map.insert(dpid, CTableIdFlowsMap(), &item))
        {
            LOG_WARN_FMT("CSwitchFlowsMap insert CTableIdFlowsMap for sw[0x%llx] failed !", dpid);
            return BNC_ERR;
        }
    }

    CTableIdFlowsMap& tableIdFlowsMap = item->second;
    CTableIdFlowsMap::iterator itTableId = tableIdFlowsMap.find(flow.getTableId());
    if (itTableId == tableIdFlowsMap.end())
    {
        std::pair<CTableIdFlowsMap::iterator, bool> result =
            tableIdFlowsMap.insert(CTableIdFlowsMap::value_type(flow.getTableId(), CFlowList()));
        if (!result.second)
        {
            LOG_WARN_FMT("CTableIdFlowsMap insert CFlowList for sw[0x%llx]tableId[%u] failed !", 
                dpid, flow.getTableId());
            return BNC_ERR;
        }
        itTableId = result.first;
    }

    CFlowList& flowList = itTableId->second;
    STL_FOR_LOOP(flowList, itFlow)
    {
        if ((*itFlow).getUuid().compare(flow.getUuid()) == 0)
        {
            *itFlow = flow;
            return BNC_OK;
        }
    }

    flowList.push_back(flow);

    return BNC_OK;
}


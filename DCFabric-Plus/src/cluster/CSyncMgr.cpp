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
*   File Name   : CSyncMgr.cpp                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-param.h"
#include "bnc-error.h"
#include "log.h"
#include "openflow-common.h"
#include "CSyncMgr.h"
#include "CSyncDefine.h"
#include "CControl.h"
#include "CConf.h"
#include "comm-util.h"
#include "CClusterMgr.h"

//默认主备同步周期，单位秒
static const UINT4 MASTER_SLAVE_SYNC_INTERVAL = 30;

const INT1* nodeTypeString[NODE_TYPE_MAX] =
{
    "none",
    "fabric_host_node",
    "fabric_sw_node",
    "nat_icmp",
    "nat_host",
    "openstack_external_node",
    "openstack_lbaas_members",
    "topo_link",
    "all_topo_link",
    "sw_tag",
    "all_sw_tag",
    "tag_info"
};

#if 0
CSyncMemory::CSyncMemory()
{
}

CSyncMemory::~CSyncMemory()
{
    STL_FOR_LOOP(m_freeEntries, it)
    {
        sync_entry_t* entry = *it;
        if (NULL != entry)
        {
            if (NULL != entry->data)
                free(entry->data);
            free(entry);
        }
    }
    STL_FOR_LOOP(m_usedEntries, it)
    {
        sync_entry_t* entry = *it;
        if (NULL != entry)
        {
            if (NULL != entry->data)
                free(entry->data);
            free(entry);
        }
    }
}

INT4 CSyncMemory::init()
{
    UINT4 i = 0;
    for (i = 0; i < init_sync_entry_count; ++i)
    {
        sync_entry_t* entry = (sync_entry_t*)malloc(sizeof(sync_entry_t));
        if (NULL == entry)
        {
            LOG_ERROR_FMT("alloc %lu bytes sync_entry_t failed !", sizeof(sync_entry_t));
            break;
        }

        entry->size = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header) + sizeof(field_pad_t) * MAX_FILED_NUM;
        entry->data = (INT1*)malloc(entry->size);
        if (NULL == entry->data)
        {
            LOG_ERROR_FMT("alloc %u bytes data failed !", entry->size);
            free(entry);
            break;
        }

        m_freeEntries.push_back(entry);
    }

    return (i > 0) ? BNC_OK : BNC_ERR;
}

sync_entry_t* CSyncMemory::allocEntry()
{
    sync_entry_t* entry = NULL;

    m_mutex.lock();
    
    if (!m_freeEntries.empty())
    {
        entry = m_freeEntries.front();
        m_freeEntries.pop_front();
        m_usedEntries.push_back(entry);

        m_mutex.unlock();
    }
    else
    {
        m_mutex.unlock();

        sync_entry_t* ent = (sync_entry_t*)malloc(sizeof(sync_entry_t));
        if (NULL == ent)
        {
            LOG_ERROR_FMT("alloc %lu bytes sync_entry_t failed !", sizeof(sync_entry_t));
            return entry;
        }

        ent->size = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header) + sizeof(field_pad_t) * MAX_FILED_NUM;
        ent->data = (INT1*)malloc(ent->size);
        if (NULL == ent->data)
        {
            LOG_ERROR_FMT("alloc %u bytes data failed !", ent->size);
            free(ent);
            return entry;
        }

        entry = ent;

        m_mutex.lock();
        m_usedEntries.push_back(entry);
        m_mutex.unlock();
    }

    return entry;
}

void CSyncMemory::releaseEntry(sync_entry_t* entry)
{
    if (NULL != entry)
    {
        m_mutex.lock();

        m_usedEntries.remove(entry);
        m_freeEntries.push_back(entry);

        m_mutex.unlock();
    }
}
#endif

void CSyncMgr::syncData(void* param)
{
    CSyncMgr* syncMgr = (CSyncMgr*)param;
    if (NULL == syncMgr)
        return;

	LOG_INFO(">> CSyncMgr: START");

	time_t tv1 = time(NULL);

    if (OFPCR_ROLE_MASTER == CClusterMgr::getInstance()->getControllerRole())
    {
        LOG_INFO("******CSyncMgr: master persist data******");
        syncMgr->persistTotal();
    }

	time_t tv2 = time(NULL);

	LOG_INFO_FMT("<< CSyncMgr: STOP tv2-tv1=%ld", tv2-tv1);
}

CSyncMgr* CSyncMgr::m_instance = NULL;

CSyncMgr* CSyncMgr::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CSyncMgr();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return m_instance;
}

CSyncMgr::CSyncMgr():m_transSeq(0),m_syncPool(TRUE)
{
}

CSyncMgr::~CSyncMgr()
{
}

INT4 CSyncMgr::init()
{
    const INT1* pConf = CConf::getInstance()->getConfig("cluster_conf", "cluster_on");
    BOOL clusterOn = (NULL != pConf) ? (atoi(pConf) > 0) : FALSE;
	LOG_INFO_FMT("cluster_on: %s", clusterOn ? "true" : "false");

    if (!clusterOn)
        return BNC_OK;

#if 0
    if (m_syncMem.init() != BNC_OK)
#else
    UINT4 size = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header) + sizeof(field_pad_t) * MAX_FILED_NUM;
    if (m_syncPool.init(size, sync_mem_node_size, "CSyncMgr") != BNC_OK)
#endif
    {
        LOG_WARN("Init m_syncPool failed !");
        return BNC_ERR;
    }

    if (m_dbClient.init() != BNC_OK)
    {
        LOG_WARN("Init CDbClientProxy failed !");
        return BNC_ERR;
    }

    if (OFPCR_ROLE_MASTER == CClusterMgr::getInstance()->getControllerRole())
        persistTransSeq(m_transSeq);

    pConf = CConf::getInstance()->getConfig("cluster_conf", "sync_interval");
    INT4 interval = (NULL != pConf) ? atoi(pConf) : MASTER_SLAVE_SYNC_INTERVAL;

	LOG_INFO_FMT("sync_interval: %d", interval);

    if (0 != interval)
    {
        if (m_timer.schedule(interval, interval, syncData, this) != BNC_OK)
            LOG_WARN("Schedule timer for syncData failed !");
    }

    return BNC_OK;
}

INT4 CSyncMgr::persistTotal()
{
    persistAllTopoLink();
    persistAllSwitchTag();
    persistTagInfo();
    //TO BE ADDED

    return BNC_OK;
}

INT4 CSyncMgr::persistTransSeq(UINT8 transSeq)
{
    if (!persistNeeded())
        return BNC_ERR;

    LOG_INFO_FMT("persist TRANS_SEQ %llu", transSeq);

	INT1 val[TABLE_STRING_LEN] = {0};
    sprintf(val, "%llu", transSeq);
    return m_dbClient.persistValue(TRANS_SEQ, val);
}

INT4 CSyncMgr::persistTopoLink(const neighbor_t& neigh)
{
    if (!persistNeeded())
        return BNC_ERR;

#if 0
    sync_entry_t* entry = m_syncMem.allocEntry();
    if (NULL == entry)
    {
        LOG_ERROR("CSyncMemory alloc entry failed !");
        return BNC_ERR;
    }
#else
    p_mem_node_t node = m_syncPool.alloc();
    if (NULL == node)
    {
        LOG_ERROR("m_syncPool alloc node failed !");
        return BNC_ERR;
    }
#endif

	stCommMsgNode_header* pstCommHeader = (stCommMsgNode_header *)node->data;
	pstCommHeader->msgLen = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header);
	pstCommHeader->msgNodeType = NODE_TYPE_TOPO_LINK;

	stSynMsgNode_header* pstSynNodeHeader = (stSynMsgNode_header *)(node->data + sizeof(stCommMsgNode_header));
	pstSynNodeHeader->synMsgType = NODE_TYPE_TOPO_LINK;
	pstSynNodeHeader->operMsgType = OPERATE_MOD;
	pstSynNodeHeader->synMsgLen = 0;

    tlv_t* pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = TOPO_LINKS_STATE;
    pTlv->len = sizeof(INT4);
    *(INT4*)pTlv->data = neigh.state;
    pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
    
    pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = TOPO_LINKS_SRC_DPID;
    pTlv->len = sizeof(UINT8);
    *(UINT8*)pTlv->data = neigh.src_dpid;
    pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
    
    pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = TOPO_LINKS_SRC_PORT;
    pTlv->len = sizeof(UINT4);
    *(UINT4*)pTlv->data = neigh.src_port;
    pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
    
    pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = TOPO_LINKS_DST_DPID;
    pTlv->len = sizeof(UINT8);
    *(UINT8*)pTlv->data = neigh.dst_dpid;
    pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
    
    pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = TOPO_LINKS_DST_PORT;
    pTlv->len = sizeof(UINT4);
    *(UINT4*)pTlv->data = neigh.dst_port;
    pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
    
    pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = TOPO_LINKS_WEIGHT;
    pTlv->len = sizeof(UINT8);
    *(UINT8*)pTlv->data = neigh.weight;
    pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;

	pstSynNodeHeader->synMsgLen = pstCommHeader->msgLen - sizeof(stCommMsgNode_header) - sizeof(stSynMsgNode_header);

	persistData(pstCommHeader);

#if 0
    m_syncMem.releaseEntry(entry);
#else
    m_syncPool.release(node);
#endif

    return BNC_OK;
}

INT4 CSyncMgr::persistAllTopoLink()
{
    if (!persistNeeded())
        return BNC_ERR;

#if 0
    sync_entry_t* entry = m_syncMem.allocEntry();
    if (NULL == entry)
    {
        LOG_ERROR("CSyncMemory alloc entry failed !");
        return BNC_ERR;
    }
#else
    p_mem_node_t node = m_syncPool.alloc();
    if (NULL == node)
    {
        LOG_ERROR("m_syncPool alloc node failed !");
        return BNC_ERR;
    }
#endif

	stCommMsgNode_header* pstCommHeader = (stCommMsgNode_header *)node->data;
	pstCommHeader->msgLen = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header);
	pstCommHeader->msgNodeType = NODE_TYPE_ALL_TOPO_LINK;

	stSynMsgNode_header* pstSynNodeHeader = (stSynMsgNode_header *)(node->data + sizeof(stCommMsgNode_header));
	pstSynNodeHeader->synMsgType = NODE_TYPE_ALL_TOPO_LINK;
	pstSynNodeHeader->operMsgType = OPERATE_MOD;
	pstSynNodeHeader->synMsgLen = 0;

    BOOL stopped = FALSE;
    tlv_t* pTlv = NULL;

    std::vector<CNeighborMap>& neighMapVec = CControl::getInstance()->getTopoMgr().getNeighMapVector();
    STL_FOR_LOOP(neighMapVec, neighMapIt)
    {
        STL_FOR_LOOP(*neighMapIt, dpidIt)
        {
            CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpidIt->first);
            if (sw.isNull() || (sw->getState() >= SW_STATE_POWER_OFF))
                continue;

            CNeighbors& portMap = dpidIt->second;
            STL_FOR_LOOP(portMap, portIt)
            {
                neighbor_t& neigh = portIt->second;
                if (NEIGH_STATE_DELETED == neigh.state)
                    continue;

                if (node->size < (pstCommHeader->msgLen + sizeof(tlv_t) + sizeof(INT4)))
                {
                    stopped = TRUE;
                    break;
                }

                pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
                pTlv->type = TOPO_LINKS_STATE;
                pTlv->len = sizeof(INT4);
                *(INT4*)pTlv->data = neigh.state;
                pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;

                if (node->size < (pstCommHeader->msgLen + sizeof(tlv_t) + sizeof(UINT8)))
                {
                    stopped = TRUE;
                    break;
                }

                pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
                pTlv->type = TOPO_LINKS_SRC_DPID;
                pTlv->len = sizeof(UINT8);
                *(UINT8*)pTlv->data = neigh.src_dpid;
                pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
                
                if (node->size < (pstCommHeader->msgLen + sizeof(tlv_t) + sizeof(UINT4)))
                {
                    stopped = TRUE;
                    break;
                }

                pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
                pTlv->type = TOPO_LINKS_SRC_PORT;
                pTlv->len = sizeof(UINT4);
                *(UINT4*)pTlv->data = neigh.src_port;
                pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
                
                if (node->size < (pstCommHeader->msgLen + sizeof(tlv_t) + sizeof(UINT8)))
                {
                    stopped = TRUE;
                    break;
                }

                pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
                pTlv->type = TOPO_LINKS_DST_DPID;
                pTlv->len = sizeof(UINT8);
                *(UINT8*)pTlv->data = neigh.dst_dpid;
                pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
                
                if (node->size < (pstCommHeader->msgLen + sizeof(tlv_t) + sizeof(UINT4)))
                {
                    stopped = TRUE;
                    break;
                }

                pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
                pTlv->type = TOPO_LINKS_DST_PORT;
                pTlv->len = sizeof(UINT4);
                *(UINT4*)pTlv->data = neigh.dst_port;
                pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
                
                if (node->size < (pstCommHeader->msgLen + sizeof(tlv_t) + sizeof(UINT8)))
                {
                    stopped = TRUE;
                    break;
                }

                pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
                pTlv->type = TOPO_LINKS_WEIGHT;
                pTlv->len = sizeof(UINT8);
                *(UINT8*)pTlv->data = neigh.weight;
                pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
            }

            if (stopped)
                break;
        }    

        if (stopped)
        {
            LOG_WARN("CSyncMgr persistTopoLinks stopped !");
            break;
        }
    }

	pstSynNodeHeader->synMsgLen = pstCommHeader->msgLen - sizeof(stCommMsgNode_header) - sizeof(stSynMsgNode_header);
	if (pstSynNodeHeader->synMsgLen > 0)
		persistData(pstCommHeader);

#if 0
    m_syncMem.releaseEntry(entry);
#else
    m_syncPool.release(node);
#endif

    return BNC_OK;
}

INT4 CSyncMgr::persistSwitchTag(UINT8 dpid, UINT4 tag)
{
    if (!persistNeeded())
        return BNC_ERR;

#if 0
    sync_entry_t* entry = m_syncMem.allocEntry();
    if (NULL == entry)
    {
        LOG_ERROR("CSyncMemory alloc entry failed !");
        return BNC_ERR;
    }
#else
    p_mem_node_t node = m_syncPool.alloc();
    if (NULL == node)
    {
        LOG_ERROR("m_syncPool alloc node failed !");
        return BNC_ERR;
    }
#endif

	stCommMsgNode_header* pstCommHeader = (stCommMsgNode_header *)node->data;
	pstCommHeader->msgLen = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header);
	pstCommHeader->msgNodeType = NODE_TYPE_SW_TAG;

	stSynMsgNode_header* pstSynNodeHeader = (stSynMsgNode_header *)(node->data + sizeof(stCommMsgNode_header));
	pstSynNodeHeader->synMsgType = NODE_TYPE_SW_TAG;
	pstSynNodeHeader->operMsgType = OPERATE_ADD;
	pstSynNodeHeader->synMsgLen = 0;

    tlv_t* pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = SW_TAG_DPID;
    pTlv->len = sizeof(UINT8);
    *(UINT8*)pTlv->data = dpid;
	pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
    
    pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = SW_TAG_TAG;
    pTlv->len = sizeof(UINT4);
    *(UINT4*)pTlv->data = tag;
	pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;

	pstSynNodeHeader->synMsgLen = pstCommHeader->msgLen - sizeof(stCommMsgNode_header) - sizeof(stSynMsgNode_header);

	persistData(pstCommHeader);

#if 0
    m_syncMem.releaseEntry(entry);
#else
    m_syncPool.release(node);
#endif

    return BNC_OK;
}

INT4 CSyncMgr::persistAllSwitchTag()
{
    if (!persistNeeded())
        return BNC_ERR;

#if 0
    sync_entry_t* entry = m_syncMem.allocEntry();
    if (NULL == entry)
    {
        LOG_ERROR("CSyncMemory alloc entry failed !");
        return BNC_ERR;
    }
#else
    p_mem_node_t node = m_syncPool.alloc();
    if (NULL == node)
    {
        LOG_ERROR("m_syncPool alloc node failed !");
        return BNC_ERR;
    }
#endif

	stCommMsgNode_header* pstCommHeader = (stCommMsgNode_header *)node->data;
	pstCommHeader->msgLen = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header);
	pstCommHeader->msgNodeType = NODE_TYPE_ALL_SW_TAG;

	stSynMsgNode_header* pstSynNodeHeader = (stSynMsgNode_header *)(node->data + sizeof(stCommMsgNode_header));
	pstSynNodeHeader->synMsgType = NODE_TYPE_ALL_SW_TAG;
	pstSynNodeHeader->operMsgType = OPERATE_MOD;
	pstSynNodeHeader->synMsgLen = 0;

    tlv_t* pTlv = NULL;

    CSwitchHMap& swMap = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    STL_FOR_LOOP(swMap, swit)
    {
		CSmartPtr<CSwitch> sw = swit->second;
		if (sw.isNotNull() && (0 != sw->getDpid()))
		{
            pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
            pTlv->type = SW_TAG_DPID;
            pTlv->len = sizeof(UINT8);
            *(UINT8*)pTlv->data = sw->getDpid();
            pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
            
            pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
            pTlv->type = SW_TAG_TAG;
            pTlv->len = sizeof(UINT4);
            *(UINT4*)pTlv->data = sw->getTag();
            pstCommHeader->msgLen += sizeof(tlv_t) + pTlv->len;
        }
    }

	CControl::getInstance()->getSwitchMgr().unlock();

	pstSynNodeHeader->synMsgLen = pstCommHeader->msgLen - sizeof(stCommMsgNode_header) - sizeof(stSynMsgNode_header);
	if (pstSynNodeHeader->synMsgLen > 0)
		persistData(pstCommHeader);

#if 0
    m_syncMem.releaseEntry(entry);
#else
    m_syncPool.release(node);
#endif

    return BNC_OK;
}

INT4 CSyncMgr::persistTagInfo()
{
    if (!persistNeeded())
        return BNC_ERR;

#if 0
    sync_entry_t* entry = m_syncMem.allocEntry();
    if (NULL == entry)
    {
        LOG_ERROR("CSyncMemory alloc entry failed !");
        return BNC_ERR;
    }
#else
    p_mem_node_t node = m_syncPool.alloc();
    if (NULL == node)
    {
        LOG_ERROR("m_syncPool alloc node failed !");
        return BNC_ERR;
    }
#endif

	stCommMsgNode_header* pstCommHeader = (stCommMsgNode_header *)node->data;
	pstCommHeader->msgLen = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header);
	pstCommHeader->msgNodeType = NODE_TYPE_TAG_INFO;

	stSynMsgNode_header* pstSynNodeHeader = (stSynMsgNode_header *)(node->data + sizeof(stCommMsgNode_header));
	pstSynNodeHeader->synMsgType = NODE_TYPE_TAG_INFO;
	pstSynNodeHeader->operMsgType = OPERATE_MOD;
	pstSynNodeHeader->synMsgLen = 0;

    tlv_t* pTlv = (tlv_t*)(node->data + pstCommHeader->msgLen);
    pTlv->type = TAG_INFO_VALUE;
    pTlv->len = CControl::getInstance()->getTagMgr().serialize(pTlv->data);

    pstSynNodeHeader->synMsgLen = sizeof(tlv_t) + pTlv->len;
	pstCommHeader->msgLen += pstSynNodeHeader->synMsgLen;

	persistData(pstCommHeader);

#if 0
    m_syncMem.releaseEntry(entry);
#else
    m_syncPool.release(node);
#endif

    return BNC_OK;
}

INT4 CSyncMgr::persistElectionGenerationId(UINT8 id)
{
    if (!persistNeeded())
        return BNC_ERR;

    LOG_INFO_FMT("persist ELECTION_GENERATION_ID %llu", id);

	INT1 val[TABLE_STRING_LEN] = {0};
    sprintf(val, "%llu", id);
    return m_dbClient.persistValue(ELECTION_GENERATION_ID, val);
}

INT4 CSyncMgr::persistMasterId(UINT8 id)
{
    if (!persistNeeded())
        return BNC_ERR;

    LOG_INFO_FMT("persist MASTER_ID %llu", id);

	INT1 val[TABLE_STRING_LEN] = {0};
    sprintf(val, "%llu", id);
    return m_dbClient.persistValue(MASTER_ID, val);
}

INT4 CSyncMgr::persistControllerInfo(UINT4 ip, UINT2 port, INT4 role)
{
    if (!persistNeeded())
        return BNC_ERR;

	INT1 key[TABLE_STRING_LEN] = {0};
	snprintf(key, TABLE_STRING_LEN, "SFabric_%s", inet_htoa(ip));

	stControllerInfo controllerInfo = {0};
	controllerInfo.ip = ip;
	controllerInfo.port = port;
	controllerInfo.role = role;

    return m_dbClient.persistData(key, (INT1*)&controllerInfo, sizeof(controllerInfo));
}

INT4 CSyncMgr::recover(const INT1* item)
{
    if (!recoverNeeded())
        return BNC_ERR;

    LOG_DEBUG_FMT("recover %s", item);

    INT4 nodeType = NODE_TYPE_MAX;
    if (strcmp(item, nodeTypeString[NODE_TYPE_TOPO_LINK]) == 0)
        nodeType = NODE_TYPE_TOPO_LINK;
    else if (strcmp(item, nodeTypeString[NODE_TYPE_ALL_TOPO_LINK]) == 0)
        nodeType = NODE_TYPE_ALL_TOPO_LINK;
    else if (strcmp(item, nodeTypeString[NODE_TYPE_SW_TAG]) == 0)
        nodeType = NODE_TYPE_SW_TAG;
    else if (strcmp(item, nodeTypeString[NODE_TYPE_ALL_SW_TAG]) == 0)
        nodeType = NODE_TYPE_ALL_SW_TAG;
    else if (strcmp(item, nodeTypeString[NODE_TYPE_TAG_INFO]) == 0)
        nodeType = NODE_TYPE_TAG_INFO;

    if (NODE_TYPE_MAX != nodeType)
    {
#if 0
        sync_entry_t* entry = m_syncMem.allocEntry();
        if (NULL == entry)
        {
            LOG_ERROR("CSyncMemory alloc entry failed !");
            return BNC_ERR;
        }
#else
        p_mem_node_t node = m_syncPool.alloc();
        if (NULL == node)
        {
            LOG_ERROR("m_syncPool alloc node failed !");
            return BNC_ERR;
        }
#endif
    
        UINT4 len = 0;
        INT4 ret = m_dbClient.queryData(nodeType, node->data, len);
        if ((BNC_OK == ret) && (len > 0) && (len <= node->size))
            recoverData((stCommMsgNode_header*)node->data);
    
#if 0
        m_syncMem.releaseEntry(entry);
#else
        m_syncPool.release(node);
#endif
    }
    else
    {
        if (strcmp(item, TRANS_SEQ) == 0)
        {
            UINT8 transSeq = 0;
            if (queryTransSeq(transSeq) == BNC_OK)
                if (transSeq > m_transSeq)
                    m_transSeq = transSeq;
        }
    }

    return BNC_OK;
}

INT4 CSyncMgr::persistData(const stCommMsgNode_header* pstCommHeader)
{
    if (NULL == pstCommHeader)
        return BNC_ERR;

    INT4 ret = m_dbClient.persistData(pstCommHeader->msgNodeType, (INT1*)pstCommHeader, pstCommHeader->msgLen);
    if (BNC_OK != ret)
    {
        LOG_WARN("CSyncMgr call persistData failed !");
        return BNC_ERR;
    }

    m_mutexSeq.lock();
    UINT8 transSeq = ++m_transSeq;
    m_mutexSeq.unlock();
    return persistTransSeq(transSeq);
}

INT4 CSyncMgr::queryTransSeq(UINT8& transSeq)
{
	INT1 val[TABLE_STRING_LEN] = {0};

    INT4 ret = m_dbClient.queryValue(TRANS_SEQ, val);
	if (BNC_OK == ret)
    	transSeq = string2Uint8(val);

	return ret;
}

INT4 CSyncMgr::queryElectionGenerationId(UINT8& id)
{
	INT1 val[TABLE_STRING_LEN] = {0};

    INT4 ret = m_dbClient.queryValue(ELECTION_GENERATION_ID, val);
	if (BNC_OK == ret)
    	id = string2Uint8(val);

	return ret;
}

INT4 CSyncMgr::queryMasterId(UINT8& id)
{
	INT1 val[TABLE_STRING_LEN] = {0};

    INT4 ret = m_dbClient.queryValue(MASTER_ID, val);
	if (BNC_OK == ret)
    	id = string2Uint8(val);

	return ret;
}

INT4 CSyncMgr::recoverData(const stCommMsgNode_header* pstCommHeader)
{
    INT4 ret = BNC_ERR;

	LOG_INFO("recoverData -- START");

	stSynMsgNode_header* pstSynNodeHeader = (stSynMsgNode_header *)(pstCommHeader + 1);
    INT1* data = (INT1*)(pstSynNodeHeader + 1);

    switch (pstCommHeader->msgNodeType)
    {
#if 0
    case FABRIC_HOST_NODE:
        LOG_INFO("recoverData -- FABRIC_HOST_NODE");
        recover_fabric_host_list(num, fields);
        break;
    case OPENSTACK_EXTERNAL_NODE:
        LOG_INFO("recoverData -- OPENSTACK_EXTERNAL_NODE");
        recover_fabric_openstack_external_list(num, fields);
        break;
    case NAT_ICMP:
        LOG_INFO("recoverData -- NAT_ICMP");
        recover_fabric_nat_icmp_iden_list(operateType, num, fields);
        break;
    case NAT_HOST:
        LOG_INFO("recoverData -- NAT_HOST");
        recover_fabric_nat_host_list(operateType, num, fields);
        break;
    case OPENSTACK_LBAAS_MEMBERS:
        LOG_INFO("recoverData -- OPENSTACK_LBAAS_MEMBERS");
        recover_fabric_openstack_lbaas_members_list(operateType, num, fields);
        break;
#endif
    case NODE_TYPE_TOPO_LINK:
        LOG_INFO("recover TOPO_LINK");
        ret = recoverTopoLink(data, pstSynNodeHeader->synMsgLen);
        break;
    case NODE_TYPE_ALL_TOPO_LINK:
        LOG_INFO("recover ALL_TOPO_LINK");
        ret = recoverAllTopoLink(data, pstSynNodeHeader->synMsgLen);
        break;
    case NODE_TYPE_ALL_SW_TAG:
        LOG_INFO("recover ALL_SW_TAG");
        ret = recoverAllSwitchTag(data, pstSynNodeHeader->synMsgLen);
        break;
    case NODE_TYPE_SW_TAG:
        LOG_INFO("recover SW_TAG");
        ret = recoverSwitchTag(data, pstSynNodeHeader->synMsgLen);
        break;
    case NODE_TYPE_TAG_INFO:
        LOG_INFO("recover TAG_INFO");
        ret = recoverTagInfo(data, pstSynNodeHeader->synMsgLen);
        break;
    default:
        break;
    }

	LOG_INFO("recoverData -- STOP");
    return ret;
}

INT4 CSyncMgr::recoverTopoLink(const INT1* data, UINT4 len)
{
    if ((NULL == data) || (0 == len))
        return BNC_ERR;

    LOG_DEBUG_FMT("recoverTopoLink: data[%p]len[%u]", data, len);

    neighbor_t neigh = {0};

    tlv_t* pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
        return BNC_ERR;
    len -= sizeof(tlv_t) + pTlv->len;
    data += sizeof(tlv_t) + pTlv->len;
    if (TOPO_LINKS_STATE != pTlv->type)
        return BNC_ERR;
    neigh.state = *(INT4*)pTlv->data;
    
    pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
        return BNC_ERR;
    len -= sizeof(tlv_t) + pTlv->len;
    data += sizeof(tlv_t) + pTlv->len;
    if (TOPO_LINKS_SRC_DPID != pTlv->type)
        return BNC_ERR;
    neigh.src_dpid = *(UINT8*)pTlv->data;
    
    pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
        return BNC_ERR;
    len -= sizeof(tlv_t) + pTlv->len;
    data += sizeof(tlv_t) + pTlv->len;
    if (TOPO_LINKS_SRC_PORT != pTlv->type)
        return BNC_ERR;
    neigh.src_port = *(UINT4*)pTlv->data;
    
    pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
        return BNC_ERR;
    len -= sizeof(tlv_t) + pTlv->len;
    data += sizeof(tlv_t) + pTlv->len;
    if (TOPO_LINKS_DST_DPID != pTlv->type)
        return BNC_ERR;
    neigh.dst_dpid = *(UINT8*)pTlv->data;
    
    pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
        return BNC_ERR;
    len -= sizeof(tlv_t) + pTlv->len;
    data += sizeof(tlv_t) + pTlv->len;
    if (TOPO_LINKS_DST_PORT != pTlv->type)
        return BNC_ERR;
    neigh.dst_port = *(UINT4*)pTlv->data;
    
    pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
        return BNC_ERR;
    len -= sizeof(tlv_t) + pTlv->len;
    data += sizeof(tlv_t) + pTlv->len;
    if (TOPO_LINKS_WEIGHT != pTlv->type)
        return BNC_ERR;
    neigh.weight = *(UINT8*)pTlv->data;
    
    CControl::getInstance()->getTopoMgr().recoverLink(neigh);

    return BNC_OK;
}

INT4 CSyncMgr::recoverAllTopoLink(const INT1* data, UINT4 len)
{
    if ((NULL == data) || (0 == len))
        return BNC_ERR;

    LOG_DEBUG_FMT("recoverAllTopoLink: data[%p]len[%u]", data, len);

    std::vector<CNeighborMap>& neighMapVec = CControl::getInstance()->getTopoMgr().getNeighMapVector();
    STL_FOR_LOOP(neighMapVec, neighMapIt)
        STL_FOR_LOOP(*neighMapIt, dpidIt)
            dpidIt->second.clear();

    neighbor_t neigh;
    tlv_t* pTlv = NULL;

    while (len > 0)
    {
        memset(&neigh, 0, sizeof(neigh));

        pTlv = (tlv_t*)data;
        if (len < sizeof(tlv_t) + pTlv->len)
            break;
        len -= sizeof(tlv_t) + pTlv->len;
        data += sizeof(tlv_t) + pTlv->len;
        if (TOPO_LINKS_STATE != pTlv->type)
            continue;
        neigh.state = *(INT4*)pTlv->data;

        pTlv = (tlv_t*)data;
        if (len < sizeof(tlv_t) + pTlv->len)
            break;
        len -= sizeof(tlv_t) + pTlv->len;
        data += sizeof(tlv_t) + pTlv->len;
        if (TOPO_LINKS_SRC_DPID != pTlv->type)
            continue;
        neigh.src_dpid = *(UINT8*)pTlv->data;

        pTlv = (tlv_t*)data;
        if (len < sizeof(tlv_t) + pTlv->len)
            break;
        len -= sizeof(tlv_t) + pTlv->len;
        data += sizeof(tlv_t) + pTlv->len;
        if (TOPO_LINKS_SRC_PORT != pTlv->type)
            continue;
        neigh.src_port = *(UINT4*)pTlv->data;

        pTlv = (tlv_t*)data;
        if (len < sizeof(tlv_t) + pTlv->len)
            break;
        len -= sizeof(tlv_t) + pTlv->len;
        data += sizeof(tlv_t) + pTlv->len;
        if (TOPO_LINKS_DST_DPID != pTlv->type)
            continue;
        neigh.dst_dpid = *(UINT8*)pTlv->data;

        pTlv = (tlv_t*)data;
        if (len < sizeof(tlv_t) + pTlv->len)
            break;
        len -= sizeof(tlv_t) + pTlv->len;
        data += sizeof(tlv_t) + pTlv->len;
        if (TOPO_LINKS_DST_PORT != pTlv->type)
            continue;
        neigh.dst_port = *(UINT4*)pTlv->data;

        pTlv = (tlv_t*)data;
        if (len < sizeof(tlv_t) + pTlv->len)
            break;
        len -= sizeof(tlv_t) + pTlv->len;
        data += sizeof(tlv_t) + pTlv->len;
        if (TOPO_LINKS_WEIGHT != pTlv->type)
            continue;
        neigh.weight = *(UINT8*)pTlv->data;

        CControl::getInstance()->getTopoMgr().recoverLink(neigh);
    }

    return BNC_OK;
}

INT4 CSyncMgr::recoverSwitchTag(const INT1* data, UINT4 len)
{
    if ((NULL == data) || (0 == len))
        return BNC_ERR;

    LOG_DEBUG_FMT("recoverSwitchTag: data[%p]len[%u]", data, len);

    tlv_t* pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
        return BNC_ERR;
    len -= sizeof(tlv_t) + pTlv->len;
    data += sizeof(tlv_t) + pTlv->len;
    if (SW_TAG_DPID != pTlv->type)
        return BNC_ERR;
    UINT8 dpid = *(UINT8*)pTlv->data;
    
    pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
        return BNC_ERR;
    len -= sizeof(tlv_t) + pTlv->len;
    data += sizeof(tlv_t) + pTlv->len;
    if (SW_TAG_TAG != pTlv->type)
        return BNC_ERR;
    UINT4 tag = *(UINT4*)pTlv->data;
    
    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
    if (sw.isNotNull())
        sw->setTag(tag);

    return BNC_OK;
}

INT4 CSyncMgr::recoverAllSwitchTag(const INT1* data, UINT4 len)
{
    if ((NULL == data) || (0 == len))
        return BNC_ERR;

    LOG_DEBUG_FMT("recoverAllSwitchTag: data[%p]len[%u]", data, len);

    UINT8 dpid = 0;
    UINT4 tag = 0;
    CSmartPtr<CSwitch> sw(NULL);
    tlv_t* pTlv = NULL;

    while (len > 0)
    {
        pTlv = (tlv_t*)data;
        if (len < sizeof(tlv_t) + pTlv->len)
            break;
        len -= sizeof(tlv_t) + pTlv->len;
        data += sizeof(tlv_t) + pTlv->len;
        if (SW_TAG_DPID != pTlv->type)
            continue;
        dpid = *(UINT8*)pTlv->data;

        pTlv = (tlv_t*)data;
        if (len < sizeof(tlv_t) + pTlv->len)
            break;
        len -= sizeof(tlv_t) + pTlv->len;
        data += sizeof(tlv_t) + pTlv->len;
        if (SW_TAG_TAG != pTlv->type)
            continue;
        tag = *(UINT4*)pTlv->data;

        sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(dpid);
        if (sw.isNotNull())
            sw->setTag(tag);
    }

    return BNC_OK;
}

INT4 CSyncMgr::recoverTagInfo(const INT1* data, UINT4 len)
{
    if ((NULL == data) || (0 == len))
        return BNC_ERR;

    LOG_DEBUG_FMT("recoverTagInfo: data[%p]len[%u]", data, len);

    tlv_t* pTlv = (tlv_t*)data;
    if (len < sizeof(tlv_t) + pTlv->len)
    {
        LOG_WARN_FMT("invalid recover data: len[%u]tlv-len[%u] !", len, pTlv->len);
        return BNC_ERR;
    }

    if (TAG_INFO_VALUE != pTlv->type)
    {
        LOG_WARN_FMT("invalid tlv-type[%u] != TAG_INFO_VALUE !", pTlv->type);
        return BNC_ERR;
    }

    CControl::getInstance()->getTagMgr().recover(pTlv->data, pTlv->len);

    return BNC_OK;
}

BOOL CSyncMgr::persistNeeded()
{
    return (CClusterMgr::getInstance()->isClusterOn() &&
            (OFPCR_ROLE_MASTER == CClusterMgr::getInstance()->getControllerRole()));
}

BOOL CSyncMgr::recoverNeeded()
{
    return (CClusterMgr::getInstance()->isClusterOn() &&
            (OFPCR_ROLE_SLAVE == CClusterMgr::getInstance()->getControllerRole()));
}


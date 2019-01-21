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
*   File Name   : CSyncMgr.h                                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CSYNCMGR_H
#define __CSYNCMGR_H

#include "bnc-type.h"
#include "bnc-param.h"
#include "CDbClientProxy.h"
#include "CTimer.h"
#include "CSyncDefine.h"
#include "CMemPool.h"
#include "CRedisSubscriber.h"

#if 0
typedef struct {
    UINT4 size;
    INT1* data;
}sync_entry_t;

/*
 * 主备同步内存管理类
 */
class CSyncMemory
{
public:
    /*
     * 默认构造函数
     */
    CSyncMemory();

    /*
     * 默认析构函数
     */
    ~CSyncMemory();

    /*
     * 初始化
     *
     * @return: 成功 or 失败
     */
    INT4 init();

    sync_entry_t* allocEntry();

    void releaseEntry(sync_entry_t* entry);

private:
    const static UINT4 init_sync_entry_count = 5;

    std::list<sync_entry_t*> m_freeEntries;
    std::list<sync_entry_t*> m_usedEntries;
    CMutex                   m_mutex;
};
#endif

/*
 * 主备同步管理类
 */
class CSyncMgr
{
public:
    static void syncData(void* param);

public:
    /*
     * 获取CSyncMgr实例
     *
     * @return: CSyncMgr*   CSyncMgr实例
     */
    static CSyncMgr* getInstance();

    /*
     * 默认析构函数
     */
    ~CSyncMgr();

    /*
     * 初始化
     *
     * @return: 成功 or 失败
     */
    INT4 init();

    INT4 persistTotal();
    INT4 persistTransSeq(UINT8 transSeq);
    INT4 persistTopoLink(const neighbor_t& neigh);
    INT4 persistAllTopoLink();
    INT4 persistSwitchTag(UINT8 dpid, UINT4 tag);
    INT4 persistAllSwitchTag();
    INT4 persistTagInfo();
    INT4 persistElectionGenerationId(UINT8 id);
    INT4 persistMasterId(UINT8 id);
    INT4 persistControllerInfo(UINT4 ip, UINT2 port, INT4 role);

    INT4 recover(const INT1* item);

    INT4 queryTransSeq(UINT8& transSeq);
    INT4 queryElectionGenerationId(UINT8& id);
    INT4 queryMasterId(UINT8& id);

private:
    /*
     * 默认构造函数
     */
    CSyncMgr();

    INT4 persistData(const stCommMsgNode_header* pstCommHeader);
    INT4 recoverData(const stCommMsgNode_header* pstCommHeader);
    INT4 recoverTopoLink(const INT1* data, UINT4 len);
    INT4 recoverAllTopoLink(const INT1* data, UINT4 len);
    INT4 recoverSwitchTag(const INT1* data, UINT4 len);
    INT4 recoverAllSwitchTag(const INT1* data, UINT4 len);
    INT4 recoverTagInfo(const INT1* data, UINT4 len);

    BOOL persistNeeded();
    BOOL recoverNeeded();

private:
    static const UINT4 sync_mem_node_size = 100;

    static CSyncMgr* m_instance;      

    CTimer         m_timer;
    CDbClientProxy m_dbClient;

    UINT8          m_transSeq;
    CMutex         m_mutexSeq;

    //CSyncMemory    m_syncMem;
    CMemEntry      m_syncPool;
};


#endif

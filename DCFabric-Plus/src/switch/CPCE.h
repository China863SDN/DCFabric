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
*   File Name   : CPCE.h                                                      *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CPCE_H
#define __CPCE_H

#include "bnc-type.h"
#include "CLFHashMap.h"
#include "CSmartPtr.h"
#include "CRefObj.h"
#include "CRWLock.h"

struct CFabricPathNode
{
    UINT8 dpid;
    UINT4 inport;
    UINT4 outport;
    CFabricPathNode* prev;
    CFabricPathNode* next;

    CFabricPathNode(UINT8 _dpid, UINT4 _inport, UINT4 _outport, CFabricPathNode* _prev=NULL, CFabricPathNode* _next=NULL)
        :dpid(_dpid),inport(_inport),outport(_outport),prev(_prev),next(_next) {}
};

class CFabricPath : public CRefObj
{
public:
    UINT4 weight;
    CFabricPathNode* head;

    CFabricPath(UINT4 _weight, CFabricPathNode* _head):weight(_weight),head(_head) {}
    ~CFabricPath() 
    {
        if (NULL != head)
        {
            CFabricPathNode* tail = head->prev;
            CFabricPathNode* next = NULL;
            while (NULL != head)
            {
                if (head != tail)
                {
                    next = head->next;
                    delete head;
                    head = next;
                }
                else
                {
                    delete head;
                    break;
                }
            }
            head = NULL;
        }
    }
};

typedef CUint64PairLFHashMap<CSmartPtr<CFabricPath> > CFabricPathMap;
typedef CUint64LFHashMap<std::map<UINT8, CSmartPtr<CFabricPath> > > CFabricPathBaseMap;

class CPCE
{
public:
    CPCE();
    ~CPCE();

    void generate();
    UINT4 getOutPort(UINT8 srcDpid, UINT8 dstDpid);

private:
    INT4 genBasePaths(CFabricPathMap& paths);
    INT4 genFirstStepPaths(CFabricPathMap& paths);
    INT4 genFurtherStepPaths(CFabricPathMap& paths);
    void insertBasePath(UINT8 srcDpid, UINT8 dstDpid, CSmartPtr<CFabricPath>& path);
    UINT4 genPath(CFabricPathMap& paths, CSmartPtr<CFabricPath>& pathLeft, 
        BOOL reverseLeft, CSmartPtr<CFabricPath>& pathRight, BOOL reverseRight);
    CFabricPathNode* dupPath(CFabricPathNode* path, BOOL reverse);
    CSmartPtr<CFabricPath> getPath(CFabricPathMap& paths, UINT8 srcDpid, UINT8 dstDpid);
    void enforcePath(UINT8 srcDpid, UINT4 outport, UINT8 dstDpid, UINT4 inport);

    void dumpPaths(CFabricPathMap& paths);

private:
    static const UINT4 path_hash_bucket_number = 1024000;
    static const UINT4 sw_hash_bucket_number = 10240;

    CFabricPathMap m_paths;
    CFabricPathBaseMap m_basePaths;
    CFabricPathMap m_oddStepPaths;
    CFabricPathMap m_evenStepPaths;
    UINT4 m_step;
    CRWLock m_rwlock;
};

#endif

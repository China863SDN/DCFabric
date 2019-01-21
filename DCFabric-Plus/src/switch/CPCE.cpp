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
*   File Name   : CPCE.cpp                                                    *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CPCE.h"
#include "comm-util.h"
#include "log.h"
#include "bnc-error.h"
#include "CControl.h"
#include "CFlowMgr.h"
#include "CTagFlowEventReportor.h"

CPCE::CPCE():
    m_paths(path_hash_bucket_number),
    m_basePaths(sw_hash_bucket_number),
    m_oddStepPaths(path_hash_bucket_number/10),
    m_evenStepPaths(path_hash_bucket_number/10),
    m_step(0)
{
}

CPCE::~CPCE()
{
}

void CPCE::generate()
{
    static CFabricPathMap paths(path_hash_bucket_number);

    paths.clear();
    m_basePaths.clear();
    m_oddStepPaths.clear();
    m_evenStepPaths.clear();
    m_step = 0;

    time_t start = time(NULL);

    if (genBasePaths(paths) != BNC_OK)
    {
    	LOG_WARN(">>> no path <<<");
        return;
    }
    m_step ++;

    if (genFirstStepPaths(paths) == BNC_OK)
    {
        m_step ++;
        while (genFurtherStepPaths(paths) == BNC_OK)
            m_step ++;
    }

    time_t end = time(NULL);
	LOG_WARN_FMT(">>> generate %lu paths take time %lus <<<", paths.size(), end - start);

	dumpPaths(paths);

    m_rwlock.wlock();
    m_paths = paths;
    m_rwlock.unlock();
}

UINT4 CPCE::getOutPort(UINT8 srcDpid, UINT8 dstDpid)
{
    UINT4 outport = 0;

    m_rwlock.rlock();
    CSmartPtr<CFabricPath> path = getPath(m_paths, srcDpid, dstDpid);
    if (path.isNotNull())
    	outport = (srcDpid < dstDpid) ? path->head->outport : path->head->prev->inport;
    m_rwlock.unlock();

    return outport;
}

INT4 CPCE::genBasePaths(CFabricPathMap& paths)
{
    INT4 ret = BNC_ERR;
    CFabricPathNode *head = NULL, *tail = NULL;
    CSmartPtr<CFabricPath> path(NULL);
    UINT8 srcDpid = 0, dstDpid = 0;
    UINT4 srcPort = 0, dstPort = 0;

    //time_t start = time(NULL);

    std::vector<CNeighborMap>& neighMapVec = CControl::getInstance()->getTopoMgr().getNeighMapVector();
    STL_FOR_LOOP(neighMapVec, neighMapIt)
    {
        STL_FOR_LOOP(*neighMapIt, dpidIt)
        {
            STL_FOR_LOOP(dpidIt->second, portIt)
            {
                neighbor_t& neigh = portIt->second;
                if (NEIGH_STATE_DELETED == neigh.state)
                    continue;

                if (getPath(paths, neigh.src_dpid, neigh.dst_dpid).isNotNull())
                    continue;
                
                if (neigh.src_dpid < neigh.dst_dpid)
                {
                    srcDpid = neigh.src_dpid;
                    srcPort = neigh.src_port;
                    dstDpid = neigh.dst_dpid;
                    dstPort = neigh.dst_port;
                }
                else
                {
                    srcDpid = neigh.dst_dpid;
                    srcPort = neigh.dst_port;
                    dstDpid = neigh.src_dpid;
                    dstPort = neigh.src_port;
                }

                head = new CFabricPathNode(srcDpid, 0, srcPort);
                tail = new CFabricPathNode(dstDpid, dstPort, 0, head, head);
                head->prev = tail;
                head->next = tail;
                path = CSmartPtr<CFabricPath>(new CFabricPath(1, head));

                paths.insert(std::pair<UINT8,UINT8>(srcDpid, dstDpid), path);
                insertBasePath(srcDpid, dstDpid, path);
                ret = BNC_OK;
#if 0
                enforcePath(srcDpid, srcPort, dstDpid, dstPort);
#else
                CTagFlowEventReportor::getInstance()->report(EVENT_TYPE_TAG_FLOW_ADD, 
                                                             EVENT_REASON_TAG_FLOW_ADD, 
                                                             srcDpid, 
                                                             srcPort, 
                                                             dstDpid, 
                                                             dstPort);
#endif
            }
        }
    }

    //time_t end = time(NULL);
	//LOG_WARN_FMT(">>> genBasePaths take time %lus <<<", end - start);

	LOG_WARN_FMT(">>> after genBasePaths, neighbor paths[%lu] basePaths[%lu] <<<", paths.size(), m_basePaths.size());
	//dumpPaths(paths);

    return ret;
}

INT4 CPCE::genFirstStepPaths(CFabricPathMap& paths)
{
    INT4 ret = BNC_ERR;

    //time_t start = time(NULL);

    CFabricPathMap neighPaths = paths;

    STL_FOR_LOOP(neighPaths, outIt)
    {
        const std::pair<UINT8,UINT8>& outKey = outIt->first;
        CSmartPtr<CFabricPath>& outPath = outIt->second;

        CFabricPathMap::CIterator inIt = outIt;
        for (++inIt; inIt != neighPaths.end(); ++inIt)
        {
            const std::pair<UINT8,UINT8>& inKey = inIt->first;
            CSmartPtr<CFabricPath>& inPath = inIt->second;
            if ((outKey.second < inKey.first) ||
                (outKey.first > inKey.second))
                //ignore cases: 1-2,3-4; 3-4,1-2
                continue;

            if (outKey.first == inKey.first)
            {
                if (getPath(paths, outKey.second, inKey.second).isNotNull())
                    continue;

                if (outKey.second < inKey.second) 
                    //1-2,1-3 ==> 2-1-3
                    genPath(paths, outPath, TRUE, inPath, FALSE);
                else 
                    //1-3,1-2 ==> 2-1-3
                    genPath(paths, inPath, TRUE, outPath, FALSE);
                ret = BNC_OK;
            }
            else if (outKey.first == inKey.second)
            {
                if (getPath(paths, inKey.first, outKey.second).isNotNull())
                    continue;

                //2-3,1-2 ==> 1-2-3
                genPath(paths, inPath, FALSE, outPath, FALSE);
                ret = BNC_OK;
            }
            else if (outKey.second == inKey.first)
            {
                if (getPath(paths, outKey.first, inKey.second).isNotNull())
                    continue;

                //1-2,2-3 ==> 1-2-3
                genPath(paths, outPath, FALSE, inPath, FALSE);
                ret = BNC_OK;
            }
            else if (outKey.second == inKey.second)
            {
                if (getPath(paths, outKey.first, inKey.first).isNotNull())
                    continue;

                if (outKey.first < inKey.first)
                    //1-3,2-3 ==> 1-3-2
                    genPath(paths, outPath, FALSE, inPath, TRUE);
                else
                    //2-3,1-3 ==> 1-3-2
                    genPath(paths, inPath, FALSE, outPath, TRUE);
                ret = BNC_OK;
            }
            else
            {
                //ignore cases: 1-3,2-4; 2-4,1-3; 2-3,1-4; 1-4,2-3
            }
        }
    }

    //time_t end = time(NULL);
	//LOG_WARN_FMT(">>> genFirstStepPaths[%u] take time %lus <<<", m_step, end - start);

	//LOG_WARN_FMT(">>> after genFirstStepPaths[%u], paths[%lu] basePaths[%lu] m_oddStepPaths[%lu] m_evenStepPaths[%lu] <<<", 
	//    m_step, paths.size(), m_basePaths.size(), m_oddStepPaths.size(), m_evenStepPaths.size());
	//dumpPaths(m_oddStepPaths);

    return ret;
}

INT4 CPCE::genFurtherStepPaths(CFabricPathMap& paths)
{
    INT4 ret = BNC_ERR;

    //time_t start = time(NULL);

    CFabricPathMap& prevStepPaths = ((m_step & 0x1) != 0) ? m_evenStepPaths : m_oddStepPaths;
    CFabricPathMap& currStepPaths = ((m_step & 0x1) != 0) ? m_oddStepPaths : m_evenStepPaths;
    currStepPaths.clear();

    STL_FOR_LOOP(prevStepPaths, prevIt)
    {
        const std::pair<UINT8,UINT8>& prevPair = prevIt->first;
        CSmartPtr<CFabricPath>& prevPath = prevIt->second;

        //2->1(1-2),2-XXX-4 ==> 1-2-XXX-4
        //2->3(2-3),2-XXX-4 ==> 3-2-XXX-4
        //2->5(2-5),2-XXX-4 ==> 4-XXX-2-5
        CFabricPathBaseMap::CPair* item = NULL;
        if (m_basePaths.find(prevPair.first, &item))
        {
            std::map<UINT8, CSmartPtr<CFabricPath> >& basePathMap = item->second;
            STL_FOR_LOOP(basePathMap, basePathIt)
            {
                if (basePathIt->first == prevPair.second)
                {
                    LOG_WARN_FMT("IMPOSSIBLE: 0x%llx-0x%llx,0x%llx-XXX-0x%llx", 
                        prevPair.first, basePathIt->first, prevPair.first, prevPair.second);
                    continue;
                }
                if (getPath(paths, basePathIt->first, prevPair.second).isNotNull())
                    continue;

                CSmartPtr<CFabricPath>& basePath = basePathIt->second;
                if (basePathIt->first < prevPair.second)
                {
                    if (basePathIt->first < prevPair.first)
                        //2->1(1-2),2-XXX-4 ==> 1-2-XXX-4
                        genPath(paths, basePath, FALSE, prevPath, FALSE);
                    else
                        //2->3(2-3),2-XXX-4 ==> 3-2-XXX-4
                        genPath(paths, basePath, TRUE, prevPath, FALSE);
                }
                else
                {
                    //2->5(2-5),2-XXX-4 ==> 4-XXX-2-5
                    genPath(paths, prevPath, TRUE, basePath, FALSE);
                }
                ret = BNC_OK;
            }
        }

        //4->1(1-4),2-XXX-4 ==> 1-4-XXX-2
        //4->3(2-4),2-XXX-4 ==> 2-XXX-4-3
        //4->5(4-5),2-XXX-4 ==> 2-XXX-4-5
        if (m_basePaths.find(prevPair.second, &item))
        {
            std::map<UINT8, CSmartPtr<CFabricPath> >& basePathMap = item->second;
            STL_FOR_LOOP(basePathMap, basePathIt)
            {
                if (basePathIt->first == prevPair.first)
                {
                    LOG_WARN_FMT("IMPOSSIBLE: 0x%llx-0x%llx,0x%llx-XXX-0x%llx", 
                        prevPair.second, basePathIt->first, prevPair.first, prevPair.second);
                    continue;
                }
                if (getPath(paths, basePathIt->first, prevPair.first).isNotNull())
                    continue;
        
                CSmartPtr<CFabricPath>& basePath = basePathIt->second;
                if (basePathIt->first < prevPair.first)
                {
                    //4->1(1-4),2-XXX-4 ==> 1-4-XXX-2
                    genPath(paths, basePath, FALSE, prevPath, TRUE);
                }
                else
                {
                    if (basePathIt->first < prevPair.second)
                        //4->3(3-4),2-XXX-4 ==> 2-XXX-4-3
                        genPath(paths, prevPath, FALSE, basePath, TRUE);
                    else
                        //4->5(4-5),2-XXX-4 ==> 2-XXX-4-5
                        genPath(paths, prevPath, FALSE, basePath, FALSE);
                }
                ret = BNC_OK;
            }
        }
    }

    //time_t end = time(NULL);
	//LOG_WARN_FMT(">>> genFurtherStepPaths[%u] take time %lus <<<", m_step, end - start);

	//LOG_WARN_FMT(">>> after genFurtherStepPaths[%u], paths[%lu] basePaths[%lu] m_oddStepPaths[%lu] m_evenStepPaths[%lu] <<<", 
	//    m_step, paths.size(), m_basePaths.size(), m_oddStepPaths.size(), m_evenStepPaths.size());
	//dumpPaths(currStepPaths);

    return ret;
}

void CPCE::insertBasePath(UINT8 srcDpid, UINT8 dstDpid, CSmartPtr<CFabricPath>& path)
{
    //srcDpid-->dstDpid-->path
    CFabricPathBaseMap::CPair* item = NULL;
    if (!m_basePaths.find(srcDpid, &item))
        if (!m_basePaths.insert(srcDpid, std::map<UINT8, CSmartPtr<CFabricPath> >(), &item))
            return;

    std::map<UINT8, CSmartPtr<CFabricPath> >& dstMap = item->second;
    //std::map<UINT8, CSmartPtr<CFabricPath> >::iterator dstIt = dstMap.find(dstDpid);
    //if (dstIt == dstMap.end())
        dstMap.insert(std::pair<UINT8, CSmartPtr<CFabricPath> >(dstDpid, path));
    //else
    //    dstIt->second = path;

    //dstDpid-->srcDpid-->path
    if (!m_basePaths.find(dstDpid, &item))
        if (!m_basePaths.insert(dstDpid, std::map<UINT8, CSmartPtr<CFabricPath> >(), &item))
            return;

    std::map<UINT8, CSmartPtr<CFabricPath> >& srcMap = item->second;
    //std::map<UINT8, CSmartPtr<CFabricPath> >::iterator srcIt = srcMap.find(srcDpid);
    //if (srcIt == srcMap.end())
        srcMap.insert(std::pair<UINT8, CSmartPtr<CFabricPath> >(srcDpid, path));
    //else
    //    srcIt->second = path;
}

UINT4 CPCE::genPath(CFabricPathMap& paths, CSmartPtr<CFabricPath>& pathLeft, BOOL reverseLeft, 
    CSmartPtr<CFabricPath>& pathRight, BOOL reverseRight)
{
    CFabricPathNode* left = dupPath(pathLeft->head, reverseLeft);
    CFabricPathNode* right = dupPath(pathRight->head, reverseRight);

    //will free node right
    CFabricPathNode* leftTail = left->prev;
    CFabricPathNode* rightTail = right->prev;
    CFabricPathNode* rightSecond = right->next;
    if (rightTail == rightSecond)
    {
        leftTail->outport = right->outport;
        leftTail->next = rightTail;
        rightTail->prev = leftTail;
        rightTail->next = left;
        left->prev = rightTail;
    }
    else
    {
        leftTail->outport = right->outport;
        leftTail->next = rightSecond;
        rightSecond->prev = leftTail;
        rightTail->next = left;
        left->prev = rightTail;
    }
    delete right;
    
    CSmartPtr<CFabricPath> pathGen = CSmartPtr<CFabricPath>(new CFabricPath(pathLeft->weight+pathRight->weight, left));
    paths.insert(std::pair<UINT8,UINT8>(left->dpid, left->prev->dpid), pathGen);

    CFabricPathMap& stepPaths = ((m_step & 0x1) != 0) ? m_oddStepPaths : m_evenStepPaths;
    stepPaths.insert(std::pair<UINT8,UINT8>(left->dpid, left->prev->dpid), pathGen);

#if 0
    enforcePath(left->dpid, left->outport, left->prev->dpid, left->prev->inport);
#else
    CTagFlowEventReportor::getInstance()->report(EVENT_TYPE_TAG_FLOW_ADD, 
                                                 EVENT_REASON_TAG_FLOW_ADD, 
                                                 left->dpid, 
                                                 left->outport, 
                                                 left->prev->dpid, 
                                                 left->prev->inport);
#endif

    return pathGen->weight;
}

CFabricPathNode* CPCE::dupPath(CFabricPathNode* path, BOOL reverse)
{
    CFabricPathNode *head = NULL, *prev = NULL, *curr = NULL, *node = NULL;
    if (!reverse)
    {
        node = path;
        do
        {
            curr = new CFabricPathNode(node->dpid, node->inport, node->outport, prev, head);
            (NULL != prev) ? prev->next = curr : head = curr;
            prev = curr;

            node = node->next;
        } while (node != path);

        head->prev = curr;
    }
    else
    {
        node = path->prev;
        do
        {
            curr = new CFabricPathNode(node->dpid, node->outport, node->inport, prev, head);
            (NULL != prev) ? prev->next = curr : head = curr;
            prev = curr;

            node = node->prev;
        } while (node != path->prev);

        head->prev = curr;
    }

    return head;
}

CSmartPtr<CFabricPath> CPCE::getPath(CFabricPathMap& paths, UINT8 srcDpid, UINT8 dstDpid)
{
    UINT8 src = (srcDpid < dstDpid) ? srcDpid : dstDpid;
    UINT8 dst = (srcDpid < dstDpid) ? dstDpid : srcDpid;

    CFabricPathMap::CPair* item = NULL;
    if (paths.find(std::pair<UINT8,UINT8>(src, dst), &item))
        return item->second;

    return CSmartPtr<CFabricPath>(NULL);
}

void CPCE::enforcePath(UINT8 srcDpid, UINT4 outport, UINT8 dstDpid, UINT4 inport)
{
    CSwitchMgr& swMgr = CControl::getInstance()->getSwitchMgr();
    CSmartPtr<CSwitch> srcSw = swMgr.getSwitchByDpid(srcDpid);
    CSmartPtr<CSwitch> dstSw = swMgr.getSwitchByDpid(dstDpid);
    if (srcSw.isNotNull() && dstSw.isNotNull())
    {
        CFlowMgr::getInstance()->install_different_switch_flow(dstSw, srcSw, outport);
        CFlowMgr::getInstance()->install_different_switch_flow(srcSw, dstSw, inport);
    }
}

void CPCE::dumpPaths(CFabricPathMap& paths)
{
    LOG_WARN_FMT("---------------dumpPaths %lu START---------------", paths.size());

    INT1 str[1024] = {0}, dpid[48] = {0};
    UINT4 len = 0, count = 0;;

    STL_FOR_LOOP(paths, it)
    {
        CSmartPtr<CFabricPath>& path = it->second;
        CFabricPathNode* node = path->head;

        len = 0;
        do
        {
            if (node != path->head)
                len += snprintf(str+len, sizeof(str)-len, " <==> ");
            len += snprintf(str+len, sizeof(str)-len, "[%u][%s][%u]", 
                node->inport, dpidUint8ToStr(node->dpid, dpid), node->outport);

            node = node->next;
        } while (node != path->head);
        LOG_WARN_FMT("####### %s", str);

        if (++count > 10)
            break;
    }

    LOG_WARN("---------------dumpPaths END---------------");
}


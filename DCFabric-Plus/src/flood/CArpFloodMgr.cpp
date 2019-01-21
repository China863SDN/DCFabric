
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
*   File Name   : CArpFloodMgr.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "CArpFloodMgr.h"
#include "log.h"

CArpFloodMgr*  CArpFloodMgr::m_pInstance = NULL;


static void* ArpFloodThread(void * param)
{
	UINT4 dstIp = 0;
	CArpFloodMap::iterator  it;
	if (NULL == param)
    {
        return NULL;
    }

	prctl(PR_SET_NAME, (unsigned long)"ArpFloodThread");  

    CArpFloodMgr* worker = (CArpFloodMgr*)param;
	
	while(1)
	{
		//LOG_ERROR_FMT("%s %d",FN,LN);
		worker->getArpRequestDstSem().wait();
		
		//LOG_ERROR_FMT("%s %d",FN,LN);
		if(!worker->getArpRequestDstQueue().empty())
		{
			dstIp = worker->getArpRequestDstQueue().front();
			worker->getArpRequestDstQueue().pop();
			//LOG_ERROR_FMT("%s %d dstIp=0x%x",FN,LN,dstIp);
			it = worker->getArpRequestDstMap().find(dstIp);
			if( worker->getArpRequestDstMap().end() != it)
			{
				//LOG_ERROR_FMT("%s %d dstIp=0x%x it->second=0x%x",FN,LN,it->first,it->second);
				COfMsgUtil::ofp13floodInternal(it->second);
				if(NULL != it->second->data)
					free( it->second->data);
				delete it->second;
				worker->getArpRequestDstMapMutex().lock();
				worker->getArpRequestDstMap().erase(it);
				worker->getArpRequestDstMapMutex().unlock();
			}
			
		}
		#if  0
		STL_FOR_MAP_LOOP(worker->getArpRequestDstMap(),iter)
		{
			it = iter;
			iter++;
			if(NULL != it->second)
			{
				
				LOG_ERROR_FMT("%s %d dstIp=0x%x ",FN,LN,it->first);
				COfMsgUtil::ofp13floodInternal(it->second);
				
				worker->getArpRequestDstMap().erase(it);
				break;
			}
			
		}
		#endif
				
	}
}
CArpFloodMgr::CArpFloodMgr()
{
	
}
CArpFloodMgr::~CArpFloodMgr()
{
	if(NULL != m_pInstance)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

INT4  CArpFloodMgr::init()
{
	INT4 ret = 0;
	if ((ret = pthread_create(&m_threadId, NULL, ArpFloodThread, (void*)this)) != 0)
    {
        LOG_ERROR_FMT("%s pthread_create failed[%d]!", toString(), ret);
        return BNC_ERR;
    }
	return BNC_OK;
}

INT4  CArpFloodMgr::AddArpFloodDstMapNode(UINT4 dstIp, packet_in_info_t *packet_in)
{
	CArpFloodMap::iterator iter = m_dstIpArpFloodMap.find(dstIp);
	if( m_dstIpArpFloodMap.end() != iter)
	{
		//LOG_ERROR_FMT("%s %d",FN,LN);
		return BNC_ERR;
	}
	m_mapmutex.lock();
	m_dstIpArpFloodMap.insert(std::make_pair(dstIp,packet_in ));
	m_mapmutex.unlock();
	m_dstIpArpQue.push(dstIp);
	m_sem.post();
	//LOG_ERROR_FMT("%s %d dstIp=0x%x datalen=%d packet_in=0x%x xid=%d",FN,LN,dstIp,packet_in->data_len,packet_in,packet_in->xid);
	return BNC_OK;
}
	
INT4  CArpFloodMgr::AddArpRequestNode(p_arp_request arpRequestNode)
{
	if((NULL == arpRequestNode)||(NULL == arpRequestNode->packet_in))
	{
		return BNC_ERR;
	}
	packet_in_info_t *packout_req_info = new packet_in_info_t();
	memcpy(packout_req_info, arpRequestNode->packet_in, sizeof(packet_in_info_t));
	packout_req_info->data = (UINT1*)malloc(arpRequestNode->packet_in->data_len*sizeof(UINT1)); 
	memcpy(packout_req_info->data, arpRequestNode->packet_in->data, arpRequestNode->packet_in->data_len);
	
	LOG_ERROR_FMT("%s %d",FN,LN);
	AddArpFloodDstMapNode(arpRequestNode->dstIp, packout_req_info);
	
	m_mutex.lock();
	STL_FOR_LOOP(m_arpfloodList,iter)
	{
		if(*iter &&( (arpRequestNode->srcIp == (*iter)->srcIp)&&(arpRequestNode->dstIp == (*iter)->dstIp)))
		{
			m_mutex.unlock();
			return BNC_ERR;
		}
	}
	m_arpfloodList.push_back(arpRequestNode);
	m_mutex.unlock();
	
	return BNC_OK;
}
INT4  CArpFloodMgr::AddArpRequestNode(UINT4 dstIp, UINT4 srcIp, packet_in_info_t *packet_in)
{
	if(NULL == packet_in)
	{
		return BNC_ERR;
	}
	packet_in_info_t *packout_req_info = new packet_in_info_t();
	memcpy(packout_req_info, packet_in, sizeof(packet_in_info_t));
	packout_req_info->data = (UINT1*)malloc(packet_in->data_len*sizeof(UINT1)); 
	memcpy(packout_req_info->data, packet_in->data, packet_in->data_len);
	
	arp_request_t* Node = new arp_request_t();
	Node->srcIp = srcIp;
	Node->dstIp = dstIp;
	Node->packet_in = packout_req_info;
	//LOG_ERROR_FMT("%s %d dstIp=0x%x srcIp=0x%x packet_in->datalen=%d",FN,LN,dstIp,srcIp,packet_in->data_len);
	AddArpFloodDstMapNode(dstIp, packout_req_info);
	
	m_mutex.lock();
	STL_FOR_LOOP(m_arpfloodList,iter)
	{
		if(*iter &&( (srcIp == (*iter)->srcIp)&&(dstIp == (*iter)->dstIp)))
		{
			m_mutex.unlock();
			return BNC_ERR;
		}
	}
	m_arpfloodList.push_back(Node);
	m_mutex.unlock();
	return BNC_OK;
}
INT4  CArpFloodMgr::AddArpRequestNode(UINT4 dstIp, UINT4 srcIp, UINT1* src_mac)
{
	packet_in_info_t packout_req_info;
	arp_t* new_arp_pkt =  new arp_t();

	packout_req_info.buffer_id = 0xffffffff;
	packout_req_info.inport = OFPP13_CONTROLLER;
	packout_req_info.xid = 0;
	packout_req_info.data_len = sizeof(arp_t);
	packout_req_info.data = (UINT1 *)new_arp_pkt;

	memcpy(new_arp_pkt->eth_head.src, src_mac, 6);
	memcpy(new_arp_pkt->eth_head.dest, arp_broadcat_mac, 6);
	new_arp_pkt->eth_head.proto = htons(ETHER_ARP);
	new_arp_pkt->hardwaretype = htons(1);
	new_arp_pkt->prototype = htons(ETHER_IP);
	new_arp_pkt->hardwaresize = 0x6;
	new_arp_pkt->protocolsize = 0x4;
	new_arp_pkt->opcode = htons(1);
	new_arp_pkt->sendip = srcIp;
	new_arp_pkt->targetip=dstIp;

	memcpy(new_arp_pkt->sendmac, src_mac, 6);
	memcpy(new_arp_pkt->targetmac, arp_zero_mac, 6);

	//AddArpRequestNode(dstIp, srcIp, &packout_req_info);
	 COfMsgUtil::ofp13floodInternal(&packout_req_info);
	delete new_arp_pkt;
	return BNC_OK;
}

INT4  CArpFloodMgr::DelArpRequestNode(UINT4 dstIp, UINT4 srcIp)
{

	//LOG_ERROR_FMT("%s %d dstIp=0x%x srcIp=0x%x",FN,LN,dstIp,srcIp);
	m_mutex.lock();
	STL_FOR_LOOP(m_arpfloodList,iter)
	{
		if(*iter &&( (srcIp == (*iter)->srcIp)&&(dstIp == (*iter)->dstIp)))
		{
			
			m_arpfloodList.remove(*iter);
			delete *iter;
			m_mutex.unlock();
			return BNC_OK;
		}
	}
	m_mutex.unlock();
	return BNC_ERR;
}

INT4  CArpFloodMgr::DelArpRequestNodeByDstIp(UINT4 dstIp)
{
	m_mutex.lock();
	STL_FOR_MAP(m_arpfloodList,iter)
	{
		if(*iter &&( dstIp == (*iter)->dstIp))
		{
			delete *iter;
			m_arpfloodList.erase(iter);
			
		}
	}
	m_mutex.unlock();
	return BNC_OK;
}

CArpFloodMgr*  CArpFloodMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CArpFloodMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
		m_pInstance->init();
	}
	return m_pInstance;
}

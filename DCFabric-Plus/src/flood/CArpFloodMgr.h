
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
*   File Name   : CArpFloodMgr.h           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/


#ifndef  _CARPFLOODMGR_H_
#define  _CARPFLOODMGR_H_

#include "bnc-type.h"
#include "bnc-error.h"
#include "bnc-inet.h"
#include "bnc-param.h"
#include "comm-util.h"
#include "COfMsgUtil.h"
#include "CSemaphore.h"

const UINT1 arp_zero_mac[] = {0x0,0x0,0x0,0x0,0x0,0x0};
const UINT1 arp_broadcat_mac[] = {0xff,0xff,0xff,0xff,0xff,0xff};

typedef   std::map<UINT4, packet_in_info_t* >    CArpFloodMap;
class CArpFloodMgr
{
	private:
		CArpFloodMgr();
		INT4 init();
		INT4  AddArpFloodDstMapNode(UINT4 dstIp, packet_in_info_t *packet_in);
		const char* toString() {return "CArpFloodMgr";}
	public:
		~CArpFloodMgr();
		INT4  AddArpRequestNode(UINT4 dstIp, UINT4 srcIp, packet_in_info_t *packet_in);
		INT4  AddArpRequestNode(UINT4 dstIp, UINT4 srcIp, UINT1* src_mac);
		INT4  AddArpRequestNode(p_arp_request arprequestNode);
		INT4  DelArpRequestNode(UINT4 dstIp, UINT4 srcIp);
		INT4  DelArpRequestNodeByDstIp(UINT4 dstIp);
		CSemaphore&    getArpRequestDstSem()  {return m_sem; }
		CArpFloodMap&  getArpRequestDstMap()  {return m_dstIpArpFloodMap; }
		CMutex&        getArpRequestDstMapMutex(){return m_mapmutex; }
		std::queue<UINT4>& getArpRequestDstQueue() {return m_dstIpArpQue;}
		static CArpFloodMgr*  getInstance();

		
	private:
		CMutex     					m_mutex;
		CSemaphore 					m_sem;
		pthread_t 					m_threadId;
		CArpFloodMap 				m_dstIpArpFloodMap;
		CMutex						m_mapmutex;
		std::queue<UINT4>           m_dstIpArpQue;
		std::list<arp_request_t *> 	m_arpfloodList;
		static CArpFloodMgr*  		m_pInstance;
	
};
#endif


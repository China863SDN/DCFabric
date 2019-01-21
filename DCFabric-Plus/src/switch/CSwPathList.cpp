
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
*   File Name   : CNetwork.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2017-11-16           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "bnc-error.h"
#include "comm-util.h"
#include "CControl.h"
#include "CPathNodeList.h"
#include "CSwPathList.h"

CSwPathList::CSwPathList()
{
	
}

CSwPathList::CSwPathList(const CSmartPtr<CSwitch >& sw):m_sw(sw),m_num(0)
{
	//m_sw = sw;

	/*
	CPathNodeList* Node = new CPathNodeList(sw, sw);
	if((sw.isNotNull())&&(NULL != Node))
	{
		m_sw_path.push_back(Node);
	}
	*/
}

CSwPathList::~CSwPathList()
{
	
}

INT4 CSwPathList::AddSwOnePath(CPathNodeList * NodeList)
{
	if(NULL == NodeList)
	{
		return BNC_ERR;
	}
	m_sw_path.push_back(NodeList);
    m_num++;
	return BNC_OK;
	
}

INT4 CSwPathList::InsertSwOnePath(CPathNodeList * NodeList)
{
	if(NULL == NodeList)
	{
		return BNC_ERR;
	}
	m_sw_path.push_front(NodeList);
    m_num++;

	return BNC_OK;
	
}

INT4 CSwPathList::DelSwOnePath(const CSmartPtr < CSwitch > & src_sw)
{
	CPathNodeList* NodeList = NULL;
	if(src_sw.isNull())
	{
		return BNC_ERR;
	}
	STL_FOR_LOOP(m_sw_path,iter)
	{
		if((NULL != *iter)&&(src_sw == (*iter)->getPathNodeListSrcSw()))
		{
			NodeList = *iter;
			m_sw_path.erase(iter);
			NodeList->DeletePathNodeList();
			m_num--;
			return BNC_OK;
		}
	}
	return BNC_ERR;
}

INT4 CSwPathList::DelSwAllPath()
{
	m_num = 0;
	return BNC_OK;
}

CPathNodeList* CSwPathList::RemoveFirstPath()
{
	CPathNodeList* NodeList = NULL;
	if(m_sw_path.empty())
	{
		return NULL;
	}
	NodeList = m_sw_path.front();
	m_sw_path.pop_front();
	return NodeList;
}

CPathNodeList*  CSwPathList::findOnePathBySw(const CSmartPtr<CSwitch >& src_sw)
{
	if(src_sw.isNull())
	{
		return NULL;
	}
	STL_FOR_LOOP(m_sw_path,iter)
	{
		if((NULL != *iter)&&(src_sw == (*iter)->getPathNodeListSrcSw()))
		{
			return *iter;
		}
	}
	return NULL;
}

CPathNodeList*  CSwPathList::findOnePathBySw(const UINT8 & src_dpid)
{
	CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(src_dpid);
	if(sw.isNull())
	{
		return NULL;
	}

	STL_FOR_LOOP(m_sw_path,iter)
	{
		if((NULL != *iter)&&(sw == (*iter)->getPathNodeListSrcSw()))
		{
			return *iter;
		}
	}
	return NULL;
	
}




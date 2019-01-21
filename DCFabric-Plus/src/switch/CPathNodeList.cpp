
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

#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CSwitch.h"
#include "CPathNode.h"
#include "CPathNodeList.h"

INT4 CPathNodeList::AddPathNode(CPathNode*  Node)
{
	if(NULL == Node)
	{
		return BNC_ERR;
	}
    m_node_list.push_back(Node);
	return BNC_OK;
}

INT4 CPathNodeList::InsertPathNode(CPathNode*  Node)
{
	if(NULL == Node)
	{
		return BNC_ERR;
	}
    m_node_list.push_front(Node);
	return BNC_OK;
}

INT4 CPathNodeList::DelPathNode(CPathNode*  Node)
{
	if(NULL == Node)
	{
		return BNC_ERR;
	}
	STL_FOR_LOOP(m_node_list,iter)
	{
		if((*iter)&&((*iter)->getPathNodeSw() == Node->getPathNodeSw()))
		{
			m_node_list.erase(iter);
			break;
		}
	}
	return BNC_OK;
}

CPathNode* CPathNodeList::CopyPathNode(CPathNode * Node)
{
	CPathNode* retNode = new CPathNode();
	if((NULL == Node)||(NULL == retNode))
	{
		return NULL;
	}
	retNode->setPathNode(Node->getPathNodeSw(), Node->getPathNodePortNo());
	return retNode;
	
	
}
INT4 CPathNodeList::AddPathNode(const CSmartPtr<CSwitch>&  sw, const UINT4 & port_no)
{
	CPathNode* Node = new CPathNode(sw, port_no);
	if(NULL == Node)
	{
		return BNC_ERR;
	}
	
	m_node_list.push_back(Node);
	return BNC_OK;
}

INT4 CPathNodeList::InsertPathNode(const CSmartPtr<CSwitch>&  sw, const UINT4 & port_no)
{
	CPathNode* Node = new CPathNode(sw, port_no);
	if(NULL == Node)
	{
		return BNC_ERR;
	}
	
	m_node_list.push_front(Node);
	return BNC_OK;
}

INT4 CPathNodeList::DelPathNode(const CSmartPtr<CSwitch>& sw)
{
	if(sw.isNull())
	{
		return BNC_ERR;
	}
	STL_FOR_LOOP(m_node_list,iter)
	{
		if((*iter)&&((*iter)->getPathNodeSw() == sw))
		{
			m_node_list.erase(iter);
			break;
		}
	}
	return BNC_OK;
}

INT4 CPathNodeList::SetPathSrcDst(const CSmartPtr<CSwitch>& src_sw, const CSmartPtr<CSwitch>& dst_sw)
{
	if(src_sw.isNull() || dst_sw.isNull())
	{
		return BNC_ERR;
	}

	m_src_sw = src_sw;
	m_dst_sw = dst_sw;

	return BNC_OK;
}


CPathNode*  CPathNodeList::FindPathNodeBySw(const CSmartPtr < CSwitch > & sw)
{
	if(sw.isNull())
	{
		return NULL;
	}

	STL_FOR_LOOP(m_node_list,iter)
	{
		if((*iter)&&((*iter)->getPathNodeSw() == sw))
		{
			return *iter;
		}
	}
	return NULL;
}

std::list<CPathNode *> CPathNodeList::CopyPathNodeList()
{
	std::list<CPathNode *> ret_NodeList;
	CPathNode* Node = NULL;
	if(m_node_list.empty())
	{
		return m_node_list;
	}
	
	STL_FOR_LOOP(m_node_list,iter)
	{
		if((*iter)&&((*iter)->getPathNodeSw().isNotNull()))
		{
			Node= new CPathNode((*iter)->getPathNodeSw(), (*iter)->getPathNodePortNo());
			ret_NodeList.push_back(Node);
		}
	}
	return ret_NodeList;
	
}
std::list<CPathNode *> CPathNodeList::DeletePathNodeList()
{
	CPathNode* Node = NULL;
	STL_FOR_LOOP(m_node_list,iter)
	{
		if((*iter)&&((*iter)->getPathNodeSw().isNotNull()))
		{
			Node = *iter;
			m_node_list.erase(iter);
			delete Node;
		}
	}
	return m_node_list;
}

CPathNodeList* CPathNodeList::ClonePathNodeList()
{
	CPathNode* Node = NULL;
	CPathNodeList*   ret_PathNodeList = new CPathNodeList();

	if(NULL == ret_PathNodeList)
	{
		return NULL;
	}

	ret_PathNodeList->m_dst_sw = m_dst_sw;
	ret_PathNodeList->m_src_sw = m_src_sw;

	STL_FOR_LOOP(m_node_list,iter)
	{
		if((*iter)&&((*iter)->getPathNodeSw().isNotNull()))
		{
			Node = new CPathNode((*iter)->getPathNodeSw(), (*iter)->getPathNodePortNo());
			ret_PathNodeList->m_node_list.push_back(Node);
		}
	}
	return ret_PathNodeList;
}

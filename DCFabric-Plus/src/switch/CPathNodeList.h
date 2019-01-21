
#ifndef _CPATHNODELIST_H_
#define _CPATHNODELIST_H_

#include "CSwitch.h"
#include "CPathNode.h"

typedef std::map<UINT8, CPathNode*>    CPathNodeMap;

class CPathNodeList
{
	public:
		CPathNodeList(){};
		CPathNodeList(const CSmartPtr<CSwitch>& src_sw, const CSmartPtr<CSwitch>& dst_sw):m_src_sw(src_sw), m_dst_sw(dst_sw){};
		~CPathNodeList(){};

		const CSmartPtr<CSwitch>& getPathNodeListDstSw() const {return m_dst_sw; };
		const CSmartPtr<CSwitch>& getPathNodeListSrcSw() const {return m_src_sw; };
		//add Node to PathList form src_sw to dst_sw 
		INT4 AddPathNode(CPathNode*  Node);
		//insert Node to PathList from src_sw to dst_sw
		INT4 InsertPathNode(CPathNode*  Node);
		//del Node from PathList by Node switch
		INT4 DelPathNode(CPathNode*  Node);
		CPathNode* CopyPathNode(CPathNode*  Node);

		//add switch Node to PathList form src_sw to dst_sw 
		INT4 AddPathNode(const CSmartPtr<CSwitch>&  sw, const UINT4 & port_no);
		//insert switch Node to PathList from src_sw to dst_sw
		INT4 InsertPathNode(const CSmartPtr<CSwitch>&  sw, const UINT4 & port_no);
		
		//del Node from PathList by Node switch
		INT4 DelPathNode(const CSmartPtr<CSwitch>& sw);
		//set the pathList src_sw and dst_sw, the beginning and the end
		INT4 SetPathSrcDst(const CSmartPtr<CSwitch>& src_sw, const CSmartPtr<CSwitch>& dst_sw);

		//find switch Node from one pathlist
        CPathNode*  FindPathNodeBySw(const CSmartPtr<CSwitch>& sw);
		std::list<CPathNode *> CopyPathNodeList();
		std::list<CPathNode *> DeletePathNodeList();
		CPathNodeList*         ClonePathNodeList();
	private:
		 CSmartPtr<CSwitch> m_src_sw;            
    	 CSmartPtr<CSwitch> m_dst_sw;           
    	 //CPathNodeMap m_node_list; 
    	 std::list<CPathNode *> m_node_list;
		
};

#endif

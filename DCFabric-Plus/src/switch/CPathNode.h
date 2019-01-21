
#ifndef _CPATHNODE_H_
#define _CPATHNODE_H_

#include "CSwitch.h"

class CPathNode
{
	public:
		CPathNode(){};
		CPathNode( const CSmartPtr<CSwitch>&  sw, const UINT4 & port_no):m_sw(sw),m_port_no(port_no){};
		~CPathNode(){};
		//set Node
		void setPathNode( const CSmartPtr<CSwitch>&  sw, const UINT4 & port_no){ m_sw = sw; m_port_no = port_no;}
		//get Node switch
		const CSmartPtr<CSwitch>& getPathNodeSw() const {return m_sw;} 
		//get Node switch port No.
		const UINT4& getPathNodePortNo()const { return m_port_no; }
	private:
		CSmartPtr<CSwitch> m_sw; 
    	UINT4 m_port_no;   
};

#endif

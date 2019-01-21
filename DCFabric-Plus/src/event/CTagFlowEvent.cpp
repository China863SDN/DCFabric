
 
#include "CTagFlowEvent.h"
#include "bnc-error.h"
#include "bnc-param.h"

CTagFlowEvent::CTagFlowEvent()
{
	
}

CTagFlowEvent::CTagFlowEvent(INT4 event, INT4 reason, std::list<sw_tagflow_t*> & tagflow_list):
    CEvent(MSG_OPER_PASSON, event, reason)
{
    std::list<sw_tagflow_t*>::iterator iter = tagflow_list.begin();
	while(tagflow_list.end() != iter)
	{
		if(NULL != *iter)
		{
			m_tagflow_list.push_back(*iter);
		}
		iter++;
	}
	tagflow_list.clear();
}

CTagFlowEvent::CTagFlowEvent(INT4 event, INT4 reason, UINT8 srcDpid, UINT4 outport, UINT8 dstDpid, UINT4 inport):
    CEvent(MSG_OPER_PASSON, event, reason),
    m_srcDpid(srcDpid),
    m_outport(outport),
    m_dstDpid(dstDpid),
    m_inport(inport)
{
}

CTagFlowEvent::~CTagFlowEvent()
{
}



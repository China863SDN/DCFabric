

#ifndef __CTAGFLOWEVENT_H
#define __CTAGFLOWEVENT_H

#include "bnc-type.h"
#include "bnc-param.h"
#include "CEvent.h"


class CTagFlowEvent : public CEvent
{
public:
    CTagFlowEvent();
    CTagFlowEvent(INT4 event, INT4 reason, std::list<sw_tagflow_t*> & tagflow_list);
    CTagFlowEvent(INT4 event, INT4 reason, UINT8 srcDpid, UINT4 outport, UINT8 dstDpid, UINT4 inport);
    ~CTagFlowEvent();

    std::list<sw_tagflow_t*> & getTagFlowList()  { return m_tagflow_list; }

    UINT8 getSrcDpid() {return m_srcDpid;}
    UINT4 getOutport() {return m_outport;}
    UINT8 getDstDpid() {return m_dstDpid;}
    UINT4 getInport() {return m_inport;}

private:
    std::list<sw_tagflow_t*> m_tagflow_list;

    UINT8 m_srcDpid; 
    UINT4 m_outport; 
    UINT8 m_dstDpid; 
    UINT4 m_inport;
};

#endif


#ifndef _CTAGFLOWEVENTCONSUM_H
#define _CTAGFLOWEVENTCONSUM_H

#include "bnc-param.h"
#include "CEventNotifier.h"



class CTagFlowEventConsum : public CEventNotifier
{
public:
    CTagFlowEventConsum();
    ~CTagFlowEventConsum();


    INT4 onregister();
    void deregister();

  	INT4 addTagFlow(std::list<sw_tagflow_t*>& tagflow_list);
  	INT4 addTagFlow(UINT8 srcDpid, UINT4 outport, UINT8 dstDpid, UINT4 inport);

    const char* toString() {return "CTagFlowConsume";}

private:
    INT4 consume(CSmartPtr<CMsgCommon> evt);

};

#endif


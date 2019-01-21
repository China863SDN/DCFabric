
#ifndef __CTAGFLOWEVENTREPORTOR_H
#define __CTAGFLOWEVENTREPORTOR_H

#include "bnc-type.h"
#include "bnc-param.h"
#include "CEventReportor.h"

class CTagFlowEventReportor : public CEventReportor
{
public:
    static CTagFlowEventReportor* getInstance();

    ~CTagFlowEventReportor();

    INT4 report(INT4 event, INT4 reason, std::list<sw_tagflow_t*> & tagflow_list);
    INT4 report(INT4 event, INT4 reason, UINT8 srcDpid, UINT4 outport, UINT8 dstDpid, UINT4 inport);

    const char* toString() {return "CTagFlowEventReportor";}

private:
    CTagFlowEventReportor();

    CMsgPath getMsgPath(INT4 event);

private:
    static CTagFlowEventReportor* m_pInstance;       
};

#endif

#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"
#include "CTagFlowEventReportor.h"
#include "CTagFlowEvent.h"

CTagFlowEventReportor* CTagFlowEventReportor::m_pInstance = NULL;       

CTagFlowEventReportor* CTagFlowEventReportor::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CTagFlowEventReportor();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return m_pInstance;
}

CTagFlowEventReportor::CTagFlowEventReportor()
{
}

CTagFlowEventReportor::~CTagFlowEventReportor()
{
}

INT4 CTagFlowEventReportor::report(INT4 event, INT4 reason, std::list<sw_tagflow_t*> & tagflow_list)
{
    if (!((EVENT_TYPE_TAG_FLOW_ADD <= event) && (EVENT_TYPE_TAG_FLOW_ADD >= event)))
        return BNC_ERR;

    CTagFlowEvent* evt = new CTagFlowEvent(event, reason, tagflow_list);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CTagFlowEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);
    
    return CEventReportor::report(evt);
}

INT4 CTagFlowEventReportor::report(INT4 event, INT4 reason, UINT8 srcDpid, UINT4 outport, UINT8 dstDpid, UINT4 inport)
{
    if (!((EVENT_TYPE_TAG_FLOW_ADD <= event) && (EVENT_TYPE_TAG_FLOW_ADD >= event)))
        return BNC_ERR;

    CTagFlowEvent* evt = new CTagFlowEvent(event, reason, srcDpid, outport, dstDpid, inport);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new CTagFlowEvent failed[%d]!", errno);
        return BNC_ERR;
    }

    CMsgPath path = getMsgPath(event);
    evt->setPath(path);
    
    return CEventReportor::report(evt);
}

CMsgPath CTagFlowEventReportor::getMsgPath(INT4 event)
{
    return g_tagflowEventPath[0];
}


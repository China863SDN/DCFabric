
#include "bnc-error.h"
#include "bnc-type.h"
#include "comm-util.h"
#include "log.h"
#include "CFlowMgr.h"
#include "CTagFlowEventConsum.h"
#include "CTagFlowEvent.h"
#include "CTagFlowEventReportor.h"
#include "CControl.h"


CTagFlowEventConsum::CTagFlowEventConsum() 
{
}

CTagFlowEventConsum::~CTagFlowEventConsum() 
{
    deregister();
}


INT4 CTagFlowEventConsum::onregister()
{
    CMsgPath path(g_tagflowEventPath[0]);
    return CEventNotifier::onregister(path);
}

void CTagFlowEventConsum::deregister()
{
    CMsgPath path(g_tagflowEventPath[0]);
    CEventNotifier::deregister(path);
}

INT4 CTagFlowEventConsum::consume(CSmartPtr<CMsgCommon> evt)
{
    INT4 ret = BNC_ERR;
    
    if (evt.isNull())
        return ret;

    CTagFlowEvent* pEvt = (CTagFlowEvent*)evt.getPtr();
    //LOG_WARN(pEvt->getDesc().c_str());
	//LOG_ERROR("***********$########**********************");
    switch (pEvt->getEvent())
    {
        case EVENT_TYPE_TAG_FLOW_ADD:
            //LOG_WARN("EVENT_TYPE_FLOW_TABLE_ADD");
            ret = addTagFlow(pEvt->getTagFlowList());
            //ret = addTagFlow(pEvt->getSrcDpid(), pEvt->getOutport(), pEvt->getDstDpid(), pEvt->getInport());
            break;
        
        default:
            LOG_WARN_FMT("%s consume invalid event[%d] !", toString(), pEvt->getEvent());
            break;
    }

    return ret;
}

INT4 CTagFlowEventConsum::addTagFlow(std::list<sw_tagflow_t*>& tagflow_list )
{
	if(0 == tagflow_list.size())
	{
		return BNC_ERR;
	}
	//LOG_ERROR("***********************************************");
	std::list<sw_tagflow_t*>::iterator iter = tagflow_list.begin();
	for(; tagflow_list.end() != iter; ++iter)
	{
		if(NULL != *iter)
		{
		 	CFlowMgr::getInstance()->install_different_switch_flow((*iter)->dst_sw, (*iter)->src_sw,  (*iter)->outport);
			//LOG_ERROR_FMT("******srcDpid=0x%x dstDpid=0x%x*************************************", (*iter)->src_sw->getDpid(), (*iter)->dst_sw->getDpid());
			delete *iter;
		}
	}
	tagflow_list.clear();
	return BNC_OK;
}

INT4 CTagFlowEventConsum::addTagFlow(UINT8 srcDpid, UINT4 outport, UINT8 dstDpid, UINT4 inport)
{
    CSwitchMgr& swMgr = CControl::getInstance()->getSwitchMgr();
    CSmartPtr<CSwitch> srcSw = swMgr.getSwitchByDpid(srcDpid);
    CSmartPtr<CSwitch> dstSw = swMgr.getSwitchByDpid(dstDpid);
    if (srcSw.isNotNull() && dstSw.isNotNull())
    {
        CFlowMgr::getInstance()->install_different_switch_flow(dstSw, srcSw, outport);
        CFlowMgr::getInstance()->install_different_switch_flow(srcSw, dstSw, inport);
    }

	return BNC_OK;
}

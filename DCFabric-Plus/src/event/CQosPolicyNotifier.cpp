
#include "log.h"
#include "bnc-type.h"
#include "bnc-error.h"
#include "CQosEvent.h"
#include "CQosEventReport.h"
#include "CQosPolicyNotifier.h"

CQosPolicyNotifier* CQosPolicyNotifier::m_pInstance = NULL;

CQosPolicyNotifier::CQosPolicyNotifier()
{
	
}
CQosPolicyNotifier::~CQosPolicyNotifier()
{
	
}
INT4 CQosPolicyNotifier::onregister() 
{
	CMsgPath path(g_qosPolicyEventPath[0]);
	if (CEventNotifier::onregister(path, FALSE))
    {
        LOG_WARN_FMT("CQosPolicyNotifier register to path[%s] failed !", path.c_str());
        CEventNotifier::deregister(path);
        return BNC_ERR;
    }

    return BNC_OK;
}
void CQosPolicyNotifier::deregister() 
{
	CMsgPath path(g_qosPolicyEventPath[0]);
	CEventNotifier::deregister(path);
    return ;
}
INT4 CQosPolicyNotifier::consume(CSmartPtr<CMsgCommon> evt)
{
	if (evt.isNull())
		return BNC_ERR;


	CEvent* pEvt = (CEvent*)evt.getPtr();
	if(NULL == pEvt)
	{
		return BNC_ERR;
	}
	switch(pEvt->getEvent())
	{
		case EVENT_TYPE_QOS_ATTCH:
			break;
		case EVENT_TYPE_QOS_UPDATE:
			break;
		case EVENT_TYPE_QOS_RULE_C:
			break;
		case EVENT_TYPE_QOS_RULE_R:
			break;
		case EVENT_TYPE_QOS_RULE_U:
			break;
		case EVENT_TYPE_QOS_RULE_D:
			break;
		default:
			break;
	}
	

	return BNC_OK;
}

INT4 CQosPolicyNotifier::init()
{
    return onregister();
}

CQosPolicyNotifier* CQosPolicyNotifier::GetInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CQosPolicyNotifier();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
}


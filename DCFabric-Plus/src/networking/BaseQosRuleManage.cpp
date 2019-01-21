#include "bnc-type.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "BaseQosRule.h"
#include "BaseQosRuleManage.h"

BaseQosRuleManage* BaseQosRuleManage::m_pInstance = NULL ;


INT4 BaseQosRuleManage::insertQosRule(BaseQosRule*  qosRule)
{
	if(NULL == qosRule)
	{
		return BNC_ERR;
	}
	BaseQosRule* qosRuleFind = findQosRuleByQosId(qosRule->GetQosId());
	if(NULL == qosRuleFind)
	{
		m_mutex.lock();
		m_qosRuleList.push_back(qosRule);
		m_mutex.unlock();
	}
	else
	{
		m_mutex.lock();
		qosRuleFind->SetObjectValue(qosRule);
		m_mutex.unlock();
	}
	return BNC_OK;
}
INT4 BaseQosRuleManage::deleteQosRule(const std::string & qos_id)
{
	m_mutex.lock();
    STL_FOR_LOOP(m_qosRuleList, iter)
    {
        if ((*iter)&&(qos_id == (*iter)->GetQosId()))
        {
           m_qosRuleList.erase(iter);
		   m_mutex.unlock();
           return BNC_ERR;
        }
    }
	m_mutex.unlock();
	return BNC_OK;
}
INT4 BaseQosRuleManage::updateQosRule(BaseQosRule* qosRule)
{
	if(NULL == qosRule)
	{
		return BNC_ERR;
	}
	BaseQosRule* qosRuleFind = findQosRuleByQosId(qosRule->GetQosId());
	if(NULL == qosRuleFind)
	{
		return BNC_ERR;
	}
	qosRuleFind->SetObjectValue(qosRule);
	return BNC_OK;
}
BaseQosRule* BaseQosRuleManage::findQosRuleByQosId(const std::string & qos_id)
{
	m_mutex.lock();
    STL_FOR_LOOP(m_qosRuleList, iter)
    {
        if ((*iter)&&(qos_id == (*iter)->GetQosId()))
        {
		   m_mutex.unlock();
           return (*iter);
        }
    }
	m_mutex.unlock();
	return NULL;
}


BaseQosRuleManage* BaseQosRuleManage::Get_Instance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new BaseQosRuleManage();
		if(NULL == m_pInstance)
		{
			return m_pInstance;
		}
	}
	return m_pInstance;
}


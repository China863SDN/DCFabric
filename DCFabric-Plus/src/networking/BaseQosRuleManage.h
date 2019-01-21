


#ifndef _BASEQOSRULEMANAGE_H_
#define _BASEQOSRULEMANAGE_H_

#include "bnc-type.h"
#include "CMutex.h"
class BaseQosRuleManage
{

	public:
		INT4 		 insertQosRule(BaseQosRule*  qosRule);
		INT4		 deleteQosRule(const std::string & qos_id);
		INT4 		 updateQosRule(BaseQosRule* qosRule);
		BaseQosRule* findQosRuleByQosId(const std::string & qos_id);   
		static BaseQosRuleManage* Get_Instance();
		
	private:
		BaseQosRuleManage(){};
		~BaseQosRuleManage(){}
	private:
		CMutex					  m_mutex;
		std::list<BaseQosRule*>   m_qosRuleList;
		static BaseQosRuleManage* m_pInstance;
		
};

#endif
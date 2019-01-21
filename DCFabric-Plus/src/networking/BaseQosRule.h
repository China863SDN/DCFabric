
#ifndef _BASEQOSRULE_H_
#define _BASEQOSRULE_H_

class BaseQosRule
{
	public:
		BaseQosRule(){};
		BaseQosRule(const std::string & qos_id, const std::string & tenant_id, const std::string & protocol, UINT8 max_rate, BOOL bIngress )
			: m_qosid(qos_id),m_tenant_id(tenant_id), m_protocol(protocol), m_max_rate(max_rate), m_bIngress(bIngress){};
		~BaseQosRule(){};

	public:
		void  SetQosId(const std::string & qos_id) {m_qosid = qos_id; }
		const std::string & GetQosId() const { return m_qosid; }


		void  SetTenantId(const std::string & tenant_id){m_tenant_id = tenant_id; }
		const std::string & GetTenantId() const {return m_tenant_id; }

		void  SetProtocol(const std::string & proto) {m_protocol = proto; }
		const std::string & GetProtocol() const {return m_protocol; }

		void  SetMaxRate(UINT8 max_rate) {m_max_rate = max_rate; }
		UINT8 GetMaxRate() { return m_max_rate; }

		void  SetDerection(BOOL bIngress) {m_bIngress = bIngress; }
		BOOL  GetDerection(){return m_bIngress; }

		INT4  SetObjectValue(BaseQosRule* qosRule)
		{
			if(NULL == qosRule)
				return BNC_ERR;
			m_qosid = qosRule->GetQosId();
			m_tenant_id = qosRule->GetTenantId();
			m_protocol = qosRule->GetProtocol();
			m_max_rate = qosRule->GetMaxRate();
			m_bIngress = qosRule->GetDerection();
			return BNC_OK;
		}
		
	private:
		std::string  m_qosid;
		std::string  m_tenant_id;
		std::string  m_protocol;
		UINT8        m_max_rate;
		BOOL		 m_bIngress;
		
};
#endif

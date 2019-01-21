
#ifndef _CNETWORK_H_
#define  _CNETWORK_H_

class CNetwork
{
	public:
		CNetwork();
		CNetwork(const std::string& networkid, const std::string& tenantid, BOOL bexternal , BOOL bshared);
		~CNetwork();

		void 	set_networkid(const std::string& networkid){m_networkid = networkid;}
		void 	set_tenantid(const std::string& tenantid){ m_tenantid = tenantid;}
		void 	set_external(BOOL bexternal ){m_external = bexternal; }
		void    set_shared(BOOL bshared){m_shared = bshared; }

		const std::string& get_networkid() const {return m_networkid; }
		const std::string get_tenantid() const {return m_tenantid; }
		BOOL       get_shared(){ return m_shared; }
		BOOL       get_external(){ return m_external; }

		void 	set_checkstatus(UINT1 status){m_checkstatus = status;}
		UINT1 	get_checkstatus() const{return m_checkstatus; }		


	private:
		std::string m_networkid;
		std::string m_tenantid;
		BOOL m_shared;
		BOOL m_external;
		UINT1 m_checkstatus;
};

#endif

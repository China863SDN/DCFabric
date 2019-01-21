#ifndef _CFLOATINGIP_H_
#define  _CFLOATINGIP_H_

class CFloatingIp
{
	public:
		CFloatingIp();
		~CFloatingIp() ;
		CFloatingIp(std::string networkid, UINT4 fixedip, UINT4 floatingip):m_networkid(networkid), m_fixedIp(fixedip), m_floatingIp(floatingip),m_checkstatus(0){};
		
	public:
		const std::string& getNetworkid() const {return m_networkid; }
		void setFixedIp(UINT4 fixedip){m_fixedIp = fixedip; }
		UINT4 getFixedIp(){ return m_fixedIp; }
		void  setFloatingIp(UINT4 floatingip) { m_floatingIp = floatingip; }
		UINT4 getFloatingIp(){ return m_floatingIp;};

		void  set_checkstatus(UINT1 status){ m_checkstatus = status; }
		UINT1 get_checkstatus(){return  m_checkstatus; }

		void  set_flowinstalled(INT4 flowinstalled){m_flowinstalled = flowinstalled; }
		INT4  get_flowinstalled(){return m_flowinstalled; }

		
		

	private:
		UINT4  m_fixedIp;
		UINT4  m_floatingIp;
		std::string m_networkid;
		UINT1  m_checkstatus;
		INT4   m_flowinstalled;
		
};
#endif
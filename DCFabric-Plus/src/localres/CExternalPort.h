#ifndef _CEXTERNALPORT_H_
#define  _CEXTERNALPORT_H_

class  CExternalPort
{
	private:
		CExternalPort();
		~CExternalPort();

	public:
		void  set_subnetid(const std::string& subnetid){m_subnetid = subnetid;}
		std::string get_subnetid() const {return m_subnetid; }

		UINT4 get_external_gatewayIp() const {return m_external_gateway_ip; }
		UINT4 get_external_outerInterfaceIp() const {return m_external_outerinterface_ip; }
		
		void  set_checkstatus(UINT1 status) { m_checkstatus = status; }
		UINT1 get_checkstatus() const { return m_checkstatus; }
	private:
		std::string m_subnetid;
		UINT4       m_external_gateway_ip;
		UINT4       m_external_outerinterface_ip;
		UINT1       m_external_gateway_mac[6];

		UINT8       m_external_dpid;
		UINT4       m_external_port;
		UINT1       m_checkstatus;

		
};
#endif
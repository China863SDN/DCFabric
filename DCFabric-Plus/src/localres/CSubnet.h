
#ifndef _CSUBNET_H_
#define  _CSUBNET_H_

class CSubnet
{
	public:
		CSubnet();
		CSubnet(const std::string& networkid, const std::string& subnetid , UINT4 dhcpIp, UINT4 gatewayIp);
		~CSubnet();
	
		void 	set_networkid(const std::string& networkid){ m_networkid = networkid;}
		void 	set_subnetid(const std::string& subnetid){ m_subnetid = subnetid;}
		void 	set_tenantid(const std::string& tenantid){ m_tenantid = tenantid;}
		void    set_cidr(const std::string& cidr){m_cidr = cidr;}
		void    set_subnetIpRange(UINT4 startIp, UINT4 endIp){m_startIp = startIp; m_endIp = endIp;}


		void    set_subnetGateway(UINT4 gatewayIp){m_gatewayIp = gatewayIp; }
		
		const std::string& get_networkid() const {return m_networkid; }
		const std::string& get_subnetid() const {return m_subnetid; }
		const std::string& get_tenantid() const{return m_tenantid; }
		const std::string& get_cidr() const {return m_cidr;}

		UINT4   get_subnetStartIp() const {return m_startIp; }
		UINT4   get_subnetEndIp() const {return m_endIp; }
		UINT4   get_subnetGateway() const {return m_gatewayIp; }
		void 	set_checkstatus(UINT1 status){m_checkstatus = status;}
		UINT1 	get_checkstatus() const{return m_checkstatus; }
		
		BOOL    isInSubnetRangeByIp(UINT4 ip);

	private:
		std::string m_networkid;
		std::string m_subnetid;
		std::string m_tenantid;
		std::string m_cidr;
		UINT4 m_startIp;
		UINT4 m_endIp;
		UINT4 m_gatewayIp;
		UINT1 m_checkstatus;
};

#endif


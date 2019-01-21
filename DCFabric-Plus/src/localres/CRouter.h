#ifndef 	_CROUTER_H_
#define     _CROUTER_H_


class   CRouter
{
	public:
		CRouter():m_routerip(0),m_checkstatus(0) {};
		CRouter(const std::string& deviceid, const std::string& networkid, const std::string& subnetid, UINT4 ip, UINT1 checkstatus)
		:m_deviceid(deviceid), m_networkid(networkid),m_subnetid(subnetid), m_routerip(ip), m_checkstatus(checkstatus){}
		
		~CRouter();
		void  setNetworkid(const std::string & networkid){m_networkid = networkid; }
		void  setSubnetid(const std::string & subnetid){m_subnetid = subnetid; }
		void  setDeviceid(const std::string& deviceid){ m_deviceid = deviceid;}
		
		const std::string& getNetworkid() const {return m_networkid; }
		const std::string& getSubnetid() const { return m_subnetid; }
		const std::string& getDeviceid() const {return m_deviceid; } 
		
		void  setRouterIp(UINT4 ip){m_routerip = ip;}
		UINT4  getRouterIp() const {return m_routerip; }
		
		void  setStatus(UINT1 status)  {m_checkstatus = status; }
		UINT1 getStatus() const {return m_checkstatus; }
		
		
	private:
		std::string  m_deviceid;
		std::string  m_networkid;
		std::string  m_subnetid;
		UINT4  m_routerip;
		UINT1  m_checkstatus;

};
#endif

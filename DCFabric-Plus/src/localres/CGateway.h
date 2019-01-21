#ifndef  _CGATEWAY_H_
#define  _CGATEWAY_H_
class   CGateway
{
	public:
		CGateway():m_gatewayip(0),m_checkstatus(0){};
		CGateway(const std::string& deviceid, const std::string& networkid, const std::string& subnetid, UINT4 ip, UINT1 checkstatus)
		:m_deviceid(deviceid),m_networkid(networkid),m_subnetid(subnetid),m_gatewayip(ip),m_checkstatus(checkstatus){ }

		~CGateway(){};
		void  setNetworkid(const std::string & networkid){m_networkid = networkid; }
		void  setSubnetid(const std::string & subnetid){m_subnetid = subnetid; }
		void  setDeviceid(const std::string & deviceid){m_deviceid = deviceid; }
		
		const std::string&  getNetworkid() const {return m_networkid; }
		const std::string&  getSubnetid() const { return m_subnetid; }
		const std::string&  getDeviceid()const {return m_deviceid; }
		void  setGatewayIp(UINT4 ip){m_gatewayip = ip;}
		UINT4  getGatewayIp() const {return m_gatewayip; }

		void  setStatus(UINT1 status){m_checkstatus = status;}
		UINT1 getStatus() const {return m_checkstatus; }
		
	private:
		std::string  m_deviceid;
		std::string  m_networkid;
		std::string  m_subnetid;
		UINT4		 m_gatewayip;
		UINT1        m_checkstatus;
};

#endif


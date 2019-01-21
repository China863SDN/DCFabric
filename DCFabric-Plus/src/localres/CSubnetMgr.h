
#ifndef CSUBNETMGR_H_
#define  CSUBNETMGR_H_

typedef 	std::map<std::string, CSubnet*>   CSubnetMap;

class CSubnetMgr
{
	private:
		CSubnetMgr();
	public:
		~CSubnetMgr();
		static CSubnetMgr* getInstance();
		BOOL addSubnetNode(CSubnet* subnetNode);
		BOOL delSubnetNode(const std::string& subnetid);
		BOOL updateSubnetNode(CSubnet* subnetNode);
		std::string findNetworkIdById(const std::string& subnetid) ;
		
		UINT4 findSubnetDhcpIpById(const std::string& subnetid) ;
		UINT4 findSubnetGatewayIpById(const std::string& subnetid) ;
		CSubnet* findSubnetById(const std::string& subnetid);

		CSubnet* findSubnetByIp(UINT4 ip);
		CSubnet* findSubnetByIp(UINT4 ip, UINT2*  count);
		
		CSubnetMap& getSubnetListMapHead();
	private:
		static CSubnetMgr* m_pInstance;
		CSubnetMap    m_pSubnetList;

	
};


#endif



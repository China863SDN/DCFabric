
#ifndef CNETWORKMGR_H_
#define  CNETWORKMGR_H_

typedef  std::map<std::string, CNetwork*>   CNetworkMap;

class CNetworkMgr
{
	public:
		~CNetworkMgr();
		static CNetworkMgr* getInstance();

	public:
		BOOL addNetworkNode(CNetwork* networkNode);
		BOOL delNetworkNode(const std::string& networkid);
		BOOL updateNetworkNode(CNetwork* networkNode);
		CNetwork* findNetworkById(const std::string& networkid);
		
		std::map<std::string, CNetwork* > getNetworkListMapHead();
	private:
		CNetworkMgr();
		void init() { }
		static CNetworkMgr* m_pInstance;
		CNetworkMap    m_pNetworkList;

	
};


#endif



#ifndef  CEXTERNALPORTMGR_H_
#define   CEXTERNALPORTMGR_H_

class CExternalPortMgr
{
	public:
		~CExternalPortMgr();
		static CExternalPortMgr*   getInstance();
	public:
		BOOL  addExternalPortNode(CExternalPort* externalportNode);
		BOOL  delExternalPortNode(std::string subnetid);
		BOOL  delExternalPortNode(UINT4 outerinterfaceIp);
		CExternalPort* findExternalPortBySubnetId(std::string subnetid);
		CExternalPort* findExternalPortByOuterInterfaceIp(UINT4 outerinterfaceIp);
		CExternalPort* findExternalPortByGatewayIp(UINT4 gatewayIp);
	private:
		CExternalPortMgr();
		void  init(){}
		static   CExternalPortMgr*   m_pInstance;
		std::map<std::string, CExternalPort* >     m_pExternalPortList;
		
};
#endif

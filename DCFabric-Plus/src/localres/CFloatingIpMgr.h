
#ifndef CFLOATINGIPMGR_H_
#define  CFLOATINGIPMGR_H_

typedef   std::map<UINT4, CFloatingIp* >  CFloatingIpMap;

class CFloatingIpMgr
{
	private:
		CFloatingIpMgr();
	public:
		~CFloatingIpMgr();
		static CFloatingIpMgr* getInstance();
		BOOL addFloatingIpNode(CFloatingIp* floatingipNode);
		BOOL delFloatingIpNode(UINT4 uifloatingip);
		CFloatingIp* findFloatingIpNodeByfixedIp(UINT4 uifixedip);
		
		CFloatingIp* findFloatingIpNodeByfloatingIp(UINT4 uifloatingip);
		INT4 judge_sw_in_use(UINT4 fix_ip);
		CFloatingIpMap& getFloatingIpListMapHeader();
		
	private:
		void init() {} ;
		static CFloatingIpMgr*  m_pInstance;

		CFloatingIpMap  m_pFloatingIpList;
};
#endif

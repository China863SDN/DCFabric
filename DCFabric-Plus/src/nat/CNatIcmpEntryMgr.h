
#ifndef  _CNATICMPENTRYMGR_H_
#define  _CNATICMPENTRYMGR_H_

#include "CNatIcmpEntry.h"

typedef std::map<UINT2/*identifier*/, CNatIcmpEntry*> CNatIcmpMap;

class  CNatIcmpEntryMgr
{
	public:
		~CNatIcmpEntryMgr();
		CNatIcmpEntry* AddNatIcmpEntry(UINT2 identifier,UINT4 host_ip,UINT1* host_mac,UINT8 sw_dpid,UINT4 inport);
		CNatIcmpEntry* findNatIcmpEntryByIp(UINT4 hostip);
		CNatIcmpEntry* findNatIcmpEntryByMac(UINT1* hostmac);
		CNatIcmpEntry* findNatIcmpEntryByIdentifier(UINT2 identifier);
		CNatIcmpMap&   getNatIcmpMapHead(){return m_naticmpentrymap;}
		static CNatIcmpEntryMgr*  getInstance();
	private:
		CNatIcmpEntryMgr();

	private:
		CNatIcmpMap m_naticmpentrymap;
		static CNatIcmpEntryMgr*  m_pInstance;
};
#endif

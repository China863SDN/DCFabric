#ifndef   _CSNATSTREAMMGR_H_
#define   _CSNATSTREAMMGR_H_

#include "CSNatStream.h"

typedef std::list<CSNatStream*> CNatStreamList;

class CSNatStreamMgr
{
	public:
		INT4 AddSNatStreamList(CSNatStream* natconn);
		INT4 AddSNatStream(CSNatStream* natconn);
		INT4 RemoveSNatStreamList();
		INT4 RemoveSNatStreamByExternalPortNo(UINT2 external_port_no);
		INT4 RemoveSNatStreamByMacAndPort(UINT1* internal_mac, UINT2 internal_port_no);
		CSNatStream* findSNatStreamByExternalPortNo(UINT2 external_port_no);
		CSNatStream* findSNatStreamByInternalIp(UINT4 external_ip);
		CSNatStream* findSNatStreamByIpAndPort(UINT4 external_ip, UINT2 internal_port_no);
		CSNatStream* findSNatStreamByMacAndPort(UINT1* internal_mac, UINT2 internal_port_no);
		CNatStreamList& getNatStreamHead(){return m_natconnlist;}
	public:
		CSNatStreamMgr();
		~CSNatStreamMgr();
	private:
		CNatStreamList m_natconnlist;
		CMutex         m_mutex;
};

#endif

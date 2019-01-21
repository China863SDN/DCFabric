
#include  "bnc-type.h"
#include  "bnc-error.h"
#include  "bnc-param.h"
#include  "comm-util.h"
#include  "CNatIcmpEntry.h"
#include  "CNatIcmpEntryMgr.h"
#include  "log.h"

CNatIcmpEntryMgr* CNatIcmpEntryMgr::m_pInstance = NULL;

CNatIcmpEntryMgr::CNatIcmpEntryMgr()
{
	
}
CNatIcmpEntryMgr::~CNatIcmpEntryMgr()
{
	if(NULL != m_pInstance)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

CNatIcmpEntry*  CNatIcmpEntryMgr::AddNatIcmpEntry(UINT2 identifier,UINT4 host_ip,UINT1* host_mac,UINT8 sw_dpid,UINT4 inport)
{
	CNatIcmpEntry* ret(NULL);
	ret = findNatIcmpEntryByIdentifier(identifier);
	if(NULL == ret)
	{
        INT1 macStr[20] = {0};
        mac2str(host_mac, macStr);
        INT1 ipStr[20] = {0};
        number2ip(host_ip, ipStr);
        LOG_WARN_FMT("create nat icmp: identifier[%d], hostIp[%s], hostMac[%s], dpid[0x%llx], port[%u]", 
            identifier, ipStr, macStr, sw_dpid, inport);

		ret = new CNatIcmpEntry(identifier, host_ip, host_mac, sw_dpid, inport);
		m_naticmpentrymap.insert(std::make_pair(identifier, ret));
	}
	else
	{
		ret->Update( identifier, host_ip, host_mac, sw_dpid, inport);
	}
	return ret;
}

CNatIcmpEntry* CNatIcmpEntryMgr::findNatIcmpEntryByIp(UINT4 hostip)
{
	CNatIcmpEntry* ret(NULL);
	STL_FOR_LOOP(m_naticmpentrymap,iter)
	{
		if(iter->second&&(hostip == iter->second->getIp()))
		{
			ret = iter->second;
			break;
		}
	}
	return ret;
}
CNatIcmpEntry* CNatIcmpEntryMgr::findNatIcmpEntryByMac(UINT1* hostmac)
{
	CNatIcmpEntry* ret(NULL);
	STL_FOR_LOOP(m_naticmpentrymap,iter)
	{
		if(iter->second&&(0 == memcmp((const void *)hostmac , (const void *)(iter->second->getMac()), MAC_LEN)))
		{
			ret = iter->second;
			break;
		}
	}
	return ret;
}
CNatIcmpEntry* CNatIcmpEntryMgr::findNatIcmpEntryByIdentifier(UINT2 identifier)
{
	CNatIcmpMap::iterator iter = m_naticmpentrymap.find(identifier);
	if(m_naticmpentrymap.end() == iter)
	{
		return NULL;
	}
	return iter->second;
}
CNatIcmpEntryMgr*  CNatIcmpEntryMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CNatIcmpEntryMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
}


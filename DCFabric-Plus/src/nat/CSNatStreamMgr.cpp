#include  "log.h"
#include  "bnc-type.h"
#include  "bnc-error.h"
#include  "bnc-param.h"
#include  "comm-util.h"
#include  "CSNatStream.h"
#include  "CSNatStreamMgr.h"


CSNatStreamMgr::CSNatStreamMgr()
{
	
}
CSNatStreamMgr::~CSNatStreamMgr()
{
	
}

INT4   CSNatStreamMgr::AddSNatStreamList(CSNatStream*  natconn)
{
	if(NULL == natconn)
	{
		return BNC_ERR;
	}
	UINT2 current_min_port_no = m_natconnlist.size();
	LOG_ERROR_FMT("current_min_port_no= %d",current_min_port_no);
	CNatStreamList::iterator  iter = m_natconnlist.begin();
	while(m_natconnlist.end() != iter)
	{
		if((*iter)->getExternalPortNo() > current_min_port_no +1 )
			break;
		else
		{
			current_min_port_no = (*iter)->getExternalPortNo();
		}
		iter++;
	}
	if(0 == current_min_port_no)
	{
		current_min_port_no = NAT_PORT_MIN_VALUE - 1;
	}
	if(current_min_port_no >= NAT_PORT_MAX_VALUE)
	{
		LOG_ERROR("current_min_port_no excced max range!!!");
		return BNC_ERR;
	}
	LOG_ERROR_FMT("current_min_port_no= %d",current_min_port_no);
	natconn->setExternalPortNo(current_min_port_no+1);
	m_mutex.lock();
	m_natconnlist.insert(iter, natconn);
	m_mutex.unlock();
	LOG_WARN_FMT("m_natconnlist size: %lu",m_natconnlist.size());
	return BNC_OK;
}

INT4   CSNatStreamMgr::AddSNatStream(CSNatStream* natconn)
{
	if (NULL == natconn)
		return BNC_ERR;

    BOOL found = FALSE;

	CNatStreamList::iterator iter = m_natconnlist.begin();
	for (; iter != m_natconnlist.end(); ++iter)
	{
		if ((*iter)->getExternalPortNo() == natconn->getExternalPortNo())
        {
            found = TRUE;
            break;
        }
		if ((*iter)->getExternalPortNo() > natconn->getExternalPortNo())
			break;
	}

	m_mutex.lock();
    if (iter == m_natconnlist.end())
    {
        m_natconnlist.push_back(natconn);
    }
    else
    {
        if (found)
        {
            delete *iter;
            *iter = natconn;
        }
        else
        {
            m_natconnlist.insert(iter, natconn);
        }
    }
	m_mutex.unlock();

	return BNC_OK;
}
INT4   CSNatStreamMgr::RemoveSNatStreamList()
{
	CNatStreamList::iterator iter;
	m_mutex.lock();
	for(iter = m_natconnlist.begin(); iter != m_natconnlist.end(); )
	{
	
		iter = m_natconnlist.erase(iter);

	}
	
	m_mutex.unlock();
	return BNC_OK;
}
INT4 CSNatStreamMgr::RemoveSNatStreamByExternalPortNo(UINT2 external_port_no)
{
	INT4 ret = BNC_ERR;
	m_mutex.lock();
	STL_FOR_LOOP(m_natconnlist,iter)
	{
		if(*iter&&((*iter)->getExternalPortNo() == external_port_no))
		{
			m_natconnlist.erase(iter);
			ret = BNC_OK;
			break;
		}
		
	}
	m_mutex.unlock();
	return ret;
}
INT4 CSNatStreamMgr::RemoveSNatStreamByMacAndPort(UINT1* internal_mac, UINT2 internal_port_no)
{
	INT4 ret = BNC_ERR;
	m_mutex.lock();
	STL_FOR_LOOP(m_natconnlist,iter)
	{
		
		if(*iter&&((*iter)->getInternalPortNo() == internal_port_no)&&(0 == memcmp((*iter)->getInternalMac(),internal_mac, MAC_LEN)))		
		{
			m_natconnlist.erase(iter);
			ret = BNC_OK;
			break;
		}
		
	}
	m_mutex.unlock();
	return ret;
}


CSNatStream* CSNatStreamMgr::findSNatStreamByExternalPortNo(UINT2 external_port_no)
{
	CSNatStream* ret(NULL);
	
	m_mutex.lock();
	STL_FOR_LOOP(m_natconnlist,iter)
	{
		if(*iter&&((*iter)->getExternalPortNo() == external_port_no))
		{
			ret = *iter;
			break;
		}
	}
	
	m_mutex.unlock();
	return ret;
}
CSNatStream* CSNatStreamMgr::findSNatStreamByInternalIp(UINT4 external_ip)
{
	CSNatStream* ret(NULL);
	
	m_mutex.lock();
	STL_FOR_LOOP(m_natconnlist,iter)
	{
		if(*iter&&((*iter)->getExternalIp() == external_ip))
		{
			ret = *iter;
			break;
		}
	}
	
	m_mutex.unlock();
	return ret;
}
CSNatStream* CSNatStreamMgr::findSNatStreamByIpAndPort(UINT4 external_ip, UINT2 internal_port_no)
{
	CSNatStream* ret(NULL);
	
	m_mutex.lock();
	STL_FOR_LOOP(m_natconnlist,iter)
	{
		if(*iter&&((*iter)->getExternalIp() == external_ip)&&((*iter)->getInternalPortNo() == internal_port_no))
		{
			ret = *iter;
			break;
		}
	}
	
	m_mutex.unlock();
	return ret;
}

CSNatStream* CSNatStreamMgr::findSNatStreamByMacAndPort(UINT1* internal_mac, UINT2 internal_port_no)
{
	CSNatStream* ret(NULL);
	
	m_mutex.lock();
	STL_FOR_LOOP(m_natconnlist,iter)
	{
		if(*iter&&((*iter)->getInternalPortNo() == internal_port_no)&&(0 == memcmp((*iter)->getInternalMac(),internal_mac, MAC_LEN)))
		{
			ret = *iter;
			break;
		}
	}
	
	m_mutex.unlock();
	return ret;
}





#include "bnc-type.h"
#include "comm-util.h"
#include "CExternalPort.h"
#include "CExternalPortMgr.h"

CExternalPortMgr* CExternalPortMgr::m_pInstance = NULL;

CExternalPortMgr::CExternalPortMgr()
{
	
}

CExternalPortMgr::~CExternalPortMgr()
{
	if(NULL != m_pInstance)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}

CExternalPortMgr* CExternalPortMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CExternalPortMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
		m_pInstance->init();
	}
	return m_pInstance;
}

BOOL CExternalPortMgr::addExternalPortNode(CExternalPort * externalportNode)
{
	std::pair< std::map<std::string,CExternalPort* >::iterator,bool > ret;
	ret = m_pExternalPortList.insert(std::make_pair(externalportNode->get_subnetid(), externalportNode));

	return ret.second;
}

BOOL CExternalPortMgr::delExternalPortNode(std::string subnetid)
{
	m_pExternalPortList.erase(subnetid);
	return TRUE;
}
BOOL  CExternalPortMgr::delExternalPortNode(UINT4 outerinterfaceIp)
{
	STL_FOR_LOOP(m_pExternalPortList, iter)
    {
        if (outerinterfaceIp == iter->second->get_external_outerInterfaceIp())
        {
            m_pExternalPortList.erase(iter++);
			return TRUE;
        }
		else
		{
			iter++;
		}

    }
    return FALSE;
}

CExternalPort* CExternalPortMgr::findExternalPortBySubnetId(std::string subnetid)
{
	
	std::map<std::string,CExternalPort* >::iterator iter;
	iter = m_pExternalPortList.find(subnetid);

	if(m_pExternalPortList.end() != iter)
	{
		return iter->second;
	}
	
	return NULL;
}

CExternalPort* CExternalPortMgr::findExternalPortByOuterInterfaceIp(UINT4 outerinterfaceIp)
{
	STL_FOR_LOOP(m_pExternalPortList, iter)
	{
		if(outerinterfaceIp == iter->second->get_external_outerInterfaceIp())
		{
			return iter->second;
		}
	}
	return NULL;
}
CExternalPort* CExternalPortMgr::findExternalPortByGatewayIp(UINT4 gatewayIp)
{
	STL_FOR_LOOP(m_pExternalPortList, iter)
	{
		if(gatewayIp == iter->second->get_external_gatewayIp())
		{
			return iter->second;
		}
	}
	return NULL;
}


#include "log.h"
#include "bnc-error.h"
#include "CProxyRes.h"
#include "COpenstackMgr.h"

CProxyResMgr* CProxyResMgr::m_pInstance = NULL;

CProxyResMgr::CProxyResMgr()
{
	
}

CProxyResMgr::~CProxyResMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

 CProxyResMgr*  CProxyResMgr::getInstance()
 {
 	if(NULL == m_pInstance)
 	{
		m_pInstance = new CProxyResMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
 }

 CResourceMgr* CProxyResMgr::getResource( CResourceMgr* resource)
{
	if(NULL == resource)
	{
		return NULL;
	}
	m_pResource = resource;
	return m_pResource;
}


INT4 CProxyResMgr::init()
{
	if (NULL == getResource(COpenstackMgr::getInstance()))
    {
        return BNC_ERR;
    }
	return BNC_OK;
}

void CProxyResMgr::PrintString()
{
	LOG_INFO("CProxyResMgr");
}



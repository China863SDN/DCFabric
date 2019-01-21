#include "CRestDefine.h"
#include "comm-util.h"
#include "log.h"

CRestDefine* CRestDefine::m_pInstance = 0;

CRestDefine::CRestDefine()
{
}

CRestDefine::~CRestDefine()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

bnc::restful::http_method CRestDefine::getMethodFromStr(const std::string & raw)
{
	STL_FOR_LOOP(m_methodList, iter)
	{
		if (std::string::npos != raw.find(iter->second))
		{
			return iter->first;
		}
	}

	return (bnc::restful::HTTP_METHOD_OTHER);
}

bnc::restful::http_version CRestDefine::getVersionFromStr(const std::string & raw)
{
	STL_FOR_LOOP(m_versionList, iter)
	{
		if (std::string::npos != raw.find(iter->second))
		{
			return iter->first;
		}
	}

	return (bnc::restful::HTTP_VERSION_OHTER);
}

bnc::restful::http_status CRestDefine::getStatusFromStr(const std::string & raw)
{
    STL_FOR_LOOP(m_statusList, iter)
    {
        if (std::string::npos != raw.find(iter->second))
        {
            return iter->first;
        }
    }

    return (bnc::restful::STATUS_OTHER);
}

void CRestDefine::getStrFromMethod(bnc::restful::http_method method, std::string & str_method)
{
	method_map::iterator iter = m_methodList.find(method);

	if (m_methodList.end() != iter)
	{
		str_method = iter->second;
	}
}

void CRestDefine::getStrFromVersion(bnc::restful::http_version version, std::string & str_version)
{
	version_map::iterator iter = m_versionList.find(version);

	if (m_versionList.end() != iter)
	{
		str_version = iter->second;
	}
}

void CRestDefine::getStrFromStatus(bnc::restful::http_status status, std::string & str_status)
{
	status_map::iterator iter = m_statusList.find(status);

	if (m_statusList.end() != iter)
	{
		str_status = iter->second;
	}
}

void CRestDefine::getStrFromContentType(bnc::restful::content_type type, std::string & str_type)
{
    content_type_map::iterator iter = m_contenttypeList.find(type);

    if (m_contenttypeList.end() != iter)
    {
        str_type = iter->second;
    }
}

CRestDefine* CRestDefine::getInstance()
{
	if (NULL == m_pInstance) {
		LOG_INFO("initialize Rest Define.");
		m_pInstance = new CRestDefine();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
		m_pInstance->init();
	}

	return m_pInstance;
}

void CRestDefine::init()
{
	// 初始化时, 将enum和对应字符串存入list中, 以便于后续的查找对应关系
	m_methodList.insert(method_map::value_type (bnc::restful::HTTP_GET, "GET"));
	m_methodList.insert(method_map::value_type (bnc::restful::HTTP_POST, "POST"));
	m_methodList.insert(method_map::value_type (bnc::restful::HTTP_PUT, "PUT"));
	m_methodList.insert(method_map::value_type (bnc::restful::HTTP_DELETE, "DELETE"));

	m_versionList.insert(version_map::value_type (bnc::restful::HTTP_1_0, "HTTP/1.0"));
	m_versionList.insert(version_map::value_type (bnc::restful::HTTP_1_1, "HTTP/1.1"));

	m_statusList.insert(status_map::value_type (bnc::restful::STATUS_OK, "200 OK"));
	m_statusList.insert(status_map::value_type (bnc::restful::STATUS_BAD_REQUEST, "400 BAD REQUEST"));
	m_statusList.insert(status_map::value_type (bnc::restful::STATUS_UNAUTHORIZED, "401 UNAUTHORIZED"));
	m_statusList.insert(status_map::value_type (bnc::restful::STATUS_FORBIDDEN, "403 FORBIDDEN"));
	m_statusList.insert(status_map::value_type (bnc::restful::STATUS_NOT_FOUND, "404 NOT FOUND"));

	m_contenttypeList.insert(content_type_map::value_type (bnc::restful::CONTENT_JSON, "application/json"));
}





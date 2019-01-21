#include "log.h"
#include "bnc-type.h"
#include "bnc-error.h"
#include "bnc-param.h"
#include "comm-util.h"
#include "CSNatConnMgr.h"
#include "CRouterGateMgr.h"
#include "CHostMgr.h"
#include "CClusterSync.h"

#define SNAT_PORT_MIN   49200
#define SNAT_PORT_MAX   59200

CSNatConnMgr* CSNatConnMgr::m_instance = NULL;

CSNatConnMgr* CSNatConnMgr::getInstance()
{
	if (NULL == m_instance)
	{
		m_instance = new CSNatConnMgr();
		if (NULL == m_instance)
		{
			exit(-1);
		}
	}
	return m_instance;
}

CSNatConnMgr::CSNatConnMgr()
{
}

CSNatConnMgr::~CSNatConnMgr()
{
}

void CSNatConnMgr::clear()
{
    m_exts.clear();
}

CSmartPtr<CSNatConn> CSNatConnMgr::createSNatConn(UINT4 external_ip, 
                                                  UINT4 host_ip, 
                                                  UINT2 internal_port,
                                                  UINT1 proto, 
                                                  const UINT1* internal_mac, 
                                                  UINT8 gateway_dpid, 
                                                  UINT8 switch_dpid)
{
    CSmartPtr<CSNatConn> ret(NULL);

    INT1 macStr[48] = {0};
    mac2str(internal_mac, macStr);
    INT1 extIpStr[20] = {0}, hostIpStr[20] = {0};
    number2ip(external_ip, extIpStr);
    number2ip(host_ip, hostIpStr);

    CSNatConnsExtMap::iterator extIt = m_exts.find(external_ip);
    if (extIt == m_exts.end())
    {
        std::pair<CSNatConnsExtMap::iterator, bool> result =
            m_exts.insert(std::make_pair(external_ip, CSNatConnsExt()));
        if (!result.second)
        {
            LOG_WARN_FMT("SNAT: insert CSNatConnsExt for external[%s] failed !", extIpStr);
            return ret;
        }
        extIt = result.first;
    }

    CSNatConnsIpMap& hostMap = (IPPROTO_TCP == proto) ? extIt->second.m_tcpHosts : extIt->second.m_udpHosts;
    CSNatConnsIpMap::iterator hostIt = hostMap.find(host_ip);
    if (hostIt == hostMap.end())
    {
        std::pair<CSNatConnsIpMap::iterator, bool> result =
            hostMap.insert(std::make_pair(host_ip, CSNatConnPortMap()));
        if (!result.second)
        {
            LOG_WARN_FMT("SNAT: insert CSNatConnPortMap for external[%s],host[%s-%s],%s failed !", 
                extIpStr, macStr, hostIpStr, (IPPROTO_TCP==proto)?"tcp":"udp");
            return ret;
        }
        hostIt = result.first;
    }

    CSNatConn* snatConn = new CSNatConn(proto, external_ip, host_ip, internal_port, internal_mac, gateway_dpid, switch_dpid);
    if (NULL == snatConn)
    {
        LOG_ERROR("SNAT: new CSNatConn failed!");
        return ret;
    }
    
    CSmartPtr<CSNatConn> ssnatConn(snatConn);
    if (allocSNatConn(ssnatConn) == BNC_OK)
    {
        CSNatConnPortMap& portMap = hostIt->second;
        portMap.insert(std::make_pair(internal_port, ssnatConn));
        ret = ssnatConn;
    
        INT1 snatIpStr[20] = {0};
        number2ip(ssnatConn->getSNatIp(), snatIpStr);
        LOG_WARN_FMT("SNAT: created %s conn as external[%s],host[%s-%s:%u],snat[%s:%u]", 
            (IPPROTO_TCP==proto)?"tcp":"udp", extIpStr, macStr, hostIpStr, internal_port, snatIpStr, ssnatConn->getSNatPort());
        CClusterSync::syncAddNatHost(*snatConn);
    }

    return ret;
}
CSmartPtr<CSNatConn> CSNatConnMgr::createSNatConn(UINT4 external_ip, 
                                                  UINT4 snat_ip, 
                                                  UINT4 host_ip, 
                                                  UINT2 snat_port,
                                                  UINT2 internal_port, 
                                                  UINT1 proto, 
                                                  const UINT1* internal_mac, 
                                                  UINT8 gateway_dpid, 
                                                  UINT8 switch_dpid)
{
    CSmartPtr<CSNatConn> ret(NULL);

    INT1 macStr[48] = {0};
    mac2str(internal_mac, macStr);
    INT1 extIpStr[20] = {0}, snatIpStr[20] = {0}, hostIpStr[20] = {0};
    number2ip(external_ip, extIpStr);
    number2ip(snat_ip, snatIpStr);
    number2ip(host_ip, hostIpStr);

    CSNatConnsExtMap::iterator extIt = m_exts.find(external_ip);
    if (extIt == m_exts.end())
    {
        std::pair<CSNatConnsExtMap::iterator, bool> result =
            m_exts.insert(std::make_pair(external_ip, CSNatConnsExt()));
        if (!result.second)
        {
            LOG_WARN_FMT("SNAT: insert CSNatConnsExt for external[%s] failed !", extIpStr);
            return ret;
        }
        extIt = result.first;
    }

    CSNatConnsIpMap& hostMap = (IPPROTO_TCP == proto) ? extIt->second.m_tcpHosts : extIt->second.m_udpHosts;
    CSNatConnsIpMap::iterator hostIt = hostMap.find(host_ip);
    if (hostIt == hostMap.end())
    {
        std::pair<CSNatConnsIpMap::iterator, bool> result =
            hostMap.insert(std::make_pair(host_ip, CSNatConnPortMap()));
        if (!result.second)
        {
            LOG_WARN_FMT("SNAT: insert CSNatConnPortMap for external[%s],host[%s-%s],%s failed !", 
                extIpStr, macStr, hostIpStr, (IPPROTO_TCP==proto)?"tcp":"udp");
            return ret;
        }
        hostIt = result.first;
    }

    CSNatConnPortMap& hostPortMap = hostIt->second;
    CSNatConnPortMap::iterator hostPortIt = hostPortMap.find(internal_port);
    if (hostPortIt == hostPortMap.end())
    {
        CSNatConn* snatConn = new CSNatConn(proto, external_ip, snat_ip, host_ip, snat_port, internal_port, internal_mac, gateway_dpid, switch_dpid);
        if (NULL == snatConn)
        {
            LOG_ERROR("SNAT: new CSNatConn failed!");
            return ret;
        }

        ret = CSmartPtr<CSNatConn>(snatConn);
        hostPortMap.insert(std::make_pair(internal_port, ret));
        LOG_WARN_FMT("SNAT: created %s conn as external[%s],host[%s-%s:%u],snat[%s:%u]", 
            (IPPROTO_TCP==proto)?"tcp":"udp", extIpStr, macStr, hostIpStr, internal_port, snatIpStr, snat_port);
    }
    else
    {
        ret = hostPortIt->second;
        *ret = CSNatConn(proto, external_ip, snat_ip, host_ip, snat_port, internal_port, internal_mac, gateway_dpid, switch_dpid);
        LOG_WARN_FMT("SNAT: updated %s conn as external[%s],host[%s-%s:%u],snat[%s:%u]", 
            (IPPROTO_TCP==proto)?"tcp":"udp", extIpStr, macStr, hostIpStr, internal_port, snatIpStr, snat_port);
    }

    CSNatConnsIpMap& snatMap = (IPPROTO_TCP == proto) ? extIt->second.m_tcpSnats : extIt->second.m_udpSnats;
    CSNatConnsIpMap::iterator snatIt = snatMap.find(snat_ip);
    if (snatIt == snatMap.end())
    {
        std::pair<CSNatConnsIpMap::iterator, bool> result =
            snatMap.insert(std::make_pair(snat_ip, CSNatConnPortMap()));
        if (!result.second)
        {
            LOG_WARN_FMT("SNAT: insert CSNatConnPortMap for external[%s],host[%s-%s],%s failed !", 
                extIpStr, macStr, hostIpStr, (IPPROTO_TCP==proto)?"tcp":"udp");
            return ret;
        }
        snatIt = result.first;
    }

    CSNatConnPortMap& snatPortMap = snatIt->second;
    CSNatConnPortMap::iterator snatPortIt = snatPortMap.find(snat_port);
    if (snatPortIt == snatPortMap.end())
    {
        snatPortMap.insert(std::make_pair(snat_port, ret));
        LOG_WARN_FMT("SNAT: created %s conn as external[%s],host[%s-%s:%u],snat[%s:%u]", 
            (IPPROTO_TCP==proto)?"tcp":"udp", extIpStr, macStr, hostIpStr, internal_port, snatIpStr, snat_port);
    }
    else
    {
        snatPortIt->second = ret;
        LOG_WARN_FMT("SNAT: updated %s conn as external[%s],host[%s-%s:%u],snat[%s:%u]", 
            (IPPROTO_TCP==proto)?"tcp":"udp", extIpStr, macStr, hostIpStr, internal_port, snatIpStr, snat_port);
    }

    return ret;
}
CSmartPtr<CSNatConn> CSNatConnMgr::findSNatConnByInt(UINT4 external_ip, UINT4 host_ip, UINT2 internal_port, UINT1 proto)
{
    CSmartPtr<CSNatConn> ret(NULL);

    CSNatConnsExtMap::iterator extIt = m_exts.find(external_ip);
    if (extIt == m_exts.end())
        return ret;

    CSNatConnsIpMap& hostMap = (IPPROTO_TCP == proto) ? extIt->second.m_tcpHosts : extIt->second.m_udpHosts;
    CSNatConnsIpMap::iterator hostIt = hostMap.find(host_ip);
    if (hostIt == hostMap.end())
        return ret;

    CSNatConnPortMap& portMap = hostIt->second;
    CSNatConnPortMap::iterator portIt = portMap.find(internal_port);
    if (portIt != portMap.end())
        ret = portIt->second;

	return ret;
}
CSmartPtr<CSNatConn> CSNatConnMgr::findSNatConnByExt(UINT4 external_ip, UINT4 snat_ip, UINT2 snat_port, UINT1 proto)
{
    CSmartPtr<CSNatConn> ret(NULL);

    CSNatConnsExtMap::iterator extIt = m_exts.find(external_ip);
    if (extIt == m_exts.end())
        return ret;

    CSNatConnsIpMap& snatMap = (IPPROTO_TCP == proto) ? extIt->second.m_tcpSnats : extIt->second.m_udpSnats;
    CSNatConnsIpMap::iterator snatIt = snatMap.find(snat_ip);
    if (snatIt == snatMap.end())
        return ret;

    CSNatConnPortMap& portMap = snatIt->second;
    CSNatConnPortMap::iterator portIt = portMap.find(snat_port);
    if (portIt != portMap.end())
        ret = portIt->second;

	return ret;
}

INT4 CSNatConnMgr::allocSNatConn(CSmartPtr<CSNatConn>& snatConn)
{
    CRouter* router = CRouterGateMgr::getInstance()->FindRouterNodeByHostMac((UINT1*)snatConn->getInternalMac());
    if (NULL == router)
    {
        INT1 macStr[48] = {0};
        mac2str(snatConn->getInternalMac(), macStr);
        LOG_WARN_FMT("NAT: can't get router by host mac[%s]!!!", macStr);
        return BNC_ERR;
    }

    CSmartPtr<CHost> routerNode = CHostMgr::getInstance()->findHostByIp(router->getRouterIp());
    if (routerNode.isNull())
    {
        INT1 ipStr[20] = {0};
        number2ip(router->getRouterIp(), ipStr);
        LOG_WARN_FMT("SNAT: can't get router by router ip[%s]!!!", ipStr);
        return BNC_ERR;
    }

    snatConn->setSNatIp(router->getRouterIp());

    CSNatConnsExtMap::iterator extIt = m_exts.find(snatConn->getExternalIp());
    if (extIt == m_exts.end())
    {
        INT1 ipStr[20] = {0};
        number2ip(snatConn->getExternalIp(), ipStr);
        LOG_WARN_FMT("SNAT: can't get CSNatConnsExt by external ip[%s]!!!", ipStr);
        return BNC_ERR;
    }

    CSNatConnsIpMap& snatMap = (IPPROTO_TCP == snatConn->getProtocol()) ? extIt->second.m_tcpSnats : extIt->second.m_udpSnats;
    CSNatConnsIpMap::iterator snatIt = snatMap.find(router->getRouterIp());
    if (snatIt == snatMap.end())
    {
        std::pair<CSNatConnsIpMap::iterator, bool> result =
            snatMap.insert(std::make_pair(router->getRouterIp(), CSNatConnPortMap()));
        if (!result.second)
            return BNC_ERR;
        snatIt = result.first;
    }

    CSNatConnPortMap& portMap = snatIt->second;
    if (portMap.empty())
    {
        snatConn->setSNatPort(SNAT_PORT_MIN);
    }
    else if (portMap.size() >= SNAT_PORT_MAX-SNAT_PORT_MIN)
    {
        INT1 extIpStr[20] = {0}, routerIpStr[20] = {0};
        number2ip(snatConn->getExternalIp(), extIpStr);
        number2ip(router->getRouterIp(), routerIpStr);
        LOG_WARN_FMT("SNAT: port used out for external ip[%s] and router ip[%s]!!!", extIpStr, routerIpStr);
        return BNC_ERR;
    }
    else
    {
        CSNatConnPortMap::reverse_iterator lastIt = portMap.rbegin();
        UINT2 port = lastIt->first + 1;
        if (port >= SNAT_PORT_MAX)
            port = SNAT_PORT_MIN;
        INT4 count = 0;
        while (portMap.find(port) != portMap.end())
        {
            if (++port >= SNAT_PORT_MAX)
                port = SNAT_PORT_MIN;
            if (++count >= SNAT_PORT_MAX-SNAT_PORT_MIN)
            {
                port = 0;
                break;
            }
        }

        if (0 == port)
        {
            INT1 extIpStr[20] = {0}, routerIpStr[20] = {0};
            number2ip(snatConn->getExternalIp(), extIpStr);
            number2ip(router->getRouterIp(), routerIpStr);
            LOG_WARN_FMT("SNAT: port used out for external ip[%s] and router ip[%s]!!!", extIpStr, routerIpStr);
            return BNC_ERR;
        }        
        snatConn->setSNatPort(port);
    }

    portMap.insert(std::make_pair(snatConn->getSNatPort(), snatConn));

    return BNC_OK;
}

void CSNatConnMgr::removeSNatConnByInt(UINT4 external_ip, UINT4 host_ip, UINT2 internal_port, UINT1 proto)
{
    CSNatConnsExtMap::iterator extIt = m_exts.find(external_ip);
    if (extIt == m_exts.end())
        return;

    CSmartPtr<CSNatConn> snatConn(NULL);

    CSNatConnsExt& ext = extIt->second;
    CSNatConnsIpMap& hostMap = (IPPROTO_TCP == proto) ? ext.m_tcpHosts : ext.m_udpHosts;
    CSNatConnsIpMap::iterator hostIt = hostMap.find(host_ip);
    if (hostIt != hostMap.end())
    {
        CSNatConnPortMap& portMap = hostIt->second;
        CSNatConnPortMap::iterator portIt = portMap.find(internal_port);
        if (portIt != portMap.end())
        {
            snatConn = portIt->second;
            portMap.erase(portIt);
        }
        if (portMap.empty())
            hostMap.erase(hostIt);
    }

    if (snatConn.isNull())
        return;

    CSNatConnsIpMap& snatMap = (IPPROTO_TCP == proto) ? ext.m_tcpSnats : ext.m_udpSnats;
    CSNatConnsIpMap::iterator snatIt = snatMap.find(snatConn->getSNatIp());
    if (snatIt != snatMap.end())
    {
        CSNatConnPortMap& portMap = snatIt->second;
        CSNatConnPortMap::iterator portIt = portMap.find(snatConn->getSNatPort());
        if (portIt != portMap.end())
            portMap.erase(portIt);
        if (portMap.empty())
            snatMap.erase(snatIt);
    }

    if (ext.m_tcpHosts.empty() && ext.m_udpHosts.empty())
        m_exts.erase(extIt);

	INT1 macStr[48] = {0};
	mac2str(snatConn->getInternalMac(), macStr);
    INT1 extIpStr[20] = {0}, hostIpStr[20] = {0}, snatIpStr[20] = {0};
    number2ip(external_ip, extIpStr);
    number2ip(host_ip, hostIpStr);
    number2ip(snatConn->getSNatIp(), snatIpStr);
    LOG_WARN_FMT("SNAT: removed %s conn as external[%s],host[%s-%s:%u],snat[%s:%u]", 
        (IPPROTO_TCP==proto)?"tcp":"udp", extIpStr, macStr, hostIpStr, internal_port, snatIpStr, snatConn->getSNatPort());
    CClusterSync::syncDelNatHost(*snatConn);
}


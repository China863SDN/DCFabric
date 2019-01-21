#include  "log.h"
#include  "bnc-type.h"
#include  "bnc-error.h"
#include  "bnc-param.h"
#include  "comm-util.h"
#include  "CSNatStream.h"
#include  "CSNatStreamMgr.h"
#include  "CNatHostConnMgr.h"

CNatHostConnMgr* CNatHostConnMgr::m_pInstance = NULL;
#if 0
CNatHostConnMgr::CNatHostConnMgr()
{
}
CNatHostConnMgr::~CNatHostConnMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

UINT2 CNatHostConnMgr::CreateNatHostConn(UINT4 host_ip, 
                                         UINT4 external_ip, 
                                         UINT2 internal_port_no,
                                         UINT1 proto_type, 
                                         UINT1* internal_mac, 
                                         UINT8 gateway_dpid, 
                                         UINT8 src_dpid)
{
	UINT2   external_port_no = 0;
	CSNatStream*    natstream(NULL);
	CSNatStreamMgr* natstreamMgr(NULL); 
	CNatStreamMap* it = NULL;
	CNatStreamMap::iterator iter;
	// 如果TCP协议
	if (IPPROTO_TCP == proto_type) {
		it = &m_tcpconnect_map;
	}
	// 如果UDP协议
	else if (IPPROTO_UDP == proto_type) {
		// 查找对应的port(以外部端口为Key)存在
		it = &m_udpconnect_map;
	}
	else {
		// output log
		LOG_INFO("NAT: remove NAT Node  failed! Not TCP/UDP!");
		return 0;
	}

	iter = it->find(host_ip);
	if(it->end() == iter)
	{
		natstream = new CSNatStream(external_ip, internal_port_no, internal_mac, gateway_dpid, src_dpid);
		natstreamMgr = new CSNatStreamMgr();
		natstreamMgr->AddSNatStreamList(natstream);
		it->insert(std::make_pair(host_ip, natstreamMgr));
		
	}
	else
	{
		natstream = iter->second->findSNatStreamByIpAndPort(external_ip, internal_port_no);
		if(NULL == natstream)
		{
			natstream = new CSNatStream(external_ip, internal_port_no, internal_mac, gateway_dpid, src_dpid);
			iter->second->AddSNatStreamList(natstream);
		}
	}
	LOG_WARN_FMT("m_tcpconnect_map size: %lu,m_tcpconnect_map size: %lu",m_tcpconnect_map.size(),m_udpconnect_map.size());
	external_port_no = natstream->getExternalPortNo();
	return external_port_no;
}
INT4 CNatHostConnMgr::CreateNatHostConn(UINT4 host_ip, 
                                        UINT4 external_ip, 
                                        UINT2 internal_port_no, 
                                        UINT2 external_port_no,
                                        UINT1 proto_type, 
                                        UINT1* internal_mac, 
                                        UINT8 gateway_dpid, 
                                        UINT8 src_dpid)
{
	CNatStreamMap* it;
	if (IPPROTO_TCP == proto_type) 
    {
		it = &m_tcpconnect_map;
	}
	else if (IPPROTO_UDP == proto_type) 
    {
		it = &m_udpconnect_map;
	}
	else 
    {
		return BNC_ERR;
	}

	CSNatStreamMgr* natstreamMgr = NULL; 
	CSNatStream*    natstream = NULL;

	CNatStreamMap::iterator iter = it->find(host_ip);
	if ((iter == it->end()) || (NULL == iter->second))
	{
		natstreamMgr = new CSNatStreamMgr();
        if (NULL == natstreamMgr)
        {
            LOG_ERROR("new CSNatStreamMgr failed!!!");
            return BNC_ERR;
        }
		natstream = new CSNatStream(external_ip, external_port_no, internal_port_no, internal_mac, gateway_dpid, src_dpid);
        if (NULL == natstream)
        {
            LOG_ERROR("new CSNatStream failed!!!");
            delete natstreamMgr;
            return BNC_ERR;
        }

		natstreamMgr->AddSNatStream(natstream);
        if (iter == it->end())
    		it->insert(std::make_pair(host_ip, natstreamMgr));
        else
            iter->second = natstreamMgr;
	}
	else
	{
        natstreamMgr = iter->second;
		natstream = natstreamMgr->findSNatStreamByIpAndPort(external_ip, internal_port_no);
		if (NULL == natstream)
		{
            natstream = new CSNatStream(external_ip, external_port_no, internal_port_no, internal_mac, gateway_dpid, src_dpid);
            if (NULL == natstream)
            {
                LOG_ERROR("new CSNatStream failed!!!");
                return BNC_ERR;
            }
    		natstreamMgr->AddSNatStream(natstream);
		}
        else
        {
            natstream->setExternalPortNo(external_port_no);
            natstream->setInternalMac(internal_mac);
            natstream->setGatewaySwDpid(gateway_dpid);
            natstream->setSrcSwDpid(src_dpid);
        }
	}

	return BNC_OK;
}
INT4  CNatHostConnMgr::RemoveNatHostByIp(UINT4 hostip)
{
	
	std::list<CSNatStream *> natstreamlist; 
	std::list<CSNatStream *>::iterator it;
	CNatStreamMap::iterator iter;
	iter = m_tcpconnect_map.find(hostip);
	if(m_tcpconnect_map.end() != iter)
	{
		natstreamlist = iter->second->getNatStreamHead();
		for( it = natstreamlist.begin(); it != natstreamlist.end();) 
		{ 
			delete (*it); 
			it = natstreamlist.erase(it); 
		}
		m_tcpconnect_map.erase(iter);
	}
	iter = m_udpconnect_map.find(hostip);
	if(m_udpconnect_map.end() != iter)
	{
		natstreamlist = iter->second->getNatStreamHead();
		for( it = natstreamlist.begin(); it != natstreamlist.end();) 
		{ 
			delete (*it); 
			it = natstreamlist.erase(it); 
		}
		m_tcpconnect_map.erase(iter);
	}
	return BNC_OK;
}
CSNatStreamMgr* CNatHostConnMgr::FindNatConnectStream(UINT4 hostip, UINT1 proto_type)
{
	CSNatStreamMgr* natstreamMgr(NULL); 
	CNatStreamMap it;
	CNatStreamMap::iterator iter;
	if (IPPROTO_TCP == proto_type) {
		it = m_tcpconnect_map;
	}
	else if(IPPROTO_UDP == proto_type)
	{
		it = m_udpconnect_map;
	}
	else
	{
		return NULL;
	}
	iter = it.find(hostip);
	if(it.end() != iter)
	{
	
		natstreamMgr = iter->second;
	}
	return natstreamMgr;
}
CSNatStream* CNatHostConnMgr::find_nat_connect(UINT4 external_ip, UINT2 external_port_no, UINT1 proto_type)
{
	CSNatStream*    ret(NULL);
	CSNatStreamMgr* natstreamMgr(NULL); 
	natstreamMgr = FindNatConnectStream(external_ip, proto_type);
	if(NULL != natstreamMgr)
	{
		ret = natstreamMgr->findSNatStreamByExternalPortNo(external_port_no);
	}
	return ret;
}

void CNatHostConnMgr::destroy_nat_connect(UINT4 host_ip, UINT2 external_port_no, UINT1 proto_type)
{
	CSNatStreamMgr* natstreamMgr(NULL); 
	CNatStreamMap it;
	CNatStreamMap::iterator iter;
	if (IPPROTO_TCP == proto_type) {
		it = m_tcpconnect_map;
	}
	else if(IPPROTO_UDP == proto_type)
	{
		it = m_udpconnect_map;
	}
	else
	{
		return ;
	}
	iter = it.find(host_ip);
	if(m_tcpconnect_map.end() != iter)
	{
	
		natstreamMgr = iter->second;
		natstreamMgr->RemoveSNatStreamByExternalPortNo(external_port_no);
	}
}

void CNatHostConnMgr::destroy_nat_connect_by_mac_and_port(CSmartPtr<CSwitch> sw, UINT4 host_ip, UINT1* internal_mac, UINT2 internal_port_no, UINT1 proto_type)
{
	
}

CNatHostConnMgr* CNatHostConnMgr::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CNatHostConnMgr();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
	}
	return m_pInstance;
}
#endif


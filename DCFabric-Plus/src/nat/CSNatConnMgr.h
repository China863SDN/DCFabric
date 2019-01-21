#ifndef  _CSNATCONNMGR_H_
#define  _CSNATCONNMGR_H_

#include "CSNatConn.h"
#include "CSmartPtr.h"

typedef std::map<UINT2/*HOST/SNAT PORT*/, CSmartPtr<CSNatConn> > CSNatConnPortMap;
typedef std::map<UINT4/*HOST/SNAT IP*/, CSNatConnPortMap> CSNatConnsIpMap;

class CSNatConnsExt
{
public:
	CSNatConnsIpMap m_tcpHosts; //tcp snat connections, key is host ip
	CSNatConnsIpMap m_udpHosts; //udp snat connections, key is host ip
	CSNatConnsIpMap m_tcpSnats; //tcp snat connections, key is snat ip
	CSNatConnsIpMap m_udpSnats; //udp snat connections, key is snat ip

    CSNatConnsExt() {}
    ~CSNatConnsExt() {}
};

typedef std::map<UINT4/*EXTERNAL IP*/, CSNatConnsExt> CSNatConnsExtMap;

class CSNatConnMgr
{
public:
	static CSNatConnMgr* getInstance();

	~CSNatConnMgr();
    void clear();

	CSmartPtr<CSNatConn> createSNatConn(UINT4 external_ip, 
                                        UINT4 host_ip, 
                                        UINT2 internal_port,
                                        UINT1 proto, 
                                        const UINT1* internal_mac, 
                                        UINT8 gateway_dpid, 
                                        UINT8 switch_dpid);
	CSmartPtr<CSNatConn> createSNatConn(UINT4 external_ip, 
                                        UINT4 snat_ip, 
                                        UINT4 host_ip, 
                                        UINT2 snat_port,
                                        UINT2 internal_port, 
                                        UINT1 proto, 
                                        const UINT1* internal_mac, 
                                        UINT8 gateway_dpid, 
                                        UINT8 switch_dpid);

	CSmartPtr<CSNatConn> findSNatConnByInt(UINT4 external_ip, UINT4 host_ip, UINT2 internal_port, UINT1 proto);
	CSmartPtr<CSNatConn> findSNatConnByExt(UINT4 external_ip, UINT4 snat_ip, UINT2 snat_port, UINT1 proto);

	void removeSNatConnByInt(UINT4 external_ip, UINT4 host_ip, UINT2 internal_port, UINT1 proto);

    CSNatConnsExtMap& getExtMap() {return m_exts;}

private:
	CSNatConnMgr();

    INT4 allocSNatConn(CSmartPtr<CSNatConn>& snatConn);

private:
	static CSNatConnMgr* m_instance;

    CSNatConnsExtMap m_exts;
};
#endif


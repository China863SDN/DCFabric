#ifndef  _CNATHOSTCONNMGR_H_
#define  _CNATHOSTCONNMGR_H_

#include "CSNatStreamMgr.h"

typedef std::map<UINT4 /*HOST IP*/, CSNatStreamMgr* /*TCP/UDP CONNECT STREAM*/> CNatStreamMap;

class CNatHostConnMgr
{
	public:
		~CNatHostConnMgr();
		UINT2 CreateNatHostConn(UINT4 host_ip, 
                                UINT4 external_ip, 
                                UINT2 internal_port_no,
                                UINT1 proto_type, 
                                UINT1* internal_mac, 
                                UINT8 gateway_dpid, 
                                UINT8 src_dpid);
		INT4  CreateNatHostConn(UINT4 host_ip, 
                                UINT4 external_ip, 
                                UINT2 internal_port_no, 
                                UINT2 external_port_no,
                                UINT1 proto_type, 
                                UINT1* internal_mac, 
                                UINT8 gateway_dpid, 
                                UINT8 src_dpid);
		INT4  RemoveNatHostByIp(UINT4 hostip);
		CSNatStreamMgr* FindNatConnectStream(UINT4 hostip, UINT1 proto_type);
		
		CSNatStream* find_nat_connect(UINT4 external_ip, UINT2 external_port_no, UINT1 proto_type);
		UINT4 GetNatConnectCountByIp(UINT4 external_ip, UINT2* port_list, UINT4* externalip_list, UINT2* proto_list);
		
		void destroy_nat_connect(UINT4 host_ip, UINT2 external_port_no, UINT1 proto_type);
		void destroy_nat_connect_by_mac_and_port(CSmartPtr<CSwitch> sw, UINT4 host_ip, UINT1* internal_mac, UINT2 internal_port_no, UINT1 proto_type);

        CNatStreamMap& getTcpNatStreamMap() {return m_tcpconnect_map;}
        CNatStreamMap& getUdpNatStreamMap() {return m_udpconnect_map;}
		
		static CNatHostConnMgr* getInstance();
	private:
		CNatHostConnMgr();
	private:
		static CNatHostConnMgr*  m_pInstance;

		CNatStreamMap m_tcpconnect_map;
		CNatStreamMap m_udpconnect_map;
};
#endif


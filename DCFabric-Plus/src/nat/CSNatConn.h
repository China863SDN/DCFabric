#ifndef _CSNATCONN_H_
#define _CSNATCONN_H_

#include "bnc-type.h"
#include "CRefObj.h"

class CSNatConn : public CRefObj
{
public:
	CSNatConn():
        m_proto(0), m_external_ip(0),m_snat_ip(0),m_snat_port(0),m_internal_ip(0),m_internal_port(0),
        m_gateway_dpid(0),m_switch_dpid(0) {memset(m_internal_mac, 0, MAC_LEN);}
	CSNatConn(UINT1 proto, UINT4 external_ip, UINT4 internal_ip, UINT2 internal_port, 
	    const UINT1* internal_mac, UINT8 gateway_dpid, UINT8 switch_dpid):
        m_proto(proto), m_external_ip(external_ip),m_snat_ip(0),m_snat_port(0),m_internal_ip(internal_ip),
        m_internal_port(internal_port),m_gateway_dpid(gateway_dpid),m_switch_dpid(switch_dpid) 
        {memcpy(m_internal_mac, internal_mac, MAC_LEN);}
	CSNatConn(UINT1 proto, UINT4 external_ip, UINT4 snat_ip, UINT4 internal_ip, UINT2 snat_port, 
	    UINT2 internal_port, const UINT1* internal_mac, UINT8 gateway_dpid, UINT8 switch_dpid):
        m_proto(proto), m_external_ip(external_ip),m_snat_ip(snat_ip),m_snat_port(snat_port),
        m_internal_ip(internal_ip),m_internal_port(internal_port),m_gateway_dpid(gateway_dpid),
        m_switch_dpid(switch_dpid) {memcpy(m_internal_mac, internal_mac, MAC_LEN);}
    ~CSNatConn() {}

    CSNatConn& operator=(const CSNatConn& rhs)
    {
        if (this != &rhs)
        {
            m_proto = rhs.m_proto;
            m_external_ip = rhs.m_external_ip;
            m_snat_ip = rhs.m_snat_ip;
            m_snat_port = rhs.m_snat_port;
            m_internal_ip = rhs.m_internal_ip;
            m_internal_port = rhs.m_internal_port;
            memcpy(m_internal_mac, rhs.m_internal_mac, MAC_LEN);
            m_gateway_dpid = rhs.m_gateway_dpid;
            m_switch_dpid = rhs.m_switch_dpid;
        }
        return *this;
    }

public:
	UINT1 getProtocol() const {return m_proto;}
	UINT4 getExternalIp() const {return m_external_ip;}
	UINT4 getSNatIp() const {return m_snat_ip;}
	UINT2 getSNatPort() const {return m_snat_port;}
	UINT4 getInternalIp() const {return m_internal_ip;}
	UINT2 getInternalPort() const {return m_internal_port;}
	const UINT1* getInternalMac() const {return m_internal_mac;}
	UINT8 getGatewayDpid() const {return m_gateway_dpid;}
	UINT8 getSwitchDpid() const {return m_switch_dpid;}

	void  setProtocol(UINT1 proto) {m_proto = proto;}
	void  setExternalIp(UINT4 external_ip) {m_external_ip = external_ip;}
	void  setSNatIp(UINT4 snat_ip) {m_snat_ip = snat_ip;}
	void  setSNatPort(UINT2 snat_port) {m_snat_port = snat_port;}
	void  setInternalIp(UINT4 internal_ip) {m_internal_ip = internal_ip;}
	void  setInternalPort(UINT2 internal_port) {m_internal_port = internal_port;}
	void  setInternalMac(const UINT1* mac) {memcpy(m_internal_mac, mac, MAC_LEN);}
	void  setGatewayDpid(UINT8 gateway_dpid) {m_gateway_dpid = gateway_dpid;}
	void  setSwitchDpid(UINT8 switch_dpid) {m_switch_dpid = switch_dpid;}

private:
    UINT1 m_proto;			        ///< protocol
	UINT4 m_external_ip;			///< external ip
	UINT4 m_snat_ip;				///< snat ip
	UINT2 m_snat_port;			    ///< snat port, host order
	UINT4 m_internal_ip;			///< host ip
	UINT2 m_internal_port;		    ///< host port, host order
	UINT1 m_internal_mac[MAC_LEN];	///< host mac
	UINT8 m_gateway_dpid;			///< gateway dpid
	UINT8 m_switch_dpid;			///< switch dpid
};
#endif

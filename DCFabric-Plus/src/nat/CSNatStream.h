

#ifndef _CSNATSTREAM_H_
#define _CSNATSTREAM_H_

#define NAT_PORT_MAX_VALUE		59200
#define NAT_PORT_MIN_VALUE		49200

class CSNatStream
{
	public:
		CSNatStream():m_external_ip(0),m_external_port_no(0),m_internal_port_no(0),m_gateway_dpid(0),m_src_dpid(0)
            {memset(m_internal_mac, 0, MAC_LEN);}
		~CSNatStream() {}
		CSNatStream(UINT4 external_ip, UINT2 internal_port_no, UINT1* internal_mac, UINT8 external_gateway_dpid, UINT8 src_dpid):
            m_external_ip(external_ip),m_external_port_no(NAT_PORT_MIN_VALUE),m_internal_port_no(internal_port_no),
            m_gateway_dpid(external_gateway_dpid),m_src_dpid(src_dpid) 
            {memcpy(m_internal_mac, internal_mac, MAC_LEN);}
		CSNatStream(UINT4 external_ip, UINT2 external_port_no, UINT2 internal_port_no, UINT1* internal_mac, 
		    UINT8 external_gateway_dpid, UINT8 src_dpid):
            m_external_ip(external_ip),m_external_port_no(external_port_no),m_internal_port_no(internal_port_no),
            m_gateway_dpid(external_gateway_dpid),m_src_dpid(src_dpid) 
            {memcpy(m_internal_mac, internal_mac, MAC_LEN);}

	public:
		UINT2 getExternalPortNo()  {return m_external_port_no;}
		UINT2 getInternalPortNo()  {return m_internal_port_no;}
		UINT4 getExternalIp()  {return m_external_ip;}
		UINT8 getGatewayDpid()   {return m_gateway_dpid;}
		UINT8 getSrcDpid()   {return m_src_dpid;}
		const UINT1* getInternalMac() {return m_internal_mac;}

		void   setExternalPortNo(UINT2 external_port_no) { m_external_port_no = external_port_no;}
		void   setInternalPortNo(UINT2 internal_port_no) {m_internal_port_no = internal_port_no; }
		void   setExternalIp(UINT4 external_ip){m_external_ip = external_ip; }
		void   setGatewaySwDpid(UINT8 gatewaydpid) {m_gateway_dpid = gatewaydpid; }
		void   setSrcSwDpid(UINT8 srcdpid) {m_src_dpid = srcdpid; }
		void   setInternalMac(UINT1 *mac) {memcpy(m_internal_mac, mac, MAC_LEN);}
	private:
		
	private:
		UINT4 m_external_ip;					///< ???IP
		UINT2 m_external_port_no;				///< ?????????
		UINT2 m_internal_port_no;				///< ???????
		UINT1 m_internal_mac[MAC_LEN];			///< ???mac
		UINT8 m_gateway_dpid;					///< ????????????dpid
		UINT8 m_src_dpid;						///< src dpid
};
#endif

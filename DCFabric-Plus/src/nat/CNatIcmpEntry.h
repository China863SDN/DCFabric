

#ifndef _CNATICMPENTRY_H_
#define _CNATICMPENTRY_H_

class CNatIcmpEntry
{
	public:
		CNatIcmpEntry(){};
		~CNatIcmpEntry(){};
		CNatIcmpEntry(UINT2 identifier,UINT4 host_ip,UINT1* host_mac,UINT8 sw_dpid,UINT4 inport):
			m_identifier(identifier),m_host_ip(host_ip),m_sw_dpid(sw_dpid), m_inport(inport){memcpy(m_host_mac, host_mac, MAC_LEN);}
		INT4 Update(UINT2 identifier,UINT4 host_ip,UINT1* host_mac,UINT8 sw_dpid,UINT4 inport)
			{m_identifier = identifier; m_host_ip = host_ip; m_sw_dpid =sw_dpid ; m_inport = inport; memcpy(m_host_mac, host_mac, MAC_LEN); return BNC_OK;}

	public:
		UINT2 getIdentifier(){return m_identifier; }
		UINT4 getIp() {return m_host_ip; }
		const UINT1* getMac() {return m_host_mac;}
		UINT8 getSwDpid() {return m_sw_dpid; }
		UINT4 getInPort() {return m_inport; }
		
		void  setIdentifier(UINT2 identifier){m_identifier = identifier; }
		void  setIp(UINT4 hostip) {m_host_ip = hostip;}
		void  setSwDpid(UINT8 sw_dpid){m_sw_dpid = sw_dpid;}
		void  setMac(UINT1 *mac) {memcpy(m_host_mac, mac, MAC_LEN);}

	private:
		UINT2 m_identifier;//icmp packet identifier
		UINT4 m_host_ip;//icmp packet host ip
		UINT1 m_host_mac[6];//icmp packet host mac
		UINT8 m_sw_dpid;
		UINT4 m_inport;
};

#endif

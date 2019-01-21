/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2016, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the BNC SDN Controller. BNC SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
*                                                                             *
*   File Name   : CFlowMgr.h		*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-29         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#ifndef _CFLOWMGR_H
#define _CFLOWMGR_H
#include "bnc-param.h"
#include "CMutex.h"
#include "CHost.h"
#include "CProxyConnectMgr.h"
#include "CFirewallRule.h"
#include "CFlowCache.h"
#include "CTagFlowEventConsum.h"


#define   FLOATING_DEL		2
#define   FLOATING_ADD		1

/*
 * 管理流表的类
 */
class CFlowMgr
{
public:
 	/*
	 * 提供实例
	 */
	static CFlowMgr* getInstance();

	~CFlowMgr();

    INT4 init();

	/*
	 * 测试�?
	 * 测试流表是否可以正常下发
	 */
	void clear_flow(const CSmartPtr<CSwitch> & sw);

	/*
	 * 下发基础流表
	 * 默认的Goto:Controller
	 */
	void install_base_flows(const CSmartPtr<CSwitch> & sw);
	
	
	void install_base_dcfabric_flows(const CSmartPtr<CSwitch> & sw);
	
	/*
	 * 下发主机output流表
	 */
	void install_local_host_flows(CSmartPtr<CHost>& host);

	/*
	 * 下发主机output流表
	 */
	void install_local_host_flows(CSmartPtr<CHost>& host, CSmartPtr<CHost>& gateway);

	/*
     * Table0: vlan交换流表
     */
    void install_swap_input_flow(const CSmartPtr<CSwitch> & srcSw, const CSmartPtr<CSwitch> & dstSw, UINT4 port_no);

	 /*
      * Table3: vlan交换流表
      */
     void install_swap_flow(const CSmartPtr<CSwitch> & srcSw, const CSmartPtr<CSwitch> & dstSw, UINT4 tag);

	/*
	 * 下发不同交换机上的主机通信流表
	 */
	void install_different_switch_flow(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost);

	/*
	 * 下发不同网段交换机上主机通信流表
	 */
	void install_different_switch_flow(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost, CSmartPtr<CHost>& srcGateway);

    /*
     * 同交换机跨网段流�?
     */
    void install_same_switch_gateway_flow(CSmartPtr<CHost>& host, CSmartPtr<CHost>& gateway);

	void install_same_switch_flow(CSmartPtr<CHost>& srchost, CSmartPtr<CHost>& dstHost);
    /*
     * 代理主机的流�?
     */
    void install_proxy_flows(CProxyConnect* proxyConnect);


	INT4 install_delete_mediaflow(INT4 svr_ip, INT4 client_ip);

	
	void install_base_default_gotonext_flow(const CSmartPtr<CSwitch> & sw);
	
	void install_different_switch_flow(CSmartPtr<CSwitch> dstSw, CSmartPtr<CSwitch> nodeSw, UINT4 output);

private:
	CFlowMgr();

	void install_base_gotonext_flow(const CSmartPtr<CSwitch> & sw, UINT1 table_id);	
	void install_base_flow(const CSmartPtr<CSwitch> & sw, UINT2 match_proto, UINT1 table_id);
	void install_base_gototable_flow(const CSmartPtr<CSwitch> & sw, UINT1 table_id, UINT1 gototable_id);

	/*
	 * 基础流表: goto: Controller
	 */
    void install_base_flow(const CSmartPtr<CSwitch> & sw, UINT1 table_id);

    /*
     * Table0: vlan pop流表
     */
    void install_vlan_base_flow(const CSmartPtr<CSwitch> & sw);

    /*
     * ip默认动作: goto: Table3
     */
    void install_ip_base_flow(const CSmartPtr<CSwitch> & sw);

    /*
     * Table6: 主机Output流表
     */
    void install_local_host_output_flow(CSmartPtr<CHost>& host, UINT1 tableid);

    /*
     * 下发Table3->Table6 主机流表
     */
    void install_local_host_find_flow(CSmartPtr<CHost>& host);

    /*
     * 下发代理主机流表
     */
    void install_proxy_host_flow(CProxyConnect* proxyConnect, CSmartPtr<CSwitch> extSw, CSmartPtr<CSwitch> hostSw);
    void install_proxy_external_flow(CProxyConnect* proxyConnect, CSmartPtr<CSwitch> extSw, CSmartPtr<CSwitch> hostSw, UINT4 outPort);
    void install_proxy_external_output_flow(CSmartPtr<CSwitch> extSw, const UINT1* gatewayMac, UINT2 extPortNo);

public:	
	void install_fabric_output_flow(CSmartPtr<CSwitch> sw,UINT1* mac, UINT4 port);
	void install_modifine_ExternalSwitch_Table_flow(CSmartPtr<CSwitch> sw);
	void install_modifine_ExternalSwitch_ExPort_flow(CSmartPtr<CSwitch> sw, UINT4 inport);
	void install_fabric_external_output_flow(CSmartPtr<CSwitch> sw,UINT4 port,UINT1* gateway_mac,UINT4 outer_interface_ip,UINT1 type);
	void install_fabric_nat_throughfirewall_from_inside_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
			UINT1* packetin_src_mac, CSmartPtr<CSwitch> sw);
	
	void install_fabric_nat_from_inside_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
									UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
									UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port, CSmartPtr<CSwitch> sw, CSmartPtr<CSwitch> gateway_sw);
	
	void install_fabric_nat_from_external_fabric_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
			UINT1* packetin_src_mac, UINT4 external_ip, UINT1* external_mac, UINT2 external_port_no, UINT4 src_vlan_vid, CSmartPtr<CSwitch> gateway_sw, UINT4 out_port);
	
	void install_fabric_nat_from_external_fabric_host_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
			UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
			UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port, CSmartPtr<CSwitch> sw, CSmartPtr<CSwitch> gateway_sw);
	
	void install_fabric_nat_sameswitch_external2host_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
			UINT1* packetin_src_mac, UINT4 external_ip, UINT1* external_mac, UINT2 external_port_no, UINT2 host_port, CSmartPtr<CSwitch> sw);
	
	void install_fabric_nat_sameswitch_host2external_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
			UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no, UINT2 gateway_out_port, CSmartPtr<CSwitch> sw);

//****************************************************floating ip***************************************************************//	
	INT4 install_add_fabric_controller_flow(const CSmartPtr<CSwitch>& sw);
	INT4 install_proactive_floating_host_to_external_flow(const CSmartPtr<CSwitch>& sw, INT4 type, UINT4 match_ip, UINT1* match_mac, UINT4 mod_src_ip, UINT1* mod_dst_mac, UINT4 vlan_id);
	INT4 install_floatingip_set_vlan_in_flow(const CSmartPtr<CSwitch>& sw, UINT4 match_ip, UINT4 mod_dst_ip, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port);
	INT4 install_add_FloatingIP_ToFixIP_OutputToHost_flow(CSmartPtr<CHost>& fixed_port, UINT4 floatingip);

	
	INT4 install_fabric_floating_internal_subnet_flow(const CSmartPtr<CSwitch>& sw, INT4 type, UINT4 dst_ip, UINT4 dst_mask);
	INT4 install_remove_FloatingIP_ToFixIP_OutputToHost_flow(CSmartPtr<CHost>& fixed_port, UINT4 floatingip);
	INT4 delete_fabric_input_flow_by_ip(const CSmartPtr<CSwitch>& sw, UINT4 ip);

	
	INT4 delete_fabric_flow_by_ip(const CSmartPtr<CSwitch>& sw, UINT4 ip, UINT2 table_id);
	INT4 delete_fabric_flow_by_mac(const CSmartPtr<CSwitch>& sw, UINT1* mac, UINT2 table_id);
	INT4 install_ip_controller_flow(const CSmartPtr<CSwitch>& sw,UINT4 ip, UINT2 table_id);
	
	INT4 delete_fabric_input_flow_by_dstmac_proto(const CSmartPtr<CSwitch>& sw, UINT1* dst_mac, UINT4 src_ip, UINT2 dst_port, UINT2 proto);
	INT4 delete_fabric_input_flow_by_srcmac_proto(const CSmartPtr<CSwitch>& sw, UINT1* src_mac, UINT4 dst_ip, UINT2 src_port, UINT2 proto);

//****************************************************firewall***************************************************************//	
    INT4 install_firewallIn_withPort_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT4 dstIP, 
        UINT4 srcIPMask, UINT1 protocol, UINT2 srcPort, UINT2 dstPort, UINT2 priority, UINT1 command, BOOL accept);
    INT4 install_firewallOut_withPort_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT4 dstIP, 
        UINT4 dstIPMask, UINT1 protocol, UINT4 srcPort, UINT4 dstPort, UINT2 priority, UINT1 command, BOOL accept);
    INT4 install_firewallIn_withoutPort_flow(const CSmartPtr<CSwitch>& sw, 
        UINT4 srcIP, UINT4 dstIP, UINT4 srcIPMask, UINT1 protocol, UINT2 priority, UINT1 command, BOOL accept);
    INT4 install_firewallOut_withoutPort_flow(const CSmartPtr<CSwitch>& sw, 
        UINT4 srcIP, UINT4 dstIP, UINT4 dstIPMask, UINT1 protocol, UINT2 priority, UINT1 command, BOOL accept);
    INT4 install_firewallIn_gotoController_flow(const CSmartPtr<CSwitch>& sw, 
        UINT4 srcIP, UINT4 dstIP, UINT4 srcIPMask, UINT1 protocol, UINT2 priority, UINT1 command, BOOL accept);
    INT4 install_firewallOut_gotoController_flow(const CSmartPtr<CSwitch>& sw, 
        UINT4 srcIP, UINT4 dstIP, UINT4 dstIPMask, UINT1 protocol, UINT2 priority, UINT1 command, BOOL accept);

    INT4 install_firewallOut_ephemeral_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT4 dstIP, 
        UINT1 protocol, UINT4 srcPort, UINT4 dstPort, UINT2 priority, BOOL accept);
    INT4 install_firewallIn_ephemeral_flow(const CSmartPtr<CSwitch>& sw, UINT4 srcIP, UINT4 dstIP, 
        UINT1 protocol, UINT4 srcPort, UINT4 dstPort, UINT2 priority, BOOL accept);

//****************************************************portforward***************************************************************//	
    INT4 install_portforwardIn_output_flow(const CSmartPtr<CSwitch>& gwSw, UINT4 srcIP, UINT2 srcPort, UINT4 dstIP, 
        UINT2 dstPort, UINT1 protocol, UINT1* inMac, UINT4 inIP, UINT2 inPort, UINT2 tag, UINT4 outputPort, UINT1 command);
    INT4 install_portforwardOut_output_flow(const CSmartPtr<CSwitch>& hostSw, UINT4 srcIP, UINT2 srcPort, UINT4 dstIP, 
        UINT2 dstPort, UINT1 protocol, UINT1* gwMac, UINT4 outIP, UINT2 outPort, UINT2 tag, UINT4 outputPort, UINT1 command);
    INT4 install_portforwardIn_gotoController_flow(const CSmartPtr<CSwitch>& gwSw, 
        UINT4 dstIP, UINT1 protocol, UINT1 command);
    INT4 install_portforwardOut_gotoController_flow(const CSmartPtr<CSwitch>& hostSw, 
        UINT4 srcIP, UINT1 protocol, UINT1 command);
    INT4 install_portforwardOut_external_flow(const CSmartPtr<CSwitch>& gwSw, 
        UINT4 srcIP, UINT1 protocol, UINT2 srcPort, UINT4 outputPort, UINT1 command, BOOL ephemeral=FALSE);

    INT4 install_portforwardIn_ephemeral_flow(const CSmartPtr<CSwitch>& sw, UINT4 outIP, UINT2 outPort, 
        UINT1 protocol, UINT1* inMac, UINT4 inIP, UINT2 inPort, UINT2 tag, UINT4 outputPort);
    INT4 install_portforwardOut_ephemeral_flow(const CSmartPtr<CSwitch>& hostSw, UINT4 inIP, UINT2 inPort, UINT1 protocol, 
        UINT1* gwMac, UINT4 outIP, UINT2 outPort, UINT2 tag, UINT4 outputPort);

//****************************************************web***************************************************************//  
    INT4 add_flow_entry(const CSmartPtr<CSwitch>& sw, gn_flow_t& flow);
    INT4 del_flow_entry(const CSmartPtr<CSwitch>& sw, gn_flow_t& flow);

    CFlowCache& getFlowCache() {return m_flowCache;}

	static INT4 install_fabric_flows(CSmartPtr<CSwitch> sw, UINT2 idle_timeout,UINT2 hard_timeout,UINT2 priority,UINT1 table_id,UINT1 command,flow_param_t* flow_param);

    static BOOL sendOfp13FlowMod(CSmartPtr<CSwitch> sw, UINT1 *flowmod_req);

private:
	static CMutex   g_sendPktMutex;
	static CFlowMgr* m_pInstance;

    CFlowCache m_flowCache;
	
	CTagFlowEventConsum m_tagflow_consume;
};

#endif

#include "fabric_flows.h"
#include "fabric_path.h"
#include "../flow-mgr/flow-mgr.h"
#include "timer.h"
#include "../conn-svr/conn-svr.h"
#include "gn_inet.h"
#include "mod-types.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "fabric_openstack_nat.h"
#include "fabric_impl.h"
#include "../qos-mgr/qos-policy.h"
#include "../qos-mgr/qos-meter.h"
#include "../cluster-mgr/cluster-mgr.h"

extern UINT4 g_reserve_ip;

// flow functions
gn_flow_t * install_add_fabric_host_input_flow(gn_switch_t *sw);
gn_flow_t * install_add_fabric_miss_match_flow(gn_switch_t* sw);
gn_flow_t * install_add_fabric_ARP_miss_match_flow(gn_switch_t* sw);
gn_flow_t * install_add_fabric_switch_input_flow(gn_switch_t *sw,gn_port_t* sw_port);
gn_flow_t * install_add_fabric_impl_last_flow(gn_switch_t * sw,UINT4 tag);
gn_flow_t * install_add_fabric_impl_first_flow(gn_switch_t * sw,UINT4 port,UINT4 tag);
gn_flow_t * install_add_fabric_impl_middle_flow(gn_switch_t * sw,UINT4 port,UINT4 tag);

void install_add_fabric_lldpmiss_controller_flow(gn_switch_t *sw);
void install_add_fabric_toVm_controller_flow(gn_switch_t *sw);

void install_delete_fabric_flow(gn_switch_t* sw);
void install_delete_fabric_impl_flow(gn_switch_t *sw,UINT4 port_no,UINT4 tag,UINT1 table_id);

void add_action_param(action_param_t** action_param, INT4 type, void* param);
void add_action_param_rear(action_param_t** action_param, INT4 type, void* param);
flow_param_t* init_flow_param();
void clear_flow_param(flow_param_t* flow_param);

INT4 install_fabric_flows(gn_switch_t * sw,
						  UINT2 idle_timeout,
						  UINT2 hard_timeout,
						  UINT2 priority,
						  UINT1 table_id,
						  UINT1 command,
						  flow_param_t* flow_param);
void install_fabric_clear_stat_flows(gn_switch_t * sw,
						  UINT2 idle_timeout,
						  UINT2 hard_timeout,
						  UINT2 priority,
						  UINT1 table_id,
						  UINT1 command,
						  flow_param_t* flow_param);
void set_security_match(gn_oxm_t* oxm, security_param_t* security_param);
void set_qos_match(gn_switch_t* sw, flow_param_t* flow_param);

extern BOOL initiative_check_if_external_and_install_flow(gn_switch_t* sw);
// new ids for mem alloc
//extern void *g_gnflow_mempool_id;
//extern void *g_gninstruction_mempool_id;
//extern void *g_gnaction_mempool_id;
// temp fucntion
//UINT1 check_down_flow(gn_switch_t* sw){
//	if(sw->dpid == 128983883520 || sw->dpid == 128983886902){
//		return 0;
//	}
//	return 1;
//};
/**************************************
 * Interface functions
 **************************************/
/*
 * initialize fabric flows
 */
void init_fabric_flows(){
	//p_fabric_flow f_flow = NULL;

	return;
}
/*
 * clear fabric flows
 */
//by:yhy 清空sw上所有table0,1,2流表
void delete_fabric_flow(gn_switch_t *sw)
{
	install_delete_fabric_flow(sw);
	return;
};
/*
 * clear fabric flows by sw
 */
void delete_fabric_flow_by_sw(gn_switch_t *sw){
	install_delete_fabric_flow(sw);
	return;
};
void delete_fabric_impl_flow(gn_switch_t *sw,UINT4 port_no,UINT4 tag,UINT1 table_id){
	if(sw&&(CONNECTED == sw->conn_state)){
		install_delete_fabric_impl_flow(sw,port_no,tag,table_id);
	}
	return;
};

//by:yhy 装载基础流表
void install_fabric_base_flows(gn_switch_t * sw)
{
	if (OFP10_VERSION == sw->ofp_version) 
	{
	 	install_add_fabric_controller_flow(sw);
		install_add_fabric_ARP_miss_match_flow(sw);
		install_add_fabric_miss_match_flow(sw);
		install_add_fabric_lldpmiss_controller_flow(sw);
	}
	else if (OFP13_VERSION == sw->ofp_version) 
	{
		install_add_fabric_controller_flow(sw);
		install_add_fabric_host_input_flow(sw);
		install_add_fabric_ARP_miss_match_flow(sw);
		//install_add_fabric_miss_match_flow(sw);
		install_add_fabric_lldpmiss_controller_flow(sw);
		
		install_add_NATTable_default_flow(sw);
		install_add_FloatingTable_default_flow(sw);
		install_add_OtherTable_default_flow(sw);
		install_add_FQInPostProcessTable_default_flow(sw);
			
		install_add_QOS_in_jump_default_flow(sw,FABRIC_QOS_IN_TABLE,FABRIC_OUTPUT_TABLE);
		install_add_QOS_jump_default_flow(sw,FABRIC_QOS_OUT_TABLE,FABRIC_FQ_OUT_POST_PROCESS_TABLE);
		install_add_FQ_out_PostProcessTable_default_flow(sw);
		install_add_fabric_toVm_controller_flow(sw);
	}
	else 
	{
		// do nothing
	}

	return;
};
/*
 * install last tag
 */
//by:yhy 对sw下载匹配vlan_vid=tag,然后执行pop_vlan并跳转至FABRIC_OUTPUT_TABLE的流表
void install_fabric_last_flow(gn_switch_t * sw,UINT4 tag)
{
	if(TRUE != initiative_check_if_external_and_install_flow(sw))
	{
		install_add_fabric_impl_last_flow(sw,tag);
	}
	return;
};
/*
 *install first tag
 */
//by:yhy 对switch匹配vlan_vid=tag的包跳转到FABRIC_SWAPTAG_TABLE,从port输出
void install_fabric_first_flow(gn_switch_t * sw,UINT4 port,UINT4 tag)
{
	install_add_fabric_impl_first_flow(sw,port,tag);
	return;
};
/*
 * install middle tag
 */
//by:yhy 
void install_fabric_middle_flow(gn_switch_t * sw,UINT4 port,UINT4 tag)
{
	install_add_fabric_impl_middle_flow(sw,port,tag);
	return;
};
/* 
 * 同一交换机下
 */
void install_fabric_same_switch_flow(gn_switch_t * sw,UINT1* mac, UINT4 port)
{
	flow_param_t* flow_param = init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);
	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
};


gn_flow_t * install_add_NATTable_default_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_FLOATING_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
	
	install_fabric_flows(sw, FABRIC_NAT_TABLE_IDLE_TIME_OUT, FABRIC_NAT_TABLE_HARD_TIME_OUT,FABRIC_PRIORITY_NAT_TABLE_DEFAULT_FLOW,FABRIC_NAT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_FloatingTable_default_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_OTHER_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
	
	install_fabric_flows(sw, FABRIC_FLOATING_TABLE_IDLE_TIME_OUT, FABRIC_FLOATING_TABLE_HARD_TIME_OUT,FABRIC_PRIORITY_FLOATING_TABLE_DEFAULT_FLOW,FABRIC_FLOATING_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

gn_flow_t * install_add_OtherTable_default_flow(gn_switch_t *sw)
{
	flow_param_t* flow_param = init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_OTHER_TABLE_IDLE_TIME_OUT, FABRIC_OTHER_TABLE_HARD_TIME_OUT, FABRIC_PRIORITY_OTHER_TABLE_DEFAULT_FLOW,
						 FABRIC_OTHER_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

gn_flow_t * install_add_FQInPostProcessTable_default_flow(gn_switch_t *sw)
{
	flow_param_t* flow_param = init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_OTHER_TABLE_IDLE_TIME_OUT, FABRIC_OTHER_TABLE_HARD_TIME_OUT, FABRIC_PRIORITY_OTHER_TABLE_DEFAULT_FLOW,
						 FABRIC_FQ_IN_POST_PROCESS_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:Hongyu Yang
 * 入口防火墙的全通
 */
gn_flow_t * install_add_FirewallIn_functionOff_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_IN_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
	
	install_fabric_flows(sw, FABRIC_FIREWALL_IN_IDLE_TIME_OUT, FABRIC_FIREWALL_IN_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_IN_DEFAULT_FLOW,FABRIC_FIREWALL_IN_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:Hongyu Yang
 * 出口防火墙的全通 
 */
gn_flow_t * install_add_FirewallOut_functionOff_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_OUT_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);
	
	install_fabric_flows(sw, FABRIC_FIREWALL_OUT_IDLE_TIME_OUT, FABRIC_FIREWALL_OUT_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_OUT_DEFAULT_FLOW,FABRIC_FIREWALL_OUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:Hongyu Yang
 * 入口防火墙无匹配drop 
 */
gn_flow_t * install_remove_FirewallIn_defaultDrop_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_IN_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);	//drop
	
	install_fabric_flows(sw, FABRIC_FIREWALL_IN_IDLE_TIME_OUT, FABRIC_FIREWALL_IN_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_IN_DEFAULT_FLOW,FABRIC_FIREWALL_IN_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:Hongyu Yang
 * 出口防火墙无匹配drop 
 */
gn_flow_t * install_remove_FirewallOut_defaultDrop_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_OUT_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);	//drop
	
	install_fabric_flows(sw, FABRIC_FIREWALL_OUT_IDLE_TIME_OUT, FABRIC_FIREWALL_OUT_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_OUT_DEFAULT_FLOW,FABRIC_FIREWALL_OUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:Hongyu Yang
 * 删除入口防火墙的指定目标IP的通过流表 
 */
gn_flow_t * install_remove_FirewallIn_flow(gn_switch_t *sw,UINT4 DstIP)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(DstIP);

	install_fabric_flows(sw, 0, 0, 0, FABRIC_FIREWALL_IN_TABLE, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:Hongyu Yang
 * 删除出口防火墙的指定源IP的通过流表
 */
gn_flow_t * install_remove_FirewallOut_flow(gn_switch_t *sw,UINT4 SrcIP)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(SrcIP);

	install_fabric_flows(sw, 0, 0, 0, FABRIC_FIREWALL_OUT_TABLE, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

gn_flow_t * install_add_FirewallIn_gotoController_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT4 SrcIPMask,UINT1 Protocol)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	LOG_PROC("INFO", "%s_%d",FN,LN);
	UINT1 JumpToTable = FABRIC_QOS_IN_TABLE;
	UINT4 port = OFPP13_CONTROLLER;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
		flow_param->match_param->ipv4_src_prefix = ntohl(SrcIPMask);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
	}

	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_IN_IDLE_TIME_OUT, FABRIC_FIREWALL_IN_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_IN_DEFAULT_FLOW,FABRIC_FIREWALL_IN_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_FirewallOut_gotoController_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT4 DstIPMask,UINT1 Protocol)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_OUT_TABLE;
	UINT4 port = OFPP13_CONTROLLER;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
		flow_param->match_param->ipv4_dst_prefix = ntohl(DstIPMask);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
	}
	
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_FIREWALL_OUT_IDLE_TIME_OUT, FABRIC_FIREWALL_OUT_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_OUT_DEFAULT_FLOW,FABRIC_FIREWALL_OUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_FirewallIn_spicificThrough_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT1 Protocol,UINT4 SrcPort,UINT4 DstPort)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_IN_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
		if(Protocol==IPPROTO_TCP)
		{
			flow_param->match_param->tcp_src =SrcPort;
			flow_param->match_param->tcp_dst =DstPort;
		}
		else if (Protocol==IPPROTO_UDP)
		{
			flow_param->match_param->udp_src =SrcPort;
			flow_param->match_param->udp_dst =DstPort;
		}
		else if(Protocol==IPPROTO_ICMP)
		{
			flow_param->match_param->icmpv4_type =SrcPort;
			flow_param->match_param->icmpv4_code =DstPort;
		}
	}

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);

	install_fabric_flows(sw, FABRIC_FIREWALL_IN_SPICIFIC_FLOW_IDLE_TIME_OUT, FABRIC_FIREWALL_IN_SPICIFIC_FLOW_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_IN_SPICIFIC_FLOW,FABRIC_FIREWALL_IN_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_FirewallOut_spicificThrough_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT1 Protocol,UINT4 SrcPort,UINT4 DstPort)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_OUT_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
		if(Protocol==IPPROTO_TCP)
		{
			flow_param->match_param->tcp_src =SrcPort;
			flow_param->match_param->tcp_dst =DstPort;
		}
		else if (Protocol==IPPROTO_UDP)
		{
			flow_param->match_param->udp_src =SrcPort;
			flow_param->match_param->udp_dst =DstPort;
		}
		else if(Protocol==IPPROTO_ICMP)
		{
			flow_param->match_param->icmpv4_type =SrcPort;
			flow_param->match_param->icmpv4_code =DstPort;
		}
	}

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);

	install_fabric_flows(sw, FABRIC_FIREWALL_OUT_SPICIFIC_FLOW_IDLE_TIME_OUT, FABRIC_FIREWALL_OUT_SPICIFIC_FLOW_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_OUT_SPICIFIC_FLOW,FABRIC_FIREWALL_OUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_FirewallIn_spicificDeny_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT1 Protocol,UINT4 SrcPort,UINT4 DstPort)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_IN_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
		if(Protocol==IPPROTO_TCP)
		{
			flow_param->match_param->tcp_src =SrcPort;
			flow_param->match_param->tcp_dst =DstPort;
		}
		else if (Protocol==IPPROTO_UDP)
		{
			flow_param->match_param->udp_src =SrcPort;
			flow_param->match_param->udp_dst =DstPort;
		}
		else if(Protocol==IPPROTO_ICMP)
		{
			flow_param->match_param->icmpv4_type =SrcPort;
			flow_param->match_param->icmpv4_code =DstPort;
		}
	}

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	install_fabric_flows(sw, FABRIC_FIREWALL_IN_SPICIFIC_FLOW_IDLE_TIME_OUT, FABRIC_FIREWALL_IN_SPICIFIC_FLOW_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_IN_SPICIFIC_FLOW,FABRIC_FIREWALL_IN_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_FirewallOut_spicificDeny_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT1 Protocol,UINT4 SrcPort,UINT4 DstPort)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_OUT_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
		if(Protocol==IPPROTO_TCP)
		{
			flow_param->match_param->tcp_src =SrcPort;
			flow_param->match_param->tcp_dst =DstPort;
		}
		else if (Protocol==IPPROTO_UDP)
		{
			flow_param->match_param->udp_src =SrcPort;
			flow_param->match_param->udp_dst =DstPort;
		}
		else if(Protocol==IPPROTO_ICMP)
		{
			flow_param->match_param->icmpv4_type =SrcPort;
			flow_param->match_param->icmpv4_code =DstPort;
		}
	}

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	install_fabric_flows(sw, FABRIC_FIREWALL_OUT_SPICIFIC_FLOW_IDLE_TIME_OUT, FABRIC_FIREWALL_OUT_SPICIFIC_FLOW_HARD_TIME_OUT,FABRIC_PRIORITY_FIREWALL_OUT_SPICIFIC_FLOW,FABRIC_FIREWALL_OUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}


/* by:Hongyu Yang
 * 在入口防火墙上下载通过流表
 */
gn_flow_t * install_add_FirewallIn_withPort_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT4 SrcIPMask,UINT1 Protocol,UINT4 SrcPort,UINT4 DstPort,UINT4 Priority)
{
	//LOG_PROC("INFO", "%s_%d_%ld",FN,LN,sw->sw_ip);
	//LOG_PROC("INFO", "%s_%d_src:%ld_%ld_%d",FN,LN,SrcIP,DstIP,SrcIPMask);
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_IN_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
		flow_param->match_param->ipv4_src_prefix = (SrcIPMask);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
		if(Protocol==IPPROTO_TCP)
		{
			flow_param->match_param->tcp_dst =DstPort;
		}
		else if (Protocol==IPPROTO_UDP)
		{
			flow_param->match_param->udp_dst =DstPort;
		}
		else if(Protocol==IPPROTO_ICMP)
		{
			flow_param->match_param->icmpv4_type =SrcPort;
			flow_param->match_param->icmpv4_code =DstPort;
		}
	}
	if(Priority >65500)
	{
		Priority=65500;
	}
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);

	install_fabric_flows(sw, FABRIC_FIREWALL_IN_IDLE_TIME_OUT, FABRIC_FIREWALL_IN_HARD_TIME_OUT,(FABRIC_PRIORITY_FIREWALL_IN_DEFAULT_FLOW+Priority),FABRIC_FIREWALL_IN_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:Hongyu Yang
 * 在出口防火墙上下载通过流表
 */
gn_flow_t * install_add_FirewallOut_withPort_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT4 DstIPMask,UINT1 Protocol,UINT4 SrcPort,UINT4 DstPort,UINT4 Priority)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_OUT_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
		flow_param->match_param->ipv4_dst_prefix = (DstIPMask);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
		if(Protocol==IPPROTO_TCP)
		{
			flow_param->match_param->tcp_src =SrcPort;
		}
		else if (Protocol==IPPROTO_UDP)
		{
			flow_param->match_param->udp_src =SrcPort;
		}
		else if(Protocol==IPPROTO_ICMP)
		{
			flow_param->match_param->icmpv4_type =SrcPort;
			flow_param->match_param->icmpv4_code =DstPort;
		}
	}
	if(Priority >65500)
	{
		Priority=65500;
	}
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);

	install_fabric_flows(sw, FABRIC_FIREWALL_OUT_IDLE_TIME_OUT, FABRIC_FIREWALL_OUT_HARD_TIME_OUT,(FABRIC_PRIORITY_FIREWALL_OUT_DEFAULT_FLOW+Priority),FABRIC_FIREWALL_OUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:Hongyu Yang
 * 在入口防火墙上下载通过流表
 */
gn_flow_t * install_add_FirewallIn_withoutPort_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT4 SrcIPMask,UINT1 Protocol,UINT2 Priority)
{
	//LOG_PROC("INFO", "%s_%d_%ld",FN,LN,sw->sw_ip);
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_IN_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
		flow_param->match_param->ipv4_src_prefix = ntohl(SrcIPMask);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
	}
	if(Priority >65500)
	{
		Priority=65500;
	}
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);

	install_fabric_flows(sw, FABRIC_FIREWALL_IN_IDLE_TIME_OUT, FABRIC_FIREWALL_IN_HARD_TIME_OUT,(FABRIC_PRIORITY_FIREWALL_IN_DEFAULT_FLOW+Priority),FABRIC_FIREWALL_IN_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	return NULL;
}

/* by:Hongyu Yang
 * 在出口防火墙上下载通过流表
 */
gn_flow_t * install_add_FirewallOut_withoutPort_flow(gn_switch_t *sw,UINT4 SrcIP,UINT4 DstIP,UINT4 DstIPMask,UINT1 Protocol,UINT4 Priority)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	UINT1 JumpToTable = FABRIC_QOS_OUT_TABLE;
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	if(SrcIP)
	{
		flow_param->match_param->ipv4_src = ntohl(SrcIP);
	}
	if(DstIP)
	{
		flow_param->match_param->ipv4_dst = ntohl(DstIP);
		flow_param->match_param->ipv4_dst_prefix = ntohl(DstIPMask);
	}
	if(Protocol)
	{
		flow_param->match_param->ip_proto = Protocol;
	}
	if(Priority >65500)
	{
		Priority=65500;
	}
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&JumpToTable);

	install_fabric_flows(sw, FABRIC_FIREWALL_OUT_IDLE_TIME_OUT, FABRIC_FIREWALL_OUT_HARD_TIME_OUT,(FABRIC_PRIORITY_FIREWALL_OUT_DEFAULT_FLOW+Priority),FABRIC_FIREWALL_OUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

gn_flow_t * install_modifine_ExternalSwitch_goto_FirewallInTable_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = FABRIC_FIREWALL_IN_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->in_port = OFPP13_CONTROLLER;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_INPUT_TABLE_PICA8_IP_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

gn_flow_t * install_modifine_ExternalSwitch_ExPort_goto_FirewallInTable_flow(gn_switch_t *sw, UINT4 inport)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
		{
			return NULL;
		}
		flow_param_t* flow_param = init_flow_param();
		UINT1 table_id = FABRIC_FIREWALL_IN_TABLE;
	
		flow_param->match_param->eth_type = ETHER_IP;
		flow_param->match_param->in_port = inport;
		
		add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	
		install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_INPUT_TABLE_PICA8_IP_FLOW,
							 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);
	
		clear_flow_param(flow_param);
		return NULL;
}

gn_flow_t * install_remove_ExternalSwitch_ExPort_goto_FirewallInTable_flow(gn_switch_t *sw, UINT4 inport)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
		{
			return NULL;
		}
		flow_param_t* flow_param = init_flow_param();
		UINT1 table_id = FABRIC_FIREWALL_IN_TABLE;
	
		flow_param->match_param->eth_type = ETHER_IP;
		flow_param->match_param->in_port = inport;
		
		add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	
		install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_INPUT_TABLE_PICA8_IP_FLOW,
							 FABRIC_INPUT_TABLE, OFPFC_DELETE, flow_param);
	
		clear_flow_param(flow_param);
		return NULL;
}

gn_flow_t * install_modifine_ExternalSwitch_QOS_in_goto_NATTable_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = FABRIC_FQ_IN_POST_PROCESS_NAT_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);

	install_fabric_flows(sw, FABRIC_QOS_IN_IDLE_TIME_OUT, FABRIC_QOS_IN_HARD_TIME_OUT, FABRIC_QOS_FIREWALL_IN_DEFAULT_FLOW,
						 FABRIC_QOS_IN_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

/* by:yhy
 * 修改external switch pop vlan的那条流表,跳转防火墙出表
 */
gn_flow_t * install_modifine_ExternalSwitch_goto_FirewallOutTable_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = FABRIC_FIREWALL_OUT_TABLE;

	flow_param->match_param->vlan_vid = (UINT2)of131_fabric_impl_get_tag_sw(sw);
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_POP_VLAN, NULL);

	if (OFP13_VERSION == sw->ofp_version) 
	{
		install_fabric_flows(sw, 
							 FABRIC_IMPL_IDLE_TIME_OUT, 
							 FABRIC_IMPL_HARD_TIME_OUT, 
							 FABRIC_PRIORITY_SWAPTAG_FLOW,
							 FABRIC_INPUT_TABLE, 
							 OFPFC_ADD, 
							 flow_param);
	}
	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_remove_QOS_jump_flow(gn_switch_t *sw,UINT1 FlowTable,UINT4 SrcIP)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(SrcIP);

	install_fabric_flows(sw, 0, 0, 0, FlowTable, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_QOS_jump_with_meter_flow(gn_switch_t *sw,UINT1 FlowTable,UINT1 GotoTable,UINT4 SrcIP,UINT4 MeterID)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->ipv4_src = ntohl(SrcIP);
	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_METER, (void*)&MeterID);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&GotoTable);
	
	install_fabric_flows(sw, FABRIC_QOS_JUMP_IDLE_TIME_OUT, FABRIC_QOS_JUMP_HARD_TIME_OUT, FABRIC_PRIORITY_QOS_JUMP_FLOW,FlowTable, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_QOS_jump_without_meter_flow(gn_switch_t *sw,UINT1 FlowTable,UINT1 GotoTable,UINT4 SrcIP)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	/*
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->ipv4_src = ntohl(SrcIP);
	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&GotoTable);

	install_fabric_flows(sw, FABRIC_QOS_JUMP_IDLE_TIME_OUT, FABRIC_QOS_JUMP_HARD_TIME_OUT, FABRIC_PRIORITY_QOS_JUMP_FLOW,FlowTable, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	*/
	return NULL;
}
gn_flow_t * install_add_QOS_jump_with_queue_flow(gn_switch_t *sw,UINT1 FlowTable,UINT1 GotoTable,UINT4 SrcIP,UINT4 QueueID)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->ipv4_src = ntohl(SrcIP);
	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_QUEUE, (void*)&QueueID);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&GotoTable);
	
	install_fabric_flows(sw, FABRIC_QOS_JUMP_IDLE_TIME_OUT, FABRIC_QOS_JUMP_HARD_TIME_OUT, FABRIC_PRIORITY_QOS_JUMP_FLOW,FlowTable, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
gn_flow_t * install_add_QOS_jump_without_queue_flow(gn_switch_t *sw,UINT1 FlowTable,UINT1 GotoTable,UINT4 SrcIP)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->ipv4_src = ntohl(SrcIP);
	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&GotoTable);
	
	install_fabric_flows(sw, FABRIC_QOS_JUMP_IDLE_TIME_OUT, FABRIC_QOS_JUMP_HARD_TIME_OUT, FABRIC_PRIORITY_QOS_JUMP_FLOW,FlowTable, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	
	return NULL;
}
gn_flow_t * install_add_QOS_jump_default_flow(gn_switch_t *sw,UINT1 FlowTable,UINT1 GotoTable)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&GotoTable);
	
	install_fabric_flows(sw, FABRIC_QOS_JUMP_IDLE_TIME_OUT, FABRIC_QOS_JUMP_HARD_TIME_OUT, FABRIC_PRIORITY_QOS_DEFAULT_JUMP_FLOW,FlowTable, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}

gn_flow_t * install_add_QOS_in_jump_default_flow(gn_switch_t *sw,UINT1 FlowTable,UINT1 GotoTable)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&GotoTable);
	
	install_fabric_flows(sw, FABRIC_QOS_JUMP_IDLE_TIME_OUT, FABRIC_QOS_JUMP_HARD_TIME_OUT, FABRIC_PRIORITY_QOS_DEFAULT_JUMP_FLOW,FlowTable, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}


gn_flow_t * install_add_FQ_out_PostProcessTable_default_flow(gn_switch_t *sw)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return NULL;
	}
	flow_param_t* flow_param = init_flow_param();
	UINT1 TargetTable =FABRIC_SWAPTAG_TABLE;
	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&TargetTable);
	
	install_fabric_flows(sw, FABRIC_FQ_OUT_POST_PROCESS_IDLE_TIME_OUT, FABRIC_FQ_OUT_POST_PROCESS_HARD_TIME_OUT, FABRIC_PRIORITY_FQ_OUT_POST_PROCESS_DEFAULT_FLOW,FABRIC_FQ_OUT_POST_PROCESS_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}



INT4 install_add_FloatingIP_ToFixIP_OutputToHost_flow(UINT4 floatingip)
{
	INT4 ret = 0;
	UINT4 fixedip = 0;
	UINT2 out_port = 0;
	gn_switch_t* sw = NULL;
	external_floating_ip_p efp = NULL;
	p_fabric_host_node hostNode=NULL;
	flow_param_t* flow_param = init_flow_param();
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	
	efp =  find_external_fixed_ip_by_floating_ip(floatingip);
	if(NULL == efp)
	{
		LOG_PROC("INFO"," %s_%d \n",FN,LN);
		clear_flow_param(flow_param);
		return 0;
	}
	
	hostNode = get_fabric_host_from_list_by_ip(efp->fixed_ip);
	if(NULL == hostNode)
	{
		LOG_PROC("INFO"," %s_%d \n",FN,LN);
		clear_flow_param(flow_param);
		return 0;
	}
	out_port = hostNode->port;
	sw = hostNode->sw;
	if((NULL == sw)||(0 == out_port))
	{
		LOG_PROC("INFO"," %s_%d \n",FN,LN);
		clear_flow_param(flow_param);
		return 0;
	}
	oxm.ipv4_dst = ntohl(efp->fixed_ip);

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(floatingip);

	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);

	  LOG_PROC("INFO"," %s_%d floatingip=0x%x\n",FN,LN,floatingip);
	ret = install_fabric_flows(sw, FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_IDLE_TIME_OUT, FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_HARD_TIME_OUT, FABRIC_PRIORITY_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW,FABRIC_OUTPUT_TABLE, OFPFC_ADD, flow_param);


	clear_flow_param(flow_param);

	return ((ret == GN_OK) ? 1:0);
}

gn_flow_t * install_remove_FloatingIP_ToFixIP_OutputToHost_flow(UINT4 floatingip)
{
	UINT4 fixedip = 0;
	UINT2 out_port = 0;
	gn_switch_t* sw = NULL;
	external_floating_ip_p efp = NULL;
	p_fabric_host_node hostNode=NULL;
	flow_param_t* flow_param = init_flow_param();
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	
	
	efp =  find_external_fixed_ip_by_floating_ip(floatingip);
	if(NULL == efp)
	{
		LOG_PROC("INFO"," %s_%d \n",FN,LN);
		clear_flow_param(flow_param);
		return NULL;
	}
	
	hostNode = get_fabric_host_from_list_by_ip(efp->fixed_ip);
	if(NULL == hostNode)
	{
		LOG_PROC("INFO"," %s_%d \n",FN,LN);
		clear_flow_param(flow_param);
		return NULL;
	}
	out_port = hostNode->port;
	sw = hostNode->sw;
	if((NULL == sw)||(0 == out_port))
	{
		LOG_PROC("INFO"," %s_%d \n",FN,LN);
		clear_flow_param(flow_param);
		return NULL;
	}
	oxm.ipv4_dst = ntohl(efp->fixed_ip);

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(floatingip);

	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);

	
	LOG_PROC("INFO"," %s_%d floatingip=0x%x\n",FN,LN,floatingip);
	install_fabric_flows(sw, FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_IDLE_TIME_OUT, FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_HARD_TIME_OUT, FABRIC_PRIORITY_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW,FABRIC_OUTPUT_TABLE, OFPFC_DELETE, flow_param);


	clear_flow_param(flow_param);

	return NULL;
}
void install_fabric_same_switch_pregototable(gn_switch_t * sw,UINT1* mac, UINT2 flow_table_id, UINT2 goto_table_id)
{
	flow_param_t* flow_param = init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&goto_table_id);

	install_fabric_flows(sw, 
						 FABRIC_IMPL_HARD_TIME_OUT, 
						 FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 flow_table_id, 
						 OFPFC_ADD, 
						 flow_param);

	clear_flow_param(flow_param);
}


void install_fabric_same_switch_security_before_OutFirewallQOS_flow(gn_switch_t * sw,UINT1* mac, UINT4 port, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = FABRIC_FIREWALL_OUT_TABLE;

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);

	install_fabric_flows(sw, 
						 FABRIC_HOST_TO_HOST_UNDER_SAME_OVS_AND_SAME_SUBNET_IDLE_TIME_OUT, 
						 FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_INPUT_TABLE, 
						 OFPFC_ADD, 
						 flow_param);

	clear_flow_param(flow_param);
}
void install_fabric_same_switch_security_flow(gn_switch_t * sw,UINT1* mac, UINT4 port, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = FABRIC_FIREWALL_IN_TABLE;

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);

	install_fabric_flows(sw, 
						 FABRIC_HOST_TO_HOST_UNDER_SAME_OVS_AND_SAME_SUBNET_IDLE_TIME_OUT, 
						 FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_FQ_OUT_POST_PROCESS_TABLE, 
						 OFPFC_ADD, 
						 flow_param);

	clear_flow_param(flow_param);
}
void install_fabric_same_switch_out_subnet_pregototable(gn_switch_t * sw,UINT1* gateway_mac,UINT4 dst_ip, UINT2 flow_table_id, UINT2 goto_table_id)
{
	flow_param_t* flow_param = init_flow_param();


	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_dst, gateway_mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&goto_table_id);


	install_fabric_flows(sw, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 flow_table_id, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

}


void install_fabric_same_switch_out_subnet_flow(gn_switch_t * sw,UINT1* gateway_mac,UINT1* dst_mac,UINT4 dst_ip,UINT4 port, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, dst_mac, 6);

	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_dst, gateway_mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);


	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

};
//by:yhy 依据输入参数装载fabric输出流表
//表:  table=FABRIC_OUTPUT_TABLE
//匹配:eth_dst=mac
//动作:output:port
void install_fabric_output_flow(gn_switch_t * sw,UINT1* mac, UINT4 port)
{
	if(0 == port)
	{
		return ;
	}
	flow_param_t* flow_param = init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);
	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, 
						 FABRIC_IMPL_IDLE_TIME_OUT, 
						 FABRIC_ARP_HARD_TIME_OUT, 
						 FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_OUTPUT_TABLE, 
						 OFPFC_ADD, 
						 flow_param);

	clear_flow_param(flow_param);

};
//by:yhy  why?具体没看懂
void install_fabric_push_tag_portflow(gn_switch_t * sw,UINT1* mac, UINT4 tag, UINT4 out_port)
{
    if (0 == tag)
    {
        return;
    }

	flow_param_t* flow_param = init_flow_param();

	UINT2 flow_table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_INPUT_TABLE;
	UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_ARP_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)tag;

	memcpy(flow_param->match_param->eth_dst, mac, 6);
	// flow_param->match_param->mask |= (1 << OFPXMT_OFB_ETH_DST);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	// set_security_match(flow_param->match_param, security_param);

	if (0 == get_nat_physical_switch_flag()) 
	{
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else 
	{
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);
	}
	if (OFP13_VERSION == sw->ofp_version) 
	{
		install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, priority_level,flow_table_id, OFPFC_ADD, flow_param);
	}

	clear_flow_param(flow_param);
	return ;

};

void install_fabric_push_tag_security_flow(gn_switch_t * sw,UINT4 dst_ip, UINT1* mac, UINT4 tag, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)tag;

	// memcpy(flow_param->match_param->eth_dst, mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_OTHER_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

void install_fabric_push_tag_security_AddLocalDstIp_pregototable(gn_switch_t * sw,UINT4 dst_ip, UINT2 flow_table_id, UINT2 goto_table_id)
{
	flow_param_t* flow_param = init_flow_param();


	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&goto_table_id);


	install_fabric_flows(sw, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 flow_table_id, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

void install_fabric_push_tag_security_AddLocalSrcMAC_postgototable(gn_switch_t * sw,UINT4 dst_ip, UINT1* LocalSrcMAC, UINT2 flow_table_id, UINT2 goto_table_id)
{
	flow_param_t* flow_param = init_flow_param();


	memcpy(flow_param->match_param->eth_src, LocalSrcMAC, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&goto_table_id);


	install_fabric_flows(sw, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 flow_table_id, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}
void install_fabric_push_tag_security_AddLocalSrc_postgototable(gn_switch_t * sw,UINT4 src_ip, UINT2 flow_table_id, UINT2 goto_table_id)
{
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(src_ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&goto_table_id);


	install_fabric_flows(sw, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 flow_table_id, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

//by:yhy 不同compute节点上的虚机通信
void install_fabric_push_tag_security_flow_AddLocalSrcMAC(gn_switch_t * sw,UINT4 dst_ip, UINT1* LocalSrcMAC,UINT1* mac, UINT4 tag, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_FIREWALL_OUT_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)tag;

	memcpy(flow_param->match_param->eth_src, LocalSrcMAC, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_OTHER_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}



void install_fabric_push_tag_out_subnet_flow(gn_switch_t * sw,UINT1* gateway_mac,UINT1* dst_mac,UINT4 dst_ip,UINT4 tag, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)tag;
	memcpy(oxm.eth_dst, dst_mac, 6);

	memcpy(flow_param->match_param->eth_dst, gateway_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst=ntohl(dst_ip);

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
			FABRIC_OTHER_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

};

UINT1 get_fabric_last_flow_table(){
	return (UINT1)FABRIC_INPUT_TABLE;
};
UINT1 get_fabric_first_flow_table(){
	return (UINT1)FABRIC_SWAPTAG_TABLE;
};
UINT1 get_fabric_middle_flow_table(){
	return (UINT1)FABRIC_INPUT_TABLE;
};
/**************************************
 * Intern flow functions
 **************************************/
//by:yhy 所有IP包跳转table:2解析
//add host input flow
/*
 * table 0;( input table)
 * match: ip ;
 * action: goto table 1 (push tag table)
 * priority: 0 (host in put flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
gn_flow_t * install_add_fabric_host_input_flow(gn_switch_t *sw)
{
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = FABRIC_PUSHTAG_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_HOST_INPUT_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
//add ip missmatch push tag flow
//by:yhy 所有table:2中没有匹配到的IP包,上送控制器
/*
 * table 1;( push tag table)
 * match: ip ;
 * action: goto controller
 * priority: 0 (missmatch push tag flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
gn_flow_t * install_add_fabric_miss_match_flow(gn_switch_t *sw)
{
	flow_param_t* flow_param = init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;

	flow_param->match_param->eth_type = ETHER_IP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_MISSMATCH_PUSHTAG_FLOW,
						 FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
//add arp missmatch push tag flow
/* markby:yhy 
 * table 0;(input table)
 * match: arp;
 * action: goto controller
 * priority: 5 (arp missmatch push tag flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
//by:yhy 下发流表(匹配IP为g_reserve_ip的包上送控制器)
INT4 install_add_fabric_controller_flow(gn_switch_t *sw)
{
	INT4 ret = 0;
    if (0 == g_reserve_ip) 
	{
        return 0;
    }

	flow_param_t* flow_param = init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;

	flow_param->match_param->eth_type = ETHER_IP;
    flow_param->match_param->ipv4_dst= g_reserve_ip;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	ret = install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_SWAPTAG_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);
	

	clear_flow_param(flow_param);
	return ((ret == GN_OK) ? 1:0);
}

void install_add_fabric_lldpmiss_controller_flow(gn_switch_t *sw)
{

	flow_param_t* flow_param = init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;


	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, 0,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

void install_add_fabric_toVm_controller_flow(gn_switch_t *sw)
{
	flow_param_t* flow_param = init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;
	flow_param->match_param->eth_type = ETHER_IP;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, 0,
						 FABRIC_OUTPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

//by:yhy 所有ARP包转送控制器
/*
 * table 0;(input table)
 * match: arp;
 * action: goto controller
 * priority: 5 (arp missmatch push tag flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
gn_flow_t * install_add_fabric_ARP_miss_match_flow(gn_switch_t *sw)
{
	flow_param_t* flow_param = init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;

	flow_param->match_param->eth_type = ETHER_ARP;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_MISSMATCH_INPUT_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;

}
//add switch input flows
/*
 * table 0;( input table)
 * match: ip ; input port;
 * action: goto table 1 (push tag table)
 * priority: 10 (switch in put flow priority)
 * idletimeout: 0 (forever)
 * hardtimeout: 0 (forever)
 */
gn_flow_t * install_add_fabric_switch_input_flow(gn_switch_t *sw,gn_port_t* sw_port)
{
	// unused function
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = FABRIC_SWAPTAG_TABLE;

	flow_param->match_param->in_port = sw_port->port_no;
	
	add_action_param(&flow_param->instruction_param, OFPIT_CLEAR_ACTIONS, (void*)&table_id);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_SWITCH_INPUT_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return NULL;
}
/*
 * delete fabric flow real
 */
//by:yhy 清空所有流表
void install_delete_fabric_flow(gn_switch_t* sw)
{
	flow_param_t* flow_param = init_flow_param();

	install_fabric_flows(sw, 0, 0, 0, 0, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
};

void install_delete_fabric_impl_flow(gn_switch_t *sw,UINT4 port_no,UINT4 tag,UINT1 table_id)
{
	// unused function
	flow_param_t* flow_param = init_flow_param();
	UINT4 port = port_no;
	UINT1 flow_table_id = table_id;

	flow_param->match_param->vlan_vid = (UINT2)tag;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);

	install_fabric_flows(sw, 0, 0, 0, flow_table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);

};
//by:yhy 对sw下载匹配vlan_vid=tag,然后执行pop_vlan并跳转至FABRIC_OUTPUT_TABLE的流表
gn_flow_t * install_add_fabric_impl_last_flow(gn_switch_t * sw,UINT4 tag)
{
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = FABRIC_FIREWALL_IN_TABLE;

	flow_param->match_param->vlan_vid = (UINT2)tag;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_POP_VLAN, NULL);

	if (OFP13_VERSION == sw->ofp_version) 
	{
		install_fabric_flows(sw, 
							 FABRIC_IMPL_IDLE_TIME_OUT, 
							 FABRIC_IMPL_HARD_TIME_OUT, 
							 FABRIC_PRIORITY_SWAPTAG_FLOW,
							 FABRIC_INPUT_TABLE, 
							 OFPFC_ADD, 
							 flow_param);
	}

	clear_flow_param(flow_param);

	return NULL;
};
//by:yhy 对switch匹配vlan_vid=tag的包跳转到FABRIC_SWAPTAG_TABLE,从port输出
gn_flow_t * install_add_fabric_impl_first_flow(gn_switch_t * sw,UINT4 port,UINT4 tag)
{
	flow_param_t* flow_param = init_flow_param();
	UINT1 table_id = (OFP10_VERSION == sw->ofp_version) ? FABRIC_INPUT_TABLE : FABRIC_SWAPTAG_TABLE;

	UINT4 outport = port;

	flow_param->match_param->vlan_vid = (UINT2)tag;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&outport);

	install_fabric_flows(sw, 
						 FABRIC_IMPL_IDLE_TIME_OUT, 
						 FABRIC_IMPL_HARD_TIME_OUT, 
						 FABRIC_PRIORITY_SWAPTAG_FLOW,
						 table_id, 
						 OFPFC_ADD, 
						 flow_param);

	clear_flow_param(flow_param);

	return NULL;

};
gn_flow_t * install_add_fabric_impl_middle_flow(gn_switch_t * sw,UINT4 port,UINT4 tag)
{
	flow_param_t* flow_param = init_flow_param();
	UINT4 outport = port;

	flow_param->match_param->vlan_vid = (UINT2)tag;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&outport);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_PRIORITY_SWAPTAG_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

	return NULL;
};
//by:yhy 删除openstack中对外交换数据的流表
void remove_fabric_openstack_external_output_flow(gn_switch_t * sw, UINT1* gateway_mac,UINT4 outer_interface_ip, UINT1 type)
{
	LOG_PROC("INFO", "------------------%s",FN);
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_OUTPUT_TABLE;

	if (1 == type) 
	{
	    memcpy(flow_param->match_param->eth_dst, gateway_mac, 6);
	}
	else if(2 == type)
	{
		flow_param->match_param->ipv4_src = ntohl(outer_interface_ip);
		flow_param->match_param->eth_type = ETHER_IP;
	}
	else 
	{
		// do nothing
	}

	if (OFP10_VERSION == sw->ofp_version) 
	{
		flow_table_id = FABRIC_INPUT_TABLE;
		UINT4 tag = of131_fabric_impl_get_tag_dpid(sw->dpid);
		flow_param->match_param->vlan_vid = tag;
		add_action_param(&flow_param->action_param, OFPAT13_POP_VLAN, (void*)&tag);
	}

	install_fabric_flows(sw, 0, 0, 0, flow_table_id, OFPFC_DELETE, flow_param);
	clear_flow_param(flow_param);
}

//by:yhy 装载openstack对外输出流表
void install_fabric_openstack_external_output_flow(gn_switch_t * sw,UINT4 port,UINT1* gateway_mac,UINT4 outer_interface_ip,UINT1 type){

	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_OUTPUT_TABLE;

	if (1 == type) 
	{
	    memcpy(flow_param->match_param->eth_dst, gateway_mac, 6);
	}
	else if(2 == type)
	{
		flow_param->match_param->ipv4_src = ntohl(outer_interface_ip);
		flow_param->match_param->eth_type = ETHER_IP;
	}
	else 
	{
	}

	if (OFP10_VERSION == sw->ofp_version) 
	{
		flow_table_id = FABRIC_INPUT_TABLE;
		UINT4 tag = of131_fabric_impl_get_tag_dpid(sw->dpid);
		flow_param->match_param->vlan_vid = tag;
		add_action_param(&flow_param->action_param, OFPAT13_POP_VLAN, (void*)&tag);
	}

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);


	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 flow_table_id, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	return;
}
//by:yhy 根据给定参数,对流表进行操作并下发(why? 未使用)
void install_fabric_openstack_floating_internal_subnet_flow(gn_switch_t* sw, INT4 type, UINT4 dst_ip, UINT4 dst_mask, security_param_t* security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_OTHER_TABLE;//FABRIC_PUSHTAG_TABLE;
	UINT1 action = 0;
	if (1 == type) 
	{
		action = OFPFC_ADD;
	}
	else if (2 == type) 
	{
		action = OFPFC_DELETE;
	}
	else 
	{
		clear_flow_param(flow_param);
		return ;
	}

	flow_param->match_param->eth_type = ETHER_IP;
	// flow_param->match_param->ipv4_src = ntohl(match_ip);
	flow_param->match_param->ipv4_dst= ntohl(dst_ip);
	flow_param->match_param->ipv4_dst_prefix = (dst_mask);

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);

	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, 
						 FABRIC_IMPL_HARD_TIME_OUT, 
						 FABRIC_IMPL_IDLE_TIME_OUT, 
						 FABRIC_PRIORITY_FLOATING_INTERNAL_SUBNET_FLOW,
						 FABRIC_INPUT_TABLE, 
						 action, flow_param);

	clear_flow_param(flow_param);

}

//by:yhy  type(1添加2删除) 根据参数下达对external_flow流表操作 (why? 目测也没被调用)
//表:  FABRIC_INPUT_TABLE
//超时:空闲=0;硬=0;
//优先:FABRIC_PRIORITY_FLOATING_EXTERNAL_HOST_SUBNET_FLOW
//匹配:ipv4_src=match_ip
//动作:更改ipv4_src->mod_src_ip;eth_dst->mod_dst_mac;vlan_vid->vlan_id
//    goto_table:FABRIC_SWAPTAG_TABLE
INT4 install_proactive_floating_host_to_external_flow(gn_switch_t* sw, INT4 type, UINT4 match_ip, UINT1* match_mac, UINT4 mod_src_ip, UINT1* mod_dst_mac, UINT4 vlan_id, security_param_t* security_param)
{
	INT4 ret = 0;
	flow_param_t* flow_param = init_flow_param();
	UINT1 action = 0;
	if (1 == type) 
	{
		action = OFPFC_ADD;
	}
	else if (2 == type) 
	{
		action = OFPFC_DELETE;
	}
	else 
	{
		clear_flow_param(flow_param);
		return 0;
	}


	UINT2 table_id = FABRIC_FIREWALL_OUT_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.ipv4_src = ntohl(mod_src_ip);
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.vlan_vid = vlan_id;

	memcpy(flow_param->match_param->eth_src, match_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(match_ip);

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	ret = install_fabric_flows(sw, 
						 FABRIC_IMPL_HARD_TIME_OUT, 
						 FABRIC_IMPL_IDLE_TIME_OUT, 
						 FABRIC_PRIORITY_FLOATING_EXTERNAL_HOST_SUBNET_FLOW,
						 FABRIC_FLOATING_TABLE, 
						 action, 
						 flow_param);

	clear_flow_param(flow_param);
	return 1;
}
//by:yhy type(1添加2删除)  装载 对外浮动ip 操作 进入负载平衡组流表
void install_proactive_floating_external_to_lbaas_group_flow(gn_switch_t* sw, INT4 type, UINT4 floatingip, UINT4 tcp_dst, UINT4 group_id)
{    
    flow_param_t* flow_param = init_flow_param();
    UINT1 action = 0;    
    if (1 == type) 
	{
        action = OFPFC_ADD;
        add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
        add_action_param(&flow_param->action_param, OFPAT13_GROUP, (void*)&group_id);
    }
    else if (2 == type) 
	{
        action = OFPFC_DELETE;
    }
    else 
	{
		clear_flow_param(flow_param);
        return ;
    }

    flow_param->match_param->eth_type = ETHER_IP;
    flow_param->match_param->ip_proto = IPPROTO_TCP;
    flow_param->match_param->ipv4_dst = ntohl(floatingip);
    flow_param->match_param->tcp_dst = tcp_dst;

    // set_security_match(flow_param->match_param, security_param);

    install_fabric_flows(sw, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_PRIORITY_FLOATING_LBAAS_FLOW, 
            FABRIC_INPUT_TABLE, action, flow_param);

    clear_flow_param(flow_param);

}
//by:yhy type(1添加2删除)装载主动 浮动ip 负载平衡 到外部流表
void install_proactive_floating_lbaas_to_external_flow(gn_switch_t* sw, INT4 type, UINT4 host_ip, 
    UINT2 host_tcp_dst, UINT1* ext_mac, UINT4 ext_vlan_id, UINT4 floatingip, UINT2 ext_tcp_dst, UINT1* floatingmac)
{    
    flow_param_t* flow_param = init_flow_param();
    UINT1 action = 0;
    if (1 == type) 
	{
      action = OFPFC_ADD;
    }
    else if (2 == type) 
	{
      action = OFPFC_DELETE;
    }
    else 
	{
		clear_flow_param(flow_param);
      return ;
    }

    UINT2 table_id = FABRIC_SWAPTAG_TABLE;

    gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.ipv4_src = ntohl(floatingip);
	memcpy(oxm.eth_dst, ext_mac, 6);
    memcpy(oxm.eth_src, floatingmac, 6);
	oxm.vlan_vid = ext_vlan_id;
    oxm.tcp_src = ext_tcp_dst;

    flow_param->match_param->eth_type = ETHER_IP;
    flow_param->match_param->ip_proto = IPPROTO_TCP;
    flow_param->match_param->ipv4_src = ntohl(host_ip);
    flow_param->match_param->tcp_src = host_tcp_dst;

    add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);


    // set_security_match(flow_param->match_param, security_param);

    install_fabric_flows(sw, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_PRIORITY_FLOATING_EXTERNAL_GROUP_SUBNET_FLOW,
          FABRIC_INPUT_TABLE, action, flow_param);

    clear_flow_param(flow_param);

}

void install_lbaas_member_to_listener_flow(gn_switch_t* sw, INT4 type, UINT4 listener_ip)
{
    flow_param_t* flow_param = init_flow_param();
    UINT1 action = 0;    
    if (1 == type) {
        action = OFPFC_ADD;
    }
    else if (2 == type) {
        action = OFPFC_DELETE;
    }
    else {
		clear_flow_param(flow_param);
        return ;
    }
    
    UINT2 table_id = FABRIC_SWAPTAG_TABLE;

    flow_param->match_param->eth_type = ETHER_IP;
    flow_param->match_param->ipv4_dst = ntohl(listener_ip);

    add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
    add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);

    // set_security_match(flow_param->match_param, security_param);

    install_fabric_flows(sw, FABRIC_IMPL_HARD_TIME_OUT, FABRIC_IMPL_IDLE_TIME_OUT, 8, 
            FABRIC_INPUT_TABLE, action, flow_param);

    clear_flow_param(flow_param);

}


void fabric_openstack_floating_ip_install_set_vlan_out_flow(gn_switch_t * sw, UINT4 match_ip, UINT1* match_mac, UINT4 mod_src_ip, UINT1* mod_dst_mac, UINT4 vlan_id, security_param_t* security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_QOS_OUT_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.ipv4_src = ntohl(mod_src_ip);
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.vlan_vid = vlan_id;

	memcpy(flow_param->match_param->eth_src, match_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_FLOATING_FLOW_IDLE_TIME_OUT, FABRIC_FLOATING_FLOW_HARD_TIME_OUT, FABRIC_PRIORITY_FLOATING_FLOW,FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

//by:yhy 下发浮动ip相关包添加vlan流表
/* by:Hongyu Yang
 * 在pica8 上下浮动ip进入后转换的流表
 */
void fabric_openstack_floating_ip_install_set_vlan_in_flow(gn_switch_t * sw, UINT4 match_ip, UINT4 mod_dst_ip, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_FQ_IN_POST_PROCESS_FLOATING_TABLE;
	UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_FLOATING_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	//oxm.ipv4_dst = ntohl(mod_dst_ip);
	oxm.vlan_vid = vlan_id;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (0 == get_nat_physical_switch_flag()) 
	{
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else 
	{
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);
	}

	install_fabric_flows(sw, 
						 FABRIC_IMPL_HARD_TIME_OUT, 
						 FABRIC_IMPL_IDLE_TIME_OUT, 
						 priority_level,
						 flow_table_id, 
						 OFPFC_ADD, 
						 flow_param);

	clear_flow_param(flow_param);
}

void fabric_openstack_floating_ip_clear_stat(gn_switch_t * sw, UINT4 match_ip, UINT4 mod_dst_ip, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_INPUT_TABLE;
	UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_FLOATING_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.ipv4_dst = ntohl(mod_dst_ip);
	oxm.vlan_vid = vlan_id;
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (0 == get_nat_physical_switch_flag()) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);
	}
	install_fabric_clear_stat_flows(sw, FABRIC_FLOATING_FLOW_IDLE_TIME_OUT, FABRIC_FLOATING_FLOW_HARD_TIME_OUT, priority_level,
			flow_table_id, OFPFC_ADD, flow_param);
	clear_flow_param(flow_param);
}
void fabric_openstack_portforward_ip_install_set_vlan_out_flow(gn_switch_t * sw, UINT4 match_ip, UINT1 match_proto, UINT2 match_ip_src_port,  UINT1* match_mac, UINT4 mod_src_ip, UINT2 mod_ip_src_port,   UINT1* mod_dst_mac,UINT4 vlan_id, security_param_t* security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.ipv4_src = ntohl(mod_src_ip);
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.vlan_vid = vlan_id;

	if(PROTO_TCP== match_proto)
	{
		oxm.tcp_src = mod_ip_src_port;//23;
	}
	if(PROTO_UDP == match_proto)
	{
		oxm.udp_src = mod_ip_src_port;//23;
	}

	memcpy(flow_param->match_param->eth_src, match_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);
	flow_param->match_param->ip_proto =  match_proto; //IPPROTO_TCP;

	if(PROTO_TCP== match_proto)
	{
		flow_param->match_param->tcp_src = match_ip_src_port; //22;
	}
	if(PROTO_UDP == match_proto)
	{
		flow_param->match_param->udp_src = match_ip_src_port; //22;
	}	
	
	

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	//set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_PORTFORWARD_FLOW,
			FABRIC_FQ_OUT_POST_PROCESS_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}
void fabric_openstack_portforward_ip_install_set_vlan_out_before_OutFirewallQOS_flow(gn_switch_t * sw, UINT4 match_ip, UINT1 match_proto, UINT2 match_ip_src_port, UINT2 match_ip_dst_port, UINT1* match_mac, UINT4 mod_src_ip, UINT2 mod_ip_src_port,   UINT1* mod_dst_mac,UINT4 vlan_id, security_param_t* security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_FIREWALL_OUT_TABLE;

	memcpy(flow_param->match_param->eth_src, match_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);
	flow_param->match_param->ip_proto =  match_proto; //IPPROTO_TCP;

	if(PROTO_TCP== match_proto)
	{
		flow_param->match_param->tcp_src = match_ip_src_port; //22;
		flow_param->match_param->tcp_dst = match_ip_dst_port;
	}
	if(PROTO_UDP== match_proto)
	{
		flow_param->match_param->udp_src = match_ip_src_port; //22;
		flow_param->match_param->udp_dst = match_ip_dst_port;
	}
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);


	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_PORTFORWARD_FLOW,
						 FABRIC_FLOATING_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}


void fabric_openstack_portforward_ip_install_set_vlan_in_flow(gn_switch_t * sw, UINT4 match_ip, UINT1 match_proto, UINT2 match_ip_src_port,UINT2 match_ip_dst_port, UINT4 match_src_ip, UINT4 mod_dst_ip,  UINT2 mod_ip_dst_port, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_FQ_IN_POST_PROCESS_PORTFORD_TABLE;
	
	UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_PORTFORWARD_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.ipv4_dst = ntohl(mod_dst_ip);
	oxm.vlan_vid = vlan_id;
	if(PROTO_TCP== match_proto)
	{
		oxm.tcp_dst = mod_ip_dst_port; //22;
	}
	if(PROTO_UDP == match_proto)
	{
		oxm.udp_dst = mod_ip_dst_port; //22;
	}
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(match_src_ip);
	flow_param->match_param->ipv4_dst = ntohl(match_ip);
	flow_param->match_param->ip_proto =  match_proto;//IPPROTO_TCP;

	if(PROTO_TCP== match_proto)
	{
		flow_param->match_param->tcp_dst = match_ip_dst_port;//23;
		flow_param->match_param->tcp_src = match_ip_src_port;
	}
	if(PROTO_UDP == match_proto)
	{
		flow_param->match_param->udp_dst = match_ip_dst_port;//23;
		flow_param->match_param->udp_src = match_ip_src_port;
	}
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (0 == get_nat_physical_switch_flag()) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);
	}

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, priority_level,
			flow_table_id, OFPFC_ADD, flow_param);


	clear_flow_param(flow_param);

}

gn_flow_t * install_add_PortForwardIP_ToFixIP_OutputToHost_flow(gn_switch_t * sw, UINT4 match_ip, UINT1 match_proto, UINT2 match_ip_dst_port, UINT4 mod_dst_ip, UINT2 mod_ip_dst_port, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_PUSHTAG_TABLE;
	
	UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_PORTFORWARD_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.ipv4_dst = ntohl(mod_dst_ip);
	oxm.vlan_vid = vlan_id;
	oxm.tcp_dst = mod_ip_dst_port; //22;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);
	flow_param->match_param->ip_proto =  match_proto;//IPPROTO_TCP;
	flow_param->match_param->tcp_dst = match_ip_dst_port;//23;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (0 == get_nat_physical_switch_flag()) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);
	}

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, priority_level,
			flow_table_id, OFPFC_ADD, flow_param);


	clear_flow_param(flow_param);

}

//华云转发，匹配目的ip，修改mac地址
void fabric_openstack_clbforward_ip_install_set_vlan_out_flow(gn_switch_t * sw, UINT4 match_ip, UINT1* match_mac, UINT1* mod_dst_mac,UINT4 vlan_id, security_param_t* security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.vlan_vid = vlan_id;

	memcpy(flow_param->match_param->eth_src, match_mac, 6);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);

	

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_CLBFORWARD_FLOW,
			FABRIC_OTHER_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

}
//华云转发，匹配目的ip，修改mac地址
void fabric_openstack_clbforward_ip_install_set_vlan_in_pregototable(gn_switch_t * sw, UINT4 match_ip, UINT2 flow_table_id, UINT2 goto_table_id)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 priority_level =FABRIC_PRIORITY_CLBFORWARD_FLOW;
	
	
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&goto_table_id);
	
	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, priority_level,
			flow_table_id, OFPFC_ADD, flow_param);


	clear_flow_param(flow_param);
	
}

void fabric_openstack_clbforward_ip_install_set_vlan_in_flow(gn_switch_t * sw, UINT4 match_ip, UINT1* mod_dst_mac, UINT4 vlan_id, UINT4 out_port)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_FQ_IN_POST_PROCESS_PORTFORD_TABLE;
	
	UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_CLBFORWARD_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	
	oxm.vlan_vid = vlan_id;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(match_ip);
	
	LOG_PROC("INFO", "####### fabric_openstack_clbforward_ip_install_set_vlan_in_flow %s %d--fabric_openstack_clbforward_ip_install_set_vlan_in_flow ",FN,LN);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (0 == get_nat_physical_switch_flag()) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);
	}

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, priority_level,
			flow_table_id, OFPFC_ADD, flow_param);


	clear_flow_param(flow_param);

}

//华云转发,组播
void fabric_openstack_clbforward_multicastip_pregototable(gn_switch_t * sw, UINT1 *match_dst_mac, UINT2 flow_table_id, UINT2 goto_table_id)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 priority_level = FABRIC_PRIORITY_CLBFORWARD_FLOW;
	if(NULL == sw)
	{
		clear_flow_param(flow_param);
		return ;
	}
	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_dst, match_dst_mac, 6);	

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&goto_table_id);

	install_fabric_flows(sw, FABRIC_IMPL_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, priority_level,
			flow_table_id, OFPFC_ADD, flow_param);


	clear_flow_param(flow_param);
	
}

void fabric_openstack_clbforward_multicastip_install_flow(gn_switch_t * src_sw,gn_switch_t * dst_sw, UINT4 match_src_ip, UINT4 match_dst_ip,UINT1 *mod_dst_mac, UINT1 match_proto, UINT4 vlan_id)
{
	UINT4 out_port = 0;
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_INPUT_TABLE;
	
	UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_CLBFORWARD_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	UINT2 table_id = FABRIC_SWAPTAG_TABLE;

	if((NULL == src_sw)||(NULL == mod_dst_mac))
	{
		clear_flow_param(flow_param);
		return ;
	}

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, mod_dst_mac, 6);
	
	oxm.vlan_vid = vlan_id;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(match_src_ip);
	flow_param->match_param->ipv4_dst = ntohl(match_dst_ip);
	flow_param->match_param->ip_proto = match_proto;
	
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	
	if (0 == get_nat_physical_switch_flag()) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		if((NULL == src_sw)||(NULL == dst_sw))
		{
			clear_flow_param(flow_param);
			LOG_PROC("ERROR","%s %d there is not exist src_sw or dst_sw",FN,LN);
			return ;
		}
		out_port = get_out_port_between_switch(src_sw->dpid, dst_sw->dpid);
		if(0 == out_port)
		{
			clear_flow_param(flow_param);
			LOG_PROC("ERROR","%s %d there is not exist outport between  src_sw and dst_sw",FN,LN);
			return ;
		}
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);
	}


	install_fabric_flows(src_sw, 100, FABRIC_ARP_HARD_TIME_OUT, priority_level,
			flow_table_id, OFPFC_ADD, flow_param);


	clear_flow_param(flow_param);
}

void remove_clbforward_multicast(gn_switch_t* sw, UINT4 srcip, UINT2 proto)
{
	flow_param_t* flow_param = init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(srcip);
	flow_param->match_param->ip_proto = proto;
	install_fabric_flows(sw, 0, 0, 0, FABRIC_INPUT_TABLE, OFPFC_DELETE, flow_param);
	clear_flow_param(flow_param);
}
	
void remove_clbforward_flow_by_IpAndMac(gn_switch_t* sw, UINT1* mac,UINT4 ip)
{
	
	UINT2 flow_table_id = FABRIC_PUSHTAG_TABLE;
	//UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_CLBFORWARD_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	flow_param_t* flow_param = init_flow_param();
	
	//LOG_PROC("INFO", "%s -- --------------[%x:%x:%x:%x:%x:%x]%d",FN,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],ip);
	
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);
	memcpy(flow_param->match_param->eth_src, mac, 6);	
	
	install_fabric_flows(sw, 0, 0, 0, FABRIC_OTHER_TABLE, OFPFC_DELETE, flow_param);
	clear_flow_param(flow_param);
	
	
}
void remove_clbforward_flow_by_vmMac(gn_switch_t* sw, UINT1* mac)
{
	flow_param_t* flow_param = init_flow_param();
	
	
	//flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_src, mac, 6);	
	
	install_fabric_flows(sw, 0, 0, 0, FABRIC_OUTPUT_TABLE, OFPFC_DELETE, flow_param);
	clear_flow_param(flow_param);
}

void remove_clbforward_flow_by_SrcIp(gn_switch_t* sw, UINT4 ip)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = (0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_FQ_IN_POST_PROCESS_TABLE;
	
	UINT2 priority_level = (0 == get_nat_physical_switch_flag()) ? FABRIC_PRIORITY_CLBFORWARD_FLOW : FABRIC_PRIORITY_PICA8ZEROTABLE_FORWARD_FLOW;
	
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);
	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, priority_level,
			flow_table_id, OFPFC_DELETE, flow_param);


	clear_flow_param(flow_param);
}

// 下发流表规则
/* by:yhy
 * host所在compute节点的OVS交换机上下发的nat出发流表
 */
void install_fabric_nat_from_inside_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
								UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
								UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw, security_param_t* security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_src, external_mac, 6);
	oxm.ipv4_src = ntohl(external_ip);
	oxm.tcp_src = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	oxm.udp_src = (IPPROTO_UDP == proto_type) ? external_port_no : 0;
	oxm.vlan_vid = (sw->dpid != gateway_sw->dpid) ? (UINT2)gateway_vlan_vid : 0;
	memcpy(oxm.eth_dst, gateway_mac, 6);

	memcpy(flow_param->match_param->eth_src, packetin_src_mac, 6);
	flow_param->match_param->ipv4_dst = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) {
		add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	}
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (sw->dpid != gateway_sw->dpid) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&gateway_out_port);
	}

	//set_security_match(flow_param->match_param, security_param);
	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_NAT_FLOW,
						 FABRIC_FQ_OUT_POST_PROCESS_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}
void install_fabric_nat_throughfirewall_from_inside_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw, security_param_t* security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_FIREWALL_OUT_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_src, external_mac, 6);
	oxm.ipv4_src = ntohl(external_ip);
	oxm.tcp_src = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	oxm.udp_src = (IPPROTO_UDP == proto_type) ? external_port_no : 0;
	oxm.vlan_vid = (sw->dpid != gateway_sw->dpid) ? (UINT2)gateway_vlan_vid : 0;
	memcpy(oxm.eth_dst, gateway_mac, 6);

	memcpy(flow_param->match_param->eth_src, packetin_src_mac, 6);
	flow_param->match_param->ipv4_dst = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);



	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_NAT_FLOW,
						 FABRIC_NAT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

// 下发流表规则
/* by:yhy
 * 不使用物理出口交换机的情况下,出口交换机上下发的nat返回流表
 */
void install_fabric_nat_from_external_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.ipv4_dst = ntohl(packetin_src_ip);
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;
	oxm.vlan_vid = (sw->dpid != gateway_sw->dpid) ? (UINT2)src_vlan_vid : 0;

	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto_type) ?  ntohs(packetin_src_port) : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (0 != oxm.vlan_vid) {
		add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	}
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);

	if (sw->dpid != gateway_sw->dpid) {
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	}
	else {
		add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&gateway_out_port);
	}

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(gateway_sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_NAT_FLOW,
						 FABRIC_PUSHTAG_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
	
}

// 下发流表规则
/* by:yhy
 * 使用物理出口交换机的情况下,出口交换机上下发的nat返回流表
 */
void install_fabric_nat_from_external_fabric_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw, UINT4 out_port)
{
	flow_param_t* flow_param = init_flow_param();

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.vlan_vid = (UINT2)src_vlan_vid;

	memcpy(flow_param->match_param->eth_dst, external_mac, 6);
	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto_type) ? external_port_no : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&out_port);

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(gateway_sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, 35,
					     FABRIC_FQ_IN_POST_PROCESS_NAT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

// 下发流表规则
/* by:yhy
 * 使用物理出口交换机的情况下,host主机所在ovs交换机上下发的nat返回流表输出至host所接端口
 */
void install_fabric_nat_from_external_fabric_host_flow(UINT4 packetin_src_ip, UINT4 packetin_dst_ip, UINT2 packetin_src_port, UINT1 proto_type,
		UINT1* packetin_src_mac, UINT4 external_ip, UINT1* gateway_mac, UINT1* external_mac, UINT2 external_port_no,
		UINT4 gateway_vlan_vid, UINT4 src_vlan_vid, UINT2 gateway_out_port,	gn_switch_t* sw, gn_switch_t* gateway_sw)
{
	flow_param_t* flow_param = init_flow_param();

	UINT1 GotoTable  =FABRIC_FIREWALL_IN_TABLE;
	
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	memcpy(oxm.eth_dst, packetin_src_mac, 6);
	oxm.ipv4_dst = ntohl(packetin_src_ip);
	oxm.tcp_dst = (IPPROTO_TCP == proto_type) ? ntohs(packetin_src_port) : 0;
	oxm.udp_dst = (IPPROTO_UDP == proto_type) ? ntohs(packetin_src_port) : 0;

	UINT2 tag =of131_fabric_impl_get_tag_sw(sw);
	flow_param->match_param->vlan_vid = (UINT2)tag;
	flow_param->match_param->ipv4_src = ntohl(packetin_dst_ip);
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = proto_type;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto_type) ? external_port_no : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto_type) ? external_port_no : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&GotoTable);
	add_action_param(&flow_param->action_param, OFPAT13_POP_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	
	
	
	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_INPUT_TABLE_NAT_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);

}

//by:yhy 根据输入参数删除fabric_input_flow
//删除sw上,table=FABRIC_INPUT_TABLE,ipv4_dst=ip的所有流表
void delete_fabric_input_flow_by_ip(gn_switch_t* sw, UINT4 ip)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_FQ_IN_POST_PROCESS_TABLE; //FABRIC_INPUT_TABLE;//(0 == get_nat_physical_switch_flag()) ? FABRIC_PUSHTAG_TABLE : FABRIC_INPUT_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}
//by:yhy 删除流表
//table:FABRIC_INPUT_TABLE
//匹配:eth_dst,ip_proto,tcp_dst/udp_dst端口号
void delete_fabric_input_flow_by_mac_portno(gn_switch_t* sw, UINT1* dst_mac, UINT2 dst_port, UINT2 proto)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_INPUT_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_dst, dst_mac, 6);
	flow_param->match_param->ip_proto = proto;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto) ? dst_port : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto) ? dst_port : 0;

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}


void delete_fabric_input_portforwardflow_by_mac_portno(gn_switch_t* sw, UINT1* src_mac, UINT2 src_port, UINT2 proto)
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_PUSHTAG_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	memcpy(flow_param->match_param->eth_src, src_mac, 6);
	flow_param->match_param->ip_proto = proto;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto) ? src_port : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto) ? src_port : 0;

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}

void delete_fabric_input_portforwardflow_by_ip_portno(gn_switch_t* sw, UINT4 dst_ip, UINT2 dst_port, UINT2 proto )
{
	flow_param_t* flow_param = init_flow_param();
	UINT2 flow_table_id = FABRIC_PUSHTAG_TABLE;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	flow_param->match_param->ip_proto = proto;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto) ? dst_port : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto) ? dst_port : 0;

	install_fabric_flows(sw, 0, 0, 0,flow_table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}


void delete_fabric_flow_by_ip(gn_switch_t* sw, UINT4 ip, UINT2 table_id)
{
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);

	install_fabric_flows(sw, 0, 0, 0, table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}

void delete_fabric_flow_by_mac(gn_switch_t* sw, UINT1* mac, UINT2 table_id)
{
	flow_param_t* flow_param = init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);

	install_fabric_flows(sw, 0, 0, 0, table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}


void delete_fabric_flow_by_ip_mac_priority(gn_switch_t* sw, UINT4 ip, UINT1* mac, UINT2 priority, UINT2 timeout, UINT2 table_id)
{
	flow_param_t* flow_param = init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);

	memcpy(flow_param->match_param->eth_dst, mac, 6);

	install_fabric_flows(sw, timeout, timeout, priority, table_id, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}

void install_fabric_deny_ip_flow(gn_switch_t* sw, UINT4 ip, UINT1* mac, UINT2 priority, UINT2 timeout, UINT2 table_id)
{
	flow_param_t* flow_param = init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);

	memcpy(flow_param->match_param->eth_dst, mac, 6);

	install_fabric_flows(sw, timeout, timeout, priority, table_id, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

//by:yhy 装载deny_flow
void install_deny_flow(gn_switch_t *sw, UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT1* dst_mac, UINT1 proto, UINT1 icmp_type,
					   UINT1 icmp_code, UINT2 sport, UINT2 dport)

{
	INT1* value = get_value(g_controller_configure, "[openvstack_conf]", "security_drop_interval");
	INT4 flag_security_drop_interval = (NULL == value) ? 100: atoi(value);
	if (0 == flag_security_drop_interval) 
	{
		return ;
	}
	//LOG_PROC("INFO", "%s -- --------------",FN);
	flow_param_t* flow_param = init_flow_param();

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(src_ip);
	//LOG_PROC("INFO", "%s -- --------------%d",FN,flow_param->match_param->ipv4_src);
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	if (src_mac)
	{
		memcpy(flow_param->match_param->eth_src, src_mac, 6);
	}
	if (dst_mac)
	{
		memcpy(flow_param->match_param->eth_dst, dst_mac, 6);
	}
	flow_param->match_param->ip_proto = proto;
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto) ? sport : 0;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto) ? dport : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto) ? sport : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto) ? dport : 0;
	flow_param->match_param->icmpv4_type= (IPPROTO_ICMP == proto) ? icmp_type : 0;
	flow_param->match_param->icmpv4_code = (IPPROTO_ICMP == proto) ? icmp_code : 0;

	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);

	install_fabric_flows(sw, flag_security_drop_interval, 0, FABRIC_PRIORITY_DENY_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}
//by:yhy 删除根据给定参数的deny_flow
void remove_deny_flow(gn_switch_t* sw, UINT1* mac)
{
	flow_param_t* flow_param = init_flow_param();

	memcpy(flow_param->match_param->eth_dst, mac, 6);

	install_fabric_flows(sw, 0, 0, FABRIC_PRIORITY_DENY_FLOW, FABRIC_PUSHTAG_TABLE, OFPFC_DELETE, flow_param);

	clear_flow_param(flow_param);
}

//by:yhy 删除根据给定参数的deny_flow
void remove_deny_flow_by_srcMAC(gn_switch_t* sw, UINT1* mac,UINT4 ip)
{
	flow_param_t* flow_param = init_flow_param();
	//LOG_PROC("INFO", "%s -- --------------[%x:%x:%x:%x:%x:%x]%d",FN,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],ip);
	
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_src = ntohl(ip);
	memcpy(flow_param->match_param->eth_src, mac, 6);	
	install_fabric_flows(sw, 0, 0, FABRIC_PRIORITY_DENY_FLOW, FABRIC_INPUT_TABLE, OFPFC_DELETE, flow_param);
	clear_flow_param(flow_param);
	
	
	flow_param = init_flow_param();
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);
	memcpy(flow_param->match_param->eth_dst, mac, 6);	
	install_fabric_flows(sw, 0, 0, FABRIC_PRIORITY_DENY_FLOW, FABRIC_INPUT_TABLE, OFPFC_DELETE, flow_param);
	clear_flow_param(flow_param);

}


void install_ip_controller_flow(gn_switch_t* sw,UINT4 ip)
{
	
	UINT2 flow_table_id = FABRIC_OUTPUT_TABLE;
	flow_param_t* flow_param = init_flow_param();
	UINT4 port = OFPP13_CONTROLLER;
	
	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ipv4_dst = ntohl(ip);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_OUTPUT, (void*)&port);
	install_fabric_flows(sw, 5,FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_QOS_DEFAULT_JUMP_FLOW, flow_table_id, OFPFC_ADD, flow_param);
	clear_flow_param(flow_param);
}


//by:yhy 将oxm中的匹配值段的表现形式转化成flow_oxm中的匹配字段表现形式
void set_flow_match(gn_oxm_t* flow_oxm, gn_oxm_t* oxm)
{
	UINT1 zero_array[24] = {0};
	if ((NULL == flow_oxm) || (NULL == oxm))
	{
		return ;
	}

	// printf("%s\n", FN);
	memcpy(flow_oxm, oxm, sizeof(gn_oxm_t));

	if (oxm->in_port) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IN_PORT);
	}
	if (oxm->in_phy_port) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IN_PHY_PORT);
	}
	if (oxm->metadata) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_METADATA);
	}
	
	if (memcmp(zero_array, oxm->eth_dst, 6)) 
	{
		flow_oxm->mask |= (1 << OFPXMT_OFB_ETH_DST);
	}	

	if (memcmp(zero_array, oxm->eth_src, 6)) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ETH_SRC);
	}
	if (oxm->eth_type) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ETH_TYPE);
	}
	if (oxm->vlan_vid) {
		// printf("**********vlan vid:%d\n", oxm->vlan_vid);
		flow_oxm->mask |= (1 << OFPXMT_OFB_VLAN_VID);
	}
	if (oxm->vlan_pcp) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_VLAN_PCP);
	}
	if (oxm->ip_dscp) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IP_DSCP);
	}
	if (oxm->ip_ecn) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IP_ECN);
	}
	if (oxm->ip_proto) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IP_PROTO);
	}
	if (oxm->ipv4_src) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV4_SRC);
	}
	if (oxm->ipv4_dst) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV4_DST);
	}
	if (oxm->tcp_src) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_TCP_SRC);
	}
	if (oxm->tcp_dst) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_TCP_DST);
	}
	if (oxm->udp_src) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_UDP_SRC);
	}
	if (oxm->udp_dst) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_UDP_DST);
	}
	if (oxm->icmpv4_type) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ICMPV4_TYPE);
	}
	if (oxm->icmpv4_code) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ICMPV4_CODE);
	}
	if (oxm->arp_op) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_OP);
	}
	if (oxm->arp_spa) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_SPA);
	}
	if (oxm->arp_tpa) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_TPA);
	}
	if (memcmp(zero_array, oxm->arp_sha, 6)) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_SHA);
	}
	if (memcmp(zero_array, oxm->arp_tha, 6)) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_ARP_THA);
	}
	if (memcmp(zero_array, oxm->ipv6_src, 16)) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV6_SRC);
	}
	if (memcmp(zero_array, oxm->ipv6_dst, 16)) {
		flow_oxm->mask |= (1 << OFPXMT_OFB_IPV6_DST);
	}

	if (oxm->mpls_label) {
		flow_oxm->mask |= ((UINT8)1 << OFPXMT_OFB_MPLS_LABEL);
	}
	if (oxm->tunnel_id) {
		flow_oxm->mask |= ((UINT8)1 << OFPXMT_OFB_TUNNEL_ID);
	}
	if (oxm->ipv4_src_prefix) {
		flow_oxm->mask |= ((UINT8)1 << OFPXMT_OFB_IPV4_SRC_PREFIX);
	}
	if (oxm->ipv4_dst_prefix) {
		flow_oxm->mask |= ((UINT8)1 << OFPXMT_OFB_IPV4_DST_PREFIX);
	}
	if (oxm->ipv6_src_prefix) {
		flow_oxm->mask |= ((UINT8)1 << OFPXMT_OFB_IPV6_SRC_PREFIX);
	}
	if (oxm->ipv6_dst_prefix) {
		flow_oxm->mask |= ((UINT8)1 << OFPXMT_OFB_IPV6_DST_PREFIX);
	}
	if (oxm->metadata_mask) {
		flow_oxm->mask |= ((UINT8)1 << OFPXMT_OFB_METADATA_MASK);
	}
}
//by:yhy why?
void set_security_match( gn_oxm_t* oxm, security_param_t* security_param)
{
	// printf("%s\n", FN);
	if ((NULL != security_param) && (security_param->ip_proto)) {
		oxm->ip_proto = security_param->ip_proto;
		oxm->icmpv4_code = security_param->icmp_code;
		oxm->icmpv4_type = security_param->imcp_type;
		oxm->tcp_src = security_param->tcp_port_num;
		oxm->udp_src = security_param->udp_port_num;
		if ((IPPROTO_ICMP == oxm->ip_proto) || (IPPROTO_TCP == oxm->ip_proto) || (IPPROTO_UDP == oxm->ip_proto)) 
			oxm->eth_type = ETHER_IP;
	}
}
//by:yhy 将instraction_param这里的instruction转化成flow_instruction中的instruction
//就是转换action的表现形式
gn_instruction_t* set_flow_instruction(gn_instruction_t* flow_instruction, action_param_t* instraction_param)
{
	// printf("%s\n", FN);
	gn_instruction_t* instruction = flow_instruction;
	gn_instruction_goto_table_t* instruction_goto = NULL;
	gn_instruction_write_metadata_t* instruction_metadata = NULL;
	gn_instruction_actions_t* instruction_write = NULL;
	gn_instruction_actions_t* instruction_apply = NULL;
	gn_instruction_actions_t* instruction_clear = NULL;
	gn_instruction_meter_t* instrucion_meter = NULL;
	gn_instruction_experimenter_t* instruction_experimenter = NULL;

	while (NULL != instraction_param) {
		switch (instraction_param->type) {
		case OFPIT_GOTO_TABLE:
		{
			instruction_goto = (gn_instruction_goto_table_t*)gn_malloc(sizeof(gn_instruction_goto_table_t));
			memset(instruction_goto, 0, sizeof(gn_instruction_goto_table_t));
			instruction_goto->type = OFPIT_GOTO_TABLE;
			instruction_goto->table_id = *(UINT1*)instraction_param->param;
			break;
		}
		case OFPIT_WRITE_METADATA:
		{
#if 1
			instruction_metadata = (gn_instruction_write_metadata_t*)gn_malloc(sizeof(gn_instruction_write_metadata_t));
			memset(instruction_metadata, 0, sizeof(gn_instruction_write_metadata_t));
			instruction_metadata->type = OFPIT_WRITE_METADATA;
			instruction_metadata->metadata = *(UINT8*)instraction_param->param;
			instruction_metadata->metadata_mask = *((UINT8*)instraction_param->param + 1);
#endif
			break;
		}
		case OFPIT_WRITE_ACTIONS:
		{
			instruction_write = (gn_instruction_actions_t*)gn_malloc(sizeof(gn_instruction_actions_t));
			memset(instruction_write, 0, sizeof(gn_instruction_actions_t));
			instruction_write->type = (UINT2)OFPIT_APPLY_ACTIONS;
			break;
		}
		case OFPIT_APPLY_ACTIONS:
		{
			// printf("***%s\n",FN);
			instruction_apply = (gn_instruction_actions_t*)gn_malloc(sizeof(gn_instruction_actions_t));
			memset(instruction_apply, 0, sizeof(gn_instruction_actions_t));
			instruction_apply->type = (UINT2)OFPIT_APPLY_ACTIONS;
			break;
		}
		case OFPIT_CLEAR_ACTIONS:
		{
			instruction_clear = (gn_instruction_actions_t*)gn_malloc(sizeof(gn_instruction_actions_t));
			memset(instruction_clear, 0, sizeof(gn_instruction_actions_t));
			instruction_clear->type = OFPIT_CLEAR_ACTIONS;
			break;
		}
		case OFPIT_METER:
		{
			instrucion_meter = (gn_instruction_meter_t*)gn_malloc(sizeof(gn_instruction_meter_t));
			memset(instrucion_meter, 0, sizeof(gn_instruction_meter_t));
			instrucion_meter->type = (UINT2)OFPIT_METER;
			instrucion_meter->meter_id = *(UINT4*)instraction_param->param;
			break;
		}
		case OFPIT_EXPERIMENTER:
		{
			instruction_experimenter = (gn_instruction_experimenter_t*)gn_malloc(sizeof(gn_instruction_experimenter_t));
			memset(instruction_experimenter, 0, sizeof(gn_instruction_experimenter_t));
			instrucion_meter->type = (UINT2)OFPIT_EXPERIMENTER;
			break;
		}
		default:
			break;
		}
		instraction_param = instraction_param->next;
	}

#if 1
	if (NULL != instruction_experimenter) {
		instruction_experimenter->next = instruction;
		instruction = (gn_instruction_t*)instruction_experimenter;
	}
#endif
	if (NULL != instrucion_meter) {
		instrucion_meter->next = instruction;
		instruction = (gn_instruction_t*)instrucion_meter;
	}
	if (NULL != instruction_apply) {
		instruction_apply->next = instruction;
		instruction = (gn_instruction_t*)instruction_apply;
	}
	if (NULL != instruction_clear) {
		instruction_clear->next = instruction;
		instruction = (gn_instruction_t*)instruction_clear;
	}
	if (NULL != instruction_write) {
		instruction_write->next = instruction;
		instruction = (gn_instruction_t*)instruction_write;
	}
	if (NULL != instruction_metadata) {
		instruction_metadata->next = instruction;
		instruction = (gn_instruction_t*)instruction_metadata;
	}
	if (NULL != instruction_goto) {
		instruction_goto->next = instruction;
		instruction = (gn_instruction_t*)instruction_goto;
	}

	return instruction;
}

void set_flow_action(gn_instruction_t* flow_instruction, action_param_t* action_param, enum ofp_instruction_type instruction_type)
{
	// printf("%s\n", FN);
	gn_instruction_t* instruction_p = flow_instruction;
	if (NULL == instruction_p) {
		// printf("%s instruction is NULL!\n", FN);
	}

	while (NULL != instruction_p)
	{
		if (instruction_type == instruction_p->type) {
			break;
		}
		instruction_p = instruction_p->next;
	}

	if (NULL == instruction_p) {
		return ;
	}

	// printf("%s********************\n", FN);
	gn_instruction_actions_t* instruction = (gn_instruction_actions_t*)instruction_p;

	while (NULL != action_param) {
		// printf("action type is %d,parameter is%d\n", action_param->type, *(UINT4*)action_param->param);
		switch (action_param->type) {
		case OFPAT13_OUTPUT:
		{
			// printf("%s****output********\n", FN);
			gn_action_output_t* action_output = (gn_action_output_t*)gn_malloc(sizeof(gn_action_output_t));
			memset(action_output, 0, sizeof(gn_action_t));
			action_output->port = *(UINT4*)action_param->param;
			action_output->type = OFPAT13_OUTPUT;
			action_output->next = instruction->actions;
			action_output->max_len = 0xffff;
			instruction->actions = (gn_action_t *)action_output;
			break;
		}
		case OFPAT13_COPY_TTL_OUT:
		{
			gn_action_t* act_copyTTLOut = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_copyTTLOut, 0, sizeof(gn_action_t));
			act_copyTTLOut->type = OFPAT13_COPY_TTL_OUT;
			act_copyTTLOut->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_copyTTLOut;
			break;
		}
		case OFPAT13_COPY_TTL_IN:
		{
			gn_action_t* act_copyTTLIn = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_copyTTLIn, 0, sizeof(gn_action_t));
			act_copyTTLIn->type = OFPAT13_COPY_TTL_IN;
			act_copyTTLIn->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_copyTTLIn;
			break;
		}
		case OFPAT13_MPLS_TTL:
		{
			gn_action_mpls_ttl_t* act_setMplsTtl = (gn_action_mpls_ttl_t*)gn_malloc(sizeof(gn_action_mpls_ttl_t));
			memset(act_setMplsTtl, 0, sizeof(gn_action_mpls_ttl_t));
			act_setMplsTtl->type = OFPAT13_MPLS_TTL;
			act_setMplsTtl->mpls_tt = *(UINT1*)action_param->param;
			act_setMplsTtl->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_setMplsTtl;
			break;
		}
		case OFPAT13_DEC_MPLS_TTL:
		{
			gn_action_t* act_decMplsTtl = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_decMplsTtl, 0, sizeof(gn_action_t));
			act_decMplsTtl->type = OFPAT13_DEC_MPLS_TTL;
			act_decMplsTtl->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_decMplsTtl;
			break;
		}
		case OFPAT13_PUSH_VLAN:
		{
			//
			gn_action_t* act_pushVlan = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_pushVlan, 0, sizeof(gn_action_t));
			act_pushVlan->type = OFPAT13_PUSH_VLAN;
			act_pushVlan->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_pushVlan;
			break;
		}
		case OFPAT13_POP_VLAN:
		{
			//
			gn_action_t* act_popVlan = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_popVlan, 0, sizeof(gn_action_t));
			act_popVlan->type = OFPAT13_POP_VLAN;
			act_popVlan->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_popVlan;
			break;
		}
		case OFPAT13_PUSH_MPLS:
		{
			//
			gn_action_t* act_pushMpls = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_pushMpls, 0, sizeof(gn_action_set_field_t));
			act_pushMpls->type = OFPAT13_PUSH_MPLS;
			act_pushMpls->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_pushMpls;
			break;
		}
		case OFPAT13_POP_MPLS:
		{
			//
			gn_action_t* act_popMpls = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_popMpls, 0, sizeof(gn_action_t));
			act_popMpls->type = OFPAT13_POP_MPLS;
			act_popMpls->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_popMpls;
			break;
		}
		case OFPAT13_SET_QUEUE:
		{
			gn_action_set_queue_t* act_queue = (gn_action_set_queue_t*)gn_malloc(sizeof(gn_action_set_queue_t));
			memset(act_queue, 0, sizeof(gn_action_group_t));
			act_queue->type = OFPAT13_SET_QUEUE;
			act_queue->queue_id = *(UINT4*)action_param->param;
			act_queue->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_queue;
			break;
		}
		case OFPAT13_GROUP:
		{
			//
			gn_action_group_t* act_group = (gn_action_group_t*)gn_malloc(sizeof(gn_action_group_t));
			memset(act_group, 0, sizeof(gn_action_group_t));
			act_group->type = OFPAT13_GROUP;
			act_group->group_id = *(UINT4*)action_param->param;
			act_group->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_group;
			break;
		}
		case OFPAT13_SET_NW_TTL:
		{
			gn_action_nw_ttl_t* act_setNwTTl = (gn_action_nw_ttl_t*)gn_malloc(sizeof(gn_action_nw_ttl_t));
			memset(act_setNwTTl, 0, sizeof(gn_action_nw_ttl_t));
			act_setNwTTl->type = OFPAT13_SET_NW_TTL;
			act_setNwTTl->nw_tt = *(UINT1*)action_param->param;
			act_setNwTTl->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_setNwTTl;
			break;
		}
		case OFPAT13_DEC_NW_TTL:
		{
			gn_action_t* act_decNwTtl = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_decNwTtl, 0, sizeof(gn_action_t));
			act_decNwTtl->type = OFPAT13_DEC_NW_TTL;
			act_decNwTtl->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_decNwTtl;
			break;
		}
		case OFPAT13_SET_FIELD:
		{
			//
			gn_action_set_field_t* act_set_field = (gn_action_set_field_t*)gn_malloc(sizeof(gn_action_set_field_t));
			memset(act_set_field, 0, sizeof(gn_action_set_field_t));
			act_set_field->type = OFPAT13_SET_FIELD;
			set_flow_match(&act_set_field->oxm_fields, (gn_oxm_t*)action_param->param);
			act_set_field->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_set_field;
			break;
		}
		case OFPAT13_PUSH_PBB:
		{
			gn_action_t* act_pushPBB = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_pushPBB, 0, sizeof(gn_action_set_field_t));
			act_pushPBB->type = OFPAT13_PUSH_PBB;
			act_pushPBB->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_pushPBB;
			break;
		}
		case OFPAT13_POP_PBB:
		{
			gn_action_t* act_popPBB = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
			memset(act_popPBB, 0, sizeof(gn_action_t));
			act_popPBB->type = OFPAT13_POP_PBB;
			act_popPBB->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_popPBB;
			break;
		}
		case OFPAT13_EXPERIMENTER:
		{
			gn_action_experimenter_t* act_experimenter = (gn_action_experimenter_t*)gn_malloc(sizeof(gn_action_experimenter_t));
			memset(act_experimenter, 0, sizeof(gn_action_experimenter_t));
			act_experimenter->type = OFPAT13_EXPERIMENTER;
			act_experimenter->experimenter = *(UINT4*)action_param->param;
			act_experimenter->next = instruction->actions;
			instruction->actions = (gn_action_t *)act_experimenter;
			break;
		}
		default:
			break;
		}
		action_param = action_param->next;
	}
}

void clear_flow_temp_data(gn_instruction_t* instruction)
{
	gn_instruction_t* temp_instruction = NULL;

	while (NULL != instruction) {
		temp_instruction = instruction;
		
		if ((OFPIT_APPLY_ACTIONS == temp_instruction->type)||(OFPIT_WRITE_ACTIONS == temp_instruction->type)) {
			gn_instruction_actions_t* action_instruction = (gn_instruction_actions_t*)temp_instruction;
			gn_action_t* action_p = action_instruction->actions;
			gn_action_t* temp_p = NULL;
			
			while(action_p)
			{
				temp_p = action_p;
				action_p = action_p->next;
				temp_p->next = NULL;
				gn_free((void**)&temp_p);
				
			}
		}
		instruction = instruction->next;
		
		temp_instruction->next = NULL;
		gn_free((void**)&temp_instruction);
	}
}
//by:yhy 在流表中配置QOS
void set_qos_match(gn_switch_t* sw, flow_param_t* flow_param)
{
	if ((NULL == sw) || (QOS_TYPE_OFF == sw->qos_type)) 
	{
		return ;
	}
	gn_oxm_t* match_oxm = NULL;
	gn_oxm_t* set_oxm = NULL;
	qos_policy_p policy_p = NULL;
	if (flow_param->match_param) 
	{
		match_oxm = flow_param->match_param;
	}
	if ((match_oxm) && (match_oxm->ipv4_dst)) 
	{
		policy_p = find_qos_policy_by_sw_dstip(sw, ntohl(match_oxm->ipv4_dst));
	}
	if (NULL == policy_p) 
	{
		if (flow_param->action_param) 
		{
			action_param_t* action = flow_param->action_param;
			while (action) 
			{
				if (OFPAT13_SET_FIELD == action->type) 
				{
					set_oxm = (gn_oxm_t*)action->param;
				}
				action = action->next;
			}
		}
		if ((set_oxm) && (0 != set_oxm->ipv4_dst)) 
		{
			policy_p = find_qos_policy_by_sw_dstip(sw, ntohl(set_oxm->ipv4_dst));
		}
	}
	if ((policy_p) && (policy_p->qos_service))
	{
		if (QOS_TYPE_METER == sw->qos_type) 
		{
			qos_meter_p meter_p = policy_p->qos_service;
			add_action_param(&flow_param->instruction_param, OFPIT_METER, (void*)&meter_p->meter_id);
		}
		else if (QOS_TYPE_QUEUE== sw->qos_type) 
		{
			gn_queue_t* queue_p = policy_p->qos_service;
			
			add_action_param(&flow_param->action_param, OFPAT13_SET_QUEUE, (void*)&queue_p->queue_id);
		}
		else 
		{
			//do nothing
		}
	}
}

//by:yhy 对sw按照参数装载一条流表项
INT4 install_fabric_flows(gn_switch_t * sw,
						  UINT2 idle_timeout,
						  UINT2 hard_timeout,
						  UINT2 priority,
						  UINT1 table_id,
						  UINT1 command,
						  flow_param_t* flow_param)
{
	INT4 ret = GN_ERR;
	flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = idle_timeout;
	flow.hard_timeout = hard_timeout;
	flow.priority = priority;
	flow.table_id = table_id;
	flow.match.type = OFPMT_OXM;
	//set_qos_match(sw, flow_param);
	set_flow_match(&flow.match.oxm_fields, flow_param->match_param);
	flow.instructions = set_flow_instruction(flow.instructions, flow_param->instruction_param);
	set_flow_action(flow.instructions, flow_param->action_param, OFPIT_APPLY_ACTIONS);
	set_flow_action(flow.instructions, flow_param->write_action_param, OFPIT_WRITE_ACTIONS);
	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = command;
	flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
	flow_mod_req.flow = &flow;
	if(sw&&(CONNECTED == sw->conn_state))
	{
		sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
		ret = GN_OK;
	}
	clear_flow_temp_data(flow.instructions);
	return ret;
}



void install_fabric_clear_stat_flows(gn_switch_t * sw,
						  UINT2 idle_timeout,
						  UINT2 hard_timeout,
						  UINT2 priority,
						  UINT1 table_id,
						  UINT1 command,
						  flow_param_t* flow_param)
{
	flow_mod_req_info_t flow_mod_req;
    gn_flow_t flow;
    memset(&flow, 0, sizeof(gn_flow_t));
	flow.create_time = g_cur_sys_time.tv_sec;
	flow.idle_timeout = idle_timeout;
	flow.hard_timeout = hard_timeout;
	flow.priority = priority;
	flow.table_id = table_id;
	flow.match.type = OFPMT_OXM;

	set_flow_match(&flow.match.oxm_fields, flow_param->match_param);
	// printf("flow match mask is %llu\n", flow.match.oxm_fields.mask);
	flow.instructions = set_flow_instruction(flow.instructions, flow_param->instruction_param);
	set_flow_action(flow.instructions, flow_param->action_param, OFPIT_APPLY_ACTIONS);
	set_flow_action(flow.instructions, flow_param->write_action_param, OFPIT_WRITE_ACTIONS);

	flow_mod_req.buffer_id = 0xffffffff;
	flow_mod_req.out_port = 0xffffffff;
	flow_mod_req.out_group = 0xffffffff;
	flow_mod_req.command = command;
	flow_mod_req.flags = OFPFF13_RESET_COUNTS;
	flow_mod_req.flow = &flow;

    // add_flow_entry(sw,&flow);
	//by:yhy 向交换机发送openflow中"OFPT_PLOW_MOD"消息,下发流表
	if(CONNECTED == sw->conn_state)
		sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
	clear_flow_temp_data(flow.instructions);
}
//by:yhy 将type和param初始化到action_param中,并将action_param->next指向自身
//by:yhy 在流表项内部添加一个action(动作)
void add_action_param(action_param_t** action_param, INT4 type, void* param)
{
	action_param_t* new_param = (action_param_t*)gn_malloc(sizeof(action_param_t));
	memset(new_param, 0, sizeof(action_param_t));
	new_param->type = type;
	new_param->param = param;
	new_param->next = *action_param;
	*action_param = new_param;
}
void add_action_param_rear(action_param_t** action_param, INT4 type, void* param)
{
	action_param_t* new_param = (action_param_t*)gn_malloc(sizeof(action_param_t));
	memset(new_param, 0, sizeof(action_param_t));
	new_param->type = type;
	new_param->param = param;
	new_param->next = NULL;
	*action_param = new_param;
}

void clear_action_param(action_param_t* action_param)
{
	action_param_t* temp = NULL;
	while (NULL != action_param) {
		temp = action_param;
		action_param = action_param->next;
		
		gn_free((void**)&temp);
	}
}
//by:yhy 生成一flow_param,分配内存空间并初始化
//by:yhy 初始化一个刘表结构体
flow_param_t* init_flow_param()
{
	flow_param_t* flow_param = (flow_param_t*)gn_malloc(sizeof(flow_param_t));
	memset(flow_param, 0, sizeof(flow_param_t));
	flow_param->match_param = (gn_oxm_t*)gn_malloc(sizeof(gn_oxm_t));
	memset(flow_param->match_param, 0, sizeof(gn_oxm_t));
	flow_param->instruction_param = NULL;
	flow_param->action_param = NULL;
	flow_param->write_action_param = NULL;

	return flow_param;
}
//by:yhy 销毁一flow_param,资源回收
void clear_flow_param(flow_param_t* flow_param)
{
	gn_free((void**)&(flow_param->match_param));
	clear_action_param(flow_param->instruction_param);
	clear_action_param(flow_param->action_param);
	clear_action_param(flow_param->write_action_param);
	gn_free((void**)&flow_param);
}

/*
 * temp added for ipv6
 * by lxf@2016.1.11
 */
#if 1
void install_fabric_push_tag_security_flow_ipv6(gn_switch_t * sw,UINT1* ipv6, UINT1* mac, UINT4 tag, security_param_p security_param)
{
	flow_param_t* flow_param = init_flow_param();

	UINT2 table_id = FABRIC_SWAPTAG_TABLE;
	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)tag;

	// memcpy(flow_param->match_param->eth_dst, mac, 6);
	flow_param->match_param->eth_type = ETHER_IPV6;
	memcpy(flow_param->match_param->ipv6_dst, ipv6, 16);
	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&table_id);
	add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, FABRIC_PRIORITY_ARP_FLOW,
						 FABRIC_INPUT_TABLE, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}
#endif



void install_fabric_ip_proto_flow(gn_switch_t * sw, UINT1 proto, UINT2 table_id, UINT2 priority,
									UINT4 src_ip, UINT4 dst_ip, UINT1* src_mac, UINT1* dst_mac, UINT2 sport, UINT2 dport,
									UINT4 mod_src_ip, UINT4 mod_dst_ip, UINT1* mod_src_mac, UINT1* mod_dst_mac, UINT2 mod_sport, UINT2 mod_dport,
									UINT2 dst_tag, UINT4 outport, UINT2 goto_id, security_param_p security_p)
{
	flow_param_t* flow_param = init_flow_param();

	gn_oxm_t oxm;
	memset(&oxm, 0 ,sizeof(gn_oxm_t));
	oxm.vlan_vid = (UINT2)dst_tag;
	oxm.ipv4_src = ntohl(mod_src_ip);
	oxm.ipv4_dst = ntohl(mod_dst_ip);
	if (mod_src_mac)
		memcpy(oxm.eth_src, mod_src_mac, 6);
	if (mod_dst_mac)
		memcpy(oxm.eth_dst, mod_dst_mac, 6);
	oxm.tcp_src = (IPPROTO_TCP == proto) ? mod_sport : 0;
	oxm.tcp_dst = (IPPROTO_TCP == proto) ? mod_dport : 0;
	oxm.udp_src = (IPPROTO_UDP == proto) ? mod_sport : 0;
	oxm.udp_dst = (IPPROTO_UDP == proto) ? mod_dport : 0;
	oxm.vlan_vid = dst_tag;

	flow_param->match_param->eth_type = ETHER_IP;
	flow_param->match_param->ip_proto = ((IPPROTO_TCP == proto) || (IPPROTO_UDP == proto)) ? proto : 0;
	flow_param->match_param->ipv4_src = ntohl(src_ip);
	flow_param->match_param->ipv4_dst = ntohl(dst_ip);
	if (src_mac)
		memcpy(flow_param->match_param->eth_src, src_mac, 6);
	if (dst_mac)
		memcpy(flow_param->match_param->eth_dst, dst_mac, 6);
	flow_param->match_param->tcp_src = (IPPROTO_TCP == proto) ? sport : 0;
	flow_param->match_param->tcp_dst = (IPPROTO_TCP == proto) ? dport : 0;
	flow_param->match_param->udp_src = (IPPROTO_UDP == proto) ? sport : 0;
	flow_param->match_param->udp_dst = (IPPROTO_UDP == proto) ? dport : 0;


	add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
	if (dst_tag)
		add_action_param(&flow_param->action_param, OFPAT13_PUSH_VLAN, NULL);
	add_action_param(&flow_param->action_param, OFPAT13_SET_FIELD, (void*)&oxm);
	if (goto_id)
		add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)&goto_id);
	if (outport)
		add_action_param(&flow_param->action_param , OFPAT13_OUTPUT, (void*)&outport);

	// set_security_match(flow_param->match_param, security_param);

	install_fabric_flows(sw, FABRIC_ARP_IDLE_TIME_OUT, FABRIC_ARP_HARD_TIME_OUT, priority,
						 table_id, OFPFC_ADD, flow_param);

	clear_flow_param(flow_param);
}

void fabric_openstack_install_fabric_vip_flows(p_fabric_host_node src_port,p_fabric_host_node dst_port, UINT1 proto, UINT4 vip, UINT1* vip_mac,
		p_fabric_host_node src_gateway, p_fabric_host_node dst_gateway, UINT4 src_tcp_port_no, UINT4 vip_tcp_port_no, security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	UINT1 src_gateway_mac[6] = {0};
	UINT1 dst_gateway_mac[6] = {0};
	UINT2 table_id = FABRIC_PUSHTAG_TABLE;
	UINT2 goto_id = 0;
	UINT4 src_outport = 0;
	UINT4 dst_outport = 0;
	UINT2 priority = FABRIC_PRIORITY_LOADBALANCE_FLOW;

	if ((NULL== src_port) || (NULL == dst_port) || (NULL == src_port->sw) || (NULL == dst_port->sw))
		return ;

	if (NULL != src_gateway)
		memcpy(src_gateway_mac, src_gateway->mac, 6);
	else
		memcpy(src_gateway_mac, vip_mac, 6);

	if (NULL != dst_gateway)
		memcpy(dst_gateway_mac, dst_gateway->mac, 6);
	else
		memcpy(dst_gateway_mac, vip_mac, 6);


	if (src_port->sw == dst_port->sw) {
		src_outport = dst_port->port;
		dst_outport = src_port->port;
	}
	else {
		src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
		dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);

		goto_id = FABRIC_SWAPTAG_TABLE;

		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}

	install_fabric_ip_proto_flow(src_port->sw, proto, table_id, priority,
								 src_port->ip_list[0], vip, src_port->mac, src_gateway_mac,src_tcp_port_no, 0,
								 vip, dst_port->ip_list[0], vip_mac, dst_port->mac, vip_tcp_port_no, 0,
								 dst_tag, src_outport, goto_id, src_security);


	install_fabric_ip_proto_flow(dst_port->sw, proto, table_id, priority,
								 dst_port->ip_list[0], vip, dst_port->mac, dst_gateway_mac, 0, vip_tcp_port_no,
								 vip, src_port->ip_list[0], vip_mac, src_port->mac, 0, src_tcp_port_no,
								 src_tag, dst_outport, goto_id, dst_security);

}

void fabric_openstack_install_fabric_vip_out_flows(gn_switch_t* ext_sw, UINT4 src_ip, UINT1* src_mac, UINT4 floating_ip, UINT1* floating_mac,
												p_fabric_host_node dst_port, UINT1 proto, UINT4 vip, UINT1* vip_mac,
												UINT1* src_gatemac, UINT1* dst_gatemac, UINT4 src_tcp_port_no, UINT4 vip_tcp_port_no, UINT1* ext_gw_mac,
												security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	UINT1 src_gateway_mac[6] = {0};
	UINT1 dst_gateway_mac[6] = {0};
	UINT2 table_id = FABRIC_INPUT_TABLE;
	UINT2 goto_id = 0;
	UINT4 src_outport = 0;
	UINT4 dst_outport = 0;
	UINT2 priority = FABRIC_PRIORITY_LOADBALANCE_FLOW;

	if ((NULL == dst_port) || (NULL == ext_sw) || (NULL == dst_port->sw))
		return ;

	if (NULL != src_gatemac)
		memcpy(src_gateway_mac, src_gatemac, 6);
	else
		memcpy(src_gateway_mac, vip_mac, 6);

	if (NULL != dst_gatemac)
		memcpy(dst_gateway_mac, dst_gatemac, 6);
	else
		memcpy(dst_gateway_mac, vip_mac, 6);


	src_tag = of131_fabric_impl_get_tag_sw(ext_sw);
	dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);

	goto_id = FABRIC_SWAPTAG_TABLE;

	// install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
	install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);

	src_outport = get_out_port_between_switch(ext_sw->dpid, dst_port->sw->dpid);

	install_fabric_ip_proto_flow(ext_sw, proto, table_id, priority,
								 src_ip, floating_ip, NULL, NULL, src_tcp_port_no, 0,
								 vip, dst_port->ip_list[0], vip_mac, dst_port->mac, vip_tcp_port_no, 0,
								 dst_tag, src_outport, 0, src_security);


	install_fabric_ip_proto_flow(dst_port->sw, proto, table_id, priority,
								 dst_port->ip_list[0], vip, dst_port->mac, dst_gateway_mac, 0, vip_tcp_port_no,
								 floating_ip, src_ip, floating_mac, ext_gw_mac, 0, src_tcp_port_no,
								 src_tag, dst_outport, goto_id, dst_security);

}


void fabric_openstack_install_fabric_floaing_vip_flows(p_fabric_host_node src_port, p_fabric_host_node dst_port, UINT1 proto, UINT4 vip, UINT1* vip_mac, UINT4 floatingip, UINT1* floatingmac,
		p_fabric_host_node src_gateway, p_fabric_host_node dst_gateway, UINT4 src_tcp_port_no, UINT4 vip_tcp_port_no, security_param_p src_security, security_param_p dst_security)
{
	UINT4 src_tag = 0;
	UINT4 dst_tag = 0;
	UINT1 src_gateway_mac[6] = {0};
	UINT1 dst_gateway_mac[6] = {0};
	UINT2 table_id = FABRIC_PUSHTAG_TABLE;
	UINT2 goto_id = 0;
	UINT4 src_outport = 0;
	UINT4 dst_outport = 0;
	UINT2 priority = FABRIC_PRIORITY_LOADBALANCE_FLOW;

	if ((NULL== src_port) || (NULL == dst_port) || (NULL == src_port->sw) || (NULL == dst_port->sw))
		return ;

	if (NULL != src_gateway)
		memcpy(src_gateway_mac, src_gateway->mac, 6);
	else
		memcpy(src_gateway_mac, vip_mac, 6);

	if (NULL != dst_gateway)
		memcpy(dst_gateway_mac, dst_gateway->mac, 6);
	else
		memcpy(dst_gateway_mac, vip_mac, 6);


	if (src_port->sw == dst_port->sw) 
	{
		src_outport = dst_port->port;
		dst_outport = src_port->port;
	}
	else 
	{
		src_tag = of131_fabric_impl_get_tag_sw(src_port->sw);
		dst_tag = of131_fabric_impl_get_tag_sw(dst_port->sw);

		goto_id = FABRIC_SWAPTAG_TABLE;

		install_fabric_output_flow(src_port->sw,src_port->mac,src_port->port);
		install_fabric_output_flow(dst_port->sw,dst_port->mac,dst_port->port);
	}

	install_fabric_ip_proto_flow(src_port->sw, proto, table_id, priority,
								 src_port->ip_list[0], floatingip, src_port->mac, src_gateway_mac,src_tcp_port_no, 0,
								 vip, dst_port->ip_list[0], vip_mac, dst_port->mac, vip_tcp_port_no, 0,
								 dst_tag, src_outport, goto_id, src_security);


	install_fabric_ip_proto_flow(dst_port->sw, proto, table_id, priority,
								 dst_port->ip_list[0], vip, dst_port->mac, dst_gateway_mac, 0, vip_tcp_port_no,
								 floatingip, src_port->ip_list[0], floatingmac, src_port->mac, 0, src_tcp_port_no,
								 src_tag, dst_outport, goto_id, dst_security);

}


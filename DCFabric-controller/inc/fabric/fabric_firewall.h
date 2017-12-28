#ifndef INC_FABRIC_FIREWALL_H_
#define INC_FABRIC_FIREWALL_H_

#include "gnflush-types.h"

//外部可引用宏定义*******************************/
#define FIREWALL_ACCESS_INNERIP_NOINCLUDE	2
#define FIREWALL_ACCESS_THROUGH				1
#define FIREWALL_ACCESS_DENY				0

#define FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH			1
#define FIREWALL_INSTALL_FLOW_MODE_IGNORE_COMPARESWITCH		0

#define FIREWALL_DIRECTION_IN			"ingress"
#define FIREWALL_DIRECTION_OUT			"egress"

#define FIREWALL_PROTOCOL_ALL			"ingress"
#define FIREWALL_PROTOCOL_TCP			"ingress"
#define FIREWALL_PROTOCOL_UDP			"ingress"

#define FIREWALL_PROTOCOL_LEN			10
#define FIREWALL_DIRECTION_LEN			10

#define IP_PROTOCOL_TCP		6
#define IP_PROTOCOL_UDP		17
#define IP_PROTOCOL_ICMP	1


#define SWICTH_TYPE_NOT_FINDED 		0
#define SWICTH_TYPE_COMPUTE_NODE	1
#define SWICTH_TYPE_LBEXTERNAL_NODE	2
#define SWICTH_TYPE_EXTERNNAL_NODE	3

//外部可引用结构*********************************/
/* by:Hongyu YANG
 * 防火墙规则结构体
 */
typedef struct _fabric_firewall_rule
{
	UINT4	OuterIP;								//规则外部IP
	UINT1 	OuterIPMask;							//外部IP掩码
	char	Protocol[10];							//针对协议
	char	Direction[10];							//防火墙方向
	UINT2 	PortMax;								//端口上限
	UINT2	PortMin;								//端口下限
	UINT1	Priority;								//优先级
	UINT1	Enable;									//使能
	struct _fabric_firewall_rule * pre;
	struct _fabric_firewall_rule * next;				
} fabric_firewall_rule, *fabric_firewall_rule_p;

/* by:Hongyu YANG
 * 防火墙策略结构体
 */
typedef struct _fabric_firewall_policy
{
	UINT4	InnerIP;									//策略内部IP
	fabric_firewall_rule_p FirewallRuleList;			//防火墙规则
	struct _fabric_firewall_policy * pre;
	struct _fabric_firewall_policy * next;				
} fabric_firewall_policy, *fabric_firewall_policy_p;




//外部可引用变量*********************************/
extern fabric_firewall_policy_p	G_firewall_persist_policy_list ;
extern fabric_firewall_policy_p	G_firewall_temp_policy_list ;


//外部可引用函数*********************************/
UINT1 fabric_firewall_D_ruleList(fabric_firewall_rule_p *firewall_rule_list);
UINT1 fabric_firewall_D_policy_list(fabric_firewall_policy_p *firewall_policy_list);

fabric_firewall_rule_p fabric_firewall_C_rule(fabric_firewall_rule_p *firewall_rule_list,UINT4 OuterIP,UINT1 OuterIPMask,char * Protocol,char * Direction,UINT2 PortMax,UINT2 PortMin,UINT1 Priority,UINT1 Enable);
fabric_firewall_policy_p fabric_firewall_C_policy(fabric_firewall_policy_p *firewall_policy_list,UINT4 InnerIP,fabric_firewall_rule_p firewall_rule_list);
fabric_firewall_policy_p fabric_firewall_R_policy(fabric_firewall_policy_p *firewall_policy_list,UINT4 InnerIP);
fabric_firewall_policy_p fabric_firewall_U_policy(fabric_firewall_policy_p *firewall_policy_list,UINT4 InnerIP,fabric_firewall_rule_p firewall_rule_list);
fabric_firewall_policy_p fabric_firewall_D_policy(fabric_firewall_policy_p *firewall_policy_list,UINT4 InnerIP);

UINT1 fabric_firewall_Compare_policyList(fabric_firewall_policy_p *policy_list_new,fabric_firewall_policy_p *policy_list_old);
UINT1 fabric_firewall_Compare_ruleList	(UINT4 InnerIP,fabric_firewall_rule_p *rule_list_new,fabric_firewall_rule_p *rule_list_old);
UINT1 fabric_firewall_Compare_rule(const fabric_firewall_rule_p *rule_new,const fabric_firewall_rule_p *rule_old);

UINT1 fabric_firewall_rule_added_event				(UINT4 InnerIP,fabric_firewall_rule_p firewall_rule_added);
UINT1 fabric_firewall_rule_changed_or_removed_event	(UINT4 InnerIP,fabric_firewall_rule_p firewall_ruleList);

UINT1 fabric_firewall_policy_added_event(fabric_firewall_policy_p * policy_list_oldList,fabric_firewall_policy_p * policy_list_newList,fabric_firewall_policy_p *policy_newAdded);
UINT1 fabric_firewall_policy_removed_event	(fabric_firewall_policy_p *policy_list_new,fabric_firewall_policy_p *policy_list_old);

void fabric_firewall_InstallFlowsByFirewallRule(UINT4 InnerIP,const fabric_firewall_rule_p firewall_rule,UINT1 mode,gn_switch_t * TargetSwitch);
void fabric_firewall_DeleteFlowsByFirewallInnerIP(UINT4 InnerIP);

void fabric_firewall_ShowAllTempPolicy(void);
void fabric_firewall_ShowAllPersistPolicy(void);
void fabric_firewall_ShowRuleList(fabric_firewall_rule_p RuleList);

void fabric_firewall_RefreshFlowBySwitch(gn_switch_t * TargetSwitch);


void fabric_firewall_SetSwitchFlowInstallFlag (gn_switch_t * TargetSwitch);
void fabric_firewall_SwitchFlowInstallFlagCountDown(gn_switch_t * TargetSwitch);
void fabric_firewall_RefreshSwitch(void);
void fabric_firewall_RefreshFirewallFlow(void);

#endif

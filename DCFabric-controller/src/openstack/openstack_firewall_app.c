#include "openstack_firewall_app.h"
#include "fabric_firewall.h"
#include "openstack_security_app.h"

static void GetIPAndMask_by_cidr(char* ori_cidr, UINT4* ip, UINT1* mask)
{
	char cidr[48] = {0};
	memcpy(cidr, ori_cidr, 48);
	
	char* token = NULL;
	char* buf = cidr; 
	INT4 count = 0;
	if(cidr[0]==0)
	{
		*ip=0;
		*mask=0;
		//LOG_PROC("INFO", "%s_%d_%d_%d",FN,LN,*ip,*mask);
		return;
	}
	while((token = strsep(&buf , "/")) != NULL)
	{
		if (0 == count) 
		{
			*ip = ip2number(token);
		}
		else 
		{
			*mask = (0 == strcmp("0", token)) ? 0:atoll(token);
		}
		count++;
	}
	//LOG_PROC("INFO", "%s_%d_%d_%d",FN,LN,*ip,*mask);
}

/* by:Hongyu Yang
 * 遍历g_fabric_host_list里面的port列表,根据port内存储的安全组信息结合安全组与安全组规则的映射,构建临时的fabric下的防火墙策略
 */
fabric_firewall_rule_p	openstack_firewall_GenerateFirewallPolicy(void)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	p_fabric_host_node 			S_temp_CurrentPort 							=NULL;
	openstack_port_p 			S_temp_PortDataNode	 						=NULL;
	openstack_node_p 			S_temp_PortData_SecurityNode 				=NULL;
	UINT1*						S_temp_PortData_SecurityNode_GroupName		=NULL;
	openstack_security_p		S_temp_TargetSecurityGroup					=NULL;
	openstack_security_rule_p	S_temp_TargetSecurityGroup_SecurityRule		=NULL;
	fabric_firewall_policy_p	H_new_FirewallPolicy						=NULL;
	fabric_firewall_policy_p	H_new_FirewallPolicy_FirewallRule_List		=NULL;
	
	
	
	UINT4 OuterIP		=0;
	UINT1 OuterIPMask	=0;
	char * Protocol		=NULL;
	char * Direction	=NULL;
	UINT2 PortMax		=1;
	UINT2 PortMin		=1;
	UINT1 Priority		=1;
	UINT1 Enable		=0;
	
	S_temp_CurrentPort = g_fabric_host_list.list;
	while(S_temp_CurrentPort)
	{
		H_new_FirewallPolicy						=NULL;
		H_new_FirewallPolicy_FirewallRule_List		=NULL;
		S_temp_PortDataNode  = (openstack_port_p)S_temp_CurrentPort->data;
		if(S_temp_PortDataNode)
		{
			H_new_FirewallPolicy = fabric_firewall_C_policy(&G_firewall_temp_policy_list,S_temp_CurrentPort->ip_list[0],NULL);
			if(H_new_FirewallPolicy)
			{
				S_temp_PortData_SecurityNode = (openstack_node_p)S_temp_PortDataNode->security_data;
				if(OPENSTACK_PORT_TYPE_HOST == S_temp_CurrentPort->type)
				{
					while(S_temp_PortData_SecurityNode)
					{//遍历安全组
						S_temp_PortData_SecurityNode_GroupName =(UINT1*) (S_temp_PortData_SecurityNode->data);
						//根据安全组名,翻译安全组规则到防火墙策略的过程
						S_temp_TargetSecurityGroup = find_security_group_into_SecurityGroupList(S_temp_PortData_SecurityNode_GroupName);
						if(S_temp_TargetSecurityGroup)
						{
							S_temp_TargetSecurityGroup_SecurityRule = S_temp_TargetSecurityGroup->security_rule_p;
							while(S_temp_TargetSecurityGroup_SecurityRule &&(0==strcmp(S_temp_TargetSecurityGroup_SecurityRule->ethertype,"IPv4")))
							{//遍历安全组规则
								UINT4 OuterIP=0;
								UINT1 OuterIPMask=0;
								GetIPAndMask_by_cidr(S_temp_TargetSecurityGroup_SecurityRule->remote_ip_prefix,&OuterIP,&OuterIPMask);
								char * Protocol=S_temp_TargetSecurityGroup_SecurityRule->protocol;
								char * Direction=S_temp_TargetSecurityGroup_SecurityRule->direction;
								UINT2 PortMax= (UINT2)(S_temp_TargetSecurityGroup_SecurityRule->port_range_max);
								UINT2 PortMin= (UINT2)(S_temp_TargetSecurityGroup_SecurityRule->port_range_min);
								UINT1 Priority=S_temp_TargetSecurityGroup_SecurityRule->priority;
								UINT1 Enable=S_temp_TargetSecurityGroup_SecurityRule->enabled;
								fabric_firewall_C_rule(&H_new_FirewallPolicy_FirewallRule_List,OuterIP,OuterIPMask,Protocol,Direction,PortMax,PortMin,Priority,Enable);
								
								S_temp_TargetSecurityGroup_SecurityRule =S_temp_TargetSecurityGroup_SecurityRule->next;
							}
						}
						S_temp_PortData_SecurityNode =S_temp_PortData_SecurityNode->next;
					}
					if(H_new_FirewallPolicy_FirewallRule_List)
					{
						fabric_firewall_U_policy(&G_firewall_temp_policy_list,S_temp_CurrentPort->ip_list[0],H_new_FirewallPolicy_FirewallRule_List);
					} 
				}
				else if(OPENSTACK_PORT_TYPE_GATEWAY == S_temp_CurrentPort->type)
				{
					while(S_temp_PortData_SecurityNode)
					{//遍历安全组
						S_temp_PortData_SecurityNode_GroupName =(UINT1*) (S_temp_PortData_SecurityNode->data);
						//根据安全组名,翻译安全组规则到防火墙策略的过程
						S_temp_TargetSecurityGroup = find_security_group_into_SecurityGroupList(S_temp_PortData_SecurityNode_GroupName);
						if(S_temp_TargetSecurityGroup)
						{
							S_temp_TargetSecurityGroup_SecurityRule = S_temp_TargetSecurityGroup->security_rule_p;
							while(S_temp_TargetSecurityGroup_SecurityRule &&(0==strcmp(S_temp_TargetSecurityGroup_SecurityRule->ethertype,"IPv4")))
							{//遍历安全组规则
								UINT4 OuterIP=0;
								UINT1 OuterIPMask=0;
								GetIPAndMask_by_cidr(S_temp_TargetSecurityGroup_SecurityRule->remote_ip_prefix,&OuterIP,&OuterIPMask);
								char * Protocol=S_temp_TargetSecurityGroup_SecurityRule->protocol;
								char * Direction=S_temp_TargetSecurityGroup_SecurityRule->direction;
								UINT2 PortMax= (UINT2)(S_temp_TargetSecurityGroup_SecurityRule->port_range_max);
								UINT2 PortMin= (UINT2)(S_temp_TargetSecurityGroup_SecurityRule->port_range_min);
								UINT1 Priority=S_temp_TargetSecurityGroup_SecurityRule->priority;
								UINT1 Enable=S_temp_TargetSecurityGroup_SecurityRule->enabled;
								fabric_firewall_C_rule(&H_new_FirewallPolicy_FirewallRule_List,OuterIP,OuterIPMask,Protocol,Direction,PortMax,PortMin,Priority,Enable);
								
								S_temp_TargetSecurityGroup_SecurityRule =S_temp_TargetSecurityGroup_SecurityRule->next;
							}
						}
						S_temp_PortData_SecurityNode =S_temp_PortData_SecurityNode->next;
					}
					if(H_new_FirewallPolicy_FirewallRule_List)
					{
						fabric_firewall_U_policy(&G_firewall_temp_policy_list,S_temp_CurrentPort->ip_list[0],H_new_FirewallPolicy_FirewallRule_List);
					} 
				}
				else if(OPENSTACK_PORT_TYPE_CLBLOADBALANCER == S_temp_CurrentPort->type)
				{
					while(S_temp_PortData_SecurityNode)
					{//遍历安全组
						S_temp_PortData_SecurityNode_GroupName =(UINT1*) (S_temp_PortData_SecurityNode->data);
						//根据安全组名,翻译安全组规则到防火墙策略的过程
						S_temp_TargetSecurityGroup = find_security_group_into_SecurityGroupList(S_temp_PortData_SecurityNode_GroupName);
						if(S_temp_TargetSecurityGroup)
						{
							S_temp_TargetSecurityGroup_SecurityRule = S_temp_TargetSecurityGroup->security_rule_p;
							while(S_temp_TargetSecurityGroup_SecurityRule &&(0==strcmp(S_temp_TargetSecurityGroup_SecurityRule->ethertype,"IPv4")))
							{//遍历安全组规则
								UINT4 OuterIP=0;
								UINT1 OuterIPMask=0;
								GetIPAndMask_by_cidr(S_temp_TargetSecurityGroup_SecurityRule->remote_ip_prefix,&OuterIP,&OuterIPMask);
								char * Protocol=S_temp_TargetSecurityGroup_SecurityRule->protocol;
								char * Direction=S_temp_TargetSecurityGroup_SecurityRule->direction;
								UINT2 PortMax= (UINT2)(S_temp_TargetSecurityGroup_SecurityRule->port_range_max);
								UINT2 PortMin= (UINT2)(S_temp_TargetSecurityGroup_SecurityRule->port_range_min);
								UINT1 Priority=S_temp_TargetSecurityGroup_SecurityRule->priority;
								UINT1 Enable=S_temp_TargetSecurityGroup_SecurityRule->enabled;
								fabric_firewall_C_rule(&H_new_FirewallPolicy_FirewallRule_List,OuterIP,OuterIPMask,Protocol,Direction,PortMax,PortMin,Priority,Enable);
								
								S_temp_TargetSecurityGroup_SecurityRule =S_temp_TargetSecurityGroup_SecurityRule->next;
							}
						}
						S_temp_PortData_SecurityNode =S_temp_PortData_SecurityNode->next;
					}
					if(H_new_FirewallPolicy_FirewallRule_List)
					{
						fabric_firewall_U_policy(&G_firewall_temp_policy_list,S_temp_CurrentPort->ip_list[0],H_new_FirewallPolicy_FirewallRule_List);
					} 
				}
				else
				{
					//do nothing
				}
			}
		}
		S_temp_CurrentPort =S_temp_CurrentPort->next;
	}
	//LOG_PROC("INFO", "%s_%d",FN,LN);
}


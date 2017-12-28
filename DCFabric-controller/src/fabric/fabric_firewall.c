#include "fabric_firewall.h"
#include "openstack_host.h"
#include "fabric_openstack_external.h"
#include "../conn-svr/conn-svr.h"
#include "openstack_security_app.h"
#include "fabric_flows.h"



extern openstack_external_node_p g_openstack_floating_list;
//全局防火墙策略
fabric_firewall_policy_p	G_firewall_persist_policy_list = NULL;
fabric_firewall_policy_p	G_firewall_temp_policy_list = NULL;

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



/* by:Hongyu Yang
 * 创建一条防火墙规则
 */
fabric_firewall_rule_p fabric_firewall_C_rule(fabric_firewall_rule_p *firewall_rule_list,UINT4 OuterIP,UINT1 OuterIPMask,char * Protocol,char * Direction,UINT2 PortMax,UINT2 PortMin,UINT1 Priority,UINT1 Enable)
{
	fabric_firewall_rule_p H_new_firewall_rule =(fabric_firewall_rule_p)malloc(sizeof(fabric_firewall_rule));
	
	if(H_new_firewall_rule)
	{
		memset(H_new_firewall_rule, 0, sizeof(fabric_firewall_rule));
		H_new_firewall_rule->OuterIP =OuterIP;
		H_new_firewall_rule->OuterIPMask =OuterIPMask;
		memcpy(H_new_firewall_rule->Protocol,Protocol,FIREWALL_PROTOCOL_LEN);
		memcpy(H_new_firewall_rule->Direction,Direction,FIREWALL_DIRECTION_LEN);
		H_new_firewall_rule->PortMax =PortMax;
		H_new_firewall_rule->PortMin =PortMin;
		H_new_firewall_rule->Priority =Priority;
		H_new_firewall_rule->Enable =Enable;
		H_new_firewall_rule->pre = NULL;
		H_new_firewall_rule->next =NULL;
		
		if(*firewall_rule_list)
		{
			(*firewall_rule_list)->pre = H_new_firewall_rule;
			H_new_firewall_rule->next =(*firewall_rule_list);
			(*firewall_rule_list) =H_new_firewall_rule;
		}
		else
		{
			(*firewall_rule_list) =H_new_firewall_rule;
		}
		return (*firewall_rule_list) ;
	}
	else
	{//无法获取空间
		LOG_PROC("ERROR", "Cant get memory :%s_%d",FN,LN);
		return NULL;
	}
}

UINT1 fabric_firewall_Copy_ruleList(fabric_firewall_rule_p *firewall_rule_src_list,fabric_firewall_rule_p *firewall_rule_dst_list)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	//fabric_firewall_ShowRuleList(*firewall_rule_src_list);
	if((!firewall_rule_src_list)||(!firewall_rule_dst_list))
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		return 0;
	}
	fabric_firewall_rule_p S_temp_firewall_rule =NULL;
	S_temp_firewall_rule =*firewall_rule_src_list;
	while(S_temp_firewall_rule)
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		fabric_firewall_C_rule(firewall_rule_dst_list,S_temp_firewall_rule-> OuterIP,S_temp_firewall_rule->  OuterIPMask,S_temp_firewall_rule-> Protocol,S_temp_firewall_rule->  Direction,S_temp_firewall_rule->  PortMax,S_temp_firewall_rule->  PortMin,S_temp_firewall_rule->  Priority,S_temp_firewall_rule->  Enable);
		S_temp_firewall_rule =S_temp_firewall_rule->next;
	}
	//fabric_firewall_ShowRuleList(*firewall_rule_dst_list);
	return 1;
}


/* by:Hongyu Yang
 * 删除防火墙规则链
 */
UINT1 fabric_firewall_D_ruleList(fabric_firewall_rule_p *firewall_rule_list)
{
	if(!firewall_rule_list)
	{
		return 1;
	}
	fabric_firewall_rule_p S_temp_firewall_rule =NULL;
	while(*firewall_rule_list)
	{
		//LOG_PROC("INFO", "%s_%d_%p",FN,LN,*firewall_rule_list);
		S_temp_firewall_rule = *firewall_rule_list;
		(*firewall_rule_list)=(*firewall_rule_list)->next;
		free(S_temp_firewall_rule);
	}
	return 1;
}

/* by:Hongyu Yang
 * 新建一条防火墙策略
 */
fabric_firewall_policy_p fabric_firewall_C_policy(fabric_firewall_policy_p *firewall_policy_list,UINT4 InnerIP,fabric_firewall_rule_p firewall_rule_list)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	fabric_firewall_policy_p H_new_firewall_policy =(fabric_firewall_policy_p)malloc(sizeof(fabric_firewall_policy));
	
	if(H_new_firewall_policy)
	{
		memset(H_new_firewall_policy, 0, sizeof(fabric_firewall_policy));
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		H_new_firewall_policy->InnerIP =InnerIP;
		H_new_firewall_policy->FirewallRuleList =firewall_rule_list;
		H_new_firewall_policy->pre = NULL;
		H_new_firewall_policy->next =NULL;
		
		if(*firewall_policy_list)
		{
			(*firewall_policy_list)->pre = H_new_firewall_policy;
			H_new_firewall_policy->next =(*firewall_policy_list);
			(*firewall_policy_list) =H_new_firewall_policy;
		}
		else
		{
			(*firewall_policy_list) =H_new_firewall_policy;
		}
		return (*firewall_policy_list) ;
	}
	else
	{//无法获取空间
		LOG_PROC("ERROR", "Cant get memory :%s_%d",FN,LN);
		return NULL;
	}
}

/* by:Hongyu Yang
 * 查找防火墙策略
 */
fabric_firewall_policy_p fabric_firewall_R_policy(fabric_firewall_policy_p *firewall_policy_list,UINT4 InnerIP)
{
	if((InnerIP)&&(firewall_policy_list)&&(*firewall_policy_list))
	{
		fabric_firewall_policy_p S_current_firewall_rule = * firewall_policy_list;
		while(S_current_firewall_rule)
		{
			if(S_current_firewall_rule->InnerIP==InnerIP)
			{
				return S_current_firewall_rule;
			}
			S_current_firewall_rule =S_current_firewall_rule->next;
		}
		return NULL;
	}
	else
	{
		return NULL;
	}
}

/* by:Hongyu Yang
 * 更新一条防火墙策略
 */
fabric_firewall_policy_p fabric_firewall_U_policy(fabric_firewall_policy_p *firewall_policy_list,UINT4 InnerIP,fabric_firewall_rule_p firewall_rule_list)
{
	if(InnerIP)
	{
		//LOG_PROC("INFO", "%s_%d_%p",FN,LN,firewall_policy_list);
		fabric_firewall_policy_p S_target_firewall_policy = fabric_firewall_R_policy(firewall_policy_list,InnerIP);
		if(S_target_firewall_policy)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			fabric_firewall_D_ruleList(&(S_target_firewall_policy->FirewallRuleList));
			S_target_firewall_policy->FirewallRuleList =firewall_rule_list;
			return S_target_firewall_policy;
		}
		else
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			return fabric_firewall_C_policy(firewall_policy_list,InnerIP,firewall_rule_list);
		}
	}
	else
	{
		return  NULL;
	}
}

/* by:Hongyu Yang
 * 删除一条防火墙策略
 */
fabric_firewall_policy_p fabric_firewall_D_policy(fabric_firewall_policy_p *firewall_policy_list,UINT4 InnerIP)
{
	if(InnerIP)
	{
		fabric_firewall_policy_p S_target_firewall_policy = fabric_firewall_R_policy(firewall_policy_list,InnerIP);
		if(S_target_firewall_policy)
		{
			fabric_firewall_D_ruleList(&(S_target_firewall_policy->FirewallRuleList));
			if((S_target_firewall_policy ->pre) &&(S_target_firewall_policy ->next))
			{
				S_target_firewall_policy ->pre ->next =S_target_firewall_policy ->next;
				S_target_firewall_policy ->next ->pre =S_target_firewall_policy ->pre;
			}
			else if((S_target_firewall_policy ->pre ==NULL) &&(S_target_firewall_policy ->next))
			{
				*firewall_policy_list =S_target_firewall_policy ->next;
				S_target_firewall_policy ->next ->pre =NULL;
			}
			else if((S_target_firewall_policy ->pre) &&(S_target_firewall_policy ->next ==NULL))
			{
				S_target_firewall_policy ->pre ->next =NULL;
			}
			else
			{
				*firewall_policy_list =NULL;
			}
			free(S_target_firewall_policy);
			return *firewall_policy_list;
		}
		else
		{
			return NULL;
		}
	}
	else	
	{
		return NULL;
	}
}

/* by:Hongyu Yang
 * 删除防火墙策略链
 */
UINT1 fabric_firewall_D_policy_list(fabric_firewall_policy_p* firewall_policy_list)
{
	if((!firewall_policy_list)&&!(* firewall_policy_list))
	{
		return 1;
	}
	fabric_firewall_policy_p S_temp_firewall_policy =NULL;
	while(*firewall_policy_list)
	{
		S_temp_firewall_policy = *firewall_policy_list;
		(*firewall_policy_list)=(*firewall_policy_list)->next;
		fabric_firewall_D_ruleList(&(S_temp_firewall_policy->FirewallRuleList));
		free(S_temp_firewall_policy);
	}
	return 1;
}

/* by:Hongyu Yang
 * 比较所有防火墙的策略
 */
UINT1 fabric_firewall_Compare_policyList(fabric_firewall_policy_p *policy_list_new,fabric_firewall_policy_p *policy_list_old)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	fabric_firewall_policy_p S_temp_policy_list_new_currentNode =NULL;
	fabric_firewall_policy_p S_temp_policy_list_old_currentNode =NULL;
	
	UINT1 S_compare_result_newInnerIP_matched		=0;
	UINT1 S_compare_result_oldInnerIP_matched		=0;

	S_temp_policy_list_new_currentNode =*policy_list_new;
	S_temp_policy_list_old_currentNode =*policy_list_old;
	//在新的里面遍历旧的节点,发现删除的和比较匹配项的规则链
	while(S_temp_policy_list_old_currentNode)
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		S_temp_policy_list_new_currentNode =*policy_list_new;
		S_compare_result_oldInnerIP_matched		=0;
		while(S_temp_policy_list_new_currentNode)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			if(S_temp_policy_list_new_currentNode->InnerIP == S_temp_policy_list_old_currentNode->InnerIP)
			{
				S_compare_result_oldInnerIP_matched 	=1;
				break;
			}
			S_temp_policy_list_new_currentNode =S_temp_policy_list_new_currentNode->next;
		}
		
		if(S_compare_result_oldInnerIP_matched)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			//TBD比较安全规则链
			fabric_firewall_Compare_ruleList(S_temp_policy_list_new_currentNode->InnerIP,&(S_temp_policy_list_new_currentNode->FirewallRuleList),&(S_temp_policy_list_old_currentNode->FirewallRuleList));
		}
		else
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			//TBD发现删除的ip
			fabric_firewall_policy_removed_event(policy_list_old,&S_temp_policy_list_old_currentNode);
		}
		
		S_temp_policy_list_old_currentNode =S_temp_policy_list_old_currentNode->next;
	}
		
	S_temp_policy_list_new_currentNode =*policy_list_new;
	S_temp_policy_list_old_currentNode =*policy_list_old;	
	//在旧的里面遍历新的节点,发现新增的
	while(S_temp_policy_list_new_currentNode)
	{
		S_temp_policy_list_old_currentNode =*policy_list_old;
		S_compare_result_newInnerIP_matched		=0;
		while(S_temp_policy_list_old_currentNode)
		{
			if(S_temp_policy_list_new_currentNode->InnerIP == S_temp_policy_list_old_currentNode->InnerIP)
			{
				S_compare_result_newInnerIP_matched 	=1;
				break;
			}
			S_temp_policy_list_old_currentNode =S_temp_policy_list_old_currentNode->next;
		}
		
		if(S_compare_result_newInnerIP_matched)
		{
			//do nothing
		}
		else
		{
			//TBD发现新增的ip
			fabric_firewall_policy_added_event(policy_list_old,policy_list_new,&S_temp_policy_list_new_currentNode);
		}
		S_temp_policy_list_new_currentNode =S_temp_policy_list_new_currentNode->next;
	}
	return 1;
}

/* by:Hongyu Yang
 * 比较防火墙的规则链
 */
UINT1 fabric_firewall_Compare_ruleList(UINT4 InnerIP,fabric_firewall_rule_p *rule_list_new,fabric_firewall_rule_p *rule_list_old)
{
	fabric_firewall_rule_p S_temp_rule_list_new =	NULL;
	fabric_firewall_rule_p S_temp_rule_list_old =	NULL;
	
	UINT1 S_compare_result_newRule_matched		=0;
	UINT1 S_compare_result_oldRule_matched		=0;
	UINT1 S_compare_result_needReinstallFlows	=0;
	
	//LOG_PROC("INFO", "%s_%d_%ld_111111111111111111111111111111111",FN,LN,InnerIP);
	//fabric_firewall_ShowRuleList(*rule_list_new);
	//LOG_PROC("INFO", "*********************************************************************");
	//fabric_firewall_ShowRuleList(*rule_list_old);
	
	
	//对新旧规则链进行分类讨论
	if(!(rule_list_new&&rule_list_old))
	{
		return 0;
	}
	else
	{
		if(((*rule_list_new)==NULL)&&((*rule_list_old)==NULL))
		{//新旧规则链一致均为空
			//do nothing
		}
		else if(((*rule_list_new)!=NULL)&&((*rule_list_old)==NULL))
		{//规则链由空变非空
			S_temp_rule_list_new =*rule_list_new;
			while(S_temp_rule_list_new)
			{
				fabric_firewall_rule_added_event(InnerIP,S_temp_rule_list_new);
				S_temp_rule_list_new =S_temp_rule_list_new ->next;
			}
		}
		else if(((*rule_list_new)==NULL)&&((*rule_list_old)!=NULL))
		{//规则链由非空变空
			fabric_firewall_rule_changed_or_removed_event(InnerIP,*rule_list_new);
		}
		else
		{//新旧规则链均非空
			S_temp_rule_list_new =*rule_list_new;
			S_temp_rule_list_old =*rule_list_old;
			while(S_temp_rule_list_old)
			{//发现删除的或者改动的
				S_temp_rule_list_new =*rule_list_new;
				S_compare_result_oldRule_matched =0;
				
				while(S_temp_rule_list_new)
				{
					if(fabric_firewall_Compare_rule(&S_temp_rule_list_old,&S_temp_rule_list_new))
					{
						S_compare_result_oldRule_matched=1;
						break;
					}	
					S_temp_rule_list_new=S_temp_rule_list_new->next;
				}
				if(S_compare_result_oldRule_matched)
				{
					//do nothing
				}
				else
				{
					//发现删除的及改动的
					S_compare_result_needReinstallFlows =1;
					break;
				}
				S_temp_rule_list_old=S_temp_rule_list_old->next;
			}
			
			if(S_compare_result_needReinstallFlows)//如果发现有变动或者删除,则直接清空该InnerIP下的所有流表重新下发//也不再去比较新增的
			{
				fabric_firewall_rule_changed_or_removed_event(InnerIP,*rule_list_new);
			}
			else//若不存在改动或者删除,则再比较是否有新增的防火墙规则
			{
				S_temp_rule_list_new =*rule_list_new;
				S_temp_rule_list_old =*rule_list_old;
				while(S_temp_rule_list_new)
				{//发现新增的规则
					S_temp_rule_list_old =*rule_list_old;
					S_compare_result_newRule_matched = 0;
					while(S_temp_rule_list_old)
					{
						if(fabric_firewall_Compare_rule(&S_temp_rule_list_old,&S_temp_rule_list_new))
						{
							S_compare_result_newRule_matched =1;
							break;
						}
						S_temp_rule_list_old =S_temp_rule_list_old->next;
					}
					
					if(S_compare_result_newRule_matched)
					{
						//do nothing
					}
					else
					{
						//发现新增的
						fabric_firewall_rule_added_event(InnerIP,S_temp_rule_list_new);
					}
					S_temp_rule_list_new =S_temp_rule_list_new->next;
				}
			}
		}
	}

	//释放原有防火墙规则链的内存
	fabric_firewall_D_ruleList(rule_list_old);
	
	fabric_firewall_Copy_ruleList(rule_list_new,rule_list_old);
	//重置防火墙规则链
	//*rule_list_old =*rule_list_new;
	//TBD??????
	//rule_list_new =NULL;
	//LOG_PROC("INFO", "%s_%d_%ld_2222222222222222222222222222222222222222222222",FN,LN,InnerIP);
	return 1;
}

/* by:Hongyu Yang
 * 比较防火墙的规则
 */
UINT1 fabric_firewall_Compare_rule(const fabric_firewall_rule_p *rule_new,const fabric_firewall_rule_p *rule_old)
{
	if(!(rule_new&&rule_old&&(*rule_new)&&(*rule_old)))
	{
		return 0;
	}
	
	if(((*rule_new)->OuterIP ==(*rule_old)->OuterIP) && ((*rule_new)->OuterIPMask ==(*rule_old)->OuterIPMask))
	{
		if((0 == strcmp((*rule_new)->Protocol,(*rule_old)->Protocol)) && (0 == strcmp((*rule_new)->Direction,(*rule_old)->Direction)))
		{
			if(((*rule_new)->PortMax ==(*rule_old)->PortMax) && ((*rule_new)->PortMin ==(*rule_old)->PortMin))
			{
				if(((*rule_new)->Enable ==(*rule_old)->Enable))
				{
					return 1;
				}
			}
		}
	}
	
	return 0;
}

/* by:Hongyu Yang
 * 防火墙的策略增加事件
 */
UINT1 fabric_firewall_policy_added_event(fabric_firewall_policy_p * policy_list_oldList,fabric_firewall_policy_p * policy_list_newList,fabric_firewall_policy_p *policy_newAdded)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	//规则增加事件
	fabric_firewall_rule_p S_temp_firewall_rule =(*policy_newAdded)->FirewallRuleList;
	while(S_temp_firewall_rule)
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		fabric_firewall_rule_added_event((*policy_newAdded)->InnerIP,S_temp_firewall_rule);
		S_temp_firewall_rule =S_temp_firewall_rule->next;
	}
	
	//增加旧list中策略节点
	fabric_firewall_U_policy(policy_list_oldList,(*policy_newAdded)->InnerIP,(*policy_newAdded)->FirewallRuleList);
	//移除新list中策略节点
	(*policy_newAdded)->FirewallRuleList =NULL;
	//fabric_firewall_D_policy(policy_list_newList,(*policy_newAdded)->InnerIP);
	
	return 1;
}

/* by:Hongyu Yang
 * 防火墙的策略删除事件
 */
UINT1 fabric_firewall_policy_removed_event(fabric_firewall_policy_p *policy_list_oldList,fabric_firewall_policy_p *policy_oldRemoved)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	//删除流表
	fabric_firewall_rule_changed_or_removed_event((*policy_oldRemoved)->InnerIP,NULL);
	//删除策略节点
	fabric_firewall_D_policy(policy_list_oldList,(*policy_oldRemoved)->InnerIP);
	
	return 1;
}

/* by:Hongyu Yang
 * 防火墙增加1条规则事件
 */
UINT1 fabric_firewall_rule_added_event(UINT4 InnerIP,const fabric_firewall_rule_p firewall_rule_added)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	if(!firewall_rule_added)
	{
		return 0;
	}
	//对新增的规则下流表
	fabric_firewall_InstallFlowsByFirewallRule(InnerIP,firewall_rule_added,FIREWALL_INSTALL_FLOW_MODE_IGNORE_COMPARESWITCH,NULL);
	return 1;
}

/* by:Hongyu Yang
 * 防火墙改动或删除1条规则事件
 * 对InnerIP清空原有流表,下发规则链firewall_ruleList对应的流表
 */
UINT1 fabric_firewall_rule_changed_or_removed_event(UINT4 InnerIP, fabric_firewall_rule_p firewall_ruleList)
{
	//清空原有流表
	fabric_firewall_DeleteFlowsByFirewallInnerIP(InnerIP);
	//全部重新下发流表
	while(firewall_ruleList)
	{
		fabric_firewall_InstallFlowsByFirewallRule(InnerIP,firewall_ruleList,FIREWALL_INSTALL_FLOW_MODE_IGNORE_COMPARESWITCH,NULL);
		firewall_ruleList=firewall_ruleList->next;
	}
	return 1;
}

UINT1 fabric_firewall_IPIsRouterIP(UINT4 InnerIP)
{
	external_port_p S_TargetExternalPort =find_openstack_external_by_outer_ip(InnerIP);
	if(S_TargetExternalPort)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


gn_switch_t * fabric_firewall_FindSwitchByIP(UINT4 InnerIP,	UINT1* SwitchType)
{
	//LOG_PROC("INFO", "%s_%d_START",FN,LN);
	gn_switch_t * S_TargetSwitch =NULL;
	external_port_p S_TargetExternalPort =find_openstack_external_by_outer_ip(InnerIP);
	*SwitchType=SWICTH_TYPE_NOT_FINDED;
	if(S_TargetExternalPort)
	{//判断是不是路由器IP
		//LOG_PROC("INFO", "%s_%d_ROUTER IP",FN,LN);
		gn_switch_t * S_TargetSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
		if(!S_TargetSwitch)
		{
			*SwitchType=SWICTH_TYPE_NOT_FINDED;
			return NULL;
		}
		else
		{
			*SwitchType=SWICTH_TYPE_EXTERNNAL_NODE;
			return S_TargetSwitch;
		}
	}
	else
	{
		UINT4 LB_InsideIP =find_openstack_clbaas_vipfloatingpool_insideip_by_extip(InnerIP);
		if(LB_InsideIP)
		{//判断是不是LB的IP
			//LOG_PROC("INFO", "%s_%d_LB IP",FN,LN);
			external_port_p  epp = get_external_port_by_hostip(LB_InsideIP);
			if(epp)
			{
				gn_switch_t * S_TargetSwitchLB =find_sw_by_dpid(epp->external_dpid);
				if(S_TargetSwitchLB)
				{
					*SwitchType=SWICTH_TYPE_LBEXTERNAL_NODE;
					return S_TargetSwitchLB;
				}
			}
			return NULL;
		}
		else
		{
			p_fabric_host_node target_host_node = get_fabric_host_from_list_by_ip(InnerIP);
			if(target_host_node)
			{//判断是不是内网虚机IP
				//LOG_PROC("INFO", "%s_%d_HOST IP",FN,LN);
				S_TargetSwitch = target_host_node->sw;
				*SwitchType=SWICTH_TYPE_COMPUTE_NODE;
				return S_TargetSwitch;
			}
			else
			{
				*SwitchType=SWICTH_TYPE_NOT_FINDED;
				return NULL;
			}
		}
	}
}

UINT1 fabric_firewall_ProtocolStringToInt(char * ProtocolString)
{
	if(ProtocolString)
	{
		if(0==strcmp(ProtocolString,"tcp"))
		{
			return IP_PROTOCOL_TCP;
		}
		else if(0==strcmp(ProtocolString,"udp"))
		{
			return IP_PROTOCOL_UDP;
		}
		else if(0==strcmp(ProtocolString,"icmp"))
		{
			return IP_PROTOCOL_ICMP;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

UINT1 fabric_firewall_IsFloatingipMatchRouterIP(external_floating_ip_p FloatingIP,UINT4 RouterIP)
{
	if(FloatingIP)
	{
		external_port_p ext_port = get_external_port_by_floatip(FloatingIP->floating_ip);
		if(ext_port)
		{
			if(ext_port->external_outer_interface_ip == RouterIP)
			{
				return 1;
			}
		}
	}
	return 0;
}

/* by:Hongyu Yang
 * 针对InnerIP下载1条firewall_rule规则的流表
 * 如果mode == FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH,那么在下流表前会比较被下流表的交换机是否与TargetSwitch一致.若一致才会下载流表
 */
void fabric_firewall_InstallFlowsByFirewallRule(UINT4 InnerIP,const fabric_firewall_rule_p firewall_rule,UINT1 mode,gn_switch_t * TargetSwitch)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	//LOG_PROC("INFO", "%s_%d InnerIP:%d outerip:%d OuterIPMask:%d Protocol:%s Direction:%s PortMax:%d PortMin:%d Priority:%d Enable:%d",FN,LN,InnerIP,firewall_rule->OuterIP,
	//		firewall_rule->OuterIPMask,firewall_rule->Protocol,firewall_rule->Direction,firewall_rule->PortMax,firewall_rule->PortMin,firewall_rule->Priority,firewall_rule->Enable);
	gn_switch_t * S_TargetSwitch =NULL;
	UINT1 SwitchType =SWICTH_TYPE_NOT_FINDED;
	UINT4 SrcIP		=0;
	UINT4 DstIP 	=0;
	UINT4 IPMask	=0;
	UINT1 Protocol	=0;
	UINT2 SrcPort	=0;
	UINT2 DstPort	=0;
	UINT2 Priority	=0;
	//根据InnerIP找所在交换机
	S_TargetSwitch =fabric_firewall_FindSwitchByIP(InnerIP,&SwitchType);
	//根据规则内容分类下发流表
	if((firewall_rule->Enable)&&(S_TargetSwitch))
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		if(0==(strcmp(firewall_rule->Direction,FIREWALL_DIRECTION_IN)))
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			//初始化流表参数
			SrcIP=firewall_rule->OuterIP;
			DstIP=InnerIP;
			IPMask=firewall_rule->OuterIPMask;
			Protocol =fabric_firewall_ProtocolStringToInt(firewall_rule->Protocol);
			Priority=firewall_rule->Priority;
			
			//分类讨论下流表
			if((firewall_rule->PortMax ==0)&&(firewall_rule->PortMin ==0))
			{//只针对IP
				if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallIn_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
						}
					}
					else
					{
						install_add_FirewallIn_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
					}
				}
				else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallIn_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
						}
					}
					else
					{
						install_add_FirewallIn_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
					}
					
				}
				else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallIn_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
						}
					}
					else
					{
						install_add_FirewallIn_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
					}
					//此处还要找同路由器下的
					openstack_external_node_p node_p = g_openstack_floating_list;
					while(node_p)
					{
						external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
						if(efp)
						{
							if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
							{
								DstIP =efp->floating_ip;
								if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
								{
									if(S_TargetSwitch == TargetSwitch)
									{
										install_add_FirewallIn_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
									}
								}
								else
								{
									install_add_FirewallIn_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
								}
								p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
								if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
								{
									if(fixed_port->sw == TargetSwitch)
									{
										install_add_FirewallIn_withoutPort_flow(fixed_port->sw, SrcIP, DstIP, IPMask, Protocol, Priority);
									}
								}
								else
								{
									install_add_FirewallIn_withoutPort_flow(fixed_port->sw, SrcIP, DstIP, IPMask, Protocol, Priority);
								}
							}
						}
						node_p=node_p->next;
					}
				}
				else
				{
					//do nothing
				}
			}
			else if((firewall_rule->PortMin >0)&&(firewall_rule->PortMax == firewall_rule->PortMin))
			{//针对一个端口
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				SrcPort=firewall_rule->PortMax;
				DstPort=firewall_rule->PortMin;
				if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
						}
					}
					else
					{
						install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
					}
				}
				else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
						}
					}
					else
					{
						install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
					}
				}
				else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
						}
					}
					else
					{
						install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
					}
					openstack_external_node_p node_p = g_openstack_floating_list;
					while(node_p)
					{
						external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
						if(efp)
						{
							if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
							{
								DstIP =efp->floating_ip;
								if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
								{
									if(S_TargetSwitch == TargetSwitch)
									{
										install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
									}
								}
								else
								{
									install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
								p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
								if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
								{
									if(fixed_port->sw == TargetSwitch)
									{
										install_add_FirewallIn_withPort_flow(fixed_port->sw,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
									}
								}
								else
								{
									install_add_FirewallIn_withPort_flow(fixed_port->sw,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
							}
						}
						node_p=node_p->next;
					}
				}
				else
				{
					//do nothing
				}
			}
			else if((firewall_rule->PortMin >0)&&(firewall_rule->PortMax > firewall_rule->PortMin))
			{//针对端口范围
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				if((firewall_rule->PortMax ==65535)&&(firewall_rule->PortMin ==1))
				{
					//LOG_PROC("INFO", "%s_%d",FN,LN);
					if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
					{
						if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
						{
							if(S_TargetSwitch == TargetSwitch)
							{
								install_add_FirewallIn_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
							}
						}
						else
						{
							install_add_FirewallIn_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
						}
						
					}
					else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
					{
						if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
						{
							if(S_TargetSwitch == TargetSwitch)
							{
								install_add_FirewallIn_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
							}
						}
						else
						{
							install_add_FirewallIn_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
						}
					}
					else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
					{
						if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
						{
							if(S_TargetSwitch == TargetSwitch)
							{
								install_add_FirewallIn_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
							}
						}
						else
						{
							install_add_FirewallIn_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
						}
						openstack_external_node_p node_p = g_openstack_floating_list;
						while(node_p)
						{
							external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
							if(efp)
							{
								if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
								{
									DstIP =efp->floating_ip;
									if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
									{
										if(S_TargetSwitch == TargetSwitch)
										{
											install_add_FirewallIn_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
										}
									}
									else
									{
										install_add_FirewallIn_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
									}
									p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
									if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
									{
										if(fixed_port->sw == TargetSwitch)
										{
											install_add_FirewallIn_withoutPort_flow(fixed_port->sw,SrcIP,DstIP,IPMask, Protocol, Priority);
										}
									}
									else
									{
										install_add_FirewallIn_withoutPort_flow(fixed_port->sw,SrcIP,DstIP,IPMask, Protocol, Priority);
									}
								}
							}
							node_p=node_p->next;
						}
					}
					else
					{
						//do nothing
					}
				}
				else
				{
					//LOG_PROC("INFO", "%s_%d",FN,LN);
					if(Protocol == IP_PROTOCOL_ICMP)
					{//针对ICMP
						//LOG_PROC("INFO", "%s_%d",FN,LN);
						SrcPort=firewall_rule->PortMax;
						DstPort=firewall_rule->PortMin;
						if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
							}
							else
							{
								install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
							}
						}
						else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
							}
							else
							{
								install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
							}
						}
						else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
							}
							else
							{
								install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
							}
							openstack_external_node_p node_p = g_openstack_floating_list;
							while(node_p)
							{
								external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
								if(efp)
								{
									if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
									{
										DstIP =efp->floating_ip;
										if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
										{
											if(S_TargetSwitch == TargetSwitch)
											{
												install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
											}
										}
										else
										{
											install_add_FirewallIn_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
										}
										p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
										if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
										{
											if(fixed_port->sw == TargetSwitch)
											{
												install_add_FirewallIn_withPort_flow(fixed_port->sw,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
											}
										}
										else
										{
											install_add_FirewallIn_withPort_flow(fixed_port->sw,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
										}
									}
								}
								node_p=node_p->next;
							}
						}
						else
						{
							//do nothing
						}
					}
					else
					{//针对TCP/UDP
						
						//LOG_PROC("INFO", "%s_%d",FN,LN);
						DstPort=SrcPort;
						if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallIn_gotoController_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol);
								}
							}
							else
							{
								install_add_FirewallIn_gotoController_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol);
							}
						}
						else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallIn_gotoController_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol);
								}
							}
							else
							{
								install_add_FirewallIn_gotoController_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol);
							}
						}
						else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
						{
							//LOG_PROC("INFO", "%s_%d",FN,LN);
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallIn_gotoController_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol);
								}
							}
							else
							{
								install_add_FirewallIn_gotoController_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol);
							}
							openstack_external_node_p node_p = g_openstack_floating_list;
							while(node_p)
							{
								external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
								if(efp)
								{
									if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
									{
										//LOG_PROC("INFO", "%s_%d",FN,LN);
										DstIP =efp->floating_ip;
										if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
										{
											if(S_TargetSwitch == TargetSwitch)
											{
												install_add_FirewallIn_gotoController_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol);
											}
										}
										else
										{
											install_add_FirewallIn_gotoController_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol);
										}
										p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
										if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
										{
											if(fixed_port->sw == TargetSwitch)
											{
												install_add_FirewallIn_gotoController_flow(fixed_port->sw, SrcIP, DstIP, IPMask, Protocol);
											}
										}
										else
										{
											install_add_FirewallIn_gotoController_flow(fixed_port->sw, SrcIP, DstIP, IPMask, Protocol);
										}
									}
								}
								node_p=node_p->next;
							}
						}
						else
						{
							//do nothing
						}
						
					}
				}
			}
		}
		if(0==(strcmp(firewall_rule->Direction,FIREWALL_DIRECTION_OUT)))
		{
			//初始化流表参数
			SrcIP=InnerIP;
			DstIP=firewall_rule->OuterIP;
			IPMask=firewall_rule->OuterIPMask;
			Protocol =fabric_firewall_ProtocolStringToInt(firewall_rule->Protocol);
			Priority=firewall_rule->Priority;
			
			if((firewall_rule->PortMax ==0)&&(firewall_rule->PortMin ==0))
			{//只针对IP
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallOut_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
						}
					}
					else
					{
						install_add_FirewallOut_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
					}
				}
				else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallOut_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
						}
					}
					else
					{
						install_add_FirewallOut_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
					}
				}
				else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallOut_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
						}
					}
					else
					{
						install_add_FirewallOut_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
					}
					openstack_external_node_p node_p = g_openstack_floating_list;
					while(node_p)
					{
						external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
						if(efp)
						{
							if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
							{
								SrcIP =efp->floating_ip;
								if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
								{
									if(S_TargetSwitch == TargetSwitch)
									{
										install_add_FirewallOut_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
									}
								}
								else
								{
									install_add_FirewallOut_withoutPort_flow(S_TargetSwitch, SrcIP, DstIP, IPMask, Protocol, Priority);
								}
								p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
								if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
								{
									if(fixed_port->sw == TargetSwitch)
									{
										install_add_FirewallOut_withoutPort_flow(fixed_port->sw, SrcIP, DstIP, IPMask, Protocol, Priority);
									}
								}
								else
								{
									install_add_FirewallOut_withoutPort_flow(fixed_port->sw, SrcIP, DstIP, IPMask, Protocol, Priority);
								}
							}
						}
						node_p=node_p->next;
					}
				}
				else
				{
					//do nothing
				}
			}
			else if((firewall_rule->PortMin >0)&&(firewall_rule->PortMax == firewall_rule->PortMin))
			{//针对一个端口
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				SrcPort=firewall_rule->PortMax;
				DstPort=firewall_rule->PortMin;
				
				if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
						}
					}
					else
					{
						install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
					}
				}
				else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
						}
					}
					else
					{
						install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
					}
				}
				else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
				{
					if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
					{
						if(S_TargetSwitch == TargetSwitch)
						{
							install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
						}
					}
					else
					{
						install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
					}

					openstack_external_node_p node_p = g_openstack_floating_list;
					while(node_p)
					{
						external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
						if(efp)
						{
							if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
							{
								SrcIP =efp->floating_ip;
								if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
								{
									if(S_TargetSwitch == TargetSwitch)
									{
										install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
									}
								}
								else
								{
									install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
								p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
								if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
								{
									if(fixed_port->sw == TargetSwitch)
									{
										install_add_FirewallOut_withPort_flow(fixed_port->sw,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
									}
								}
								else
								{
									install_add_FirewallOut_withPort_flow(fixed_port->sw,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
							}
						}
						node_p=node_p->next;
					}
				}
				else
				{
					//do nothing
				}
			}
			else if((firewall_rule->PortMin >0)&&(firewall_rule->PortMax > firewall_rule->PortMin))
			{//针对端口范围
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				if((firewall_rule->PortMax ==65535)&&(firewall_rule->PortMin ==1))
				{//所有端口
					//LOG_PROC("INFO", "%s_%d",FN,LN);
					if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
					{
						if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
						{
							if(S_TargetSwitch == TargetSwitch)
							{
								install_add_FirewallOut_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
							}
						}
						else
						{
							install_add_FirewallOut_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
						}
					}
					else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
					{
						if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
						{
							if(S_TargetSwitch == TargetSwitch)
							{
								install_add_FirewallOut_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
							}
						}
						else
						{
							install_add_FirewallOut_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
						}
					}
					else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
					{
						if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
						{
							if(S_TargetSwitch == TargetSwitch)
							{
								install_add_FirewallOut_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
							}
						}
						else
						{
							install_add_FirewallOut_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
						}
						openstack_external_node_p node_p = g_openstack_floating_list;
						while(node_p)
						{
							external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
							if(efp)
							{
								if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
								{
									SrcIP =efp->floating_ip;
									if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
									{
										if(S_TargetSwitch == TargetSwitch)
										{
											install_add_FirewallOut_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
										}
									}
									else
									{
										install_add_FirewallOut_withoutPort_flow(S_TargetSwitch,SrcIP,DstIP,IPMask, Protocol, Priority);
									}

									p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
									if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
									{
										if(fixed_port->sw == TargetSwitch)
										{
											install_add_FirewallOut_withoutPort_flow(fixed_port->sw,SrcIP,DstIP,IPMask, Protocol, Priority);
										}
									}
									else
									{
										install_add_FirewallOut_withoutPort_flow(fixed_port->sw,SrcIP,DstIP,IPMask, Protocol, Priority);
									}
								}
							}
							node_p=node_p->next;
						}
					}
					else
					{
						//do nothing
					}
				}
				else
				{//局部端口
					//LOG_PROC("INFO", "%s_%d",FN,LN);
					if(Protocol == IP_PROTOCOL_ICMP)
					{//针对ICMP
						//LOG_PROC("INFO", "%s_%d",FN,LN);
						SrcPort=firewall_rule->PortMax;
						DstPort=firewall_rule->PortMin;
						if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
							}
							else
							{
								install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
							}
						}
						else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
							}
							else
							{
								install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
							}
						}
						else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
								}
							}
							else
							{
								install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
							}

							openstack_external_node_p node_p = g_openstack_floating_list;
							while(node_p)
							{
								external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
								if(efp)
								{
									if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
									{
										SrcIP =efp->floating_ip;
										if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
										{
											if(S_TargetSwitch == TargetSwitch)
											{
												install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
											}
										}
										else
										{
											install_add_FirewallOut_withPort_flow(S_TargetSwitch,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
										}
										p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
										if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
										{
											if(fixed_port->sw == TargetSwitch)
											{
												install_add_FirewallOut_withPort_flow(fixed_port->sw,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
											}
										}
										else
										{
											install_add_FirewallOut_withPort_flow(fixed_port->sw,SrcIP, DstIP, IPMask, Protocol, SrcPort, DstPort, Priority);
										}
									}
								}
								node_p=node_p->next;
							}
						}
						else
						{
							//do nothing
						}
					}
					else
					{//针对TCP/UDP
						
						//LOG_PROC("INFO", "%s_%d",FN,LN);
						DstPort=SrcPort;
						if(SWICTH_TYPE_COMPUTE_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallOut_gotoController_flow(S_TargetSwitch, SrcIP, DstIP,IPMask, Protocol);
								}
							}
							else
							{
								install_add_FirewallOut_gotoController_flow(S_TargetSwitch, SrcIP, DstIP,IPMask, Protocol);
							}
						}
						else if(SWICTH_TYPE_LBEXTERNAL_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallOut_gotoController_flow(S_TargetSwitch, SrcIP, DstIP,IPMask, Protocol);
								}
							}
							else
							{
								install_add_FirewallOut_gotoController_flow(S_TargetSwitch, SrcIP, DstIP,IPMask, Protocol);
							}
						}
						else if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
						{
							if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
							{
								if(S_TargetSwitch == TargetSwitch)
								{
									install_add_FirewallOut_gotoController_flow(S_TargetSwitch, SrcIP, DstIP,IPMask, Protocol);
								}
							}
							else
							{
								install_add_FirewallOut_gotoController_flow(S_TargetSwitch, SrcIP, DstIP,IPMask, Protocol);
							}
							openstack_external_node_p node_p = g_openstack_floating_list;
							while(node_p)
							{
								external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
								if(efp)
								{
									if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
									{
										SrcIP =efp->floating_ip;
										if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
										{
											if(S_TargetSwitch == TargetSwitch)
											{
												install_add_FirewallOut_gotoController_flow(S_TargetSwitch, SrcIP, DstIP,IPMask, Protocol);
											}
										}
										else
										{
											install_add_FirewallOut_gotoController_flow(S_TargetSwitch, SrcIP, DstIP,IPMask, Protocol);
										}
										p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
										if(FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH ==mode)
										{
											if(fixed_port->sw == TargetSwitch)
											{
												install_add_FirewallOut_gotoController_flow(fixed_port->sw, SrcIP, DstIP,IPMask, Protocol);
											}
										}
										else
										{
											install_add_FirewallOut_gotoController_flow(fixed_port->sw, SrcIP, DstIP,IPMask, Protocol);
										}
									}
								}
								node_p=node_p->next;
							}
						}
						else
						{
							//do nothing
						}

					}
				}
			}
		}
	}
}

/* by:Hongyu Yang
 * 针对InnerIP删除所有规则的流表
 */
void fabric_firewall_DeleteFlowsByFirewallInnerIP(UINT4 InnerIP)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	gn_switch_t * S_TargetSwitch =NULL;
	UINT1 SwitchType =SWICTH_TYPE_NOT_FINDED;
	//根据InnerIP找所在交换机
	S_TargetSwitch =fabric_firewall_FindSwitchByIP(InnerIP,&SwitchType);
	if(S_TargetSwitch)
	{
		//删除入口防火墙流表
		install_remove_FirewallIn_flow(S_TargetSwitch,InnerIP);
		//删除出口防火墙流表
		install_remove_FirewallOut_flow(S_TargetSwitch,InnerIP);
		if(SWICTH_TYPE_EXTERNNAL_NODE == SwitchType)
		{
			openstack_external_node_p node_p = g_openstack_floating_list;
			while(node_p)
			{
				external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
				if(efp)
				{
					if(fabric_firewall_IsFloatingipMatchRouterIP(efp,InnerIP))
					{
						install_remove_FirewallIn_flow(S_TargetSwitch,efp->floating_ip);
						install_remove_FirewallOut_flow(S_TargetSwitch,efp->floating_ip);
						p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
						install_remove_FirewallIn_flow(fixed_port->sw,efp->floating_ip);
						install_remove_FirewallOut_flow(fixed_port->sw,efp->floating_ip);
					}
				}
				node_p=node_p->next;
			}
		}
	}
}

UINT1 fabric_firewall_CheckIsTargetIPMatchCIDR(UINT4 TargetIP,UINT4 CIDR_IP,UINT1 CIDR_Mask)
{
	UINT4 Mask =0;
	UINT1 i=32;
	UINT1 j=CIDR_Mask;
	for(i=32;i>0;i--)
	{
		if(j>0)
		{
			Mask = (Mask|0x0001);
			j--;
		}
		if(i>1)
        {
            Mask =(Mask <<1);
        }
	}

	if((ntohl(TargetIP) & Mask )==(ntohl(CIDR_IP) & Mask ))
	{
		return 1;
	}
	return 0;
}

UINT4 fabric_firewall_CheckIPIsFloatingIPAndReturnRouterIP(UINT4 TargetIP)
{
	external_floating_ip_p FloatingIP = find_external_floating_ip_by_floating_ip(TargetIP);
	if(FloatingIP)
	{
		external_port_p ext_port = get_external_port_by_floatip(FloatingIP->floating_ip);
		if(ext_port)
		{
			return ext_port->external_outer_interface_ip;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
/* by:Hongyu Yang
 * 根据输入参数,判断防火墙是否应该通过
 */
UINT1 fabric_firewall_CheckFirewallAccessByDataPacketInfo(char*Direction,UINT4 SrcIP,UINT4 DstIP,UINT2 SrcPort,UINT2 DstPort,char*Protocol)
{
	LOG_PROC("INFO", "%s_%d_%s_%u_%u_%d_%d_s",FN,LN,Direction,SrcIP,DstIP,SrcPort,DstPort,Protocol);
	fabric_firewall_policy_p S_temp_CurrentFirewallFolicy   =G_firewall_persist_policy_list;
	fabric_firewall_rule_p   S_temp_CurrentFirewallRule		=NULL;
	UINT4 FloatingIPToRouterIP =0;
	UINT4 ComparedIP =0;
	if (0 == get_security_group_on_config()) 
	{
		return FIREWALL_ACCESS_THROUGH;
	}
	
	if(0 == strcmp(Direction,FIREWALL_DIRECTION_IN))
	{
		FloatingIPToRouterIP = fabric_firewall_CheckIPIsFloatingIPAndReturnRouterIP(DstIP);
		if (FloatingIPToRouterIP)
		{
			ComparedIP = FloatingIPToRouterIP;
		}
		else
		{
			ComparedIP =DstIP;
		}
		while(S_temp_CurrentFirewallFolicy)
		{//遍历防火墙策略
			if(S_temp_CurrentFirewallFolicy->InnerIP == ComparedIP)
			{//匹配innerIP
				S_temp_CurrentFirewallRule =S_temp_CurrentFirewallFolicy->FirewallRuleList;
				while(S_temp_CurrentFirewallRule)
				{//遍历防火墙规则
					if(0 == strcmp(Protocol,S_temp_CurrentFirewallRule->Protocol))
					{//比较协议
						if(fabric_firewall_CheckIsTargetIPMatchCIDR(SrcIP,S_temp_CurrentFirewallRule->OuterIP,S_temp_CurrentFirewallRule->OuterIPMask))
						{//匹配outerIP以及网段
							if((S_temp_CurrentFirewallRule->PortMax ==0)&&(S_temp_CurrentFirewallRule->PortMin ==0))
							{//无端口限制
								return FIREWALL_ACCESS_THROUGH;
							}
							else if((S_temp_CurrentFirewallRule->PortMax >0)&&(S_temp_CurrentFirewallRule->PortMin == S_temp_CurrentFirewallRule->PortMax))
							{//限制某一固定端口
								if(S_temp_CurrentFirewallRule->PortMax == DstPort)
								{
									return FIREWALL_ACCESS_THROUGH;
								}
							}
							else if((S_temp_CurrentFirewallRule->PortMax >0)&&(S_temp_CurrentFirewallRule->PortMax > S_temp_CurrentFirewallRule->PortMin))
							{//限制端口范围
								if((S_temp_CurrentFirewallRule->PortMax >= DstPort) &&(DstPort >= S_temp_CurrentFirewallRule->PortMin))
								{
									return FIREWALL_ACCESS_THROUGH;
								}
							}
						}
					}
					S_temp_CurrentFirewallRule =S_temp_CurrentFirewallRule->next;
				}
				return FIREWALL_ACCESS_DENY;
			}
			S_temp_CurrentFirewallFolicy=S_temp_CurrentFirewallFolicy->next;
		}
		return FIREWALL_ACCESS_INNERIP_NOINCLUDE;
	}
	else if(0 == strcmp(Direction,FIREWALL_DIRECTION_OUT))
	{
		FloatingIPToRouterIP = fabric_firewall_CheckIPIsFloatingIPAndReturnRouterIP(SrcIP);
		if (FloatingIPToRouterIP)
		{
			ComparedIP = FloatingIPToRouterIP;
		}
		else
		{
			ComparedIP =SrcIP;
		}
		while(S_temp_CurrentFirewallFolicy)
		{//遍历防火墙策略
			if(S_temp_CurrentFirewallFolicy->InnerIP == ComparedIP)
			{//匹配innerIP
				S_temp_CurrentFirewallRule =S_temp_CurrentFirewallFolicy->FirewallRuleList;
				while(S_temp_CurrentFirewallRule)
				{//遍历防火墙规则
					if(0 == strcmp(Protocol,S_temp_CurrentFirewallRule->Protocol))
					{//比较协议
						if(fabric_firewall_CheckIsTargetIPMatchCIDR(DstIP,S_temp_CurrentFirewallRule->OuterIP,S_temp_CurrentFirewallRule->OuterIPMask))
						{//匹配outerIP以及网段
							if((S_temp_CurrentFirewallRule->PortMax ==0)&&(S_temp_CurrentFirewallRule->PortMin ==0))
							{//无端口限制
								return FIREWALL_ACCESS_THROUGH;
							}
							else if((S_temp_CurrentFirewallRule->PortMax >0)&&(S_temp_CurrentFirewallRule->PortMin == S_temp_CurrentFirewallRule->PortMax))
							{//限制某一固定端口
								if(S_temp_CurrentFirewallRule->PortMax == SrcPort)
								{
									return FIREWALL_ACCESS_THROUGH;
								}
							}
							else if((S_temp_CurrentFirewallRule->PortMax >0)&&(S_temp_CurrentFirewallRule->PortMax > S_temp_CurrentFirewallRule->PortMin))
							{//限制端口范围
								if((S_temp_CurrentFirewallRule->PortMax >= SrcPort) &&(DstPort >= S_temp_CurrentFirewallRule->PortMin))
								{
									return FIREWALL_ACCESS_THROUGH;
								}
							}
						}
					}
					S_temp_CurrentFirewallRule =S_temp_CurrentFirewallRule->next;
				}
				return FIREWALL_ACCESS_DENY;
			}
			S_temp_CurrentFirewallFolicy =S_temp_CurrentFirewallFolicy->next;
		}
		return FIREWALL_ACCESS_INNERIP_NOINCLUDE;
	}
	else
	{
		return FIREWALL_ACCESS_INNERIP_NOINCLUDE;
	}
}

void fabric_firewall_ShowAllTempPolicy(void)
{
	printf("temp++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	fabric_firewall_policy_p S_temp_currentPolicy =G_firewall_temp_policy_list;
	while(S_temp_currentPolicy)
	{
		printf("InnerIP:	%d\n", S_temp_currentPolicy->InnerIP);
		printf("pre: 		%p\n", S_temp_currentPolicy->pre);
		printf("next: 		%p\n", S_temp_currentPolicy->next);
		fabric_firewall_rule_p S_temp_currentPolicyRule =S_temp_currentPolicy->FirewallRuleList;
		while(S_temp_currentPolicyRule)
		{
			printf("	OuterIP: 	%d\n", 	S_temp_currentPolicyRule->OuterIP);
			printf("	OuterIPMask:%d\n", 		S_temp_currentPolicyRule->OuterIPMask);
			printf("	Protocol: 	%s\n", 		S_temp_currentPolicyRule->Protocol);
			printf("	Direction: 	%s\n", 		S_temp_currentPolicyRule->Direction);
			printf("	PortMax: 	%d\n", 		S_temp_currentPolicyRule->PortMax);
			printf("	PortMin: 	%d\n", 		S_temp_currentPolicyRule->PortMin);
			printf("	Priority: 	%d\n", 		S_temp_currentPolicyRule->Priority);
			printf("	Enable: 	%d\n", 		S_temp_currentPolicyRule->Enable);
			printf("	pre: 		%p\n", 		S_temp_currentPolicyRule->pre);
			printf("	next: 		%p\n", 		S_temp_currentPolicyRule->next);
			printf("\n");
			S_temp_currentPolicyRule=S_temp_currentPolicyRule->next;
		}
		printf("-------------------------------------------------------------\n");

		
		S_temp_currentPolicy=S_temp_currentPolicy->next;
	}
}

void fabric_firewall_ShowAllPersistPolicy(void)
{
	printf("persist++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	fabric_firewall_policy_p S_temp_currentPolicy =G_firewall_persist_policy_list;
	while(S_temp_currentPolicy)
	{
		printf("InnerIP:	%d\n", S_temp_currentPolicy->InnerIP);
		printf("pre: 		%p\n", S_temp_currentPolicy->pre);
		printf("next: 		%p\n", S_temp_currentPolicy->next);
		fabric_firewall_rule_p S_temp_currentPolicyRule =S_temp_currentPolicy->FirewallRuleList;
		while(S_temp_currentPolicyRule)
		{
			printf("	OuterIP: 	%d\n", 	S_temp_currentPolicyRule->OuterIP);
			printf("	OuterIPMask:%d\n", 		S_temp_currentPolicyRule->OuterIPMask);
			printf("	Protocol: 	%s\n", 		S_temp_currentPolicyRule->Protocol);
			printf("	Direction: 	%s\n", 		S_temp_currentPolicyRule->Direction);
			printf("	PortMax: 	%d\n", 		S_temp_currentPolicyRule->PortMax);
			printf("	PortMin: 	%d\n", 		S_temp_currentPolicyRule->PortMin);
			printf("	Priority: 	%d\n", 		S_temp_currentPolicyRule->Priority);
			printf("	Enable: 	%d\n", 		S_temp_currentPolicyRule->Enable);
			printf("	pre: 		%p\n", 		S_temp_currentPolicyRule->pre);
			printf("	next: 		%p\n", 		S_temp_currentPolicyRule->next);
			printf("\n");
			S_temp_currentPolicyRule=S_temp_currentPolicyRule->next;
		}
		printf("-------------------------------------------------------------\n");

		
		S_temp_currentPolicy=S_temp_currentPolicy->next;
	}
}

void fabric_firewall_ShowRuleList(fabric_firewall_rule_p RuleList)
{
		fabric_firewall_rule_p S_temp_currentPolicyRule =RuleList;
		while(S_temp_currentPolicyRule)
		{
			printf("	OuterIP: 	%d\n", 	S_temp_currentPolicyRule->OuterIP);
			printf("	OuterIPMask:%d\n", 		S_temp_currentPolicyRule->OuterIPMask);
			printf("	Protocol: 	%s\n", 		S_temp_currentPolicyRule->Protocol);
			printf("	Direction: 	%s\n", 		S_temp_currentPolicyRule->Direction);
			printf("	PortMax: 	%d\n", 		S_temp_currentPolicyRule->PortMax);
			printf("	PortMin: 	%d\n", 		S_temp_currentPolicyRule->PortMin);
			printf("	Priority: 	%d\n", 		S_temp_currentPolicyRule->Priority);
			printf("	Enable: 	%d\n", 		S_temp_currentPolicyRule->Enable);
			printf("	pre: 		%p\n", 		S_temp_currentPolicyRule->pre);
			printf("	next: 		%p\n", 		S_temp_currentPolicyRule->next);
			printf("\n");
			S_temp_currentPolicyRule=S_temp_currentPolicyRule->next;
		}
		printf("-------------------------------------------------------------\n");


}

void fabric_firewall_RefreshFlowBySwitch(gn_switch_t * TargetSwitch)
{
	UINT4						S_temp_CurrentInnerIP =0;
	fabric_firewall_policy_p 	S_temp_current_FirewallPolicy =NULL;
	fabric_firewall_rule_p 		S_temp_current_FirewallPolicyRule =NULL;
	
	S_temp_current_FirewallPolicy = G_firewall_persist_policy_list;
	while (S_temp_current_FirewallPolicy)
	{
		S_temp_CurrentInnerIP =S_temp_current_FirewallPolicy->InnerIP;
		S_temp_current_FirewallPolicyRule =  S_temp_current_FirewallPolicy->FirewallRuleList;
		while(S_temp_current_FirewallPolicyRule)
		{
			fabric_firewall_InstallFlowsByFirewallRule(S_temp_CurrentInnerIP,S_temp_current_FirewallPolicyRule,FIREWALL_INSTALL_FLOW_MODE_COMPARESWITCH,TargetSwitch);
			S_temp_current_FirewallPolicyRule = S_temp_current_FirewallPolicyRule->next;
		}
		S_temp_current_FirewallPolicy =S_temp_current_FirewallPolicy->next;
	}
}

void fabric_firewall_SetSwitchFlowInstallFlag (gn_switch_t * TargetSwitch)
{
	TargetSwitch->FirewallFlowInstallFlag =3;
}

void fabric_firewall_SwitchFlowInstallFlagCountDown(gn_switch_t * TargetSwitch)
{
	if(TargetSwitch->FirewallFlowInstallFlag)
	{
		TargetSwitch->FirewallFlowInstallFlag --;
	}
}

void fabric_firewall_RefreshSwitch(void)
{
	UINT4 num;
    gn_switch_t  *CurrentSwitch = NULL;

    for(num=0; num < g_server.max_switch; num++)
    {
        CurrentSwitch = &g_server.switches[num];
        if(CurrentSwitch&&(CONNECTED == CurrentSwitch->conn_state)&& (CurrentSwitch->FirewallFlowInstallFlag))
        {
			fabric_firewall_RefreshFlowBySwitch(CurrentSwitch);
			fabric_firewall_SwitchFlowInstallFlagCountDown(CurrentSwitch);
        }
    }
}

void fabric_firewall_RefreshFirewallFlow(void)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	fabric_firewall_policy_p	S_temp_CurrentPolicy=G_firewall_persist_policy_list;
	fabric_firewall_rule_p S_temp_CurrentRule =NULL;
	while(S_temp_CurrentPolicy)
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		S_temp_CurrentRule =S_temp_CurrentPolicy->FirewallRuleList;
		while(S_temp_CurrentRule)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			fabric_firewall_InstallFlowsByFirewallRule(S_temp_CurrentPolicy->InnerIP,S_temp_CurrentRule,FIREWALL_INSTALL_FLOW_MODE_IGNORE_COMPARESWITCH,NULL);
			S_temp_CurrentRule = S_temp_CurrentRule->next;
		}
		S_temp_CurrentPolicy =S_temp_CurrentPolicy->next;
	}
}

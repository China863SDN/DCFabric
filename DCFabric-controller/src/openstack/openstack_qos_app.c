#include "openstack_qos_app.h"
#include "../restful-svr/openstack-server.h"
#include "fabric_host.h"
#include "fabric_openstack_external.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "fabric_flows.h"
#include "../conn-svr/conn-svr.h"
#include "../qos-mgr/qos-mgr.h"
#include "../qos-mgr/qos-policy.h"

char * TYPE_Router ="network:router_gateway";
char * TYPE_FloatingIP ="network:floatingip";
char * TYPE_Loadbalance ="network:LOADBALANCER_VIP";

openstack_qos_binding_p 	G_openstack_qos_floating_binding_List 		=NULL;	
openstack_qos_binding_p 	G_openstack_qos_router_binding_List 		=NULL;
openstack_qos_binding_p 	G_openstack_qos_loadbalancer_binding_List 	=NULL;
openstack_qos_rule_p		G_openstack_qos_rule_List					=NULL;
openstack_qos_instance_p 	G_openstack_qos_instance_List				=NULL;

void openstack_qos_ReloadRules(void);
void openstack_qos_rule_ConventDataFromJSON(char *jsonString);

openstack_qos_rule_p    openstack_qos_C_rule(openstack_qos_rule_p *qos_rule_list,char * id,char * name,char * description,char * tenant_id,char * protocol,char * type,UINT1  shared,UINT8  max_rate);
openstack_qos_rule_p    openstack_qos_R_rule(openstack_qos_rule_p *qos_rule_list,char * id);
openstack_qos_rule_p    openstack_qos_U_rule(openstack_qos_rule_p *qos_rule_list,char * id,char * name,char * description,char * tenant_id,char * protocol,char * type,UINT1  shared,UINT8  max_rate);
openstack_qos_rule_p    openstack_qos_D_rule(openstack_qos_rule_p *qos_rule_list,char * id);
void openstack_qos_D_rule_list(openstack_qos_rule_p qos_rule_list);

UINT1 openstack_qos_Compare_rule(openstack_qos_rule_p *rule_list_new,openstack_qos_rule_p *rule_list_old);
UINT1 openstack_qos_rule_added_event(openstack_qos_rule_p *rule_list_old,openstack_qos_rule_p rule_added);
UINT1 openstack_qos_rule_changed_event(openstack_qos_rule_p *rule_list_old,openstack_qos_rule_p rule_changed);
UINT1 openstack_qos_rule_removed_event(openstack_qos_rule_p *rule_list_old,openstack_qos_rule_p rule_removed);

openstack_qos_binding_p    openstack_qos_C_binding(openstack_qos_binding_p *qos_binding_list,char * id,char * qos_id,char * tenant_id,char * device_owner,char * subnet_id,char * router_id,UINT4 ip);
openstack_qos_binding_p    openstack_qos_R_binding(openstack_qos_binding_p *qos_binding_list,char * id);
openstack_qos_binding_p    openstack_qos_U_binding(openstack_qos_binding_p *qos_binding_list,char * id,char * qos_id,char * tenant_id,char * device_owner,char * subnet_id,char * router_id,UINT4 ip);
openstack_qos_binding_p    openstack_qos_D_binding(openstack_qos_binding_p *qos_binding_list,char * id);
openstack_qos_binding_p    openstack_qos_Compare_binding(openstack_qos_binding_p* qos_binding_list_new,openstack_qos_binding_p *qos_binding_list_old);
void openstack_qos_D_binding_list(openstack_qos_binding_p qos_binding_list);

UINT1 openstack_qos_binding_added_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_added);
UINT1 openstack_qos_binding_changed_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_old,openstack_qos_binding_p binding_new  );
UINT1 openstack_qos_binding_removed_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_removed);

UINT1 openstack_qos_binding_WithqosRule_added_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_added);
UINT1 openstack_qos_binding_WithoutqosRule_added_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_added);
UINT1 openstack_qos_binding_qosRule_added_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_old,openstack_qos_binding_p binding_new);
UINT1 openstack_qos_binding_qosRule_changed_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_old,openstack_qos_binding_p binding_new);
UINT1 openstack_qos_binding_qosRule_removeded_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_old,openstack_qos_binding_p binding_new);
UINT1 openstack_qos_binding_PortWithqosRule_removeded_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_removed);
UINT1 openstack_qos_binding_PortWithoutqosRule_removeded_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_removed);


openstack_qos_instance_p    openstack_qos_C_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id,char * qos_rule_id,char *qos_operation_type,UINT8 	switch_dpid,UINT4	meter_id,UINT4	table_id);
openstack_qos_instance_p    openstack_qos_R_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id);
openstack_qos_instance_p    openstack_qos_U_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id,char * qos_rule_id,char *qos_operation_type,UINT8 	switch_dpid,UINT4	meter_id,UINT4	table_id);
openstack_qos_instance_p    openstack_qos_D_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id);

/* by:yhy
 * 初始化qos binding list
 */
void openstack_qos_InitBindingList(openstack_qos_binding_p * Binding_List)
{
	openstack_qos_binding_p H_new_qos_binding =(openstack_qos_binding_p)malloc(sizeof(openstack_qos_binding));
	
	if(H_new_qos_binding)
	{
		memset(H_new_qos_binding, 0, sizeof(openstack_qos_binding));
		strcpy(H_new_qos_binding->qos_id,"default");
		strcpy(H_new_qos_binding->id,"default");
		strcpy(H_new_qos_binding->tenant_id,"default");
		strcpy(H_new_qos_binding->device_owner,"default");
		strcpy(H_new_qos_binding->subnet_id,"default");
		strcpy(H_new_qos_binding->router_id,"default");
		H_new_qos_binding->ip=0;
		H_new_qos_binding->pre=NULL;
		H_new_qos_binding->next=NULL;
		
		* Binding_List = H_new_qos_binding;
	}
	else
	{
		LOG_PROC("ERROR", "Cant get memory :%s_%d",FN,LN);
	}
}

/* by:yhy
 * qos binding list所有全局变量的初始化
 */
void openstack_qos_Init(void)
{
	openstack_qos_InitBindingList(&G_openstack_qos_floating_binding_List);
	openstack_qos_InitBindingList(&G_openstack_qos_router_binding_List);
	openstack_qos_InitBindingList(&G_openstack_qos_loadbalancer_binding_List);
}

/* by:yhy 
 * 下载默认qos跳转流表
 */
void openstack_qos_InstallDefaultQosFlow(gn_switch_t * sw,UINT1 FlowTable,UINT1 GotoTable)
{
	install_add_QOS_jump_default_flow(sw,FlowTable,GotoTable);
}

/*QOS RULE相关----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* by:yhy 
 * 刷新QOS信息
 */
void openstack_qos_ReloadRules(void)
{
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	char g_openstack_ip[16] = {0};
	UINT4 g_openstack_port = 9696;
	UINT1 g_openstack_on = (NULL == value)?0:atoll(value);
	if( 1 == g_openstack_on)
	{
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.52.200", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoll(value));

		getOpenstackInfo(g_openstack_ip, "/v2.0/qoses", g_openstack_port, "qoses" , NULL, EOPENSTACK_GET_QOS);
	} 
}

/* by:yhy 
 * 注:max_rate_s转换时可能出现溢出
 */
void openstack_qos_rule_ConventDataFromJSON(char *jsonString)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json =NULL;
	json_t *temp=NULL;
	json_t *temp_second=NULL;
	openstack_qos_rule_p		S_openstack_qos_rule_List		=NULL;
	
	char 	id			[QOS_ID_LEN]			={0};
	char 	name		[QOS_NAME_LEN]			={0};			
	char 	description	[QOS_DESCRIPTION_LEN]	={0};		
	char 	tenant_id	[QOS_TENANT_LEN]		={0};					
	char 	protocol	[QOS_PROTOCOL_LEN]		={0};	
	char 	type		[QOS_TYPE_LEN]			={0};	
	//char	shared_s	[64]					={0};
	char	max_rate_s	[64]					={0};
	UINT1 	shared;	
	UINT8 	max_rate;
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	
	parse_type = json_parse_document(&json,tempString);
	if (parse_type != JSON_OK)
	{
		return;
	}
	else
	{
		if(json)
		{
			json_t *qoses = json_find_first_label(json, "qoses");
			if(qoses&&qoses->child)
			{
				json_t *qos  = qoses->child->child;
				while(qos)
				{
					temp = json_find_first_label(qos, "description");
					if(temp)
					{
						strcpy(description,temp->child->text);
						json_free_value(&temp);
					}
					
					temp = json_find_first_label(qos, "tenant_id");
					if(temp)
					{
						strcpy(tenant_id,temp->child->text);
						json_free_value(&temp);
					}
					
					temp = json_find_first_label(qos, "policies");
					if(temp)
					{
						if(temp->child && JSON_OBJECT == temp->child->type)
						{
							temp_second = json_find_first_label(temp->child, "max_rate");
							if(temp_second)
							{
								strcpy(max_rate_s,temp_second->child->text);
								json_free_value(&temp_second);
								max_rate =atol(max_rate_s);
							}
							temp_second = json_find_first_label(temp->child, "protocol");
							if(temp_second)
							{
								strcpy(protocol,temp_second->child->text);
								json_free_value(&temp_second);
							}
						}
						json_free_value(&temp);
					}
					
					temp = json_find_first_label(qos, "shared");
					if(temp)
					{
						if(temp->child->type ==JSON_TRUE)
						{
							shared=1;
						}
						else
						{
							shared=0;
						}
						json_free_value(&temp);
					}
					
					temp = json_find_first_label(qos, "type");
					if(temp)
					{
						strcpy(type,temp->child->text);
						json_free_value(&temp);
					}
					
					temp = json_find_first_label(qos, "id");
					if(temp)
					{
						strcpy(id,temp->child->text);
						json_free_value(&temp);
					}
					
					temp = json_find_first_label(qos, "name");
					if(temp)
					{
						strcpy(name,temp->child->text);
						json_free_value(&temp);
					}
					
					qos =qos->next;
					openstack_qos_U_rule(&S_openstack_qos_rule_List,id,name,description,tenant_id,protocol,type,shared,max_rate);
					//LOG_PROC("INFO", "%s_%d_%p",FN,LN,S_openstack_qos_rule_List);
				}
				
				json_free_value(&qoses);
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				openstack_qos_Compare_rule(&S_openstack_qos_rule_List, &G_openstack_qos_rule_List);
				openstack_qos_D_rule_list(S_openstack_qos_rule_List);
			}
			json_free_value_all(&json);
		}
	}
}

/* by:yhy
 * 增
 */	
openstack_qos_rule_p    openstack_qos_C_rule(openstack_qos_rule_p * qos_rule_list,char * id,char * name,char * description,char * tenant_id,char * protocol,char * type,UINT1  shared,UINT8  max_rate)
{
	if((shared == 1) ||(shared == 0))
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		openstack_qos_rule_p H_new_qos_rule =(openstack_qos_rule_p)malloc(sizeof(openstack_qos_rule));
		
		if(H_new_qos_rule)
		{
			memset(H_new_qos_rule, 0, sizeof(openstack_qos_rule));
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			memcpy(H_new_qos_rule->id,id,QOS_ID_LEN);
			memcpy(H_new_qos_rule->name,name,QOS_NAME_LEN);
			memcpy(H_new_qos_rule->description,description,QOS_DESCRIPTION_LEN);
			memcpy(H_new_qos_rule->tenant_id,tenant_id,QOS_TENANT_LEN);
			memcpy(H_new_qos_rule->protocol,protocol,QOS_PROTOCOL_LEN);
			memcpy(H_new_qos_rule->type,type,QOS_TYPE_LEN);
			H_new_qos_rule->shared =shared;
			H_new_qos_rule->max_rate =max_rate;
			H_new_qos_rule->pre = NULL;
			H_new_qos_rule->next =NULL;
			
			if(*qos_rule_list)
			{
				(*qos_rule_list)->pre = H_new_qos_rule;
				H_new_qos_rule->next =(*qos_rule_list);
				(*qos_rule_list) =H_new_qos_rule;
			}
			else
			{
				(*qos_rule_list) =H_new_qos_rule;
			}
			return (*qos_rule_list) ;
		}
		else
		{//无法获取空间
			LOG_PROC("ERROR", "Cant get memory :%s_%d",FN,LN);
			return NULL;
		}
	}
	else
	{
		return NULL;
	}
}

/* by:yhy
 * 查
 */
openstack_qos_rule_p    openstack_qos_R_rule(openstack_qos_rule_p * qos_rule_list,char * id)
{
	if(id)
	{
		openstack_qos_rule_p S_current_qos_rule = * qos_rule_list;
		while(S_current_qos_rule)
		{
			if(0 == strcmp(S_current_qos_rule->id,id))
			{
				return S_current_qos_rule;
			}
			S_current_qos_rule =S_current_qos_rule->next;
		}
		return NULL;
	}
	else
	{
		return NULL;
	}
}

/* by:yhy
 * 改
 */
openstack_qos_rule_p    openstack_qos_U_rule(openstack_qos_rule_p * qos_rule_list,char * id,char * name,char * description,char * tenant_id,char * protocol,char * type,UINT1  shared,UINT8  max_rate)
{
	if(id)
	{
		//LOG_PROC("INFO", "%s_%d_%p",FN,LN,qos_rule_list);
		openstack_qos_rule_p S_target_qos_rule = openstack_qos_R_rule(qos_rule_list,id);
		if(S_target_qos_rule)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			memcpy(S_target_qos_rule->name,name,QOS_NAME_LEN);
			memcpy(S_target_qos_rule->description,description,QOS_DESCRIPTION_LEN);
			memcpy(S_target_qos_rule->tenant_id,tenant_id,QOS_TENANT_LEN);
			memcpy(S_target_qos_rule->protocol,protocol,QOS_PROTOCOL_LEN);
			memcpy(S_target_qos_rule->type,type,QOS_TYPE_LEN);
			S_target_qos_rule->shared =shared;
			S_target_qos_rule->max_rate =max_rate;
			return S_target_qos_rule;
		}
		else
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			return openstack_qos_C_rule(qos_rule_list,id,name,description,tenant_id,protocol,type,shared,max_rate);
		}
	}
	else
	{
		return  NULL;
	}
}

/* by:yhy
 * 删
 */
openstack_qos_rule_p    openstack_qos_D_rule(openstack_qos_rule_p *qos_rule_list,char * id)
{
	if(id)
	{
		openstack_qos_rule_p S_target_qos_rule = openstack_qos_R_rule(qos_rule_list,id);
		if(S_target_qos_rule)
		{
			if((S_target_qos_rule ->pre) &&(S_target_qos_rule ->next))
			{
				S_target_qos_rule ->pre ->next =S_target_qos_rule ->next;
				S_target_qos_rule ->next ->pre =S_target_qos_rule ->pre;
			}
			else if((S_target_qos_rule ->pre ==NULL) &&(S_target_qos_rule ->next))
			{
				*qos_rule_list =S_target_qos_rule ->next;
				S_target_qos_rule ->next ->pre =NULL;
			}
			else if((S_target_qos_rule ->pre) &&(S_target_qos_rule ->next ==NULL))
			{
				S_target_qos_rule ->pre ->next =NULL;
			}
			else
			{
				*qos_rule_list =NULL;
			}
			free(S_target_qos_rule);
			return *qos_rule_list;
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

/* by:yhy
 * rule list删除
 */
void openstack_qos_D_rule_list(openstack_qos_rule_p qos_rule_list)
{
	openstack_qos_rule_p S_target_qos_rule =NULL;
	while(qos_rule_list)
	{
		S_target_qos_rule =qos_rule_list;
		qos_rule_list=qos_rule_list->next;
		free(S_target_qos_rule);
	}
}

/* by:yhy
 * 比较
 */
UINT1    openstack_qos_Compare_rule(openstack_qos_rule_p *rule_list_new,openstack_qos_rule_p *rule_list_old)
{
	openstack_qos_rule_p S_qos_rule_old_sentinel = NULL;
	openstack_qos_rule_p S_qos_rule_new_sentinel = NULL;
	openstack_qos_rule_p S_qos_rule_temp		 = NULL;
	UINT1 old_sentinel_matched = FALSE;
	UINT1 old_sentinel_finded  = FALSE;
	UINT1 new_sentinel_matched = FALSE;
	UINT1 new_sentinel_finded  = FALSE;
	
	
	//新比旧-----------------------------------------------------------------------------------------------
	S_qos_rule_old_sentinel = *rule_list_old;
	S_qos_rule_new_sentinel = *rule_list_new;
	while(S_qos_rule_new_sentinel)
	{
		new_sentinel_matched = FALSE;
		new_sentinel_finded  = FALSE;
		
		S_qos_rule_old_sentinel = *rule_list_old;
		while(S_qos_rule_old_sentinel)
		{
			if(0 == strcmp(S_qos_rule_new_sentinel->id,S_qos_rule_old_sentinel->id))
			{
				new_sentinel_finded =TRUE;
				if(S_qos_rule_new_sentinel->max_rate == S_qos_rule_old_sentinel->max_rate)
				{
					new_sentinel_matched = TRUE;
					break;
				}
				else
				{
					new_sentinel_matched = FALSE;
					break;
				}
			}
			S_qos_rule_old_sentinel =S_qos_rule_old_sentinel->next;
		}
		
		if(new_sentinel_finded)
		{//找到
			if(new_sentinel_matched)
			{//匹配一致
				//TBD 不处理
			}
			else
			{//匹配不一致
				//TBD,发现改动过的binding,不处理,理论上经过前一次的比较修改后不会有不一致的
			}
		}
		else
		{//未找到
			//TBD,发现新增的binding
			openstack_qos_rule_added_event(rule_list_old,S_qos_rule_new_sentinel);
		}
		
		S_qos_rule_new_sentinel =S_qos_rule_new_sentinel->next;
	}
	
	//旧比新-------------------------------------------------------------------------------------------------
	S_qos_rule_old_sentinel = *rule_list_old;
	S_qos_rule_new_sentinel = *rule_list_new;
	while(S_qos_rule_old_sentinel)
	{
		old_sentinel_matched = FALSE;
		old_sentinel_finded  = FALSE;
		
		S_qos_rule_new_sentinel = *rule_list_new;
		while(S_qos_rule_new_sentinel)
		{
			if(0 == strcmp(S_qos_rule_old_sentinel->id,S_qos_rule_new_sentinel->id))
			{
				old_sentinel_finded =TRUE;
				if(S_qos_rule_old_sentinel->max_rate == S_qos_rule_new_sentinel->max_rate)
				{
					old_sentinel_matched = TRUE;
					break;
				}
				else
				{
					old_sentinel_matched = FALSE;
					break;
				}
			}
			S_qos_rule_new_sentinel =S_qos_rule_new_sentinel->next;
		}
		
		S_qos_rule_temp = S_qos_rule_old_sentinel->next;
		if(old_sentinel_finded)
		{//找到
			if(old_sentinel_matched)
			{//匹配一致
				//TBD
			}
			else
			{//匹配不一致
				//TBD,发现改动过的binding
				openstack_qos_rule_changed_event(rule_list_old,S_qos_rule_new_sentinel);
			}
		}
		else
		{//未找到
			//TBD,发现删除的binding
			openstack_qos_rule_removed_event(rule_list_old,S_qos_rule_old_sentinel);
		}
		
		S_qos_rule_old_sentinel = S_qos_rule_temp;
	}
	return 1;
}

/* by:yhy useful
 *
 */
UINT1 openstack_qos_rule_added_event(openstack_qos_rule_p *rule_list_old,openstack_qos_rule_p rule_added)
{
	//qos规则增加,则在控制器G_openstack_qos_rule_List内增加规则
	if(rule_added)
	{
		openstack_qos_C_rule(rule_list_old,rule_added-> id,rule_added->name,rule_added->description,rule_added->tenant_id,rule_added->protocol,rule_added->type,rule_added->shared,rule_added->max_rate);
	}
	else
	{
	}
	return 1;
}

/* by:yhy useful
 *
 */
UINT1 openstack_qos_rule_changed_event(openstack_qos_rule_p *rule_list_old,openstack_qos_rule_p rule_changed)
{
	//qos规则改变,则在控制器G_openstack_qos_instance_List查找受影响的实例,修改meter值,queue值
	if(rule_changed)
	{
		openstack_qos_U_rule(rule_list_old,rule_changed->id,rule_changed->name,rule_changed->description,rule_changed->tenant_id,rule_changed->protocol,rule_changed->type,rule_changed->shared,rule_changed->max_rate);
		if (g_is_cluster_on && g_controller_role == OFPCR_ROLE_MASTER)
		{
			//1.查找关联的路由器
			openstack_qos_binding_p S_current_qos_binding = G_openstack_qos_router_binding_List;
			while(S_current_qos_binding)
			{
				if(0 == strcmp(S_current_qos_binding->qos_id,rule_changed->id))
				{
					openstack_qos_instance_p S_target_qos_instance =openstack_qos_R_instance(&G_openstack_qos_instance_List,S_current_qos_binding->id);
					if(S_target_qos_instance)
					{
						gn_switch_t * S_TargetSwitch =find_sw_by_dpid(S_target_qos_instance->switch_dpid);
						if(S_TargetSwitch)
						{
							gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
							meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
							
							S_Meter.meter_id		=S_target_qos_instance->meter_id;
							S_Meter.flags			=OFPMF_BURST;
							S_Meter.type			=OFPMBT_DROP;
							S_Meter.rate			=(rule_changed->max_rate)/1000;
							S_Meter.burst_size		=(rule_changed->max_rate)/1000;
							S_MeterCommand.command	=OFPMC_MODIFY;
							// 对sw下删除meter命令
							S_TargetSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetSwitch, (UINT1 *)&S_MeterCommand);
						}
					}
				}
				S_current_qos_binding =S_current_qos_binding->next;
			}
			//2.查找关联的浮动ip
			S_current_qos_binding=G_openstack_qos_floating_binding_List;
			while(S_current_qos_binding)
			{
				if(0 == strcmp(S_current_qos_binding->qos_id,rule_changed->id))
				{
					openstack_qos_instance_p S_target_qos_instance =openstack_qos_R_instance(&G_openstack_qos_instance_List,S_current_qos_binding->id);
					if(S_target_qos_instance)
					{
						gn_switch_t * S_TargetSwitch =find_sw_by_dpid(S_target_qos_instance->switch_dpid);
						if(S_TargetSwitch)
						{
							add_qos_rule(rule_changed->id, 0, S_TargetSwitch,S_current_qos_binding->ip, rule_changed->max_rate, rule_changed->max_rate, rule_changed->max_rate, 1);
						}
					}
				}
				S_current_qos_binding =S_current_qos_binding->next;
			}
			//3.查找关联的负载均衡
			S_current_qos_binding = G_openstack_qos_loadbalancer_binding_List;
			while(S_current_qos_binding)
			{
				if(0 == strcmp(S_current_qos_binding->qos_id,rule_changed->id))
				{
					openstack_qos_instance_p S_target_qos_instance =openstack_qos_R_instance(&G_openstack_qos_instance_List,S_current_qos_binding->id);
					if(S_target_qos_instance)
					{
						gn_switch_t * S_TargetSwitch =find_sw_by_dpid(S_target_qos_instance->switch_dpid);
						if(S_TargetSwitch)
						{
							gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
							meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
							
							S_Meter.meter_id		=S_target_qos_instance->meter_id;
							S_Meter.flags			=OFPMF_BURST;
							S_Meter.type			=OFPMBT_DROP;
							S_Meter.rate			=(rule_changed->max_rate)/1000;
							S_Meter.burst_size		=(rule_changed->max_rate)/1000;
							S_MeterCommand.command	=OFPMC_MODIFY;
							// 对sw下删除meter命令
							S_TargetSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetSwitch, (UINT1 *)&S_MeterCommand);
						}
					}
				}
				S_current_qos_binding =S_current_qos_binding->next;
			}
		}
	}
	else
	{
	}
	return 1;
}

/* by:yhy useful
 *
 */
UINT1 openstack_qos_rule_removed_event(openstack_qos_rule_p *rule_list_old,openstack_qos_rule_p rule_removed)
{
	//qos规则删除,则在控制器G_openstack_qos_rule_List内删除规则
	if(rule_removed)
	{
		openstack_qos_D_rule(rule_list_old,rule_removed->id);
	}
	else
	{
	}
	return 1;
}

/*QOS binding相关----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* by:yhy
 * 增
 */
openstack_qos_binding_p    openstack_qos_C_binding(openstack_qos_binding_p * qos_binding_list ,char * id,char * qos_id,char * tenant_id,char * device_owner,char * subnet_id,char * router_id,UINT4 ip)
{
	openstack_qos_binding_p H_new_qos_binding =(openstack_qos_binding_p)malloc(sizeof(openstack_qos_binding));
	
	if(H_new_qos_binding)
	{
		memset(H_new_qos_binding, 0, sizeof(openstack_qos_binding));
		strcpy(H_new_qos_binding->qos_id,qos_id);
		strcpy(H_new_qos_binding->id,id);
		strcpy(H_new_qos_binding->tenant_id,tenant_id);
		strcpy(H_new_qos_binding->device_owner,device_owner);
		strcpy(H_new_qos_binding->subnet_id,subnet_id);
		strcpy(H_new_qos_binding->router_id,router_id);
		H_new_qos_binding->ip=ip;
		H_new_qos_binding->pre=NULL;
		H_new_qos_binding->next=NULL;
		
		if((*qos_binding_list)->next)
		{
			(*qos_binding_list)->next->pre =H_new_qos_binding;
			H_new_qos_binding->next =(*qos_binding_list)->next;		
			H_new_qos_binding->pre =(*qos_binding_list);
			(*qos_binding_list)->next =H_new_qos_binding;
		}
		else
		{
			(*qos_binding_list)->next =H_new_qos_binding;
			H_new_qos_binding ->pre =(*qos_binding_list);
		}
		return( *qos_binding_list);
	}
	else
	{//无法获取空间
		LOG_PROC("ERROR", "Cant get memory :%s_%d",FN,LN);
		return NULL;
	}
}

/* by:yhy
 * 查
 */
openstack_qos_binding_p    openstack_qos_R_binding(openstack_qos_binding_p * qos_binding_list,char * id)
{
	if(id)
	{
		openstack_qos_binding_p S_current_qos_binding = * qos_binding_list;
		while(S_current_qos_binding)
		{
			if(0 == strcmp(S_current_qos_binding->id,id))
			{
				return S_current_qos_binding;
			}
			S_current_qos_binding =S_current_qos_binding->next;
		}
		return NULL;
	}
	else
	{
		return NULL;
	}
}

/* by:yhy
 * 改
 */
openstack_qos_binding_p    openstack_qos_U_binding(openstack_qos_binding_p * qos_binding_list,char * id,char * qos_id,char * tenant_id,char * device_owner,char * subnet_id,char * router_id,UINT4 ip)
{
	if(id)
	{
		openstack_qos_binding_p S_target_qos_binding =openstack_qos_R_binding(qos_binding_list,id);
		if(S_target_qos_binding)
		{
			strcpy(S_target_qos_binding->qos_id,qos_id);
			strcpy(S_target_qos_binding->tenant_id,tenant_id);
			strcpy(S_target_qos_binding->device_owner,device_owner);
			strcpy(S_target_qos_binding->subnet_id,subnet_id);
			strcpy(S_target_qos_binding->router_id,router_id);
			S_target_qos_binding->ip=ip;
			return S_target_qos_binding;
		}
		else
		{
			return openstack_qos_C_binding(qos_binding_list,id,qos_id,tenant_id,device_owner,subnet_id,router_id,ip);
		}
	}
	else
	{
		return NULL;
	}
}

/* by:yhy
 * 删
 */
openstack_qos_binding_p    openstack_qos_D_binding(openstack_qos_binding_p *qos_binding_list,char * id)
{
	if(id )
	{
		openstack_qos_binding_p S_target_qos_binding =openstack_qos_R_binding(qos_binding_list,id);
		if(S_target_qos_binding)
		{
			if((S_target_qos_binding ->pre) &&(S_target_qos_binding ->next))
			{
				S_target_qos_binding ->pre ->next =S_target_qos_binding ->next;
				S_target_qos_binding ->next ->pre =S_target_qos_binding ->pre;
			}
			else if((S_target_qos_binding ->pre ==NULL) &&(S_target_qos_binding ->next))
			{
				*qos_binding_list =S_target_qos_binding ->next;
				S_target_qos_binding ->next ->pre =NULL;
			}
			else if((S_target_qos_binding ->pre) &&(S_target_qos_binding ->next ==NULL))
			{
				S_target_qos_binding ->pre ->next =NULL;
			}
			else
			{
				*qos_binding_list =NULL;
			}
			free(S_target_qos_binding);
			return *qos_binding_list;
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

/* by:yhy
 * 删除binding list
 */
void openstack_qos_D_binding_list(openstack_qos_binding_p qos_binding_list)
{
	openstack_qos_binding_p S_target_qos_binding =NULL;
	while(qos_binding_list)
	{
		S_target_qos_binding =qos_binding_list;
		qos_binding_list=qos_binding_list->next;
		free(S_target_qos_binding);
	}
}

/* by:yhy
 * 比较
 */
openstack_qos_binding_p    openstack_qos_Compare_binding(openstack_qos_binding_p *qos_binding_list_new,openstack_qos_binding_p *qos_binding_list_old)
{
	openstack_qos_binding_p S_qos_binding_old_sentinel = NULL;
	openstack_qos_binding_p S_qos_binding_new_sentinel = NULL;
	openstack_qos_binding_p S_qos_binding_temp = NULL;
	UINT1 old_sentinel_matched = FALSE;
	UINT1 old_sentinel_finded  = FALSE;
	UINT1 new_sentinel_matched = FALSE;
	UINT1 new_sentinel_finded  = FALSE;
	
	//LOG_PROC("INFO", "------------------------------------------------%s_%d",FN,LN);
	//LOG_PROC("INFO", "%p,%p",qos_binding_list_new,qos_binding_list_old);
	//LOG_PROC("INFO", "%p,%p",*qos_binding_list_new,*qos_binding_list_old);
	
	S_qos_binding_old_sentinel =*qos_binding_list_old;
	S_qos_binding_new_sentinel =*qos_binding_list_new;
	while(S_qos_binding_new_sentinel)
	{//遍历新的,寻找新增的binding或者改动过的binding
		new_sentinel_matched = FALSE;
		new_sentinel_finded  = FALSE;
		//LOG_PROC("INFO", "------------------------------------------------%s_%d",FN,LN);
		S_qos_binding_old_sentinel =*qos_binding_list_old;
		while(S_qos_binding_old_sentinel)
		{
			if(0 == strcmp(S_qos_binding_new_sentinel->id,S_qos_binding_old_sentinel->id))
			{
				new_sentinel_finded =TRUE;
				if(0 == strcmp(S_qos_binding_new_sentinel->qos_id,S_qos_binding_old_sentinel->qos_id))
				{
					new_sentinel_matched = TRUE;
					break;
				}
				else
				{
					new_sentinel_matched = FALSE;
					break;
				}
			}
			S_qos_binding_old_sentinel = S_qos_binding_old_sentinel->next;
		}
		
		if(new_sentinel_finded)
		{//找到
			if(new_sentinel_matched)
			{//匹配一致
				//TBD
			}
			else
			{//匹配不一致
				//TBD,发现改动过的binding
				//LOG_PROC("INFO", "------------------------------------------------%s_%d",FN,LN);
				openstack_qos_binding_changed_event(qos_binding_list_old,S_qos_binding_old_sentinel,S_qos_binding_new_sentinel);
			}
		}
		else
		{//未找到
			//TBD,发现新增的binding
			//LOG_PROC("INFO", "------------------------------------------------%s_%d",FN,LN);
			openstack_qos_binding_added_event(qos_binding_list_old,S_qos_binding_new_sentinel);
		}
		S_qos_binding_new_sentinel = S_qos_binding_new_sentinel->next;						
	}
	
	
	S_qos_binding_old_sentinel =*qos_binding_list_old;
	S_qos_binding_new_sentinel =*qos_binding_list_new;
	//LOG_PROC("INFO", "------------------------------------------------%s_%d",FN,LN);
	while(S_qos_binding_old_sentinel)
	{//遍历旧的,寻找删除的binding或者改动过的binding
		old_sentinel_matched = FALSE;
		old_sentinel_finded  = FALSE;
		
		S_qos_binding_new_sentinel =*qos_binding_list_new;
		while(S_qos_binding_new_sentinel)
		{
			if(0 == strcmp(S_qos_binding_old_sentinel->id,S_qos_binding_new_sentinel->id))
			{
				old_sentinel_finded =TRUE;
				if(0 == strcmp(S_qos_binding_old_sentinel->qos_id,S_qos_binding_new_sentinel->qos_id))
				{
					old_sentinel_matched = TRUE;
					break;
				}
				else
				{
					old_sentinel_matched = FALSE;
					break;
				}
			}
			S_qos_binding_new_sentinel =S_qos_binding_new_sentinel->next;
		}
		S_qos_binding_temp = S_qos_binding_old_sentinel->next;
		if(old_sentinel_finded)
		{//找到
			if(old_sentinel_matched)
			{//匹配一致
				//TBD
			}
			else
			{//匹配不一致
				//TBD,发现改动过的binding
				//LOG_PROC("INFO", "------------------------------------------------%s_%d",FN,LN);
				openstack_qos_binding_changed_event(qos_binding_list_old,S_qos_binding_old_sentinel,S_qos_binding_new_sentinel);
			}
		}
		else
		{//未找到
			//TBD,发现删除的binding
			//LOG_PROC("INFO", "------------------------------------------------%s_%d",FN,LN);
			openstack_qos_binding_removed_event(qos_binding_list_old,S_qos_binding_old_sentinel);
		}
		
		S_qos_binding_old_sentinel =S_qos_binding_temp;
	}
	return NULL;
}

/* by:yhy useful
 *
 */
UINT1 openstack_qos_binding_added_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_added)
{
	if(binding_added && binding_added->id && binding_added->qos_id && binding_added->tenant_id &&binding_added->device_owner)
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		openstack_qos_C_binding(binding_list_old,binding_added->id,binding_added->qos_id,binding_added->tenant_id,binding_added->device_owner,binding_added->subnet_id,binding_added->router_id,binding_added->ip);
		
		UINT1 S_binding_added_qos_notNULL = strcmp((binding_added->qos_id),"");
		if(S_binding_added_qos_notNULL !=0)
		{
			openstack_qos_binding_WithqosRule_added_event(binding_list_old,binding_added);
		}
		else 
		{
			openstack_qos_binding_WithoutqosRule_added_event(binding_list_old,binding_added);
		}
		
	}
	return 1;
}

/* by:yhy useful
 *
 */
UINT1 openstack_qos_binding_changed_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_old,openstack_qos_binding_p binding_new)
{
	if(binding_old && binding_new && binding_old->id && binding_new->qos_id)
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		UINT1 S_binding_old_qos_notNULL = strcmp((binding_old->qos_id),"");
		UINT1 S_binding_new_qos_notNULL = strcmp((binding_new->qos_id),"");
		
		openstack_qos_U_binding(binding_list_old,binding_old->id,binding_new->qos_id,binding_new->tenant_id,binding_new->device_owner,binding_new->subnet_id,binding_new->router_id,binding_new->ip);
		
		if((S_binding_new_qos_notNULL)&&(S_binding_old_qos_notNULL == 0))
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			openstack_qos_binding_qosRule_added_event(binding_list_old,binding_old,binding_new);
		}
		else if((S_binding_new_qos_notNULL)&&(S_binding_old_qos_notNULL))
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			openstack_qos_binding_qosRule_changed_event(binding_list_old,binding_old,binding_new);
		}
		else if((S_binding_new_qos_notNULL == 0)&&(S_binding_old_qos_notNULL))
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			openstack_qos_binding_qosRule_removeded_event(binding_list_old,binding_old,binding_new);
		}
		else
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			//do nothing
		}
		
	}
	return 1;
}

/* by:yhy useful
 *
 */
UINT1 openstack_qos_binding_removed_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_removed)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	if(binding_list_old && binding_removed && binding_removed->id)
	{
	
		UINT1 S_binding_removed_qos_notNULL = strcmp((binding_removed->qos_id),"");
		if(S_binding_removed_qos_notNULL)
		{
			openstack_qos_binding_PortWithqosRule_removeded_event(binding_list_old,binding_removed);
		}
		else
		{
			openstack_qos_binding_PortWithoutqosRule_removeded_event(binding_list_old,binding_removed);
		}
	
		openstack_qos_D_binding(binding_list_old,binding_removed->id);
	}
	return 1;
}

//add event
UINT1 openstack_qos_binding_WithqosRule_added_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_added)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	if(0 == strcmp((binding_added->device_owner),TYPE_Router))
	{
		gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
		meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
		//根据ip找sw
		external_port_p S_TargetExternalPort =find_openstack_external_by_outer_ip(binding_added->ip);
		if(S_TargetExternalPort)
		{
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//查找rule list 获取rate
				openstack_qos_rule_p  S_TargetQosRule = openstack_qos_R_rule(&G_openstack_qos_rule_List,binding_added->qos_id);
				if(S_TargetQosRule)
				{
					//自增sw的meterid
					S_TargetExternalSwitch->Meter_ID ++;
					//构建instance
					openstack_qos_instance_p S_TargetQosInstance = openstack_qos_C_instance(&G_openstack_qos_instance_List,binding_added->id,binding_added->qos_id,QOS_OPERATION_METER,S_TargetExternalPort->external_dpid,S_TargetExternalSwitch->Meter_ID,4);
					if(S_TargetQosInstance)
					{
						//构建meter
						S_Meter.meter_id		=S_TargetQosInstance->meter_id;
						S_Meter.flags			=OFPMF_BURST;
						S_Meter.type			=OFPMBT_DROP;
						S_Meter.rate			=(S_TargetQosRule->max_rate)/1000;
						S_Meter.burst_size		=(S_TargetQosRule->max_rate)/1000;
						S_MeterCommand.command	=OFPMC_ADD;
						//对sw下meter
						S_TargetExternalSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetExternalSwitch, (UINT1 *)&S_MeterCommand);
						//对sw下flow
						install_add_QOS_jump_with_meter_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_added->ip,S_TargetQosInstance->meter_id);

						//对关联浮动IP下流表
						openstack_qos_binding_p S_current_qos_binding=G_openstack_qos_floating_binding_List;
						while(S_current_qos_binding)
						{
							//LOG_PROC("INFO", "%s_%d",FN,LN);
							if(0 == strcmp(S_current_qos_binding->router_id,binding_added->router_id))
							{
								install_add_QOS_jump_with_meter_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,S_current_qos_binding->ip,S_TargetQosInstance->meter_id);
							}
							S_current_qos_binding =S_current_qos_binding->next;
						}
					}
					else
					{
						LOG_PROC("WARNING", "%s_%d:%s",FN,LN,"can't create QOS instance menory");
					}
				}
				else
				{
					LOG_PROC("WARNING", "%s_%d;%s",FN,LN,"can't find QOS rule by QOS ID");
				}
			}
		}
	}
	else if(0 == strcmp((binding_added->device_owner),TYPE_FloatingIP))
	{
		//根据浮动ip找浮动ip结构体
		external_floating_ip_p S_TargetFloatingIPStruct =get_external_floating_ip_by_floating_ip(binding_added->ip);
		//根据浮动ip结构体中固定IP找固定iP对应host node
		if(S_TargetFloatingIPStruct)
		{
			p_fabric_host_node S_TargetFloatingIPPort =get_fabric_host_from_list_by_ip(S_TargetFloatingIPStruct->fixed_ip);
			if(S_TargetFloatingIPPort)
			{
				gn_switch_t * S_TargetFloatingIPPortSwitch	 =	S_TargetFloatingIPPort->sw;
				if(S_TargetFloatingIPPortSwitch)
				{
					//查找rule list 获取rate
					openstack_qos_rule_p  S_TargetQosRule = openstack_qos_R_rule(&G_openstack_qos_rule_List,binding_added->qos_id);
					if(S_TargetQosRule)
					{
						//自增sw的meterid
						S_TargetFloatingIPPortSwitch->Meter_ID ++;
						//构建instance
						openstack_qos_instance_p S_TargetQosInstance = openstack_qos_C_instance(&G_openstack_qos_instance_List,binding_added->id,binding_added->qos_id,QOS_OPERATION_QUEUE,S_TargetFloatingIPPortSwitch->dpid,S_TargetFloatingIPPortSwitch->Meter_ID,4);
						
						//设置交换机qos类型
						S_TargetFloatingIPPortSwitch->qos_type= QOS_TYPE_QUEUE;
						//添加qos rule下发queue
						add_qos_rule(S_TargetQosRule->id, 0, S_TargetFloatingIPPortSwitch,binding_added->ip, S_TargetQosRule->max_rate, S_TargetQosRule->max_rate, S_TargetQosRule->max_rate, 1);
						//根据qos的目标ip查找策略,从中提取queue id
						qos_policy_p policy = find_qos_policy_by_sw_dstip(S_TargetFloatingIPPortSwitch, binding_added->ip);
						
						if(policy)
						{
							gn_queue_t* queue =(gn_queue_t*)(policy->qos_service);
							//下发绑定queue id的流表
							if(NULL != queue)
							{
								install_add_QOS_jump_with_queue_flow(S_TargetFloatingIPPortSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_SWAPTAG_TABLE,binding_added->ip,queue->queue_id);
							}
						}
						else
						{
							LOG_PROC("WARNING", "%s_%d:%s",FN,LN,"can't find QOS policy");
						}
					}
					else
					{
						LOG_PROC("WARNING", "%s_%d;%s",FN,LN,"can't find QOS rule by QOS ID");
					}
					
				}
			}
			//查找浮动ip的出口交换机,在出口交换机上增加流表
			external_port_p S_TargetFloatingExternalPort =find_openstack_external_by_floating_ip(binding_added->ip);
			if(S_TargetFloatingExternalPort)
			{
				gn_switch_t * S_TargetFloatingExternalSwitch =find_sw_by_dpid(S_TargetFloatingExternalPort->external_dpid);
				if(S_TargetFloatingExternalSwitch)
				{
					openstack_qos_binding_p S_current_qos_binding = G_openstack_qos_router_binding_List;
					while(S_current_qos_binding)
					{
						if(0 == strcmp((S_current_qos_binding->router_id),(binding_added->router_id)))
						{
							openstack_qos_instance_p S_target_qos_instance =openstack_qos_R_instance(&G_openstack_qos_instance_List,S_current_qos_binding->id);
							{
								if(S_target_qos_instance)
								{
									install_add_QOS_jump_with_meter_flow(S_TargetFloatingExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_added->ip,S_target_qos_instance->meter_id);
								}
								else
								{
									install_add_QOS_jump_without_meter_flow(S_TargetFloatingExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_added->ip);
								}
							}
						}
						S_current_qos_binding =S_current_qos_binding->next;
					}
				}
			}	
		}
	}
	else if(0 == strcmp((binding_added->device_owner),TYPE_Loadbalance))
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
		meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
		//根据ip找sw
		external_port_p S_TargetExternalPort =find_openstack_external_by_network_id(binding_added->subnet_id);
		if(S_TargetExternalPort)
		{
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//查找rule list 获取rate
				openstack_qos_rule_p  S_TargetQosRule = openstack_qos_R_rule(&G_openstack_qos_rule_List,binding_added->qos_id);
				if(S_TargetQosRule)
				{
					//自增sw的meterid
					S_TargetExternalSwitch->Meter_ID ++;
					//构建instance
					openstack_qos_instance_p S_TargetQosInstance = openstack_qos_C_instance(&G_openstack_qos_instance_List,binding_added->id,binding_added->qos_id,QOS_OPERATION_METER,S_TargetExternalPort->external_dpid,S_TargetExternalSwitch->Meter_ID,4);
				
					if(S_TargetQosInstance)
					{
						//构建meter
						S_Meter.meter_id		=S_TargetQosInstance->meter_id;
						S_Meter.flags			=OFPMF_BURST;
						S_Meter.type			=OFPMBT_DROP;
						S_Meter.rate			=(S_TargetQosRule->max_rate)/1000;
						S_Meter.burst_size		=(S_TargetQosRule->max_rate)/1000;
						S_MeterCommand.command	=OFPMC_ADD;
						//对sw下meter
						S_TargetExternalSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetExternalSwitch, (UINT1 *)&S_MeterCommand);
						//对sw下flow
						install_add_QOS_jump_with_meter_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_added->ip,S_TargetQosInstance->meter_id);
					}
					else
					{
						LOG_PROC("WARNING", "%s_%d:%s",FN,LN,"can't create QOS instance memory");
					}
				}
				else
				{
					LOG_PROC("WARNING", "%s_%d;%s",FN,LN,"can't find QOS rule by QOS ID");
				}
			}
		}
	}
	return 1;
}
//add event
UINT1 openstack_qos_binding_WithoutqosRule_added_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_added)
{
	//LOG_PROC("QOS", "%s_%d",FN,LN);
	if(0 == strcmp((binding_added->device_owner),TYPE_Router))
	{
		//根据ip找sw
		//LOG_PROC("QOS", "%s_%d_%d",FN,LN,binding_added->ip);
		external_port_p S_TargetExternalPort =find_openstack_external_by_outer_ip(binding_added->ip);
		if(S_TargetExternalPort)
		{
			//LOG_PROC("QOS", "%s_%d_%ld",FN,LN,S_TargetExternalPort->external_dpid);
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//LOG_PROC("QOS", "%s_%d",FN,LN);
				//对sw下flow
				install_add_QOS_jump_without_meter_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_added->ip);
			}
		}
	}
	else if(0 == strcmp((binding_added->device_owner),TYPE_FloatingIP))
	{
		//根据浮动ip找浮动ip结构体
		external_floating_ip_p S_TargetFloatingIPStruct =get_external_floating_ip_by_floating_ip(binding_added->ip);
		if(S_TargetFloatingIPStruct)
		{
			//根据浮动ip结构体中固定IP找固定iP对应host node
			p_fabric_host_node S_TargetFloatingIPPort =get_fabric_host_from_list_by_ip(S_TargetFloatingIPStruct->fixed_ip);
			if(S_TargetFloatingIPPort)
			{
				//根据找到的host node提取对应的交换机OVS
				gn_switch_t * S_TargetFloatingIPPortSwitch	 =	S_TargetFloatingIPPort->sw;
				if(S_TargetFloatingIPPortSwitch)
				{
					//下发无queue的跳转流表
					install_add_QOS_jump_without_queue_flow(S_TargetFloatingIPPortSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_SWAPTAG_TABLE,binding_added->ip);
				}
				
				external_port_p S_TargetFloatingExternalPort =find_openstack_external_by_floating_ip(binding_added->ip);
				if(S_TargetFloatingExternalPort)
				{
					gn_switch_t * S_TargetFloatingExternalSwitch =find_sw_by_dpid(S_TargetFloatingExternalPort->external_dpid);
					if(S_TargetFloatingExternalSwitch)
					{
						openstack_qos_binding_p S_current_qos_binding = G_openstack_qos_router_binding_List;
						while(S_current_qos_binding)
						{
							if(0 == strcmp((S_current_qos_binding->router_id),(binding_added->router_id)))
							{
								openstack_qos_instance_p S_target_qos_instance =openstack_qos_R_instance(&G_openstack_qos_instance_List,S_current_qos_binding->id);
								{
									if(S_target_qos_instance)
									{
										install_add_QOS_jump_with_meter_flow(S_TargetFloatingExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_added->ip,S_target_qos_instance->meter_id);
									}
									else
									{
										install_add_QOS_jump_without_meter_flow(S_TargetFloatingExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_added->ip);
									}
								}
							}
							S_current_qos_binding =S_current_qos_binding->next;
						}
					}
				}		
			}
		}
	}
	else if(0 == strcmp((binding_added->device_owner),TYPE_Loadbalance))
	{
		external_port_p S_TargetExternalPort =find_openstack_external_by_network_id(binding_added->subnet_id);
		if(S_TargetExternalPort)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				//对sw下flow
				install_add_QOS_jump_without_meter_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_added->ip);
			}
		}
	}
	return 1;
}

//change event
UINT1 openstack_qos_binding_qosRule_added_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_old,openstack_qos_binding_p binding_new)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	if(0 == strcmp((binding_old->device_owner),TYPE_Router))
	{
		openstack_qos_binding_WithqosRule_added_event(binding_list_old,binding_new);
	}
	else if(0 == strcmp((binding_old->device_owner),TYPE_FloatingIP))
	{
		openstack_qos_binding_WithqosRule_added_event(binding_list_old,binding_new);
	}
	else if(0 == strcmp((binding_old->device_owner),TYPE_Loadbalance))
	{
		openstack_qos_binding_WithqosRule_added_event(binding_list_old,binding_new);
	}
	return 1;
}
//change event
UINT1 openstack_qos_binding_qosRule_changed_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_old,openstack_qos_binding_p binding_new)
{
	if(0 == strcmp((binding_old->device_owner),TYPE_Router))
	{
		gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
		meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
		//根据路由器gateway ip找到external port
		external_port_p S_TargetExternalPort =find_openstack_external_by_outer_ip(binding_old->ip);
		if(S_TargetExternalPort)
		{
			//根据external port的dpid找物理交换机句柄
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//查询instance
				openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_old->id);
				if(S_TargetQosInstance)
				{
					//查找rule list 获取rate
					openstack_qos_rule_p  S_TargetQosRule = openstack_qos_R_rule(&G_openstack_qos_rule_List,S_TargetQosInstance->qos_rule_id);
					if(S_TargetQosRule)
					{
						//更新instance
						openstack_qos_U_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id,binding_new->qos_id,QOS_OPERATION_METER,S_TargetQosInstance->switch_dpid,S_TargetQosInstance->meter_id,S_TargetQosInstance->table_id);
					
						//构建meter
						S_Meter.meter_id		=S_TargetQosInstance->meter_id;
						S_Meter.flags			=OFPMF_BURST;
						S_Meter.type			=OFPMBT_DROP;
						S_Meter.rate			=(S_TargetQosRule->max_rate)/1000;
						S_Meter.burst_size		=(S_TargetQosRule->max_rate)/1000;
						S_MeterCommand.command	=OFPMC_MODIFY;
						// 对sw下删除meter命令
						S_TargetExternalSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetExternalSwitch, (UINT1 *)&S_MeterCommand);
					}
					else
					{
						LOG_PROC("WARNING", "%s_%d;%s",FN,LN,"can't find QOS rule by QOS ID");
					}	
				}
			}
		}
	}
	else if(0 == strcmp((binding_old->device_owner),TYPE_FloatingIP))
	{
		external_floating_ip_p S_TargetFloatingIPStruct =get_external_floating_ip_by_floating_ip(binding_old->ip);
		//根据浮动ip结构体中固定IP找固定iP对应host node
		if(S_TargetFloatingIPStruct)
		{
			p_fabric_host_node S_TargetFloatingIPPort =get_fabric_host_from_list_by_ip(S_TargetFloatingIPStruct->fixed_ip);
			if(S_TargetFloatingIPPort)
			{
				gn_switch_t * S_TargetFloatingIPPortSwitch	 =	S_TargetFloatingIPPort->sw;
				if(S_TargetFloatingIPPortSwitch)
				{
					//构建instance
					openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_old->id);
					if(S_TargetQosInstance)
					{
						//查找rule list 获取rate
						openstack_qos_rule_p  S_TargetQosRule = openstack_qos_R_rule(&G_openstack_qos_rule_List,S_TargetQosInstance->qos_rule_id);	
						if(S_TargetQosRule)
						{
							//更新instance
							openstack_qos_U_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id,binding_new->qos_id,QOS_OPERATION_METER,S_TargetQosInstance->switch_dpid,S_TargetQosInstance->meter_id,S_TargetQosInstance->table_id);
						
							//更新queue
							add_qos_rule(S_TargetQosRule->id, 0, S_TargetFloatingIPPortSwitch,binding_new->ip, S_TargetQosRule->max_rate, S_TargetQosRule->max_rate, S_TargetQosRule->max_rate, 1);
						}
						else
						{
							LOG_PROC("WARNING", "%s_%d;%s",FN,LN,"can't find QOS rule by QOS ID");
						}
					}
				}
			}
		}
	}
	else if(0 == strcmp((binding_old->device_owner),TYPE_Loadbalance))
	{
		gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
		meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
		//根据路由器gateway ip找到external port
		external_port_p S_TargetExternalPort =find_openstack_external_by_network_id(binding_old->subnet_id);
		if(S_TargetExternalPort)
		{
			//根据external port的dpid找物理交换机句柄
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//查询instance
				openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_old->id);
				if(S_TargetQosInstance)
				{
					//查找rule list 获取rate
					openstack_qos_rule_p  S_TargetQosRule = openstack_qos_R_rule(&G_openstack_qos_rule_List,S_TargetQosInstance->qos_rule_id);
					
					if(S_TargetQosRule)
					{
						//更新instance
						openstack_qos_U_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id,binding_new->qos_id,QOS_OPERATION_METER,S_TargetQosInstance->switch_dpid,S_TargetQosInstance->meter_id,S_TargetQosInstance->table_id);
					
						//构建meter
						S_Meter.meter_id		=S_TargetQosInstance->meter_id;
						S_Meter.flags			=OFPMF_BURST;
						S_Meter.type			=OFPMBT_DROP;
						S_Meter.rate			=(S_TargetQosRule->max_rate)/1000;
						S_Meter.burst_size		=(S_TargetQosRule->max_rate)/1000;
						S_MeterCommand.command	=OFPMC_MODIFY;
						// 对sw下删除meter命令
						S_TargetExternalSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetExternalSwitch, (UINT1 *)&S_MeterCommand);
					}
					else
					{
						LOG_PROC("WARNING", "%s_%d;%s",FN,LN,"can't find QOS rule by QOS ID");
					}
				}
			}
		}
	}
	return 1;
}
//change event
UINT1 openstack_qos_binding_qosRule_removeded_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_old,openstack_qos_binding_p binding_new)
{
	//LOG_PROC("INFO", "%s_%d",FN,LN);
	if(0 == strcmp((binding_old->device_owner),TYPE_Router))
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
		meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
		//根据路由器gateway ip找到external port
		external_port_p S_TargetExternalPort =find_openstack_external_by_outer_ip(binding_old->ip);
		if(S_TargetExternalPort)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			//根据external port的dpid找物理交换机句柄
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				//重下gateway ip流表
				install_add_QOS_jump_without_meter_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_old->ip);
			
				// 查询instance
				openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_old->id);
				if(S_TargetQosInstance)
				{
					// 构建meter
					S_Meter.meter_id		=S_TargetQosInstance->meter_id;
					S_Meter.type			=OFPMBT_DROP;
					S_MeterCommand.command	=OFPMC_DELETE;
					// 对sw下删除meter命令
					S_TargetExternalSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetExternalSwitch, (UINT1 *)&S_MeterCommand);
					// 移除项
					openstack_qos_D_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id);
				}
				
				//重下浮动 ip流表
				openstack_qos_binding_p S_current_qos_binding=G_openstack_qos_floating_binding_List;
				while(S_current_qos_binding)
				{
					//LOG_PROC("INFO", "%s_%d",FN,LN);
					if(0 == strcmp(S_current_qos_binding->router_id,binding_old->router_id))
					{
						install_add_QOS_jump_without_meter_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,S_current_qos_binding->ip);
					}
					S_current_qos_binding =S_current_qos_binding->next;
				}
				
			}
		}
	}
	else if(0 == strcmp((binding_old->device_owner),TYPE_FloatingIP))
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		// 查询instance
		openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_old->id);
		external_floating_ip_p S_TargetFloatingIPStruct =get_external_floating_ip_by_floating_ip(binding_old->ip);
		//校验浮动IP
		if(S_TargetFloatingIPStruct)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			//删除浮动IP对应host所在compute节点上ovs内相关内容
			p_fabric_host_node S_TargetFloatingIPPort =get_fabric_host_from_list_by_ip(S_TargetFloatingIPStruct->fixed_ip);
			if(S_TargetFloatingIPPort)
			{
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				//根据找到的host node提取对应的交换机OVS
				gn_switch_t * S_TargetFloatingIPPortSwitch	 =	S_TargetFloatingIPPort->sw;
				if(S_TargetFloatingIPPortSwitch)
				{
					//LOG_PROC("INFO", "%s_%d",FN,LN);
					//下发无queue的跳转流表
					install_add_QOS_jump_without_queue_flow(S_TargetFloatingIPPortSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_SWAPTAG_TABLE,binding_old->ip);
					// 删queue
					delete_qos_rule(S_TargetFloatingIPPortSwitch, binding_old->ip);			
				}
			}
			
		}
		if(S_TargetQosInstance)
		{
			//删除instance
			openstack_qos_D_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id);
		}
	}
	else if(0 == strcmp((binding_old->device_owner),TYPE_Loadbalance))
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
		meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
		//根据路由器gateway ip找到external port
		external_port_p S_TargetExternalPort =find_openstack_external_by_network_id(binding_old->subnet_id);
		if(S_TargetExternalPort)
		{
			//LOG_PROC("INFO", "%s_%d",FN,LN);
			//根据external port的dpid找物理交换机句柄
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//LOG_PROC("INFO", "%s_%d",FN,LN);
				//重下gateway ip流表
				install_add_QOS_jump_without_meter_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,FABRIC_OUTPUT_TABLE,binding_old->ip);		
				// 查询instance
				openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_old->id);
				if(S_TargetQosInstance)
				{
					// 构建meter
					S_Meter.meter_id		=S_TargetQosInstance->meter_id;
					S_Meter.type			=OFPMBT_DROP;
					S_MeterCommand.command	=OFPMC_DELETE;
					// 对sw下删除meter命令
					S_TargetExternalSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetExternalSwitch, (UINT1 *)&S_MeterCommand);
					// 移除项
					openstack_qos_D_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id);
				}
			}
		}
	}
	return 1;
}

//remove event
UINT1 openstack_qos_binding_PortWithqosRule_removeded_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_removed)
{
	if(0 == strcmp((binding_removed->device_owner),TYPE_Router))
	{
		gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
		meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
		//根据路由器gateway ip找到external port
		external_port_p S_TargetExternalPort =find_openstack_external_by_outer_ip(binding_removed->ip);
		if(S_TargetExternalPort)
		{
			//根据external port的dpid找物理交换机句柄
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				// 删流表
				install_remove_QOS_jump_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,binding_removed->ip);
				// 查询instance
				openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_removed->id);
				if(S_TargetQosInstance)
				{
					// 构建meter
					S_Meter.meter_id		=S_TargetQosInstance->meter_id;
					S_Meter.type			=OFPMBT_DROP;
					S_MeterCommand.command	=OFPMC_DELETE;
					// 对sw下删除meter命令
					S_TargetExternalSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetExternalSwitch, (UINT1 *)&S_MeterCommand);
					// 移除项
					openstack_qos_D_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id);
				}
			}
		}
	}
	else if(0 == strcmp((binding_removed->device_owner),TYPE_FloatingIP))
	{
		// 查询instance
		openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_removed->id);
		
		external_floating_ip_p S_TargetFloatingIPStruct =get_external_floating_ip_by_floating_ip(binding_removed->ip);
		//校验浮动IP
		if(S_TargetFloatingIPStruct)
		{
			//删除浮动IP对应host所在compute节点上ovs内相关内容
			p_fabric_host_node S_TargetFloatingIPPort =get_fabric_host_from_list_by_ip(S_TargetFloatingIPStruct->fixed_ip);
			if(S_TargetFloatingIPPort)
			{
				//根据找到的host node提取对应的交换机OVS
				gn_switch_t * S_TargetFloatingIPPortSwitch	 =	S_TargetFloatingIPPort->sw;
				if(S_TargetFloatingIPPortSwitch)
				{
					// 删流表
					install_remove_QOS_jump_flow(S_TargetFloatingIPPortSwitch,FABRIC_QOS_OUT_TABLE,binding_removed->ip);
					// 删queue
					delete_qos_rule(S_TargetFloatingIPPortSwitch, binding_removed->ip);			
				}
			}
			
			//删除浮动IP对应出口交换机上相关内容
			external_port_p S_TargetExternalPort =find_openstack_external_by_floating_ip(binding_removed->ip);
			if(S_TargetExternalPort)
			{
				gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
				if(S_TargetExternalSwitch)
				{
					//	直接删流表
					install_remove_QOS_jump_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,binding_removed->ip);
				}
			}
		}
		if(S_TargetQosInstance)
		{
			//删除instance
			openstack_qos_D_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id);
		}
	}
	else if(0 == strcmp((binding_removed->device_owner),TYPE_Loadbalance))
	{
		gn_meter_t S_Meter ={0,0,0,0,0,0,0,0};
		meter_mod_req_info_t S_MeterCommand ={0,0,&S_Meter};
		//根据路由器gateway ip找到external port
		external_port_p S_TargetExternalPort =find_openstack_external_by_network_id(binding_removed->subnet_id);
		if(S_TargetExternalPort)
		{
			//根据external port的dpid找物理交换机句柄
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				// 删流表
				install_remove_QOS_jump_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,binding_removed->ip);
				// 查询instance
				openstack_qos_instance_p S_TargetQosInstance = openstack_qos_R_instance(&G_openstack_qos_instance_List,binding_removed->id);
				if(S_TargetQosInstance)
				{
					// 构建meter
					S_Meter.meter_id		=S_TargetQosInstance->meter_id;
					S_Meter.type			=OFPMBT_DROP;
					S_MeterCommand.command	=OFPMC_DELETE;
					// 对sw下删除meter命令
					S_TargetExternalSwitch->msg_driver.msg_handler[OFPT13_METER_MOD](S_TargetExternalSwitch, (UINT1 *)&S_MeterCommand);
					// 移除项
					openstack_qos_D_instance(&G_openstack_qos_instance_List,S_TargetQosInstance->qos_binding_id);
				}
			}
		}
	}
	return 1;
}
//remove event
UINT1 openstack_qos_binding_PortWithoutqosRule_removeded_event(openstack_qos_binding_p * binding_list_old,openstack_qos_binding_p binding_removed)
{
	if(0 == strcmp((binding_removed->device_owner),TYPE_Router))
	{
		//根据路由器gateway ip找到external port
		external_port_p S_TargetExternalPort =find_openstack_external_by_outer_ip(binding_removed->ip);
		if(S_TargetExternalPort)
		{
			//根据external port的dpid找物理交换机句柄
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//直接删流表
				install_remove_QOS_jump_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,binding_removed->ip);
			}		
		}
	}
	else if(0 == strcmp((binding_removed->device_owner),TYPE_FloatingIP))
	{
		external_floating_ip_p S_TargetFloatingIPStruct =get_external_floating_ip_by_floating_ip(binding_removed->ip);
		//校验浮动IP
		if(S_TargetFloatingIPStruct)
		{
			//删除浮动IP对应host所在compute节点上ovs内相关内容
			p_fabric_host_node S_TargetFloatingIPPort =get_fabric_host_from_list_by_ip(S_TargetFloatingIPStruct->fixed_ip);
			if(S_TargetFloatingIPPort)
			{
				//根据找到的host node提取对应的交换机OVS
				gn_switch_t * S_TargetFloatingIPPortSwitch	 =	S_TargetFloatingIPPort->sw;
				if(S_TargetFloatingIPPortSwitch)
				{
					//直接删流表
					install_remove_QOS_jump_flow(S_TargetFloatingIPPortSwitch,FABRIC_QOS_OUT_TABLE,binding_removed->ip);
				}
			}
			
			//删除浮动IP对应出口交换机上相关内容
			external_port_p S_TargetExternalPort =find_openstack_external_by_floating_ip(binding_removed->ip);
			if(S_TargetExternalPort)
			{
				gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
				if(S_TargetExternalSwitch)
				{
					//直接删流表
					install_remove_QOS_jump_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,binding_removed->ip);
				}
			}
		}
	}
	else if(0 == strcmp((binding_removed->device_owner),TYPE_Loadbalance))
	{
		//根据路由器gateway ip找到external port
		external_port_p S_TargetExternalPort =find_openstack_external_by_network_id(binding_removed->subnet_id);
		if(S_TargetExternalPort)
		{
			//根据external port的dpid找物理交换机句柄
			gn_switch_t * S_TargetExternalSwitch =find_sw_by_dpid(S_TargetExternalPort->external_dpid);
			if(S_TargetExternalSwitch)
			{
				//直接删流表
				install_remove_QOS_jump_flow(S_TargetExternalSwitch,FABRIC_QOS_OUT_TABLE,binding_removed->ip);
			}		
		}
	}
	return 1;
}

/*QOS instance相关----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* by:yhy
 * 增
 */
openstack_qos_instance_p    openstack_qos_C_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id,char * qos_rule_id,char *qos_operation_type,UINT8 	switch_dpid,UINT4	meter_id,UINT4	table_id)
{
	openstack_qos_instance_p H_new_qos_instance =(openstack_qos_instance_p)malloc(sizeof(openstack_qos_instance));
	
	if(H_new_qos_instance)
	{
		memset(H_new_qos_instance, 0, sizeof(openstack_qos_instance));
		strcpy(H_new_qos_instance->qos_rule_id,qos_rule_id);
		strcpy(H_new_qos_instance->qos_binding_id,qos_binding_id);
		strcpy(H_new_qos_instance->qos_operation_type,qos_operation_type);
		H_new_qos_instance->switch_dpid=switch_dpid;
		H_new_qos_instance->meter_id=meter_id;
		H_new_qos_instance->table_id=table_id;
		
		if(*qos_instance_list)
		{
			(*qos_instance_list)->pre = H_new_qos_instance;
			H_new_qos_instance->next =(*qos_instance_list);
			(*qos_instance_list) =H_new_qos_instance;
		}
		else
		{
			(*qos_instance_list )=H_new_qos_instance;
		}
		return( *qos_instance_list);
	}
	else
	{//无法获取空间
		LOG_PROC("ERROR", "Cant get memory :%s_%d",FN,LN);
		return NULL;
	}
}

/* by:yhy
 * 查
 */
openstack_qos_instance_p    openstack_qos_R_instance(openstack_qos_instance_p * qos_instance_list,char * qos_binding_id)
{
	openstack_qos_instance_p S_current_qos_instance = * qos_instance_list;
	while(S_current_qos_instance)
	{
		if(0 == strcmp(S_current_qos_instance->qos_binding_id,qos_binding_id))
		{
			return S_current_qos_instance;
		}
		S_current_qos_instance =S_current_qos_instance->next;
	}
	return NULL;
}

/* by:yhy
 * 改
 */
openstack_qos_instance_p    openstack_qos_U_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id,char * qos_rule_id,char *qos_operation_type,UINT8 	switch_dpid,UINT4	meter_id,UINT4	table_id)
{
	openstack_qos_instance_p S_target_qos_instance =openstack_qos_R_instance(qos_instance_list,qos_binding_id);
	if(S_target_qos_instance)
	{
		strcpy(S_target_qos_instance->qos_rule_id,qos_rule_id);
		strcpy(S_target_qos_instance->qos_operation_type,qos_operation_type);
		S_target_qos_instance->switch_dpid=switch_dpid;
		S_target_qos_instance->meter_id=meter_id;
		S_target_qos_instance->table_id=table_id;
		return S_target_qos_instance;
	}
	else
	{
		return openstack_qos_C_instance(qos_instance_list,qos_binding_id,qos_rule_id,qos_operation_type,switch_dpid,meter_id,table_id);
	}
}

/* by:yhy
 * 删
 */
openstack_qos_instance_p    openstack_qos_D_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id)
{
	openstack_qos_instance_p S_target_qos_instance =openstack_qos_R_instance(qos_instance_list,qos_binding_id);
	if(S_target_qos_instance)
	{
		if((S_target_qos_instance ->pre) &&(S_target_qos_instance ->next))
		{
			S_target_qos_instance ->pre ->next =S_target_qos_instance ->next;
			S_target_qos_instance ->next ->pre =S_target_qos_instance ->pre;
		}
		else if((S_target_qos_instance ->pre ==NULL) &&(S_target_qos_instance ->next))
		{
			(*qos_instance_list) =S_target_qos_instance ->next;
			S_target_qos_instance ->next ->pre =NULL;
		}
		else if((S_target_qos_instance ->pre) &&(S_target_qos_instance ->next ==NULL))
		{
			S_target_qos_instance ->pre ->next =NULL;
		}
		else
		{
			(*qos_instance_list) =NULL;
		}
		free(S_target_qos_instance);
		return (*qos_instance_list);
	}
	else
	{
		return NULL;
	}
}

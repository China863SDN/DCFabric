/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
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

/*
 * openstack_app.c
 *
 *  Created on: Jun 18, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */
#include "openstack_app.h"
#include "fabric_openstack_arp.h"
#include "fabric_floating_ip.h"
#include "../inc/fabric/fabric_flows.h"
#include "openflow-common.h"

#define DIRECTION_NULL 	0
#define DIRECTION_IN 	1
#define DIRECTION_OUT 	2

#define SRC_MAC_UNUSED  0
#define SRC_MAC_USED    1

#define SECURITY_DROP_IDLE_TIMEOUT  103
#define SECURITY_DROP_HARD_TIMEOUT  103

extern void *g_openstack_security_rule_id;
extern void *g_openstack_security_group_id;
extern openstack_security_p g_openstack_security_list_temp;
extern void *g_openstack_security_group_id_temp;
extern void *g_openstack_host_security_id;

UINT1 CompareSecurityBetweenOld2NewAndDownloadFlow(p_fabric_host_node hostNode_p,gn_switch_t* sw,UINT1* Old_security_group,UINT1* New_security_group,const UINT2 Old_security_num,const UINT2 New_security_num,UINT1* SrcMAC_Used);
void GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(p_fabric_host_node hostNode_p,gn_switch_t* sw,INT1 Direction,UINT1* SrcMAC_Used);
void GenerateAndInstall_DenyFlow_by_DeletedSecurityRule (p_fabric_host_node hostNode_p,gn_switch_t* sw, openstack_security_rule_p DeletedSecurityRule ,UINT1* SrcMAC_Used);

void update_openstack_external_by_host(p_fabric_host_node host);
//by:yhy 通过给定的参数,创建一个openstack_network_p节点并添加到g_openstack_host_network_list中
openstack_network_p create_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external)
{
	openstack_network_p network = NULL;
	//printf("Tenant id: %s | Network_id: %s | Shared: %d \n",tenant_id,network_id,shared);
	network = create_openstack_host_network(tenant_id,network_id,shared,external);
	add_openstack_host_network(network);
	return network;
};

INT4 compare_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external,
		openstack_network_p network_p)
{
	if ((network_p) 
		&& compare_str(tenant_id, network_p->tenant_id)
		&& compare_str(network_id, network_p->network_id)
		&& (external == network_p->external)
		&& (shared == network_p->shared)) {
		return 1;
	}

	return 0;
}
//by:yhy 根据给定的参数更新openstack的app network
openstack_network_p update_openstack_app_network(
		char* tenant_id,
		char* network_id,
		UINT1 shared,
		UINT1 external)
{
	openstack_network_p network = NULL;
	network = find_openstack_host_network_by_network_id(network_id);
	if(network == NULL) 
	{
		network = create_openstack_app_network(tenant_id,network_id,shared,external);
//		add_openstack_host_network(network);
		if (network)
		{
			network->check_status = (UINT1)CHECK_CREATE;
		}
	}
	else if(compare_openstack_app_network(tenant_id, network_id, shared, external, network)) 
	{
		network->check_status = (UINT1)CHECK_LATEST;
	}
	else 
	{
		strcpy(network->tenant_id,tenant_id);
//		strcpy(network->network_id,network_id);
		network->external = external;
		network->shared = shared;
		network->check_status = (UINT1)CHECK_UPDATE;
	}

	return network;
};
//by:yhy 根据给定的参数,创建(openstack_subnet_p)subnet
openstack_subnet_p create_openstack_app_subnet(
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		UINT4 gateway_ip,
		UINT4 start_ip,
		UINT4 end_ip,
		UINT1* gateway_ipv6,
		UINT1* start_ipv6,
		UINT1* end_ipv6,
		char* cidr,
		UINT1 external)
{
	openstack_subnet_p subnet = NULL;
	openstack_network_p network = NULL;
	subnet = create_openstack_host_subnet(tenant_id,network_id,subnet_id,gateway_ip,start_ip,end_ip, gateway_ipv6, start_ipv6, end_ipv6, cidr, external);
	add_openstack_host_subnet(subnet);
	// find network
	network = find_openstack_host_network_by_network_id(network_id);
	if(network == NULL)
	{
		network = create_openstack_app_network(tenant_id,network_id,0,0);
	}
	network->subnet_num++;

	return subnet;
};

INT4 compare_openstack_app_subnet(
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		UINT4 gateway_ip,
		UINT4 start_ip,
		UINT4 end_ip,
		UINT1* gateway_ipv6,
		UINT1* start_ipv6,
		UINT1* end_ipv6,
		char* cidr, 
		UINT1 external,
		openstack_subnet_p subnet_p)
{
	 // printf("1.%s,%s,%s,%d,%d,%d,%s,%s,%s,%s,%d\n",tenant_id, network_id, subnet_id, gateway_ip, start_ip, end_ip,
	 //			gateway_ipv6, start_ipv6, end_ipv6, cidr,external);
	 // printf("2.%s,%s,%s,%d,%d,%d,%s,%s,%s,%s,%d\n",subnet_p->tenant_id, subnet_p->network_id, subnet_p->subnet_id, 
	 //	subnet_p->gateway_ip, subnet_p->start_ip, subnet_p->end_ip,
	 //		subnet_p->gateway_ipv6, subnet_p->start_ipv6, subnet_p->end_ipv6, subnet_p->cidr,subnet_p->external);	

	if ((subnet_p)
		&& compare_str(tenant_id, subnet_p->tenant_id)
		&& compare_str(network_id, subnet_p->network_id)
		&& compare_str(subnet_id, subnet_p->subnet_id)
		&& (gateway_ip == subnet_p->gateway_ip)
		&& (start_ip== subnet_p->start_ip)
		&& (end_ip == subnet_p->end_ip)
		&& compare_array(gateway_ipv6, subnet_p->gateway_ipv6, 16)
		&& compare_array(start_ipv6, subnet_p->start_ipv6, 16)
		&& compare_array(end_ipv6, subnet_p->end_ipv6, 16)
		&& (external == subnet_p->external)
		&& compare_str(cidr, subnet_p->cidr)) {
		return 1;
	}
	
	return 0;
		
}
//by:yhy 根据给定的参数更新openstack的app subnet
openstack_subnet_p update_openstack_app_subnet(
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		UINT4 gateway_ip,
		UINT4 start_ip,
		UINT4 end_ip,
		UINT1* gateway_ipv6,
		UINT1* start_ipv6,
		UINT1* end_ipv6,
		char* cidr)
{
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);

	UINT1 external = 0;
	openstack_network_p network_p = find_openstack_host_network_by_network_id(network_id);
	if (network_p && (network_p->external)) {
		external = network_p->external;
	}
	if(subnet == NULL) {
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,gateway_ip,start_ip, end_ip,
						gateway_ipv6, start_ipv6, end_ipv6,cidr,external);
//		add_openstack_host_subnet(subnet);
		if (subnet) 
			subnet->check_status = (UINT2)CHECK_CREATE;
	}
	else if (compare_openstack_app_subnet(tenant_id, network_id, subnet_id, gateway_ip, start_ip, end_ip,
				gateway_ipv6, start_ipv6, end_ipv6, cidr, external, subnet)) {
		subnet->check_status = (UINT2)CHECK_LATEST;
	}
	else{
		strcpy(subnet->tenant_id,tenant_id);
		strcpy(subnet->network_id,network_id);
//		strcpy(subnet->subnet_id, subnet_id);
		subnet->gateway_ip = gateway_ip;
		subnet->start_ip = start_ip;
		subnet->end_ip = end_ip;
		strcpy(subnet->cidr, cidr);
		subnet->external = external;
		subnet->check_status = (UINT2)CHECK_UPDATE;
	}

	if (subnet && is_check_status_changed(subnet->check_status)) {
		create_proactive_floating_internal_subnet_flow_by_subnet(subnet);
	}
	
	return subnet;
};
//by:yhy 根据给定参数创建openstack的一个host节点及对应的子网节点
p_fabric_host_node create_openstack_app_port(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id,
		UINT2 security_num,
		UINT1* security_group)
{
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id, security_num, security_group);
	//add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL)
	{
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,ipv6,NULL,NULL,"",0);
	}
	subnet->port_num++;
	return port;
};

openstack_port_p create_openstack_app_gateway(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
	// add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,ipv6,NULL,NULL,"",0);
	}
	subnet->gateway_port = port;
	subnet->port_num++;
	return port;
};

openstack_port_p create_openstack_app_dhcp(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	openstack_port_p port = NULL;
	openstack_subnet_p subnet = NULL;
	port = create_openstack_host_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
	// add_openstack_host_port(port);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL){
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,0,0,0,NULL,NULL,NULL,"",0);
	}
	subnet->dhcp_port = port;
	subnet->port_num++;
	return port;
};
//by:yhy 比较参数host_p中各项与其他给定参数是否一致
INT4 compare_openstack_app_host_by_rest(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id,
		UINT2 security_num,
		UINT1* security_group,
		p_fabric_host_node host_p,
		UINT1* SrcMAC_Used)
{
	if ((host_p)
		// && compare_pointer(sw, host_p->sw)
		&& (port_type == host_p->type)
		// && (port_no == host_p->port)
		&& (ip == host_p->ip_list[0])
		&& compare_array(ipv6, host_p->ipv6, 16)
		&& compare_array(mac, host_p->mac, 6)) 
	{
		openstack_port_p port_p = (openstack_port_p)host_p->data;
		if ((port_p)
			&& compare_str(tenant_id, port_p->tenant_id)
			&& compare_str(network_id, port_p->network_id)
			&& compare_str(subnet_id, port_p->subnet_id)
			&& compare_str(port_id, port_p->port_id))
		{
			/* need security?? */
			openstack_port_p port_p_data = (openstack_port_p)host_p->data;			
			return (CompareSecurityBetweenOld2NewAndDownloadFlow(host_p,host_p->sw,port_p_data->security_data,security_group,port_p_data->security_num ,security_num,SrcMAC_Used));
		}
	}
	

	return 0;
}
UINT1 CompareSecurityItemBetweenOld2NewAndDownloadFlow(p_fabric_host_node hostNode_p,gn_switch_t* sw,openstack_security_rule_p Old_security_item,openstack_security_rule_p New_security_item,UINT1* SrcMAC_Used)
{
	openstack_security_rule_p Temp_DeletedSecurityRule =NULL;
	openstack_security_rule_p Temp_AddedSecurityRule =NULL;
	openstack_security_rule_p Temp_NewSecurityRule =NULL;
	openstack_security_rule_p Temp_OldSecurityRule =NULL;
	INT1 OldSecurityItem_Matched = 0;
	INT1 NewSecurityItem_Matched = 0;
	UINT1 Return_Value =1;//1表示安全组没有改动,0表示改动过
	//LOG_PROC("INFO","-----------------------%s",FN);
	//比对减少的规则
	Temp_NewSecurityRule =New_security_item;
	Temp_OldSecurityRule =Old_security_item;
	//LOG_PROC("INFO", "%s---------------------------------------Old->New",FN);
	while(Temp_OldSecurityRule)
	{
		//LOG_PROC("INFO", "%s--------------------------------------Old-%s",FN,Temp_OldSecurityRule->rule_id);
		OldSecurityItem_Matched =0;
		while(Temp_NewSecurityRule)
		{
			//LOG_PROC("INFO", "%s--------------------------------------New-%s",FN,Temp_NewSecurityRule->rule_id);
			if(0==(strcmp(Temp_NewSecurityRule->rule_id,Temp_OldSecurityRule->rule_id)))
			{//找到安全组中一致的规则
				OldSecurityItem_Matched =1;
				break;
			}
			Temp_NewSecurityRule =Temp_NewSecurityRule->next;
		}
		
		if(1 == OldSecurityItem_Matched)
		{//旧安全组中规则在新安全组中存在一样的规则//不做任何动作
		}
		else
		{//旧安全组中的规则在新安全组中被删除
			//LOG_PROC("INFO", "%s---------------------------------------Del a Old Rule",FN);
			Return_Value =0;
			GenerateAndInstall_DenyFlow_by_DeletedSecurityRule(hostNode_p,sw,Temp_OldSecurityRule,SrcMAC_Used);
		}
		Temp_NewSecurityRule =New_security_item;
		Temp_OldSecurityRule =Temp_OldSecurityRule->next;
	}
	
	//比对增加的规则
	Temp_NewSecurityRule =New_security_item;
	Temp_OldSecurityRule =Old_security_item;
	//LOG_PROC("INFO", "%s---------------------------------------New->Old",FN);
	while(Temp_NewSecurityRule)
	{
		//LOG_PROC("INFO", "%s--------------------------------------New-%s",FN,Temp_NewSecurityRule->rule_id);
		NewSecurityItem_Matched =0;
		while(Temp_OldSecurityRule)
		{
			//LOG_PROC("INFO", "%s--------------------------------------Old-%s",FN,Temp_OldSecurityRule->rule_id);
			if(0==(strcmp(Temp_NewSecurityRule->rule_id,Temp_OldSecurityRule->rule_id)))
			{//找到安全组中一致的规则
				NewSecurityItem_Matched =1;
				break;
			}
			Temp_OldSecurityRule =Temp_OldSecurityRule->next;
		}
		
		if(1 == NewSecurityItem_Matched)
		{//新安全组中规则在旧安全组中存在一样的规则//不做任何动作
		}
		else
		{//新安全组中的规则在旧安全组中不存在//即新增一条规则
			//LOG_PROC("INFO", "%s---------------------------------------Add a New Rule",FN);
			Return_Value =0;
		}
		Temp_OldSecurityRule =Old_security_item;
		Temp_NewSecurityRule =Temp_NewSecurityRule->next;
	}
	//LOG_PROC("INFO", "%s---------------------------------------stop,return %d",FN,Return_Value);
	return Return_Value;
}

//比较新旧安全组是否变化
UINT1 CompareSecurityBetweenOld2NewAndDownloadFlow(p_fabric_host_node hostNode_p,gn_switch_t* sw,UINT1* Old_security_group,UINT1* New_security_group,const UINT2 Old_security_num,const UINT2 New_security_num,UINT1* SrcMAC_Used)
{
	openstack_node_p Temp_OldSecurityGroup_List =(openstack_node_p)Old_security_group;
	openstack_node_p Temp_NewSecurityGroup_List =(openstack_node_p)New_security_group;
	openstack_security_p Temp_OldSecurityGroup_ListMember =NULL;
	openstack_security_p Temp_NewSecurityGroup_ListMember =NULL;
	openstack_security_p Temp_DeletedSecurityGroup =NULL;
	openstack_security_rule_p Temp_DeletedSecurityRule =NULL;
	INT1 OldSecurityGroup_Matched = 0;
	UINT1 Return_Value =1;//1表示安全组没有改动,0表示改动过
	//LOG_PROC("INFO", "%s---------------------------------------start,old:%d,new:%d,mac:%d:%d:%d:%d:%d:%d",FN,Old_security_num,New_security_num,hostNode_p->mac[0],hostNode_p->mac[1],hostNode_p->mac[2],hostNode_p->mac[3],hostNode_p->mac[4],hostNode_p->mac[5]);
	if(0==New_security_num)
	{
		if(0==Old_security_num)
		{//安全组未改变//不处理
		}
		else
		{//安全组清空//下发全阻断流表
			GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(hostNode_p,sw,DIRECTION_IN,SrcMAC_Used);
			GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(hostNode_p,sw,DIRECTION_OUT,SrcMAC_Used);
			//LOG_PROC("INFO", "%s---------------------------------------tag1",FN);
			Return_Value = 0;
		}
	}
	else
	{
		if(0==Old_security_num)
		{//安全组数量增加//不处理
			//LOG_PROC("INFO", "%s---------------------------------------tag2",FN);
			Return_Value = 0;
		}
		else
		{//安全组可能存在变动//遍历安全组变化
			while(Temp_OldSecurityGroup_List)
			{//遍历旧安全组
				Temp_OldSecurityGroup_ListMember =(openstack_security_p)(Temp_OldSecurityGroup_List->data);
				//LOG_PROC("INFO", "%s---------------------------------------old:%p,%s",FN,&(Temp_OldSecurityGroup_ListMember->security_group),Temp_OldSecurityGroup_ListMember->security_group);
				if(NULL != Temp_OldSecurityGroup_ListMember)
				{
					Temp_NewSecurityGroup_List=(openstack_node_p)New_security_group;
					while(Temp_NewSecurityGroup_List)
					{//遍历新安全组
						Temp_NewSecurityGroup_ListMember =(openstack_security_p)(Temp_NewSecurityGroup_List->data);	
						//LOG_PROC("INFO", "%s---------------------------------------new:%s",FN,Temp_NewSecurityGroup_ListMember->security_group);
						if(NULL != Temp_NewSecurityGroup_ListMember)
						{
							if(0==(strcmp(Temp_NewSecurityGroup_ListMember->security_group,Temp_OldSecurityGroup_ListMember->security_group)))
							{//找到一致的安全组
								OldSecurityGroup_Matched =1;
								break;
							}
							Temp_NewSecurityGroup_List =Temp_NewSecurityGroup_List->next;
						}
					}
					
					if(1 == OldSecurityGroup_Matched)
					{//旧安全组在新安全组中存在//一一比对安全组中的规则
						if(0 == CompareSecurityItemBetweenOld2NewAndDownloadFlow(hostNode_p,sw,Temp_OldSecurityGroup_ListMember->security_rule_p,Temp_NewSecurityGroup_ListMember->security_rule_p,SrcMAC_Used))
						{
							//LOG_PROC("INFO", "%s---------------------------------------tag3",FN);
							Return_Value = 0;
						}
					}
					else
					{//旧安全组在新安全组中删除
						//下发针对这个安全组的deny流表
						//LOG_PROC("INFO", "%s---------------------------------------tag4",FN);
						Return_Value = 0;
						Temp_DeletedSecurityGroup =Temp_OldSecurityGroup_ListMember;
						Temp_DeletedSecurityRule =Temp_DeletedSecurityGroup->security_rule_p;
						while(Temp_DeletedSecurityRule)
						{
							//构造deny流表
							GenerateAndInstall_DenyFlow_by_DeletedSecurityRule(hostNode_p,sw,Temp_DeletedSecurityRule,SrcMAC_Used);
							Temp_DeletedSecurityRule = Temp_DeletedSecurityRule ->next;
						}	
					}
				}
				OldSecurityGroup_Matched = 0;
				Temp_OldSecurityGroup_List =Temp_OldSecurityGroup_List->next;
			}
		}
	}
	if(New_security_num >Old_security_num)
	{
		//LOG_PROC("INFO", "%s---------------------------------------tag5",FN);
		Return_Value = 0;
	}
	//LOG_PROC("INFO", "%s---------------------------------------stop,return %d",FN,Return_Value);
	return Return_Value;
}
//对本host(port)进行全阻断
void GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(p_fabric_host_node hostNode_p,gn_switch_t* sw,INT1 Direction,UINT1* SrcMAC_Used)
{
	UINT2 security_drop_idle_timeout =SECURITY_DROP_IDLE_TIMEOUT;
	UINT2 security_drop_hard_timeout =SECURITY_DROP_HARD_TIMEOUT;
	//LOG_PROC("INFO", "222222:%s---------------------------------------",FN);
	if(sw != NULL) 
	{
		flow_param_t* flow_param = init_flow_param();
		
		flow_param->match_param->eth_type = ETHER_IP;

		if(Direction == DIRECTION_IN)
		{//阻断进
			if (hostNode_p->mac)
			{
				memcpy(flow_param->match_param->eth_src, (hostNode_p->mac), 6);
				(* SrcMAC_Used) =SRC_MAC_USED;
				add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
				install_fabric_flows(sw, 
									 security_drop_idle_timeout, 
									 security_drop_hard_timeout, 
									 FABRIC_PRIORITY_DENY_FLOW,
									 FABRIC_INPUT_TABLE, 
									 OFPFC_ADD, 
									 flow_param);

				clear_flow_param(flow_param);
			}		
		}
		else if(Direction == DIRECTION_OUT)
		{//阻断出
			if (hostNode_p->mac)
			{
				memcpy(flow_param->match_param->eth_dst, (hostNode_p->mac), 6);
				add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
				install_fabric_flows(sw, 
									 security_drop_idle_timeout, 
									 security_drop_hard_timeout, 
									 FABRIC_PRIORITY_DENY_FLOW,
									 FABRIC_INPUT_TABLE, 
									 OFPFC_ADD, 
									 flow_param);

				clear_flow_param(flow_param);
			}
		}
		else
		{//其他//不处理
		}
	}
}
//根据DeletedSecurityRule对sw下发阻断流表
void GenerateAndInstall_DenyFlow_by_DeletedSecurityRule (p_fabric_host_node hostNode_p,gn_switch_t* sw, openstack_security_rule_p DeletedSecurityRule ,UINT1* SrcMAC_Used)
{
	char  Rule_RemoteIPPrefix_Copy[OPENSTACK_SECURITY_GROUP_LEN] ={0};
	char * IPPrefix_HeadPointer =NULL;
	char  IPAddr_STR[OPENSTACK_SECURITY_GROUP_LEN] ={0};
	char  IPPrefix_STR[OPENSTACK_SECURITY_GROUP_LEN] ={0};
	UINT4 IP_Val 	=0;
	UINT4 IP_Prefix =0;
	UINT4 Temp_Port =0;
	UINT2 security_drop_idle_timeout =SECURITY_DROP_IDLE_TIMEOUT;
	UINT2 security_drop_hard_timeout =SECURITY_DROP_HARD_TIMEOUT;
	//LOG_PROC("INFO", "111111:%s---------------------------------------",FN);
	flow_param_t* flow_param = init_flow_param();
	
	if((sw != NULL)&&(DeletedSecurityRule != NULL)) 
	{
		
		if(0 == strlen(DeletedSecurityRule->protocol))
		{//协议为空,全网deny
			//TBD//////////////
			GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(hostNode_p,sw,DIRECTION_IN,SrcMAC_Used);
			GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(hostNode_p,sw,DIRECTION_OUT,SrcMAC_Used);
		}
		else
		{//存在协议区分
			memcpy(Rule_RemoteIPPrefix_Copy,DeletedSecurityRule->remote_ip_prefix,OPENSTACK_SECURITY_GROUP_LEN);
			//LOG_PROC("INFO", "TEST------%s-%s-------------------",FN,Rule_RemoteIPPrefix_Copy);
			
			if(strlen(Rule_RemoteIPPrefix_Copy)>=9)
			{//IP地址有效
				
				IPPrefix_HeadPointer =strchr(Rule_RemoteIPPrefix_Copy,'/');
				strncpy(IPAddr_STR,Rule_RemoteIPPrefix_Copy,(strlen(Rule_RemoteIPPrefix_Copy)-strlen(IPPrefix_HeadPointer)));
				strncpy(IPPrefix_STR,(IPPrefix_HeadPointer+1),(strlen(IPPrefix_HeadPointer)-1));
				
				IP_Val=inet_addr(IPAddr_STR);
				IP_Prefix =atoi(IPPrefix_STR);
				
				flow_param->match_param->eth_type = ETHER_IP;
				memcpy(flow_param->match_param->eth_src, hostNode_p->mac, 6);
				
				if(0 == strcmp("icmp", DeletedSecurityRule->protocol))
				{//ICMP
					//LOG_PROC("INFO", "TEST------%s--------------------",FN);
					flow_param->match_param->ip_proto = IPPROTO_ICMP;

					if(0 == strcmp("ingress", DeletedSecurityRule->direction))
					{//入口
						flow_param->match_param->ipv4_src = IP_Val;
						//flow_param->match_param->ipv4_src_prefix =IP_Prefix;
					}
					else if(0 == strcmp("egress", DeletedSecurityRule->direction))
					{//出口
						flow_param->match_param->ipv4_dst = IP_Val;
						//flow_param->match_param->ipv4_dst_prefix =IP_Prefix;
					}
					else
					{//错误
						return;
					}
					//LOG_PROC("INFO", "TEST------%s--------------------",FN);
					if((DeletedSecurityRule->port_range_min !=0)&&(DeletedSecurityRule->port_range_max !=0))
					{
						flow_param->match_param->icmpv4_type = DeletedSecurityRule->port_range_min;
						flow_param->match_param->icmpv4_code = DeletedSecurityRule->port_range_max;
					}
					add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
					/////////改参数
					install_fabric_flows(sw, 
										 security_drop_idle_timeout, 
										 security_drop_hard_timeout, 
										 FABRIC_PRIORITY_DENY_FLOW,
										 FABRIC_INPUT_TABLE, 
										 OFPFC_ADD, 
										 flow_param);

					clear_flow_param(flow_param);
					return;
				}
				else if(0 == strcmp("tcp", DeletedSecurityRule->protocol))
				{//TCP
					
					flow_param->match_param->ip_proto = IPPROTO_TCP;
					if(0 == strcmp("ingress", DeletedSecurityRule->direction))
					{//入口
						flow_param->match_param->ipv4_src = ntohl(IP_Val);
						//flow_param->match_param->ipv4_src_prefix =IP_Prefix;
						
						if((DeletedSecurityRule->port_range_max)==(DeletedSecurityRule->port_range_min))
						{//最大端口号与最小端口号一致
							if(0==(DeletedSecurityRule->port_range_max))
							{//端口为0//不对端口做限制
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
							else
							{//只有一个端口//下发一个deny
								flow_param->match_param->tcp_src =DeletedSecurityRule->port_range_min;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
						}
						else
						{//最大端口号与最小端口号不一致
							//LOG_PROC("INFO", "TEST------%d-%d-------------------",DeletedSecurityRule->port_range_max,DeletedSecurityRule->port_range_min);
							for(Temp_Port = DeletedSecurityRule->port_range_min ; Temp_Port <=DeletedSecurityRule->port_range_max; Temp_Port++)
							{
								//LOG_PROC("INFO", "TEST------%d-------------------",Temp_Port);
								flow_param->match_param->tcp_src = Temp_Port;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);
							}
							clear_flow_param(flow_param);
							return ;
						}
					}
					else if(0 == strcmp("egress", DeletedSecurityRule->direction))
					{//出口
						flow_param->match_param->ipv4_dst = ntohl(IP_Val);
						//flow_param->match_param->ipv4_dst_prefix =IP_Prefix;

						if((DeletedSecurityRule->port_range_max)==(DeletedSecurityRule->port_range_min))
						{//最大端口号与最小端口号一致
							if(0==(DeletedSecurityRule->port_range_max))
							{//端口为0//不对端口做限制
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
							else
							{//只有一个端口//下发一个deny
								flow_param->match_param->tcp_dst =DeletedSecurityRule->port_range_min;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
						}
						else
						{//最大端口号与最小端口号不一致
							for(Temp_Port = DeletedSecurityRule->port_range_min ; Temp_Port <=DeletedSecurityRule->port_range_max; Temp_Port++)
							{
								flow_param->match_param->tcp_dst = Temp_Port;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);
							}
							clear_flow_param(flow_param);
							return ;
						}
					}
					else
					{//错误//不处理
						if(0 == strcmp("ingress", DeletedSecurityRule->direction))
						{//入口
							flow_param->match_param->ipv4_src = IP_Val;
						}
						else if(0 == strcmp("egress", DeletedSecurityRule->direction))
						{//出口
							flow_param->match_param->ipv4_dst = IP_Val;
						}
						else
						{//错误
							return;
						}
						
						add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
						/////////改参数
						install_fabric_flows(sw, 
											 security_drop_idle_timeout, 
											 security_drop_hard_timeout, 
											 FABRIC_PRIORITY_DENY_FLOW,
											 FABRIC_INPUT_TABLE, 
											 OFPFC_ADD, 
											 flow_param);

						clear_flow_param(flow_param);
						return;
					}	
				}
				else if(0 == strcmp("udp", DeletedSecurityRule->protocol))
				{//UDP
					flow_param->match_param->ip_proto = IPPROTO_UDP;
					if(0 == strcmp("ingress", DeletedSecurityRule->direction))
					{//入口
						flow_param->match_param->ipv4_src = ntohl(IP_Val);
						//flow_param->match_param->ipv4_src_prefix =IP_Prefix;

						if((DeletedSecurityRule->port_range_max)==(DeletedSecurityRule->port_range_min))
						{//最大端口号与最小端口号一致
							if(0==(DeletedSecurityRule->port_range_max))
							{//端口为0//不对端口做限制
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
							else
							{//只有一个端口//下发一个deny
								flow_param->match_param->udp_src =DeletedSecurityRule->port_range_min;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
						}
						else
						{//最大端口号与最小端口号不一致
							for(Temp_Port = DeletedSecurityRule->port_range_min ; Temp_Port <=DeletedSecurityRule->port_range_max; Temp_Port++)
							{
								flow_param->match_param->udp_src = Temp_Port;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);
							}
							clear_flow_param(flow_param);
							return ;
						}
					}
					else if(0 == strcmp("egress", DeletedSecurityRule->direction))
					{//出口
						flow_param->match_param->ipv4_dst = ntohl(IP_Val);
						//flow_param->match_param->ipv4_dst_prefix =IP_Prefix;

						if((DeletedSecurityRule->port_range_max)==(DeletedSecurityRule->port_range_min))
						{//最大端口号与最小端口号一致
							if(0==(DeletedSecurityRule->port_range_max))
							{//端口为0//不对端口做限制
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
							else
							{//只有一个端口//下发一个deny
								flow_param->match_param->udp_dst =DeletedSecurityRule->port_range_min;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
						}
						else
						{//最大端口号与最小端口号不一致
							for(Temp_Port = DeletedSecurityRule->port_range_min ; Temp_Port <=DeletedSecurityRule->port_range_max; Temp_Port++)
							{
								flow_param->match_param->udp_dst = Temp_Port;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);
							}
							clear_flow_param(flow_param);
							return ;
						}
					}
					else
					{//错误//不处理
						return;
					}
				}
				else
				{//其他协议不支持//不处理
					return;
				}
			}
			else
			{//IP为null
				////////////不处理
			}			
		}
	}
}




//by:yhy 根据给定的参数更新一各openstack的host节点
p_fabric_host_node update_openstack_app_host_by_rest(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id,
		UINT2 security_num,
		UINT1* security_group) 
{
	p_fabric_host_node port = NULL;
	UINT1 SrcMAC_Used = SRC_MAC_UNUSED;
	//	LOG_PROC("INFO","PORT UPDATE!");
	port = get_fabric_host_from_list_by_mac(mac);
	if(port == NULL)
	{
		port = create_openstack_app_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,security_num,security_group);
		//add_openstack_host_port(port);
		if (port) 
		{
			port->check_status = (UINT2)CHECK_CREATE;
		}
	}
	else if (1==compare_openstack_app_host_by_rest(sw,port_type,port_no,ip,ipv6,mac,tenant_id,
				network_id,subnet_id,port_id,security_num,security_group,port,&SrcMAC_Used)) 
	{
		//LOG_PROC("INFO","-----------------------%s-tag1",FN);
        openstack_port_p port_p = (openstack_port_p)port->data;
        port_p->security_num = security_num;
		clear_openstack_host_security_node(port_p->security_data);
		port_p->security_data = security_group;		
		port->check_status = (UINT2)CHECK_LATEST;
		//LOG_PROC("INFO","CHECK_LATEST");
	}
	else 
	{
		//LOG_PROC("INFO","-----------------------%s-tag2",FN);
		openstack_port_p port_p = (openstack_port_p)port->data;
        port->type = port_type;
		port->ip_list[0] = ip;
		if (ipv6)
			memcpy(port->ipv6[0], ipv6, 16);
		strcpy(port_p->tenant_id,tenant_id);
		strcpy(port_p->network_id,network_id);
		strcpy(port_p->port_id,port_id);
		strcpy(port_p->subnet_id, subnet_id);
		port_p->security_num = security_num;
		clear_openstack_host_security_node(port_p->security_data);
		port_p->security_data = security_group;
		port->check_status = (UINT2)CHECK_UPDATE;
		//LOG_PROC("INFO","CHECK_UPDATE");
	}

	if (is_check_status_changed(port->check_status)) 
	{
		//LOG_PROC("INFO","is_check_status_changed");
		if(SrcMAC_Used == SRC_MAC_UNUSED)
		{
			openstack_ip_remove_deny_flow(mac,ip);
		}
		update_openstack_external_by_host(port);
	}
	return port;
};
/*
openstack_node_p openstack_security_app_ConventOldGroupList2NewGroupList(UINT1* security_group)
{
	openstack_node_p Temp_NewSecurityGroupList =(openstack_node_p)security_group;
	openstack_node_p Temp_OldSecurityGroupList =NULL;
	openstack_security_p Temp_NewSecurityGroup =NULL;
	openstack_security_p Temp_OldSecurityGroup =NULL;
	LOG_PROC("INFO","-----------------------%s",FN);
	while(Temp_NewSecurityGroupList)
	{
		Temp_NewSecurityGroup =Temp_NewSecurityGroupList->data;
		Temp_OldSecurityGroup =update_openstack_security_group(Temp_NewSecurityGroup->security_group);
		Temp_OldSecurityGroupList = add_openstack_host_security_node((UINT1*)Temp_OldSecurityGroup, Temp_OldSecurityGroupList);
		
		Temp_NewSecurityGroupList =Temp_NewSecurityGroupList->next;
	}	
	return Temp_OldSecurityGroupList;
}
*/
/*
//by:yhy 有问题
p_fabric_host_node update_openstack_app_host_by_rest_temp(
	gn_switch_t* sw,
	UINT1 port_type,
	UINT4 port_no,
	UINT4 ip,
	UINT1* ipv6,
	UINT1* mac,
	char* tenant_id,
	char* network_id,
	char* subnet_id,
	char* port_id,
	UINT2 security_num,
	UINT1* security_group) 
{
	LOG_PROC("INFO", "TEST-------%s-MAC:[%3d:%3d:%3d:%3d:%3d:%3d]SecurityNum:%d",FN,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],security_num);
	p_fabric_host_node port = NULL;
	UINT1 SrcMAC_Used = SRC_MAC_UNUSED;
	UINT1* Temp_security_group =NULL;

	port = get_fabric_host_from_list_by_mac(mac);
	if(port == NULL)
	{//新建的port
		///TBD  新旧更新(有问题)
		//ADD
		Temp_security_group =(UINT1*)(openstack_security_app_ConventOldGroupList2NewGroupList(security_group));
		//ADD
		clear_openstack_host_security_node(security_group);
		//MOD
		port = create_openstack_app_port(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,security_num,Temp_security_group);
		if (port) 
		{
			port->check_status = (UINT2)CHECK_CREATE;
			LOG_PROC("INFO","CHECK_CREATE");
		}
	}
	else if (1==compare_openstack_app_host_by_rest(sw,port_type,port_no,ip,ipv6,mac,tenant_id,
			 network_id,subnet_id,port_id,security_num,security_group,port,&SrcMAC_Used)) 
	{//port所有比对均一致
		///TBD 
		//MOD
		clear_openstack_host_security_node(security_group);
		port->check_status = (UINT2)CHECK_LATEST;
		LOG_PROC("INFO","CHECK_LATEST");
	}
	else 
	{//prot属性发生改变
		///TBD
		//ADD
		Temp_security_group =(UINT1*)(openstack_security_app_ConventOldGroupList2NewGroupList(security_group));
		//ADD
		clear_openstack_host_security_node(security_group);
		
		openstack_port_p port_p = (openstack_port_p)port->data;
        port->type = port_type;
		port->ip_list[0] = ip;
		if (ipv6)
			memcpy(port->ipv6[0], ipv6, 16);
		strcpy(port_p->tenant_id,tenant_id);
		strcpy(port_p->network_id,network_id);
		strcpy(port_p->port_id,port_id);
		strcpy(port_p->subnet_id, subnet_id);
		port_p->security_num = security_num;
		clear_openstack_host_security_node(port_p->security_data);
		//MOD
		port_p->security_data = Temp_security_group;
		port->check_status = (UINT2)CHECK_UPDATE;
		LOG_PROC("INFO","CHECK_UPDATE");
	}

	if (is_check_status_changed(port->check_status)) 
	{
		if(SrcMAC_Used == SRC_MAC_UNUSED)
		{
			openstack_ip_remove_deny_flow(mac,ip);
		}
		update_openstack_external_by_host(port);
	}
	return port;
};
*/
p_fabric_host_node update_openstack_app_gateway_by_rest(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
//	LOG_PROC("INFO","Gateway UPDATE!");
	port = update_openstack_app_host_by_rest(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL)
	{
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,ipv6,NULL,NULL,"",0);
	}
    
    if (OPENSTACK_PORT_TYPE_ROUTER_INTERFACE == port_type)
	{
	    subnet->gateway_port = port;
    }
	return port;
};

p_fabric_host_node update_openstack_app_dhcp_by_rest(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac,
		char* tenant_id,
		char* network_id,
		char* subnet_id,
		char* port_id)
{
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
//	LOG_PROC("INFO","DHCP UPDATE!");
	port = update_openstack_app_host_by_rest(sw,port_type,port_no,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
	// add to subnet;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
	if(subnet == NULL) {
		subnet = create_openstack_app_subnet(tenant_id,network_id,subnet_id,ip,0,0,ipv6,NULL,NULL,"",0);
	}
	subnet->dhcp_port = port;
	return port;
};
p_fabric_host_node update_openstack_app_host_by_sdn(
		gn_switch_t* sw,
		UINT1 port_type,
		UINT4 port_no,
		UINT4 ip,
		UINT1* ipv6,
		UINT1* mac){
	p_fabric_host_node port = NULL;
	port = get_fabric_host_from_list_by_mac(mac);
	if(port == NULL){
		port = create_openstack_app_port(sw,port_type,port_no,ip,ipv6,mac,"","","","",0,NULL);
//		add_openstack_host_port(port);
	}else{
		port->ip_list[0] = ip;
		memcpy(port->ipv6[0], ipv6, 16);
		port->sw = sw;
		port->port = port_no;
	}
	return port;
};

openstack_network_p find_openstack_app_network_by_network_id(char* network_id){
	return find_openstack_host_network_by_network_id(network_id);
};

openstack_subnet_p find_openstack_app_subnet_by_subnet_id(char* subnet_id){
	return find_openstack_host_subnet_by_subnet_id(subnet_id);
};

//openstack_port_p find_openstack_app_host_by_mac(UINT1* mac){
//	return find_openstack_host_port_by_mac(mac);
//};

//openstack_port_p find_openstack_app_host_by_ip_tenant(UINT4 ip,char* tenant_id){
//	return find_openstack_host_port_by_ip_tenant(ip,tenant_id);
//};

//openstack_port_p find_openstack_app_host_by_ip_network(UINT4 ip,char* network_id){
//	return find_openstack_host_port_by_ip_network(ip,network_id);
//};
//
//openstack_port_p find_openstack_app_host_by_ip_subnet(UINT4 ip,char* subnet_id){
//	return find_openstack_host_port_by_ip_subnet(ip,subnet_id);
//};
//by:yhy 根据host查找其对应的网关端口(根据子网ID是否一致)
p_fabric_host_node find_openstack_app_gateway_by_host(p_fabric_host_node host)
{
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	openstack_port_p port_p = (openstack_port_p)host->data;
	subnet = find_openstack_host_subnet_by_subnet_id(port_p->subnet_id);
	if (NULL != subnet) 
	{
		port = subnet->gateway_port;
	}
	return port;
};

p_fabric_host_node find_openstack_app_gateway_by_subnet_id(char* subnet_id){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
    if (NULL != subnet) {
        port = subnet->gateway_port;
    }
	return port;
};
p_fabric_host_node find_openstack_app_dhcp_by_host(openstack_port_p host){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(host->subnet_id);
    if (NULL != subnet) {
        port = subnet->dhcp_port;
    }
	return port;
};
p_fabric_host_node find_openstack_app_dhcp_by_subnet_id(char* subnet_id){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	subnet = find_openstack_host_subnet_by_subnet_id(subnet_id);
    if (NULL != subnet) {
        port = subnet->dhcp_port;
    }
	return port;
};

UINT4 find_openstack_app_hosts_by_subnet_id(char* subnet_id, p_fabric_host_node* host_list){
	return find_fabric_host_ports_by_subnet_id(subnet_id,host_list);
};


UINT4 check_openstack_app_host_is_gateway_by_mac(UINT1* mac){
	p_fabric_host_node port = NULL;
	openstack_subnet_p subnet = NULL;
	port = get_fabric_host_from_list_by_mac(mac);
	if(port != NULL){
		subnet = find_openstack_host_subnet_by_geteway_ip(port->ip_list[0]);
		if(subnet != NULL){
			return subnet->gateway_ip;
		}
	}
	return 0;
};
//by:yhy 根据p_fabric_host_node节点信息,更新对外网关端口
void update_openstack_external_by_host(p_fabric_host_node host)
{
	if ((host) && (OPENSTACK_PORT_TYPE_GATEWAY == host->type)) 
	{
		openstack_port_p port_p = (openstack_port_p)host->data;
		if (port_p)
		{
			update_openstack_external_by_outer_interface(host->ip_list[0], host->mac, port_p->network_id);
		}
	}
}
//by:yhy 检查状态位是否改变
INT4 is_check_status_changed(UINT1 check_status)
{
	if (((UINT2)CHECK_CREATE == check_status) || ((UINT2)CHECK_UPDATE == check_status)) 
	{
		return 1;
	}

	return 0;
}

INT1 show_security()
{
	p_fabric_host_node Temp_HostNodeList = g_fabric_host_list.list;
	openstack_port_p Temp_Port =NULL;
	openstack_node_p Temp_SecurityInfo_useTempGroup =NULL;
	openstack_security_p Temp_SecurityGroup_useTempGroup =NULL;
	LOG_PROC("INFO","-----------------------%s",FN);
	while(Temp_HostNodeList)
	{//遍历list中所有port
		if(OPENSTACK_PORT_TYPE_HOST == (Temp_HostNodeList->type))
		{//找到OPENSTACK_PORT_TYPE_HOST类型port
			Temp_Port =(openstack_port_p)Temp_HostNodeList->data;
			if(Temp_Port)
			{
				Temp_SecurityInfo_useTempGroup =(openstack_node_p)Temp_Port->security_data;
				LOG_PROC("INFO", "%s---------------------------------------start,mac:%d:%d:%d:%d:%d:%d",FN,Temp_HostNodeList->mac[0],Temp_HostNodeList->mac[1],Temp_HostNodeList->mac[2],Temp_HostNodeList->mac[3],Temp_HostNodeList->mac[4],Temp_HostNodeList->mac[5]);
				while(Temp_SecurityInfo_useTempGroup)
				{
					Temp_SecurityGroup_useTempGroup =(openstack_security_p)Temp_SecurityInfo_useTempGroup->data;
					LOG_PROC("INFO", "%s---------------------------------------useTempGroup:%p,%s",FN,&(Temp_SecurityGroup_useTempGroup->security_group),Temp_SecurityGroup_useTempGroup->security_group);
					Temp_SecurityInfo_useTempGroup =Temp_SecurityInfo_useTempGroup ->next;
				}
			}
		}
		Temp_HostNodeList = Temp_HostNodeList->next;
	}
}

//by:yhy 本函数是把各port指向新的临时安全组list的指针指回更新后的旧的安全组list上
INT1 update_From_NewSecurityInfo_To_OldSecurityInfo(p_fabric_host_node HostNode_list)
{
	p_fabric_host_node Temp_HostNodeList = g_fabric_host_list.list;
	openstack_port_p Temp_Port =NULL;
	openstack_node_p Temp_SecurityInfo_useTempGroup =NULL;
	openstack_node_p Temp_SecurityInfo_usePersistGroup = NULL;
	openstack_node_p NeedDeleted_SecurityInfo_useTempGroup =NULL;
	openstack_security_p Temp_SecurityGroup_useTempGroup =NULL;
	openstack_security_p Temp_SecurityGroup_usePersistGroup =NULL;
	//LOG_PROC("INFO","-----------------------%s",FN);
	while(Temp_HostNodeList)
	{//遍历list中所有port
		if(OPENSTACK_PORT_TYPE_HOST == (Temp_HostNodeList->type))
		{//找到OPENSTACK_PORT_TYPE_HOST类型port
			Temp_Port =(openstack_port_p)Temp_HostNodeList->data;
			if(Temp_Port)
			{
				Temp_SecurityInfo_useTempGroup =(openstack_node_p)Temp_Port->security_data;
				//LOG_PROC("INFO", "%s---------------------------------------start,mac:%d:%d:%d:%d:%d:%d",FN,Temp_HostNodeList->mac[0],Temp_HostNodeList->mac[1],Temp_HostNodeList->mac[2],Temp_HostNodeList->mac[3],Temp_HostNodeList->mac[4],Temp_HostNodeList->mac[5]);
				while(Temp_SecurityInfo_useTempGroup)
				{//遍历该port的Group链表
					Temp_SecurityGroup_useTempGroup =(openstack_security_p)Temp_SecurityInfo_useTempGroup->data;
					//LOG_PROC("INFO", "%s---------------------------------------useTempGroup:%p,%s",FN,&(Temp_SecurityGroup_useTempGroup->security_group),Temp_SecurityGroup_useTempGroup->security_group);
					//在PersistGroup内存中找到同名Group
					Temp_SecurityGroup_usePersistGroup = update_openstack_security_group(Temp_SecurityGroup_useTempGroup->security_group);
					//LOG_PROC("INFO", "%s---------------------------------------usePersistGroup:%p,%s",FN,&(Temp_SecurityGroup_usePersistGroup->security_group),Temp_SecurityGroup_usePersistGroup->security_group);
					//构造使用persistGroup的新的Group链表
					Temp_SecurityInfo_usePersistGroup = add_openstack_host_security_node((UINT1*)Temp_SecurityGroup_usePersistGroup, Temp_SecurityInfo_usePersistGroup);
					Temp_SecurityInfo_useTempGroup =Temp_SecurityInfo_useTempGroup ->next;
				}	
				//释放指向TempGroup的安全信息
				clear_openstack_host_security_node(Temp_Port->security_data);
				//重新将port的security_data指向PersistGroup
				Temp_Port->security_data =(UINT1*)Temp_SecurityInfo_usePersistGroup;
			}
		}
		Temp_SecurityInfo_usePersistGroup =NULL;
		Temp_HostNodeList = Temp_HostNodeList->next;
	}
	//show_security();
}






//by:yhy 本函数是遍历输入的新旧groupList中的各个group节点,根据新的临时安全组信息更新到持久化的就的安全组信息中去
//输入旧g_openstcak_security_list  和  新g_openstack_security_list_temp
INT1 update_From_NewSecurityGroup_To_OldSecurityGroup(openstack_security_p NewSecurityGroupList,openstack_security_p OldSecurityGroupList)
{
	openstack_security_p Temp_NewSecurityGroupList =NewSecurityGroupList;
	openstack_security_p Temp_OldSecurityGroupList =OldSecurityGroupList;
	openstack_security_p Temp_PreSecurityGroup =NULL;
	openstack_security_p Temp_AddedSecurityGroup =NULL;
	openstack_security_p Temp_NeedDeletedSecurityGroup =NULL;
	openstack_security_rule_p Temp_NeedDeletedSecurityRule =NULL;
	
	UINT1 SecurityGroup_Matched = 0; 
	//LOG_PROC("INFO","-----------------------%s",FN);
	//遍历新的GroupList//查找新增的
	while(Temp_NewSecurityGroupList)
	{
		while(Temp_OldSecurityGroupList)
		{
			if(0==(strcmp(Temp_NewSecurityGroupList->security_group,Temp_OldSecurityGroupList->security_group)))
			{//找到安全组中一致的规则
				SecurityGroup_Matched =1;
				break;
			}
			Temp_OldSecurityGroupList =Temp_OldSecurityGroupList->next;
		}	
		if(SecurityGroup_Matched)
		{//新安全组列表中此项在旧安全组列表中存在//删除旧group指向的各个rule//将旧group的规则项指向新的rule
			//删除旧项//TBD//NEED_CHECK
			Temp_NeedDeletedSecurityRule = Temp_OldSecurityGroupList->security_rule_p;
			while(Temp_NeedDeletedSecurityRule)
			{
				openstack_security_rule_p ReadyToDeleted =Temp_NeedDeletedSecurityRule;
				Temp_NeedDeletedSecurityRule =Temp_NeedDeletedSecurityRule ->next;
				mem_free(g_openstack_security_rule_id,ReadyToDeleted);
			}
			//指向新项//TBD//NEED_CHECK
			Temp_OldSecurityGroupList->security_rule_p = Temp_NewSecurityGroupList->security_rule_p;
		}
		else
		{//新安全组列表中此项在旧安全组列表中不存在//添加安全组条目
			///TBD//NEED_CHECK
			Temp_AddedSecurityGroup = update_openstack_security_group(Temp_NewSecurityGroupList->security_group);
			if (Temp_AddedSecurityGroup) 
			{
				Temp_AddedSecurityGroup->security_rule_p = Temp_NewSecurityGroupList->security_rule_p;
			}
		}
		Temp_OldSecurityGroupList =OldSecurityGroupList;	
		SecurityGroup_Matched = 0; 
		Temp_NewSecurityGroupList =Temp_NewSecurityGroupList->next;
	}
	
	//RESET
	Temp_NewSecurityGroupList =NewSecurityGroupList;
	Temp_OldSecurityGroupList =OldSecurityGroupList;
	SecurityGroup_Matched = 0; 
	
	//遍历旧的GroupList//查找删除的
	while(Temp_OldSecurityGroupList)
	{
		while(Temp_NewSecurityGroupList)
		{
			if(0==(strcmp(Temp_NewSecurityGroupList->security_group,Temp_OldSecurityGroupList->security_group)))
			{//找到安全组中一致的规则
				SecurityGroup_Matched =1;
				break;
			}
			Temp_NewSecurityGroupList =Temp_NewSecurityGroupList->next;
		}	
		if(SecurityGroup_Matched)
		{//旧安全组列表中此项在新安全组列表中存在
			//上面已处理
		}
		else
		{//旧安全组列表中此项在新安全组列表中不存在//删除旧安全组条目
			///TBD
			Temp_NeedDeletedSecurityGroup = Temp_OldSecurityGroupList;
			Temp_NeedDeletedSecurityRule = Temp_OldSecurityGroupList->security_rule_p;
			
			if(OldSecurityGroupList == Temp_NeedDeletedSecurityGroup)
			{//是第一个
				OldSecurityGroupList = Temp_NeedDeletedSecurityGroup->next;
			}
			else
			{
				Temp_PreSecurityGroup->next = Temp_NeedDeletedSecurityGroup->next;
			}
			
			while(Temp_NeedDeletedSecurityRule)
			{
				openstack_security_rule_p ReadyToDeleted =Temp_NeedDeletedSecurityRule;
				Temp_NeedDeletedSecurityRule =Temp_NeedDeletedSecurityRule ->next;
				mem_free(g_openstack_security_rule_id,ReadyToDeleted);
			}
			
			mem_free(g_openstack_security_group_id,Temp_NeedDeletedSecurityGroup);	
		}
		//复位
		Temp_NewSecurityGroupList =NewSecurityGroupList;	
		SecurityGroup_Matched = 0; 
		//预存
		Temp_PreSecurityGroup =Temp_OldSecurityGroupList;
		Temp_OldSecurityGroupList =Temp_OldSecurityGroupList->next;
	}
}
INT1 Clear_g_openstack_security_list_temp()
{
	openstack_security_p temp_node =NULL;
	//LOG_PROC("INFO","-----------------------%s",FN);
	while(g_openstack_security_list_temp)
	{
		temp_node = g_openstack_security_list_temp;
		g_openstack_security_list_temp =g_openstack_security_list_temp->next;
		mem_free(g_openstack_security_group_id_temp,temp_node);
	}
	g_openstack_security_list_temp=NULL;
}

/*
INT1 update_From_NewSecurityGroupList_To_OldSecurityGroupList(openstack_security_p NewSecurityGroupList,openstack_security_p OldSecurityGroupList)
{
	openstack_security_p Temp_NewSecurityGroupList =NewSecurityGroupList;
	openstack_security_p Temp_OldSecurityGroupList =OldSecurityGroupList;
	
	UINT1 SecurityGroup_Matched = 0; 
	
	//遍历新的GroupList//查找新增的
	while(Temp_NewSecurityGroupList)
	{
		while(Temp_OldSecurityGroupList)
		{
			if(0==(strcmp(Temp_NewSecurityGroupList->security_group,Temp_OldSecurityGroupList->security_group)))
			{//找到安全组中一致的规则
				SecurityGroup_Matched =1;
				break;
			}
			Temp_OldSecurityGroupList =Temp_OldSecurityGroupList->next;
		}	
		if(SecurityGroup_Matched)
		{//新安全组列表中此项在旧安全组列表中存在//比对规则条目
			//TBD其实不用比较,直接将新Group指向规则链表的指针替换旧的即可
			
			update_From_NewSecurityRuleList_To_OldSecurityRuleList(
																   Temp_NewSecurityGroupList,
																   Temp_OldSecurityGroupList,
																   Temp_NewSecurityGroupList->security_rule_p,
																   Temp_OldSecurityGroupList->security_rule_p
															      );
			
		}
		else
		{//新安全组列表中此项在旧安全组列表中不存在//添加安全组条目
			///TBD
		}
		Temp_OldSecurityGroupList =OldSecurityGroupList;	
		SecurityGroup_Matched = 0; 
		Temp_NewSecurityGroupList =Temp_NewSecurityGroupList->next;
	}
	
	//RESET
	Temp_NewSecurityGroupList =NewSecurityGroupList;
	Temp_OldSecurityGroupList =OldSecurityGroupList;
	SecurityGroup_Matched = 0; 
	
	//遍历旧的GroupList//查找删除的
	while(Temp_OldSecurityGroupList)
	{
		while(Temp_NewSecurityGroupList)
		{
			if(0==(strcmp(Temp_NewSecurityGroupList->security_group,Temp_OldSecurityGroupList->security_group)))
			{//找到安全组中一致的规则
				SecurityGroup_Matched =1;
				break;
			}
			Temp_NewSecurityGroupList =Temp_NewSecurityGroupList->next;
		}	
		if(SecurityGroup_Matched)
		{//旧安全组列表中此项在新安全组列表中存在//比对规则条目
			///TBD 上面新比旧已处理
			
			update_From_NewSecurityRuleList_To_OldSecurityRuleList(
																   Temp_NewSecurityGroupList,
																   Temp_OldSecurityGroupList,
																   Temp_NewSecurityGroupList->security_rule_p,
																   Temp_OldSecurityGroupList->security_rule_p
																  );
			
		}
		else
		{//旧安全组列表中此项在新安全组列表中不存在//删除安全组条目
			///TBD
		}
		Temp_NewSecurityGroupList =NewSecurityGroupList;	
		SecurityGroup_Matched = 0; 
		Temp_OldSecurityGroupList =Temp_OldSecurityGroupList->next;
	}
}



INT1 update_From_NewSecurityRuleList_To_OldSecurityRuleList(openstack_security_p NewSecurityGroupList,openstack_security_p OldSecurityGroupList,openstack_security_rule_p NewSecurityRuleList ,openstack_security_rule_p OldSecurityRuleList)
{
	openstack_security_rule_p Temp_NewSecurityRuleList =NewSecurityRuleList;
	openstack_security_rule_p Temp_OldSecurityRuleList =OldSecurityRuleList;
	openstack_security_rule_p Temp_OldSecurityRuleList_Pre =NULL;
	openstack_security_rule_p Temp_OldSecurityRuleList_Next =NULL;
	
	openstack_security_rule_p rule_node =NULL;
	
	UINT1 SecurityRule_Matched 	= 0; 
 
	//遍历新的RuleList//查找新增的
	while(Temp_NewSecurityRuleList)
	{
		while(Temp_OldSecurityRuleList)
		{
			if(0==(strcmp(Temp_NewSecurityRuleList->rule_id,Temp_OldSecurityRuleList->rule_id)))
			{//找到安全组中一致的规则
				SecurityRule_Matched =1;
				break;
			}
			Temp_OldSecurityRuleList =Temp_OldSecurityRuleList->next;
		}	
		if(SecurityRule_Matched)
		{//新安全规则列表中此项在旧安全规则列表中存在
			//不处理		
		}
		else
		{//新安全规则列表中此项在旧安全规则列表中不存在
			//TBD
			//新建这条rule
			rule_node = create_security_rule(
											 Temp_NewSecurityRuleList->group_id, 
											 Temp_NewSecurityRuleList->rule_id, 
											 Temp_NewSecurityRuleList->direction, 
											 Temp_NewSecurityRuleList->ethertype, 
											 itoa(Temp_NewSecurityRuleList->port_range_max), 
											 itoa(Temp_NewSecurityRuleList->port_range_min), 
											 Temp_NewSecurityRuleList->protocol,
											 Temp_NewSecurityRuleList->remote_group_id, 
											 Temp_NewSecurityRuleList->remote_ip_prefix, 
											 Temp_NewSecurityRuleList->tenant_id
											);
			//将这条rule加到旧的安全组中
			if(rule_node) 
			{
				rule_node->check_status = (UINT2)CHECK_CREATE;
				add_security_rule_into_group(group_id, rule_node);
			}
		}
		
		Temp_OldSecurityRuleList =OldSecurityRuleList;	
		SecurityRule_Matched = 0; 
		Temp_NewSecurityRuleList =Temp_NewSecurityRuleList->next;
	}
	
	//reset
	Temp_NewSecurityRuleList =NewSecurityRuleList;
	Temp_OldSecurityRuleList =OldSecurityRuleList;
	SecurityRule_Matched 	= 0; 
	
	//遍历旧的RuleList//查找删除的
	while(Temp_OldSecurityRuleList)
	{
		Temp_OldSecurityRuleList_Next = Temp_OldSecurityRuleList->next;//TBD//needcheck
		while(Temp_NewSecurityRuleList)
		{
			if(0==(strcmp(Temp_NewSecurityRuleList->rule_id,Temp_OldSecurityRuleList->rule_id)))
			{//找到安全组中一致的规则
				SecurityRule_Matched =1;
				break;
			}
			Temp_NewSecurityRuleList =Temp_NewSecurityRuleList->next;
		}	
		if(SecurityRule_Matched)
		{//旧安全规则列表中此项在新安全规则列表中存在
			//不处理
		}
		else
		{//旧安全规则列表中此项在新安全规则列表中不存在//删除安全规则条目
			//TBD//needcheck
			if(NULL == Temp_OldSecurityRuleList_Pre)
			{
				OldSecurityGroupList->security_rule_p =Temp_OldSecurityRuleList->next;
			}
			else
			{
				Temp_OldSecurityRuleList_Pre ->next =Temp_OldSecurityRuleList->next;
			}
			mem_free(g_openstack_security_rule_id,Temp_OldSecurityRuleList);
			SecurityRule_Deleted =1;
		}
		
		Temp_NewSecurityRuleList =NewSecurityRuleList;	
		SecurityRule_Matched = 0; 
		Temp_OldSecurityRuleList_Pre=Temp_OldSecurityRuleList;
		Temp_OldSecurityRuleList =Temp_OldSecurityRuleList_Next;
	}
}
*/

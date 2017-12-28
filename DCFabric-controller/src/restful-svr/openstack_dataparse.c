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
 *
 *  Created on: june 20, 2017
 *  Author: yangcaiyuan
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: june 20, 2017
 */


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "gnflush-types.h"
#include "openstack_host.h"
#include "openstack_app.h"
#include "fabric_openstack_external.h"
#include "fabric_openstack_nat.h"
#include "fabric_thread.h"
#include "openstack_lbaas_app.h"
#include "timer.h"
#include "openstack_routers.h"
#include "../conn-svr/conn-svr.h"
#include "event.h"
#include "openstack-server.h"
#include "openstack_clbaas_app.h"
#include "openstack_dataparse.h"
#include "openstack_qos_app.h"
#include "fabric_openstack_gateway.h"




extern UINT1 Openstack_Info_Ready_flag;

INT4 createOpenstackQOS(char* jsonString, void* param)
{
    
}

INT4 createOpenstackNetwork(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	json_t *json=NULL,*temp=NULL;
	
	char tenant_id[OPENSTACK_STRINGLEN] ={0};
	char network_id[OPENSTACK_STRINGLEN] = {0};
	
	INT4 parse_type = 0;
	INT4 totalNum = 0;
	UINT1 shared=0;
	UINT1 external=0;
 
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))
	{
		LOG_PROC("ERROR", "%s failed tempString=%s", FN,tempString);
		return GN_ERR;
	}
	json_t *networks = json_find_first_label(json, "networks");
	if(networks)
	{
		json_t *network  = networks->child->child;
		while(network)
		{
			temp = json_find_first_label(network, "tenant_id");
			if(temp)
			{
				strcpy(tenant_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(network, "id");
			if(temp)
			{
				strcpy(network_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(network, "shared");
			if(temp)
			{
				if(temp->child->type==JSON_TRUE)
				{
					shared=1;
				}
				else if(temp->child->type==JSON_FALSE)
				{
					shared=0;
				}
				else
				{
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(network, "router:external");
			if(temp)
			{
				if(temp->child->type==JSON_TRUE)
				{
					external=1;
				}
				else if(temp->child->type==JSON_FALSE)
				{
					external=0;
				}
				else
				{
				}
				json_free_value(&temp);
			}
			openstack_network_p network_p = update_openstack_app_network(tenant_id,network_id,shared,external);
			if ((network_p) && (is_check_status_changed(network_p->check_status))) 
			{
				totalNum ++;
			}
			network=network->next;
		}
		json_free_value(&networks);
	}
	json_free_value_all(&json);
	return GN_OK;

}
INT4 createOpenstackRouter(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	json_t *json=NULL,*temp=NULL;
	
	char network_id[OPENSTACK_STRINGLEN] = {0};
	char routers_id[OPENSTACK_STRINGLEN*3] = {0};
	char subnet_id[OPENSTACK_STRINGLEN] = {0};
    char routers_external_ip[OPENSTACK_STRINGLEN] = {0};
	UINT1 ipv6addr[16] = {0};
	UINT4 ipv4addr = 0;
	INT4 parse_type = 0;

	
	parse_type = json_parse_document(&json,tempString);
	
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return GN_ERR;
	}
	json_t *routers = json_find_first_label(json, "routers");
	if(routers)
	{
		reset_openstack_router_status();
		json_t * pRouter  = routers->child->child;
		while(pRouter)
		{
			memset(routers_id, 0, sizeof(routers_id));
			memset(subnet_id, 0,  sizeof(subnet_id));

			temp = json_find_first_label(pRouter, "id");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(routers_id,temp->child->text);
				}
				json_free_value(&temp);
			}

			json_t *  pGateWayInfo  = json_find_first_label(pRouter, "external_gateway_info");
            if(pGateWayInfo &&pGateWayInfo->child)
            {
                if(pGateWayInfo->child && JSON_OBJECT == pGateWayInfo->child->type)
                {
					json_t *fix_ips = json_find_first_label(pGateWayInfo->child, "external_fixed_ips");
					if(fix_ips&&fix_ips->child)
					{
						json_t *fix_ip = fix_ips->child->child;
						if(fix_ip)
						{
							temp = json_find_first_label(fix_ip, "subnet_id");
							if(temp)
							{
								strcpy(subnet_id,temp->child->text);
								json_free_value(&temp);
							}
							temp = json_find_first_label(fix_ip, "ip_address");
							if(temp){
								memset(ipv6addr, 0, 16);
								ipv4addr = 0;
								if(temp->child->text)
								{
									if (strchr(temp->child->text, ':')) 
									{
										ipv6_str_to_number(temp->child->text, ipv6addr);
									}
									else 
									{
										ipv4addr = inet_addr(temp->child->text) ;
									}
								}
								json_free_value(&temp);
							}
						}
						json_free_value(&fix_ips);
					}
                    
                }
				json_free_value(&pGateWayInfo);
            }
			
			update_openstack_router_list(&g_openstack_router_list, routers_id, ipv4addr, network_id);
			if(('\0' != routers_id[0])&& ('\0' !=subnet_id[0]))
			{
				update_openstack_clbaas_subnet_by_routerrest(routers_id, subnet_id);
			}
			pRouter=pRouter->next;
		}
		
		json_free_value(&routers);
		visit_openstack_router_list_find_unvalid();
	}
	json_free_value_all(&json);
	return GN_OK;
}
INT4 createOpenstackSubnet(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	json_t *json=NULL,*temp=NULL;
	char tenant_id[OPENSTACK_STRINGLEN] ={0};
	char network_id[OPENSTACK_STRINGLEN] = {0};
	char subnet_id[OPENSTACK_STRINGLEN] = {0};
	char cidr[30] = {0};
	INT4 parse_type = 0;
	INT4 totalNum = 0;
	INT4 gateway_ip = 0, start_ip = 0, end_ip = 0;
	UINT1 gateway_ipv6[16] = {0};
	UINT1 start_ipv6[16] = {0};
	UINT1 end_ipv6[16] = {0};
	UINT4 longtemp = 0;

	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return GN_ERR;
	}
	json_t *subnets = json_find_first_label(json, "subnets");
	if(subnets)
	{
		json_t *subnet  = subnets->child->child;
		while(subnet)
		{
			temp = json_find_first_label(subnet, "tenant_id");
			if(temp)
			{
				strcpy(tenant_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(subnet, "network_id");
			if(temp)
			{
				strcpy(network_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(subnet, "id");
			if(temp)
			{
				strcpy(subnet_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(subnet, "cidr");
			if(temp)
			{
				strcpy(cidr,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(subnet, "gateway_ip");
			if(temp)
			{
				memset(gateway_ipv6, 0, 16);
				gateway_ip = 0;
				if(temp->child->text)
				{
					if (strchr(temp->child->text, ':')) 
					{
						ipv6_str_to_number(temp->child->text, gateway_ipv6);
					}
					else 
					{
						longtemp = inet_addr(temp->child->text) ;
						gateway_ip = longtemp;
					}
				}
				json_free_value(&temp);
			}
			json_t *allocations = json_find_first_label(subnet, "allocation_pools");
			if(allocations)
			{
				json_t *allocation = allocations->child->child;
				while(allocation)
				{
					temp = json_find_first_label(allocation, "start");
					if(temp)
					{
						memset(start_ipv6, 0, 16);
						start_ip = 0;
						if(temp->child->text)
						{
							if (strchr(temp->child->text, ':')) 
							{
								ipv6_str_to_number(temp->child->text, start_ipv6);
							}
							else 
							{
								longtemp = inet_addr(temp->child->text) ;
								start_ip = longtemp;
							}
						}
						json_free_value(&temp);
					}
					temp = json_find_first_label(allocation, "end");
					if(temp)
					{
						memset(end_ipv6, 0, 16);
						end_ip = 0;
						if(temp->child->text)
						{
							if (strchr(temp->child->text, ':')) 
							{
								ipv6_str_to_number(temp->child->text, end_ipv6);
							}
							else 
							{
								longtemp = inet_addr(temp->child->text) ;
								end_ip = longtemp;
							}
						}
						json_free_value(&temp);
					}
					allocation=allocation->next;
				}
				json_free_value(&allocations);
			}
			openstack_subnet_p subnet_p = update_openstack_app_subnet(tenant_id,network_id,subnet_id,
				                                                      gateway_ip,start_ip,end_ip,	
																	  gateway_ipv6, start_ipv6, end_ipv6, cidr);
			if ((subnet_p) && (is_check_status_changed(subnet_p->check_status))) 
			{
				totalNum ++;
			}
			subnet=subnet->next;
		}
		json_free_value(&subnets);
	}
	json_free_value_all(&json);
	return GN_OK;
}
INT4 createOpenstackPort(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	json_t *json=NULL,*temp=NULL;
	char tenant_id[OPENSTACK_STRINGLEN] ={0};
	char network_id[OPENSTACK_STRINGLEN] = {0};
	char subnet_id[OPENSTACK_STRINGLEN] = {0};
	char port_id[OPENSTACK_STRINGLEN] = {0};
	char device_id[48]={0};
	char port_status[48]={0};
	char qos_id[48] = {0};
	INT4 parse_type = 0;
	char port_type[40] = {0};
	char* computer="compute:nova";
	char* computer_none="compute:None";
	char* dhcp="network:dhcp";
	char* floating = "network:floatingip";
	//char* clbcomputer = "network:LOADBALANCER_VIP";
	INT4 ip = 0;
	UINT1 ipv6[16] = {0};
	UINT1 mac[6]={0};
	UINT4 port_number = 0;
	UINT1 type = 0;
	UINT2 security_num = 0;
	UINT2 security_num_temp = 0;
	UINT1* security_port_p = NULL;
	UINT1* security_port_p_temp = NULL;
	INT4 totalNum = 0;

	parse_type = json_parse_document(&json,tempString);
	
	//LOG_PROC("INFO", "%s tempString=%s", FN,tempString);
	
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	openstack_qos_binding_p 	S_openstack_qos_router_binding_List 		=NULL;
	openstack_qos_InitBindingList(&S_openstack_qos_router_binding_List); 
	openstack_qos_binding_p 	S_openstack_qos_loadbalancer_binding_List 		=NULL;
	openstack_qos_InitBindingList(&S_openstack_qos_loadbalancer_binding_List); 
	
	json_t *ports = json_find_first_label(json, "ports");
	if(ports){
		json_t *port  = ports->child->child;
		while(port){
			strcpy(qos_id,"");
			json_t *qoses = json_find_first_label(port, "qos");
			if(qoses)
			{
				json_t *qos = qoses->child->child;
				while(qos)
				{
					strcpy(qos_id,qos->text);
					qos =qos->next;
				}
				json_free_value(&qoses);
			}
			temp = json_find_first_label(port, "tenant_id");
			if(temp){
				strcpy(tenant_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "status");
			if(temp){
				strcpy(port_status,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "network_id");
			if(temp){
				strcpy(network_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "id");
			if(temp){
				strcpy(port_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "device_id");
			if(temp){
				strcpy(device_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "device_owner");
			if(temp){
				strcpy(port_type,temp->child->text);
				type = get_openstack_port_type(port_type);
				//port_type = temp->child->text;
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "mac_address");
			if(temp){
				macstr2hex(temp->child->text,mac);
				json_free_value(&temp);
			}
			json_t *fix_ips = json_find_first_label(port, "fixed_ips");
			if(fix_ips){//ip = temp->child->text;
				json_t *fix_ip = fix_ips->child->child;
				while(fix_ip){
					temp = json_find_first_label(fix_ip, "subnet_id");
					if(temp){
						strcpy(subnet_id,temp->child->text);
						json_free_value(&temp);
					}
					temp = json_find_first_label(fix_ip, "ip_address");
					if(temp){
						memset(ipv6, 0, 16);
						ip = 0;
						if(temp->child->text)
						{
							if (strchr(temp->child->text, ':')) {
								// printf("ipv6: %s\n", temp->child->text);
								ipv6_str_to_number(temp->child->text, ipv6);
								//nat_show_ipv6(ipv6);
							}
							else {
								UINT4 longtemp = inet_addr(temp->child->text) ;
								ip = longtemp;
							}
						}
						json_free_value(&temp);
					}
					fix_ip=fix_ip->next;
				}
				json_free_value(&fix_ips);
			}

			json_t *security_groups = json_find_first_label(port, "security_groups");
			if (security_groups) 
			{
				json_t *security_group = security_groups->child->child;
				openstack_node_p head_p = NULL;
				security_num = 0;
				security_port_p = NULL;
				
				openstack_node_p head_p_temp = NULL;
				security_num_temp = 0;
				security_port_p_temp = NULL;
				while (security_group) 
				{
					char * SecurityGroupID =(char *)malloc(48*sizeof(char));
					memset(SecurityGroupID,0,48);
					strcpy(SecurityGroupID,security_group->text);
	
					head_p = add_openstack_host_security_node((UINT1*)SecurityGroupID, head_p);
					security_port_p = (UINT1*)head_p;
					security_num++;	
					
					security_group = security_group->next;
				}
				json_free_value(&security_groups);
			}

			p_fabric_host_node host_p = NULL;
			if(strcmp(port_type,computer)==0|| strcmp(port_type,floating)==0 || strcmp(port_type,computer_none)==0)
			{
				host_p = update_openstack_app_host_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,security_num,security_port_p);
			}
			else if(strcmp(port_type,dhcp)==0)
			{
				//LOG_PROC("INFO","DHCP UPDATE!");
				host_p = update_openstack_app_dhcp_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id);
				clear_openstack_host_security_node(security_port_p);
				clear_openstack_host_security_node(security_port_p_temp);
			}
			else
			{
				//LOG_PROC("INFO","GATEWAY UPDATE!");
				host_p = update_openstack_app_gateway_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id);
				//clear_openstack_host_security_node(security_port_p);
				//clear_openstack_host_security_node(security_port_p_temp);
				if(strcmp(port_type,"network:router_gateway")==0)
				{
					//????·?????????????
					openstack_port_p port_p = (openstack_port_p)host_p->data;
					port_p->security_num = security_num;
					clear_openstack_host_security_node(port_p->security_data);
					port_p->security_data = security_port_p;	
					//????·??????QOS???
					openstack_qos_C_binding(&S_openstack_qos_router_binding_List,port_id,qos_id,tenant_id,port_type,network_id,device_id,ip);
					//LOG_PROC("INFO", "%s_%d_%p",FN,LN,S_openstack_qos_router_binding_List);
				}
				else if((strcmp(port_type,"network:LOADBALANCER_VIP")==0)&&(strcmp(port_status,"DOWN")==0))
				{
					//???????????????????
					openstack_port_p port_p = (openstack_port_p)host_p->data;
					port_p->security_num = security_num;
					clear_openstack_host_security_node(port_p->security_data);
					port_p->security_data = security_port_p;
					//????????????QOS???
					openstack_qos_C_binding(&S_openstack_qos_loadbalancer_binding_List,port_id,qos_id,tenant_id,port_type,network_id,device_id,ip);
					//LOG_PROC("INFO", "%s_%d_%p",FN,LN,S_openstack_qos_loadbalancer_binding_List);
				}
				else
				{
					clear_openstack_host_security_node(security_port_p);
					clear_openstack_host_security_node(security_port_p_temp);
				}
			}

			if ((host_p) && (is_check_status_changed(host_p->check_status))) 
			{
				totalNum++;
			}
			if((OPENSTACK_PORT_TYPE_GATEWAY == type)||(OPENSTACK_PORT_TYPE_ROUTER_INTERFACE == type))
			{
				update_openstack_router_outerinterface(device_id, network_id, subnet_id, ip, type);
			}
			
			port=port->next;
		}
		json_free_value(&ports);
		if(Openstack_Info_Ready_flag)
		{
			openstack_qos_Compare_binding(&S_openstack_qos_router_binding_List,&G_openstack_qos_router_binding_List);
			openstack_qos_Compare_binding(&S_openstack_qos_loadbalancer_binding_List,&G_openstack_qos_loadbalancer_binding_List);
		}
	}
	json_free_value_all(&json);
	if(NULL != S_openstack_qos_router_binding_List)
	{
		openstack_qos_D_binding_list(S_openstack_qos_router_binding_List);
		S_openstack_qos_router_binding_List = NULL;
	}
	if(NULL != S_openstack_qos_loadbalancer_binding_List)
	{
		openstack_qos_D_binding_list(S_openstack_qos_loadbalancer_binding_List);		
		S_openstack_qos_loadbalancer_binding_List = NULL;
	}
	return GN_OK;
}
INT4 createOpenstackFloating(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	json_t *json=NULL,*temp=NULL;
	INT4 parse_type = 0;
	char router_id[128] = {0};
	char port_id[OPENSTACK_STRINGLEN] = {0};
	UINT4 fixed_ip;//inner ip
	UINT4 floating_ip;//outer ip
	INT4 totalNum = 0;
	char tenant_id[48] ={0};
	char id[48] = {0};
	char qos_id[48] = {0};
	parse_type = json_parse_document(&json,tempString);
	
	//LOG_PROC("INFO", "%s tempString=%s", FN,tempString);
	
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	json_t *floatings = json_find_first_label(json, "floatingips");
	openstack_qos_binding_p 	S_openstack_qos_floating_binding_List 		=NULL;
	openstack_qos_InitBindingList(&S_openstack_qos_floating_binding_List); 
	if(floatings)
	{
		json_t *floating_ip_one  = floatings->child->child;
		while(floating_ip_one)
		{
			fixed_ip = 0;
			floating_ip = 0;
			bzero(router_id, 48);
			bzero(port_id, 48);
			
			temp = json_find_first_label(floating_ip_one, "id");
			if(temp)
			{
				strcpy(id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(floating_ip_one, "qos");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(qos_id,temp->child->text);
					json_free_value(&temp);
				}
			}
			temp = json_find_first_label(floating_ip_one, "tenant_id");
			if(temp)
			{
				strcpy(tenant_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(floating_ip_one, "router_id");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(router_id,temp->child->text);
					json_free_value(&temp);
				}
			}
			temp = json_find_first_label(floating_ip_one, "port_id");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(port_id,temp->child->text);
					json_free_value(&temp);
				}
			}
			temp = json_find_first_label(floating_ip_one, "fixed_ip_address");
			if(temp)
			{
				if(temp->child->text)
				{
					fixed_ip = inet_addr(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(floating_ip_one, "floating_ip_address");
			if(temp)
			{
				if(temp->child->text)
				{
					floating_ip = inet_addr(temp->child->text);
				}
				json_free_value(&temp);
			}
			external_floating_ip_p efp = create_floatting_ip_by_rest(fixed_ip,floating_ip,port_id,router_id, floatingtype_General);
			if ((efp) && (is_check_status_changed(efp->check_status))) 
			{
				totalNum++;
			}
			floating_ip_one=floating_ip_one->next;
			if(('\0' != id[0]) &&('\0' != router_id[0]) && floating_ip)
			{
				openstack_qos_C_binding(&S_openstack_qos_floating_binding_List,id,qos_id,tenant_id,"network:floatingip","",router_id,floating_ip);
			}
			//LOG_PROC("INFO", "%s_%d_%p",FN,LN,S_openstack_qos_floating_binding_List);
		}
		if(Openstack_Info_Ready_flag)
		{
			openstack_qos_Compare_binding(&S_openstack_qos_floating_binding_List,& G_openstack_qos_floating_binding_List);
		}
		json_free_value(&floatings);
	}
	json_free_value_all(&json);
	if(NULL != S_openstack_qos_floating_binding_List)
	{
		openstack_qos_D_binding_list(S_openstack_qos_floating_binding_List);
		S_openstack_qos_floating_binding_List = NULL;
	}
	return GN_OK;
}
INT4 createOpenstackSecurity(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	json_t *json=NULL,*temp=NULL;
	INT4 parse_type = 0;
	char security_group_id[OPENSTACK_STRINGLEN]={0};
	char direction[OPENSTACK_STRINGLEN] = {0};
	char ethertype[OPENSTACK_STRINGLEN] = {0};
	char rule_id[OPENSTACK_STRINGLEN] = {0};
	char port_range_max[OPENSTACK_STRINGLEN] = {0};
	char port_range_min[OPENSTACK_STRINGLEN] = {0};
	char protocol[OPENSTACK_STRINGLEN] = {0};
	char remote_group_id[OPENSTACK_STRINGLEN] = {0};
	char remote_ip_prefix[OPENSTACK_STRINGLEN] = {0};
	char security_tenant_id[OPENSTACK_STRINGLEN] = {0};
	UINT2 priority =0;
	UINT1 enabled =0;
	//LOG_PROC("INFO", "%s:%s", FN,jsonString);
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	
	json_t *security_groups = json_find_first_label(json, "security_group_rules");
	if(security_groups)
	{
		json_t *security_group  = security_groups->child->child;
		while(security_group)
		{
			temp = json_find_first_label(security_group, "security_group_id");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(security_group_id,temp->child->text);	
				}
				json_free_value(&temp);
			}
			strcpy(direction,"");
			temp = json_find_first_label(security_group, "direction");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(direction,temp->child->text);
				}
				json_free_value(&temp);
			}
			strcpy(ethertype,"");
			temp = json_find_first_label(security_group, "ethertype");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(ethertype,temp->child->text);	
				}
				json_free_value(&temp);
			}
			strcpy(rule_id,"");
			temp = json_find_first_label(security_group, "id");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(rule_id, temp->child->text);
				}
				json_free_value(&temp);
			}
			strcpy(port_range_max,"");
			temp = json_find_first_label(security_group, "port_range_max");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(port_range_max, temp->child->text);
				}
				json_free_value(&temp);
			}
			strcpy(port_range_min,"");
			temp = json_find_first_label(security_group, "port_range_min");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(port_range_min, temp->child->text);
				}
				json_free_value(&temp);
			}
			strcpy(protocol,"");
			temp = json_find_first_label(security_group, "protocol");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(protocol, temp->child->text);
				}
				json_free_value(&temp);
			}
			strcpy(remote_group_id,"");
			temp = json_find_first_label(security_group, "remote_group_id");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(remote_group_id, temp->child->text);
				}
				json_free_value(&temp);
			}
			strcpy(remote_ip_prefix,"");
			temp = json_find_first_label(security_group, "remote_ip_prefix");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(remote_ip_prefix, temp->child->text);
				}
				json_free_value(&temp);
			}
			strcpy(security_tenant_id,"");
			temp = json_find_first_label(security_group, "tenant_id");
			if(temp)
			{
				if(temp->child->text)
				{
					strcpy(security_tenant_id, temp->child->text);
				}
				json_free_value(&temp);
			}
			
			temp = json_find_first_label(security_group, "priority");
			if(temp)
			{
				if(temp->child->text)
				{
					priority = atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			
			temp = json_find_first_label(security_group, "enabled");
			if(temp)
			{
				if(temp->child->type)
				{
					enabled =  (JSON_TRUE == temp->child->type)?1:0;
				}
				json_free_value(&temp);
			}
			
			update_security_rule(security_group_id,rule_id,direction,ethertype,port_range_max,port_range_min,protocol,remote_group_id,remote_ip_prefix,security_tenant_id,priority,enabled);
			security_group = security_group->next;
		}
		json_free_value(&security_groups);
	}
	json_free_value_all(&json);
	return GN_OK;
}
INT4 createOpenstackPortforward(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	port_forward_param_p forwardParam = NULL;
	char forward_status[OPENSTACK_STRINGLEN] = {0};
	char forward_in_addr[OPENSTACK_STRINGLEN] = {0};
	char forward_protol[OPENSTACK_STRINGLEN] = {0};
	char forward_in_port[OPENSTACK_STRINGLEN] = {0};
	char forward_outside_port[OPENSTACK_STRINGLEN] = {0};
	char network_id[OPENSTACK_STRINGLEN] = {0};
	char external_ip[OPENSTACK_STRINGLEN] = {0};

	forwardParam = (port_forward_param_p)param;
    parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json)||(NULL == forwardParam))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return GN_ERR;
	}
	json_t *port_forward = json_find_first_label(json, "routers");
	if(port_forward)
	{
		if(port_forward->child&&port_forward->child)
		{
			json_t* portForwardObj = port_forward->child->child;
            while(portForwardObj)
            {
                json_t *  pGateWayInfo  = json_find_first_label(portForwardObj, "external_gateway_info");
                if(pGateWayInfo &&pGateWayInfo->child)
                {
                    if(pGateWayInfo->child && JSON_OBJECT == pGateWayInfo->child->type)
                    {
                        temp = json_find_first_label(pGateWayInfo->child, "network_id");
                        if(temp)
                        {
							if(temp->child && temp->child->text)
	                        {
	                            strcpy(network_id, temp->child->text);
	                        }
							 json_free_value(&temp);
                        }

                        temp = json_find_first_label(pGateWayInfo->child, "external_fixed_ips");
                        if(temp)
						{
							if(temp->child && temp->child->child)
                        	{
	                            json_t* extIpObj = json_find_first_label(temp->child->child, "ip_address");
	                            if(extIpObj)
	                            {
	                                strcpy(external_ip, extIpObj->child->text);
	                                json_free_value(&extIpObj);
	                            }
							}

                            json_free_value(&temp);
                        }
                    }
                    json_free_value(&pGateWayInfo);
                }

				json_t *  portforwardings  = json_find_first_label(portForwardObj, "portforwardings");
				if(portforwardings && portforwardings->child && portforwardings->child->child )
				{
					json_t* pPortForwardObj = portforwardings->child->child; 

					while(pPortForwardObj)
					{
						memset(forward_status, 0, sizeof(forward_status));
						memset(forward_in_addr, 0, sizeof(forward_in_addr));
						memset(forward_protol, 0, sizeof(forward_protol));
						memset(forward_in_port, 0, sizeof(forward_in_port));
						memset(forward_outside_port, 0, sizeof(forward_outside_port));

						temp = json_find_first_label(pPortForwardObj, "status");
						if(temp)
						{
							if(temp->child && temp->child->text)
							{
								strcpy(forward_status, temp->child->text);
							}
							json_free_value(&temp);
						}

						temp = json_find_first_label(pPortForwardObj, "inside_addr");
						if(temp)
						{
							if(temp->child && temp->child->text)
							{
								strcpy(forward_in_addr, temp->child->text);
							}
							json_free_value(&temp);
						}

						temp = json_find_first_label(pPortForwardObj, "protocol");
						if(temp)
						{
							if(temp->child && temp->child->text)
							{
								strcpy(forward_protol, temp->child->text);
							}
							json_free_value(&temp);
						}

						temp = json_find_first_label(pPortForwardObj, "inside_port");
						if(temp)
						{
							if(temp->child && temp->child->text)
							{
								strcpy(forward_in_port, temp->child->text);
								
							}
							json_free_value(&temp);
						}

						temp = json_find_first_label(pPortForwardObj, "outside_port");
						if(temp)
						{
							if(temp->child && temp->child->text)
							{
								strcpy(forward_outside_port, temp->child->text);
							}
							json_free_value(&temp);
						}

						update_opstack_portforward_list(&(forwardParam->list_header), forward_protol, forward_status, network_id,  external_ip, forward_in_addr,  forward_in_port, forward_outside_port);	

						pPortForwardObj = pPortForwardObj->next;
					}

					json_free_value(&portforwardings);
				}

				portForwardObj = portForwardObj->next;
			}
		} 

		json_free_value(&port_forward);
	}
	json_free_value_all(&json);
	return GN_OK;
}
INT4 createOpenstackLbaaspools(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char tenant_id[OPENSTACK_STRINGLEN] ={0};
	char lb_pool_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 lb_status=0;
	UINT1 lb_protocol=0;
	UINT1 lbaas_method=0;
	INT4 totalNum = 0;

	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	json_t *lbaas_pools = json_find_first_label(json, "pools");
	if(lbaas_pools){
		json_t *lbaas_pool  = lbaas_pools->child->child;
		while(lbaas_pool){
			temp = json_find_first_label(lbaas_pool, "tenant_id");
			strcpy(tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(tenant_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "id");
			strcpy(lb_pool_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_pool_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "status");
			if(temp){
				if(temp->child->text && strcmp(temp->child->text,"ACTIVE")==0){
					lb_status=LBAAS_OK;
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "protocol");
			if(temp){
				if(temp->child->text){
					if(strcmp(temp->child->text,"HTTP")==0){
						lb_protocol=LBAAS_PRO_HTTP;
					}else if(strcmp(temp->child->text,"HTTPS")==0){
						lb_protocol=LBAAS_PRO_HTTPS;
					}else if(strcmp(temp->child->text,"TCP")==0){
						lb_protocol=LBAAS_PRO_TCP;
					}
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "lb_method");
			if(temp){
				if(temp->child->text){
					if(strcmp(temp->child->text,"ROUNT_ROBIN")==0){
						lbaas_method=LB_M_ROUNT_ROBIN;
					}else if(strcmp(temp->child->text,"LEAST_CONNECTIONS")==0){
						lbaas_method=LB_M_LEAST_CONNECTIONS;
					}else if(strcmp(temp->child->text,"SOURCE_IP")==0){
						lbaas_method=LB_M_SOURCE_IP;
					}
				}
				json_free_value(&temp);
			}
			openstack_lbaas_pools_p pool_p = update_openstack_lbaas_pool_by_poolrest(
					tenant_id,
					lb_pool_id,
					lb_status,
					lb_protocol,
					lbaas_method);
			lbaas_pool = lbaas_pool->next;
			if ((pool_p) && (is_check_status_changed(pool_p->check_status)))
				totalNum++;
		}
		json_free_value(&lbaas_pools);
	}
	json_free_value_all(&json);
	return GN_OK;
}
INT4 createOpenstackLbaasvips(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char lb_pool_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 vip_status=0;
	UINT4 vip_ip_adress=0;
	UINT4 protocol_port=0;
	UINT1 connect_limit=0;
	UINT1 session_persistence=0;
	INT4 totalNum = 0;
	
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	json_t *lbaas_pools = json_find_first_label(json, "vips");
	if(lbaas_pools){
		json_t *lbaas_pool  = lbaas_pools->child->child;
		while(lbaas_pool){
			temp = json_find_first_label(lbaas_pool, "address");
			if(temp){
				if(temp->child->text){
					vip_ip_adress= inet_addr(temp->child->text) ;
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "pool_id");
			strcpy(lb_pool_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_pool_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "status");
			if(temp){
				if(temp->child->text && strcmp(temp->child->text,"ACTIVE")==0){
					vip_status=LBAAS_OK;
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "protocol_port");
			if(temp){
				if(temp->child->text){
					protocol_port=atoll(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "connection_limit");
			if(temp){
				if(temp->child->text){
					connect_limit=atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "session_persistence");
			if(temp){
				json_t *temp2 = temp->child->child;
				if(temp2){
					// temp2 = json_find_first_label(temp2, "type");
					if(temp2->child->text){
						if(strcmp(temp2->child->text,"SOURCE_IP")==0){
							session_persistence=SEPER_SOURCE_IP;
						}else if(strcmp(temp2->child->text,"HTTP_COOKIE")==0){
							session_persistence=SEPER_HTTP_COOKIE;
						}else if(strcmp(temp2->child->text,"APP_COOKIE")==0){
							session_persistence=SEPER_APP_COOKIE;
						}else{
							session_persistence= SEPER_NO_LIMIT;
						}
					}
					json_free_value(&temp2);
				}
				json_free_value(&temp);
			}
			openstack_lbaas_pools_p pool_p = update_openstack_lbaas_pool_by_viprest(
					lb_pool_id,
					protocol_port,
					vip_ip_adress,
					connect_limit,
					vip_status,session_persistence);
			if ((pool_p) && is_check_status_changed(pool_p->check_status))
				totalNum++;
			lbaas_pool = lbaas_pool->next;
		}
		json_free_value(&lbaas_pools);
	}
	json_free_value_all(&json);
	return GN_OK;
}
INT4 createOpenstackLbaasmember(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char lb_member_id[OPENSTACK_STRINGLEN] = {0};
	char lb_pool_id[OPENSTACK_STRINGLEN] = {0};
	char lb_tenant_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 mem_status=0;
	UINT1 mem_weight=0;
	UINT4 mem_protocol_port=0;
	UINT4 mem_fixed_ip=0;
	INT4 totalNum = 0;
	
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	json_t *lbaas_members = json_find_first_label(json, "members");
	if(lbaas_members){
		json_t *lbaas_member  = lbaas_members->child->child;
		while(lbaas_member){
			temp = json_find_first_label(lbaas_member, "address");
			if(temp){
				if(temp->child->text){
					mem_fixed_ip= inet_addr(temp->child->text) ;
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_member, "id");
			strcpy(lb_member_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_member_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_member, "pool_id");
			strcpy(lb_pool_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_pool_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_member, "tenant_id");
			strcpy(lb_tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_tenant_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_member, "status");
			if(temp){
				if(temp->child->text && strcmp(temp->child->text,"ACTIVE")==0){
					mem_status=LBAAS_OK;
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_member, "protocol_port");
			if(temp){
				if(temp->child->text){
					mem_protocol_port=atoll(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_member, "weight");
			if(temp){
				if(temp->child->text){
					mem_weight=atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			openstack_lbaas_members_p member_p = update_openstack_lbaas_member_by_rest(
					lb_member_id,
					lb_tenant_id,
					lb_pool_id,
					mem_weight,
					mem_protocol_port,
					mem_status,
					mem_fixed_ip);
			if ((member_p) && is_check_status_changed(member_p->check_status)) 
				totalNum++;
			lbaas_member = lbaas_member->next;
		}
		json_free_value(&lbaas_members);
	}
	json_free_value_all(&json);
	return GN_OK;
}
INT4 createOpenstackLbaaslistener(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char lb_listener_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 lb_listener_type=0;
	UINT4 check_frequency=0;
	UINT4 lb_lis_overtime=0;
	UINT1 lb_lis_retries=0;
	INT4 totalNum = 0;
	
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	json_t *lbaas_listeners = json_find_first_label(json, "health_monitors");
	if(lbaas_listeners){
		json_t *lbaas_listener  = lbaas_listeners->child->child;
		while(lbaas_listener){
			temp = json_find_first_label(lbaas_listener, "id");
			strcpy(lb_listener_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_listener_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "type");
			if(temp){
				if(temp->child->text && strcmp(temp->child->text,"PING")==0){
					lb_listener_type=LBAAS_LISTENER_PING;
				}else if(temp->child->text && strcmp(temp->child->text,"TCP")==0){
					lb_listener_type=LBAAS_LISTENER_TCP;
				}else if(temp->child->text && strcmp(temp->child->text,"HTTP")==0){
					lb_listener_type=LBAAS_LISTENER_HTTP;
				}else if(temp->child->text && strcmp(temp->child->text,"HTTPS")==0){
					lb_listener_type=LBAAS_LISTENER_HTTPS;
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "delay");
			if(temp){
				if(temp->child->text){
					check_frequency=atoll(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "timeout");
			if(temp){
				if(temp->child->text){
					lb_lis_overtime=atoll(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "max_retries");
			if(temp){
				if(temp->child->text){
					lb_lis_retries=atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			openstack_lbaas_listener_p listener_p = update_openstack_lbaas_listener_by_rest(
					lb_listener_id,
					lb_listener_type,
					check_frequency,
					lb_lis_overtime,
					lb_lis_retries);
			if ((listener_p) && is_check_status_changed(listener_p->check_status))
				totalNum++;
			lbaas_listener = lbaas_listener->next;
		}
		json_free_value(&lbaas_listeners);
	}
	json_free_value_all(&json);
	return GN_OK;
}
INT4 createOpenstackClbaaspools(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char tenant_id[OPENSTACK_STRINGLEN] ={0};
	char lb_pool_id[OPENSTACK_STRINGLEN] = {0};
	char lb_subnet_id[OPENSTACK_STRINGLEN] = {0};
	UINT4 max_conn = 0;
	//INT4 totalNum = 0;
	
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	
	json_t *lbaas_pools = json_find_first_label(json, "pools");
	if(lbaas_pools&&lbaas_pools->child){
		json_t *lbaas_pool = lbaas_pools->child->child;
		while(lbaas_pool){
			temp = json_find_first_label(lbaas_pool, "tenant_id");
			strcpy(tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(tenant_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "id");
			strcpy(lb_pool_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_pool_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "ha_subnet_id");
			strcpy(lb_subnet_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_subnet_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_pool, "maxconn");
			if(temp){
				if(temp->child->text){
					max_conn = atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			/*
			openstack_lbaas_pools_p pool_p = update_openstack_clbaas_pool_by_clbpoolrest(tenant_id,lb_pool_id, max_conn);
			if ((pool_p) && (is_check_status_changed(pool_p->check_status)))
				totalNum++;
			*/
			lbaas_pool = lbaas_pool->next;
		}
		json_free_value(&lbaas_pools);
	}
	json_free_value_all(&json);
	return GN_ERR;
}

INT4 createOpenstackClbaasvips(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char tenant_id[OPENSTACK_STRINGLEN] ={0};
	char subnet_id[OPENSTACK_STRINGLEN] = {0};
	char lb_pool_id[OPENSTACK_STRINGLEN] = {0};
	char lb_port_id[OPENSTACK_STRINGLEN] = {0};
	char lb_network_id[OPENSTACK_STRINGLEN] = {0};
	char lb_vip_id[OPENSTACK_STRINGLEN] = {0};
	char lb_router_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 ipv6addr[16] = {0};
	UINT4 ipv4addr = 0;
	//UINT4 fixed_ip = 0;
	//UINT4 floating_ip = 0;
	UINT1 isPublicType = 0;
	//INT4 totalNum = 0;
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	
	json_t *lbaas_vips = json_find_first_label(json, "vips");
	if(lbaas_vips&&lbaas_vips->child){
		json_t *lbaas_vip = lbaas_vips->child->child;
		while(lbaas_vip){
			temp = json_find_first_label(lbaas_vip, "is_public");
			if(temp)
			{
				if(temp->child->type){
					isPublicType =  (JSON_TRUE == temp->child->type)?1:0;
					
				}
				json_free_value(&temp);
			}
			
			
			temp = json_find_first_label(lbaas_vip, "tenant_id");
			strcpy(tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(tenant_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_vip, "id");
			strcpy(lb_vip_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_vip_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_vip, "network_id");
			strcpy(lb_network_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_network_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_vip, "pool_id");
			strcpy(lb_pool_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_pool_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_vip, "port_id");
			strcpy(lb_port_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_port_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}

			
			json_t *fix_ips = json_find_first_label(lbaas_vip, "fixed_ips");
			if(fix_ips&&fix_ips->child)
			{
				json_t *fix_ip = fix_ips->child->child;
				while(fix_ip){
					temp = json_find_first_label(fix_ip, "subnet_id");
					strcpy(subnet_id,"");
					if(temp){
						strcpy(subnet_id,temp->child->text);
						json_free_value(&temp);
					}
					temp = json_find_first_label(fix_ip, "ip_address");
					if(temp){
						memset(ipv6addr, 0, 16);
						ipv4addr = 0;
						if(temp->child->text)
						{
							if (strchr(temp->child->text, ':')) {
								// printf("ipv6: %s\n", temp->child->text);
								ipv6_str_to_number(temp->child->text, ipv6addr);
								//nat_show_ipv6(ipv6);
							}
							else {
								UINT4 longtemp = inet_addr(temp->child->text) ;
								ipv4addr = longtemp;
							}
						}
						json_free_value(&temp);
					}
					fix_ip=fix_ip->next;
				}
				json_free_value(&fix_ips);
			}

			if('\0' != subnet_id[0])
			{
				openstack_clbaas_routersubnet_p lb_subnet = find_openstack_clbaas_subnet_by_subnet_id(subnet_id);
				if(lb_subnet)
				{
					strcpy(lb_router_id, lb_subnet->router_id);
				}
				p_fabric_host_node hostNode=NULL;
				hostNode = get_fabric_host_from_list_by_ip(ipv4addr);
				if(hostNode)
				{
					//hostNode->type = isPublicType?OPENSTACK_PORT_TYPE_CLBLOADBALANCER:OPENSTACK_PORT_TYPE_HOST;
				}
			}
			
	


			if(lb_pool_id&&ipv4addr)
			{
				lb_vipfloating = update_openstack_clbaas_vipfloatingpool_by_vipsrest(lb_pool_id, lb_router_id, lb_port_id, ipv4addr, isPublicType);
				if(lb_vipfloating&&lb_vipfloating->ext_ip&&lb_vipfloating->inside_ip){	
					//LOG_PROC("INFO", "insert floating ip lb_vipfloating->ext_ip=0x%x lb_vipfloating->inside_ip=0x%x\n", lb_vipfloating->ext_ip,lb_vipfloating->inside_ip );
					//create_floatting_ip_by_rest(lb_vipfloating->inside_ip,lb_vipfloating->ext_ip,lb_vipfloating->ex_port_id,lb_vipfloating->router_id, floatingtype_Clbvips);
					//lb_vipfloating->ext_ip = 0;
					//lb_vipfloating->inside_ip = 0;
				}
			}

			
			/*
			openstack_lbaas_pools_p pool_p = update_openstack_clbaas_pool_by_clbviprest(tenant_id, lb_pool_id, ipv4addr);
			if ((pool_p) && is_check_status_changed(pool_p->check_status))
				totalNum++;
			*/
			lbaas_vip = lbaas_vip->next;
		}
		json_free_value(&lbaas_vips);
	}
	json_free_value_all(&json);
	return GN_OK;
}

INT4 createOpenstackClbaasloadbalancer(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	UINT1 tenant_id[OPENSTACK_STRINGLEN] ={0};
	UINT1 lb_pool_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 lb_loadbancer_id[OPENSTACK_STRINGLEN] = {0};

	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json)) 
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	
	json_t *lbaas_loadbalancers = json_find_first_label(json, "load_balancers");
	if(lbaas_loadbalancers&&lbaas_loadbalancers->child){
		json_t *lbaas_loadbalancer = lbaas_loadbalancers->child->child;
		while(lbaas_loadbalancer){
			
			temp = json_find_first_label(lbaas_loadbalancer, "tenant_id");
			strcpy(tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(tenant_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_loadbalancer, "id");
			strcpy(lb_loadbancer_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_loadbancer_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_loadbalancer, "pool_id");
			strcpy(lb_pool_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_pool_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			if('\0' != lb_loadbancer_id[0])
			{
				update_openstack_clbaas_loadbalancer_by_loadbalancerrest(lb_loadbancer_id, lb_pool_id, tenant_id);
			}

			lbaas_loadbalancer = lbaas_loadbalancer->next;
		}
		json_free_value(&lbaas_loadbalancers);
	}
	json_free_value_all(&json);
	return GN_OK;
}


INT4 createOpenstackClbaasinterface(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	UINT1 HA_host_mac[6] = {0};
	UINT1 tenant_id[OPENSTACK_STRINGLEN] ={0};
	UINT1 subnet_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 network_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 lb_port_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 lb_loadbancer_id[OPENSTACK_STRINGLEN] = {0};
	UINT1 lb_interface_type = 0;
	UINT1 ipv6addr[16] = {0};
	UINT4 ipv4addr = 0;
	UINT1 isPublicType = 0;
	//openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	
	json_t *lbaas_interfaces = json_find_first_label(json, "interfaces");
	if(lbaas_interfaces&&lbaas_interfaces->child){
		json_t *lbaas_interface = lbaas_interfaces->child->child;
		while(lbaas_interface){
			temp = json_find_first_label(lbaas_interface, "is_public");
			if(temp)
			{
				if(temp->child->type){
					isPublicType =  (JSON_TRUE == temp->child->type)?1:0;
				}
				json_free_value(&temp);
			}
			
			
			temp = json_find_first_label(lbaas_interface, "tenant_id");
			strcpy(tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(tenant_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_interface, "load_balancer_id");
			strcpy(lb_loadbancer_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_loadbancer_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_interface, "network_id");
			strcpy(network_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(network_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_interface, "port_id");
			strcpy(lb_port_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_port_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}

			temp = json_find_first_label(lbaas_interface, "type");
			lb_interface_type = CLBAAS_INTERFACE_UNKOWN;
			if(temp){
				if(temp->child->text){
					if(strcmp(temp->child->text,"HA")==0)
					{
						lb_interface_type = CLBAAS_INTERFACE_HA;
					}
					else if(strcmp(temp->child->text,"VIP")==0)
					{
						lb_interface_type = CLBAAS_INTERFACE_VIP; 
					}
					else
					{
						lb_interface_type = CLBAAS_INTERFACE_UNKOWN;
					}
					
				}
				json_free_value(&temp);
			}
			
			json_t *fix_ips = json_find_first_label(lbaas_interface, "fixed_ips");
			if(fix_ips&&fix_ips->child)
			{
				json_t *fix_ip = fix_ips->child->child;
				while(fix_ip){
					temp = json_find_first_label(fix_ip, "subnet_id");
					strcpy(subnet_id,"");
					if(temp){
						strcpy(subnet_id,temp->child->text);
						json_free_value(&temp);
					}
					temp = json_find_first_label(fix_ip, "ip_address");
					if(temp){
						memset(ipv6addr, 0, 16);
						ipv4addr = 0;
						if(temp->child->text)
						{
							if (strchr(temp->child->text, ':')) {
								// printf("ipv6: %s\n", temp->child->text);
								ipv6_str_to_number(temp->child->text, ipv6addr);
								//nat_show_ipv6(ipv6);
							}
							else {
								UINT4 longtemp = inet_addr(temp->child->text) ;
								ipv4addr = longtemp;
							}
						}
						json_free_value(&temp);
					}
					fix_ip=fix_ip->next;
				}
				json_free_value(&fix_ips);
			}

			if(CLBAAS_INTERFACE_HA == lb_interface_type)
			{
				if('\0' != lb_loadbancer_id[0])
				{
					update_openstack_clbaas_loadbalancer_by_clbinterfacerest(lb_loadbancer_id, tenant_id, ipv4addr);
				}
			}

			lbaas_interface = lbaas_interface->next;
		}
		json_free_value(&lbaas_interfaces);
	}
	json_free_value_all(&json);
	return GN_OK;
}

INT4 createOpenstackClbaaslistener(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char  tenant_id[OPENSTACK_STRINGLEN] ={0};
	char  lb_pool_id[OPENSTACK_STRINGLEN] = {0};
	char  lb_listener_id[OPENSTACK_STRINGLEN] = {0};
	INT4  client_timeout = 0;
	INT4  lb_port = 0;
	UINT1 lb_method = 0;
	UINT1 lb_protocol = 0;
	UINT4 lb_maxconn = 0;
	UINT1 lb_session_persistence = 0;
	t_lb_health_check lb_health_check ={0};
	
	parse_type = json_parse_document(&json,tempString);
	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	
	json_t *lbaas_listeners = json_find_first_label(json, "listeners");
	if(lbaas_listeners&&lbaas_listeners->child){
		json_t *lbaas_listener = lbaas_listeners->child->child;
		while(lbaas_listener){

			temp = json_find_first_label(lbaas_listener, "lb_method");
			if(temp){
				if(temp->child->text){
					if(strcmp(temp->child->text,"ROUNT_ROBIN")==0){
						lb_method=LB_M_ROUNT_ROBIN;
					}else if(strcmp(temp->child->text,"LEAST_CONNECTIONS")==0){
						lb_method=LB_M_LEAST_CONNECTIONS;
					}else if(strcmp(temp->child->text,"SOURCE_IP")==0){
						lb_method=LB_M_SOURCE_IP;
					}
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "protocol");
			if(temp){
				if(temp->child->text){
					if(strcmp(temp->child->text,"HTTP")==0){
						lb_protocol=LBAAS_PRO_HTTP;
					}else if(strcmp(temp->child->text,"HTTPS")==0){
						lb_protocol=LBAAS_PRO_HTTPS;
					}else if(strcmp(temp->child->text,"TCP")==0){
						lb_protocol=LBAAS_PRO_TCP;
					}
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "tenant_id");
			strcpy(tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(tenant_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "client_timeout");
			if(temp){
				if(temp->child->text){
					client_timeout = atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "lb_pool_id");
			strcpy(lb_pool_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_pool_id,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "port");
			if(temp){
				if(temp->child->text){
					lb_port = atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_listener, "maxconn");
			if(temp){
				if(temp->child->text){
					lb_maxconn = atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			json_t *health_checks = json_find_first_label(lbaas_listener, "health_check");
			if(health_checks)
			{
				json_t *health_check = health_checks->child;
				while(health_check){
					temp = json_find_first_label(health_check, "delay");
					if(temp){
						if(temp->child->text){
							lb_health_check.delay = atoi(temp->child->text);
							
						}
						json_free_value(&temp);
					}
					temp = json_find_first_label(health_check, "timeout");
					if(temp){
							if(temp->child->text){
								lb_health_check.timeout = atoi(temp->child->text);
								
							}
							json_free_value(&temp);
					}
					temp = json_find_first_label(health_check, "fail");
					if(temp){
						if(temp->child->text){
							lb_health_check.fail = atoi(temp->child->text);
							
						}
						json_free_value(&temp);
					}
					temp = json_find_first_label(health_check, "type");
					if(temp){
						if(temp->child->text && strcmp(temp->child->text,"PING")==0){
							lb_health_check.type=LBAAS_LISTENER_PING;
							
						}else if(temp->child->text && strcmp(temp->child->text,"TCP")==0){
							lb_health_check.type=LBAAS_LISTENER_TCP;
						}else if(temp->child->text && strcmp(temp->child->text,"HTTP")==0){
							lb_health_check.type=LBAAS_LISTENER_HTTP;
						}else if(temp->child->text && strcmp(temp->child->text,"HTTPS")==0){
							lb_health_check.type=LBAAS_LISTENER_HTTPS;
						}
						json_free_value(&temp);
					}
					temp = json_find_first_label(health_check, "rise");
					if(temp){
						if(temp->child->text){
							lb_health_check.rise = atoi(temp->child->text);
						}
						json_free_value(&temp);
					}
					temp = json_find_first_label(health_check, "enabled");
					if(temp){
						if(temp->child->type){
							lb_health_check.enabled =  (JSON_TRUE == temp->child->type)?1:0;
						}
						json_free_value(&temp);
				    }
					health_check = health_check->next;
				}
				json_free_value(&health_checks);
			}
			json_t *session_persistences = json_find_first_label(lbaas_listener, "session_persistence");
			if(session_persistences)
			{
				json_t *session_persistence = session_persistences->child;
				while(session_persistence){
					temp = json_find_first_label(session_persistence, "type");
					if(temp){
						if(temp->child->type){
							if(strcmp(temp->child->text,"SOURCE_IP")==0){
							lb_session_persistence=SEPER_SOURCE_IP;
							}else if(strcmp(temp->child->text,"HTTP_COOKIE")==0){
								lb_session_persistence=SEPER_HTTP_COOKIE;
							}else if(strcmp(temp->child->text,"APP_COOKIE")==0){
								lb_session_persistence=SEPER_APP_COOKIE;
							}else{
								lb_session_persistence= SEPER_NO_LIMIT;
							}
							
						}
						json_free_value(&temp);
					}
					session_persistence = session_persistence->next;
				}
				json_free_value(&session_persistences);
			}

			temp = json_find_first_label(lbaas_listener, "lb_listener_id");
			strcpy(lb_listener_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_listener_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
		
			/*
			openstack_lbaas_pools_p pool_p = update_openstack_clbaas_pool_by_clblistenrest(
												tenant_id, 
												lb_pool_id, 
												lb_protocol,
												lb_port, 
												lb_maxconn,
												lb_method,
												lb_session_persistence);

			openstack_lbaas_listener_p listener_p = update_openstack_clbaas_listener_by_clblistenrest(
																	 lb_listener_id,
																	 lb_health_check.type,
																	 lb_health_check.delay,
																	 lb_health_check.timeout,
																	 lb_health_check.fail);
			*/
		
			
			lbaas_listener = lbaas_listener->next;
		}
		json_free_value(&lbaas_listeners);
	}
	json_free_value_all(&json);
	return  GN_OK;
}
INT4 createOpenstackClbaasbackend(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char tenant_id[OPENSTACK_STRINGLEN] ={0};
	char updated_at[OPENSTACK_STRINGLEN] = {0};
	char lb_backend_id[OPENSTACK_STRINGLEN] = {0};
	char lb_listener_id[OPENSTACK_STRINGLEN] = {0};
	UINT4 ipv4addr = 0;
	INT4  lb_port = 0;	
	parse_type = json_parse_document(&json,tempString);

	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}
	
	json_t *lbaas_backends = json_find_first_label(json, "backends");
	if(lbaas_backends&&lbaas_backends->child){
		json_t *lbaas_backend = lbaas_backends->child->child;
		while(lbaas_backend){
			temp = json_find_first_label(lbaas_backend, "tenant_id");
			strcpy(tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(tenant_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_backend, "updated_at");
			strcpy(updated_at,"");
			if(temp){
				if(temp->child->text){
					strcpy(updated_at,temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_backend, "port");
			if(temp){
				if(temp->child->text){
					lb_port = atoi(temp->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_backend, "listeners");
			strcpy(lb_listener_id,"");
			if(temp){
				if(temp->child&&temp->child->child&&(temp->child->child->text)){
					strcpy(lb_listener_id,temp->child->child->text);
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_backend, "address");
			if(temp){
				if(temp->child->text){
					ipv4addr = inet_addr(temp->child->text) ;
				}
				json_free_value(&temp);
			}
			
			temp = json_find_first_label(lbaas_backend, "id");
			strcpy(lb_backend_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_backend_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			update_openstack_clbaas_backendip_by_backendrest(ipv4addr);
			
			lbaas_backend = lbaas_backend->next;
		}
		json_free_value(&lbaas_backends);
	}
	json_free_value_all(&json);
	return  GN_OK;
}	
INT4 createOpenstackClbaasbackendListen(char* jsonString, void* param)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char tenant_id[OPENSTACK_STRINGLEN] ={0};
	char lb_backend_id[OPENSTACK_STRINGLEN] = {0};
	char lb_listener_id[OPENSTACK_STRINGLEN] = {0};
	UINT4 lb_weight = 0;
	
	parse_type = json_parse_document(&json,tempString);

	if((parse_type != JSON_OK)||(NULL == json))	
	{
		LOG_PROC("INFO", "%s failed tempString=%s", FN,tempString);
		return	GN_ERR;
	}

	json_t *lbaas_backendlisteners = json_find_first_label(json, "backend_listener_bindings");
	if(lbaas_backendlisteners&&lbaas_backendlisteners->child){
		json_t *lbaas_backendlistener = lbaas_backendlisteners->child->child;
		while(lbaas_backendlistener){
			temp = json_find_first_label(lbaas_backendlistener, "tenant_id");
			strcpy(tenant_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(tenant_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_backendlistener, "backend_id");
			strcpy(lb_backend_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_backend_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}

			temp = json_find_first_label(lbaas_backendlistener, "listener_id");
			strcpy(lb_listener_id,"");
			if(temp){
				if(temp->child->text){
					strcpy(lb_listener_id,temp->child->text);
					
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(lbaas_backendlistener, "weight");
			if(temp){
				if(temp->child->text){
					lb_weight = atoi(temp->child->text);
					
				}
				json_free_value(&temp);
			}

			/*
			openstack_lbaas_members_p lb_member_p = update_openstack_clbaas_member_by_backendbindrest(
																			lb_backend_id,
																			tenant_id,
																			lb_weight);
			*/
														
			lbaas_backendlistener = lbaas_backendlistener->next;
		}
		json_free_value(&lbaas_backendlisteners);
	}
	json_free_value_all(&json);
	return  GN_OK;
}



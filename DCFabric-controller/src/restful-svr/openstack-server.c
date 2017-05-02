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
 * fabric_openstack_external.h
 *
 *  Created on: sep 11, 2015
 *  Author: yanglei
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: sep 11, 2015
 */

#include "openstack-server.h"
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

void createPortFabric(char* jsonString,const char* stringType);

void createPortForward(char* jsonString, port_forward_param_p forwardParam);


static char * g_openstack_server_name;
UINT4 g_openstack_port = 9696;
char g_openstack_ip[16] = {0};
//by:yhy 全局标志 openstack是否启用;由配置文件中读取
UINT4 g_openstack_on = 0;
//by:yhy openstack特殊服务用保留IP,被初始化成169.254.169.254 此ip表示DHCP没有获得到ip,便分配成这个保留IP
UINT4 g_openstack_fobidden_ip = 0;
//by:yhy 未使用
INT4 numm = 0;

UINT1 FirstTime_GetSecurityInfo =1;

//定时刷新
void 	*g_reload_timer 	= NULL;
void 	*g_reload_timerid 	= NULL;
UINT4 	g_reload_interval 	= 30;

//端口转发列表
static struct _openstack_port_forward *  local_openstack_forward_list = NULL;
extern openstack_security_p g_openstack_security_list ;
extern openstack_security_p g_openstack_security_list_temp ;

//by:yhy  从openstack的控制节点通过restfulAPI获取token id
//操作成功返回"1"失败返回"0"
int getNewTokenId(char *ip,char *tenantName,char *username,char *password)
{
    int sockfd, ret, i, h, iErrno = 0;
    int Length_BuffSended =0;
    struct sockaddr_in servaddr;
    char str1[4096], str2[4096], buf1[4096], buf2[819200], str[128];
    socklen_t len;
    fd_set   t_set1;
    struct timeval  tv;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
    {
        iErrno = errno; 
        LOG_PROC("ERROR", "%s -- socket error! Error code: %d",FN,iErrno);
        return 0;
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(35357);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0 )
    {
        iErrno = errno; 
        LOG_PROC("ERROR", "%s -- net_pton error! Error code: %d",FN,iErrno);
        return 0;
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        iErrno = errno; 
        LOG_PROC("ERROR", "%s -- connect error! Error code: %d",FN,iErrno);
        return 0;
    }

    memset(str2, 0, 4096);
    strcat(str2, "{\"auth\": {\"tenantName\": \"");
    strcat(str2,tenantName);
    strcat(str2,"\",\"passwordCredentials\": {\"username\":\"");
    strcat(str2,username);
    strcat(str2,"\",\"password\":\"");
    strcat(str2,password);
    strcat(str2,"\"}}}");
    memset(str, 0, 128);
    len = strlen(str2);
    sprintf(str, "%d", len);

    memset(str1, 0, 4096);
    strcat(str1, "POST /v2.0/tokens HTTP/1.0\n");
    strcat(str1, "Content-Type: application/json\n");
    strcat(str1, "Content-Length: ");
    strcat(str1, str);
    strcat(str1, "\n\n");

    strcat(str1, str2);
    strcat(str1, "\r\n\r\n");

    while(Length_BuffSended !=strlen(str1))
    {
        ret = send(sockfd,(str1+Length_BuffSended),(strlen(str1)-Length_BuffSended),0); 
        if(ret <0)
        {
            iErrno = errno;
            if((EINTR== iErrno)||(EAGAIN ==iErrno))
            {
                continue;
            }
            else
            {
                LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
                close(sockfd);
                return 0;
            }
        }
        else
        {
            Length_BuffSended += ret;
        }
    }

    memset(buf2, 0, 819200);
    while(1)
    {
        // usleep(1000000);
        tv.tv_sec= 1;
        tv.tv_usec= 0;
        FD_ZERO(&t_set1);
        FD_SET(sockfd, &t_set1);
        h= 0;
        h= select(sockfd +1, &t_set1, NULL, NULL, &tv);
        if (h > 0) 
        {
            memset(buf1, 0, 4096);
            i= read(sockfd, buf1, 4095);
            if (i > 0)
            {
                strcat(buf2, buf1);
            }
            else if (0 == i)
            {
                break;
            }
        }
        else if (h < 0)
        {
            iErrno = errno;
            if(EAGAIN == iErrno || EINTR== iErrno)
            {
                continue;
            }
            else
            {
                LOG_PROC("ERROR", "%s,select error code: %d",FN,iErrno);
                close(sockfd);
                return 0;
            }
        }
    }

    close(sockfd);

    const char *p = strstr(buf2,"\r\n\r\n");
	if( NULL == p)
	{
		LOG_PROC("ERROR", "%s %d",FN,LN);
        return 0;
	}
    INT4 parse_type = 0;
    json_t *json=NULL;

    parse_type = json_parse_document(&json,p);
    if (parse_type != JSON_OK)
    {
        return 0;
    }
    else
    {
        if (json) 
        {
            json_t *access=NULL,*token = NULL;
            access = json_find_first_label(json, "access");
            if(access)
            {
                json_t *access_t  = access->child;
                if(access_t)
                {
                    token = json_find_first_label(access_t, "token");
                    if(token)
                    {
                        json_t *token_t = token->child;
                        if(token_t)
                        {
                            json_t *token_id = json_find_first_label(token_t,"id");
                            if(token_id)
                            {
                                g_openstack_server_name = token_id->child->text;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            printf(" no json find :%s\n",p);
        }
    }

    return 1;
}

//by:yhy 获得openstack北桥数据
//这一部分restful与openstack控制节点
//最终数据更新入g_openstack_host_network_list
void getOpenstackInfo(char *ip,char *url,int port,char *stringType, void* param, enum EOpenStack_GetType getType){
	char *string;
	int sockfd, ret, i, h,iErrno = 0;
	int Length_BuffSended =0;
    struct sockaddr_in servaddr;
	char str1[4096], str2[4096], buf[4096], str[128], str3[819200];
	socklen_t len;
	fd_set   t_set1;
	struct timeval  tv;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) 
	{
		iErrno = errno;	
		LOG_PROC("ERROR", "%s -- socket error! iErrno: %d",FN,iErrno);
		return ;
	};
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0 )
	{
		iErrno = errno; 
		LOG_PROC("ERROR", "%s -- net_pton error! iErrno: %d",FN,iErrno);
		return ;
	};
	//by:yhy 捆绑套接字
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		iErrno = errno;
		LOG_PROC("ERROR", "%s -- connect error! iErrno: %d",FN,iErrno);
		return ;
	}

	char str_tenantname[48] = {0};
	char str_username[48] = {0};
	char str_password[48] = {0};
	
	INT1 *value = get_value(g_controller_configure, "[openvstack_conf]", "tenantname");
    NULL == value ? strncpy(str_tenantname, "admin", 48 - 1) : strncpy(str_tenantname, value, 48 - 1);

	value = get_value(g_controller_configure, "[openvstack_conf]", "username");
	NULL == value ? strncpy(str_username, "admin", 48 - 1) : strncpy(str_username, value, 48 - 1);

	value = get_value(g_controller_configure, "[openvstack_conf]", "password");
	NULL == value ? strncpy(str_password, "admin", 48 - 1) : strncpy(str_password, value, 48 - 1);
	
	//by:yhy 获取token
		
    if (0 == getNewTokenId(ip, str_tenantname, str_username, str_password))	
	{
		return ;
	}

	memset(str2, 0, 4096);
	strcat(str2, "{}");
    memset(str, 0, 128);
	len = strlen(str2);
	sprintf(str, "%d", len);

	memset(str1, 0, 4096);
    strcat(str1, "GET ");
	strcat(str1,url);
	strcat(str1, " HTTP/1.1\n");
	strcat(str1, "Content-Type:application/json\n");
	strcat(str1,"X-Auth-Token:");
	if (NULL == g_openstack_server_name) 
	{
		LOG_PROC("ERROR", "Openstack authentication failure! Please check the configuration.");

        return ;
	}
	strcat(str1,g_openstack_server_name);
	strcat(str1,"\n\n");
	strcat(str1, "Content-Length: ");
	strcat(str1, str);
	strcat(str1, "\n\n");
	strcat(str1, str2);
	strcat(str1, "\r\n\r\n");

	
	while(Length_BuffSended !=strlen(str1))
	{
		ret = send(sockfd,(str1+Length_BuffSended),(strlen(str1)-Length_BuffSended),0); 
		if(ret <0)
		{
			iErrno = errno;
			if((EINTR== iErrno)||(EAGAIN ==iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				close(sockfd);

				return;
			}
		}
		else
		{
			Length_BuffSended += ret;
		}
	}
	memset(str3, 0, 819200);
	while(1)
	{
		// usleep(1000000);
        FD_ZERO(&t_set1);
        FD_SET(sockfd, &t_set1);

		tv.tv_sec= 1;
		tv.tv_usec= 0;
		h= 0;
		h= select(sockfd +1, &t_set1, NULL, NULL, &tv);

		if (h > 0)
		{
			memset(buf, 0, 4096);
			i= read(sockfd, buf, 4095);
			strcat(str3,buf);
			if (0 == i)
			{
				close(sockfd);
				break;
			}
		}
        else if (h < 0)
        {
			iErrno = errno;
			if(EAGAIN == iErrno || EINTR== iErrno)
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,select error code: %d",FN,iErrno);
				close(sockfd);
				return;
			}
        }
	}
	close(sockfd);
	char *p = strstr(str3,"\r\n\r\n");
	if(NULL == p)
	{
		LOG_PROC("ERROR", "%s %d",FN,LN);
		return;
	}
	char *strend = "<head>";
    char *p1 = strstr(p,strend);
	if(NULL == p1)
	{
		LOG_PROC("ERROR", "%s %d",FN,LN);
		return;
	}
	int endindex=p1-p;
	if(endindex>0)  
	{
        string=(char*)malloc((endindex)*sizeof(char));
        strncpy(string, p+1, endindex-1); 
        string[endindex-1]='\0';
    }

	if(EOPENSTACK_GET_NORMAL == getType)
	{	
		createPortFabric(string,stringType);
	}
	else if(EOPENSTACK_GET_PORTFORWARD == getType)
	{
		LOG_PROC("INFO", "EOPENSTACK_GET_PORTFORWARD");
		createPortForward(string, param); 
	}


	if(string != NULL)
	{
		free(string);
	}

}

//by:yhy 更新openstack的浮动IP
void updateOpenstackFloating()
{
	// config
	INT1 *value = NULL;

	// config & check openstack
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	g_openstack_on = (NULL == value)?0:atoll(value);
	if( 1 == g_openstack_on)
	{
		// get openstack ip
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.53.51", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		// get port;
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoll(value));
		//
		getOpenstackInfo(g_openstack_ip,"/v2.0/floatingips",g_openstack_port,"floating" , NULL, EOPENSTACK_GET_NORMAL);
		// LOG_PROC("INFO", "Openstack Floating Info Updated");
	}
	else
	{
		LOG_PROC("INFO", "Openstack Floating Info Update Failed");
	}
}

void show_create_port_log(const char* stringType, INT4 totalNum)
{
	if (0 != totalNum) {
		LOG_PROC("INFO","OPENSTACK [%d] updated!\ttype: %s", totalNum, stringType);
	}
}

void reset_unchecked_flag(char* stringType)
{
	// TBD
}

void remove_unchecked_port(char* stringType)
{
	// TBD
}
//by:yhy  根据stringtype提取jsonstring中的有效信息  并将有效信息构建成openstack_network_p更新入g_openstack_host_network_list
void createPortFabric( char *jsonString,const char *stringType)
{
    const char *tempString = jsonString;
	INT4 parse_type = 0;
	INT4 totalNum = 0;
	json_t *json=NULL,*temp=NULL;
	char* networkType="network",*subnetType="subnet",*portType="port",*floatingType="floating", *routersType="routers";
	char* securityType = "security-group-rules";
	char* lbpools = "pools";
	char* lbvips = "vips";
	char* lbmembers = "lbmem";
	char* lblistener = "lblistener";
	char tenant_id[48] ={0};
	char network_id[48] = {0};
	char subnet_id[48] = {0};
	char port_id[48] = {0};
	char cidr[30] = {0};
	char router_id[48]={0};
	char security_group_id[48]={0};
	char direction[48] = {0};
	char ethertype[48] = {0};
	char rule_id[48] = {0};
	char port_range_max[48] = {0};
	char port_range_min[48] = {0};
	char protocol[48] = {0};
	char remote_group_id[48] = {0};
	char remote_ip_prefix[48] = {0};
	char security_tenant_id[48] = {0};
        char routers_id[128] = {0};
        char routers_external_ip[48] = {0};
	UINT2 security_num = 0;
	UINT2 security_num_temp = 0;
	UINT1* security_port_p = NULL;
	UINT1* security_port_p_temp = NULL;
	//LOG_PROC("INFO", "%s",FN);
	parse_type = json_parse_document(&json,tempString);
	if (parse_type != JSON_OK)
	{
		return;
	}
	else
	{
		if (json) 
		{
			if(strcmp(stringType,networkType)==0)
			{
				json_t *networks = json_find_first_label(json, "networks");
			    UINT1 shared=0;
				UINT1 external=0;
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
			}
       		       else if(0 == strcmp(stringType, routersType))
			{ 
				json_t *routers = json_find_first_label(json, "routers");
				if(routers)
				{
					json_t * pRouter  = routers->child->child;
					while(pRouter)
					{
						memset(routers_id, 0, sizeof(routers_id));
    
						temp = json_find_first_label(pRouter, "id");
						if(temp)
						{
							strcpy(routers_id,temp->child->text);
							json_free_value(&temp);
						}

						update_openstack_router_list(&g_openstack_router_list, routers_id, routers_external_ip, network_id);

						pRouter=pRouter->next;
					}
					json_free_value(&routers);
				}
			}
			else if(strcmp(stringType,subnetType)==0)
			{
//				char *tenant_id,*network_id,*subnet_id,*cidr;
				INT4 gateway_ip = 0, start_ip = 0, end_ip = 0;
				UINT1 gateway_ipv6[16] = {0};
				UINT1 start_ipv6[16] = {0};
				UINT1 end_ipv6[16] = {0};
				json_t *subnets = json_find_first_label(json, "subnets");
				UINT4 longtemp = 0;
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

			}
			else if(strcmp(stringType,portType)==0)
			{
				//char *tenant_id,*network_id,*subnet_id,*port_id;
				char port_type[40] = {0};
				char* computer="compute:nova";
				char* computer_none="compute:None";
				char* dhcp="network:dhcp";
				char* floating = "network:floatingip";
				INT4 ip = 0;
				UINT1 ipv6[16] = {0};
				UINT1 mac[6]={0};
				UINT4 port_number = 0;
				UINT1 type = 0;
				json_t *ports = json_find_first_label(json, "ports");
				if(ports){
					json_t *port  = ports->child->child;
					while(port){
						temp = json_find_first_label(port, "tenant_id");
						if(temp){
							strcpy(tenant_id,temp->child->text);
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
						temp = json_find_first_label(port, "device_owner");
						if(temp){
							strcpy(port_type,temp->child->text);
							type = get_openstack_port_type(port_type);
							//port_type = temp->child->text;
							json_free_value(&temp);
						}
						temp = json_find_first_label(port, "mac_address");
						if(temp){
							//printf("mac_address:%s \n",);
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
						// printf("security_groups\n");
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
								if(FirstTime_GetSecurityInfo)
								{
									openstack_security_p temp_p = update_openstack_security_group(security_group->text);
									head_p = add_openstack_host_security_node((UINT1*)temp_p, head_p);
									security_port_p = (UINT1*)head_p;
									security_num++;	
									//LOG_PROC("INFO","------333");
								}
								else
								{
									openstack_security_p temp_p_temp = update_openstack_security_group_temp(security_group->text);
									head_p_temp = add_openstack_host_security_node((UINT1*)temp_p_temp, head_p_temp);
									security_port_p_temp = (UINT1*)head_p_temp;
									security_num_temp++;
									//LOG_PROC("INFO","---------444");
								}
								security_group = security_group->next;
							}
							json_free_value(&security_groups);
						}

						p_fabric_host_node host_p = NULL;
						if(strcmp(port_type,computer)==0|| strcmp(port_type,floating)==0 || strcmp(port_type,computer_none)==0)
						{
							//LOG_PROC("INFO","PORT UPDATE!");
							if(FirstTime_GetSecurityInfo)
							{
								host_p = update_openstack_app_host_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,security_num,security_port_p);
							}
							else
							{
								host_p = update_openstack_app_host_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,security_num_temp,security_port_p_temp);
							}
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
							clear_openstack_host_security_node(security_port_p);
							clear_openstack_host_security_node(security_port_p_temp);
						}

						if ((host_p) && (is_check_status_changed(host_p->check_status))) 
						{
							totalNum++;
						}
						port=port->next;
					}
					json_free_value(&ports);
				}
			}
			else if(strcmp(stringType,floatingType)==0)
			{
				json_t *floatings = json_find_first_label(json, "floatingips");
				UINT4 fixed_ip;//inner ip
				UINT4 floating_ip;//outer ip
				if(floatings){
					json_t *floating_ip_one  = floatings->child->child;
					while(floating_ip_one){
						fixed_ip = 0;
						floating_ip = 0;
						bzero(router_id, 48);
						bzero(port_id, 48);
						
						temp = json_find_first_label(floating_ip_one, "router_id");
						if(temp){
							if(temp->child->text){
								strcpy(router_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(floating_ip_one, "port_id");
						if(temp){
							if(temp->child->text){
								strcpy(port_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(floating_ip_one, "fixed_ip_address");
						if(temp){
							if(temp->child->text){
								fixed_ip = inet_addr(temp->child->text);
							}
							json_free_value(&temp);
						}
						temp = json_find_first_label(floating_ip_one, "floating_ip_address");
						if(temp){
							if(temp->child->text){
								floating_ip = inet_addr(temp->child->text);
							}
							json_free_value(&temp);
						}
						external_floating_ip_p efp = create_floatting_ip_by_rest(fixed_ip,floating_ip,port_id,router_id);
						if ((efp) && (is_check_status_changed(efp->check_status))) {
							totalNum++;
						}
						floating_ip_one=floating_ip_one->next;
						
					}
					json_free_value(&floatings);
				}
			}
			else if(strcmp(stringType, securityType)==0) 
			{
				json_t *security_groups = json_find_first_label(json, "security_group_rules");
				if(security_groups){
					json_t *security_group  = security_groups->child->child;
					while(security_group){
						temp = json_find_first_label(security_group, "security_group_id");
						if(temp){
							if(temp->child->text){
								strcpy(security_group_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(direction,"");
						temp = json_find_first_label(security_group, "direction");
						if(temp){
							if(temp->child->text){
								strcpy(direction,temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(ethertype,"");
						temp = json_find_first_label(security_group, "ethertype");
						if(temp){
							if(temp->child->text){
								strcpy(ethertype,temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(rule_id,"");
						temp = json_find_first_label(security_group, "id");
						if(temp){
							if(temp->child->text){
								strcpy(rule_id, temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(port_range_max,"");
						temp = json_find_first_label(security_group, "port_range_max");
						if(temp){
							if(temp->child->text){
								strcpy(port_range_max, temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(port_range_min,"");
						temp = json_find_first_label(security_group, "port_range_min");
						if(temp){
							if(temp->child->text){
								strcpy(port_range_min, temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(protocol,"");
						temp = json_find_first_label(security_group, "protocol");
						if(temp){
							if(temp->child->text){
								strcpy(protocol, temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(remote_group_id,"");
						temp = json_find_first_label(security_group, "remote_group_id");
						if(temp){
							if(temp->child->text){
								strcpy(remote_group_id, temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(remote_ip_prefix,"");
						temp = json_find_first_label(security_group, "remote_ip_prefix");
						if(temp){
							if(temp->child->text){
								strcpy(remote_ip_prefix, temp->child->text);
								json_free_value(&temp);
							}
						}
						strcpy(security_tenant_id,"");
						temp = json_find_first_label(security_group, "tenant_id");
						if(temp){
							if(temp->child->text){
								strcpy(security_tenant_id, temp->child->text);
								json_free_value(&temp);
							}
						}
						
						
						///TBD
						if(FirstTime_GetSecurityInfo)
						{
							openstack_security_rule_p rule_p = update_security_rule(security_group_id,rule_id,direction,ethertype,
															   port_range_max,port_range_min,protocol,remote_group_id,remote_ip_prefix,security_tenant_id);
							if ((rule_p) && (is_check_status_changed(rule_p->check_status))) 
							{
								totalNum++;
							}
						}
						else
						{
							update_security_rule_temp(security_group_id,rule_id,direction,ethertype,
												 port_range_max,port_range_min,protocol,remote_group_id,remote_ip_prefix,security_tenant_id);
						}
						
						security_group = security_group->next;
					}
					json_free_value(&security_groups);
				}
			}
			else if(strcmp(stringType,lbpools)==0)
			{
				char lb_pool_id[48] = {0};
				UINT1 lb_status=0;
				UINT1 lb_protocol=0;
				UINT1 lbaas_method=0;
				json_t *lbaas_pools = json_find_first_label(json, "pools");
				if(lbaas_pools){
					json_t *lbaas_pool  = lbaas_pools->child->child;
					while(lbaas_pool){
						temp = json_find_first_label(lbaas_pool, "tenant_id");
						strcpy(tenant_id,"");
						if(temp){
							if(temp->child->text){
								strcpy(tenant_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_pool, "id");
						strcpy(lb_pool_id,"");
						if(temp){
							if(temp->child->text){
								strcpy(lb_pool_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_pool, "status");
						if(temp){
							if(temp->child->text && strcmp(temp->child->text,"ACTIVE")==0){
								lb_status=LBAAS_OK;
								json_free_value(&temp);
							}
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
								json_free_value(&temp);
							}
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
								json_free_value(&temp);
							}
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
			}
			else if(strcmp(stringType,lbvips)==0)
			{
				char lb_pool_id[48] = {0};
				UINT1 vip_status=0;
				UINT4 vip_ip_adress=0;
				UINT4 protocol_port=0;
				UINT1 connect_limit=0;
				UINT1 session_persistence=0;
				json_t *lbaas_pools = json_find_first_label(json, "vips");
				if(lbaas_pools){
					json_t *lbaas_pool  = lbaas_pools->child->child;
					while(lbaas_pool){
						temp = json_find_first_label(lbaas_pool, "address");
						if(temp){
							if(temp->child->text){
								vip_ip_adress= inet_addr(temp->child->text) ;
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_pool, "pool_id");
						strcpy(lb_pool_id,"");
						if(temp){
							if(temp->child->text){
								strcpy(lb_pool_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_pool, "status");
						if(temp){
							if(temp->child->text && strcmp(temp->child->text,"ACTIVE")==0){
								vip_status=LBAAS_OK;
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_pool, "protocol_port");
						if(temp){
							if(temp->child->text){
								protocol_port=atoll(temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_pool, "connection_limit");
						if(temp){
							if(temp->child->text){
								connect_limit=atoi(temp->child->text);
								json_free_value(&temp);
							}
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
									json_free_value(&temp2);
									json_free_value(&temp);
								}
							}
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
			}
			else if(strcmp(stringType,lbmembers)==0)
			{
				char lb_member_id[48] = {0};
				char lb_pool_id[48] = {0};
				char lb_tenant_id[48] = {0};
				UINT1 mem_status=0;
				UINT1 mem_weight=0;
				UINT4 mem_protocol_port=0;
				UINT4 mem_fixed_ip=0;
				json_t *lbaas_members = json_find_first_label(json, "members");
				if(lbaas_members){
					json_t *lbaas_member  = lbaas_members->child->child;
					while(lbaas_member){
						temp = json_find_first_label(lbaas_member, "address");
						if(temp){
							if(temp->child->text){
								mem_fixed_ip= inet_addr(temp->child->text) ;
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_member, "id");
						strcpy(lb_member_id,"");
						if(temp){
							if(temp->child->text){
								strcpy(lb_member_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_member, "pool_id");
						strcpy(lb_pool_id,"");
						if(temp){
							if(temp->child->text){
								strcpy(lb_pool_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_member, "tenant_id");
						strcpy(lb_tenant_id,"");
						if(temp){
							if(temp->child->text){
								strcpy(lb_tenant_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_member, "status");
						if(temp){
							if(temp->child->text && strcmp(temp->child->text,"ACTIVE")==0){
								mem_status=LBAAS_OK;
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_member, "protocol_port");
						if(temp){
							if(temp->child->text){
								mem_protocol_port=atoll(temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_member, "weight");
						if(temp){
							if(temp->child->text){
								mem_weight=atoi(temp->child->text);
								json_free_value(&temp);
							}
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
			}
			else if(strcmp(stringType,lblistener)==0)
			{
				char lb_listener_id[48] = {0};
				UINT1 lb_listener_type=0;
				UINT4 check_frequency=0;
				UINT4 lb_lis_overtime=0;
				UINT1 lb_lis_retries=0;
				json_t *lbaas_listeners = json_find_first_label(json, "health_monitors");
				if(lbaas_listeners){
					json_t *lbaas_listener  = lbaas_listeners->child->child;
					while(lbaas_listener){
						temp = json_find_first_label(lbaas_listener, "id");
						strcpy(lb_listener_id,"");
						if(temp){
							if(temp->child->text){
								strcpy(lb_listener_id,temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_listener, "type");
						if(temp){
							if(temp->child->text && strcmp(temp->child->text,"PING")==0){
								lb_listener_type=LBAAS_LISTENER_PING;
								json_free_value(&temp);
							}else if(temp->child->text && strcmp(temp->child->text,"TCP")==0){
								lb_listener_type=LBAAS_LISTENER_TCP;
								json_free_value(&temp);
							}else if(temp->child->text && strcmp(temp->child->text,"HTTP")==0){
								lb_listener_type=LBAAS_LISTENER_HTTP;
								json_free_value(&temp);
							}else if(temp->child->text && strcmp(temp->child->text,"HTTPS")==0){
								lb_listener_type=LBAAS_LISTENER_HTTPS;
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_listener, "delay");
						if(temp){
							if(temp->child->text){
								check_frequency=atoll(temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_listener, "timeout");
						if(temp){
							if(temp->child->text){
								lb_lis_overtime=atoll(temp->child->text);
								json_free_value(&temp);
							}
						}
						temp = json_find_first_label(lbaas_listener, "max_retries");
						if(temp){
							if(temp->child->text){
								lb_lis_retries=atoi(temp->child->text);
								json_free_value(&temp);
							}
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
			}
		}
	}

	// show log
	show_create_port_log(stringType, totalNum);
}

void createPortForward(char* jsonString, port_forward_param_p forwardParam)
{
	const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;

	char forward_status[48] = {0};
	char forward_in_addr[48] = {0};
	char forward_protol[48] = {0};
	char forward_in_port[48] = {0};
	char forward_outside_port[48] = {0};
	char network_id[48] = {0};
	char external_ip[48] = {0};

	LOG_PROC("INFO", "%s ", jsonString);
	
    parse_type = json_parse_document(&json,tempString);
	if (parse_type != JSON_OK)
	{
	    LOG_PROC("INFO", "createPortForward failed");
		return;
	}

	if (json) 
	{
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
                            if(temp && temp->child && temp->child->text)
                            {
                                strcpy(network_id, temp->child->text);
                                json_free_value(&temp);
                            }

                            temp = json_find_first_label(pGateWayInfo->child, "external_fixed_ips");
                            if(temp && temp->child && temp->child->child)
                            {
                                json_t* extIpObj = json_find_first_label(temp->child->child, "ip_address");
                                if(extIpObj)
                                {
                                    strcpy(external_ip, extIpObj->child->text);
                                    json_free_value(&extIpObj);
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
							if(temp && temp->child && temp->child->text)
							{
								strcpy(forward_status, temp->child->text);
								json_free_value(&temp);
							}

							temp = json_find_first_label(pPortForwardObj, "inside_addr");
							if(temp && temp->child && temp->child->text)
							{
								strcpy(forward_in_addr, temp->child->text);
								json_free_value(&temp);
							}

							temp = json_find_first_label(pPortForwardObj, "protocol");
							if(temp&&temp->child && temp->child->text)
							{
								strcpy(forward_protol, temp->child->text);
								json_free_value(&temp);
							}

							temp = json_find_first_label(pPortForwardObj, "inside_port");
							if(temp && temp->child && temp->child->text)
							{
								strcpy(forward_in_port, temp->child->text);
								json_free_value(&temp);
							}

							temp = json_find_first_label(pPortForwardObj, "outside_port");
							if(temp && temp->child && temp->child->text)
							{
								strcpy(forward_outside_port, temp->child->text);
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

	}
}

//by:yhy 刷新security_group 信息
void reload_security_group_info()
{
	INT1 *value = NULL;

	// config & check openstack 
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	g_openstack_on = (NULL == value)?0:atoll(value);
	if( 1 == g_openstack_on)
	{
		// get openstack ip
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.52.200", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		// get port;
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoll(value));

		getOpenstackInfo(g_openstack_ip, "/v2.0/security-group-rules", g_openstack_port, "security-group-rules" , NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip, "/v2.0/ports", g_openstack_port, "port", NULL, EOPENSTACK_GET_NORMAL);
		if(!FirstTime_GetSecurityInfo)
		{
			update_From_NewSecurityGroup_To_OldSecurityGroup(g_openstack_security_list_temp,g_openstack_security_list);
			update_From_NewSecurityInfo_To_OldSecurityInfo(g_fabric_host_list.list);
			Clear_g_openstack_security_list_temp();
		}
		else
		{
			FirstTime_GetSecurityInfo=0;
		}
	} 

}
//by:yhy 刷新openstack的net信息
void reload_net_info()
{
	INT1 *value = NULL;
	// config & check openstack
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	g_openstack_on = (NULL == value)?0:atoll(value);
	if( 1 == g_openstack_on)
	{
		// get openstack ip
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.52.200", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		// get port;
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoll(value));

		getOpenstackInfo(g_openstack_ip,"/v2.0/networks",g_openstack_port,"network", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/subnets",g_openstack_port,"subnet" , NULL, EOPENSTACK_GET_NORMAL);
	} 

}
//by:yhy 此处因为reload
void reoad_lbaas_info()
{
	INT1 *value = NULL;

	// config & check openstack
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	g_openstack_on = (NULL == value)?0:atoll(value);
	if( 1 == g_openstack_on){
		// get openstack ip
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.52.200", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		// get port;
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoll(value));

		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/pools",g_openstack_port,"pools", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/vips",g_openstack_port,"vips", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/members",g_openstack_port,"lbmem" , NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/health_monitors",g_openstack_port,"lblistener", NULL, EOPENSTACK_GET_NORMAL);
	} 
}

//by:yhy openstack中需定时刷新的服务
void reload_tx_timer(void *para, void *tid)
{
	LOG_PROC("TIMER", "reload_tx_timer start");
	reload_openstack_host_network();
        reload_security_group_info();
	reload_floating_ip();
	reload_openstack_lbaas_info();
        //reload_routers();
        reload_port_forward();
	LOG_PROC("TIMER", "reload_tx_timer stop");
}

void reload_routers()
{
	if( 1 == g_openstack_on)
	{
		getOpenstackInfo(g_openstack_ip,"/v2.0/routers",g_openstack_port,"routers", NULL, EOPENSTACK_GET_NORMAL);
	}
}

void reload_port_forward()
{
    openstack_port_forward_p copy_list = NULL;
    openstack_port_forward_p old_list = NULL;
    openstack_port_forward_p new_list = NULL;
    port_forward_proc_p forwardProc = NULL;
    openstack_port_forward_p copy_node = NULL;
    
    if( 1 == g_openstack_on)
    {       
        port_forward_param forwardParam;
        forwardParam.list_header = NULL;

        getOpenstackInfo(g_openstack_ip,"/v2.0/routers",g_openstack_port,"port_forward", &forwardParam, EOPENSTACK_GET_PORTFORWARD);
		

      /*char* pString="{\"routers\": [{\"status\": \"ACTIVE\", \"external_gateway_info\": {\"network_id\": \"6b8cc3ab-cb41-4f86-b645-a2d90dfdf41a\", \"enable_snat\": true, \"external_fixed_ips\": [{\"subnet_id\": \"3d2074dd-a2dd-477c-8a9a-04664fab9169\", \"ip_address\": \"172.16.49.103\"}]}, \"availability_zone_hints\": [], \"availability_zones\": [\"nova\"], \"portforwardings\": [{\"status\": \"ENABLE\", \"inside_addr\": \"10.10.10.100\", \"protocol\": \"tcp\", \"outside_port\": \"80\", \"inside_port\": \"80\"}], \"name\": \"hujin_test\", \"gw_port_id\": \"12a4a376-583e-42ab-a3f8-2ea0667c38ca\", \"admin_state_up\": true, \"tenant_id\": \"aeb30280fa7e4085877cc707f54e380b\", \"distributed\": false, \"routes\": [], \"ha\": false, \"id\": \"1925c0bb-5771-4aef-b11e-dd24d2f284b8\", \"vlan_id\": null, \"description\": \"\"}, {\"status\": \"ACTIVE\", \"external_gateway_info\": {\"network_id\": \"6b8cc3ab-cb41-4f86-b645-a2d90dfdf41a\", \"enable_snat\": true, \"external_fixed_ips\": [{\"subnet_id\": \"3d2074dd-a2dd-477c-8a9a-04664fab9169\", \"ip_address\": \"172.16.49.104\"}]}, \"availability_zone_hints\": [], \"availability_zones\": [\"nova\"], \"portforwardings\": [], \"name\": \"router\", \"gw_port_id\": \"ac8aaf5c-aa8c-4366-94a5-0307a32da7b9\", \"admin_state_up\": true, \"tenant_id\": \"aeb30280fa7e4085877cc707f54e380b\", \"distributed\": false, \"routes\": [], \"ha\": false, \"id\": \"01d88f75-1b15-4c22-ac5d-01bde9ff7ae9\", \"vlan_id\": null, \"description\": \"\"}, {\"status\": \"ACTIVE\", \"external_gateway_info\": {\"network_id\": \"6b8cc3ab-cb41-4f86-b645-a2d90dfdf41a\", \"enable_snat\": true, \"external_fixed_ips\": [{\"subnet_id\": \"3d2074dd-a2dd-477c-8a9a-04664fab9169\", \"ip_address\": \"172.16.49.108\"}]}, \"availability_zone_hints\": [], \"availability_zones\": [\"nova\"], \"portforwardings\": [], \"name\": \"liusu_router01\", \"gw_port_id\": \"1fd212f5-1fb9-4fbe-82ef-f8011ed687a8\", \"admin_state_up\": true, \"tenant_id\": \"aeb30280fa7e4085877cc707f54e380b\", \"distributed\": false, \"routes\": [], \"ha\": false, \"id\": \"159ab664-e87e-4e22-8b01-1d8600541dbd\", \"vlan_id\": null, \"description\": \"\"}, {\"status\": \"ACTIVE\", \"external_gateway_info\": null, \"availability_zone_hints\": [], \"availability_zones\": [\"nova\"], \"portforwardings\": [], \"name\": \"test\", \"gw_port_id\": null, \"admin_state_up\": true, \"tenant_id\": \"d2c7a47970024a269c79bf3ac6367fec\", \"distributed\": false, \"routes\": [], \"ha\": false, \"id\": \"96d94bc2-24c6-4449-90da-1cbf19c19e13\", \"vlan_id\": null, \"description\": \"\"}]}";
*/
        //createPortForward(pString, &forwardParam);
        
        openstack_port_forward_p oldForwardNode = local_openstack_forward_list;

        openstack_port_forward_p newForwardNode = NULL;

        bool bFind = true;
        //查找需要删除的列表
        while(oldForwardNode)
        {
            bFind = false;
            newForwardNode = forwardParam.list_header;

            while(newForwardNode)
            {
                if( 0 == strcmp(oldForwardNode->src_ip, newForwardNode->src_ip) &&
                        oldForwardNode->src_port == newForwardNode->src_port)
                {
                    if(oldForwardNode->dst_port == newForwardNode->dst_port &&
                            oldForwardNode->proto == newForwardNode->proto)
                    {
                        bFind = true;
                    }

                    break;
                }

                newForwardNode = newForwardNode->next;
            }

            if(!bFind)
            {
                copy_node = copy_openstack_portfoward(oldForwardNode);
                if(NULL != copy_node)
                {
                    copy_node->next = old_list;
                    old_list = copy_node;
                } 
            }

            oldForwardNode = oldForwardNode->next;
        }

        //查找新的列表

        newForwardNode = forwardParam.list_header ;


        while(newForwardNode)
        {
            //begin 备份列表
            copy_node = copy_openstack_portfoward(newForwardNode);
            if(NULL != copy_node)
            {
                copy_node->next = copy_list;
                copy_list = copy_node;
            }
            //End

            bFind = false;
            oldForwardNode =  local_openstack_forward_list;
            while(oldForwardNode)
            {
                if( 0 == strcmp(oldForwardNode->src_ip, newForwardNode->src_ip) &&
                        oldForwardNode->src_port == newForwardNode->src_port)
                {
                    if(oldForwardNode->dst_port == newForwardNode->dst_port &&
                            oldForwardNode->proto == newForwardNode->proto)
                    {
                        bFind = true;
                    }

                    break;
                }

                oldForwardNode = oldForwardNode->next;
            }

            if(!bFind)
            {
                newForwardNode->next = new_list;
                new_list = newForwardNode; 
            }

            newForwardNode = newForwardNode->next;
        } 

        //切换列表             
        destory_port_forward_list(local_openstack_forward_list);//释放旧的 
        local_openstack_forward_list = forwardParam.list_header;//存新的

        forwardProc = (port_forward_proc_p)gn_malloc(sizeof(port_forward_proc));
        if(NULL == forwardProc)
        {
            goto FAIL;	
        }

        p_Queue_node pNode = get_MsgSock_Node(g_pMsgNodeBuff); 

        if(NULL == pNode)
        {
            goto FAIL;
        }

        forwardProc->old_list = old_list;
        forwardProc->new_list = new_list;
        forwardProc->copy_list = copy_list;

        if(NULL != copy_list)
        {
		  LOG_PROC("INFO", "copy_list is not null.......................");
        }
        gst_msgsock_t newsockmsgProc = {0};

        newsockmsgProc.uiMsgType = PORT_FORWARD;
        newsockmsgProc.iSockFd = 0;      
        newsockmsgProc.uiSw_Index = 0;  
        newsockmsgProc.uiUsedFlag = 1; 
        newsockmsgProc.param = forwardProc;

        push_MsgSock_into_queue( &g_tProc_queue, pNode, &newsockmsgProc); 
        Write_Event(EVENT_PROC);
    }

    return;
FAIL:

    destory_port_forward_list(old_list);
    destory_port_forward_list(new_list);
    destory_port_forward_list(copy_list);

    if(NULL != forwardProc)
    {
        gn_free(&forwardProc);
    }
}

//by:yhy 初始化openstack的reload服务(即定时清空,然后重新从openstack控制节点的restfulAPI读入信息,刷新一遍,以防改动)
void init_openstack_reload()
{
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "reload_on");
	UINT4 reload_on_flag = ((NULL == value) ? 0 : atoll(value));
	if (reload_on_flag) 
	{
		value = get_value(g_controller_configure, "[openvstack_conf]", "reload_interval");
		g_reload_interval = ((NULL == value) ? 30 : atoll(value));
		g_reload_timerid = timer_init(1);
		timer_creat(g_reload_timerid, g_reload_interval, NULL, &g_reload_timer, reload_tx_timer);
	}
}

//by:yhy 初始化openstack相关的操作
//by:yhy !!!
void initOpenstackFabric()
{
	INT1 *value = NULL;
	//by:yhy config & check openstack
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	g_openstack_on = (NULL == value)?0:atoll(value);
	if( 1 == g_openstack_on)
	{
		//by:yhy get openstack ip
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.52.200", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		//by:yhy get port;
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoll(value));
		//by:yhy 169.254.169.254表示DHCP没有获取到IP
		g_openstack_fobidden_ip = inet_addr("169.254.169.254");
		
		//by:yhy 启动fabric_arp_flood_thread,fabric_flow_thread
		//start_fabric_thread();  // modify by ycy
		//by:yhy 初始化openstack_host所需要的一些全局变量
		init_openstack_host();
		//by:yhy 初始化openstack_external用到的全局变量
		init_openstack_external();
		//by:yhy 读取(并更新和持久化)openstack外部端口配置
		read_external_port_config();
		//by:yhy 初始化NAT并分配内存
		init_nat_mem_pool();
		//by:yhy 创建nat_host头结点
		init_nat_host();
		//by:yhy 初始化openstack中负载均衡服务(lbaas)所需的相关全局变量并分配内存
		init_openstack_lbaas();
		//by:yhy 初始化openstack的刷新服务(即定时清空)
		init_openstack_reload();
		//by:yhy 在上面清空操作后,从openstack控制节点读入信息
		getOpenstackInfo(g_openstack_ip,"/v2.0/networks",g_openstack_port,"network", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/subnets",g_openstack_port,"subnet", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/floatingips",g_openstack_port,"floating", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/security-group-rules",g_openstack_port,"security-group-rules", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/ports",g_openstack_port,"port", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/pools",g_openstack_port,"pools", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/vips",g_openstack_port,"vips", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/members",g_openstack_port,"lbmem", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/health_monitors",g_openstack_port,"lblistener", NULL, EOPENSTACK_GET_NORMAL);

                reload_port_forward();

		
		FirstTime_GetSecurityInfo=0;
		LOG_PROC("INFO", "Init Openstack service finished");
	}
	else
	{
		LOG_PROC("INFO", "Init Openstack service Failed");
	}
//	show_openstack_total();
}



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

void createPortFabric(char* jsonString,const char* stringType);
static char * g_openstack_server_name;
UINT4 g_openstack_port = 9696;
char g_openstack_ip[16] = {0};
UINT4 g_openstack_on = 0;
UINT4 g_openstack_fobidden_ip = 0;
INT4 numm = 0;
//获取token id
void getNewTokenId(char *ip,char *tenantName,char *username,char *password)
{
	int sockfd, ret, i, h;
    struct sockaddr_in servaddr;
	char str1[4096], str2[4096], buf[1024], *str;
	socklen_t len;
	fd_set   t_set1;
	struct timeval  tv;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			printf("socket error!\n");
			exit(0);
	};
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(35357);
	if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0 ){
			printf("net_pton error!\n");
			exit(0);
	};

	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
			printf("connect error!\n");
			exit(0);
	}

	//·¢ËÊ¾Ý        memset(str2, 0, 4096);
	strcat(str2, "{\"auth\": {\"tenantName\": \"");
	strcat(str2,tenantName);
	strcat(str2,"\",\"passwordCredentials\": {\"username\":\"");
	strcat(str2,username);
	strcat(str2,"\",\"password\":\"");
	strcat(str2,password);
	strcat(str2,"\"}}}");
	str=(char *)malloc(128);
	len = strlen(str2);
	sprintf(str, "%d", len);

	memset(str1, 0, 4096);
	strcat(str1, "POST /v2.0/tokens HTTP/1.1\n");
	strcat(str1, "Content-Type: application/json\n");
	strcat(str1, "Content-Length: ");
	strcat(str1, str);
	strcat(str1, "\n\n");

	strcat(str1, str2);
	strcat(str1, "\r\n\r\n");

	ret = send(sockfd,str1,strlen(str1),0);  
	if (ret < 0) {
			exit(0);
	}
	FD_ZERO(&t_set1);
	FD_SET(sockfd, &t_set1);

	while(1){
			usleep(1000000);
			tv.tv_sec= 0;
			tv.tv_usec= 0;
			h= 0;
			h= select(sockfd +1, &t_set1, NULL, NULL, &tv);

			//if (h == 0) continue;
			if (h < 0) {
					close(sockfd);
			};

			if (h > 0){
					memset(buf, 0, 4096);
					i= read(sockfd, buf, 4095);
					if (i==0){
							close(sockfd);
							return;
					}
			}
			const char *p = strstr(buf,"\r\n\r\n");
			INT4 parse_type = 0;
			json_t *json=NULL;
			parse_type = json_parse_document(&json,p);
			if (parse_type != JSON_OK)
			{
				return;
			}else{
				if (json) {
					json_t *access=NULL,*token = NULL;
					access = json_find_first_label(json, "access");
					if(access){
						json_t *access_t  = access->child;
						if(access_t){
							token = json_find_first_label(access_t, "token");
							if(token){
								json_t *token_t = token->child;
								if(token_t){
									json_t *token_id = json_find_first_label(token_t,"id");
									if(token_id){
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
	}
	close(sockfd);
	free(buf);
}

//获得openstack北桥数据
void getOpenstackInfo(char *ip,char *url,int port,char *stringType){
	char *string;
	int sockfd, ret, i, h;
    struct sockaddr_in servaddr;
	char str1[4096], str2[4096], buf[4096], *str,str3[819200];
	socklen_t len;
	fd_set   t_set1;
	struct timeval  tv;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			printf("socket error!\n");
			exit(0);
	};
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0 ){
			printf("net_pton error!\n");
			exit(0);
	};

	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
			printf("connect error!\n");
			exit(0);
	}
    getNewTokenId(ip,"admin","admin","admin");
	//·¢ËÊ¾Ý        memset(str2, 0, 4096);
	strcat(str2, "{}");
	str=(char *)malloc(128);
	len = strlen(str2);
	sprintf(str, "%d", len);

	memset(str1, 0, 4096);
    strcat(str1, "GET ");
	strcat(str1,url);
	strcat(str1, " HTTP/1.1\n");
	strcat(str1, "Content-Type:application/json\n");
	strcat(str1,"X-Auth-Token:");
	strcat(str1,g_openstack_server_name);
	strcat(str1,"\n\n");
	strcat(str1, "Content-Length: ");
	strcat(str1, str);
	strcat(str1, "\n\n");

	strcat(str1, str2);
	strcat(str1, "\r\n\r\n");

	ret = send(sockfd,str1,strlen(str1),0);  
	if (ret < 0) {
			exit(0);
	}
	FD_ZERO(&t_set1);
	FD_SET(sockfd, &t_set1);
	memset(str3, 0, 819200);
	while(1){
			usleep(1000000);
			tv.tv_sec= 0;
			tv.tv_usec= 0;
			h= 0;
			h= select(sockfd +1, &t_set1, NULL, NULL, &tv);

			//if (h == 0) continue;
			if (h < 0) {
					close(sockfd);
			};

			if (h > 0){
					memset(buf, 0, 4096);
					i= read(sockfd, buf, 4095);
					strcat(str3,buf);
					if (i==0){
							close(sockfd);
							break;
					}
			}
	}
	char *p = strstr(str3,"\r\n\r\n");
	char *strend = "<head>";
    char *p1 = strstr(p,strend);
	int endindex=p1-p;
	if(endindex>0)  {
        string=(char*)malloc((endindex)*sizeof(char));
        strncpy(string, p+1, endindex-1); //
        string[endindex-1]='\0';
//        printf("%s\n", string);
    }
	close(sockfd);
	createPortFabric(string,stringType);
	if(string != NULL)
		free(string);
}

void initOpenstackFabric(){
	// config
	INT1 *value = NULL;

	// config & check openstack
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	g_openstack_on = (NULL == value)?0:atoi(value);
	if( 1 == g_openstack_on){
		// get openstack ip
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.52.200", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		// get port;
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoi(value));
		//
		g_openstack_fobidden_ip = inet_addr("169.254.169.254");

		start_fabric_thread();

		init_openstack_host();
		init_openstack_external();
		read_external_port_config();
		init_nat_mem_pool();
		init_nat_host();
		getOpenstackInfo(g_openstack_ip,"/v2.0/networks",g_openstack_port,"network");
		getOpenstackInfo(g_openstack_ip,"/v2.0/subnets",g_openstack_port,"subnet");
		getOpenstackInfo(g_openstack_ip,"/v2.0/ports",g_openstack_port,"port");
		getOpenstackInfo(g_openstack_ip,"/v2.0/floatingips",g_openstack_port,"floating");
		getOpenstackInfo(g_openstack_ip,"/v2.0/security-group-rules",g_openstack_port,"security-group-rules");
		LOG_PROC("INFO", "Init Openstack service finished");
	}else{
		LOG_PROC("INFO", "Init Openstack service Failed");
	}
//	show_openstack_total();
}
void updateOpenstackFloating(){
	// config
	INT1 *value = NULL;

	// config & check openstack
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	g_openstack_on = (NULL == value)?0:atoi(value);
	if( 1 == g_openstack_on){
		// get openstack ip
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.53.51", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		// get port;
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoi(value));
		//
		getOpenstackInfo(g_openstack_ip,"/v2.0/floatingips",g_openstack_port,"floating");
		LOG_PROC("INFO", "Openstack Floating Info Updated");
	}else{
		LOG_PROC("INFO", "Openstack Floating Info Update Failed");
	}
}

void createPortFabric( char *jsonString,const char *stringType){
    const char *tempString = jsonString;
	INT4 parse_type = 0;
	UINT2 totalNum = 0;
	json_t *json=NULL,*temp=NULL;
	char* networkType="network",*subnetType="subnet",*portType="port",*floatingType="floating";
	char* securityType = "security-group-rules";
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
	UINT2 security_num = 0;
	UINT1* security_port_p = NULL;

	parse_type = json_parse_document(&json,tempString);
	if (parse_type != JSON_OK)
	{
		return;
	}else{
		if (json) {
			if(strcmp(stringType,networkType)==0){
				json_t *networks = json_find_first_label(json, "networks");
//				char *tenant_id,*network_id;
			    UINT1 shared=0;
				if(networks){
					json_t *network  = networks->child->child;
					while(network){
						temp = json_find_first_label(network, "tenant_id");
						if(temp){
							//tenant_id = temp->child->text;
							strcpy(tenant_id,temp->child->text);

							json_free_value(&temp);
						}
						temp = json_find_first_label(network, "id");
						if(temp){
							//network_id = temp->child->text;
							strcpy(network_id,temp->child->text);
							json_free_value(&temp);
						}
						temp = json_find_first_label(network, "shared");
						if(temp){
							if(temp->child->type==JSON_TRUE){
								shared=1;
							}else if(temp->child->type==JSON_FALSE){
								shared=0;
							}else{
							}
							json_free_value(&temp);
						}
						update_openstack_app_network(tenant_id,network_id,shared);
						network=network->next;
						totalNum ++;
					}
					json_free_value(&networks);
					LOG_PROC("INFO","OPENSTACK NETWORK  UPDATE!   [%d] updated!",totalNum);
					totalNum=0;
				}
			}else if(strcmp(stringType,subnetType)==0){
//				char *tenant_id,*network_id,*subnet_id,*cidr;
				INT4 gateway_ip = 0, start_ip = 0, end_ip = 0;
				json_t *subnets = json_find_first_label(json, "subnets");
				UINT4 longtemp = 0;
				if(subnets){
					json_t *subnet  = subnets->child->child;
					while(subnet){
						temp = json_find_first_label(subnet, "tenant_id");
						if(temp){
							strcpy(tenant_id,temp->child->text);
							json_free_value(&temp);
						}
						temp = json_find_first_label(subnet, "network_id");
						if(temp){
							strcpy(network_id,temp->child->text);
							json_free_value(&temp);
						}
						temp = json_find_first_label(subnet, "id");
						if(temp){
							strcpy(subnet_id,temp->child->text);
							json_free_value(&temp);
						}
						temp = json_find_first_label(subnet, "cidr");
						if(temp){
							strcpy(cidr,temp->child->text);
							json_free_value(&temp);
						}
						temp = json_find_first_label(subnet, "gateway_ip");
						if(temp){
							longtemp = inet_addr(temp->child->text) ;
							gateway_ip = longtemp;
							json_free_value(&temp);
						}
						json_t *allocations = json_find_first_label(subnet, "allocation_pools");
						if(allocations){
							json_t *allocation = allocations->child->child;
							while(allocation){
								temp = json_find_first_label(allocation, "start");
								if(temp){
									longtemp  = inet_addr(temp->child->text) ;
									start_ip = longtemp;
									json_free_value(&temp);
								}
								temp = json_find_first_label(allocation, "end");
								if(temp){
									longtemp = inet_addr(temp->child->text) ;
									end_ip = longtemp;
									json_free_value(&temp);
								}
								allocation=allocation->next;
							}
							json_free_value(&allocations);
						}
						update_openstack_app_subnet(tenant_id,network_id,subnet_id,gateway_ip,start_ip,end_ip,cidr);
						subnet=subnet->next;
						totalNum++;
					}
					json_free_value(&subnets);
					LOG_PROC("INFO","OPENSTACK SUBNETS  UPDATE!   [%d] updated!",totalNum);
					totalNum=0;
				}

			}else if(strcmp(stringType,portType)==0){
				//char *tenant_id,*network_id,*subnet_id,*port_id;
				char port_type[40] = {0};
				char* computer="compute:nova";
				char* dhcp="network:dhcp";
				char* floating = "network:floatingip";
				INT4 ip = 0;
				UINT1 mac[6]={0};
				UINT4 port_number = 0;
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
									UINT4 longtemp = inet_addr(temp->child->text) ;
									ip = longtemp;
									json_free_value(&temp);
								}
								fix_ip=fix_ip->next;
							}
							json_free_value(&fix_ips);
						}

						json_t *security_groups = json_find_first_label(port, "security_groups");
						// printf("security_groups\n");
						if (security_groups) {
							json_t *security_group = security_groups->child->child;
							openstack_node_p head_p = NULL;
							security_num = 0;
							security_port_p = NULL;
							while (security_group) {
								openstack_security_p temp_p = update_openstack_security_group(security_group->text);
								head_p = add_openstack_host_security_node((UINT1*)temp_p, head_p);
								security_port_p = (UINT1*)head_p;
								security_num++;
								security_group = security_group->next;
							}
							json_free_value(&security_groups);
						}

						if(strcmp(port_type,computer)==0|| strcmp(port_type,floating)==0){
//							LOG_PROC("INFO","PORT UPDATE!");
							update_openstack_app_host_by_rest(NULL,port_number,ip,mac,tenant_id,network_id,subnet_id,port_id,security_num,security_port_p);
						}else if(strcmp(port_type,dhcp)==0){
//							LOG_PROC("INFO","DHCP UPDATE!");
							update_openstack_app_dhcp_by_rest(NULL,port_number,ip,mac,tenant_id,network_id,subnet_id,port_id);
						}else{
//							LOG_PROC("INFO","GATEWAY UPDATE!");
							update_openstack_app_gateway_by_rest(NULL,port_number,ip,mac,tenant_id,network_id,subnet_id,port_id);
						}
						port=port->next;
						totalNum++;
					}
					json_free_value(&ports);
					LOG_PROC("INFO","OPENSTACK PORTS  UPDATE!   [%d] updated!",totalNum);
					totalNum=0;
				}
			}
			else if(strcmp(stringType,floatingType)==0){
				json_t *floatings = json_find_first_label(json, "floatingips");
				UINT4 fixed_ip;//inner ip
				UINT4 floating_ip;//outer ip
				if(floatings){
					json_t *floating_ip_one  = floatings->child->child;
					while(floating_ip_one){
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
						create_floatting_ip_by_rest(fixed_ip,floating_ip,port_id,router_id);
						floating_ip_one=floating_ip_one->next;
						totalNum++;
					}
					json_free_value(&floatings);
					LOG_PROC("INFO","OPENSTACK FLOATINGIP  UPDATE!   [%d] updated!",totalNum);
					totalNum=0;
				}
			}
			else if (0 == strcmp(stringType, securityType)) {
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

						
						update_security_rule(security_group_id,rule_id,direction,ethertype,port_range_max,port_range_min,
								protocol,remote_group_id,remote_ip_prefix,security_tenant_id);
						security_group = security_group->next;
						totalNum++;
					}
					json_free_value(&security_groups);
					LOG_PROC("INFO","OPENSTACK SECURITY UPDATE!   [%d] updated!",totalNum);
					totalNum=0;
				}
			}
		}
	}
}

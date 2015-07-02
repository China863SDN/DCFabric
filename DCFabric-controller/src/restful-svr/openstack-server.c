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

void createPortFabric(char* jsonString,const char* stringType);
static char * g_openstack_server_name;
UINT4 g_openstack_port = 9696;
char g_openstack_ip[16] = {0};
UINT4 g_openstack_on = 0;
UINT4 g_openstack_fobidden_ip = 0;
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
		LOG_PROC("INFO", "Init Openstack service finished");
		// get openstack ip
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.52.200", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		// get port;
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoi(value));
		//
		g_openstack_fobidden_ip = inet_addr("169.254.169.254");

		init_openstack_host();
		getOpenstackInfo(g_openstack_ip,"/v2.0/networks",g_openstack_port,"network");
		getOpenstackInfo(g_openstack_ip,"/v2.0/subnets",g_openstack_port,"subnet");
		getOpenstackInfo(g_openstack_ip,"/v2.0/ports",g_openstack_port,"port");
	}else{
		LOG_PROC("INFO", "Init Openstack service Failed");
	}
//	show_openstack_total();
}
void createPortFabric( char *jsonString,const char *stringType){
    const char *tempString = jsonString;
	INT4 parse_type = 0;
	json_t *json=NULL,*temp=NULL;
	char* networkType="network",*subnetType="subnet",*portType="port";
	char tenant_id[48] ={0};
	char network_id[48] = {0};
	char subnet_id[48] = {0};
	char port_id[48] = {0};
	char cidr[30] = {0};
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
					}
					json_free_value(&networks);
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
					}
					json_free_value(&subnets);
				}

			}else if(strcmp(stringType,portType)==0){
				//char *tenant_id,*network_id,*subnet_id,*port_id;
				char port_type[40] = {0};
				char* computer="compute:nova";
				char* dhcp="network:dhcp";
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
						if(strcmp(port_type,computer)==0){
//							LOG_PROC("INFO","PORT UPDATE!");
							update_openstack_app_host_by_rest(NULL,port_number,ip,mac,tenant_id,network_id,subnet_id,port_id);
						}else if(strcmp(port_type,dhcp)==0){
//							LOG_PROC("INFO","DHCP UPDATE!");
							update_openstack_app_dhcp_by_rest(NULL,port_number,ip,mac,tenant_id,network_id,subnet_id,port_id);
						}else{
//							LOG_PROC("INFO","GATEWAY UPDATE!");
							update_openstack_app_gateway_by_rest(NULL,port_number,ip,mac,tenant_id,network_id,subnet_id,port_id);
						}
						port=port->next;
					}

					json_free_value(&ports);
				}
			}
		}
	}
}

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
#include "fabric_openstack_gateway.h"
#include "openstack_lbaas_app.h"
#include "timer.h"
#include "openstack_routers.h"
#include "openstack_qos_app.h"
#include "../conn-svr/conn-svr.h"
#include "event.h"
#include "openstack-server.h"
#include "openstack_clbaas_app.h"
#include "openstack_dataparse.h"


static char * g_openstack_server_name;
UINT4 g_openstack_port = 9696;
char g_openstack_ip[16] = {0};
//by:yhy 全局标志 openstack是否启用;由配置文件中读取
UINT4 g_openstack_on = 0;
//by:yhy openstack特殊服务用保留IP,被初始化成169.254.169.254 此ip表示DHCP没有获得到ip,便分配成这个保留IP
UINT4 g_openstack_fobidden_ip = 0;



//定时刷新
void 	*g_reload_timer 	= NULL;
void 	*g_reload_timerid 	= NULL;
UINT4 	 g_reload_interval 	= 30;

UINT1 	Openstack_Info_Ready_flag =0;
//端口转发列表
static struct _openstack_port_forward *  local_openstack_forward_list = NULL;






stopenstack_parse  CreateOpenstackInfo[] =
{
	{"network", 				EOPENSTACK_GET_NORMAL, 			createOpenstackNetwork			},
	{"router", 					EOPENSTACK_GET_NORMAL, 			createOpenstackRouter				},
	{"subnet", 					EOPENSTACK_GET_NORMAL, 			createOpenstackSubnet				},
	{"port",   					EOPENSTACK_GET_NORMAL, 			createOpenstackPort				},
	{"floating",   				EOPENSTACK_GET_NORMAL, 			createOpenstackFloating			},
	{"security-group-rules",   	EOPENSTACK_GET_NORMAL, 			createOpenstackSecurity			},
		
	{"pools",   				EOPENSTACK_GET_LOADBALANCE, 	createOpenstackLbaaspools		},
	{"vips",   					EOPENSTACK_GET_LOADBALANCE, 	createOpenstackLbaasvips			},
	{"lbmem",   				EOPENSTACK_GET_LOADBALANCE, 	createOpenstackLbaasmember		},
	{"lblistener",   			EOPENSTACK_GET_LOADBALANCE, 	createOpenstackLbaaslistener		},

	{"port_forward",   			EOPENSTACK_GET_PORTFORWARD, 	createOpenstackPortforward		},
	{"clbpools",   				EOPENSTACK_GET_CLBLOADBALANCE, 	createOpenstackClbaaspools		},
	{"clbvips",   				EOPENSTACK_GET_CLBLOADBALANCE, 	createOpenstackClbaasvips		},
	{"clbloadbalancers",		EOPENSTACK_GET_CLBLOADBALANCE,  createOpenstackClbaasloadbalancer},
	{"clbinterfaces",			EOPENSTACK_GET_CLBLOADBALANCE,	createOpenstackClbaasinterface	},
	{"clblistener",   			EOPENSTACK_GET_CLBLOADBALANCE, 	createOpenstackClbaaslistener	},
	{"clbbackends",   			EOPENSTACK_GET_CLBLOADBALANCE, 	createOpenstackClbaasbackend		},
	{"clbbackendlisten",   		EOPENSTACK_GET_CLBLOADBALANCE, 	createOpenstackClbaasbackendListen}
	

};


//by:yhy  从openstack的控制节点通过restfulAPI获取token id
//操作成功返回"1"失败返回"0"
int getNewTokenId(char *ip,char *tenantName,char *username,char *password)
{
    int sockfd = -1, ret, i, h, iErrno = 0;
    int Length_BuffSended =0;	
    INT4 sockopt = 1;
    struct sockaddr_in servaddr;
    char str1[4096], str2[4096], buf1[4096], buf2[819200], str[128];
    socklen_t len;
    fd_set   t_set1;
    struct timeval  tv;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        iErrno = errno; 
        LOG_PROC("ERROR", "%s -- socket error! Error code: %d",FN,iErrno);
        return 0;
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sockopt, sizeof(sockopt));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&sockopt, sizeof(sockopt));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(35357);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0)
    {
        iErrno = errno; 
        LOG_PROC("ERROR", "%s -- net_pton error! Error code: %d",FN,iErrno);
		close(sockfd);
        return 0;
    }

    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
#if 0
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        iErrno = errno; 
        LOG_PROC("ERROR", "%s -- connect error! Error code: %d",FN,iErrno);
        return 0;
    }
#endif

    memset(str2, 0, 4096);
    strcat(str2, "{\"auth\": {\"tenantName\": \"");
    strcat(str2, tenantName);
    strcat(str2, "\",\"passwordCredentials\": {\"username\":\"");
    strcat(str2, username);
    strcat(str2, "\",\"password\":\"");
    strcat(str2, password);
    strcat(str2, "\"}}}");
    memset(str, 0, 128);
    len = strlen(str2);
    sprintf(str, "%d", len);

    memset(str1, 0, 4096);
    strcat(str1, "POST /v2.0/tokens HTTP/1.0\r\n");
    strcat(str1, "Content-Type: application/json\r\n");
    strcat(str1, "Content-Length: ");
    strcat(str1, str);
    strcat(str1, "\r\n\r\n");

    strncat(str1, str2, 4096-strlen(str1)-5);
    strcat(str1, "\r\n\r\n");
    //LOG_PROC("INFO", "%s %d:%s",FN,LN,str1);

    Length_BuffSended = 0;
    while (Length_BuffSended != strlen(str1))
    {
        ret = send(sockfd, (str1+Length_BuffSended), strlen(str1+Length_BuffSended), 0); 
        if (ret < 0)
        {
            iErrno = errno;
            if ((EINTR == iErrno) || (EAGAIN == iErrno))
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
    while (1)
    {
        // usleep(1000000);
        tv.tv_sec = 3; //wait 3s for peer's answer
        tv.tv_usec = 0;
        FD_ZERO(&t_set1);
        FD_SET(sockfd, &t_set1);

        h = select(sockfd +1, &t_set1, NULL, NULL, &tv);
        if (h > 0) 
        {
            while (1)
            {
                memset(buf1, 0, 4096);
                i = read(sockfd, buf1, 4095);
                if (i > 0)
                {
                    strcat(buf2, buf1);
                }
                else if (i < 0)
                {
                    iErrno = errno;
                    if ((EAGAIN == iErrno) || (EINTR == iErrno))
                    {
                        //LOG_PROC("DEBUG", "%s %d:continue read",FN,LN);
                        continue;
                    }
                    else
                    {
                        LOG_PROC("ERROR", "%s %d:read error code: %d",FN,LN,iErrno);
                        close(sockfd);
                        return 0;
                    }
                }
                else
                {
                    //LOG_PROC("DEBUG", "%s %d:read finished",FN,LN);
                    break;
                }
            }

            break; //after all data received, stop select.
        }
        else if (h < 0)
        {
            iErrno = errno;
            if ((EAGAIN == iErrno) || (EINTR == iErrno))
            {
                LOG_PROC("DEBUG", "%s %d:continue select",FN,LN);
                continue;
            }
            else
            {
                LOG_PROC("ERROR", "%s %d:select error code: %d",FN,LN,iErrno);
                close(sockfd);
                return 0;
            }
        } else {
            LOG_PROC("WARNING", "%s %d:select timeout within 3s",FN,LN);
            break;
        }
    }

    close(sockfd);
    sockfd = -1;

    const char *p = strstr(buf2,"\r\n\r\n");
    if (NULL == p)
    {
        LOG_PROC("ERROR", "%s %d buf2 = %s",FN,LN,buf2);
        return 0;
    }

    INT4 parse_type = 0;
    json_t *json=NULL;
	//LOG_PROC("INFO", "%s %d:%s",FN,LN,p);
    parse_type = json_parse_document(&json,p);
    if((parse_type != JSON_OK)||(NULL == json))
    {
		LOG_PROC("ERROR", "%s %d:json_parse_document failed",FN,LN);
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
                            if(token_id&&token_id->child)
                            {
                                //g_openstack_server_name = token_id->child->text;
                                strcpy(g_openstack_server_name, token_id->child->text);
								json_free_value(&token_id);
                            }
							json_free_value(&token_t);
                        }
						json_free_value(&token);
                    }
					json_free_value(&access_t);
                }
				json_free_value(&access);
            }
			json_free_value(&json);
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
INT4 getOpenstackInfo(char *ip,char *url,int port,char *stringType, void* param, enum EOpenStack_GetType getType){
	char *string;
	int sockfd, ret, i, h,iErrno = 0;
	int Length_BuffSended =0;
    struct sockaddr_in servaddr;
	char str1[4096], str2[4096], buf[4096], str[128], str3[819200];
	socklen_t len;
	fd_set   t_set1;
	struct timeval  tv;
	int k = 0;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		iErrno = errno;	
		LOG_PROC("ERROR", "%s -- socket error! iErrno: %d",FN,iErrno);
		return GN_ERR;
	}
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0)
	{
		iErrno = errno; 
		LOG_PROC("ERROR", "%s -- net_pton error! iErrno: %d",FN,iErrno);
		return GN_ERR;
	};

	//by:yhy 捆绑套接字
	connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
#if 0
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		iErrno = errno;
		LOG_PROC("ERROR", "%s -- connect error! iErrno: %d",FN,iErrno);
		return GN_ERR;
	}
#endif

	char str_tenantname[48] = {0};
	char str_username[48] = {0};
	char str_password[48] = {0};
	
	INT1 *value = get_value(g_controller_configure, "[openvstack_conf]", "tenantname");
    NULL == value ? strncpy(str_tenantname, "admin", 48 - 1) : strncpy(str_tenantname, value, 48 - 1);

	value = get_value(g_controller_configure, "[openvstack_conf]", "username");
	NULL == value ? strncpy(str_username, "admin", 48 - 1) : strncpy(str_username, value, 48 - 1);

	value = get_value(g_controller_configure, "[openvstack_conf]", "password");
	NULL == value ? strncpy(str_password, "admin", 48 - 1) : strncpy(str_password, value, 48 - 1);

	memset(g_openstack_server_name, 0, 1024);
	 
	//by:yhy 获取token
    if (0 == getNewTokenId(ip, str_tenantname, str_username, str_password))	
	{
		LOG_PROC("ERROR", "---%s %d  getNewTokenId",FN,LN);
		close(sockfd);
		return GN_ERR;
	}

	memset(str2, 0, 4096);
	strcat(str2, "{}");
    memset(str, 0, 128);
	len = strlen(str2);
	sprintf(str, "%d", len);

	memset(str1, 0, 4096);
    strcat(str1, "GET ");
	strcat(str1, url);
	strcat(str1, " HTTP/1.1\n");
	strcat(str1, "Content-Type:application/json\n");
	strcat(str1, "X-Auth-Token:");
	if (NULL == g_openstack_server_name) 
	{
		LOG_PROC("ERROR", "Openstack authentication failure! Please check the configuration.");
        close(sockfd);
        return GN_ERR;
	}
	strcat(str1, g_openstack_server_name);
	strcat(str1, "\n\n");
	strcat(str1, "Content-Length: ");
	strcat(str1, str);
	strcat(str1, "\n\n");
	strcat(str1, str2);
	strcat(str1, "\r\n\r\n");

    Length_BuffSended = 0;
	while (Length_BuffSended != strlen(str1))
	{
		ret = send(sockfd, (str1+Length_BuffSended), strlen(str1+Length_BuffSended), 0); 
		if (ret <0)
		{
			iErrno = errno;
			if ((EINTR == iErrno) || (EAGAIN == iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				close(sockfd);
				return GN_ERR;
			}
		}
		else
		{
			Length_BuffSended += ret;
		}
	}

	memset(str3, 0, 819200);
	int t_start = time(NULL);
	while (1)
	{
		// usleep(1000000);
		tv.tv_sec = 5;
		tv.tv_usec = 0;
        FD_ZERO(&t_set1);
        FD_SET(sockfd, &t_set1);

		h = select(sockfd +1, &t_set1, NULL, NULL, &tv);
		if (h > 0)
		{
            while (1)
            {
    			memset(buf, 0, 4096);
    			i = read(sockfd, buf, 4095);
    			if (i > 0) 
                {
    			    strcat(str3, buf);
                }
                else if (i < 0)
                {
                    iErrno = errno;
                    if ((EAGAIN == iErrno) || (EINTR == iErrno))
                    {
                        //LOG_PROC("DEBUG", "%s %d:continue read",FN,LN);
                        continue;
                    }
                    else
                    {
                        LOG_PROC("ERROR", "%s %d:read error code: %d url=%s",FN,LN,iErrno,url);
                        close(sockfd);
                        return 0;
                    }
                }
                else
                {
                    //LOG_PROC("DEBUG", "%s %d:read finished",FN,LN);
                    break;
                }
            }
            
            break; //after all data received, stop select.
		}
        else if (h < 0)
        {
			iErrno = errno;
			if (EAGAIN == iErrno || EINTR == iErrno)
			{
                LOG_PROC("DEBUG", "%s %d:continue select url=%s",FN,LN,url);
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,select error code: %d url=%s",FN,iErrno,url);
				close(sockfd);
				return GN_ERR;
			}
        } else {
            LOG_PROC("WARNING", "%s %d:select timeout within %d s url=%s",FN,LN,tv.tv_sec,url);
            break;
        }
	}

	close(sockfd);
    sockfd = -1;
	
    int t_end = time(NULL);
	if(t_end - t_start >= 3)
	{
		LOG_PROC("INFO", "%s %d:recv within %d s url=%s",FN,LN,t_end - t_start,url);
	}
	
	char *p = strstr(str3,"\r\n\r\n");
	if(NULL == p)
	{
		LOG_PROC("ERROR", "%s %d",FN,LN);
		return GN_ERR;
	}
	char *strend = "<head>";
    char *p1 = strstr(p,strend);
	if(NULL == p1)
	{
		LOG_PROC("ERROR", "%s %d",FN,LN);
		return GN_ERR;
	}
	int endindex=p1-p;
	if(endindex>0)  
	{
        string=(char*)malloc((endindex)*sizeof(char));
        strncpy(string, p+1, endindex-1); 
        string[endindex-1]='\0';
    }

	if(EOPENSTACK_GET_QOS == getType)
	{
		//LOG_PROC("INFO", "%s ", string);
		openstack_qos_rule_ConventDataFromJSON(string);
		
	}

	for (k=0; k < sizeof(CreateOpenstackInfo)/sizeof(stopenstack_parse); k++)
	{
		if((getType == CreateOpenstackInfo[k].eGetType)&&(0 == strcmp(stringType, CreateOpenstackInfo[k].stringType)))
		{
			//LOG_PROC("INFO", "%s \n", string);
			CreateOpenstackInfo[k].callback_func(string, param );
			break;
		}
	}
	if(string != NULL)
	{
		free(string);
	}
	return GN_OK;
}

//by:yhy 更新openstack的浮动IP
INT4 updateOpenstackFloating()
{
	INT4 iRet = GN_ERR;
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
		iRet = getOpenstackInfo(g_openstack_ip,"/v2.0/floatingips",g_openstack_port,"floating" , NULL, EOPENSTACK_GET_NORMAL);
		// LOG_PROC("INFO", "Openstack Floating Info Updated");
	}
	else
	{
		LOG_PROC("INFO", "Openstack Floating Info Update Failed");
		iRet = GN_ERR;
	}
	return iRet;
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




//by:yhy 刷新openstack的net信息
INT4 reload_net_info()
{
	INT4 iRet = GN_ERR;
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

		iRet = getOpenstackInfo(g_openstack_ip,"/v2.0/networks",g_openstack_port,"network", NULL, EOPENSTACK_GET_NORMAL);
		iRet += getOpenstackInfo(g_openstack_ip,"/v2.0/subnets",g_openstack_port,"subnet" , NULL, EOPENSTACK_GET_NORMAL);
	} 
	return iRet ;

}
//by:yhy 此处因为reload
INT4 reoad_lbaas_info()
{
	INT4 iRet = GN_ERR;
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

		iRet = 	getOpenstackInfo(g_openstack_ip,"/v2.0/lb/pools",g_openstack_port,"pools", NULL, EOPENSTACK_GET_LOADBALANCE);
		iRet += getOpenstackInfo(g_openstack_ip,"/v2.0/lb/vips",g_openstack_port,"vips", NULL, EOPENSTACK_GET_LOADBALANCE);
		iRet += getOpenstackInfo(g_openstack_ip,"/v2.0/lb/members",g_openstack_port,"lbmem" , NULL, EOPENSTACK_GET_LOADBALANCE);
		iRet += getOpenstackInfo(g_openstack_ip,"/v2.0/lb/health_monitors",g_openstack_port,"lblistener", NULL, EOPENSTACK_GET_LOADBALANCE);
	}
	return iRet ;
}

//by:yhy openstack中需定时刷新的服务
void reload_tx_timer(void *para, void *tid)
{
	LOG_PROC("INFO", "%s_%d-----------------------------------------------------------------------------------------------------------------",FN,LN);
	
	openstack_qos_ReloadRules();
	reload_openstack_host_network();
    reload_security_group_info();
	reload_floating_ip();
	reload_openstack_lbaas_info();
    reload_port_forward();
	reload_clbs_info();
	Openstack_Info_Ready_flag =1;
	LOG_PROC("INFO", "%s_%d-----------------------------------------------------------------------------------------------------------------",FN,LN);
}

INT4 reload_clbs_info()
{
	INT4 iRet = GN_ERR;
	if( 1 == g_openstack_on)
	{
		reset_openstack_clbaas_vipfloatingpoolflag();
		iRet = 	 getOpenstackInfo(g_openstack_ip,"/v2.0/routers",g_openstack_port,"router", NULL, EOPENSTACK_GET_NORMAL);
		iRet +=  getOpenstackInfo(g_openstack_ip,"/v2.0/clb/vips",g_openstack_port,"clbvips", NULL, EOPENSTACK_GET_CLBLOADBALANCE);
		if(GN_OK == iRet)
		{
			remove_openstack_clbaas_vipfloatingpool_unchecked();	
		}

		
		reset_openstack_clbaas_loadbalancerflag();
		iRet =  getOpenstackInfo(g_openstack_ip,"/v2.0/clb/load_balancers",g_openstack_port,"clbloadbalancers", NULL, EOPENSTACK_GET_CLBLOADBALANCE);
		iRet += getOpenstackInfo(g_openstack_ip,"/v2.0/clb/interfaces",g_openstack_port,"clbinterfaces", NULL, EOPENSTACK_GET_CLBLOADBALANCE);
		if(GN_OK == iRet)
		{
			remove_openstack_clbaas_loadbalancer_unchecked();
		}

		reset_openstack_clbaas_backendflag();
		iRet = getOpenstackInfo(g_openstack_ip,"/v2.0/clb/backends",g_openstack_port,"clbbackends", NULL, EOPENSTACK_GET_CLBLOADBALANCE);
		if(GN_OK == iRet)
		{
			remove_openstack_clbaas_backend_unchecked();
		}
	}
}
void reload_routers()
{
	if( 1 == g_openstack_on)
	{
		getOpenstackInfo(g_openstack_ip,"/v2.0/routers",g_openstack_port,"router", NULL, EOPENSTACK_GET_NORMAL);
	}
}

void reload_port_forward()
{
	INT4 iRet = GN_ERR;
    openstack_port_forward_p copy_list = NULL;
    openstack_port_forward_p old_list = NULL;
    openstack_port_forward_p new_list = NULL;
    port_forward_proc_p forwardProc = NULL;
    openstack_port_forward_p copy_node = NULL;
    
    if( 1 == g_openstack_on)
    {       
        port_forward_param forwardParam;
        forwardParam.list_header = NULL;

        iRet = getOpenstackInfo(g_openstack_ip,"/v2.0/routers",g_openstack_port,"port_forward", &forwardParam, EOPENSTACK_GET_PORTFORWARD);
		if(GN_OK != iRet)
		{
			return;
		}

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
                if((0 == strcmp(oldForwardNode->src_ip, newForwardNode->src_ip)) &&
                   (oldForwardNode->src_port_start == newForwardNode->src_port_start) &&
                   (oldForwardNode->src_port_end == newForwardNode->src_port_end))
                {
                    if((oldForwardNode->dst_port_start == newForwardNode->dst_port_start) &&
                       (oldForwardNode->dst_port_end == newForwardNode->dst_port_end) &&
                       (oldForwardNode->proto == newForwardNode->proto))
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
                if((0 == strcmp(oldForwardNode->src_ip, newForwardNode->src_ip)) &&
                   (oldForwardNode->src_port_start == newForwardNode->src_port_start) &&
                   (oldForwardNode->src_port_end == newForwardNode->src_port_end))
                {
                    if((oldForwardNode->dst_port_start == newForwardNode->dst_port_start) &&
                       (oldForwardNode->dst_port_end == newForwardNode->dst_port_end) &&
                       (oldForwardNode->proto == newForwardNode->proto))
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

        if(GN_OK == push_MsgSock_into_queue( &g_tProc_queue, pNode, &newsockmsgProc))
        {
        	Write_Event(EVENT_PROC);
        }
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

	g_openstack_server_name = gn_malloc(1024);
	
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
		//read_external_port_config();
		//by:yhy 初始化NAT并分配内存
		init_nat_mem_pool();
		//by:yhy 创建nat_host头结点
		init_nat_host();
		//by:yhy 初始化openstack中负载均衡服务(lbaas)所需的相关全局变量并分配内存
		init_openstack_lbaas();

		init_openstack_clbaas(); //ycy 华云LB
		init_openstack_router_gateway(); //ycy external outer interface
		
		//by:yhy 初始化openstack的刷新服务(即定时清空)
		init_openstack_reload();
		
		//by:yhy 在上面清空操作后,从openstack控制节点读入信息
		getOpenstackInfo(g_openstack_ip,"/v2.0/networks",g_openstack_port,"network", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/subnets",g_openstack_port,"subnet", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/floatingips",g_openstack_port,"floating", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/security-group-rules",g_openstack_port,"security-group-rules", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/ports",g_openstack_port,"port", NULL, EOPENSTACK_GET_NORMAL);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/pools",g_openstack_port,"pools", NULL, EOPENSTACK_GET_LOADBALANCE);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/vips",g_openstack_port,"vips", NULL, EOPENSTACK_GET_LOADBALANCE);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/members",g_openstack_port,"lbmem", NULL, EOPENSTACK_GET_LOADBALANCE);
		getOpenstackInfo(g_openstack_ip,"/v2.0/lb/health_monitors",g_openstack_port,"lblistener", NULL, EOPENSTACK_GET_LOADBALANCE);
		

        reload_port_forward();
		reload_clbs_info();

		LOG_PROC("INFO", "Init Openstack service finished");
	}
	else
	{
		LOG_PROC("INFO", "Init Openstack service Failed");
	}
//	show_openstack_total();
}



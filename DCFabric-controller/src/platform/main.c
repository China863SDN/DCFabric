/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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

/******************************************************************************
*                                                                             *
*   File Name   : main.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*   Modify      : 2015-5-19 by bnc
*                                                                             *
******************************************************************************/
#ifndef VERSION
#define VERSION 0x020000
#endif
#define PRODUCT "DCFabric"

#include "common.h"
#include "timer.h"
#include "../conn-svr/conn-svr.h"
#include "../restful-svr/restful-svr.h"
#include "../topo-mgr/topo-mgr.h"
#include "../meter-mgr/meter-mgr.h"
#include "../group-mgr/group-mgr.h"
#include "../tenant-mgr/tenant-mgr.h"
#include "../user-mgr/user-mgr.h"
#include "../stats-mgr/stats-mgr.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "../cluster-mgr/redis_client.h"
#include "../flow-mgr/flow-mgr.h"
#include "forward-mgr.h"
#include "../ovsdb/ovsdb.h"
#include "../event/event_service.h"
#include "../restful-svr/openstack-server.h"
#include "../fabric/fabric_impl.h"
#include "../openstack/fabric_openstack_external.h"
#include "../fabric/fabric_floating_ip.h"
#include "openstack_lbaas_app.h"
#include "../overload-mgr/overload-mgr.h"
#include "../qos-mgr/qos-mgr.h"
#include <sys/prctl.h>   


#define START_DATE __DATE__  // compile date.
#define START_TIME __TIME__  // compile time.
//by:yhy 全局 配置文件句柄
ini_file_t *g_controller_configure;
//by:yhy 全局 控制器MAC
UINT1 g_controller_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//by:yhy 全局 控制器IP
UINT4 g_controller_ip = 0;
//by:yhy 全局 保留MAC
UINT1 g_reserve_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//by:yhy 全局 保留IP(配置文件中有该参数,若未配置,则等于控制器IP)
UINT4 g_reserve_ip = 0;
UINT1 g_fabric_start_flag = 0;

void *g_auto_timer = NULL;
UINT1 g_auto_init_flag = 0;
void* auto_test_p = NULL;
//by:yhy 全局主从备份标志
UINT1 g_is_cluster_on = 0;

UINT4 g_max_nofile = 65535;

void show_copy_right()
{
//    struct tm *when;
//    time_t now;

    INT1 stars[] = {"*****************************************************************" };
    INT1 spaces[] = {"*                                                               *" };
    INT1 ver[81], info[81], author[81], company[81], copyright[81],author2[81];
    UINT4 len = strlen(stars);

//    now = time(NULL);
//    when = localtime(&now);

    sprintf(ver, "*                        %s v%d.%d.%d                         *", PRODUCT,
            (int)(VERSION & 0xFF0000) >> 16, (int) (VERSION & 0xFF00) >> 8, (int) (VERSION & 0xFF));

    if (strlen(ver) != len)
    {
        ver[len - 1] = '*';
        ver[len] = '\0';
    }

    sprintf(info, "*               Started at %8s, on%12s             *", START_TIME, START_DATE);
    sprintf(author, "*                GreenNet: XueQiuBao, DengChao                  *");
    sprintf(author2, "*                    BNC: ZhaoLiangZhi, YangLei                 *");
    sprintf(company, "*          Copyright GreenNet & BNC 2015-10 ~ 2099-10           *");
    sprintf(copyright, "*                  (c) All Rights Reserved.                     *");

    printf("\n");
    printf("%s\n", stars);
    printf("%s\n", spaces);
    printf("%s\n", ver);
    printf("%s\n", info);
    printf("%s\n", spaces);
    printf("%s\n", author);
    printf("%s\n", author2);
    printf("%s\n", spaces);
    printf("%s\n", company);
    printf("%s\n", copyright);
    printf("%s\n", stars);
}

INT4 read_configuration()
{
    g_controller_configure = read_ini(CONFIGURE_FILE);
    if(NULL == g_controller_configure)
    {
        LOG_PROC("ERROR", "%s", "Get controller configuration failed");
        return GN_ERR;
    }
    
	//
	INT1* value = NULL;
	value = get_value(g_controller_configure, "[controller]", "cluster_on");
    g_is_cluster_on = (NULL == value) ? 0: atoi(value);

	init_qos_mgr();

    return GN_OK;
}
//by:yhy 初始化fabric时自启动的
//by:yhy 仅仅运行一次
void init_openstack_fabric_auto_start()
{
	INT1* value = NULL;
	int return_value = 0;
	value = get_value(g_controller_configure, "[openvstack_conf]", "auto_fabric");
	return_value = (NULL == value) ? 0: atoi(value);
	if (0 != return_value) 
	{
		if (0 == g_fabric_start_flag) 
		{
			LOG_PROC("INFO", "Setup fabric impl");
			of131_fabric_impl_setup();
		}
		g_fabric_start_flag = 1;
	}

	//by:yhy 对Pica8下发外联口相关的流表
	init_external_flows();
	//by:yhy 下发浮动IP相关流表(内部又重新启动一个定时器周期执行操作)
	init_proactive_floating_check_mgr();
	//by:yhy 定期刷新主机列表
	init_host_check_mgr();
	//by:yhy 定期检查external(对外交换机,发送ARP请求)
	init_external_mac_check_mgr();
	// kill timer   why? 为什么干掉了?(目测init_openstack_fabric_auto_start函数内部调用的各功能都独自启动了单独的定时器)
    timer_kill(auto_test_p, &g_auto_timer);
	//by:yhy 启动lbaas监听,定时刷新状态
	start_openstack_lbaas_listener();
}

//by:yhy 初始化(每隔10s启动一次init_openstack_fabric_auto_start)这个功能
void init_openstack_fabric_auto_start_delay()
{
	if (1 == g_auto_init_flag) 
	{
		return ;
	}

	// set the flag
	g_auto_init_flag = 1;

	void *g_auto_timerid = NULL;
	UINT4 g_auto_interval = 10;

	// set the timer
    g_auto_timerid = timer_init(1);
    auto_test_p = timer_creat(g_auto_timerid, g_auto_interval, NULL, &g_auto_timer, init_openstack_fabric_auto_start);
}

//get net card info
INT4 get_controller_inet_info()
{
    INT1 if_name[10] = {0};
	//linux socket struct
    struct sockaddr_in *b;
    int sock, ret;
	//linux net card struct
    struct ifreq ifr;    

	//get net card interface
    INT1* value = get_value(g_controller_configure, "[controller]", "manager_eth");
    NULL == value ? strncpy(if_name, "eth0", 10 - 1) : strncpy(if_name, value, 10 - 1);
	
	//linux netlink
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return GN_ERR;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);

	//get netcard interface mac address
    ret = ioctl(sock, SIOCGIFHWADDR, &ifr, sizeof(ifr));
    if (ret == 0)
    {
        memcpy(g_controller_mac, ifr.ifr_hwaddr.sa_data, 6);
        memcpy(g_reserve_mac, g_controller_mac, 6);
    }
    else
    {
        close(sock);
        return ret;
    }

	//get netcard interface ip address
    ret = ioctl(sock, SIOCGIFADDR, &ifr, sizeof(ifr));
    if (ret == 0)
    {
        b = (struct sockaddr_in *) &ifr.ifr_addr;
        g_controller_ip = ntohl(*(UINT4 *) &b->sin_addr);

		INT1* res_value = get_value(g_controller_configure, "[controller]", "reserve_ip");
		if ((NULL == res_value) || (0 == atoll(res_value))) {
		    g_reserve_ip = g_controller_ip;
		}
		else {
		    g_reserve_ip = ntohl(ip2number(res_value));
		}
    }
    else
    {
        close(sock);
        return ret;
    }

    close(sock);
    return ret;
}

void gnflush_init()
{
    return;
}
app_init(gnflush_init);

void gnflush_proc(gn_switch_t *sw)
{
    return;
}
app_proc(gnflush_proc);

void gnflush_fini()
{
    return;
}
app_fini(gnflush_fini);

//by:yhy 具体参见app_init的注释
static void mod_initcalls()
{
    initcall_t *p_init;

    p_init = &__start_appinit_sec;
    do
    {
        (*p_init)();
        p_init++;
    } while (p_init < &__stop_appinit_sec);
}

//by:yhy 具体参见app_fini的注释
static void mod_finicalls()
{
    initcall_t *p_fini;

    p_fini = &__start_appfini_sec;
    do
    {
        (*p_fini)();
        p_fini++;
    } while (p_fini < &__stop_appfini_sec);
}



INT4 signal_Recv(INT4 iSigno)
{
	printf("get signal No: %d !\n",iSigno);
	return GN_OK;
}

void *signal_proc()
{
	sigset_t set;
    INT4 iSigno=0;
	INT4 iRet = 0; 
	
	prctl(PR_SET_NAME, (unsigned long) "Sig_Proc_Thread" ) ;  

	sigemptyset(&set);
	sigfillset(&set);
	pthread_sigmask(SIG_SETMASK, &set, NULL);
	
	while(1)
	{
        iRet = sigwait(&set, &iSigno);
		if(0 != iRet)
		{
			LOG_PROC("INFO","Get signal failed !");
		}
		LOG_PROC("INFO","Recv signal %d !",iSigno);
		switch(iSigno)
		{
			case SIGPIPE:  break;
			case SIGINT:   
				{
					// process by main thread function wait_exit
					break;
				} 
			default: break;
		}

			
	}
	return NULL;
}

INT4 init_signal()
{
	pthread_t signal_thread;
	
	sigset_t set;
	sigemptyset(&set);
	sigfillset(&set);
	sigdelset(&set,  SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL); //主线程屏蔽信号集处理
	
	if(pthread_create(&signal_thread, NULL, signal_proc,NULL) != 0 )
    {
        return GN_ERR;
    }
	return GN_OK;
}
INT4 module_init()
{
    INT4 ret = GN_OK;

	
	ret = init_signal(); //对于INT以外的信号集进行处理 
	if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init signal proc failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init signal proc finished");
    } 
	//by:yhy 系统时间获取线程(1Hz频率刷新  g_cur_sys_time)
    ret = init_sys_time();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init system time failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init system time finished");
    } 
	
	//绿网
	/*Need to be deleted
    ret = init_forward_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init forward manager failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init forward manager finished");
    }
	*/

	//Meter
    /*ret = init_meter_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init meter manager failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init meter manager finished");
    }*/
	
	//Group
    /*ret = init_group_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init group manager failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init group manager finished");
    }*/

	//绿网
	/*Need to be deleted
    ret = init_tenant_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init tenant manager failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init tenant manager finished");
    }
	*/

	//by:yhy 初始化流表
    ret = init_flow_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init flow manager failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init flow manager finished");
    }

	//by:yhy open vswitch 
    ret = init_ovsdb();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init ovsdb manager failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init ovsdb manager finished");
    }

	//by:yhy 初始化交换机信息
    ret = init_conn_svr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init controller service failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init controller service finished");
    }

	//绿网无用
	/*Need to be deleted
    if(GN_OK != init_mac_user())
    {
        LOG_PROC("ERROR", "Init mac user failed");
        return GN_ERR;
    }
	*/

	//初始化拓补
    ret = init_topo_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init network topology failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init network topology finished");
    }

	//restful服务初始化
    ret = init_restful_svr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init restful service failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init restful service finished");
    }
    
	//初始化用于获取switch信息的相关机制
    ret = init_stats_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init statistic manager failed");
        return GN_ERR;
    }
    else
    {
    	LOG_PROC("INFO", "Init statistic manager finished");
    }
    start_fabric_thread();
	//by:yhy 初始化Openstack相关
    initOpenstackFabric();
	
    if (g_is_cluster_on)
    {//by:yhy 开启主备
        ret = init_redis_client();
        if(GN_OK != ret)
        {
            LOG_PROC("ERROR", "Init redis client failed");
            return GN_ERR;
        }
        else
        {
            LOG_PROC("INFO", "Init redis client finished");
        }  

    	ret = init_sync_mgr();
    	if(GN_OK != ret)
        {
            LOG_PROC("ERROR", "Init synchronization manager failed");
            return GN_ERR;
        }
        else
        {
            LOG_PROC("INFO", "Init synchronization manager finished");
        }

        ret = init_cluster_mgr();
        if(GN_OK != ret)
        {
            LOG_PROC("ERROR", "Init cluster manager failed");
            return GN_ERR;
        }
        else
        {
            LOG_PROC("INFO", "Init cluster manager finished");
        }
    }

	//by:yhy 负载监测管理
    ret = init_overload_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init overload mgr failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init overload mgr finished");
    }
	
	ret = init_timer_task();
	if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init timer task failed");
        return GN_ERR;
    }
    else
    {
        LOG_PROC("INFO", "Init timer task finished");
    }

    //by:yhy 目测为无效函数app_init(gnflush_init)中的gnflush为空函数
    mod_initcalls();


    return GN_OK;
}


void module_fini()
{
    fini_conn_svr();

    //????module_fini
    mod_finicalls();
}

void sigint_handler(int arg)
{
    g_server.state = FALSE;
}
//by:yhy 将键盘发出的"Ctrl+C"动作信号 绑定到 sigint_handler函数上, 每隔1s自调用一次
void wait_exit()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigfillset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    while(1)
    {
        if(g_server.state == FALSE)
        {
            break;
        }
       // sleep(1000);
       
	   MsecSleep(1000*1000);
    }
}

INT4 check_nofile()
{
    struct rlimit rlim;
    int nRet = getrlimit(RLIMIT_NOFILE, &rlim);
    if(0 != nRet)
    {
        LOG_PROC("ERROR", "getrlimit failed for error %d", errno);
        return GN_ERR;
    }

    if(rlim.rlim_cur < g_max_nofile)
    {
        LOG_PROC("ERROR", "system nofile must not less %d", g_max_nofile);
        return GN_ERR;
    }

    return GN_OK;
}

int main(int argc, char **argv)
{
    INT4 ret = 0;
	//by:yhy   show copyright
    show_copy_right();

    //ret = check_nofile();

    if(GN_OK != ret)
    {
        goto EXIT;
    }

    ret = read_configuration();
    if(GN_OK != ret)
    {
        goto EXIT;
    }
    ret = get_controller_inet_info();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "%s", "Get controller inet info failed");
        goto EXIT;
    }
	//by:yhy 各模块初始化
    ret = module_init();
    if(GN_OK != ret)
    {
        goto EXIT;
    }
    else
    {
        LOG_PROC("INFO", "***** All modules initialized succeed *****\n");
    }

    init_handler();
    //by:yhy <obscure>
    thread_topo_change_start();
    //by:yhy 启动openflow的监听端口,等待交换机连入
    ret = start_openflow_server();
    if(GN_ERR == ret)
    {
        goto EXIT;
    }

    
    wait_exit();
EXIT:
    module_fini();
    return ret;
}

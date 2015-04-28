/******************************************************************************
*                                                                             *
*   File Name   : main.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef VERSION
#define VERSION 0x020000
#endif
#define PRODUCT "GNFlush"

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
#include "../flow-mgr/flow-mgr.h"
#include "../ovsdb/ovsdb.h"

#define START_DATE __DATE__  // compile date.
#define START_TIME __TIME__  // compile time.

ini_file_t *g_controller_configure;
UINT1 g_controller_mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
UINT4 g_controller_ip = 0;

void show_copy_right()
{
//    struct tm *when;
//    time_t now;

    INT1 stars[] = {"*****************************************************************" };
    INT1 spaces[] = {"*                                                               *" };
    INT1 ver[81], info[81], author[81], company[81], copyright[81];
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
    sprintf(author, "*                    Author: XueQiuBao, DengChao                *");
    sprintf(company, "*               Copyright GreeNet 2015-10 ~ 2099-10             *");
    sprintf(copyright, "*                  (c) All Rights Reserved.                     *");

    printf("\n");
    printf("%s\n", stars);
    printf("%s\n", spaces);
    printf("%s\n", ver);
    printf("%s\n", info);
    printf("%s\n", spaces);
    printf("%s\n", author);
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

    return GN_OK;
}

INT4 get_controller_inet_info()
{
    INT1 if_name[10] = {0};
    struct sockaddr_in *b;
    int sock, ret;
    struct ifreq ifr;

    INT1 *value = get_value(g_controller_configure, "[controller]", "manager_eth");
    NULL == value ? strncpy(if_name, "eth0", 10 - 1) : strncpy(if_name, value, 10 - 1);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return GN_ERR;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, if_name);
    ret = ioctl(sock, SIOCGIFHWADDR, &ifr, sizeof(ifr));
    if (ret == 0)
    {
        memcpy(g_controller_mac, ifr.ifr_hwaddr.sa_data, 6);
    }
    else
    {
        close(sock);
        return ret;
    }

    ret = ioctl(sock, SIOCGIFADDR, &ifr, sizeof(ifr));
    if (ret == 0)
    {
        b = (struct sockaddr_in *) &ifr.ifr_addr;
        g_controller_ip = ntohl(*(UINT4 *) &b->sin_addr);
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

//����__start_appinit_sec��__stop_appinit_sec֮����ڵĺ���ָ��app_init(x)
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

//����__start_appfini_sec��__stop_appfini_sec֮����ڵĺ���ָ��app_fini(x)
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

INT4 module_init()
{
    INT4 ret = GN_OK;

    ret = init_sys_time();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init system time failed");
        return GN_ERR;
    }

    ret = init_forward_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init forward manager failed");
        return GN_ERR;
    }

    ret = init_meter_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init meter manager failed");
        return GN_ERR;
    }

    ret = init_group_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init group manager failed");
        return GN_ERR;
    }

    ret = init_tenant_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init tenant manager failed");
        return GN_ERR;
    }

    ret = init_cluster_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init cluster manager failed");
        return GN_ERR;
    }

    ret = init_flow_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init flow manager failed");
        return GN_ERR;
    }

    ret = init_ovsdb();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init ovsdb manager failed");
        return GN_ERR;
    }

    ret = init_conn_svr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init controller service failed");
        return GN_ERR;
    }

    if(GN_OK != init_mac_user())
    {
        LOG_PROC("ERROR", "Init mac user failed");
        return GN_ERR;
    }

    ret = init_topo_mgr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init network topology failed");
        return GN_ERR;
    }

    ret = init_restful_svr();
    if(GN_OK != ret)
    {
        LOG_PROC("ERROR", "Init restful service failed");
        return GN_ERR;
    }

//    ret = init_stats_mgr();
//    if(GN_OK != ret)
//    {
//        LOG_PROC("ERROR", "Init statistic manager failed");
//        return GN_ERR;
//    }

    //����module_init
    mod_initcalls();

    return GN_OK;
}

void module_fini()
{
    fini_conn_svr();

    //����module_fini
    mod_finicalls();
}

int main(int argc, char **argv)
{
    INT4 ret = 0;

    show_copy_right();
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

    ret = module_init();
    if(GN_OK != ret)
    {
        goto EXIT;
    }
    else
    {
        LOG_PROC("INFO", "***** All modules initialized succeed *****\n");
    }

    ret = start_openflow_server();
    if(GN_ERR == ret)
    {
        goto EXIT;
    }

EXIT:
    module_fini();
    return ret;
}

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
 *   File Name   : user-mgr.c           *
 *   Author      : greenet Administrator           *
 *   Create Date : 2015-3-3           *
 *   Version     : 1.0           *
 *   Function    : .           *
 *                                                                             *
 ******************************************************************************/

#include "user-mgr.h"
#include "timer.h"
#include "mem_pool.h"
#include "../tenant-mgr/tenant-mgr.h"

mac_user_table_t g_macuser_table;


INT1 init_mac_user()
{
    INT1 *value = NULL;
    UINT4 idx = 0;

    value = get_value(g_controller_configure, "[controller]", "macuser_hsize");
    g_macuser_table.macuser_hsize = (value == NULL ? 1024 : atoll(value));
    g_macuser_table.macuser_hsize_tot = g_macuser_table.macuser_hsize * g_server.max_switch;

    value = get_value(g_controller_configure, "[controller]", "macuser_lifetime");
    g_macuser_table.macuser_lifetime = (value == NULL ? 10 : atoll(value));

    pthread_mutex_init(&g_macuser_table.macuser_mutex, NULL);


    //分配全局指针内存
    g_macuser_table.macuser_tb = (mac_user_t **)gn_malloc(g_macuser_table.macuser_hsize_tot * sizeof(mac_user_t *));

    //分配MAC地址用户表内存
    g_macuser_table.macuser_memid = mem_create(sizeof(mac_user_t), g_macuser_table.macuser_hsize_tot);
    if (NULL == g_macuser_table.macuser_memid)
    {
        return GN_ERR;
    }

    //创建定时器
    g_macuser_table.macuser_timer = timer_init(g_macuser_table.macuser_hsize_tot);
    g_macuser_table.macuser_cnt = 0;

    //初始化预设交换机
    for(; idx < g_server.max_switch; idx++)
    {
        //init users list
        g_server.switches[idx].users = (mac_user_t **)gn_malloc(g_macuser_table.macuser_hsize * sizeof(mac_user_t *));
        pthread_mutex_init(&g_server.switches[idx].users_mutex, NULL);
    }

    return GN_OK;
}

//查询该IP用户是否存在，全局遍历最好不要使用
mac_user_t* search_ip_user(UINT4 ip)
{
    UINT4 index = 0;
    for(; index < g_macuser_table.macuser_hsize_tot; index++)
    {
        if(g_macuser_table.macuser_tb[index]->ipv4 == ip)
        {
            return g_macuser_table.macuser_tb[index];
        }
    }

    return NULL;
}

//查询该MAC用户是否存在
mac_user_t *search_mac_user(UINT1 *mac)
{
    mac_user_t *p_macuser = NULL;
    UINT4 index = 0;
    UINT4 search_cnt = 0;

    index = mac_hash(mac, g_macuser_table.macuser_hsize_tot);
    p_macuser = g_macuser_table.macuser_tb[index];
    while (p_macuser)
    {
        if (p_macuser->mac[5] == mac[5] && p_macuser->mac[4] == mac[4] && p_macuser->mac[3] == mac[3])
        {
            if (p_macuser->mac[2] == mac[2] && p_macuser->mac[1] == mac[1] && p_macuser->mac[0] == mac[0])
            {
                pthread_mutex_lock(&g_macuser_table.macuser_mutex);

                if (g_cur_sys_time.tv_sec - p_macuser->tv_last_sec >= g_macuser_table.macuser_lifetime)
                {
                    p_macuser->tv_last_sec = g_cur_sys_time.tv_sec;
                }

                pthread_mutex_unlock(&g_macuser_table.macuser_mutex);
//                printf("#### Find user: sw[%u][%s:%d], ", p_macuser->sw->index,
//                        inet_htoa(ntohl(p_macuser->sw->sw_ip)), ntohs(p_macuser->sw->sw_port));
//                printf("user[%s], inport[%d]\n", inet_htoa(p_macuser->ipv4), p_macuser->port);
                return p_macuser;
            }
        }

        search_cnt++;
        if (search_cnt > 10)
        {
            printf("%s: search_cnt:  %d\n ", FN, search_cnt);
        }

        p_macuser = p_macuser->next;
    }

    return NULL;
}

void del_mac_user(mac_user_t *macuser)
{
    UINT4 idx = 0;
    UINT4 hash_index;
    mac_user_table_t *p_macuser_table = NULL;

    if (!macuser)
    {
        return;
    }

    p_macuser_table = macuser->macuser_table;
    if (!p_macuser_table)
    {
        return;
    }

    hash_index = mac_hash(macuser->mac, p_macuser_table->macuser_hsize_tot);
    
    //delete from global
    pthread_mutex_lock(&p_macuser_table->macuser_mutex);
    if (macuser == p_macuser_table->macuser_tb[hash_index])
    {
        p_macuser_table->macuser_tb[hash_index] = p_macuser_table->macuser_tb[hash_index]->next;
        if (p_macuser_table->macuser_tb[hash_index])
        {
            p_macuser_table->macuser_tb[hash_index]->prev = NULL;
        }

        p_macuser_table->macuser_cnt--;
    }
    else
    {
        if (macuser->prev)
        {
            macuser->prev->next = macuser->next;
            if (macuser->next)
            {
                macuser->next->prev = macuser->prev;
            }
            p_macuser_table->macuser_cnt--;
        }
    }
    pthread_mutex_unlock(&p_macuser_table->macuser_mutex);
    macuser->next = NULL;

    //delete from switch
    for(idx = 0; idx < g_macuser_table.macuser_hsize; idx++)
    {
        if(macuser->sw->users[idx] == macuser)
        {
            pthread_mutex_lock(&macuser->sw->users_mutex);
            macuser->sw->users[idx] = NULL;
            pthread_mutex_unlock(&macuser->sw->users_mutex);
        }
    }

    timer_kill(p_macuser_table->macuser_timer, &(macuser->timer));
    mem_free(p_macuser_table->macuser_memid, (void *) macuser);
    macuser = NULL;
}

void timeout_mac_user(void *para, void *tid)
{
    UINT4 idx = 0;
    UINT4 hash_index;
    mac_user_table_t *p_macuser_table = NULL;
    mac_user_t *p_macuser = (mac_user_t *) para;

    if (!p_macuser)
    {
        return;
    }

    p_macuser_table = p_macuser->macuser_table;
    if (!p_macuser_table)
    {
        return;
    }

    hash_index = mac_hash(p_macuser->mac, p_macuser_table->macuser_hsize_tot);

    pthread_mutex_lock(&p_macuser_table->macuser_mutex);

    if (g_cur_sys_time.tv_sec - p_macuser->tv_last_sec <= p_macuser_table->macuser_lifetime)
    {
        pthread_mutex_unlock(&p_macuser_table->macuser_mutex);
        return;
    }

    //timeout global
    if (p_macuser == p_macuser_table->macuser_tb[hash_index])
    {
        p_macuser_table->macuser_tb[hash_index] = p_macuser_table->macuser_tb[hash_index]->next;
        if (p_macuser_table->macuser_tb[hash_index])
        {
            p_macuser_table->macuser_tb[hash_index]->prev = NULL;
        }

        p_macuser_table->macuser_cnt--;
    }
    else
    {
        if (p_macuser->prev)
        {
            p_macuser->prev->next = p_macuser->next;
            if (p_macuser->next)
            {
                p_macuser->next->prev = p_macuser->prev;
            }
            p_macuser_table->macuser_cnt--;
        }
    }
    pthread_mutex_unlock(&p_macuser_table->macuser_mutex);
    p_macuser->next = NULL;

    //delete from switch
    for(idx = 0; idx < g_macuser_table.macuser_hsize; idx++)
    {
        if(p_macuser->sw->users[idx] == p_macuser)
        {
            pthread_mutex_lock(&p_macuser->sw->users_mutex);
            p_macuser->sw->users[idx] = NULL;
            pthread_mutex_unlock(&p_macuser->sw->users_mutex);
        }
    }

    timer_kill(p_macuser_table->macuser_timer, &(p_macuser->timer));
    mem_free(p_macuser_table->macuser_memid, (void *) p_macuser);
    p_macuser = NULL;
}

static mac_user_t * add_mac_user(mac_user_t * macuser)    //将该MAC地址添加到表中
{
    mac_user_t *p_macuser = NULL;
    UINT4 idx = 0;
    UINT4 hash_index = 0;
    UINT4 search_cnt = 0;

    //add to global
    pthread_mutex_unlock(&g_macuser_table.macuser_mutex);

    hash_index = mac_hash(macuser->mac, g_macuser_table.macuser_hsize_tot);
    p_macuser = g_macuser_table.macuser_tb[hash_index];

    pthread_mutex_lock(&g_macuser_table.macuser_mutex);
    if (p_macuser)
    {
        while (p_macuser)
        {
            if (p_macuser->mac[5] == macuser->mac[5]
                    && p_macuser->mac[4] == macuser->mac[4]
                    && p_macuser->mac[3] == macuser->mac[3])
            {
                if (p_macuser->mac[2] == macuser->mac[2]
                        && p_macuser->mac[1] == macuser->mac[1]
                        && p_macuser->mac[0] == macuser->mac[0])    //已经有此记录
                {
                    mem_free(g_macuser_table.macuser_memid, (void *)macuser);
                    macuser = p_macuser;
                    break;
                }
            }

            search_cnt++;
            if (search_cnt > 10)
                printf("%s: search_cnt:  %d\n ", FN, search_cnt);

            p_macuser = p_macuser->next;
        }

        if (!p_macuser)
        {
            timer_creat(g_macuser_table.macuser_timer, g_macuser_table.macuser_lifetime,
                    macuser, &(macuser->timer), timeout_mac_user);
            g_macuser_table.macuser_tb[hash_index]->prev = macuser;
            macuser->next = g_macuser_table.macuser_tb[hash_index];
            g_macuser_table.macuser_tb[hash_index] = macuser;
            g_macuser_table.macuser_cnt++;
        }
    }
    else
    {
        timer_creat(g_macuser_table.macuser_timer, g_macuser_table.macuser_lifetime,
                macuser, &(macuser->timer), timeout_mac_user);
        g_macuser_table.macuser_tb[hash_index] = macuser;
        g_macuser_table.macuser_cnt++;
    }
    pthread_mutex_unlock(&g_macuser_table.macuser_mutex);

    //add to switch
    for(idx = 0; idx < g_macuser_table.macuser_hsize; idx++)
    {
        if(macuser->sw->users[idx] == NULL)
        {
            pthread_mutex_lock(&macuser->sw->users_mutex);
            macuser->sw->users[idx] = macuser;
            pthread_mutex_unlock(&macuser->sw->users_mutex);
            break;
        }
    }

//    printf("#### New user: sw[%u][%s:%d], ", macuser->sw->index,
//            inet_htoa(ntohl(macuser->sw->sw_ip)), ntohs(macuser->sw->sw_port));
//    printf("user[%s], inport[%d]\n", inet_htoa(macuser->ipv4), macuser->port);

    return macuser;
}

//
mac_user_t *create_mac_user(gn_switch_t *sw, UINT1 *mac, UINT4 inport, UINT4 host_ip, UINT4 *host_ipv6)           //新建该MAC地址用户
{
    mac_user_t *p_macuser = NULL;
    p_macuser = (mac_user_t *)mem_get(g_macuser_table.macuser_memid);
    if (p_macuser)
    {
        p_macuser->sw = sw;
        p_macuser->tv_last_sec = g_cur_sys_time.tv_sec;
        p_macuser->port = inport;
        p_macuser->ipv4 = host_ip;
        if (host_ipv6)
        {
            memcpy(p_macuser->ipv6, host_ipv6, 16);
        }
        p_macuser->macuser_table = &g_macuser_table;
        memcpy(p_macuser->mac, mac, 6);
        p_macuser->tenant_id = query_tenant_id(mac);

        add_mac_user(p_macuser);
        return p_macuser;
    }
    else
    {
        return NULL;
    }
}

void close_mac_user()
{
    UINT4 hsize = 0;
    mac_user_t *pMacUser = NULL;
    mac_user_t *pTmpMacUser = NULL;
    for (hsize = 0; hsize < g_macuser_table.macuser_hsize_tot; hsize++)
    {
        pMacUser = g_macuser_table.macuser_tb[hsize];
        while (pMacUser)
        {
            if (!pMacUser)
                break;

            pTmpMacUser = pMacUser->next;

            timer_kill(g_macuser_table.macuser_timer, &(pMacUser->timer));
            mem_free(g_macuser_table.macuser_memid, (void *) pMacUser);

            pMacUser = pTmpMacUser;
        }
        g_macuser_table.macuser_tb[hsize] = NULL;
    }

    g_macuser_table.macuser_cnt = 0;
}

void fini_mac_user()
{
    g_macuser_table.macuser_cnt = 0;
    timer_destroy(&(g_macuser_table.macuser_timer));
    pthread_mutex_destroy(&(g_macuser_table.macuser_mutex));
    mem_destroy(g_macuser_table.macuser_memid);
    free(g_macuser_table.macuser_tb);
    g_macuser_table.macuser_tb = NULL;
}

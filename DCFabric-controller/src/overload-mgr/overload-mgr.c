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
*   File Name   : overload-mgr.c           *
*   Author      : bnc Administrator           *
*   Create Date : 2016-04-26           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "overload-mgr.h"
#include "mem_pool.h"
#include "ini.h"
#include "../fabric/fabric_host.h"
#include "error_info.h"
#include "timer.h"
#include "fabric_flows.h"



//overload
void* g_msg_counter_list_id = NULL;
msg_counter_t* g_msg_counter_list = NULL;
pthread_mutex_t g_msg_counter_list_mutex;

pthread_t g_overload_pid;
UINT4 g_overload_internal = 5;
UINT4 g_overload_threshold = 1000;
UINT4 g_overload_start = 3;
UINT4 g_overload_stop = 6;
UINT4 g_overload_flow_timeout = 60;
UINT4 g_overload_monitor_timeout = 300;


UINT1 g_empty_mac[6] = {0};
UINT1 g_broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};



void add_msg_counter(gn_switch_t* sw, UINT4 ip, const UINT1* mac)
{    
    if (NULL == sw 
            || 0 == sw->state 
            || 0 == ip 
            || 0 == memcmp(mac, g_empty_mac, 6) 
            || 0 == memcmp(mac, g_broadcast_mac, 6))
    {
        return;
    }
    
    msg_counter_t* node = g_msg_counter_list;
    msg_counter_t* temp = NULL;
    while (node)
    {
        if (sw == node->sw && node->ip == ip && 0 == memcmp(mac, node->mac, 6))
        {
            temp = node;
            break;
        }
        
        node = node->next;
    }

    if (NULL == temp)
    {
        temp = (msg_counter_t*)mem_get(g_msg_counter_list_id);
        if (temp)
        {
            memset(temp, 0, sizeof(msg_counter_t));
            temp->sw = sw;
            temp->ip = ip;
            memcpy(temp->mac, mac, 6);
            temp->timestamp = g_cur_sys_time.tv_sec + g_cur_sys_time.tv_usec / 1000000;
            temp->next = g_msg_counter_list;
            g_msg_counter_list = temp;
        }
        else
        {
            LOG_PROC("WARNING", "Msg overload: mem_get failure!!!");
        }
    }

    temp->count++;
}


void forbid_msg(gn_switch_t* sw, UINT4 ip, UINT1* mac)
{
    if (NULL != sw && sw->state)
    {
        install_fabric_deny_ip_flow(sw, ip, mac, FABRIC_PRIORITY_DENY_FLOW, g_overload_flow_timeout, FABRIC_INPUT_TABLE);
    }
}


void allow_msg(gn_switch_t* sw, UINT4 ip, UINT1* mac)
{
    if (NULL != sw && sw->state)
    {
        delete_fabric_flow_by_ip_mac_priority(sw, ip, mac, FABRIC_PRIORITY_DENY_FLOW, g_overload_flow_timeout, FABRIC_INPUT_TABLE);
    }
}


void *msg_overload_thread(void *value)
{
    INT1 ip_str[48] = {0};
    INT1 mac_str[48] = {0};
    while (TRUE)
    {
        sleep(g_overload_internal);

        UINT8 cur_time = g_cur_sys_time.tv_sec + g_cur_sys_time.tv_usec / 1000000;
        msg_counter_t* node = g_msg_counter_list;
        msg_counter_t* pre = node;
        msg_counter_t* tmp = NULL;
        while (node)
        {   
            if (node->count >= g_overload_threshold)
            {
                memset(ip_str, 0, 48);
                memset(mac_str, 0, 48);
                LOG_PROC("WARNING", "Msg overload happen, ip=%s, mac=%s", 
                    number2ip(node->ip, ip_str), 
                    mac2str(node->mac, mac_str));
                node->start++;
                node->stop = 0;
                if (node->start >= g_overload_start)
                {
                    node->flag = TRUE;
                    node->start = 0;
                    memset(ip_str, 0, 48);
                    memset(mac_str, 0, 48);
                    LOG_PROC("WARNING", "Msg overload: install forbidden flow, ip=%s, mac=%s",                 
                        number2ip(node->ip, ip_str), 
                        mac2str(node->mac, mac_str));
                    forbid_msg(node->sw, node->ip, node->mac);
                }
            }
            else
            {
                node->start = 0;
                if (node->flag)
                {
                    node->stop++;
                    if (node->stop >= g_overload_stop)
                    {
                        node->flag = FALSE;
                        node->stop = 0;
                        memset(ip_str, 0, 48);
                        memset(mac_str, 0, 48);
                        LOG_PROC("WARNING", "Msg overload: remove forbidden flow, ip=%s, mac=%s",
                            number2ip(node->ip, ip_str), 
                            mac2str(node->mac, mac_str));
                        allow_msg(node->sw, node->ip, node->mac);
                    }
                }
            }

            if (node->count)
            {
                node->timestamp = cur_time;
            }
            else if ((cur_time - node->timestamp) >= g_overload_monitor_timeout)
            {
                tmp = node->next;
                if (pre != node)
                {
                    pre->next = node->next;
                }
                else
                {
                    g_msg_counter_list = NULL;
                }

                mem_free(g_msg_counter_list_id, node);
                node = tmp;
                continue;
            }

            node->count = 0;
            pre = node;
            node = node->next;
        }

    }
}


INT4 init_overload_mgr()
{
    INT1* value = get_value(g_controller_configure, "[overload_conf]", "overload_interval");
    g_overload_internal= ((NULL == value) ? 5 : atoll(value));
    value = get_value(g_controller_configure, "[overload_conf]", "overload_threshold");
    g_overload_threshold = ((NULL == value) ? 1000 : atoll(value));
    value = get_value(g_controller_configure, "[overload_conf]", "overload_start");
    g_overload_start = ((NULL == value) ? 3 : atoll(value));
    value = get_value(g_controller_configure, "[overload_conf]", "overload_stop");
    g_overload_stop = ((NULL == value) ? 6 : atoll(value));
    value = get_value(g_controller_configure, "[overload_conf]", "overload_flow_timeout");
    g_overload_flow_timeout = ((NULL == value) ? 60 : atoll(value));
    value = get_value(g_controller_configure, "[overload_conf]", "overload_monitor_timeout");
    g_overload_monitor_timeout = ((NULL == value) ? 300 : atoll(value));

    g_msg_counter_list_id = mem_create(sizeof(msg_counter_t), FABRIC_HOST_LIST_MAX_NUM);
    
    pthread_mutex_init(&g_msg_counter_list_mutex, NULL);
    pthread_create(&g_overload_pid, NULL, msg_overload_thread, NULL);

    return GN_OK;
}




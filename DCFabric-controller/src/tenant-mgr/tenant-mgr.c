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
*   File Name   : tenant-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "tenant-mgr.h"
#include "mem_pool.h"
#include "timer.h"
#include "openflow-common.h"
#include "../flow-mgr/flow-mgr.h"
#include "../user-mgr/user-mgr.h"
#include "../conn-svr/conn-svr.h"
#include <limits.h>

static tenant_container_t g_tenant_container;
static tenant_member_container_t g_tenant_member_container;

////Test function
//static void print_member_list()
//{
//    UINT2 idx = 0;
//    tenant_member_t* member = NULL;
//    for(; idx < g_tenant_member_container.table_size; idx++)
//    {
//        member = g_tenant_member_container.member_table[idx];
//        printf("List %d, Members: ", idx);
//        while(member)
//        {
//            printf("  %02x:%02x:%02x:%02x:%02x:%02x \n", member->mac[0], member->mac[1], member->mac[2], member->mac[3], member->mac[4], member->mac[5]);
//            member = member->next;
//        }
//        printf("\n");
//    }
//}

/************************************************************************
 *   内部模块间接口，也可以作为Rest接口提供
 ************************************************************************/
static tenant_member_t* search_tenant_member(UINT1 *mac)
{
    INT4 search_cnt = 0;
    UINT2 idx = mac_hash(mac, g_tenant_member_container.table_size);
    tenant_member_t* member = g_tenant_member_container.member_table[idx];
    while(member)
    {
        if (member->mac[5] == mac[5] &&
            member->mac[4] == mac[4] &&
            member->mac[3] == mac[3])
        {
            if (member->mac[2] == mac[2] &&
                member->mac[1] == mac[1] &&
                member->mac[0] == mac[0])
            {
                return member;
            }
        }

        search_cnt++;
        if(search_cnt > 1)
        {
            printf("%s: Hash conflict: %d\n", FN, search_cnt);
        }

        member = member->next;
    }

    return NULL;
}

INT4 query_tenant_id(UINT1 *mac)
{
    tenant_member_t* member = search_tenant_member(mac);
    if(member)
    {
        return member->tenant_id;
    }
    return -1;
}

//flow_mod_type: OFPFC_ADD; OFPFC_DELETE_STRICT;
void tenant_send_flow_mod_l2(gn_switch_t* sw, UINT1* eth_src, UINT1* eth_dst, UINT2 flow_mod_type)
{
    gn_flow_t tenant_flow;
    memset(&tenant_flow, 0, sizeof(gn_flow_t));

    memcpy(tenant_flow.match.oxm_fields.eth_src, eth_src, 0);
    tenant_flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_SRC);

    memcpy(tenant_flow.match.oxm_fields.eth_dst, eth_dst, 0);
    tenant_flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);

    //租户隔离流表超时时间
    tenant_flow.idle_timeout = TENANT_TIMEOUT;
    tenant_flow.hard_timeout = TENANT_TIMEOUT;

    //租户隔离优先级
    tenant_flow.priority = FLOW_TENANT_PRIORITY;

    enable_flow_entry(sw, &tenant_flow);
}

//flow_mod_type: OFPFC_ADD; OFPFC_DELETE_STRICT;
void tenant_send_flow_mod_l3(gn_switch_t* sw, UINT4 ipv4_src, UINT4 ipv4_dst, UINT2 flow_mod_type)
{
    gn_flow_t tenant_flow;
    memset(&tenant_flow, 0, sizeof(gn_flow_t));

    tenant_flow.match.oxm_fields.ipv4_src = ipv4_src;
    tenant_flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_SRC);

    tenant_flow.match.oxm_fields.ipv4_dst = ipv4_dst;
    tenant_flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);

    //租户隔离流表超时时间
    tenant_flow.idle_timeout = TENANT_TIMEOUT;
    tenant_flow.hard_timeout = TENANT_TIMEOUT;

    //租户隔离优先级
    tenant_flow.priority = FLOW_TENANT_PRIORITY;

    enable_flow_entry(sw, &tenant_flow);
}

//flow_mod_type: OFPFC_ADD; OFPFC_DELETE_STRICT;
static void tenant_delete_flow_mod(UINT1* member_mac)
{
    UINT4 idx = 0;
    gn_flow_t tenant_flow;

    memset(&tenant_flow, 0, sizeof(gn_flow_t));

    //租户隔离流表超时时间
    tenant_flow.idle_timeout = TENANT_TIMEOUT;
    tenant_flow.hard_timeout = TENANT_TIMEOUT;

    //租户隔离优先级
    tenant_flow.priority = FLOW_TENANT_PRIORITY;

    for(;idx < g_server.max_switch; idx++)
    {
        if(g_server.switches[idx].state)
        {
            //正向
            memcpy(tenant_flow.match.oxm_fields.eth_src, member_mac, 6);
            tenant_flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_SRC);
            enable_flow_entry(&(g_server.switches[idx]), &tenant_flow);

            //反向
            tenant_flow.match.oxm_fields.mask = 0;
            memcpy(tenant_flow.match.oxm_fields.eth_dst, member_mac, 6);
            tenant_flow.match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
            enable_flow_entry(&(g_server.switches[idx]), &tenant_flow);
        }
    }
}

/************************************************************************
 *   Rest 接口
 ************************************************************************/
//tenant management
INT4 create_tenant(const char* tenant_name)
{
    UINT4 idx = 0;
    UINT4 idx_check = 0;
    tenant_t* new_tenant = NULL;

    if(g_tenant_container.tenant_cnt >= g_tenant_container.max_tenants)
    {
        return MAX_LIMITED;
    }

    for(idx=0; idx<g_tenant_container.max_tenants; idx++)
    {
        if(NULL == g_tenant_container.tenants[idx])
        {
            for(idx_check = idx; idx_check<g_tenant_container.max_tenants; idx_check++)
            {
                if((g_tenant_container.tenants[idx_check])
                        && (0 == strcmp(g_tenant_container.tenants[idx_check]->tenant_name, tenant_name)))
                {
                    return TNT_EXIST;
                }
            }

            new_tenant = (tenant_t*)gn_malloc(sizeof(tenant_t));
            new_tenant->member_mac = (UINT1**)gn_malloc(g_tenant_container.max_memebers * sizeof(UINT1**));
            new_tenant->tenant_id = idx;
            strncpy(new_tenant->tenant_name, tenant_name, TENANT_NAME_LEN - 1);
            g_tenant_container.tenants[idx] = new_tenant;
            g_tenant_container.tenant_cnt++;

            printf("Create tenant succeed! %d %s\n", idx, g_tenant_container.tenants[idx]->tenant_name);
            return 0;
        }
        else
        {
            if(0 == strcmp(g_tenant_container.tenants[idx]->tenant_name, tenant_name))
            {
                return TNT_EXIST;
            }
        }
    }

    return MAX_LIMITED;
}

tenant_container_t* query_tenants()
{
    return &g_tenant_container;
}

tenant_t* query_tenant_members(UINT4 tenant_id)
{
    if (tenant_id >= g_tenant_container.max_tenants)
    {
        return NULL;
    }

    return g_tenant_container.tenants[tenant_id];
}

void modify_user_tenant(UINT1* mac, INT4 tenant_id)
{
    int idx_sw = 0;
    gn_switch_t *sw = NULL;

    if (tenant_id >= (INT4)g_tenant_container.max_tenants)
    {
        return;
    }

    for(; idx_sw < g_server.max_switch; idx_sw++)
    {
        sw = &g_server.switches[idx_sw];
        if(sw && (sw->state))
        {
            mac_user_t *p_macuser = search_mac_user(mac);
            if (p_macuser)
            {
                p_macuser->tenant_id = tenant_id;
                return;
            }
        }
    }
}


INT4 clear_tenant_members(UINT4 tenant_id)
{
    int idx = 0;
    tenant_t* tenant = NULL;

    if (tenant_id >= g_tenant_container.max_tenants)
    {
        return TNT_NOT_EXIST;
    }

    tenant = g_tenant_container.tenants[tenant_id];
    if(NULL == tenant)
    {
        return TNT_NOT_EXIST;
    }

    for(; idx < g_tenant_container.max_memebers; idx++)
    {
        if (tenant->member_mac[idx])
        {
            delete_tenant_member(tenant->member_mac[idx]);
            gn_free((void**)(&(tenant->member_mac[idx])));
        }
    }

    return 0;
}

INT4 delete_tenant(UINT4 tenant_id)
{
    tenant_t* tenant = NULL;

    if (tenant_id >= g_tenant_container.max_tenants)
    {
        return TNT_NOT_EXIST;
    }

    tenant = g_tenant_container.tenants[tenant_id];
    if(NULL == tenant)
    {
        return TNT_NOT_EXIST;
    }

    clear_tenant_members(tenant_id);
    gn_free((void**)(&g_tenant_container.tenants[tenant_id]));
    g_tenant_container.tenant_cnt--;

    return 0;
}

void clear_tenants()
{
    UINT4 idx = 0;

    for(; idx < g_tenant_container.max_tenants; idx++)
    {
        delete_tenant(idx);
    }
}

//tenant member management
INT4 add_tenant_member(UINT4 tenant_id, UINT1* mac)
{
    UINT4 idx = 0;
    tenant_member_t *new_member = NULL;
    tenant_t* tenant = NULL;

    if (tenant_id >= g_tenant_container.max_tenants)
    {
        return TNT_NOT_EXIST;
    }

    tenant = g_tenant_container.tenants[tenant_id];
    if(NULL == tenant)
    {
        return TNT_NOT_EXIST;
    }

    if(g_tenant_container.max_memebers <= tenant->member_cnt)
    {
        return MAX_LIMITED;
    }

    if(query_tenant_id(mac) != -1)
    {
        return MEB_EXIST;
    }

    //g_tenant_container add new member
    for(; idx < g_tenant_container.max_memebers; idx++)
    {
        if (NULL == tenant->member_mac[idx])
        {
            tenant->member_mac[idx] = (UINT1*)gn_malloc(6);
            memcpy(tenant->member_mac[idx], mac, 6);
            break;
        }
    }


    if (g_tenant_container.max_memebers <= idx)
    {
        return MAX_LIMITED;
    }

    //g_tenant_member_container add new member
    idx = mac_hash(mac, g_tenant_member_container.table_size);

    new_member = (tenant_member_t *)mem_get(g_tenant_member_container.table_mem_id);
    memset(new_member, 0, sizeof(tenant_member_t));
    new_member->tenant_id = tenant_id;
    memcpy(new_member->mac, mac, 6);

    if(g_tenant_member_container.member_table[idx])  //not first node
    {
        g_tenant_member_container.member_table[idx]->pre = new_member;
        new_member->next = g_tenant_member_container.member_table[idx];
        g_tenant_member_container.member_table[idx] = new_member;
    }
    else    //is first node
    {
        g_tenant_member_container.member_table[idx] = new_member;
    }

    modify_user_tenant(new_member->mac, tenant_id);
    tenant_delete_flow_mod(new_member->mac);
    g_tenant_member_container.member_cnt++;
    return OPT_SUC;
}

INT4 delete_tenant_member(UINT1* mac)
{
    UINT4 idx = 0;
    tenant_member_t* p_member = search_tenant_member(mac);
    if(p_member)
    {
        tenant_t* p_tenant = g_tenant_container.tenants[p_member->tenant_id];

        modify_user_tenant(mac, -1);
        tenant_delete_flow_mod(p_member->mac);

        //delete from tenant_member_container
        idx = mac_hash(mac, g_tenant_member_container.table_size);
        if (p_member == g_tenant_member_container.member_table[idx])  //is the first node
        {
            g_tenant_member_container.member_table[idx] = g_tenant_member_container.member_table[idx]->next;
            if(g_tenant_member_container.member_table[idx])
            {
                g_tenant_member_container.member_table[idx]->pre = NULL;
            }
        }
        else    //is not the first node
        {
            if(p_member->pre)
            {
                p_member->pre->next = p_member->next;
                if(p_member->next)
                {
                    p_member->next->pre = p_member->pre;
                }
            }
        }

        mem_free(g_tenant_member_container.table_mem_id, (void *)p_member);

        //delete from tenant_container
        if (p_tenant)
        {
            for(; idx < g_tenant_container.max_memebers; idx++)
            {
                if(p_tenant->member_mac[idx] && (0 == memcmp(p_tenant->member_mac[idx], mac, 6)))
                {
                    gn_free((void**)(&p_tenant->member_mac[idx]));
                }
            }
        }

        return 0;
    }

    return MEB_NOT_EXIST;
}


/************************************************************************
 *   服务启停接口
 ************************************************************************/

INT4 init_tenant_mgr()
{
    //初始化tenant_container
    g_tenant_container.max_tenants = TENANT_MAX;
    g_tenant_container.max_memebers = TENANT_MAX_MEMBER;
    g_tenant_container.tenant_cnt = 0;
    g_tenant_container.tenant_mutex = 0;
    g_tenant_container.tenants = (tenant_t **)gn_malloc(sizeof(tenant_member_t **) * TENANT_MAX);

    //初始化tenant_member_container
    g_tenant_member_container.table_size = TENANT_MAX * TENANT_MAX_MEMBER;
    g_tenant_member_container.member_cnt = 0;
    g_tenant_member_container.member_mutex = 0;
    g_tenant_member_container.member_table = (tenant_member_t **)gn_malloc(g_tenant_member_container.table_size * sizeof(tenant_member_t **));
    g_tenant_member_container.table_mem_id = mem_create(sizeof(tenant_member_t), g_tenant_member_container.table_size);
    if (NULL == g_tenant_member_container.table_mem_id)
    {
//        write_log(LOG_SYS_ERR, "\nCreate memory pool for 'TenantMemberMemPool' failed!");
        return GN_ERR;
    }

    return GN_OK;
}

void fini_tenant_mgr()
{
    clear_tenants();
    gn_free((void**)(&(g_tenant_container.tenants)));
    gn_free((void**)(&(g_tenant_member_container.member_table)));
    mem_destroy(g_tenant_member_container.table_mem_id);
}

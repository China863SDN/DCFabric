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
*   File Name   : tenant-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "gnflush-types.h"

#ifndef TENANT_MGR_H_
#define TENANT_MGR_H_

#define TENANT_NAME_LEN 128
#define TENANT_MAX_MEMBER 128
#define TENANT_MAX 128
#define TENANT_TIMEOUT 0

#define SYS_ERR -1
#define OPT_SUC 0
#define TNT_EXIST 1
#define TNT_NOT_EXIST 2
#define TNT_STATE_INVALID 3
#define MEB_EXIST 4
#define MEB_NOT_EXIST 5
#define MEB_STATE_INVALID 6
#define MAX_LIMITED 7

#pragma pack(1)
typedef struct tenant
{
    UINT4 tenant_id;                    //租户ID
    UINT4 member_cnt;                   //该租户下当前成员总数
    char tenant_name[TENANT_NAME_LEN];  //租户名称
    UINT1 **member_mac;                 //租户成员MAC信息
    UINT1 status;                       //租户状态：0，正常；  （预留）
}tenant_t;

typedef struct tenant_member
{
    struct tenant_member *pre;   //下一个租户成员
    struct tenant_member *next;  //下一个租户成员
    UINT1 mac[6];       //用户MAC地址
    UINT1 status;       //租户状态：0，正常； （预留）
    UINT4 tenant_id;    //所在租户的租户ID
}tenant_member_t;

typedef struct tenant_container
{
    tenant_t **tenants;                 //所有租户
    pthread_mutex_t *tenant_mutex;      //租户锁
    UINT4 max_tenants;          //允许最大租户数
    UINT4 max_memebers;         //每个租户允许最大成员数
    UINT4 tenant_cnt;           //当前租户总数
}tenant_container_t;

typedef struct tenant_member_container
{
    tenant_member_t **member_table; //分配存储哈希表内存的首地址
    void *table_mem_id;             //内存池标志
    pthread_mutex_t *member_mutex;  //成员锁
    UINT4 member_cnt;               //当前所有租户成员总数
    UINT4 table_size;               //指定哈希表大小，成员最大个数
}tenant_member_container_t;
#pragma pack()

//tenant management
INT4 create_tenant(const char* tenant_name);
tenant_container_t* query_tenants();
INT4 delete_tenant(UINT4 tenant_id);
void clear_tenants();

//tenant member management
INT4 add_tenant_member(UINT4 tenant_id, UINT1* mac);
INT4 delete_tenant_member(UINT1* mac);
INT4 clear_tenant_members(UINT4 tenant_id);
tenant_t* query_tenant_members(UINT4 tenant_id);

INT4 query_tenant_id(UINT1 *mac);

//tenant flow mod
void tenant_send_flow_mod_l2(gn_switch_t* sw, UINT1* eth_src, UINT1* eth_dst, UINT2 flow_mod_type);
void tenant_send_flow_mod_l3(gn_switch_t* sw, UINT4 ipv4_src, UINT4 ipv4_dst, UINT2 flow_mod_type);

INT4 init_tenant_mgr();
void fini_tenant_mgr();

#endif /* TENANT_MGR_H_ */

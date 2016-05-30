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
*   File Name   : group-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-10           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "group-mgr.h"
#include "../conn-svr/conn-svr.h"
#include "../flow-mgr/flow-mgr.h"
#include "mem_pool.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"

INT4 group_weight_flag = 0;

gn_group_t *find_group_by_id(gn_switch_t *sw, UINT4 group_id)
{
    gn_group_t *p_group = sw->group_entries;

    while(p_group)
    {
        if(group_id == p_group->group_id)
        {
            return p_group;
        }

        p_group = p_group->next;
    }

    return NULL;
}

INT4 add_group_entry(gn_switch_t *sw, gn_group_t *group)
{
    INT4 ret = 0;
    group_mod_req_info_t group_mod_req_info;
    if(0 == sw->state)
    {
        return EC_SW_STATE_ERR;
    }

    pthread_mutex_lock(&sw->group_entry_mutex);

    memset(&group_mod_req_info, 0, sizeof(group_mod_req_info_t));
    group_mod_req_info.group = find_group_by_id(sw, group->group_id);
    if(NULL == group_mod_req_info.group)
    {
        group->next = sw->group_entries;
        if(sw->group_entries)
        {
            sw->group_entries->pre = group;
        }
        sw->group_entries = group;
        group_mod_req_info.group = group;
    }
    group_mod_req_info.command = OFPMC_ADD;
    ret = sw->msg_driver.msg_handler[OFPT13_GROUP_MOD](sw, (UINT1 *)&group_mod_req_info);

    pthread_mutex_unlock(&sw->group_entry_mutex);
    return ret;
}

INT4 modify_group_entry(gn_switch_t *sw, gn_group_t *group)
{
    INT4 ret = 0;
    gn_group_t *p_group = NULL;
    group_mod_req_info_t group_mod_req_info;
    if(0 == sw->state)
    {
        return EC_SW_STATE_ERR;
    }

    pthread_mutex_lock(&sw->group_entry_mutex);

    memset(&group_mod_req_info, 0, sizeof(group_mod_req_info_t));
    group_mod_req_info.group = group;
    p_group = find_group_by_id(sw, group->group_id);
    if(NULL == p_group)
    {
        pthread_mutex_unlock(&sw->group_entry_mutex);
        return EC_GROUP_NOT_EXIST;
    }

    group->next = p_group->next;
    group->pre = p_group->pre;
    free(p_group);

    if(group->pre)
    {
        group->pre->next = group;
    }
    else
    {
        sw->group_entries = group;
    }

    if(group->next)
    {
        group->next->pre = group;
    }

    group_mod_req_info.command = OFPMC_MODIFY;
    ret = sw->msg_driver.msg_handler[OFPT13_GROUP_MOD](sw, (UINT1 *)&group_mod_req_info);

    pthread_mutex_unlock(&sw->group_entry_mutex);
    return ret;
}

INT4 delete_group_entry(gn_switch_t *sw, gn_group_t *group)
{
    INT4 ret = 0;
    group_mod_req_info_t group_mod_req_info;
    if(0 == sw->state)
    {
        return EC_SW_STATE_ERR;
    }

    memset(&group_mod_req_info, 0, sizeof(group_mod_req_info_t));
    pthread_mutex_lock(&sw->group_entry_mutex);
    gn_group_t *p_group = find_group_by_id(sw, group->group_id);

    if(NULL == p_group)
    {
        p_group = group;
    }
    else
    {
        gn_group_free(group);
        if(p_group->pre)
        {
            p_group->pre->next = p_group->next;
            if(p_group->next)
            {
                p_group->next->pre = p_group->pre;
            }
        }
        else
        {
            sw->group_entries = p_group->next;
            if(p_group->next)
            {
                p_group->next->pre = NULL;
            }
        }
    }

    group_mod_req_info.group = p_group;
    group_mod_req_info.command = OFPGC_DELETE;
    ret = sw->msg_driver.msg_handler[OFPT13_GROUP_MOD](sw, (UINT1 *)&group_mod_req_info);

    gn_group_free(p_group);
    pthread_mutex_unlock(&sw->group_entry_mutex);

    return ret;
}


void clear_group_entries(gn_switch_t *sw)
{
    group_mod_req_info_t group_mod_req_info;
    gn_group_t group;
    gn_group_t *p_group = sw->group_entries;
    pthread_mutex_lock(&sw->group_entry_mutex);
    while(sw->group_entries)
    {
        p_group = sw->group_entries;
        sw->group_entries = p_group->next;
        while(p_group)
        {
            sw->group_entries = p_group->next;
            gn_group_free(p_group);
            p_group = sw->group_entries;

        }
    }

    sw->group_entries = NULL;
    pthread_mutex_unlock(&sw->group_entry_mutex);

    memset(&group_mod_req_info, 0, sizeof(group_mod_req_info_t));
    memset(&group, 0, sizeof(gn_group_t));
    group.group_id = OFPG_ALL;
    group.type = OFPGT_ALL;
    group_mod_req_info.group = &group;
    group_mod_req_info.command = OFPGC_DELETE;
    sw->msg_driver.msg_handler[OFPT13_GROUP_MOD](sw, (UINT1 *)&group_mod_req_info);
}

static void group_bucket_free(group_bucket_t *bucket)
{
//    LIST_FREE(bucket->actions, gn_action_t, free);
    gn_action_t *p_action = bucket->actions;
    while(p_action)
    {
        bucket->actions = p_action->next;
        // mem_free(g_gnaction_mempool_id, (void *)p_action);
        gn_free((void**)&p_action);
        p_action = bucket->actions;
    }

    free(bucket);
}

group_bucket_t* create_lbaas_group_bucket(UINT4 between_portno, UINT1* host_mac, UINT4 host_ip, UINT2 host_tcp_dst,
            UINT2 host_vlan_id, UINT2 weight)
{
    group_bucket_t* bucket = (group_bucket_t *)gn_malloc(sizeof(group_bucket_t));

    gn_action_output_t *output_action = (gn_action_output_t *)gn_malloc(sizeof(gn_action_output_t));
    output_action->port = between_portno;
    output_action->type = OFPAT13_OUTPUT;
    output_action->max_len = 0xffff;
    output_action->next = NULL;                    

    gn_action_set_field_t* act_set_field = (gn_action_set_field_t*)gn_malloc(sizeof(gn_action_set_field_t));
    memset(act_set_field, 0, sizeof(gn_action_set_field_t));
    act_set_field->type = OFPAT13_SET_FIELD;
    memcpy(act_set_field->oxm_fields.eth_dst, host_mac, 6);
    act_set_field->oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);
    act_set_field->oxm_fields.ipv4_dst = htonl(host_ip);
    act_set_field->oxm_fields.mask |= (1 << OFPXMT_OFB_IPV4_DST);
    act_set_field->oxm_fields.tcp_dst = host_tcp_dst;
    act_set_field->oxm_fields.mask |= (1 << OFPXMT_OFB_TCP_DST);
    act_set_field->oxm_fields.vlan_vid = host_vlan_id;
    act_set_field->oxm_fields.mask |= (1 << OFPXMT_OFB_VLAN_VID);
    act_set_field->next = (gn_action_t*)output_action;

    gn_action_t* act_pushVlan = (gn_action_t*)gn_malloc(sizeof(gn_action_t));
    memset(act_pushVlan, 0, sizeof(gn_action_t));
    act_pushVlan->type = OFPAT13_PUSH_VLAN;
    act_pushVlan->next =  (gn_action_t*)act_set_field;

    bucket->watch_port = OFPP13_ANY;
    bucket->watch_group = OFPG_ANY;

    if (0 == group_weight_flag) {
        bucket->weight = 1;
    }
    else {
        bucket->weight = weight;
    }
    bucket->next = NULL;

    bucket->actions = act_pushVlan;
    return bucket;

}



void gn_group_free(gn_group_t *group)
{
//    LIST_FREE(group->buckets, group_bucket_t, group_bucket_free);
    group_bucket_t *p_bucket = group->buckets;
    while(p_bucket)
    {
        group->buckets = p_bucket->next;
        group_bucket_free(p_bucket);
        p_bucket = group->buckets;
    }

    free(group);
}


INT4 init_group_mgr()
{
    INT4 ret = 0;

    INT1 *value = NULL;
    value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
    UINT4 openstack_flag = (NULL == value) ? 0: atoi(value);
    if (openstack_flag) {
        value = get_value(g_controller_configure, "[openvstack_conf]", "group_weight_on");
        group_weight_flag = (NULL == value) ? 0: atoi(value);
    }

    return ret;
}


void fini_group_mgr()
{

}

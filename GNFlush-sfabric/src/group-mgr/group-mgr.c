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
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"

static gn_group_t *find_group_by_id(gn_switch_t *sw, UINT4 group_id)
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
        if(p_group->pre)
        {
            p_group->pre->next = p_group->next;
        }
        else
        {
            sw->group_entries = p_group->next;
        }
    }

    group_mod_req_info.group = p_group;
    group_mod_req_info.command = OFPMC_DELETE;
    ret = sw->msg_driver.msg_handler[OFPT13_GROUP_MOD](sw, (UINT1 *)&group_mod_req_info);

    free(p_group);
    pthread_mutex_unlock(&sw->group_entry_mutex);

    return ret;
}


void clear_group_entries(gn_switch_t *sw)
{
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
    pthread_mutex_unlock(&sw->group_entry_mutex);
}

static void group_bucket_free(group_bucket_t *bucket)
{
    LIST_FREE(bucket->actions, gn_action_t, free);
    free(bucket);
}

void gn_group_free(gn_group_t *group)
{
    LIST_FREE(group->buckets, group_bucket_t, group_bucket_free);
    free(group);
}


INT4 init_group_mgr()
{
    INT4 ret = 0;

    return ret;
}

void fini_group_mgr()
{

}

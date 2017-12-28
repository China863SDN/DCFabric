#include "meter-mgr.h"
#include "../conn-svr/conn-svr.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"

/* by:yhy
 * 在sw的meter_entries寻找与meter_id匹配的项
 */
gn_meter_t *find_meter_by_id(gn_switch_t *sw, UINT4 meter_id)
{
    gn_meter_t *p_meter = sw->meter_entries;

    while(p_meter)
    {
        if(meter_id == p_meter->meter_id)
        {
            return p_meter;
        }
        p_meter = p_meter->next;
    }
    return NULL;
}
/* by:yhy
 * 对sw添加meter
 */
INT4 add_meter_entry(gn_switch_t *sw, gn_meter_t *meter)
{
    INT4 ret = 0;
    meter_mod_req_info_t meter_mod_req_info;
    if(INITSTATE == sw->conn_state)
    {
        return EC_SW_STATE_ERR;
    }

    pthread_mutex_lock(&sw->meter_entry_mutex);

    memset(&meter_mod_req_info, 0, sizeof(meter_mod_req_info_t));
    meter_mod_req_info.meter = find_meter_by_id(sw, meter->meter_id);
    if(NULL == meter_mod_req_info.meter)
    {
        meter->next = sw->meter_entries;
        if(sw->meter_entries)
        {
            sw->meter_entries->pre = meter;
        }
        sw->meter_entries = meter;
        meter_mod_req_info.meter = meter;
    }
    meter_mod_req_info.command = OFPMC_ADD;
    ret = sw->msg_driver.msg_handler[OFPT13_METER_MOD](sw, (UINT1 *)&meter_mod_req_info);

    pthread_mutex_unlock(&sw->meter_entry_mutex);
    return ret;
}



/* by:yhy
 * 在sw中寻找匹配meter的项,找到后更新,并下发.无法找到匹配项的话不做处理.
 */
INT4 modify_meter_entry(gn_switch_t *sw, gn_meter_t *meter)
{
    INT4 ret = 0;
    gn_meter_t *p_meter = NULL;
    meter_mod_req_info_t meter_mod_req_info;
    if(INITSTATE == sw->conn_state)
    {
        return EC_SW_STATE_ERR;
    }

    pthread_mutex_lock(&sw->meter_entry_mutex);

    memset(&meter_mod_req_info, 0, sizeof(meter_mod_req_info_t));
    meter_mod_req_info.meter = meter;
    p_meter = find_meter_by_id(sw, meter->meter_id);
    if(NULL == p_meter)
    {
        pthread_mutex_unlock(&sw->meter_entry_mutex);
        free(meter);
        return EC_METER_NOT_EXIST;
    }

    meter->next = p_meter->next;
    meter->pre = p_meter->pre;
    free(p_meter);

    if(meter->pre)
    {
        meter->pre->next = meter;
    }
    else
    {
        sw->meter_entries = meter;
    }

    if(meter->next)
    {
        meter->next->pre = meter;
    }

    meter_mod_req_info.command = OFPMC_MODIFY;
    ret = sw->msg_driver.msg_handler[OFPT13_METER_MOD](sw, (UINT1 *)&meter_mod_req_info);

    pthread_mutex_unlock(&sw->meter_entry_mutex);
    return ret;
}

/* by:yhy 
 * 删除sw上的meter
 */
INT4 delete_meter_entry(gn_switch_t *sw, gn_meter_t *meter)
{
    INT4 ret = 0;
    meter_mod_req_info_t meter_mod_req_info;
    if(INITSTATE == sw->conn_state)
    {
        return EC_SW_STATE_ERR;
    }

    memset(&meter_mod_req_info, 0, sizeof(meter_mod_req_info_t));
    pthread_mutex_lock(&sw->meter_entry_mutex);
    gn_meter_t *p_meter = find_meter_by_id(sw, meter->meter_id);

    if(NULL == p_meter)
    {
        meter_mod_req_info.meter = meter;
    }
    else
    {
        if(p_meter->pre)
        {
            p_meter->pre->next = p_meter->next;
            if(p_meter->next)
            {
                p_meter->next->pre = p_meter->pre;
            }
        }
        else
        {
            sw->meter_entries = p_meter->next;
            if(p_meter->next)
            {
                p_meter->next->pre = NULL;
        }
    }

    meter_mod_req_info.meter = p_meter;
    }

    meter_mod_req_info.command = OFPMC_DELETE;
    ret = sw->msg_driver.msg_handler[OFPT13_METER_MOD](sw, (UINT1 *)&meter_mod_req_info);

    free(p_meter);
    pthread_mutex_unlock(&sw->meter_entry_mutex);

    return ret;
}
/* by:yhy
 * 清空sw上的所有meter
 */
void clear_meter_entries(gn_switch_t *sw)
{
    meter_mod_req_info_t meter_mod_req_info;
    gn_meter_t meter;
    gn_meter_t *p_meter = sw->meter_entries;
    pthread_mutex_lock(&sw->meter_entry_mutex);
    while(sw->meter_entries)
    {
        p_meter = sw->meter_entries;
        sw->meter_entries = p_meter->next;
        free(p_meter);
    }

    sw->meter_entries = NULL;
    pthread_mutex_unlock(&sw->meter_entry_mutex);

    meter_mod_req_info.meter = &meter;
    meter.meter_id = OFPM_ALL;
	meter.type = OFPMBT_DROP;
    meter_mod_req_info.command = OFPMC_DELETE;
    sw->msg_driver.msg_handler[OFPT13_METER_MOD](sw, (UINT1 *)&meter_mod_req_info);
}

/* by:yhy
 *
 */
INT4 init_meter_mgr()
{
    return GN_OK;
}
/* by:yhy
 *
 */
void fini_meter_mgr()
{
}

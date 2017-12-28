#include "qos-meter.h"
#include "mem_pool.h"
#include "openflow-common.h"
#include "fabric_impl.h"
#include "qos-mgr.h"
#include "../meter-mgr/meter-mgr.h"

#define QOS_METER_LIST_MAX_NUM	40960
#define QOS_SWTICH_MAX_TAG  4096

void *g_qos_meter_id = NULL;
qos_meter_p g_qos_meter_list = NULL;

INT4 g_meter_id[QOS_SWTICH_MAX_TAG][QOS_METER_LIST_MAX_NUM];


INT4 gen_qos_meter_id(gn_switch_t* sw);

qos_meter_p create_qos_policy_meter(qos_policy_p policy_p);

qos_meter_p update_qos_policy_meter(qos_policy_p policy_p);



/* by:yhy
 * 生成meter id
 */
INT4 gen_qos_meter_id(gn_switch_t* sw)
{
	UINT4 tag = of131_fabric_impl_get_tag_dpid(sw->dpid);

	if (0 == tag) 
	{
		return GN_ERR;
	}
	
	INT4 seq = 1;

	for (; seq < QOS_METER_LIST_MAX_NUM; seq++) 
	{
		if (0 == g_meter_id[tag][seq]) 
		{
			g_meter_id[tag][seq] = 1;
			return seq;	
		}
	}

	return 0;
}

/* by:yhy
 * 初始化QOS meter功能相关内存
 */
INT4 init_qos_policy_meter()
{
	destory_qos_policy_meter();
	g_qos_meter_id = mem_create(sizeof(qos_meter), QOS_METER_LIST_MAX_NUM);

	if (NULL == g_qos_meter_id) 
	{
		LOG_PROC("ERROR", "Can't initialize qos meter.");
		return GN_ERR;
	}

	return GN_OK;
}

/* by:yhy
 * 销毁QOS meter功能相关内存
 */
INT4 destory_qos_policy_meter()
{
	if (NULL != g_qos_meter_id) 
	{
		mem_destroy(g_qos_meter_id);
		g_qos_meter_id = NULL;
	}
	g_qos_meter_list = NULL;
	return GN_OK;
}

/* by:yhy
 * 查找QOS策略
 */
INT4 find_qos_policy_meter(qos_policy_p policy_p)
{
	if (NULL == policy_p) 
	{
		return 0;
	}
	qos_meter_p list_p = g_qos_meter_list;
	qos_meter_p qos_service = (qos_meter_p)policy_p->qos_service;

	if ((NULL == qos_service) || (0 == qos_service->meter_id)) 
	{
		return 0;
	}
	// loop to find the qos meter
	while (list_p) 
	{
		if (qos_service->meter_id == list_p->meter_id) 
		{
			return qos_service->meter_id;
		}
		list_p = list_p->next;
	}
	return 0;
}

/* by:yhy
 * 根据策略,新建一个meter
 */
qos_meter_p create_qos_policy_meter(qos_policy_p policy_p)
{
	qos_meter_p meter_p = (qos_meter_p)mem_get(g_qos_meter_id);

	if (NULL == meter_p) 
	{
		LOG_PROC("INFO", "Fail to create qos policy meter.");
		return NULL;
	}

	meter_p->meter_id = gen_qos_meter_id(policy_p->sw);

	if (0 == meter_p->meter_id) 
	{
		return NULL;
	}

	gn_meter_t* meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));
	
	meter->meter_id = meter_p->meter_id;
	meter->burst_size = policy_p->burst_size;
	meter->flags = OFPMF_KBPS;
	meter->rate = policy_p->max_speed;
	meter->type = OFPMBT_DROP;
	
 	add_meter_entry(policy_p->sw, meter);
	
	return meter_p;
}

/* by:yhy
 * 增加QOS meter
 */
qos_meter_p add_qos_policy_meter(qos_policy_p policy_p)
{	
	if (NULL == policy_p) 
	{
		return NULL;
	}

	qos_meter_p meter_p = NULL;

	if (find_qos_policy_meter(policy_p)) 
	{
		meter_p = update_qos_policy_meter(policy_p);
	}
	else 
	{
		meter_p = create_qos_policy_meter(policy_p);
		if (NULL != meter_p) 
		{
			meter_p->next = g_qos_meter_list;
			g_qos_meter_list = meter_p;
		}
	}

	return meter_p;
}

/* by:yhy
 * 更新QOS策略
 */
qos_meter_p update_qos_policy_meter(qos_policy_p policy_p)
{	
	if ((NULL == policy_p) || (NULL == policy_p->qos_service)) 
	{
		return NULL;
	}
		
	qos_meter_p meter_p = (qos_meter_p)policy_p->qos_service;
	gn_meter_t* meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));
	if (NULL == meter) 
	{
		LOG_PROC("INFO", "Fail to malloc gn_meter_t.");
		return NULL;
	}
	meter->meter_id = meter_p->meter_id;
	meter->burst_size = policy_p->burst_size;
	meter->flags = OFPMF_KBPS;
	meter->rate = policy_p->max_speed;
	meter->type = OFPMBT_DROP;
	
	modify_meter_entry(policy_p->sw, meter);

	return meter_p;
}

/* by:yhy
 * 删除sw上meter_id对应的meter
 */
INT4 delete_qos_policy_meter(gn_switch_t* sw, UINT4 meter_id)
{
	gn_meter_t* meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));

	if (NULL == meter) 
	{
		LOG_PROC("INFO", "Failed to malloc gn_mter_t.");
		return GN_ERR;
	}
	
	meter->meter_id = meter_id;
	delete_meter_entry(sw, meter);
	
	return GN_OK;
}



#include "qos-policy.h"
#include "mem_pool.h"
#include "common.h"
#include "fabric_openstack_external.h"
#include "ini.h"
#include "../conn-svr/conn-svr.h"
#include "qos-meter.h"
#include "qos-policy-file.h"
#include "qos-mgr.h"

/* global definition */
INT1 g_qos_id[QOS_POLICY_MAX_NUM] = {0};

void *g_qos_policy_id = NULL;
qos_policy_p g_qos_policy_list = NULL;


static INT4 set_qos_policy_id(INT4 id, INT4 status);

INT4 get_qos_policy_id(INT4 id);

qos_policy_p create_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, 
							   UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority);




/* by:yhy
 *
 */
INT4 init_qos_policy()
{
	destroy_qos_policy();
	
	g_qos_policy_id = mem_create(sizeof(qos_policy), QOS_POLICY_MAX_NUM);

	if (NULL == g_qos_policy_id) 
	{
		LOG_PROC("ERROR", "Create qos policy failed.");
		return GN_ERR;
	}

	return GN_OK;
}

/* by:yhy
 *
 */
INT4 destroy_qos_policy()
{
	if (g_qos_policy_id != NULL) 
	{
		mem_destroy(g_qos_policy_id);
		g_qos_policy_id = NULL;
	}

	g_qos_policy_list = NULL;

	return GN_OK;
}

/* by:yhy
 *
 */
INT4 gen_qos_policy_id(gn_switch_t* sw, UINT4 dst_ip)
{
	INT4 seq = 1;

	for (; seq < QOS_POLICY_MAX_NUM; seq++) 
	{
		if (0 == g_qos_id[seq]) 
		{
			g_qos_id[seq] = 1;
	
			return seq;	
		}
	}

	return 0;
}

/* by:yhy
 *
 */
static INT4 set_qos_policy_id(INT4 id, INT4 status)
{
	g_qos_id[id] = status;

	return 0;
}

/* by:yhy
 *
 */
INT4 get_qos_policy_id(INT4 id)
{
	return g_qos_id[id];
}

/* by:yhy
 * 创建QOS策略
 */
qos_policy_p create_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority)
{
	qos_policy_p policy_p = (qos_policy_p)mem_get(g_qos_policy_id);
	
	if (NULL == policy_p) 
	{
		LOG_PROC("INFO", "Fail to create qos policy, can't get memory");
		return NULL;
	}

	INT4 qos_policy_id = (0 == id) ? gen_qos_policy_id(sw, dst_ip) : id;

	if (0 == qos_policy_id) 
	{
		LOG_PROC("INFO", "Fail to generate qos policy id.");
		return NULL;
	}
	else 
	{
		strcpy(policy_p->qos_policy_name, name);
		policy_p->qos_policy_id = qos_policy_id;
		policy_p->sw= sw;
		policy_p->dst_ip = dst_ip;
		policy_p->min_speed = min_speed;
		policy_p->max_speed = max_speed;
		policy_p->burst_size = burst;
		policy_p->priority = priority;
	}
	
	return policy_p;
}

/* by:yhy
 * 添加QOS策略(无则新建,有则更新)
 */
qos_policy_p add_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, 
							    UINT8 max_speed, UINT8 burst, UINT4 priority)
{
	qos_policy_p policy_p = find_qos_policy_by_sw_dstip(sw, dst_ip);

	if (NULL == policy_p) 
	{
		policy_p = create_qos_policy(name, id, sw, dst_ip, min_speed, max_speed, burst, priority);
		
		if (policy_p) 
		{
			policy_p->next = g_qos_policy_list;
			g_qos_policy_list = policy_p;
		}
	}
	else 
	{
		update_qos_policy(name, id, sw, dst_ip, min_speed, max_speed, burst, priority, policy_p);
	}

	return policy_p;
}

/* by:yhy
 * 更新QOS策略
 */
INT4 update_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, 
						   UINT8 max_speed, UINT8 burst, UINT4 priority, qos_policy_p policy_p)
{
	set_qos_policy_id(id, 1);

	if (strlen(name))
		strcpy(policy_p->qos_policy_name, name);
	policy_p->sw= sw;
	policy_p->dst_ip = dst_ip;
	if (min_speed)
		policy_p->min_speed = min_speed;
	if (max_speed)
		policy_p->max_speed = max_speed;
	if (priority)
		policy_p->priority = priority;
	if (id)
		policy_p->qos_policy_id= id;
	if (burst)
		policy_p->burst_size = burst;

	return GN_OK;
}



/* by:yhy
 * 在QOS策略表中找到匹配sw和dst_ip的策略项
 */
qos_policy_p find_qos_policy_by_sw_dstip(gn_switch_t* sw, UINT4 dst_ip)
{
	qos_policy_p list = g_qos_policy_list;

	while (list) 
	{
		if ((list->sw == sw) && (list->dst_ip == dst_ip)) 
		{
			return list;
		}

		list = list->next;
	}

	return NULL;
}

/* by:yhy
 *
 */
INT4 delete_qos_policy_by_sw_dstip(gn_switch_t* sw, UINT4 dst_ip)
{
	if (NULL == sw)
	{
		return GN_ERR;
	}

	UINT4 id = 0;

	qos_policy_p prev_p = g_qos_policy_list;

	if (NULL == prev_p) 
	{
		return id;
	}
	
	qos_policy_p next_p = prev_p->next;

	if ((prev_p->sw == sw) && (prev_p->dst_ip == dst_ip)) 
	{

		id = prev_p->qos_policy_id;
		g_qos_policy_list = prev_p->next;
		set_qos_policy_id(prev_p->qos_policy_id, 0);
		//remove_qos_policy_from_file(prev_p);
		mem_free(g_qos_policy_id, prev_p);
	}

	while ((prev_p) && (next_p)) 
	{
		if ((next_p->sw == sw) && (next_p->dst_ip == dst_ip)) 
		{
			
			id = next_p->qos_policy_id;
			prev_p->next = next_p->next;
			set_qos_policy_id(next_p->qos_policy_id, 0);
			//remove_qos_policy_from_file(next_p);
			mem_free(g_qos_policy_id, next_p);
		}

		prev_p = prev_p->next;
		if(prev_p)
		{
			next_p = prev_p->next;
		}
		
	}

	return id;
}

/* by:yhy
 * 获取QOS策略
 */
qos_policy_p get_qos_policy_list()
{
	return g_qos_policy_list;
}

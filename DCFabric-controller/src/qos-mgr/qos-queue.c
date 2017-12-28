#include "qos-queue.h"
#include "mem_pool.h"
#include "gnflush-types.h"
#include "qos-queue-ovsdb.h"
#include "fabric_impl.h"
#include "openflow-common.h"
#include "../cluster-mgr/cluster-mgr.h"
/* global defines */
void *g_qos_queue_id = NULL;
INT1 g_queue_id[1000] = {0};


INT4 gen_qos_queue_id(gn_switch_t* sw, UINT4 port_no);

gn_queue_t* create_qos_policy_queue(qos_policy_p policy_p);

gn_queue_t* update_qos_policy_queue(qos_policy_p policy_p, gn_queue_t* queue);

gn_qos_t* find_qos_by_queue(gn_switch_t* sw, gn_queue_t* queue);

gn_qos_t* find_qos_by_port_no(gn_switch_t* sw, UINT4 port_no);

INT4 add_qos_entery(gn_switch_t* sw, UINT4 port_no);



/* by:yhy
 * 生成队列id
 */
INT4 gen_qos_queue_id(gn_switch_t* sw, UINT4 port_no)
{
	if ((NULL == sw) || (0 == port_no))
	{
		return 0;
	}

	gn_port_t* port_p = NULL;

	INT4 idx = 0;

	for (; idx < MAX_PORTS; idx++)
	{
		port_p = &sw->ports[idx];
		if (port_p->port_no == port_no)
		{
			break;
		}
	}

	if (NULL == port_p)
	{
		for (idx = 0; idx < MAX_PORTS; idx++) 
		{
			neighbor_t* nei = sw->neighbor[idx];
			//if ((nei) && (nei->port) && (nei->port->port_no == port_no)) {
			if ((nei->bValid) && (nei->port) && (nei->port->port_no == port_no)) 
			{
				port_p = nei->port;
				break;
			}
		}
	}

	if ((NULL == port_p) || (port_p->port_no != port_no))
	{
		return 0;
	}

	INT4 seq = 1;

	port_p->queue_ids[1] = 1;
	

	for (; seq < MAX_QUEUE_ID; seq++) 
	{
		if (0 == port_p->queue_ids[seq]) 
		{
			port_p->queue_ids[seq] = 1;
	
			return seq;	
		}
	}

	return 0;
}

/* by:yhy
 * 初始化QOS queue功能相关内存
 */
INT4 init_qos_policy_queue()
{
	return GN_OK;
}

// destroy qos policy queue
INT4 destory_qos_policy_queue()
{
	return GN_OK;
}

// find qos queue by policy
/* by:yhy 在policy_p->sw->queue_entries中寻找policy_p是否已经存在
 *
 */
gn_queue_t* find_qos_policy_queue(qos_policy_p policy_p)
{
	if ((NULL == policy_p) || (NULL == policy_p->sw)) 
	{
		return NULL;
	}

	gn_queue_t* queue = policy_p->sw->queue_entries;
	gn_queue_t* policy_queue = (gn_queue_t*)policy_p->qos_service;

	if ((NULL == queue) || (NULL == policy_queue)) 
	{
		return NULL;
	}

	while (queue)
	{
		if ((queue->queue_id == policy_queue->queue_id)) 
		{
			return queue;
		}
	
		queue = queue->next;
	}

	return NULL;
}

// create qos policy queue
/*
 *
 */
gn_queue_t* create_qos_policy_queue(qos_policy_p policy_p)
{
	if ((NULL == policy_p) || (NULL == policy_p->sw)) 
	{
		return NULL;
	}

	UINT4 port_no = get_port_no_between_sw_ip(policy_p->sw, policy_p->dst_ip);
	if (0 == port_no) 
	{
		return NULL;
	}

	UINT4 queue_id = gen_qos_queue_id(policy_p->sw, port_no);
		
	if (0 == queue_id) 
	{
		LOG_PROC("ERROR", "Fail to generate queue id.");
		return NULL;
	}

	gn_queue_t* queue = (gn_queue_t*)gn_malloc(sizeof(gn_queue_t));

	if (NULL == queue) 
	{
		LOG_PROC("ERROR", "Create qos policy queue failed.");
		return NULL;
	}	
	
	queue->queue_id = queue_id;	
	queue->min_rate = policy_p->min_speed ;
	queue->max_rate = policy_p->max_speed ;
	queue->burst = policy_p->burst_size ;
	queue->priority = policy_p->priority;
	queue->port_no = port_no;

	add_qos_entery(policy_p->sw, port_no);
	add_queue_in_queue_table(policy_p->sw, queue);

	return queue;
}

// update qos policy queue
/* by:yhy
 * 更新QOS策略队列
 */
gn_queue_t* update_qos_policy_queue(qos_policy_p policy_p, gn_queue_t* queue)
{
	if ((NULL == policy_p) || (NULL == queue))
	{
		return NULL;
	}

	// update qos policy 
	queue->min_rate = policy_p->min_speed ;
	queue->max_rate = policy_p->max_speed ;
	queue->burst = policy_p->burst_size ;
	queue->priority = policy_p->priority;

	update_queue_in_queue_table(policy_p->sw, queue);

	return queue;
}

// add qos policy queue
/* by:yhy
 * 添加QOS策略队列(若存在则更新,不存在则新建)
 */
gn_queue_t* add_qos_policy_queue(qos_policy_p policy_p)
{		
	gn_queue_t* queue_p = find_qos_policy_queue(policy_p);

	if (queue_p) 
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		queue_p = update_qos_policy_queue(policy_p, queue_p);
	}
	else 
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		queue_p = create_qos_policy_queue(policy_p);
	}

	return queue_p;
}

// find qos by queue
gn_qos_t* find_qos_by_queue(gn_switch_t* sw, gn_queue_t* queue) 
{
	gn_qos_t* qos = sw->qos_entries;

	while (qos) 
	{
		if (qos->port_no == queue->port_no) 
		{
			return qos;
		}	
		qos = qos->next;
	}

	return NULL;
}
/* by:yhy
 * 在sw上添加针对port_no的QOS queue
 */
INT4 add_qos_entery(gn_switch_t* sw, UINT4 port_no)
{
	if (NULL == find_qos_by_port_no(sw, port_no)) 
	{
		//LOG_PROC("INFO", "%s_%d",FN,LN);
		gn_qos_t* qos = (gn_qos_t*)gn_malloc(sizeof(gn_qos_t));

		if (NULL == qos) 
		{
			LOG_PROC("INFO", "Fail to malloc qos add qos policy.");
			return GN_ERR;
		}

		qos->port_no = port_no;

		add_qos_in_qos_table(sw, qos);

		//create_default_queue_in_queue_table(sw, port_no);
	}

	return GN_OK;
}

/* by:yhy
 * 查找sw->qos_entries中匹配port_no的项
 */
gn_qos_t* find_qos_by_port_no(gn_switch_t* sw, UINT4 port_no)
{
	gn_qos_t* qos = sw->qos_entries;

	while (qos)
	{
		if (qos->port_no == port_no) 
		{
			return qos;
		}
	}
	
	return NULL;
}




// delete qos policy queue
INT4 delete_qos_policy_queue(gn_switch_t* sw, gn_queue_t* queue)
{
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return GN_OK;
	}
	gn_qos_t* qos = find_qos_by_queue(sw, queue);

	if (qos) 
	{
		delete_queue_in_qos_table(sw, qos->qos_uuid, queue);
	}

	delete_queue_in_queue_table(sw, queue);
	
	return GN_OK;
}


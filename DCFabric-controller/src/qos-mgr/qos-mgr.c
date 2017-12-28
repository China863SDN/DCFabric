#include "qos-mgr.h"
#include "ini.h"
#include "qos-meter.h"
#include "qos-policy.h"
#include "qos-policy-file.h"
#include "qos-queue.h"
#include "timer.h"
#include "fabric_impl.h"
#include "fabric_openstack_external.h"
#include "../conn-svr/conn-svr.h"
#include "../ovsdb/ovsdb.h"
#include "../cluster-mgr/cluster-mgr.h"

/* global define */
INT4 g_qos_status = 0;		// 0:off, 1: on
INT4 g_qos_type = 0; 		// 0: off; 1: auto; 2:meter; 3:queue

void *g_qos_mgr_timerid = NULL;
UINT4 g_qos_mgr_interval = 30;
void *g_qos_mgr_timer = NULL;

//by:yhy 最大速率 bps
const UINT8 const_max_speed = 1099511627776;			//1024*1024*1024*1024

/* Internal function */

INT4 add_qos_rule(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, 
					 UINT8 max_speed, UINT8 burst, UINT4 priority);

INT4 add_qos_rule_service(qos_policy_p policy_p);

INT4 delete_qos_rule(gn_switch_t* sw, UINT4 dst_ip);

INT4 reload_qos_policy();


void qos_mgr_timer_thread(void *para, void *tid);


/* by:yhy
 * 读取QOS配置文件,初始化参数
 */
void init_qos_mgr()
{
	INT1* value = get_value(g_controller_configure, "[openvstack_conf]", "qos_on");
	g_qos_status = (NULL == value) ? 0 : atoi(value);

	value = get_value(g_controller_configure, "[openvstack_conf]", "qos_type");
	if ((NULL == value) || (0 == strcmp("off", value))) 
	{
		g_qos_type = QOS_TYPE_OFF;
	}
	else if (0 == strcmp("auto", value)) 
	{
		g_qos_type = QOS_TYPE_AUTO;
	}
	else if (0 == strcmp("meter", value)) 
	{
		g_qos_type = QOS_TYPE_METER;
	}
	else if (0 == strcmp("queue", value)) 
	{
		g_qos_type = QOS_TYPE_QUEUE;
	}
	else 
	{
		g_qos_type = QOS_TYPE_OFF;
	}

	value = get_value(g_controller_configure, "[openvstack_conf]", "qos_interval");
	g_qos_mgr_interval = (NULL == value) ? 30 : atoi(value);

	/*
	void *g_auto_timerid = NULL;
	UINT4 g_auto_interval = 30;
	void *auto_qos_timer = NULL;
    g_auto_timerid = timer_init(1);
    timer_creat(g_auto_timerid, g_auto_interval, NULL, &auto_qos_timer, start_qos_mgr);
    */
}

/* by:yhy 启动QOS
 * 未被调用
 */
void start_qos_mgr()
{
	
	init_qos_policy_meter();
	init_qos_policy_queue();
	

	LOG_PROC("INFO", "Start Qos Service.");


	init_qos_mgr_timer();
	
	init_qos_policy();

	//init_qos_policy_file();
}


void destroy_qos_mgr()
{
	// do nothing
}

INT4 get_qos_mgr_status()
{
	return g_qos_status;
}

/* by:yhy
 * 获取QOS功能的类型
 */
INT4 get_qos_mgr_type()
{
	if (0 == get_qos_mgr_status()) 
	{
		return QOS_TYPE_OFF;
	}
	
	return (INT4)g_qos_type;
}

/* by:yhy
 * 添加QOS规则
 */
INT4 add_qos_rule(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority)
{
	// add qos policy
	qos_policy_p policy_p = add_qos_policy(name, id, sw, dst_ip, min_speed, max_speed,burst, priority);

	if (NULL == policy_p) 
	{
		LOG_PROC("ERROR", "Add qos policy failed.");
		return GN_ERR;
	}
	if (g_is_cluster_on && g_controller_role != OFPCR_ROLE_MASTER)
	{
		return GN_OK;
	}
	add_qos_rule_service(policy_p);

	//save_qos_policy_to_file(policy_p);

	return GN_OK;
}

/* by:yhy
 * 添加QOS策略服务
 */
INT4 add_qos_rule_service(qos_policy_p policy_p)
{
	if ((NULL == policy_p) || (NULL == policy_p->sw)) 
	{
		return GN_ERR;
	}
	if (QOS_TYPE_METER == policy_p->sw->qos_type) 
	{
		policy_p->qos_service = (void*)add_qos_policy_meter(policy_p);
	}
	else if (QOS_TYPE_QUEUE == policy_p->sw->qos_type) 
	{
		// since queue must be combined with the port, so the policy without certern port won't be added
		// and qos_mgr_time_thread will scan the ones without servcie timely
		UINT4 port_no = get_port_no_between_sw_ip(policy_p->sw, policy_p->dst_ip);
		if (0 == port_no) 
		{
			return GN_ERR;
		}
		policy_p->qos_service = (void*)add_qos_policy_queue(policy_p);
	}
	else 
	{
		// do nothing
	}
	return GN_OK;
}

 
/* by:yhy
 * 
 */
INT4 delete_qos_rule(gn_switch_t* sw, UINT4 dst_ip)
{
	qos_policy_p policy_p = find_qos_policy_by_sw_dstip(sw, dst_ip);

	if ((NULL == policy_p) || (NULL == policy_p->qos_service)) 
	{
		LOG_PROC("INFO", "Fail to delete qos rule. rule not exist.");
		return GN_ERR;
	}

	// judge qos type, and save various service data
	if (QOS_TYPE_METER == sw->qos_type) 
	{
		qos_meter_p meter = (qos_meter_p)policy_p->qos_service;
		delete_qos_policy_meter(sw, meter->meter_id);
	}
	else if (QOS_TYPE_QUEUE == sw->qos_type) 
	{
		gn_queue_t* queue = (gn_queue_t*)policy_p->qos_service;
		delete_qos_policy_queue(sw, queue);
	}
	else 
	{
		// do nothing
	}	

	UINT4 return_id = delete_qos_policy_by_sw_dstip(sw, dst_ip);

	if (0 == return_id) 
	{
		LOG_PROC("INFO", "Delete qos policy failed.");
		return GN_ERR;
	}

	return GN_OK;
}

/* by:yhy
 * 根据参数添加QOS规则
 */
INT4 add_qos_rule_by_rest_api(INT1* name, INT4 id, INT4 sw_type, INT1* sw_param, UINT4 dst_ip, UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority)
{
	if (NULL == sw_param) 
	{
		LOG_PROC("INFO", "Fail to add qos rule by rest api. sw param unknown.");
		return GN_ERR;
	}

	if (0 == strlen(name)) 
	{
		strcpy(name, "default");
	}

	gn_switch_t* sw = NULL;

	switch (sw_type) 
	{		
		case SW_DPID: 
		{
			UINT8 dpid;
			dpidStr2Uint8(sw_param, &dpid);
			sw = find_sw_by_dpid(dpid);
			break;
		}
		case SW_ALL: 
		{
			// TODO
			LOG_PROC("INFO", "All SW unspoorted.");
			break;
		}	
		case SW_EXTERNAL: 
		{
			external_port_p external_p = get_external_port();
			if (external_p)
			{
				sw = find_sw_by_dpid(external_p->external_dpid);
			}
			break;
		}
		default: 
		{
			LOG_PROC("INFO", "Unknow param.");
			break;	
		}
	}

	// 1024*1024*1024*1024
	if ((min_speed > const_max_speed) || (max_speed > const_max_speed) || (burst > const_max_speed)) 
	{
		LOG_PROC("INFO", "qos speed out of range. max counst is %llu", const_max_speed);
		return 0;
	}

	// install qos policy
	if ((sw) && (QOS_TYPE_OFF != sw->qos_type)) 
	{
		add_qos_rule(name, 0, sw, dst_ip, min_speed, max_speed, burst, priority);
	}

	return 0;
	
}


// delete qos rule
INT4 delete_qos_rule_by_rest_api(INT4 sw_type, INT1* sw_param, UINT4 dst_ip)
{
	if (NULL == sw_param) 
	{
		LOG_PROC("INFO", "Fail to delete qos ruel by rest api. sw param unknown.");
		return GN_ERR;
	}

	gn_switch_t* sw = NULL;
	
	switch (sw_type) 
	{
		case SW_DPID: 
		{
			UINT8 dpid;
			dpidStr2Uint8(sw_param, &dpid);
			sw = find_sw_by_dpid(dpid);
			break;
		}
		case SW_ALL: 
		{
			LOG_PROC("INFO", "All SW unspoorted.");
			break;
		}
		case SW_EXTERNAL: 
		{
			external_port_p external_p = get_external_port();
			if (external_p) 
			{
				sw = find_sw_by_dpid(external_p->external_dpid);
			}
			break;
		}
		default: 
		{
			LOG_PROC("INFO", "Unknow param.");
			break;
		}
	}
	if (sw) 
	{
		delete_qos_rule(sw, dst_ip);
	}

	return GN_OK;
}

/* by:yhy
 * 刷新QOS策略
 */
INT4 reload_qos_policy()
{
	qos_policy_p list = get_qos_policy_list();
	while (list) 
	{
		if (NULL == list->qos_service) 
		{
			add_qos_rule_service(list);
		}

		list = list->next;
	}
	return GN_OK;
}


/* by:yhy
 * QOS定时器执行功能: 刷新qos策略
 */
void qos_mgr_timer_thread(void *para, void *tid)
{
	reload_qos_policy();
}

/* by:yhy
 * 启动QOS功能用定时器
 */
INT4 init_qos_mgr_timer()
{
	UINT qos_type = get_qos_mgr_type();

	if ((QOS_TYPE_QUEUE == qos_type) || (QOS_TYPE_AUTO == qos_type))
	{
		qos_mgr_timer_thread(NULL, NULL);
		g_qos_mgr_timerid = timer_init(1);
		timer_creat(g_qos_mgr_timerid, g_qos_mgr_interval, NULL, &g_qos_mgr_timer, qos_mgr_timer_thread);
	}

	return GN_OK;
}

/* by:yhy
 * 
 */
INT4 init_sw_qos_type(gn_switch_t* sw)
{
	INT4 return_value = QOS_TYPE_OFF;

	if (0 == get_qos_mgr_status()) 
	{
		return_value = QOS_TYPE_OFF;
	}

	INT4 type = get_qos_mgr_type();

	switch(type) 
	{
		case QOS_TYPE_OFF: 
			/* no break */
		case QOS_TYPE_METER: 
			/* no break */
		case QOS_TYPE_QUEUE: 
		{
			return_value = type;
			break;
		}
		case QOS_TYPE_AUTO: 
		{
			return_value = (0 == get_conn_fd_by_sw_ip(sw->sw_ip)) ? QOS_TYPE_METER : QOS_TYPE_QUEUE;
			break;
		}
		default:
			break;

	}

	return return_value;
}


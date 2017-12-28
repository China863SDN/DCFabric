#ifndef QOS_POLICY_H_
#define QOS_POLICY_H_

#include "common.h"
#include "gnflush-types.h"

#define QOS_POLICY_MAX_NUM		10000
#define QOS_PORT_MAX_NUM		1000
#define QOS_POLICY_NAME_LENGTH 	64

enum SW_PARAM_TYPE
{
	SW_INVALID = 0,			// invalid
	SW_DPID = 1, 			// dpid
	SW_ALL = 2, 			// all sw
	SW_EXTERNAL = 3			// external sw
};



/* by:yhy
 * QOS策略
 */
typedef struct _qos_policy 
{
	INT4 qos_status;				//0: not installed
	INT1 qos_policy_name[QOS_POLICY_NAME_LENGTH];
	INT4 qos_policy_id;
	gn_switch_t* sw;
	UINT4 dst_ip;					//qos针对的ip
	UINT8 min_speed;
	UINT8 max_speed;
	UINT4 priority;
	UINT8 burst_size;
	void* qos_service;				//qos服务是 qn_queue_t 还是 qos_meter_p 
	struct _qos_policy* next;
}qos_policy, *qos_policy_p;

INT4 init_qos_policy();

INT4 destroy_qos_policy();

INT4 gen_qos_policy_id(gn_switch_t* sw, UINT4 port_no);

qos_policy_p add_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, 
					   		   UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority);

INT4 update_qos_policy(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, 
						   UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority, qos_policy_p policy_p);

qos_policy_p find_qos_policy_by_sw_dstip(gn_switch_t* sw, UINT4 dst_ip);

INT4 delete_qos_policy_by_sw_dstip(gn_switch_t* sw, UINT4 dst_ip);

qos_policy_p get_qos_policy_list();
#endif


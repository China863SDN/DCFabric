#ifndef QOS_MGR_H_
#define QOS_MGR_H_

#include "common.h"
#include "gnflush-types.h"

/*
  * define the qos type
  */
enum QOS_TYPE
{
	QOS_TYPE_OFF = 0,		///< do not use qos 
	QOS_TYPE_AUTO = 1,		///< according to swich's support, decide the qos type
	QOS_TYPE_METER = 2,		///< use meter
	QOS_TYPE_QUEUE = 3		///< use queue
};

void init_qos_mgr();

void start_qos_mgr();

void destroy_qos_mgr();

INT4 get_qos_mgr_status();

INT4 get_qos_mgr_type();

INT4 add_qos_rule_by_rest_api(INT1* name, INT4 id, INT4 sw_type, INT1* sw_param, UINT4 dst_ip, 
								   UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority);

INT4 delete_qos_rule_by_rest_api(INT4 sw_type, INT1* sw_param, UINT4 dst_ip);

INT4 delete_qos_rule(gn_switch_t* sw, UINT4 dst_ip);

INT4 init_qos_mgr_timer();

INT4 init_sw_qos_type(gn_switch_t* sw);
INT4 add_qos_rule(INT1* name, INT4 id, gn_switch_t* sw, UINT4 dst_ip, UINT8 min_speed, UINT8 max_speed, UINT8 burst, UINT4 priority);
#endif


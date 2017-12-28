#ifndef QOS_QUEUE_H_
#define QOS_QUEUE_H_

#include "qos-mgr.h"
#include "qos-policy.h"

// define the qos queue struct
typedef struct _qos_queue 
{
	gn_qos_t* qos;
	gn_queue_t* queue;

	struct _qos_queue* next;
	
}qos_queue, *qos_queue_p;

/*
 * init qos queue
 *
 * @brief: this function is used to initialize qos queue
 *
 * @param: none
 * 
 * @return: INT4				GN_OK: success; GN_ERR: fail
 */
INT4 init_qos_policy_queue();

/*
 * destory qos queue
 *
 * @brief: this function is used to destory qos queue
 *
 * @param: none
 *
 * @return: INT4				GN_OK: success; GN_ERR: fail
 */
INT4 destory_qos_policy_queue();

/*
 * find qos queue by qos policy
 *
 * @brief: this function is used to find qos meter by qos policy
 *
 * @param: policy_p			qos policy
 *
 * @return: INT4				NULL: fail; other: queue pointer
 */
gn_queue_t* find_qos_policy_queue(qos_policy_p policy_p);

// create_qos_policy_queue
// update_qos_policy_queue


/*
 * add qos queue
 * 
 * @brief: this function is used to add qos queue
 * 
 * @param: policy_p 			qos policy
 *
 * @return: gn_queue_t*		NULL: fail; other: queue pointer
 */
gn_queue_t* add_qos_policy_queue(qos_policy_p policy_p);


/*
 * delete qos policy queue
 *
 * @brief: this function is used to delete qos queue
 * 
 * @param: sw				the switch
 * @param: queue_id			the qos queue id
 *
 * @return: INT4				GN_OK:success; GN_ERR:fail
 */
INT4 delete_qos_policy_queue(gn_switch_t* sw, gn_queue_t* queue);
#endif


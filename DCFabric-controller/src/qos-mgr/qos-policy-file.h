#ifndef QOS_POLICY_FILE_H_
#define QOS_POLICY_FILE_H_

#include "qos-policy.h"

void init_qos_policy_file();

INT4 save_qos_policy_to_file();

INT4 remove_qos_policy_from_file(qos_policy_p policy_p);


INT4 read_qos_policy_from_file();


#endif


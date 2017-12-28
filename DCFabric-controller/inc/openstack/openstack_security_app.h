#ifndef SRC_OPENSTACK_OPENSTACK_SECURITY_APP_H_
#define SRC_OPENSTACK_OPENSTACK_SECURITY_APP_H_

#include "gnflush-types.h"
#include "openstack_host.h"
#include "forward-mgr.h"


#define OPENSTACK_SECURITY_GROUP_LEN 48
#define OPENSTACK_SECURITY_GROUP_MAX_NUM 128
#define OPENSTACK_SECURITY_RULE_MAX_NUM 1024









//by:yhy 安全组规则
typedef struct _openstack_security_rule 
{
	char group_id[OPENSTACK_SECURITY_GROUP_LEN];
	char direction[OPENSTACK_SECURITY_GROUP_LEN];
	char ethertype[OPENSTACK_SECURITY_GROUP_LEN];
	char rule_id[OPENSTACK_SECURITY_GROUP_LEN];
	UINT4 port_range_max;
	UINT4 port_range_min;
	char protocol[OPENSTACK_SECURITY_GROUP_LEN];
	char remote_group_id[OPENSTACK_SECURITY_GROUP_LEN];
	char remote_ip_prefix[OPENSTACK_SECURITY_GROUP_LEN];
	char tenant_id[OPENSTACK_SECURITY_GROUP_LEN];
	UINT2 check_status;
	UINT2 priority;
	UINT1 enabled;
	struct _openstack_security_rule* next;
} openstack_security_rule, *openstack_security_rule_p;



//by:yhy 安全组
typedef struct _openstack_security 
{
	char security_group[OPENSTACK_SECURITY_GROUP_LEN];		//安全组名称
	openstack_security_rule_p security_rule_p;				//安全组规则链表
	struct _openstack_security* next;
} openstack_security, *openstack_security_p;




//by:yhy group链表
extern openstack_security_p g_openstack_security_list ;
extern openstack_security_p g_openstack_security_list_temp ;
//by:yhy 每个虚机的安全组信息内存池
extern void *g_openstack_host_security_id ;
//by:yhy group内存池
extern void *g_openstack_security_group_id ;
extern void *g_openstack_security_group_id_temp ;
//by:yhy rule内存池
extern void *g_openstack_security_rule_id ;


UINT4 get_security_group_on_config();

openstack_security_rule_p create_security_rule(char* group_id, char* rule_id, char* direction, char* ethertype, char* port_range_max,char* port_range_min, char* protocol, char* remote_group_id, char* remote_ip_prefix, char* tenant_id,UINT2 priority,UINT1 enabled);
openstack_security_p add_security_rule_into_group(char* group_id, openstack_security_rule_p rule_p);



INT4 openstack_security_group_main_check(p_fabric_host_node src_port, p_fabric_host_node dst_port,packet_in_info_t *packet_in, security_param_t* src_security, security_param_t* dst_security);



#endif /* SRC_OPENSTACK_OPENSTACK_SECURITY_APP_H_ */

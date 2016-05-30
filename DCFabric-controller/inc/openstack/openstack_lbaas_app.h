/*
 * openstack_lbaas_app.h
 *
 *  Created on: 1 7, 2016
 *      Author: yang
 */

#ifndef INC_OPENSTACK_LBAAS_APP_H_
#define INC_OPENSTACK_LBAAS_APP_H_

#include "gnflush-types.h"
#include "openstack_host.h"

#define OPENSTACK_LBAAS_LEN 48
#define OPENSTACK_LBAAS_POOLS_MAX_NUM 48
#define OPENSTACK_LBAAS_MEMBERS_MAX_NUM 512
#define OPENSTACK_LBAAS_LISTENER_MAX_NUM 48
#define OPENSTACK_LBAAS_CONNECT_MAX_NUM 102400
#define OPENSTACK_LBAAS_LISTENER_MEMBER_MAX_NUM OPENSTACK_LBAAS_LISTENER_MAX_NUM*OPENSTACK_LBAAS_LISTENER_MAX_NUM

#define OPENSTACK_LBAAS_NODE_MAX_NUM OPENSTACK_LBAAS_POOLS_MAX_NUM+OPENSTACK_LBAAS_MEMBERS_MAX_NUM*3+OPENSTACK_LBAAS_LISTENER_MAX_NUM + OPENSTACK_LBAAS_CONNECT_MAX_NUM

/*
 * struct define:
 * 1. openstack_lbaas_node 					// container used to save data
 * 2. openstack_lbaas_members				// lbaas members
 * 3. openstack_lbaas_pools					// lbaas pools
 * 4. openstack_lbaas_listener				// lbaas listeners
 * 5. openstack_lbaas_listener_member		// save listener request
 * 6. openstack_lbaas_connect				// save lbaas connect
 */
typedef struct _openstack_lbaas_node{
	UINT1* data;
	struct _openstack_lbaas_node* next;
}openstack_lbaas_node,*openstack_lbaas_node_p;

typedef struct _openstack_lbaas_members {
	char member_id[OPENSTACK_LBAAS_LEN];
	char tenant_id[OPENSTACK_LBAAS_LEN];
	char pool_id[OPENSTACK_LBAAS_LEN];
	UINT2 weight;
	UINT4 protocol_port;
	UINT1 status;
    UINT1 ping_status;
	UINT4 fixed_ip;
	UINT1 connect_numbers;
	UINT2 check_status;
	openstack_lbaas_node_p connect_ips;
    UINT4 group_flow_installed;
}openstack_lbaas_members, *openstack_lbaas_members_p;

typedef struct _openstack_lbaas_pools {
	char pools_id[OPENSTACK_LBAAS_LEN];
	char tenant_id[OPENSTACK_LBAAS_LEN];
	UINT1 protocol;
	UINT4 protocol_port;
	UINT1 status;
	UINT1 vips_status;
	UINT1 lbaas_method;
	UINT4 ipaddress;
	UINT1 connect_limit;
	UINT1 session_persistence;
	UINT4 weight_count;
	UINT2 check_status;
	openstack_lbaas_node_p last_round_robin_member;
	openstack_lbaas_node_p pool_member_list;
    UINT4 group_flow_installed;
} openstack_lbaas_pools, *openstack_lbaas_pools_p;

typedef struct openstack_lbaas_listener_member
{
	p_fabric_host_node dst_port;
	UINT4 dst_ip;
	// UINT1 status;
	UINT4 protocol_port;
	char member_id[OPENSTACK_LBAAS_LEN];
	UINT4 init_time;
	UINT1 request_time;
	UINT4 seq_id;
	UINT1 wait_status;
	openstack_lbaas_members_p member_p;
	struct openstack_lbaas_listener_member* next;
}openstack_lbaas_listener_member_t, *openstack_lbaas_listener_member_p;

typedef struct _openstack_lbaas_listener {
	char listener_id[OPENSTACK_LBAAS_LEN];
	UINT1 type;
    UINT4 check_frequency;
	UINT4 overtime;
	UINT1 retries;
	UINT2 check_status;
	pthread_t listener_pid;
	pthread_mutex_t listener_mutex;
	openstack_lbaas_listener_member_p listener_member_list;
} openstack_lbaas_listener, *openstack_lbaas_listener_p;

typedef struct openstack_lbaas_connect
{
	UINT4 ext_ip;
	UINT4 inside_ip;
	UINT4 vip;
	UINT4 src_port_no;
	UINT4 ext_port_no;
}openstack_lbaas_connect_t, *openstack_lbaas_connect_p;

/*
 * enum define:
 * 1. lbaas_status
 * 2. listener_type
 * 3. lbaas_method
 * 4. session_persistence_e
 * 5. protocol_type
 * 6. lbass_node_type
 * 7. lbaas_listener_member_status_type
 */

enum lbaas_status
{
    LBAAS_OK = 1,
    LBAAS_ERROR = 0
};

enum listener_type
{
	LBAAS_LISTENER_PING = 0,
	LBAAS_LISTENER_TCP = 1,
	LBAAS_LISTENER_HTTP=2,
	LBAAS_LISTENER_HTTPS=3
};

enum lbaas_method
{
    LB_M_ROUNT_ROBIN = 0,
	LB_M_LEAST_CONNECTIONS = 1,
	LB_M_SOURCE_IP=2
};

enum session_persistence_e
{
    SEPER_NO_LIMIT = 0,
	SEPER_SOURCE_IP = 1,
	SEPER_HTTP_COOKIE=2,
	SEPER_APP_COOKIE=3
};

enum protocol_type
{
    LBAAS_PRO_HTTP = 0,
	LBAAS_PRO_HTTPS = 1,
	LBAAS_PRO_TCP=2
};

enum lbass_node_type
{
	LBAAS_NODE_POOL = 0,
	LBAAS_NODE_MEMBER,
	LBAAS_NODE_LISTENER,
	LBAAS_NODE_POOL_MEMBER,
	LBAAS_NODE_CONNECT,
};

enum lbaas_listener_member_status_type
{
	LBAAS_LISTENER_MEMBER_INACTIVE = 0,
	LBAAS_LISTENER_MEMBER_ACTIVE = 1,
	LBAAS_LISTENER_MEMBER_INITIALIZE = 2,
};


extern openstack_lbaas_node_p g_openstack_lbaas_members_list;


/*
 * define public functions
 */

/*
 * this function is called to initialize the mem pool
 */
void init_openstack_lbaas();

/*
 * this function is called to destroy the mem pool
 */
void destory_openstack_lbaas();

/*
 * this function is used to update lbaas pool by pool id
 */
openstack_lbaas_pools_p update_openstack_lbaas_pool_by_poolrest(
		char* tenant_id,
		char* pool_id,
		UINT1 status,
		UINT1 protocol,
		UINT1 lbaas_method);

/*
 * this function is used to update lbaas pool by vip
 */
openstack_lbaas_pools_p update_openstack_lbaas_pool_by_viprest(
		char* pool_id,
		UINT4 protocol_port,
		UINT4 ipaddress,
		UINT1 connect_limit,
		UINT1 vips_status,
		UINT1 session_persistence);

/*
 * this function is used to update lbaas member
 */
openstack_lbaas_members_p update_openstack_lbaas_member_by_rest(
		char* member_id,
		char* tenant_id,
		char* pool_id,
		UINT1 weight,
		UINT4 protocol_port,
		UINT1 status,
		UINT4 fixed_ip);

/*
 * this function to update lbaas listener
 */
openstack_lbaas_listener_p update_openstack_lbaas_listener_by_rest(
		char* listener_id,
		UINT1 type,
		UINT4 check_frequency,
		UINT4 overtime,
		UINT1 retries);

/*
 * this function is used to find lbaas pool by ip
 */
openstack_lbaas_pools_p find_openstack_lbaas_pool_by_ip(UINT4 ip);

/*
 * this function is used to remove pool by pool id
 */
openstack_lbaas_pools_p remove_openstack_lbaas_pool_by_pool_id(char* pool_id);

/*
 * this function is used to find lbaas member by ip
 */
openstack_lbaas_members_p find_openstack_lbaas_member_by_ip(UINT4 vip, UINT4 inside_ip);

/*
 * this function is used to remove lbaas member by member id
 */
openstack_lbaas_members_p remove_openstack_lbaas_member_by_member_id(char* member_id);

/*
 * this function is used to remove lbaas listener by listener id
 */
openstack_lbaas_listener_p remove_openstack_lbaas_listener_by_listener_id(char* listener_id);

/*
 * this function is used to get lbaas ip by ip and proto
 */
UINT4 get_openstack_lbaas_ip_by_ip_proto(UINT4 ip, UINT1 proto);

/*
 * this function is used to update listener member status
 */
void update_openstack_lbaas_listener_member_status(UINT1 type, p_fabric_host_node dst_port, UINT4 seq_id, UINT2 port_no, UINT1 code);

/*
 * this function is used to start lbaas listener thread
 */
void start_openstack_lbaas_listener();

/*
 *  this function is used to create openstack lbaas connect
 */
UINT4 create_openstack_lbaas_connect(UINT4 ext_ip, UINT4 inside_ip, UINT4 vip, UINT4 src_port_no);

/*
 * this function is used to remove lbaas connect by ip
 */
void remove_openstack_lbaas_connect(UINT4 ext_ip, UINT4 inside_ip, UINT4 vip, UINT4 src_port_no);

/*
 * this function is used to remove lbaas connect by ip and port no
 */
void remove_openstack_lbaas_connect_by_ext_ip_portno(UINT4 ext_ip, UINT4 src_port_no);

void clear_openstack_lbaas_info();

void reload_openstack_lbaas_info();



#endif /* INC_OPENSTACK_LBAAS_APP_H_ */

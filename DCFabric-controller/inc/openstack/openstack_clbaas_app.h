/*
 * openstack_lbaas_app.h
 *
 *  Created on: 6 20, 2017
 *      Author: yang
 */

#ifndef INC_OPENSTACK_CLBAAS_APP_H_
#define INC_OPENSTACK_CLBAAS_APP_H_

#include "gnflush-types.h"
#include "openstack_host.h"


#define OPENSTACK_CLBAAS_POOLS_MAX_NUM 48
#define OPENSTACK_CLBAAS_MEMBERS_MAX_NUM 512
#define OPENSTACK_CLBAAS_NODE_MAX_NUM OPENSTACK_LBAAS_POOLS_MAX_NUM*3+OPENSTACK_LBAAS_MEMBERS_MAX_NUM*3


enum clbass_node_type
{
	CLBAAS_NODE_ROUTERSUBNET = 0,
	CLBAAS_NODE_VIPFLOATINGIP,
	CLBAAS_NODE_INTERFACE,
	CLBAAS_NODE_BACKENDIP
};

enum clbass_interface_type
{
	CLBAAS_INTERFACE_UNKOWN = 0,
	CLBAAS_INTERFACE_HA ,
	CLBAAS_INTERFACE_VIP 
};

typedef struct openstack_clbaas_routersubnet
{
	char router_id[OPENSTACK_LBAAS_LEN];
	char subnet_id[OPENSTACK_LBAAS_LEN];
	UINT2 check_status;
	
}openstack_clbaas_routersubnet_t, *openstack_clbaas_routersubnet_p;
typedef struct openstack_clbaas_vipfloating
{
	char pool_id[OPENSTACK_LBAAS_LEN];
	char router_id[OPENSTACK_LBAAS_LEN];
	//char ex_subnet_id[OPENSTACK_LBAAS_LEN];
	char ex_port_id[OPENSTACK_LBAAS_LEN];
	UINT4 ext_ip;
	UINT4 inside_ip;
	UINT2 ext_status;
	UINT2 inside_status;
	
}openstack_clbaas_vipfloating_t, *openstack_clbaas_vipfloating_p;


typedef struct openstack_clbass_loadbalancer
{
	char 	loadbalancer_id[OPENSTACK_LBAAS_LEN];
	char 	pool_id[OPENSTACK_LBAAS_LEN];
	char    tenant_id[OPENSTACK_LBAAS_LEN];
	UINT4   HA_interfaceIp;
	UINT2	check_status;
	
}openstack_clbass_loadbalancer_t, *openstack_clbass_loadbalancer_p;

typedef struct openstack_clbass_backend
{
	UINT4  backendip;
	UINT2  check_status;
}openstack_clbass_backend_t, *openstack_clbass_backend_p;


openstack_lbaas_pools_p find_openstack_lbaas_pool_by_pool_id(char* pool_id);

openstack_clbaas_routersubnet_p find_openstack_clbaas_subnet_by_subnet_id(char* subnet_id);


openstack_clbaas_vipfloating_p find_openstack_clbaas_vipfloating_by_ip(UINT4 fixedIp, UINT4 floatingip);



openstack_clbaas_routersubnet_p update_openstack_clbaas_subnet_by_routerrest(
		char* router_id,
		char* subnet_id );

openstack_clbaas_vipfloating_p update_openstack_clbaas_vipfloatingpool_by_vipsrest(
		char* pool_id,
		char* router_id,
		char* port_id,
		UINT4 ipv4Addr,
		UINT1 is_publictype);

openstack_clbaas_vipfloating_p find_openstack_clbaas_vipfloatingpool_by_extip(UINT4 ext_ip);

UINT4 find_openstack_clbaas_vipfloatingpool_insideip_by_extip(UINT4 ext_ip);

void reset_openstack_clbaas_vipfloatingpoolflag();

void remove_openstack_clbaas_vipfloatingpool_unchecked();

void reset_openstack_clbaas_backendflag();
void remove_openstack_clbaas_backend_unchecked();
void remove_openstack_clbaas_backend_internalflows_byIp(UINT4 destip);
void remove_openstack_clbaas_backend_internalflows_byMac(gn_switch_t *sw, char *mac);





openstack_clbass_loadbalancer_p find_openstack_clbaas_loadbalancer_by_HAInterfaceIp(UINT4 HAInterfaceIp);
openstack_clbass_loadbalancer_p find_openstack_clbaas_loadbalancer_by_PoolidAndHAInterfaceIp(char* pool_id, UINT4 HAInterfaceIp);

void reset_openstack_clbaas_loadbalancerflag();

void remove_openstack_clbaas_loadbalancer_unchecked();

openstack_clbass_loadbalancer_p update_openstack_clbaas_loadbalancer_by_loadbalancerrest
	(
		char* loadbalancer_id,
		char* pool_id,
		char* tenant_id
	);
openstack_clbass_loadbalancer_p update_openstack_clbaas_loadbalancer_by_clbinterfacerest
	(
		char* loadbalancer_id,
		char* tenant_id,
		UINT4 HAInterfaceIp
	);


openstack_lbaas_pools_p update_openstack_clbaas_pool_by_clbpoolrest(
		char* tenant_id,
		char* pool_id,
		UINT4 conn_limit );
		

openstack_lbaas_pools_p update_openstack_clbaas_pool_by_clbviprest(
		char* tenant_id,
		char* pool_id,
		UINT4 ipaddress );

openstack_lbaas_pools_p update_openstack_clbaas_pool_by_clblistenrest(
		char* tenant_id,
		char* pool_id,
		UINT1 protocol,
		UINT4 protocol_port,
		UINT4 conn_limit,
		UINT1 lbaas_method,
		UINT1 session_persistence);

openstack_lbaas_members_p update_openstack_clbaas_member_by_backendrest(
		char* member_id,
		char* tenant_id,
		//char* pool_id,
		UINT4 protocol_port,
		UINT4 fixed_ip);

openstack_lbaas_members_p update_openstack_clbaas_member_by_backendbindrest(
		char* member_id,
		char* tenant_id,
		UINT4 weight);


openstack_lbaas_listener_p update_openstack_clbaas_listener_by_clblistenrest(
		char* listener_id,
		UINT1 type,
		UINT4 check_frequency,
		UINT4 overtime,
		UINT1 retries);


openstack_clbass_backend_p update_openstack_clbaas_backendip_by_backendrest
	(
		UINT4 backendip
	);



#endif


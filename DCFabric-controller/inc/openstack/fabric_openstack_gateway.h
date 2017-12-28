
/*
 * fabric_openstack_gateway.h
 *
 *  Created on: 6 20, 2017
 *      Author: yang
 */

#ifndef INC_FABRIC_OPENSTACK_GATEWAY_H_
#define INC_FABRIC_OPENSTACK_GATEWAY_H_

#define 	OPENSTACK_DEVICE_ID_LEN		48

#define     OPENSTACK_ROUTER_NUM					64
#define 	OPENSTACK_ROUTER_INTERFACE_NUM		OPENSTACK_ROUTER_NUM
#define		OPENSTATCK_ROUTER_GATEWAY_NUM		OPENSTACK_ROUTER_NUM*10
#define		OPENSTACK_ROUTER_NODE_NUM			OPENSTACK_ROUTER_INTERFACE_NUM+OPENSTATCK_ROUTER_GATEWAY_NUM


enum openstack_router_outerinterface_node_type
{
	ROUTER_OUTERINTERFACE_NODE_ROUTER = 0,
};

typedef struct _openstack_router_gateway  
{
	char	network_id[OPENSTACK_NETWORK_ID_LEN];
	char	subnet_id[OPENSTACK_SUBNET_ID_LEN];
	UINT4   interfaceip;
	UINT4   port_type;
	UINT1   check_status;
	struct _openstack_router_gateway *next;
	
}openstack_router_gateway_t, *openstack_router_gateway_p;

typedef struct _openstack_router_outerinterface
{
	char	device_id[OPENSTACK_DEVICE_ID_LEN];
	UINT1   check_status;
	
	void*   data;
	//struct _openstack_router_outerinterface *next;
}openstack_router_outerinterface_t, *openstack_router_outerinterface_p;


typedef struct _openstack_router_outerinterface_node
{
	UINT1* data;														
	struct _openstack_router_outerinterface_node* next;
}openstack_router_outerinterface_node_t,*openstack_router_outerinterface_node_p;

void init_openstack_router_gateway();

void destroy_openstack_router_gateway();

UINT4 find_openstack_router_outerinterfaceip_by_router(openstack_router_outerinterface_p node_router_outerinterface, UINT4 *outer_interface_ip);

openstack_router_outerinterface_p    find_openstack_router_outerinterface_by_deviceid(char* deviceid);

openstack_router_outerinterface_p    find_openstack_router_outerinterface_by_networkid(char* network_id);

openstack_router_outerinterface_p    find_openstack_router_outerinterface_by_subnetid(char* subnet_id);

openstack_router_outerinterface_p    find_openstack_router_outerinterface_by_networkAndsubnetid(char* network_id, char* subnet_id);

openstack_router_outerinterface_p    update_openstack_router_outerinterface(char* device_id, char* network_id, char* subnet_id, UINT4 port_ip, UINT4 port_type );

void   reset_openstack_router_outerinterface_checkflag();

void   remove_openstack_router_outerinterface_uncheck();
void   remove_openstack_router_outerinterface_from_olddeviceid(char* device_id,  UINT4 port_ip);


#endif



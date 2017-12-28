

#ifndef INC_OPENSTACK_ROUTERS_H_
#define INC_OPENSTACK_ROUTERS_H_
////////////////////////////////////////////////////////////////////////
#include <stdbool.h>
#include "gnflush-types.h"

#define OPENSTACK_IP_LEN 48

struct _openstack_router;
struct _openstack_port_forward;

extern struct _openstack_router *  g_openstack_router_list;

extern struct _openstack_port_forward *  g_openstack_forward_list;

typedef struct _openstack_router
{
	UINT1 status;
	char router_id[128];
	UINT4 external_fixed_ips;
    struct _openstack_router* next;
}openstack_router,* openstack_router_p;


typedef struct _openstack_port_forward{
    int  proto;                    //0:tcp 1:udp
    int  state;                    //0:disable 1:able
    char src_ip[OPENSTACK_IP_LEN]; //external
    char dst_ip[OPENSTACK_IP_LEN]; //internal
    UINT4 n_src_ip;                //external
    UINT4 n_dst_ip;                //internal
    UINT2 src_port_start;          //external
    UINT2 src_port_end;            //external
    UINT2 dst_port_start;          //internal
    UINT2 dst_port_end;            //internal
    char network_id[OPENSTACK_IP_LEN];

    struct _openstack_port_forward * next;
}openstack_port_forward,* openstack_port_forward_p;

typedef struct _port_forward_param
{
  openstack_port_forward_p list_header;
}port_forward_param, *port_forward_param_p;


typedef struct _port_proc_param
{
	openstack_port_forward_p new_list;

	openstack_port_forward_p old_list;

	openstack_port_forward_p copy_list;

}port_forward_proc, *port_forward_proc_p;

UINT1 visit_openstack_router_list_find_unvalid(void);
UINT1 reset_openstack_router_status(void);
//路由列表相关接口
openstack_router_p create_openstack_router(char* router_id,UINT4 external_fixed_ips);

void destory_openstack_router(openstack_router_p router_node);

bool update_openstack_router_list(openstack_router_p * header,  char* router_id, UINT4 fixed_ip, char* network_id);

openstack_router_p get_openstack_router_by_router_id(openstack_router_p header, char* router_id);


//端口转发相关接口

openstack_port_forward_p create_openstack_portfoward(int protol, int status, char* network_id,  char* src_ip, char* dst_ip, char *in_port, char* outside_port);

openstack_port_forward_p copy_openstack_portfoward(openstack_port_forward_p src_node);

bool update_opstack_portforward_list(openstack_port_forward_p* header, char* protol, char* status , char* network_id, char* src_ip, char* dst_ip, char *in_port, char* outside_port);

openstack_port_forward_p get_opstack_portforward_by_src_ip(openstack_port_forward_p header, char* src_ip, int src_port);

void destory_port_forward_list(openstack_port_forward_p header);

#endif




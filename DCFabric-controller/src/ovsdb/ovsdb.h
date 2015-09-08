/******************************************************************************
*                                                                             *
*   File Name   : ovsdb.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-20           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef OVSDB_H_
#define OVSDB_H_

#include "gnflush-types.h"

#define  NETWORK_TYPE_GRE           1
#define  NETWORK_TYPE_VXLAN         2

#define  OVSDB_SERVER_PORT          6640
#define  OVSDB_BUFF_LEN             20480
#define  OVSDB_MAX_CONNECTION       50

#define  SEARCH_ALL_TABLE_ID        "1"
#define  ADD_BRIDGE_ID              "2"
#define  SET_FAILMODE_OF_ID         "3"
#define  SET_CONTROLLER_ID          "4"
#define  ADD_PORT_ID                "5"
#define  ADD_INTERFACE_OPTION_ID    "6"

#define  INTERFACE_TYPE_GRE         "gre"
#define  INTERFACE_TYPE_VLAN        "vlan"
#define  INTERFACE_TYPE_VXLAN       "vxlan"
#define  INTERFACE_TYPE_INTERNAL    "internal"
#define  INTERFACE_TYPE_PATCH       "patch"

#define  TYPE_GRE                   "gre"
#define  TYPE_VLAN                  "vlan"
#define  TYPE_VXLAN                 "vxlan"

#define  INTERNAL_BR                "br-int"
#define  EXTERNAL_BR                "br-ex"

#define  FAIL_MODE_SECURE           "secure"

#define  OFP_10                     "OpenFlow10"
#define  OFP_13                     "OpenFlow13"

#define  NODE_TYPE_CONTROL          1
#define  NODE_TYPE_COMPUTE          2
#define  NEUTRON_BRIDGE_MAX_NUM     5


typedef struct openvswitch
{
    char _uuid[64];
}openvswitch_t;

typedef struct ovs_bridge
{
    BOOL is_using;
    char _uuid[63];
    UINT8 dpid;
    char name[15];
    UINT1 of_proto;
}ovs_bridge_t;

typedef struct ovsdb_server
{
//    UINT4 node_type;  //���ƽڵ㻹�Ǽ���ڵ�
    UINT4 resv;
    UINT4 node_ip;
    int node_fd;
    openvswitch_t open_vswitch;
    ovs_bridge_t bridge[NEUTRON_BRIDGE_MAX_NUM];
}ovsdb_server_t;

pthread_t g_ovsdb_recv_tid;

UINT1 g_tunnel_type;        //openstack�������� gre/vxlan
UINT1 g_ovsdb_of_version;   //openstack ovs��openflow�汾
UINT4 g_ovsdb_port;         //ovsdb�ӿڣ�Ĭ�϶˿�6640

extern ovsdb_server_t g_ovsdb_nodes[];

INT4 init_ovsdb();

#endif /* OVSDB_H_ */

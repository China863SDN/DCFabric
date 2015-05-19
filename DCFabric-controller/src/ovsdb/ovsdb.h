/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

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
#define  NEUTRON_BRIDGE_MAX_NUM     3


typedef struct openvswitch
{
    char _uuid[64];
}openvswitch_t;

typedef struct ovs_bridge
{
    BOOL is_using;
    char _uuid[63];
    char dpid[32];
    char name[15];
    UINT1 of_proto;
}ovs_bridge_t;

typedef struct ovsdb_server
{
    UINT4 node_type;  //控制节点还是计算节点
    UINT4 resv;
    UINT4 node_ip;
    int node_fd;
    openvswitch_t open_vswitch;
    ovs_bridge_t bridge[NEUTRON_BRIDGE_MAX_NUM];
}ovsdb_server_t;


pthread_t g_ovsdb_recv_tid;

UINT1 g_tunnel_type;        //openstack网络类型 gre/vxlan
UINT1 g_ovsdb_of_version;   //openstack ovs的openflow版本
UINT4 g_ovsdb_port;         //ovsdb接口，默认端口6640


INT4 init_ovsdb();

#endif /* OVSDB_H_ */

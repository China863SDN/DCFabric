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
*   File Name   : hbase_client.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-20           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef HBASE_CLIENT_H_
#define HBASE_CLIENT_H_

#include "common.h"

#define TABLE_STRING_LEN 64
#define TOPO_VER "topo_version"
#define MASTER_ID "master_id"
#define CUSTOM_MASTER_ID "custom_master_id"
#define ELECTION_GENERATION_ID "election_generation_id"
#define CLUSTER_ONOFF "cluster_onoff"
#define TRANS_SEQ "trans_seq"

//topology
#define TOPO_TABLE "T_TOPOLOGY"
#define TOPO_TABLE_CF "CF_TOPO"

//common
#define COMMON_TABLE "T_COMMON"
#define COMMON_TABLE_CF "CF_COMM"

//flow
#define DYNAMIC_FLOW_TABLE "T_FLOW"
#define DYNAMIC_FLOW_TABLE_CF "CF_FLOW"

//controller info
#define CONTROLLER_INFO_TABLE "T_CONTROLLER_INFO"
#define CONTROLLER_INFO_TABLE_CF "CF_CONTROLLER_INFO"

//controller data
#define CONTROLLER_DATA_TABLE "T_CONTROLLER_DATA"
#define CONTROLLER_DATA_TABLE_CF "CF_CONTROLLER_DATA"

#define MAX_PAD_LEN  64
extern UINT4 MAX_FILED_NUM;
extern UINT4 TABLE_DATA_LEN;

//node type
enum NODE_TYPE
{
	FABRIC_HOST_NODE         = 0x01,
	FABRIC_SW_NODE 	         = 0x02,
	
	NAT_ICMP			     = 0x03,
	NAT_HOST			     = 0x04,
	
	OPENSTACK_EXTERNAL_NODE  = 0x05,
	OPENSTACK_LBAAS_MEMBERS  = 0x06
};

//operation type (add|delete)
enum OPERATE_TYPE
{
	OPERATE_ADD = 0x01,   // Ìí¼Ó	
	OPERATE_DEL = 0x02,   // É¾³ý
	OPERATE_MOD = 0x03    // ÐÞ¸Ä
};

//sw list filed type
enum SW_LIST_FILED_TYPE 
{
	SW_LIST_OWN_DPID  = 0x01,
	SW_LIST_OWN_TAG  = 0x02,
	SW_LIST_OWN_PREDPID = 0x03,
	SW_LIST_OWN_PORT  = 0x04,
	SW_LIST_NEIGHBOR_DPID  = 0x05,
	SW_LIST_NEIGHBOR_PORT  = 0x06
};

//host list filed type
enum HOST_LIST_FILED_TYPE 
{
    HOST_LIST_HOST_IP = 0x11,
	HOST_LIST_SW_DPID  = 0x12,
	HOST_LIST_SW_PORT  = 0x13
};


//openstack external list filed type
enum EXTERNAL_FILED_TYPE 
{
    EXTERNAL_NETWORK_ID = 0x21,
    EXTERNAL_GATEWAY_IP = 0x22,
    EXTERNAL_OUTER_INTERFACE_IP = 0x23,
    EXTERNAL_GATEWAY_MAC = 0x24,
    EXTERNAL_OUTER_INTERFACE_MAC = 0x25,
    EXTERNAL_DPID = 0x26,
    EXTERNAL_PORT = 0x27
};


//nat icmp iden filed type
enum NAT_ICMP_FILED_TYPE 
{
    NAT_ICMP_IDENTIFIER = 0x31,
    NAT_ICMP_HOST_IP = 0x32,
    NAT_ICMP_MAC = 0x33,
    NAT_ICMP_SW_DPID = 0x34,
    NAT_ICMP_INPORT = 0x35
};

//nat host filed type
enum NAT_HOST_FILED_TYPE 
{
    NAT_HOST_IP = 0x41,
    NAT_HOST_TCP_PORT = 0x42,
    NAT_HOST_UDP_PORT = 0x43,
    NAT_HOST_EXTERNAL_PORTNO = 0x44,
    NAT_HOST_INTERNAL_IP = 0x45,
    NAT_HOST_INTERNAL_PORTNO = 0x46,
    NAT_HOST_INTERNAL_MAC = 0x47,
    NAT_HOST_GATEWAY_DPID = 0x48,
    NAT_HOST_SRC_DPID = 0x49
};

//openstack lbaas mems filed type
enum LBAAS_MEMBERS_FILED_TYPE 
{
    LBAAS_MEMBERS_EXT_ID = 0x51,
    LBAAS_MEMBERS_INSIDE_IP = 0x52,
    LBAAS_MEMBERS_VIP = 0x53,
    LBAAS_MEMBERS_SRC_PORTNO = 0x54,
    LBAAS_MEMBERS_EXT_PORTNO = 0x55
};


typedef struct field_pad
{
	UINT1 type;
	UINT1 len;
	INT1  pad[MAX_PAD_LEN];
}field_pad_t;

extern field_pad_t* g_filed_pad_master;
extern field_pad_t* g_filed_pad_slave;
extern UINT4 g_filed_num_master ;
extern UINT4 g_filed_num_slave;
extern INT1 g_hbase_ip[];
extern INT1 g_hbase_port[];

void hbase_client_add_record(INT1 *table_name, INT1 *cloumn_family, INT1 *row, INT4 num, INT1 *column[], INT1 *value[]);
void persist_topology();
void query_topology();
void delete_record();
void persist_value(const INT1* key, const INT1* value);
void query_value(const INT1* key, INT1* value);
void persist_controller(UINT4 ip, UINT2 port, INT1* role);
void deletet_controller(INT1* key);
void query_controller_all(INT1* value);
void query_controller_port(INT1* key, INT1* value);
void query_controller_role(INT1* key, INT1* value);
void reset_controller_role(INT1* key, INT1* value);
INT4 IsInDB(INT1* key);
void persist_data(UINT4 node_type, UINT4 operate_type, INT4 num, field_pad_t* field_pad_p);
void query_data();
INT4 init_hbase_client();
void fini_hbase_client();
INT4 init_sync_mgr();
void fini_sync_mgr();
#endif /* HBASE_CLIENT_H_ */

/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2016, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the BNC SDN Controller. BNC SDN
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
*   File Name   : CSyncDefine.h                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CSYNCDEFINE_H
#define __CSYNCDEFINE_H

#include "bnc-type.h"

#define MAX_FILED_NUM               204800
#define MAX_PAD_LEN                 64
#define TABLE_STRING_LEN            64

//key
#define TOPO_VER                    "topo_version"
#define MASTER_ID                   "master_id"
#define CUSTOM_MASTER_ID            "custom_master_id"
#define ELECTION_GENERATION_ID      "election_generation_id"
#define CLUSTER_ONOFF               "cluster_onoff"
#define TRANS_SEQ                   "trans_seq"

//topology
#define TOPO_TABLE                  "T_TOPOLOGY"
#define TOPO_TABLE_CF               "CF_TOPO"

//common
#define COMMON_TABLE                "T_COMMON"
#define COMMON_TABLE_CF             "CF_COMM"

//flow
#define DYNAMIC_FLOW_TABLE          "T_FLOW"
#define DYNAMIC_FLOW_TABLE_CF       "CF_FLOW"

//controller info
#define CONTROLLER_INFO_TABLE       "T_CONTROLLER_INFO"
#define CONTROLLER_INFO_TABLE_CF    "CF_CONTROLLER_INFO"

//controller data
#define CONTROLLER_DATA_TABLE       "T_CONTROLLER_DATA"
#define CONTROLLER_DATA_TABLE_CF    "CF_CONTROLLER_DATA"

//node sync type
enum NODE_SYNC_TYPE
{
	DATASYNC_TIMER = 0x01,
	DATASYNC_TRIGGER = 0x02
};

//node type
enum NODE_TYPE
{
	NODE_TYPE_NONE          = 0x00,
	FABRIC_HOST_NODE        = 0x01,
	FABRIC_SW_NODE 	        = 0x02,
	
	NAT_ICMP                = 0x03,
	NAT_HOST                = 0x04,
	
	OPENSTACK_EXTERNAL_NODE = 0x05,
	OPENSTACK_LBAAS_MEMBERS = 0x06,

	NODE_TYPE_TOPO_LINK     = 0x07,
	NODE_TYPE_ALL_TOPO_LINK = 0x08,

	NODE_TYPE_SW_TAG        = 0x09,
	NODE_TYPE_ALL_SW_TAG    = 0x0a,
	NODE_TYPE_TAG_INFO      = 0x0b,

    NODE_TYPE_MAX
};

extern const INT1* nodeTypeString[NODE_TYPE_MAX];

//operation type (add|delete|modify)
enum OPERATE_TYPE
{
	OPERATE_ADD = 0x01,
	OPERATE_DEL = 0x02,
	OPERATE_MOD = 0x03
};

//sw list filed type
enum SW_LIST_FILED_TYPE 
{
	SW_LIST_OWN_DPID       = 0x01,
	SW_LIST_OWN_TAG        = 0x02,
	SW_LIST_OWN_PREDPID    = 0x03,
	SW_LIST_OWN_PORT       = 0x04,
	SW_LIST_NEIGHBOR_DPID  = 0x05,
	SW_LIST_NEIGHBOR_PORT  = 0x06
};

//host list filed type
enum HOST_LIST_FILED_TYPE 
{
    HOST_LIST_HOST_IP  = 0x11,
	HOST_LIST_SW_DPID  = 0x12,
	HOST_LIST_SW_PORT  = 0x13
};

//openstack external list filed type
enum EXTERNAL_FILED_TYPE 
{
    EXTERNAL_NETWORK_ID          = 0x20,
	EXTERNAL_SUBNETWORK_ID       = 0X21,	
    EXTERNAL_GATEWAY_IP          = 0x22,
    EXTERNAL_OUTER_INTERFACE_IP  = 0x23,
    EXTERNAL_GATEWAY_MAC         = 0x24,
    EXTERNAL_OUTER_INTERFACE_MAC = 0x25,
    EXTERNAL_DPID                = 0x26,
    EXTERNAL_PORT                = 0x27
};

//nat icmp iden filed type
enum NAT_ICMP_FILED_TYPE 
{
    NAT_ICMP_IDENTIFIER = 0x31,
    NAT_ICMP_HOST_IP    = 0x32,
    NAT_ICMP_MAC        = 0x33,
    NAT_ICMP_SW_DPID    = 0x34,
    NAT_ICMP_INPORT     = 0x35
};

//nat host filed type
enum NAT_HOST_FILED_TYPE 
{
    NAT_HOST_IP              = 0x41,
    NAT_HOST_TCP_PORT        = 0x42,
    NAT_HOST_UDP_PORT        = 0x43,
    NAT_HOST_EXTERNAL_PORTNO = 0x44,
    NAT_HOST_INTERNAL_IP     = 0x45,
    NAT_HOST_INTERNAL_PORTNO = 0x46,
    NAT_HOST_INTERNAL_MAC    = 0x47,
    NAT_HOST_GATEWAY_DPID    = 0x48,
    NAT_HOST_SRC_DPID        = 0x49
};

//openstack lbaas mems filed type
enum LBAAS_MEMBERS_FILED_TYPE 
{
    LBAAS_MEMBERS_EXT_ID     = 0x51,
    LBAAS_MEMBERS_INSIDE_IP  = 0x52,
    LBAAS_MEMBERS_VIP        = 0x53,
    LBAAS_MEMBERS_SRC_PORTNO = 0x54,
    LBAAS_MEMBERS_EXT_PORTNO = 0x55
};

//topo links filed type
enum TOPO_LINKS_FILED_TYPE 
{
    TOPO_LINKS_STATE    = 0x61,
    TOPO_LINKS_SRC_DPID = 0x62,
    TOPO_LINKS_SRC_PORT = 0x63,
    TOPO_LINKS_DST_DPID = 0x64,
    TOPO_LINKS_DST_PORT = 0x65,
    TOPO_LINKS_WEIGHT   = 0x66
};

//sw tag filed type
enum SW_TAG_FILED_TYPE 
{
    SW_TAG_DPID = 0x71,
    SW_TAG_TAG  = 0x72
};

//tag info filed type
enum TAG_INFO_FILED_TYPE 
{
    TAG_INFO_VALUE = 0x81
};

typedef struct field_pad
{
	UINT1 type;
	UINT1 len;
	UINT1 rev[6];
	INT1  pad[MAX_PAD_LEN];
}field_pad_t;

typedef struct
{
	UINT4 msgLen;
	UINT4 msgNodeType ;
}stCommMsgNode_header, *pstCommMsgNode_header; 

typedef struct
{
	UINT4 synMsgType;
	UINT4 operMsgType;
	UINT2 rev;
	UINT4 synMsgLen;
}stSynMsgNode_header, *pstSynMsgNode_header; 

typedef struct
{
	UINT4 ip;
	UINT2 port;
	INT4  role;
}stControllerInfo;

#endif

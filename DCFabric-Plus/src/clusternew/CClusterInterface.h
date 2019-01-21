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
*   File Name   : CClusterInterface.h                                         *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#ifndef __CCLUSTERINTERFACE_H
#define __CCLUSTERINTERFACE_H

#include "bnc-type.h"

typedef enum
{
    CLUSTER_OPER_NONE                                   = 0x00,

    //connection
    CLUSTER_OPER_KEEPALIVE_REQ                          = 0x10,
    CLUSTER_OPER_KEEPALIVE_RSP                          = 0x11,

    //election
    CLUSTER_OPER_HELLO_REQ                              = 0x20,
    CLUSTER_OPER_HELLO_RSP                              = 0x21,
    CLUSTER_OPER_ELECTION_PROPOSAL                      = 0x22,
    CLUSTER_OPER_ELECTION_NOTIFY                        = 0x23,
    CLUSTER_OPER_ELECTION_INFORM                        = 0x24,

    //synchronization
    CLUSTER_OPER_SYNC_SW_LIST_REQ                       = 0x30,
    CLUSTER_OPER_SYNC_SW_LIST_RSP                       = 0x31,

    CLUSTER_OPER_SYNC_HOST_LIST_REQ                     = 0x40,
    CLUSTER_OPER_SYNC_HOST_LIST_RSP                     = 0x41,

    CLUSTER_OPER_SYNC_OPENSTACK_EXT_LIST_REQ            = 0x50,
    CLUSTER_OPER_SYNC_OPENSTACK_EXT_LIST_RSP            = 0x51,

    CLUSTER_OPER_SYNC_NAT_ICMP_LIST_REQ                 = 0x60,
    CLUSTER_OPER_SYNC_NAT_ICMP_LIST_RSP                 = 0x61,

    CLUSTER_OPER_SYNC_NAT_HOST_LIST_REQ                 = 0x70,
    CLUSTER_OPER_SYNC_NAT_HOST_LIST_RSP                 = 0x71,
    CLUSTER_OPER_SYNC_ADD_NAT_HOST_REQ                  = 0x72,
    CLUSTER_OPER_SYNC_ADD_NAT_HOST_RSP                  = 0x73,
    CLUSTER_OPER_SYNC_DEL_NAT_HOST_REQ                  = 0x74,
    CLUSTER_OPER_SYNC_DEL_NAT_HOST_RSP                  = 0x75,

    CLUSTER_OPER_SYNC_OPENSTACK_LBAAS_MEMBERS_LIST_REQ  = 0x80,
    CLUSTER_OPER_SYNC_OPENSTACK_LBAAS_MEMBERS_LIST_RSP  = 0x81,
    CLUSTER_OPER_SYNC_ADD_OPENSTACK_LBAAS_MEMBERS_REQ   = 0x82,
    CLUSTER_OPER_SYNC_ADD_OPENSTACK_LBAAS_MEMBERS_RSP   = 0x83,
    CLUSTER_OPER_SYNC_DEL_OPENSTACK_LBAAS_MEMBERS_REQ   = 0x84,
    CLUSTER_OPER_SYNC_DEL_OPENSTACK_LBAAS_MEMBERS_RSP   = 0x85,

    CLUSTER_OPER_SYNC_TOPO_LINK_LIST_REQ                = 0x90,
    CLUSTER_OPER_SYNC_TOPO_LINK_LIST_RSP                = 0x91,
    CLUSTER_OPER_SYNC_TOPO_LINK_REQ                     = 0x92,
    CLUSTER_OPER_SYNC_TOPO_LINK_RSP                     = 0x93,

    CLUSTER_OPER_SYNC_SW_TAG_LIST_REQ                   = 0xA0,
    CLUSTER_OPER_SYNC_SW_TAG_LIST_RSP                   = 0xA1,
    CLUSTER_OPER_SYNC_SW_TAG_REQ                        = 0xA2,
    CLUSTER_OPER_SYNC_SW_TAG_RSP                        = 0xA3,

    CLUSTER_OPER_SYNC_TAG_INFO_REQ                      = 0xB0,
    CLUSTER_OPER_SYNC_TAG_INFO_RSP                      = 0xB1,

    CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_REQ               = 0xC0,
    CLUSTER_OPER_SYNC_FLOW_ENTRY_LIST_RSP               = 0xC1,
    CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_REQ                = 0xC2,
    CLUSTER_OPER_SYNC_ADD_FLOW_ENTRY_RSP                = 0xC3,
    CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_REQ                = 0xC4,
    CLUSTER_OPER_SYNC_DEL_FLOW_ENTRY_RSP                = 0xC5,

    //...
    CLUSTER_OPER_MAX                                    = 0xFF
}cluster_operation_e;

typedef enum
{
    CLUSTER_CAUSE_SUCC                                      = 0x00,
    CLUSTER_CAUSE_FAIL                                      = 0x01,

    //synchronization
    CLUSTER_CAUSE_RECOVER_SW_LIST_FAIL                      = 0x30,
    CLUSTER_CAUSE_RECOVER_SW_CRT_FAIL                       = 0x31,
    CLUSTER_CAUSE_RECOVER_SW_UPD_FAIL                       = 0x32,
    CLUSTER_CAUSE_RECOVER_SW_DEL_FAIL                       = 0x33,

    CLUSTER_CAUSE_RECOVER_HOST_LIST_FAIL                    = 0x40,

    CLUSTER_CAUSE_RECOVER_OPENSTACK_EXT_LIST_FAIL           = 0x50,

    CLUSTER_CAUSE_RECOVER_NAT_ICMP_LIST_FAIL                = 0x60,

    CLUSTER_CAUSE_RECOVER_NAT_HOST_LIST_FAIL                = 0x70,
    CLUSTER_CAUSE_RECOVER_ADD_NAT_HOST_FAIL                 = 0x71,
    CLUSTER_CAUSE_RECOVER_DEL_NAT_HOST_FAIL                 = 0x72,

    CLUSTER_CAUSE_RECOVER_OPENSTACK_LBAAS_MEMBERS_LIST_FAIL = 0x80,
    CLUSTER_CAUSE_RECOVER_ADD_OPENSTACK_LBAAS_MEMBERS_FAIL  = 0x81,
    CLUSTER_CAUSE_RECOVER_DEL_OPENSTACK_LBAAS_MEMBERS_FAIL  = 0x82,

    CLUSTER_CAUSE_RECOVER_TOPO_LINK_LIST_FAIL               = 0x90,
    CLUSTER_CAUSE_RECOVER_TOPO_LINK_FAIL                    = 0x91,

    CLUSTER_CAUSE_RECOVER_SW_TAG_LIST_FAIL                  = 0xA0,
    CLUSTER_CAUSE_RECOVER_SW_TAG_FAIL                       = 0xA1,

    CLUSTER_CAUSE_RECOVER_TAG_INFO_FAIL                     = 0xB0,

    CLUSTER_CAUSE_RECOVER_FLOW_ENTRY_LIST_FAIL              = 0xC0,
    CLUSTER_CAUSE_RECOVER_ADD_FLOW_ENTRY_FAIL               = 0xC1,
    CLUSTER_CAUSE_RECOVER_DEL_FLOW_ENTRY_FAIL               = 0xC2,

    //...
    CLUSTER_CAUSE_MAX                                       = 0xFF
}cluster_cause_e;

typedef enum
{
    CLUSTER_STATE_INIT     = 0x00,
    CLUSTER_STATE_ELECTING = 0x01,
    CLUSTER_STATE_WORKING  = 0x02,

    //...
    CLUSTER_STATE_MAX      = 0xFF
}cluster_state_e;

typedef struct
{
   INT4 operation; //@cluster_operation_e
   INT4 length; //data length
   INT1 data[0];
   //...
}cluster_interface_t;

typedef struct
{
    INT4  state; //@cluster_state_e
    UINT4 master;
    //...
}cluster_hello_req_t;
typedef cluster_hello_req_t cluster_hello_rsp_t;

typedef struct
{
    UINT4 master;
    //...
}cluster_keepalive_req_t;
typedef struct
{
    INT4  cause; //@cluster_cause_e
    UINT4 master;
    //...
}cluster_keepalive_rsp_t;

typedef struct
{
    UINT4 ip;
    //...
}cluster_election_proposal_t;
typedef cluster_keepalive_req_t cluster_election_notify_t;
typedef cluster_keepalive_req_t cluster_election_inform_t;

//host field type
enum HOST_FIELD_TYPE 
{
    HOST_TYPE      = 0x0401,
    HOST_MAC       = 0x0402,
    HOST_IP        = 0x0403,
    HOST_IPV6      = 0x0404,
	HOST_SW_DPID   = 0x0405,
	HOST_SW_PORT   = 0x0406,
	HOST_SUBNET_ID = 0x0407,
	HOST_TENANT_ID = 0x0408,
    HOST_FIXED_IP  = 0x0409
};

//nat icmp field type
enum NAT_ICMP_FIELD_TYPE 
{
    NAT_ICMP_IDENTIFIER = 0x0601,
    NAT_ICMP_HOST_IP    = 0x0602,
    NAT_ICMP_MAC        = 0x0603,
    NAT_ICMP_SW_DPID    = 0x0604,
    NAT_ICMP_INPORT     = 0x0605
};

//nat host field type
enum NAT_HOST_FIELD_TYPE 
{
    NAT_HOST_PROTOCOL        = 0x0701,
    NAT_HOST_EXTERNAL_IP     = 0x0702,
    NAT_HOST_SNAT_IP         = 0x0703,
    NAT_HOST_SNAT_PORT       = 0x0704,
    NAT_HOST_INTERNAL_IP     = 0x0705,
    NAT_HOST_INTERNAL_PORT   = 0x0706,
    NAT_HOST_INTERNAL_MAC    = 0x0707,
    NAT_HOST_GATEWAY_DPID    = 0x0708,
    NAT_HOST_SWITCH_DPID     = 0x0709
};

//topo link field type
enum TOPO_LINK_FIELD_TYPE 
{
    TOPO_LINK_STATE    = 0x0901,
    TOPO_LINK_SRC_DPID = 0x0902,
    TOPO_LINK_SRC_PORT = 0x0903,
    TOPO_LINK_DST_DPID = 0x0904,
    TOPO_LINK_DST_PORT = 0x0905,
    TOPO_LINK_WEIGHT   = 0x0906
};

//sw tag field type
enum SW_TAG_FIELD_TYPE 
{
    SW_TAG_DPID = 0x0A01,
    SW_TAG_TAG  = 0x0A02
};

//tag info field type
enum TAG_INFO_FIELD_TYPE 
{
    TAG_INFO_VALUE = 0x0B01
};

//flow entry field type
enum FLOW_ENTRY_FIELD_TYPE 
{
	FLOW_ENTRY_SW_DPID                  = 0x0C01,
	FLOW_ENTRY_UUID                     = 0x0C02,
	FLOW_ENTRY_CREATE_TIME              = 0x0C03,
	FLOW_ENTRY_TABLE_ID                 = 0x0C04,
	FLOW_ENTRY_IDLE_TIMEOUT             = 0x0C05,
	FLOW_ENTRY_HARD_TIMEOUT             = 0x0C06,
	FLOW_ENTRY_PRIORITY                 = 0x0C07,
	FLOW_ENTRY_MATCH                    = 0x0C08,
	FLOW_ENTRY_INSTRUCTIONS             = 0x0C09,

    //oxm fields
	FLOW_ENTRY_OXM_INPORT               = 0x0C10,
	FLOW_ENTRY_OXM_INPHYPORT            = 0x0C11,
	FLOW_ENTRY_OXM_METADATA             = 0x0C12,
	FLOW_ENTRY_OXM_ETHDST               = 0x0C13,
	FLOW_ENTRY_OXM_ETHSRC               = 0x0C14,
	FLOW_ENTRY_OXM_ETHTYPE              = 0x0C15,
	FLOW_ENTRY_OXM_VLANID               = 0x0C16,
	FLOW_ENTRY_OXM_VLANPCP              = 0x0C17,
	FLOW_ENTRY_OXM_IPDSCP               = 0x0C18,
	FLOW_ENTRY_OXM_IPECN                = 0x0C19,
	FLOW_ENTRY_OXM_IPPROTO              = 0x0C1a,
	FLOW_ENTRY_OXM_IPV4SRC              = 0x0C1b,
	FLOW_ENTRY_OXM_IPV4DST              = 0x0C1c,
	FLOW_ENTRY_OXM_TCPSRC               = 0x0C1d,
	FLOW_ENTRY_OXM_TCPDST               = 0x0C1e,
	FLOW_ENTRY_OXM_UDPSRC               = 0x0C1f,
	FLOW_ENTRY_OXM_UDPDST               = 0x0C20,
	FLOW_ENTRY_OXM_ICMPV4TYPE           = 0x0C21,
	FLOW_ENTRY_OXM_ICMPV4CODE           = 0x0C22,
	FLOW_ENTRY_OXM_ARPOP                = 0x0C23,
	FLOW_ENTRY_OXM_ARPSPA               = 0x0C24,
	FLOW_ENTRY_OXM_ARPTPA               = 0x0C25,
	FLOW_ENTRY_OXM_ARPSHA               = 0x0C26,
	FLOW_ENTRY_OXM_ARPTHA               = 0x0C27,
	FLOW_ENTRY_OXM_IPV6SRC              = 0x0C28,
	FLOW_ENTRY_OXM_IPV6DST              = 0x0C29,
	FLOW_ENTRY_OXM_MPLSLABEL            = 0x0C2a,
	FLOW_ENTRY_OXM_TUNNELID             = 0x0C2b,
	FLOW_ENTRY_OXM_IPV4SRCPREFIX        = 0x0C2c,
	FLOW_ENTRY_OXM_IPV4DSTPREFIX        = 0x0C2d,
	FLOW_ENTRY_OXM_IPV6SRCPREFIX        = 0x0C2e,
	FLOW_ENTRY_OXM_IPV6DSTPREFIX        = 0x0C2f,
	FLOW_ENTRY_OXM_METADATAMASK         = 0x0C30,
	FLOW_ENTRY_OXM_MASK                 = 0x0C31,

    //instructions
	FLOW_ENTRY_INSTRUCION_GOTOTABLE     = 0x0C40,
	FLOW_ENTRY_INSTRUCION_WRITEMETADATA = 0x0C41,
	FLOW_ENTRY_INSTRUCION_WRITEACTIONS  = 0x0C42,
	FLOW_ENTRY_INSTRUCION_APPLYACTIONS  = 0x0C43,
	FLOW_ENTRY_INSTRUCION_CLEARACTIONS  = 0x0C44,
	FLOW_ENTRY_INSTRUCION_METER         = 0x0C45,
	FLOW_ENTRY_INSTRUCION_EXPERIMENTER  = 0x0C46,

    //actions
	FLOW_ENTRY_ACION_OUTPUT             = 0x0C50,
	FLOW_ENTRY_ACION_MPLSTTL            = 0x0C51,
	FLOW_ENTRY_ACION_PUSHVLAN           = 0x0C52,
	FLOW_ENTRY_ACION_POPVLAN            = 0x0C53,
	FLOW_ENTRY_ACION_PUSHMPLS           = 0x0C54,
	FLOW_ENTRY_ACION_POPMPLS            = 0x0C55,
	FLOW_ENTRY_ACION_SETQUEUE           = 0x0C56,
	FLOW_ENTRY_ACION_GROUP              = 0x0C57,
	FLOW_ENTRY_ACION_SETNWTTL           = 0x0C58,
	FLOW_ENTRY_ACION_SETFIELD           = 0x0C59,
	FLOW_ENTRY_ACION_PUSHPBB            = 0x0C5a,
	FLOW_ENTRY_ACION_POPPBB             = 0x0C5b,
	FLOW_ENTRY_ACION_EXPERIMENTER       = 0x0C5c
};

typedef struct
{
    INT4 len; //following data length
    INT1 data[0];
    //...
}cluster_sync_req_t;
typedef struct
{
    INT4 cause; //@cluster_cause_e
    //...
}cluster_sync_rsp_t;

#endif /* __CCLUSTERINTERFACE_H */

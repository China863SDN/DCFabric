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
*   File Name   : bnc-param.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef __BNC_PARAM_H
#define __BNC_PARAM_H
#include "bnc-type.h"
#include "CSmartPtr.h"

#define MAX_PORTS       65535//150
#define MAX_QUEUE_ID    1024//40960
#define UUID_LEN        40

class CSwitch;

enum OPENSTACK_CHECK_STATUS {
	CHECK_UNKNOWN = 0,
	CHECK_CREATE = 1, 
	CHECK_UPDATE = 2,
	CHECK_LATEST = 3,
	CHECK_UNCHECKED = 4,
};

enum OF_MSG_STATE
{
    OF_MSG_STATE_NEW,        //新消�?
    OF_MSG_STATE_OLD         //旧消息，已被处理�?
};


enum OF_HANDLER_TYPE
{
    OF_HANDLER_PACKETIN_IP_TCP_DSTIP,
    OF_HANDLER_PACKETIN_IP_UDP_DSTIP,
    OF_HANDLER_PACKETIN_IP_ICMP,
    OF_HANDLER_PACKETIN_ARP_DSTIP,
    OF_HANDLER_PACKETIN_LLDP,
    OF_HANDLER_PACKETIN_IPV6,
    OF_HANDLER_PACKETIN_VLAN,
    OF_HANDLER_DEFAULT,

    OF_HANDLER_INVALID
};


enum OF_HANDLER_DEFAULT_TYPE
{
    //openflow 10
    OF_HANDLER_DEFAULT_HELLO,
    OF_HANDLER_DEFAULT_ERROR,
    OF_HANDLER_DEFAULT_ECHO_REQUEST,
    OF_HANDLER_DEFAULT_ECHO_REPLY,
    OF_HANDLER_DEFAULT_VENDOR,
    OF_HANDLER_DEFAULT_FEATURES_REQUEST,
    OF_HANDLER_DEFAULT_FEATURES_REPLY,
    OF_HANDLER_DEFAULT_GET_CONFIG_REQUEST,
    OF_HANDLER_DEFAULT_GET_CONFIG_REPLY,
    OF_HANDLER_DEFAULT_SET_CONFIG,
    OF_HANDLER_DEFAULT_FLOW_REMOVED,
    OF_HANDLER_DEFAULT_PORT_STATUS,
    OF_HANDLER_DEFAULT_PACKET_OUT,
    OF_HANDLER_DEFAULT_FLOW_MOD,
    OF_HANDLER_DEFAULT_PORT_MOD,
    OF_HANDLER_DEFAULT_STATS_REQUEST,
    OF_HANDLER_DEFAULT_STATS_REPLY,
    OF_HANDLER_DEFAULT_BARRIER_REQUEST,
    OF_HANDLER_DEFAULT_BARRIER_REPLY,
    OF_HANDLER_DEFAULT_QUEUE_GET_CONFIG_REQUEST,
    OF_HANDLER_DEFAULT_QUEUE_GET_CONFIG_REPLY,

    //openflow 13
    OF_HANDLER_DEFAULT13_HELLO,
    OF_HANDLER_DEFAULT13_ERROR,
    OF_HANDLER_DEFAULT13_ECHO_REQUEST,
    OF_HANDLER_DEFAULT13_ECHO_REPLY,
    OF_HANDLER_DEFAULT13_EXPERIMENTER,
    OF_HANDLER_DEFAULT13_FEATURES_REQUEST,
    OF_HANDLER_DEFAULT13_FEATURES_REPLY,
    OF_HANDLER_DEFAULT13_GET_CONFIG_REQUEST,
    OF_HANDLER_DEFAULT13_GET_CONFIG_REPLY,
    OF_HANDLER_DEFAULT13_SET_CONFIG,
    OF_HANDLER_DEFAULT13_FLOW_REMOVED,
    OF_HANDLER_DEFAULT13_PORT_STATUS,
    OF_HANDLER_DEFAULT13_PACKET_OUT,
    OF_HANDLER_DEFAULT13_FLOW_MOD,
    OF_HANDLER_DEFAULT13_GROUP_MOD,
    OF_HANDLER_DEFAULT13_PORT_MOD,
    OF_HANDLER_DEFAULT13_TABLE_MOD,
    OF_HANDLER_DEFAULT13_MULTIPART_REQUEST,
    OF_HANDLER_DEFAULT13_MULTIPART_REPLY,
    OF_HANDLER_DEFAULT13_BARRIER_REQUEST,
    OF_HANDLER_DEFAULT13_BARRIER_REPLY,
    OF_HANDLER_DEFAULT13_QUEUE_GET_CONFIG_REQUEST,
    OF_HANDLER_DEFAULT13_QUEUE_GET_CONFIG_REPLY,
    OF_HANDLER_DEFAULT13_ROLE_REQUEST,
    OF_HANDLER_DEFAULT13_ROLE_REPLY,
    OF_HANDLER_DEFAULT13_GET_ASYNC_REQUEST,
    OF_HANDLER_DEFAULT13_GET_ASYNC_REPLY,
    OF_HANDLER_DEFAULT13_SET_ASYNC,
    OF_HANDLER_DEFAULT13_METER_MOD
};


typedef struct packet_in_info
{
    UINT4 xid;
    UINT4 buffer_id;
    UINT4 inport;
    UINT4 data_len;
	UINT1 In_TableID;
    UINT1 *data;
} packet_in_info_t;


typedef struct switch_desc {
   INT1 mfr_desc[256];       /* Manufacturer description. */
   INT1 hw_desc[256];        /* Hardware description. */
   INT1 sw_desc[256];        /* Software description. */
   INT1 serial_num[32];      /* Serial number. */
   INT1 dp_desc[256];        /* Human readable description of datapath. */
} switch_desc_t;

//负载状�?
typedef enum
{
    LOAD_IDLE = 0,  // 0% - 25%
    LOAD_LIGHT = 1, // 25% - 50
    LOAD_BUSY = 2,  // 50% - 75%
    LOAD_HEAVY = 3, // 75% - 100%
    LOAD_FULL = 4   // 100%
}LOAD_STATUS;

typedef struct port_stats
{
    LOAD_STATUS rx_status;
    LOAD_STATUS tx_status;
    UINT1 pad[2];
    double rx_kbps;        //rxkB/s
    double tx_kbps;        //txkB/s
    double rx_kpps;        //rxpck/s
    double tx_kpps;        //txpck/s
    UINT8 rx_packets;      //Number of received packets.
    UINT8 tx_packets;      //Number of transmitted packets.
    UINT8 rx_bytes;        //Number of received bytes.
    UINT8 tx_bytes;        //Number of transmitted bytes.
    UINT4 max_speed;       //Max port bitrate in kbps
    UINT4 duration_sec;    //
    UINT4 timestamp;       //
}port_stats_t;

typedef enum 
{
    NEIGH_STATE_NONE,
    NEIGH_STATE_ESTABLISHED,
    NEIGH_STATE_DELETED,
    NEIGH_STATE_UNALIVE,
}sw_neighbor_state_e;

typedef struct neighbor
{
    INT4  state;
    UINT8 src_dpid;
    UINT4 src_port;
    UINT8 dst_dpid;
    UINT4 dst_port;
    UINT8 weight;
}neighbor_t;

//交换机端口类�?
typedef enum
{
    PORT_TYPE_NONE,
    PORT_TYPE_MGMT,
    PORT_TYPE_HOST,
    PORT_TYPE_SWITCH,
    PORT_TYPE_EXTERNAL,
}sw_port_type_e;

//交换机端口状�?
typedef enum 
{
    PORT_STATE_INIT,
    PORT_STATE_UP,
    PORT_STATE_DOWN,
    PORT_STATE_BLOCKED,
    PORT_STATE_FAILOVER,
    PORT_STATE_DELETED,
}sw_port_state_e;

typedef struct gn_port
{
    INT4  state; //@sw_port_state_e
    INT4  type;  //@sw_port_type_e
    UINT4 port_no;
    UINT1 hw_addr[6];
    INT1  pad[2];
    INT1  name[16];
    UINT4 curr;
    UINT4 advertised;
    UINT4 supported;
    UINT4 peer;
    UINT4 config;
	UINT2 queue_ids[MAX_QUEUE_ID];
    port_stats_t stats;

    UINT4 lldpDetectTimes; //for TOPO keep-alive
} gn_port_t;

typedef struct stats_req_info
{
    UINT2 type;
    UINT2 flags;
    UINT4 xid;
    UINT1 *data;
}stats_req_info_t;

typedef struct flow_stats_req_data
{
    UINT4 out_port;
    UINT4 out_group;
    UINT1 table_id;
}flow_stats_req_data_t;

typedef struct port_stats_req_data
{
    UINT4 port_no;
}port_stats_req_data_t;


typedef struct packout_req_info
{
    UINT4 xid;
    UINT4 buffer_id;
    UINT4 inport;
    UINT4 outport;
    UINT2 max_len;
    UINT2 data_len;
    UINT1 *data;
}packout_req_info_t;

typedef struct flow_stats
{
    LOAD_STATUS status;
    UINT4 kbps;
    UINT4 kpps;
    UINT8 byte_count;
    UINT8 packet_count;
    UINT4 max_speed;      //Max port bitrate in kbps
    UINT4 duration_sec;   //port 存活时长
    UINT4 timestamp;      //处理成取样时间间隔的整数�?
}flow_stats_t;



typedef struct gn_oxm
{
    UINT4 in_port;        /* Switch input port. */
    UINT4 in_phy_port;    /* Switch physical input port. */
    UINT8 metadata;       /* Metadata passed between tables. */
    UINT1 eth_dst[6];     /* Ethernet destination address. */
    UINT1 eth_src[6];     /* Ethernet source address. */
    UINT2 eth_type;       /* Ethernet frame type. */
    UINT2 vlan_vid;       /* VLAN id. */
    UINT1 vlan_pcp;       /* VLAN priority. */
    UINT1 ip_dscp;        /* IP DSCP (6 bits in ToS field). */
    UINT1 ip_ecn;         /* IP ECN (2 bits in ToS field). */
    UINT1 ip_proto;       /* IP protocol. */
    UINT4 ipv4_src;       /* IPv4 source address. */
    UINT4 ipv4_dst;       /* IPv4 destination address. */
    UINT2 tcp_src;        /* TCP source port. */
    UINT2 tcp_dst;        /* TCP destination port. */
    UINT2 udp_src;        /* UDP source port. */
    UINT2 udp_dst;        /* UDP destination port. */
//    UINT2 sctp_src;       /* SCTP source port. */
//    UINT2 sctp_dst;       /* SCTP destination port. */
    UINT1 icmpv4_type;    /* ICMP type. */
    UINT1 icmpv4_code;    /* ICMP code. */
    UINT1 arp_op;         /* ARP opcode. */
    UINT4 arp_spa;        /* ARP source IPv4 address. */
    UINT4 arp_tpa;        /* ARP target IPv4 address. */
    UINT1 arp_sha[6];        /* ARP source hardware address. */
    UINT1 arp_tha[6];        /* ARP target hardware address. */
    UINT1 ipv6_src[16];       /* IPv6 source address. */
    UINT1 ipv6_dst[16];       /* IPv6 destination address. */
//    UINT4 ipv6_flabel;    /* IPv6 Flow Label */
//    UINT1 icmpv6_type;    /* ICMPv6 type. */
//    UINT1 icmpv6_code;    /* ICMPv6 code. */
//    UINT1 ipv6_nd_target; /* Target address for ND. */
//    UINT1 ipv6_nd_sll;    /* Source link-layer for ND. */
//    UINT1 ipv6_nd_tll;    /* Target link-layer for ND. */
    UINT4 mpls_label;     /* MPLS label. */
//    UINT1 mpls_tc;        /* MPLS TC. */
//    UINT1 ofpxmt_ofp_mpls_bos; /* MPLS BoS bit. */
//    UINT1 pbb_isid;       /* PBB I-SID. */
    UINT4 tunnel_id;      /* Logical Port Metadata. */
//    UINT1 ipv6_exthdr;    /* IPv6 Extension Header pseudo-field */
    UINT4 ipv4_src_prefix;
    UINT4 ipv4_dst_prefix;
    UINT4 ipv6_src_prefix;
    UINT4 ipv6_dst_prefix;
    UINT8 metadata_mask;
    UINT8 mask;
}gn_oxm_t;


typedef struct gn_match
{
    UINT2 type;
    gn_oxm_t oxm_fields;
}gn_match_t;
typedef struct gn_instruction
{
    struct gn_instruction *next;
    UINT2 type;
}gn_instruction_t;

typedef struct gn_flow
{
    struct gn_flow *prev;
    struct gn_flow *next;
    INT1 uuid[UUID_LEN];
    INT1 creater[16];
    UINT8 create_time;
    UINT1 table_id;
    UINT1 status;
    UINT2 idle_timeout;
    UINT2 hard_timeout;
    UINT2 priority;
    flow_stats_t stats;
    gn_match_t match;
    gn_instruction_t *instructions;
}gn_flow_t;

typedef struct gn_action
{
    struct gn_action *next;
    UINT2 type;
}gn_action_t;

typedef struct gn_action_output
{
    struct gn_action *next;
    UINT2 type;
    UINT4 port;
    UINT2 max_len;
}gn_action_output_t;

typedef struct gn_action_set_queue
{
    struct gn_action *next;
    UINT2 type;
    UINT4 queue_id;
}gn_action_set_queue_t;

typedef struct gn_action_group
{
    struct gn_action *next;
    UINT2 type;
    UINT4 group_id;
}gn_action_group_t;

typedef struct gn_action_mpls_ttl
{
    struct gn_action *next;
    UINT2 type;
    UINT1 mpls_tt;
}gn_action_mpls_ttl_t;

typedef struct gn_action_nw_ttl
{
    struct gn_action *next;
    UINT2 type;
    UINT1 nw_tt;
}gn_action_nw_ttl_t;

typedef struct gn_action_push
{
    struct gn_action *next;
    UINT2 type;
    UINT1 mpls_tt;
}gn_action_push_t;

typedef struct gn_action_pop
{
    struct gn_action *next;
    UINT2 type;
    UINT1 mpls_tt;
}gn_action_pop_t;

typedef struct gn_action_set_field
{
    struct gn_action *next;
    UINT2 type;
    gn_oxm_t oxm_fields;
}gn_action_set_field_t;

typedef struct gn_action_experimenter
{
    struct gn_action *next;
    UINT2 type;
    UINT4 experimenter;
}gn_action_experimenter_t;

typedef struct gn_instruction_goto_table
{
    struct gn_instruction *next;
    UINT2 type;
    UINT1 table_id;
}gn_instruction_goto_table_t;

typedef struct gn_instruction_write_metadata
{
    struct gn_instruction *next;
    UINT2 type;
    UINT8 metadata;
    UINT8 metadata_mask;
}gn_instruction_write_metadata_t;

typedef struct gn_instruction_actions
{
    struct gn_instruction *next;
    UINT2 type;
    gn_action_t *actions;
}gn_instruction_actions_t;

typedef struct gn_instruction_meter
{
    struct gn_instruction *next;
    UINT2 type;
    UINT4 meter_id;
}gn_instruction_meter_t;

typedef struct gn_instruction_experimenter
{
    struct gn_instruction *next;
    UINT2 type;
    UINT2 len;
    UINT4 experimenter;
}gn_instruction_experimenter_t;

typedef struct flow_mod_req_info
{
    UINT4 xid;
    UINT4 buffer_id;
    UINT4 out_port;
    UINT4 out_group;
    UINT1 command;
    UINT2 flags;
    gn_flow_t *flow;
}flow_mod_req_info_t;

typedef struct sw_path_node
{
    //CSmartPtr<CSwitch> sw; //路径点交换机
    UINT8 sw_dpid;
    UINT4 port_no;         //路径点交换机port
} sw_path_node_t, *p_sw_path_node;

typedef struct sw_path
{
    //CSmartPtr<CSwitch> src_sw;            //路径起点
    //CSmartPtr<CSwitch> dst_sw;            //路径终点
    UINT8   src_dpid;
	UINT8   dst_dpid;
    std::list<sw_path_node_t*> node_list; //路径经过的所有点
} sw_path_t, *p_sw_path;

typedef struct sw_path_list
{
    //CSmartPtr<CSwitch> dst_sw;       //路径链所属交换机
    UINT8    dst_dpid;
    std::vector<sw_path_t*> path_list; //其它交换机到该交换机的所有路�?
   // std::map<UINT8/*dpid*/, sw_path_t*> path_list; //其它交换机到该交换机的所有路�?
} sw_path_list_t, *p_sw_path_list;


typedef struct sw_tagflow
{
	CSmartPtr<CSwitch> dst_sw;            //路径终点
	CSmartPtr<CSwitch> src_sw;            //路径起点
	UINT4   outport;
   
}sw_tagflow_t, *p_sw_tagflow;
	
typedef struct ip_addr_s 
{
    UINT4 is_ip6; /*value only 0 & 1*/
    union
    {
        UINT4 ip4;
        UINT1 ip6[IPV6_LEN];
    };
} ip_addr_t;


typedef struct
{
    UINT4  dstIp;
	UINT4  srcIp;
	packet_in_info_t *packet_in;
} arp_request_t, *p_arp_request;


typedef struct
{
    UINT4  dstIp;
	packet_in_info_t *packet_in;
} arp_floodnode_t, *p_arp_arpflood;


typedef struct security_param_set
{
	 UINT2 tcp_port_num;        /* port number*/
	 UINT2 udp_port_num;        /* port number*/
	 UINT1 ip_proto;        	/* IP protocol*/
	 UINT1 imcp_type; 	    	/* ICMP type. */
	 UINT1 icmp_code;   	  	/* ICMP code. */
	 UINT1 rev;
	 
}security_param_t, *security_param_p;

typedef struct action_param
{
	INT4 type;
	void* param;
	struct action_param *next;
}action_param_t, *action_param_p;

//by:yhy 流表参数结构�?
typedef struct flow_param
{
	gn_oxm_t* match_param;
	action_param_t* instruction_param;
	action_param_t* action_param;			/* OFPIT_APPLY_ACTIONS use*/
	action_param_t* write_action_param;		/* OFPIT_WRITE_ACTIONS use*/
}flow_param_t;

typedef struct param_set
{
	UINT4 src_ip; //network order
	UINT4 dst_ip; //network order
	UINT1 src_mac[6];
	UINT1 dst_mac[6];
	CSmartPtr<CSwitch>  src_sw;
	CSmartPtr<CSwitch>  dst_sw;
	CSmartPtr<CSwitch>  out_sw;
	UINT1 proto;
	UINT1 Rev[3];
	UINT2 src_port_no; //network order
	UINT2 dst_port_no; //host order
	UINT4 src_inport;
	UINT4 dst_inport;
	UINT1 src_gateway_mac[6];
	UINT1 dst_gateway_mac[6];
	UINT4 mod_src_ip;
	UINT4 mod_dst_ip;
	UINT4 mod_src_port_no;
	UINT4 mod_dst_port_no;
	UINT4 src_vlanid;
	UINT4 dst_vlanid;
	UINT1 packet_src_mac[6];
	UINT1 packet_dst_mac[6];
	UINT4 outer_ip; //network order
	UINT1 outer_mac[6];
	UINT1 outer_gateway_mac[6];
	UINT4 dst_gateway_output;
	UINT4 vip;
	UINT1 vip_mac[6];
	UINT4 vip_tcp_port_no;
}param_set_t, *param_set_p;

#endif

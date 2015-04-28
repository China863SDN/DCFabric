/******************************************************************************
*                                                                             *
*   File Name   : mod-types.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-2           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef MOD_TYPES_H_
#define MOD_TYPES_H_

#define GN_FLOW_MAX_LEN 1500

extern const UINT8 MASK_SET;

#define UUID_LEN 40

//���ⳡ������������ȼ�
#define FLOW_FIREWALL_PRIORITY 5
#define FLOW_TENANT_PRIORITY 6

//���ⳡ���������ʶ
#define FLOW_FIREWALL_CREATER "Firewall"
#define FLOW_TENANT_CREATER "Tenant"
#define FLOW_L2_CREATER "L2Forwarder"
#define FLOW_L3_CREATER "L3Forwarder"

//����״̬
typedef enum
{
    LOAD_IDLE = 0,  // 0% - 25%
    LOAD_LIGHT = 1, // 25% - 50
    LOAD_BUSY = 2,  // 50% - 75%
    LOAD_HEAVY = 3, // 75% - 100%
    LOAD_FULL = 4   // 100%
}LOAD_STATUS;

enum ENTRY_STATUS
{
    ENTRY_DISABLED = 0,
    ENTRY_ENABLED = 1,
    ENTRY_TIMEOUT = 2,
    ENTRY_UNKNOWN = 3
};

#pragma pack(1)

//typedef struct gn_oxm
//{
////    struct gn_oxm *prev;
//    struct gn_oxm *next;
//    UINT2 oxm_class;
//    UINT1 oxm_field_type;
//    UINT1 data_length;
//    UINT1 data[0];
//}gn_oxm_t;

typedef struct gn_oxm
{
    UINT8 mask;
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
//    UINT4 tunnel_id;      /* Logical Port Metadata. */
//    UINT1 ipv6_exthdr;    /* IPv6 Extension Header pseudo-field */
    UINT4 ipv4_src_prefix;
    UINT4 ipv4_dst_prefix;
    UINT4 ipv6_src_prefix;
    UINT4 ipv6_dst_prefix;
    UINT8 metadata_mask;
}gn_oxm_t;

typedef struct gn_match
{
    UINT2 type;
    gn_oxm_t oxm_fields;
}gn_match_t;

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

typedef struct gn_action_group
{
    struct gn_action *next;
    UINT2 type;
    UINT4 group_id;
}gn_action_group_t;

typedef struct gn_action_set_queue
{
    struct gn_action *next;
    UINT2 type;
    UINT4 queue_id;
}gn_action_set_queue_t;

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

typedef struct gn_instruction
{
    struct gn_instruction *next;
    UINT2 type;
}gn_instruction_t;

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

typedef struct flow_stats
{
    LOAD_STATUS status;
    UINT4 kbps;
    UINT4 kpps;
    UINT8 byte_count;
    UINT8 packet_count;
    UINT4 max_speed;      //Max port bitrate in kbps
    UINT4 duration_sec;   //port ���ʱ��
    UINT4 timestamp;      //�����ȡ��ʱ������������
}flow_stats_t;

typedef struct gn_flow
{
    struct gn_flow *prev;
    struct gn_flow *next;
    INT1 uuid[40];
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

struct gn_switch;
typedef struct gn_meter
{
    struct gn_meter *pre;
    struct gn_meter *next;
    UINT4 meter_id;
    UINT2 flags;
    UINT2 type;
    UINT4 rate;
    UINT4 burst_size;
    UINT1 prec_level;
}gn_meter_t;

typedef struct group_bucket
{
    struct group_bucket *next;
    UINT2 weight;
    UINT4 watch_port;
    UINT4 watch_group;
    gn_action_t *actions;
}group_bucket_t;

typedef struct gn_group
{
    struct gn_group *pre;
    struct gn_group *next;
    UINT1 type;
    UINT2 group_id;
    group_bucket_t *buckets;
}gn_group_t;

#pragma pack()

#endif /* MOD_TYPES_H_ */

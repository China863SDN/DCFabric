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
*   File Name   : gn_inet.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef GN_INET_H_
#define GN_INET_H_

//#include "common.h"

#define MAX_PKT 1600

//const UINT2 ETHER_LLDP = 0x88cc;
//const UINT2 ETHER_ARP = 0x0806;
//const UINT2 ETHER_IP = 0x0800;
//const UINT2 ETHER_IPV6 = 0x86DD;


enum ether_type
{
    ETHER_LLDP = 0x88cc,
    ETHER_ARP = 0x0806,
    ETHER_IP = 0x0800,
    ETHER_IPV6 = 0x86DD,
    ETHER_VLAN = 0x8100,
    ETHER_MPLS = 0x8847
};

//enum ip_proto
//{
//    IPPROTO_ICMP = 1,
//    IPPROTO_TCP = 6,
//    IPPROTO_UDP = 17
//};

/* 802.1AB-2005 LLDP support code */
enum lldp_tlv_type{
    /* start of mandatory TLV */
    LLDP_END_OF_LLDPDU_TLV = 0,
    LLDP_CHASSIS_ID_TLV = 1,
    LLDP_PORT_ID_TLV = 2,
    LLDP_TTL_TLV = 3,
    /* end of mandatory TLV */
    /* start of optional TLV */ /*NOT USED */
    LLDP_PORT_DESC_TLV = 4,
    LLDP_SYSTEM_NAME_TLV = 5,
    LLDP_SYSTEM_DESC_TLV = 6,
    LLDP_SYSTEM_CAPABILITY_TLV = 7,
    LLDP_MGMT_ADDR_TLV = 8
    /* end of optional TLV */
};

enum lldp_chassis_id_subtype {
    LLDP_CHASSIS_IP_MAC              = 4,
    LLDP_CHASSIS_ID_LOCALLY_ASSIGNED = 7
};

enum lldp_port_id_subtype {
    LLDP_PORT_ID_COMPONENT        = 2,
    LLDP_PORT_ID_LOCALLY_ASSIGNED = 7
};

#pragma pack(1)
typedef struct st_ether
{
    UINT1 dest[6];
    UINT1 src[6];
    UINT2 proto;
    UINT1 data[0];
}ether_t;

typedef struct st_arp
{
    ether_t eth_head;
    UINT2 hardwaretype;
    UINT2 prototype;
    UINT1 hardwaresize;
    UINT1 protocolsize;
    UINT2 opcode;
    UINT1 sendmac[6];
    UINT4 sendip;
    UINT1 targetmac[6];
    UINT4 targetip;
    UINT1 data[0];
}arp_t;

typedef struct st_ip
{
    ether_t eth_head;
    UINT1 hlen;
    UINT1 tos;
    UINT2 len;
    UINT2 ipid;
    UINT2 fragoff;
    UINT1 ttl;
    UINT1 proto;
    UINT2 cksum;
    UINT4 src;
    UINT4 dest;
    UINT1 data[0];
}ip_t;

typedef struct st_tcp
{
    UINT2 sport;
    UINT2 dport;
    UINT4 seq;
    UINT4 ack;
    UINT1 offset;
    UINT1 code;
    UINT2 window;
    UINT2 cksum;
    UINT2 urg;
    UINT1 data[0];
}tcp_t;

typedef struct st_udp
{
    UINT2 sport;
    UINT2 dport;
    UINT2 len;
    UINT2 cksum;
    UINT1 data[0];
}udp_t;

typedef struct st_imcp
{
    UINT1 type;
    UINT1 code;
    UINT2 cksum;
    UINT2 id;
    UINT2 seq;
    UINT1 data[0];
}icmp_t;

typedef  struct  st_ipv6
{
    UINT4 version;
    UINT2 len; // payload len. not including header.
    UINT1 next_head;
    UINT1 hop;
    union {
        UINT1 src[16];  // 128 bit.
        UINT4 src1[4];  // 128 bit.
        UINT8 src2[2];
    };
    union {
        UINT1 dest[16]; // 128 bit.
        UINT4 dest1[4]; // 128 bit.
        UINT8 dest2[2];
    };
    UINT1 data[0];
}ipv6_t;


typedef struct st_t802_1q
{
    UINT2 proto;
    UINT2 vlan;           //4,    12      ���ȼ� + vlanid
    UINT1 data[0];
}t802_1q_t;

typedef struct st_dhcp
{
    UINT1 opt;
    UINT1 htype;
    UINT1 hlen;
    UINT1 hops;
    UINT4 xid;
    UINT2 secd;
    UINT2 flg;
    UINT4 cipaddr;  //client ip addr;
    UINT4 yipaddr;  //you ip addr;
    UINT4 sipaddr;  //server ip addr;
    UINT4 gipaddr;  //agent ip addr;
    UINT1 cmcaddr[16];
    UINT1 hostname[64];
    UINT1 filename[128];
    UINT4 sname;
    UINT1 data[0];
}dhcp_t;

typedef struct st_tlv
{
    UINT1 type;
    UINT1 len;
    char  data[0];
}tlv_t;

typedef struct st_dns_a
{
    UINT2 label_ptr;
    UINT2 type;         // type
    UINT2 clas;         // class
    UINT4 ttl;
    UINT2 len;
    UINT1  data[0];
}dns_a_t;

typedef struct st_dns_q
{
    UINT1   len;
    char    *data;
}dns_q_t;

typedef struct st_dns
{
    UINT2 id;
    UINT2 flags;
    UINT2 questions;
    UINT2 answer;
    UINT2 author;
    UINT2 add;
    dns_q_t  *dns_q;
}dns_t;

typedef struct st_ethpkt_info
{
    UINT1 data[MAX_PKT];
    UINT2 len;
    UINT2 vlan_id;
    INT4  qid;
    UINT1 ifindex;
    ether_t *pEth;
    ip_t *pIp;
    arp_t *pArp;
    tcp_t *pTcp;
    udp_t *pUdp;
    icmp_t *pIcmp;
    t802_1q_t *q8021q;
}ethpkt_info_t;

typedef struct st_lldp
{
    ether_t eth_head;

    //9 Byte
    UINT2 chassis_tlv_type_and_len;   //0x0207
    UINT1 chassis_tlv_subtype;       //MAC             4
    UINT8 chassis_tlv_id;            //dpid

    //5 Byte
    UINT2 port_tlv_type_and_len;      //0x0403
    UINT1 port_tlv_subtype;          //local assigned    7
    UINT2 port_tlv_id;                //send port

    //4 Byte
    UINT2 ttl_tlv_type_and_len;      //0x0602
    UINT2 ttl_tlv_ttl;               //ttl 120

    //2
    UINT2 endof_lldpdu_tlv_type_and_len;
}lldp_t;

#pragma pack()
#endif /* GN_INET_H_ */

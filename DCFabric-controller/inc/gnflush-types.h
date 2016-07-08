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
*   File Name   : gnflush-types.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-26           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef GNFLUSH_TYPES_H_
#define GNFLUSH_TYPES_H_

#include "common.h"
#include "ini.h"
#include "json.h"
#include "mod-types.h"
#include "error_info.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#define MAX_PORTS 150
#define MAX_QUEUE_ID 40960

extern ini_file_t *g_controller_configure;
extern UINT1 g_controller_mac[];
extern UINT4 g_controller_ip;
extern UINT4 g_controller_south_port;
extern UINT1 g_is_cluster_on;
extern UINT1 g_reserve_mac[];
extern UINT4 g_reserve_ip;


enum http_type
{
    HTTP_GET = 0,
    HTTP_POST = 1,
    HTTP_PUT = 2,
    HTTP_DELETE = 3
};
enum forward_ip_type
{
	IP_DROP = 1,
    CONTROLLER_FORWARD = 2,
    IP_FLOOD = 3,
    BROADCAST_DHCP = 4,
    IP_PACKET = 5,
	IP_HANDLE_ERR = 6,
	Internal_port_flow = 7,
	Internal_out_subnet_flow = 8,
	Floating_ip_flow = 9,
	Nat_ip_flow = 10,
	Internal_vip_flow = 11,
	External_vip_flow = 12,
	Internal_floating_vip_flow = 13,
};

#pragma pack(1)
struct gn_switch;
struct gn_port;

typedef INT1 *(*restful_handler_t)(const INT1 *url, json_t *root);
typedef struct restful_handles
{
    UINT1 used;
    INT1 url[128];
    UINT4 url_len;
    restful_handler_t handler;
}restful_handles_t;

typedef struct packet_in_info
{
    UINT4 xid;          //packet in transection id
    UINT4 buffer_id;    //buffer id
    UINT4 inport;       //��ڽ�����˿ں�
    UINT4 data_len;     //��̫����ݰ��
    UINT1 *data;        //��̫����ݰ�
}packet_in_info_t;

typedef INT4 (*packet_in_proc_t)(struct gn_switch *sw, packet_in_info_t *packet_in_info);

typedef struct mac_user mac_user_t;
typedef struct mac_user_table
{
    UINT4 macuser_hsize;            //ָ���û���ϣ���С  ������
    UINT4 macuser_hsize_tot;        //ָ���û���ϣ���С  ������
    UINT4 macuser_lifetime;         //ָ���û������ʱ��  ������  ��ʵ��SDN�������hard time
    mac_user_t **macuser_tb;        //����洢��ϣ���׵�ַ���ڴ�
    void *macuser_memid;            //�ڴ�ر�־
    pthread_mutex_t macuser_mutex;  //ȫ�ֱ� ��ѯʱ����
    void *macuser_timer;            //��ʱ����־
    UINT4 macuser_cnt;              //��¼MAC ��ַ�û���
}mac_user_table_t;


struct mac_user
{
    struct gn_switch *sw;     //���ڽ�����
    UINT4 port;          //��MAC��ַ��Ӧ�Ķ˿�  �����ֽ���
    INT4 tenant_id;      //��ʶ���û������ĸ��⻧����
    UINT4 ipv4;          //ip,�����ֽ���
    UINT1 ipv6[16];      //ipv6,�����ֽ���
    UINT1 mac[6];        //�û�MAC��ַ
    time_t tv_last_sec;  //�����µ�ʱ��
    void *timer;         //�����Ķ�ʱ��

    mac_user_table_t *macuser_table;
    mac_user_t *next;
    mac_user_t *prev;
};

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
    UINT4 duration_sec;    //port ���ʱ��
    UINT4 timestamp;       //�����ȡ��ʱ����������
}port_stats_t;

typedef struct gn_server
{
    UINT4 sock_fd;
    UINT4 ip;
    UINT4 port;
    UINT4 buff_num;
    UINT4 buff_len;
    UINT4 max_switch;
    UINT4 cur_switch;
    UINT4 cpu_num;
    struct gn_switch *switches;
    UINT1 state;
    UINT1 pad[3];
}gn_server_t;

typedef struct buffer_cache
{
    UINT1 *start_pos;        //������´δ�����ʼλ��
    UINT1 *buff;
    UINT4 len;
    UINT4 bak_len;           //�ϸ�bufferδ������ĳ���
}buffer_cache_t;

typedef struct buffer_list
{
    buffer_cache_t *buff_arr;
    UINT4 head;
    UINT4 tail;
}buffer_list_t;

typedef struct neighbour
{
    struct gn_switch *neigh_sw;     //���ڵĽ�����
    struct gn_port *neigh_port;  //�����ڽ�����ֱ���Ķ˿�
}neighbour_t;

typedef struct gn_port
{
    UINT4 port_no;
    UINT1 hw_addr[6];
    UINT1 pad[2];
    INT1 name[16];
    UINT4 curr;
    UINT4 advertised;
    UINT4 supported;
    UINT4 peer;
    UINT4 config;
    UINT4 state;
	UINT4 queue_ids[MAX_QUEUE_ID];
    neighbour_t *neighbour;
    mac_user_t *user_info;
    port_stats_t stats;     //portʵʱ��������Ϣ
} gn_port_t;

typedef gn_port_t *(*port_convertter_t)(UINT1 *of_port, gn_port_t *new_port);
typedef UINT1 (*oxm_convertter_t)(UINT1 *of_flow, gn_oxm_t *new_oxm);
typedef INT4 (*msg_handler_t)(struct gn_switch *sw, UINT1 *of_msg);

typedef struct convertter
{
    port_convertter_t port_convertter;
    oxm_convertter_t oxm_convertter;
}convertter_t;

typedef struct msg_driver
{
    UINT4 type_max;
    convertter_t *convertter;
    msg_handler_t *msg_handler;
}msg_driver_t;

typedef struct switch_desc {
   char mfr_desc[256];       /* Manufacturer description. */
   char hw_desc[256];        /* Hardware description. */
   char sw_desc[256];        /* Software description. */
   char serial_num[32];      /* Serial number. */
   char dp_desc[256];        /* Human readable description of datapath. */
}switch_desc_t;

typedef struct neighbor
{
    struct gn_switch *sw;
    gn_port_t *port;
    UINT8 weight;
}neighbor_t;

typedef struct gn_flowmod_helper
{
    gn_flow_t flow;
    gn_instruction_actions_t instruction;
    gn_action_output_t action_output;
    gn_action_set_field_t action_set_field;
}gn_flowmod_helper_t;

struct mac_user_table;
typedef struct gn_switch
{
    UINT1 ofp_version;
    UINT1 state;
    UINT1 pad[2];
    INT4 index;
    UINT8 dpid;
    UINT4 sw_ip;
    UINT4 sw_port;
    UINT4 sock_fd;
    UINT4 n_buffers;
    UINT4 n_tables;
    UINT4 n_ports;
    UINT4 capabilities;
    switch_desc_t sw_desc;
    gn_port_t lo_port;
    gn_port_t ports[MAX_PORTS];
    neighbor_t *neighbor[MAX_PORTS];
    mac_user_t **users;
    msg_driver_t msg_driver;
    buffer_list_t recv_buffer;
    UINT4 send_len;
    UINT1 *send_buffer;
    gn_flowmod_helper_t flowmod_helper;
    gn_flow_t *flow_entries;
    gn_meter_t *meter_entries;
	INT4 qos_type;
	gn_qos_t* qos_entries;
    gn_group_t* group_entries;
	gn_queue_t* queue_entries;
    pthread_mutex_t users_mutex;
    pthread_mutex_t send_buffer_mutex;
    pthread_mutex_t flow_entry_mutex;
    pthread_mutex_t meter_entry_mutex;
    pthread_mutex_t group_entry_mutex;
    pthread_t pid_recv;   //�հ�
    pthread_t pid_proc;   //�����
    UINT8 connected_since;
    UINT8 weight;         //sw_weight
    UINT1 sock_state;     //socket status
    pthread_mutex_t sock_state_mutex;
}gn_switch_t;


/*
 * ��Ϣ������Ϣ�ṹ��
 */
typedef struct role_req_info
{
    UINT4 role;
    UINT8 generation_id;
}role_req_info_t;

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

typedef struct meter_mod_req_info
{
    UINT4 xid;
    UINT2 command;
    gn_meter_t *meter;
}meter_mod_req_info_t;

typedef struct group_mod_req_info
{
    UINT4 xid;
    UINT2 command;
    gn_group_t *group;
}group_mod_req_info_t;

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

typedef struct stats_req_info
{
    UINT2 type;
    UINT2 flags;
    UINT4 xid;
    UINT1 *data;
}stats_req_info_t;

typedef struct neutron_network
{
    BOOL is_using;               //�Ƿ���ռ��
    INT1 name[64];
    INT1 physical_network[64];
    BOOL admin_state_up;
    INT1 tenant_id[64];
    INT1 network_type[16];
    BOOL router_external;
    BOOL shared;
    INT1 id[64];
    INT1 segmentation_id;
}neutron_network_t;

typedef struct neutron_subnet
{
    BOOL is_using;              //�Ƿ���ռ��
    INT1 ippool_start[32];
    INT1 ippool_end[32];
    INT1 host_routes[64];
    INT1 cidr[32];
    INT1 id[64];
    INT1 name[64];
    BOOL enable_dhcp;
    INT1 network_id[64];
    INT1 tenant_id[64];
    INT1 dns_nameservers[64];
    INT1 gateway_ip[32];
    INT1  ip_version;
    BOOL shared;
}neutron_subnet_t;

typedef struct neutron_port
{
    BOOL is_using;       //�Ƿ���ռ��
    char binding_host_id[63];

    char allowed_address_pairs[64];
    char extra_dhcp_opts[64];
    char device_owner[64];
    char subnet_id[64];
    char ip_address[32];
    char id[64];
    char security_groups[32];
    char device_id[80];
    char name[32];
    BOOL admin_state_up;
    char network_id[63];
    char tenant_id[64];
    char binding_vif_type[32];
    BOOL port_filter;
    char mac_address[31];
}neutron_port_t;
#pragma pack()

typedef void (*initcall_t)();
extern initcall_t __start_appinit_sec, __stop_appinit_sec;
#define data_attr_init __attribute__ ((section ("appinit_sec")))
#define app_init(x)   initcall_t _##x data_attr_init = x

typedef void (*finicall_t)();
extern finicall_t __start_appfini_sec, __stop_appfini_sec;
#define data_attr_fini __attribute__ ((section ("appfini_sec")))
#define app_fini(x)   finicall_t _##x data_attr_fini = x

typedef void (*proccall_t)(gn_switch_t *sw);
extern proccall_t __start_appproc_sec, __stop_appproc_sec;
#define data_attr_proc __attribute__ ((section ("appproc_sec")))
#define app_proc(x)   proccall_t _##x data_attr_proc = x


extern gn_server_t g_server;

#endif /* GNFLUSH_TYPES_H_ */

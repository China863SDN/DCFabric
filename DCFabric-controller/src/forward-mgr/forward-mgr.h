/******************************************************************************
*                                                                             *
*   File Name   : forward-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef FORWARD_MGR_H_
#define FORWARD_MGR_H_

#include "gnflush-types.h"

#define MAX_L3_SUBNET 10
#define MAX_L3_SUBNET_NAME_LEN 64

#pragma pack(1)
typedef struct subnet
{
    BOOL is_using;       //�Ƿ���ռ��
    INT1 name[64];       //������
    INT1 netmask[16];    //��������
    UINT4 gw_prefix;     //��������
    UINT4 gw_ip;         //����ip      �����ֽ���
    UINT4 gw_minip;      //��Сip      �����ֽ���
    UINT4 gw_maxip;      //���ip      �����ֽ���
}subnet_t;

typedef struct forward_handler
{
    packet_in_proc_t lldp;
    packet_in_proc_t arp;
    packet_in_proc_t ip;
    packet_in_proc_t ipv6;
}forward_handler_t;
#pragma pack()


//�����ṩ������ע�ᣬ��ͬ�Ľ�����ʹ�ò�ͨ�Ĵ����߼�
extern forward_handler_t g_default_forward_handler;

//L3������Ϣ
extern subnet_t g_subnet_info[];

UINT4 find_gateway_ip(UINT4 ip);
subnet_t *search_l3_subnet(UINT4 ip);
INT4 create_l3_subnet(INT1 *name, INT1 *masked_ip);
INT4 destory_l3_subnet(INT1 *masked_ip);

INT4 packet_in_process(gn_switch_t *sw, packet_in_info_t *packet_in_info);
INT4 init_forward_mgr();
void fini_forward_mgr();

#endif /* FORWARD_MGR_H_ */

/******************************************************************************
*                                                                             *
*   File Name   : tenant-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "gnflush-types.h"

#ifndef TENANT_MGR_H_
#define TENANT_MGR_H_

#define TENANT_NAME_LEN 128
#define TENANT_MAX_MEMBER 128
#define TENANT_MAX 128
#define TENANT_TIMEOUT 0

#define SYS_ERR -1
#define OPT_SUC 0
#define TNT_EXIST 1
#define TNT_NOT_EXIST 2
#define TNT_STATE_INVALID 3
#define MEB_EXIST 4
#define MEB_NOT_EXIST 5
#define MEB_STATE_INVALID 6
#define MAX_LIMITED 7

#pragma pack(1)
typedef struct tenant
{
    UINT4 tenant_id;                    //�⻧ID
    UINT4 member_cnt;                   //���⻧�µ�ǰ��Ա����
    char tenant_name[TENANT_NAME_LEN];  //�⻧����
    UINT1 **member_mac;                 //�⻧��ԱMAC��Ϣ
    UINT1 status;                       //�⻧״̬��0��������  ��Ԥ����
}tenant_t;

typedef struct tenant_member
{
    struct tenant_member *pre;   //��һ���⻧��Ա
    struct tenant_member *next;  //��һ���⻧��Ա
    UINT1 mac[6];       //�û�MAC��ַ
    UINT1 status;       //�⻧״̬��0�������� ��Ԥ����
    UINT4 tenant_id;    //�����⻧���⻧ID
}tenant_member_t;

typedef struct tenant_container
{
    tenant_t **tenants;                 //�����⻧
    pthread_mutex_t *tenant_mutex;      //�⻧��
    UINT4 max_tenants;          //��������⻧��
    UINT4 max_memebers;         //ÿ���⻧��������Ա��
    UINT4 tenant_cnt;           //��ǰ�⻧����
}tenant_container_t;

typedef struct tenant_member_container
{
    tenant_member_t **member_table; //����洢��ϣ���ڴ���׵�ַ
    void *table_mem_id;             //�ڴ�ر�־
    pthread_mutex_t *member_mutex;  //��Ա��
    UINT4 member_cnt;               //��ǰ�����⻧��Ա����
    UINT4 table_size;               //ָ����ϣ���С����Ա������
}tenant_member_container_t;
#pragma pack()

//tenant management
INT4 create_tenant(const char* tenant_name);
tenant_container_t* query_tenants();
INT4 delete_tenant(UINT4 tenant_id);
void clear_tenants();

//tenant member management
INT4 add_tenant_member(UINT4 tenant_id, UINT1* mac);
INT4 delete_tenant_member(UINT1* mac);
INT4 clear_tenant_members(UINT4 tenant_id);
tenant_t* query_tenant_members(UINT4 tenant_id);

INT4 query_tenant_id(UINT1 *mac);

//tenant flow mod
void tenant_send_flow_mod_l2(gn_switch_t* sw, UINT1* eth_src, UINT1* eth_dst, UINT2 flow_mod_type);
void tenant_send_flow_mod_l3(gn_switch_t* sw, UINT4 ipv4_src, UINT4 ipv4_dst, UINT2 flow_mod_type);

INT4 init_tenant_mgr();
void fini_tenant_mgr();

#endif /* TENANT_MGR_H_ */

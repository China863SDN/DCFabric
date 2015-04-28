/******************************************************************************
*                                                                             *
*   File Name   : flow-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-2           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef FLOW_MGR_H_
#define FLOW_MGR_H_

#include "gnflush-types.h"

extern void *g_gnflow_mempool_id;
extern void *g_gninstruction_mempool_id;
extern void *g_gnaction_mempool_id;

//����uuid��������
gn_flow_t * find_flow_entry_by_id(gn_switch_t *sw, gn_flow_t *flow);

//����������Ϣ��������
gn_flow_t * find_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//�����·�һ������
INT4 add_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//�޸�һ������
INT4 modify_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//ʹ������Ч(�·���������)
INT4 enable_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//������ʧЧ(�ӽ�������ɾ��,�������ϱ���)
INT4 disable_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//ɾ��һ������(�ӽ������Ϳ�������ɾ��)
INT4 delete_flow_entry(gn_switch_t *sw, gn_flow_t *flow);

//�����������(�ӽ������Ϳ�������ɾ����������)
INT4 clear_flow_entries(gn_switch_t *sw);

//�·�table_miss����
INT4 flow_mod_table_miss(gn_switch_t *sw);

//����ʱ
INT4 flow_entry_timeout(gn_switch_t *sw, gn_flow_t *flow);

//�ͷ�flow_entry�ڴ�ռ�
void gn_flow_free(gn_flow_t *flow);

INT4 init_flow_mgr();
#endif /* FLOW_MGR_H_ */

/******************************************************************************
*                                                                             *
*   File Name   : topo-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef TOPO_MGR_H_
#define TOPO_MGR_H_

#include "gnflush-types.h"
#include "../forward-mgr/forward-mgr.h"

#define NO_PATH -1

//�ڽӾ����ʾ��ͼ
typedef struct adac_matrix
{
    UINT1 *V;              //����洢�ռ�
    UINT4 **src_port;      //�ĸ��˿�����һ���ڵ�����
    UINT4 **A;             //�ڽӾ���
    gn_switch_t ***sw;
}adac_matrix_t;

extern adac_matrix_t g_adac_matrix;
extern UINT4 **g_short_path;     //v������������·������
extern UINT4 **g_short_weight;   //v�����������·����������

INT4 lldp_packet_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info);
INT4 init_topo_mgr();

#endif /* TOPO_MGR_H_ */

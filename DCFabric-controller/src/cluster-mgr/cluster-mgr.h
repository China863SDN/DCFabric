/******************************************************************************
*                                                                             *
*   File Name   : cluster-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef CLUSTER_MGR_H_
#define CLUSTER_MGR_H_

#include "gnflush-types.h"

#define MAX_CONTROLLER 8

#pragma pack(1)
typedef struct cluster_node
{
    UINT4 cluster_id;
    INT1 controller_ip[36];
}cluster_node_t;
#pragma pack()

extern UINT1 g_controller_role;
extern UINT8 g_election_generation_id;
extern cluster_node_t g_controller_cluster[];   //控制器集群管理结构

INT4 update_role(UINT4 role);
INT4 init_cluster_mgr();
void fini_cluster_mgr();

#endif /* CLUSTER_MGR_H_ */

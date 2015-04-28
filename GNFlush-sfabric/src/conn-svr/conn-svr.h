/******************************************************************************
*                                                                             *
*   File Name   : conn-svr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef CONN_SVR_H_
#define CONN_SVR_H_

#include "msg_handler.h"
#include "gnflush-types.h"

INT4 init_conn_svr();
void fini_conn_svr();
INT4 start_openflow_server();

gn_switch_t *find_sw_by_dpid(UINT8 dpid);
void free_switch(gn_switch_t *sw);
UINT1 *init_sendbuff(gn_switch_t *sw, UINT1 of_version, UINT1 type, UINT2 buf_len, UINT4 transaction_id);
INT4 send_of_msg(gn_switch_t *sw, UINT4 total_len);
#endif /* CONN_SVR_H_ */

/******************************************************************************
*                                                                             *
*   File Name   : l3.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef L3_H_
#define L3_H_

#include "forward-mgr.h"


void l3_proc(gn_switch_t *sw, packet_in_info_t *packet_in_info, UINT4 gw_ip);
INT4 init_l3();
void fini_l3();

#endif /* L3_H_ */

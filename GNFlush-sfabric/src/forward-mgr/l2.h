/******************************************************************************
*                                                                             *
*   File Name   : l2.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef L2_H_
#define L2_H_

#include "forward-mgr.h"


void l2_proc(gn_switch_t *sw, mac_user_t *user_src, mac_user_t *user_dst, packet_in_info_t *packet_in_info);

INT4 init_l2();
void fini_l2();

#endif /* L2_H_ */

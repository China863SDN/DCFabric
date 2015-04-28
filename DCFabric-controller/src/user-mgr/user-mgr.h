/******************************************************************************
*                                                                             *
*   File Name   : user-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef USER_MGR_H_
#define USER_MGR_H_

#include "gnflush-types.h"

extern mac_user_table_t g_macuser_table;

mac_user_t* search_ip_user(UINT4 ip);
mac_user_t* search_mac_user(UINT1 *mac);
mac_user_t *create_mac_user(gn_switch_t *sw, UINT1 *mac, UINT4 inport, UINT4 host_ip, UINT4 *host_ipv6);           //新建该MAC地址用户
void close_mac_user();
void del_mac_user(mac_user_t *macuser);

INT1 init_mac_user();
void fini_mac_user();

#endif /* USER_MGR_H_ */

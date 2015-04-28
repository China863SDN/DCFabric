/******************************************************************************
*                                                                             *
*   File Name   : app_impl.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-24           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef APP_IMPL_H_
#define APP_IMPL_H_

#include "gnflush-types.h"

//by switch,packet in包中包含链路发现的lldp包，注册后会影响现有的链路发现
INT4 register_of_msg_hander(gn_switch_t *sw, UINT1 type, msg_handler_t msg_handler);
INT4 unregister_of_msg_hander(gn_switch_t *sw, UINT1 type);

//by eth_type,将影响全局交换机的packet_in数据包的处理
//但该操作对已经使用了register_of_msg_hander注册packet_in app托管的交换机无效
//eth_type取值范围： enum ether_type
INT1 register_handler_ether_packets(UINT2 eth_type, packet_in_proc_t packet_handler);

//注册新的restful接口
INT4 register_restful_handler(UINT1 type, const INT1 *url, restful_handler_t restful_handler);

#endif /* APP_IMPL_H_ */

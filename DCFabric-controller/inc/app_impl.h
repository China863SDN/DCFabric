/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

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

//by switch,packet in���а�����·���ֵ�lldp����ע����Ӱ�����е���·����
INT4 register_of_msg_hander(gn_switch_t *sw, UINT1 type, msg_handler_t msg_handler);
INT4 unregister_of_msg_hander(gn_switch_t *sw, UINT1 type);

//by eth_type,��Ӱ��ȫ�ֽ�������packet_in���ݰ��Ĵ���
//���ò������Ѿ�ʹ����register_of_msg_handerע��packet_in app�йܵĽ�������Ч
//eth_typeȡֵ��Χ�� enum ether_type
INT1 register_handler_ether_packets(UINT2 eth_type, packet_in_proc_t packet_handler);

//ע���µ�restful�ӿ�
INT4 register_restful_handler(UINT1 type, const INT1 *url, restful_handler_t restful_handler);

#endif /* APP_IMPL_H_ */

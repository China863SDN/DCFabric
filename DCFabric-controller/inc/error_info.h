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
*   File Name   : error_info.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef ERROR_INFO_H_
#define ERROR_INFO_H_

#include "common.h"

#define ERROR_CODE_MAX 50

enum ERROR_CODE
{
    GN_ERR = -1,
    GN_OK = 0,

    //控制器服务相关
    EC_ROLE_IS_SLAVE = 1000,
    EC_ROLE_IS_EQUAL = 1001,
    EC_ROLE_IS_MASTER = 1002,
    EC_ROLE_IS_OTHER = 1003,
    EC_ROLE_INVALID = 1004,

    //交换机相关
    EC_SW_STATE_ERR = 1030,
    EC_SW_NOT_EXIST = 1031,
    EC_SW_SEND_MSG_ERR = 1032,
    EC_SW_NO_PATH = 1033,

    //flow mod相关
    EC_FLOW_EXIST = 1060,
    EC_FLOW_NOT_EXIST = 1061,

    //meter mod相关
    EC_METER_EXIST = 1090,
    EC_METER_NOT_EXIST = 1091,
    EC_METER_INVALID_FLAG = 1092,
    EC_METER_INVALID_TYPE = 1093,

    //group mod相关
    EC_GROUP_EXIST = 1120,
    EC_GROUP_NOT_EXIST = 1121,
    EC_GROUP_INVALID_TYPE = 1122,

    //restful
    EC_RESTFUL_INVALID_REQ = 1150,
    EC_RESTFUL_INVALID_ARGUMENTS = 1151,
    EC_RESTFUL_REQUIRE_SRCDPID = 1152,
    EC_RESTFUL_REQUIRE_DSTDPID = 1153,
    EC_RESTFUL_REQUIRE_TENANT_NM = 1154

};

#pragma pack(1)

typedef struct error_info
{
    INT4 code;
    INT1 msg[64];
}error_info_t;

#pragma pack()

extern error_info_t g_error_info[];

INT1 *get_error_msg(INT4 code);

#endif /* ERROR_INFO_H_ */

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
*   File Name   : error_info.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "error_info.h"
#include <locale.h>

error_info_t g_error_info[ERROR_CODE_MAX];
INT1 *UNKNOWN_ERROR = "unknown error";

error_info_t g_error_info[] =
{
    {GN_OK, "operater success"}, 
    {GN_ERR, "operate failed"}, 

    {EC_ROLE_IS_SLAVE, "the role of controller is slave"}, 
    {EC_ROLE_IS_EQUAL, "the role of controller is equal"}, 
    {EC_ROLE_IS_MASTER, "the role of controller is master"},
    {EC_ROLE_IS_OTHER, "the role of controller is other"}, 
    {EC_ROLE_INVALID, "the role of controller is wrong"}, 

    {EC_SW_STATE_ERR, "the state of switch is error"},
    {EC_SW_NOT_EXIST, "the state of switch isn't exist"},
    {EC_SW_SEND_MSG_ERR, "send message to switch fail"}, 
    {EC_SW_NO_PATH, "the path of controller isn't exist"}, 

    {EC_FLOW_EXIST, "flow table exist"}, 
    {EC_FLOW_NOT_EXIST, "flow table isn't exist"}, 

    {EC_METER_EXIST, "measure table exist"}, 
    {EC_METER_NOT_EXIST, "measure table exist"}, 
    {EC_METER_INVALID_FLAG, "measure table tag invalid"}, 
    {EC_METER_INVALID_TYPE, "measure table type invalid"},

    {EC_GROUP_EXIST, "group table exist"}, 
    {EC_GROUP_NOT_EXIST, "group table isn't exist"},
    {EC_GROUP_INVALID_TYPE, "group table type invalid"},

    {EC_RESTFUL_INVALID_REQ, "invalid request"},
    {EC_RESTFUL_REQUIRE_SRCDPID, "need to input source switch DPID"},
    {EC_RESTFUL_REQUIRE_DSTDPID, "need to input destination switch dpid"},
    {EC_RESTFUL_REQUIRE_TENANT_NM, "need to input the name of tenant"}
};


INT1 *get_error_msg(INT4 code)
{
    UINT4 idx = 0;

    for(; idx < ERROR_CODE_MAX; idx++)
    {
        if(g_error_info[idx].code == code)
        {
            return g_error_info[idx].msg;
        }
    }

    return UNKNOWN_ERROR;
}

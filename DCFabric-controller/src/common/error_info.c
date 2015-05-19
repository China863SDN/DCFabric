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
INT1 *UNKNOWN_ERROR = "未知错误";

error_info_t g_error_info[] =
{
    {GN_OK, "操作成功"},
    {GN_ERR, "操作失败"},

    {EC_ROLE_IS_SLAVE, "控制器角色为slave"},
    {EC_ROLE_IS_EQUAL, "控制器角色为equal"},
    {EC_ROLE_IS_MASTER, "控制器角色为master"},
    {EC_ROLE_IS_OTHER, "控制器角色为other"},
    {EC_ROLE_INVALID, "角色错误"},

    {EC_SW_STATE_ERR, "交换机状态错误"},
    {EC_SW_NOT_EXIST, "交换机不存在"},
    {EC_SW_SEND_MSG_ERR, "发送消息到交换机失败"},
    {EC_SW_NO_PATH, "交换机路径不存在"},

    {EC_FLOW_EXIST, "流表已存在"},
    {EC_FLOW_NOT_EXIST, "流表不存在"},

    {EC_METER_EXIST, "计量表已存在"},
    {EC_METER_NOT_EXIST, "计量表不存在"},
    {EC_METER_INVALID_FLAG, "计量表标识无效"},
    {EC_METER_INVALID_TYPE, "计量表类型无效"},

    {EC_GROUP_EXIST, "组表已存在"},
    {EC_GROUP_NOT_EXIST, "组表不存在"},
    {EC_GROUP_INVALID_TYPE, "组表类型无效"},

    {EC_RESTFUL_INVALID_REQ, "无效的请求"},
    {EC_RESTFUL_REQUIRE_SRCDPID, "需要输入源端交换机DPID"},
    {EC_RESTFUL_REQUIRE_DSTDPID, "需要输入目的端交换机DPID"},
    {EC_RESTFUL_REQUIRE_TENANT_NM, "需要输入租户名称"}
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

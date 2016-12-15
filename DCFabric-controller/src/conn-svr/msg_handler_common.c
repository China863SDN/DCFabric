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
*   File Name   : msg_handler_common.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-13           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "msg_handler.h"
#include "app_impl.h"
#include "openflow-common.h"
#include "../qos-mgr/qos-mgr.h"

msg_handler_t of_message_handler[1];
msg_handler_t of13_message_handler[OFP13_MAX_MSG];

//app消息处理注册函数
INT4 register_of_msg_hander(gn_switch_t *sw, UINT1 type, msg_handler_t msg_handler)
{
    msg_handler_t *new_msg_handers = NULL;
    if(sw->ofp_version == OFP10_VERSION)
    {
        new_msg_handers = (msg_handler_t *)malloc(sizeof(msg_handler_t) * OFP10_MAX_MSG);
        memcpy(new_msg_handers, of10_message_handler, sizeof(msg_handler_t) * OFP10_MAX_MSG);
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        new_msg_handers = (msg_handler_t *)malloc(sizeof(msg_handler_t) * OFP13_MAX_MSG);
        memcpy(new_msg_handers, of13_message_handler, sizeof(msg_handler_t) * OFP13_MAX_MSG);
    }
    else
    {
        LOG_PROC("ERROR", "OpenFlow verion [%d] is unsupported!", sw->ofp_version);
        return GN_ERR;
    }

    sw->msg_driver.msg_handler = new_msg_handers;
    sw->msg_driver.msg_handler[type] = msg_handler;

    return GN_OK;
}

//app消息处理反注册函数
INT4 unregister_of_msg_hander(gn_switch_t *sw, UINT1 type)
{
    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.msg_handler[type] = of13_message_handler[type];
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.msg_handler[type] = of13_message_handler[type];
    }
    else
    {
        LOG_PROC("[ERROR]", "OpenFlow verion [%d] is unsupported!", sw->ofp_version);
        return GN_ERR;
    }

    return GN_OK;
}

//调用__start_appproc_sec和__stop_appproc_sec之间节内的函数指针app_proc(x)
static void mod_proccalls(gn_switch_t *sw)
{
    proccall_t *p_proc;   //函数指针变量

    p_proc = &__start_appproc_sec;
    do
    {
        (*p_proc)(sw);
        p_proc++;
    } while (p_proc < &__stop_appproc_sec);
}

INT4 of_msg_hello(gn_switch_t *sw, UINT1 *of_msg)
{
    struct ofp_header *header = (struct ofp_header*)of_msg;

    sw->ofp_version = header->version;
    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.type_max = OFP10_MAX_MSG;
        sw->msg_driver.msg_handler = of10_message_handler;
        sw->msg_driver.convertter = &of10_convertter;
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.type_max = OFP13_MAX_MSG;
        sw->msg_driver.msg_handler = of13_message_handler;
        sw->msg_driver.convertter = &of13_convertter;
    }
    else
    {
        LOG_PROC("ERROR", "OpenFlow version unsupported, --version %u", header->version);
        return GN_ERR;
    }

    mod_proccalls(sw);
    return sw->msg_driver.msg_handler[0](sw, of_msg);
}

//初始化默认消息处理
msg_handler_t of_message_handler[1] =
{
    of_msg_hello,       /* OFPT_HELLO */
};



void message_process(gn_switch_t *sw, UINT1 *ofmsg)
{
    struct ofp_header *header = (void *)ofmsg;
//    if(1)
//        printf("@@@@ Recv msg type[%d]\n", header->type);

    sw->msg_driver.msg_handler[header->type](sw, ofmsg);
}

/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2016, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the BNC SDN Controller. BNC SDN
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
*   File Name   : CMsgOperator.h                                              *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMSGOPERATOR_H
#define __CMSGOPERATOR_H

#include "bnc-type.h"
#include "CMsgCommon.h"

typedef enum {
    MSG_OPERATOR_NONE = 0,
    MSG_OPERATOR_PRODUCER,
    MSG_OPERATOR_CONSUMER,

    MSG_OPERATOR_INVALID
}msg_operator_type_e;

/*
 * 消息的操作者（生产者或消费者）基类定义
 */
class CMsgOperator
{
public:
    /*
     * 默认构造函�?
     */
    CMsgOperator();

    /*
     * 带类型参数的构造函�?
     */
    CMsgOperator(INT4 type);

    /*
     * 默认析构函数
     */
    virtual ~CMsgOperator();

    /*
     * 获取操作者类�?
     * @return: INT4    返回操作者类�?
     */
    INT4 getType() {return m_type;}

    /*
     * 设置操作者类�?
     * @param: type     操作者类�?
     * @return: None
     */
    void setType(INT4 type) {m_type = type;}

    /*
     * 基本消息路径注册，供子类调用
     * ret: 成功 or 失败
     */
    INT4 onregister(CMsgPath& path);

    /*
     * 基本消息路径撤销注册，供子类调用
     * @param: path    消息路径
     * ret: None
     */
    void deregister(CMsgPath& path);

    /*
     * 通知消费者有新消息，可能部分消费者需要重�?
     * @param: path    消息路径
     */
    virtual void notify(CMsgPath& path);

    /*
     * 获取操作者名�?
     * ret: 返回操作者名�?
     */
    virtual const char* toString() {return "CMsgOperator";}

private:
    INT4 m_type; //@msg_operator_type_e
};

#endif

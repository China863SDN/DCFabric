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
*   File Name   : CMsg.h                                                      *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMSG_H
#define __CMSG_H

#include "bnc-type.h"
#include "CMsgCommon.h"

enum MsgState_e
{
    MSG_STATE_INIT,

    //...
    MSG_STATE_MAX
};

/*
 * openflow消息封装类
 */
class CMsg : public CMsgCommon
{
public:
    CMsg(INT4 sockfd, INT4 version, INT4 type, INT1* data, INT4 len);
    virtual ~CMsg();

    /*
     * 获取消息状态
     *
     * @return: INT4        返回消息状态
     */
    INT4 getState() {return m_state;}

    /*
     * 设置消息状态
     *
     * @param: state         消息状态
     *
     * @return: None
     */
    void setState(INT4 state) {m_state = state;}

    /*
     * 获取socket连接描述符
     *
     * @return: INT4        返回socket连接描述符
     */
    INT4 getSockfd() {return m_sockfd;}

    /*
     * 获取消息版本
     *
     * @return: INT4        返回消息版本
     */
    INT4 getVersion() {return m_version;}

    /*
     * 获取消息类型
     *
     * @return: INT4        返回消息类型
     */
    INT4 getType() {return m_type;}

    /*
     * 获取消息长度
     *
     * @return: INT4        返回消息长度
     */
    INT4 length() {return m_len;}

    /*
     * 获取消息内容指针
     *
     * @return: INT1*      返回消息内容指针
     */
    INT1* getData() {return m_data;}

    /*
     * 设置消息路径
     *
     * @return: None
     */
    void setPath();

    /*
     * 设置消息关键字
     *
     * @return: None
     */
    void setKey();

private:
    INT4     m_state;    ///< 消息状态
    INT4     m_sockfd;   ///< socket
    INT4     m_version;  ///< 消息版本
    INT4     m_type;     ///< 消息类型
    INT4     m_len;      ///< 消息长度
    INT1*    m_data;     ///< 消息码流
};

#endif

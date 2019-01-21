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
*   File Name   : CPortEvent.h                                                *
*   Author      : bnc jqjiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CPORTEVENT_H
#define __CPORTEVENT_H

#include "bnc-type.h"
#include "CEvent.h"

enum port_event_type_e
{
    PORT_EVENT_NONE,
    PORT_EVENT_ADD,
    PORT_EVENT_MODIFY,
    PORT_EVENT_DELETE,
    
    //...
    PORT_EVENT_MAX,
};

/*
 * 事件封装类
 */
class CPortEvent : public CEvent
{
public:
    CPortEvent();
    CPortEvent(INT4 event, INT4 reason, INT4 type, INT4 sockfd, INT4 dpid, INT4 port_no);
    ~CPortEvent();

    /*
     * 获取事件类型
     *
     * @return: INT4        返回事件类型
     */
    INT4 getType() {return m_type;}

    /*
     * 设置事件类型
     *
     * @param: type         事件类型
     *
     * @return: None
     */
    void setType(INT4 type) {m_type = type;}

    /*
     * 获取端口socket
     *
     * @return: INT4        返回 port id
     */

   INT4 getSockfd() {return m_sockfd;}

    /*
     * 设置端口所属交换机socket
     *
     * @param: sockfd         端口所属交换机socket
     *
     * @return: None
     */
    void setSockfd(INT4 sockfd) {m_sockfd = sockfd;}

    /*
     * 获取端口所属交换机DPID
     *
     * @return: UINT8        返回端口所属交换机DPID
     */
    UINT8 getDpid() {return m_dpid;}

    /*
     * 设置端口所属交换机DPID
     *
     * @param: dpid         端口所属交换机DPID
     *
     * @return: None
     */
    void setDpid(UINT8 dpid)    {m_dpid = dpid;}

    /*
     * 获取端口
     *
     * @return: UINT8        返回端口
     */
    UINT4 getPort() {return m_port_no;}

    /*
     * 设置端口
     *
     * @param: port         端口
     *
     * @return: None
     */
    void setPort(UINT4 port)    {m_port_no = port;}

    /*
     * 获取端口state
     *
     * @return: UINT4        返回端口state
     */
    INT4 getState() {return m_state;}

    /*
     * 设置端口state
     *
     * @param: state         设置端口state
     *
     * @return: UINT4
     */
    void setState(INT4 stateNew)    {m_state = stateNew;}

    /*
     * 获取端口statenew
     *
     * @return: UINT4        返回端口statenew
     */
    INT4 getStateNew() {return m_stateNew;}

    /*
     * 设置端口statenew
     *
     * @param: statenew         设置端口statenew
     *
     * @return: UINT4
     */
    void setStateNew(INT4 stateNew)    {m_stateNew = stateNew;}

private:
	INT4  m_type;     ///< 端口事件类型
    INT4  m_sockfd;   ///< 端口属于交换机socket
    UINT8 m_dpid;     ///< 端口属于交换机DPID
    UINT4 m_port_no;  ///< 端口号
	INT4  m_state;    ///< 端口当前状态
	INT4  m_stateNew; ///< 端口新状态
};

#endif

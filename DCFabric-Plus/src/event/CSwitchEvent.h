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
*   File Name   : CSwitchEvent.h                                              *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CSWITCHEVENT_H
#define __CSWITCHEVENT_H

#include "bnc-type.h"
#include "CEvent.h"
#include "CSwitch.h"

typedef struct
{
    UINT4 ip;
    UINT2 port;
    INT4  sockfd;
    INT4  state;
} switch_dupl_conn_t;

/*
 * 交换机事件封装类
 */
class CSwitchEvent : public CEvent
{
public:
    CSwitchEvent();
    CSwitchEvent(INT4 event, INT4 reason, CSmartPtr<CSwitch>& sw);
    ~CSwitchEvent();

    const INT1* getMac() {return m_mac;}

    /*
     * 获取交换机DPID
     *
     * @return: UINT8        返回交换机DPID
     */
    UINT8 getDpid() {return m_dpid;}

    /*
     * 设置交换机DPID
     *
     * @param: dpid         交换机DPID
     *
     * @return: None
     */
    void setDpid(UINT8 dpid)    {m_dpid = dpid;}

    UINT4 getIp() {return m_ip;}

    UINT2 getPort() {return m_port;}

    /*
     * 获取交换机当前socket
     *
     * @return: INT4        返回交换机当前socket
     */
    INT4 getSockfd() {return m_sockfd;}

    /*
     * 设置交换机当前socket
     *
     * @param: sockfd         交换机当前socket
     *
     * @return: None
     */
    void setSockfd(INT4 sockfd) {m_sockfd = sockfd;}

    /*
     * 获取交换机之前连接信息
     *
     * @return: switch_dupl_conn_t        返回交换机之前连接信息
     */
    const switch_dupl_conn_t& getDuplConn() {return m_duplConn;}

    /*
     * 设置交换机之前连接信息
     *
     * @param: duplConn         交换机之前连接信息
     *
     * @return: None
     */
    void setDuplConn(const switch_dupl_conn_t& duplConn) {m_duplConn = duplConn;}

private:
    INT1               m_mac[MAC_LEN]; ///< 交换机MAC
    UINT8              m_dpid;         ///< 交换机DPID
    UINT4              m_ip;           ///< 交换机在当前连接使用的IP
    UINT2              m_port;         ///< 交换机在当前连接使用的PORT
    INT4               m_sockfd;       ///< 交换机当前连接socketfd
    switch_dupl_conn_t m_duplConn;     ///< 交换机之前连接信息
};

#endif

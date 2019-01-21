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
*   File Name   : CLinkEvent.h                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CLINKEVENT_H
#define __CLINKEVENT_H

#include "bnc-type.h"
#include "bnc-param.h"
#include "CEvent.h"

/*
 * 拓扑边事件封装类
 */
class CLinkEvent : public CEvent
{
public:
    CLinkEvent();
    CLinkEvent(INT4 event, INT4 reason, UINT8 dpid, UINT4 port, UINT8 neighDpid, UINT4 neighPort);
    ~CLinkEvent();

    /*
     * 获取交换机DPID
     *
     * @return: UINT8        返回交换机DPID
     */
    UINT8 getSrcDpid() {return m_srcDpid;}

    /*
     * 设置交换机DPID
     *
     * @param: dpid         交换机DPID
     *
     * @return: None
     */
    void setSrcDpid(UINT8 dpid)    {m_srcDpid = dpid;}

    /*
     * 获取交换机端口号
     *
     * @return: UINT4        返回交换机端口号
     */
    UINT4 getSrcPort() {return m_srcPort;}

    /*
     * 设置交换机端口号
     *
     * @param: port         交换机端口号
     *
     * @return: None
     */
    void setSrcPort(UINT4 port)    {m_srcPort = port;}

    /*
     * 获取邻居交换机DPID
     *
     * @return: UINT8        返回交换机DPID
     */
    UINT8 getDstDpid() {return m_dstDpid;}

    /*
     * 设置邻居交换机DPID
     *
     * @param: dpid         交换机DPID
     *
     * @return: None
     */
    void setDstDpid(UINT8 dpid)    {m_dstDpid = dpid;}

    /*
     * 获取邻居交换机端口号
     *
     * @return: UINT4        返回交换机端口号
     */
    UINT4 getDstPort() {return m_dstPort;}

    /*
     * 设置邻居交换机端口号
     *
     * @param: port         交换机端口号
     *
     * @return: None
     */
    void setDstPort(UINT4 port)    {m_dstPort = port;}

private:
    UINT8 m_srcDpid;
    UINT4 m_srcPort;
    UINT8 m_dstDpid;
    UINT4 m_dstPort;
};

#endif

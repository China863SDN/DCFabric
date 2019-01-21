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
*   File Name   : CConnectMgr.h     *
*   Author      : bnc xflu          *
*   Create Date : 2016-9-7          *
*   Version     : 1.0               *
*   Function    : .                 *
*                                                                             *
******************************************************************************/
#ifndef _CCONNECTMGR_H
#define _CONNECTMGR_H

#include "CConnect.h"
#include <list>

/*
 * 用来管理主机之间的连接
 * 可能会废弃不使用
 */
class CConnectMgr
{
public:
    /*
     * 默认析构函数
     */
    ~CConnectMgr();

    /*
     * 获取实例
     */
    static CConnectMgr* getInstance();

    /*
     * 添加连接
     */
    void addConnect();

    /*
     * 删除连接
     */
    void deleteConnect();

private:
    /*
     * 默认构造函数
     */
    CConnectMgr();

    /*
     * 初始化
     */
    void init();

    static CConnectMgr* m_pInstance;        ///< 静态实例成员
    std::list<CConnect> m_connectList;      ///< 连接列表

};



#endif

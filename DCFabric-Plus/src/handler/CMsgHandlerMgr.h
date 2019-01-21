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
*   File Name   : CMsgHandlerMgr.h                                            *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMSGHANDLERMGR_H
#define __CMSGHANDLERMGR_H

#include "bnc-type.h"

class CMsgHandler;

/*
 * handler管理类
 */
class CMsgHandlerMgr
{
public:
    /*
     * 默认构造函数
     */
    CMsgHandlerMgr();

    /*
     * 默认析构函数
     */
    ~CMsgHandlerMgr();

    /*
     * 初始化
     *
     * @return: 成功 or 失败
     */
    INT4 init();
    
private:
    std::vector<CMsgHandler*> m_handlers;
};


#endif

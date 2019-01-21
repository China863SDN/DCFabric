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
*   File Name   : CLoopBuffer.h                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CLOOPBUFFER_H
#define __CLOOPBUFFER_H

#include "bnc-type.h"
#include "CMutex.h"
#include "CRefObj.h"

/*
 * 循环缓冲区类
 */
class CLoopBuffer : public CRefObj
{
public:
    /*
     * 默认构造函数
     */
    CLoopBuffer(BOOL isThreadSafe=FALSE);

    /*
     * 带参数的构造函数
     * @param size: 指定的buffer大小
     */
    CLoopBuffer(UINT4 size, BOOL isThreadSafe=FALSE);

    /*
     * 默认析构函数
     */
    ~CLoopBuffer();

    /*
     * 向缓冲区写入数据
     * @param data: 指向数据的指针
     * @param len: 数据长度
     * ret: 成功 or 失败
     */
    BOOL write(const INT1* data, UINT4 len);

    /*
     * 从缓冲区读出数据
     * @param buffer: 存储数据的指针
     * @param len: 需读出数据的长度
     * @param peek: 是否需要移动指针
     * ret: 成功 or 失败
     */
    BOOL read(INT1* buffer, UINT4 len, BOOL peek=FALSE);

    /*
     * 返回缓冲区中数据的长度
     */
    UINT4 length() {return m_curLen;}
    
    /*
     * 清空缓存
     */
    void reset();

    /*
     * 清除部分数据
     */
    void clear(UINT4 len);

private:
    BOOL   m_isThreadSafe;
    INT1*  m_buffer;
    UINT4  m_totalLen;
    UINT4  m_curLen;
    INT1*  m_head;
    CMutex m_mutex;
};

#endif

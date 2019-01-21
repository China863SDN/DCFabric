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
*   File Name   : CBuffer.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef __CBUFFER_H
#define __CBUFFER_H

#include "bnc-type.h"
#include "CRefObj.h"
#include "CMutex.h"

/*
 * 缓冲区类
 */
class CBuffer : public CRefObj
{
public:
    CBuffer();
    CBuffer(UINT4 size);
    ~CBuffer();

    /*
     * 向缓冲区写入数据
     * param data: 指向数据的指针
     * param len: 数据长度
     * ret: 实际写入数据的长度
     */
    UINT4 write(const INT1* data, UINT4 len);

    /*
     * 从缓冲区读出数据
     * @param buffer: 存储数据的指针
     * @param len: 需读出数据的长度
     * ret: 实际读出数据的长度
     */
    UINT4 read(INT1* buffer, UINT4 len);

    /*
     * 返回缓冲区中数据的长度
     */
    UINT4 length() {return m_len;}

    /*
     * 获取缓冲区指针
     * ret: 返回缓冲区指针
     */
    INT1* getData() {return m_buffer;}

    /*
     * 返回缓冲区是否为空
     */
    BOOL empty() {return (0 == m_len);}

    /*
     * 清空缓存
     */
    void clear() {m_len = 0;}
    
private:
    INT1*  m_buffer;
    UINT4  m_size;
    UINT4  m_len;
    CMutex m_mutex;
};


#endif

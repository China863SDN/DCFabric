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
*   File Name   : COvsdbRecvWorker.h                                          *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __COVSDBRECVWORKER__H
#define __COVSDBRECVWORKER__H

#include "CRecvWorker.h"
#include "CLFHashMap.h"
#include "CBuffer.h"
#include "CSmartPtr.h"

typedef CUint32LFHashMap<CSmartPtr<CBuffer> > CBufferHMap;

/**
 * OVSDB worker类
 * 负责接收来自OVSDB监听端口的消息
 */
class COvsdbRecvWorker : public CRecvWorker
{
public:
	/*
	 * 默认构造函数
	 */
    COvsdbRecvWorker();

    /*
     * 默认析构函数
     */
    virtual ~COvsdbRecvWorker();

    /*
     * 增加连接
     *
     * @param: fd                   socket描述符
     *
     * @return: INT4                成功 or 失败
     */
    virtual INT4 addConnFd(INT4 fd);

    /*
     * 处理收到消息
     * 继承自基类
     *
     * @param: sockfd           socket描述符
     * @param: buffer           消息指针
     * @param: len              消息长度
     *
     * @return: None
     */
    virtual INT4 process(INT4 sockfd, INT1* buffer, UINT4 len);

    /*
     * 处理交换机断开连接
     * 继承自基类
     *
     * @param: sockfd           socket描述符
     *
     * @return: None
     */
    virtual void processPeerDisconn(INT4 sockfd);

    /*
     * 获取RecvWorker名称
     * ret: 返回RecvWorker名称
     */
    virtual const char* toString() {return "COvsdbRecvWorker";}

private:
    /*
     * 根据sockfd获取对应的CBuffer
     *
     * @param: sockfd         交换机连接sockfd
     *
     * @return: CSmartPtr<CBuffer>
     */
    CSmartPtr<CBuffer> getBuffer(INT4 sockfd);

    /*
     * 根据sockfd创建对应的CBuffer
     *
     * @param: sockfd         交换机连接sockfd
     *
     * @return: CSmartPtr<CBuffer>
     */
    CSmartPtr<CBuffer> createBuffer(INT4 sockfd);

    /*
     * 根据sockfd删除对应的CBuffer
     *
     * @param: sockfd         交换机连接sockfd
     *
     * @return: None
     */
    void delBuffer(INT4 sockfd);

private:
    static const UINT4 segment_buffer_size = 204800;
    static const UINT4 hash_bucket_number  = 10240;
    
    CBufferHMap m_buffers;
};

#endif

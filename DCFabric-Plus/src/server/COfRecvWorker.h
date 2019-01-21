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
*   File Name   : COfRecvWorker.h                                             *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __COFRECVWORKER__H
#define __COFRECVWORKER__H

#include "CRecvWorker.h"
#include "CMemPool.h"
#include "CLoopBuffer.h"
#include "openflow-common.h"
#include "CMsg.h"

/*
 * Openflow worker类
 * 负责接收来自Openflow监听端口的请求
 * 并将封装消息入队列
 */
class COfRecvWorker : public CRecvWorker
{
public:
    /*
     * 默认构造函数
     */
    COfRecvWorker();

    /*
     * 默认析构函数
     */
    virtual ~COfRecvWorker();

    /*
     * 初始化
     *
     * @return:  成功 or 失败
     */
    virtual INT4 init();

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
     * @return: 成功 or 失败
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
     * 当交换机断开连接时，处理本地清理工作
     *
     * @param: sockfd           socket描述符
     *
     * @return: None
     */
    void processConnClose(INT4 sockfd);

    /*
     * 获取RecvWorker名称
     * ret: 返回RecvWorker名称
     */
    virtual const char* toString() {return "COfRecvWorker";}
    
    /*
     * 获取recvWorker的内存池，用于释放内存
     *
     * @return: CMemPool&
     */
    CMemPool& getMemPool() {return m_memPool;}

private:
    /*
     * 根据sockfd获取对应的recvBuffer
     *
     * @param: sockfd         交换机连接sockfd
     *
     * @return: CSmartPtr<CLoopBuffer>
     */
    CSmartPtr<CLoopBuffer> getRecvBuffer(INT4 sockfd);

    /*
     * 根据sockfd创建对应的recvBuffer
     *
     * @param: sockfd         交换机连接sockfd
     *
     * @return: CSmartPtr<CLoopBuffer>
     */
    CSmartPtr<CLoopBuffer> createRecvBuffer(INT4 sockfd);

    /*
     * 根据sockfd删除对应的recvBuffer
     *
     * @param: sockfd         交换机连接sockfd
     *
     * @return: None
     */
    void delRecvBuffer(INT4 sockfd);

    /*
     * 收到消息后处理
     *
     * @param: sockfd         交换机连接sockfd
     * @param: buffer         消息内容
     * @param: len            消息长度
     * @param: recvBuffer     分段消息写入recvBuffer
     *
     * @return: None
     */
    INT4 procRecvBuffer(INT4 sockfd, INT1* buffer, UINT4 len, CSmartPtr<CLoopBuffer>& recvBuffer);

private:
    //static const UINT4 hash_bucket_number = 10240;
    static const UINT4 recv_buffer_size = 65536;
    
    //CUint32HashMap<CSmartPtr<CLoopBuffer> > m_recvBuffers;
    CSmartPtr<CLoopBuffer> m_recvBuffer[65536];
    CMemPool m_memPool;
};

#endif

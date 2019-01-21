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
*   File Name   : CServer.h                                                   *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CSERVER_H
#define __CSERVER_H

#include "bnc-type.h"
#include "CMsgHandlerMgr.h"
#include "CMsgTree.h"
#include "CSwitchMgr.h"
#include "CTopoMgr.h"
#include "CTagMgr.h"
#include "COfRecvWorker.h"
#include "CHttpRecvWorker.h"
#include "COvsdbRecvWorker.h"
#include "COfTcpListener.h"
#include "CHttpTcpListener.h"
#include "COvsdbTcpListener.h"

/*
 * 服务器类
 *     管理服务的公共资源
 */
class CServer
{
public:
    /*
     * 获取CServer实例
     *
     * @return: CServer*            CServer实例
     */
    static CServer* getInstance();

    /*
     * 初始化
     *
     * @return: INT4        成功 or 失败
     */
    INT4 init();

    /*
     * 根据sockfd获取一个openflow RecvWorker
     *
     * @return: CSmartPtr<COfRecvWorker>
     */
    CSmartPtr<COfRecvWorker> mapOfRecvWorker(INT4 sockfd);

    /*
     * 根据sockfd获取一个http RecvWorker
     *
     * @return: CSmartPtr<CHttpRecvWorker>
     */
    CSmartPtr<CHttpRecvWorker> mapHttpRecvWorker(INT4 sockfd);

    /*
     * 根据sockfd获取一个ovsdb RecvWorker
     *
     * @return: CSmartPtr<COvsdbRecvWorker>
     */
    CSmartPtr<COvsdbRecvWorker> mapOvsdbRecvWorker(INT4 sockfd);
    
    std::string& getControllerEth() {return m_controllerEth;}

	/*
     * 获取控制器管理MAC
     *
     * @return: 控制器管理MAC
     */
    UINT1* getControllerMac() {return m_controllerMac;}
		
    /*
     * 获取控制器管理IP
     *
     * @return: 控制器管理IP
     */
    UINT4 getControllerIp() {return m_controllerIp;}

    /*
     * 获取控制器OpenFlow端口
     *
     * @return: 控制器OpenFlow端口
     */
    UINT4 getOfPort() {return m_ofPort;}

private:
    /*
     * 默认构造函数
     */
    CServer();

    /*
     * 默认析构函数
     */
    ~CServer();

private:
    //CServer实例
    static CServer* m_instance;       
    
    //控制器管理ETH+MAC+IP
    std::string m_controllerEth;
	UINT1 m_controllerMac[MAC_LEN];
    UINT4 m_controllerIp;

    //控制器OpenFlow PORT
    UINT2 m_ofPort;
    
    //openflow连接监听线程
    CSmartPtr<CTcpListener> m_ofListener;

    //openflow消息接收worker列表
    INT4 m_ofRecvWorkerCount;
    std::vector<CSmartPtr<COfRecvWorker> > m_ofRecvWorkers;

    //控制器HTTP PORT
    UINT2 m_httpPort;
    
    //http连接监听线程
    CSmartPtr<CTcpListener> m_httpListener;

    //http消息接收worker列表
    std::vector<CSmartPtr<CHttpRecvWorker> > m_httpRecvWorkers;

    //控制器OVSDB PORT
    UINT2 m_ovsdbPort;
    
    //OVSDB连接监听线程
    CSmartPtr<CTcpListener> m_ovsdbListener;

    //OVSDB消息接收worker列表
    std::vector<CSmartPtr<COvsdbRecvWorker> > m_ovsdbRecvWorkers;
};

#endif

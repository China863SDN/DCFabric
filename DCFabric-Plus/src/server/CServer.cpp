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
*   File Name   : CServer.cpp           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-7-1           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "CServer.h"
#include "log.h"
#include "bnc-error.h"
#include "CConf.h"
#include "comm-util.h"
#include "CCpuCtrl.h"

#define OF_CONTROL_PORT     6633
#define HTTP_SERVER_PORT    8081
#define OVSDB_SERVER_PORT   6640

CServer* CServer::m_instance = NULL;

CServer* CServer::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CServer();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return (m_instance);
}

CServer::CServer():m_controllerIp(0),m_ofPort(0),m_ofRecvWorkerCount(0),m_httpPort(0),m_ovsdbPort(0)
{
    memset(m_controllerMac, 0, MAC_LEN);
}

CServer::~CServer()
{
}

INT4 CServer::init()
{
    UINT4 recvNum = CConf::getInstance()->getRecvThreadNumber();
    LOG_INFO_FMT("recv_thread_num %u", recvNum);

    if (0 == recvNum)
        recvNum = 1;

    INT4 quota = CCpuCtrl::getInstance()->getQuota("COfRecvWorker");
    if ((quota > 0) && (quota < (INT4)recvNum))
        recvNum = quota;

    for (UINT4 i = 0; i < recvNum; i++)
    {
        COfRecvWorker* worker = new COfRecvWorker();
        if (NULL == worker)
        {
            LOG_ERROR("new COfRecvWorker failed!");
            return BNC_ERR;
        }
        if (worker->init() != BNC_OK)
        {
            LOG_ERROR("init COfRecvWorker failed!");
            delete worker;
            return BNC_ERR;
        }
        m_ofRecvWorkers.push_back(CSmartPtr<COfRecvWorker>(worker));
        ++ m_ofRecvWorkerCount;
    }

    const INT1* pEth = CConf::getInstance()->getConfig("controller", "controller_eth");
    m_controllerEth = (NULL == pEth) ? "eth0" : pEth;
	if (::getControllerMac(m_controllerEth.c_str(), m_controllerMac) != BNC_OK)
    {
        LOG_WARN_FMT("getControllerMac by %s failed!", m_controllerEth.c_str());
        return BNC_ERR;
    }
    if (::getControllerIp(m_controllerEth.c_str(), &m_controllerIp) != BNC_OK)
    {
        LOG_WARN_FMT("getControllerIp by %s failed!", m_controllerEth.c_str());
        return BNC_ERR;
    }
    LOG_INFO_FMT("controller_eth %s, ip %x", m_controllerEth.c_str(), m_controllerIp);

    const INT1* pPort = CConf::getInstance()->getConfig("controller", "openflow_server_port");
    m_ofPort = (NULL == pPort) ? OF_CONTROL_PORT : atoi(pPort);
    LOG_INFO_FMT("openflow_server_port %u", m_ofPort);

    CTcpListener* tcpLister = new COfTcpListener(m_controllerIp, m_ofPort);
    if (NULL == tcpLister)
    {
        LOG_ERROR("new COfTcpListener failed!");
        return BNC_ERR;
    }
    if (tcpLister->init() != BNC_OK)
    {
        LOG_ERROR("init COfTcpListener failed!");
        delete tcpLister;
        return BNC_ERR;
    }
    m_ofListener = CSmartPtr<CTcpListener>(tcpLister);

    //for (UINT4 i = 0; i < recvNum; i++)
    {
        CHttpRecvWorker* worker = new CHttpRecvWorker();
        if (NULL == worker)
        {
            LOG_ERROR("new CHttpRecvWorker failed!");
            return BNC_ERR;
        }
        if (worker->init() != BNC_OK)
        {
            LOG_ERROR("init CHttpRecvWorker failed!");
            delete worker;
            return BNC_ERR;
        }
        m_httpRecvWorkers.push_back(CSmartPtr<CHttpRecvWorker>(worker));
    }

    pPort = CConf::getInstance()->getConfig("restful_conf", "http_server_port");
    m_httpPort = (NULL == pPort) ? HTTP_SERVER_PORT : atoi(pPort);
    LOG_INFO_FMT("http_server_port %u", m_httpPort);

    tcpLister = new CHttpTcpListener(m_controllerIp, m_httpPort);
    if (NULL == tcpLister)
    {
        LOG_ERROR("new CHttpTcpListener failed!");
        return BNC_ERR;
    }
    if (tcpLister->init() != BNC_OK)
    {
        LOG_ERROR("init CHttpTcpListener failed!");
        return BNC_ERR;
    }
    m_httpListener = CSmartPtr<CTcpListener>(tcpLister);

    //for (UINT4 i = 0; i < recvNum; i++)
    {
        COvsdbRecvWorker* worker = new COvsdbRecvWorker();
        if (NULL == worker)
        {
            LOG_ERROR("new COvsdbRecvWorker failed!");
            return BNC_ERR;
        }
        if (worker->init() != BNC_OK)
        {
            LOG_ERROR("init COvsdbRecvWorker failed!");
            delete worker;
            return BNC_ERR;
        }
        m_ovsdbRecvWorkers.push_back(CSmartPtr<COvsdbRecvWorker>(worker));
    }

    pPort = CConf::getInstance()->getConfig("ovsdb_conf", "ovsdb_port");
    m_ovsdbPort = (NULL == pPort) ? OVSDB_SERVER_PORT : atoi(pPort);
    LOG_INFO_FMT("ovsdb_port %u", m_ovsdbPort);

    tcpLister = new COvsdbTcpListener(m_controllerIp, m_ovsdbPort);
    if (NULL == tcpLister)
    {
        LOG_ERROR("new COvsdbTcpListener failed!");
        return BNC_ERR;
    }
    if (tcpLister->init() != BNC_OK)
    {
        LOG_ERROR("init COvsdbTcpListener failed!");
        return BNC_ERR;
    }
    m_ovsdbListener = CSmartPtr<CTcpListener>(tcpLister);

    LOG_INFO("init CServer success");
    return BNC_OK;
}

CSmartPtr<COfRecvWorker> CServer::mapOfRecvWorker(INT4 sockfd)
{
    //处理此交换机消息的COfRecvWorker为第sockfd%num�?
    return m_ofRecvWorkers[sockfd % m_ofRecvWorkerCount];
}

CSmartPtr<CHttpRecvWorker> CServer::mapHttpRecvWorker(INT4 sockfd)
{
    //处理此交换机消息的CHttpRecvWorker为第sockfd%num�?
    if (m_httpRecvWorkers.empty())
    {
        LOG_WARN("none CHttpRecvWorker!");
        return CSmartPtr<CHttpRecvWorker>(NULL);
    }
    
    INT4 index = sockfd % m_httpRecvWorkers.size();
    if (index >= (INT4)CConf::getInstance()->getRecvThreadNumber())
    {
        LOG_WARN_FMT("invalid CHttpRecvWorker index[%d] !", index);
        return CSmartPtr<CHttpRecvWorker>(NULL);
    }

    return m_httpRecvWorkers[index];
}

CSmartPtr<COvsdbRecvWorker> CServer::mapOvsdbRecvWorker(INT4 sockfd)
{
    //处理此交换机消息的COvsdbRecvWorker为第sockfd%num�?
    if (m_ovsdbRecvWorkers.empty())
    {
        LOG_WARN("none COvsdbRecvWorker!");
        return CSmartPtr<COvsdbRecvWorker>(NULL);
    }
    
    INT4 index = sockfd % m_ovsdbRecvWorkers.size();
    if (index >= (INT4)CConf::getInstance()->getRecvThreadNumber())
    {
        LOG_WARN_FMT("invalid COvsdbRecvWorker index[%d] !", index);
        return CSmartPtr<COvsdbRecvWorker>(NULL);
    }

    return m_ovsdbRecvWorkers[index];
}


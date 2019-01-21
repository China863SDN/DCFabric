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
*   File Name   : CClusterService.h                                           *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CCLUSTERSERVICE_H
#define __CCLUSTERSERVICE_H

#include "bnc-type.h"
#include "CClusterDefine.h"
#include "CClusterTcpListener.h"
#include "CClusterRecvWorker.h"
#include "openflow-common.h"

class CClusterService
{
public:
    static CClusterService* getInstance();

    ~CClusterService();

    INT4 init();
    void clear();

    INT4 connectRemote(UINT4 ip);
    INT4 remoteConnect(INT4 sockfd, UINT4 ip, UINT2 port);
    void remoteDisconnect(INT4 sockfd);

    std::vector<cluster_controller_t>& getControllers() {return m_controllers;}
    INT4 getControllerRole() {return m_controllerRole;}
    INT4 updateRole(INT4 role);

    BOOL isClusterOn() {return m_clusterOn;}
    const char* toString() {return "CClusterService";}

private:
    CClusterService();

    INT4 initVirtualIp();
    INT4 initServers();
    INT4 initControllers();
    void connectRemotes();
    INT4 createRemote(INT4 sockfd, UINT4 ip, UINT2 port);
    void updateConfig();
    INT4 sendGratuitousArp();

private:
    static CClusterService* m_instance;

    BOOL  m_clusterOn; 
    INT4  m_controllerRole;
    UINT4 m_virtualIp;
    UINT4 m_ip;
    UINT2 m_port;
    CClusterTcpListener m_listener;
    CClusterRecvWorker  m_recvWorker;
    std::vector<cluster_controller_t> m_controllers;
    CMutex m_mutex;
};

#endif /* __CCLUSTERSERVICE_H */

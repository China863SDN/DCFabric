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
*   File Name   : CClusterMgr.h                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CCLUSTERMGR_H
#define __CCLUSTERMGR_H

#include "bnc-type.h"
#include "zookeeper.h"
#include "openflow-common.h"

#define MAX_CONTROLLER_COUNT    8

/*
 * 集群管理�?
 */
class CClusterMgr
{
public:
    /*
     * 获取CClusterMgr实例
     *
     * @return: CClusterMgr*   CClusterMgr实例
     */
    static CClusterMgr* getInstance();

    /*
     * 默认析构函数
     */
    ~CClusterMgr();

    /*
     * 初始�?
     *
     * @return: 成功 or 失败
     */
    INT4 init();

    void setDebugLevel();
    INT4 connect();
    void disconnect();
    INT4 createRoot();
    INT4 createNode();
    INT4 createData();
    INT4 createElectionGenerationId();
    INT4 createMasterId();
    INT4 createEphemeralNode();
    void deleteEphemeralNode();

    void getElectionGenerationId();
    INT4 updateElectionGenerationId(UINT8 id);
    void getMasterId();
    INT4 updateMasterId(UINT8 id);
    UINT8 zkGetMasterId();

    INT4 watchNodes();
    INT4 watchMaster();

    BOOL isMaster() {return (m_clusterId == m_masterId);}
    INT4 getConnectState();
    void checkElectionFinished();

    INT4 getControllerRole() {return m_controllerRole;}
    void setControllers(INT4 count, INT1** nodes);
    INT4 updateRole(INT4 role);
    void updateControllerInfo(INT4 count, INT1** nodes);

    INT4 getClusterState() {return m_clusterState;}
    void setClusterState(INT4 state) {m_clusterState = state;}
    UINT8 getClusterId() {return m_clusterId;}

    BOOL isClusterOn() {return m_clusterOn;}

private:
    /*
     * 默认构造函�?
     */
    CClusterMgr();

private:
    static CClusterMgr* m_instance;      

    BOOL       m_clusterOn; 
    INT4       m_clusterState;
    UINT8      m_clusterId; //控制器id，被初始化成本控制器的IP地址后便不再改变
    UINT8      m_electionGenerationId; //master选举版本�?
    UINT8      m_masterId; //当前master的id
    INT1       m_zookeeperServer[300]; //zookeeper服务器地址  ip:port,ip:port,ip:port,ip:port
    INT4       m_controllerRole; //当前控制器角�?
    INT4       m_controllerCount; //当前集群中控制器�?
    INT1       m_controllers[MAX_CONTROLLER_COUNT][30];
    zhandle_t* m_zkhandler;
};

#endif

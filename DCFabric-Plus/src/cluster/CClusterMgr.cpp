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
*   File Name   : CClusterMgr.cpp                                             *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-param.h"
#include "bnc-error.h"
#include "log.h"
#include "CClusterMgr.h"
#include "openflow-common.h"
#include "CConf.h"
#include "comm-util.h"
#include "CSyncMgr.h"
#include "CControl.h"
#include "COfMsgUtil.h"

const INT1* SDN_ROOT_PATH 		= "/sdn_root";
const INT1* SDN_NODE_PATH 		= "/sdn_root/sdn_node";
const INT1* SDN_DATA_PATH 		= "/sdn_root/sdn_data";
const INT1* SDN_ELEC_PATH 		= "/sdn_root/sdn_data/election_generation_id";
const INT1* SDN_MASTER_PATH 	= "/sdn_root/sdn_data/master_id";
const INT1* SDN_TOPO_VER_PATH 	= "/sdn_root/sdn_data/topology_version_id";

enum CLUSTER_STATE
{
	CLUSTER_STOP  = 0,
	CLUSTER_SETUP = 1
};

//zookeeper_init的回调函�?
static void wt_zookeeper_init(zhandle_t* zh, INT4 type, INT4 state, const INT1* path, void* watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            LOG_WARN("Connected to zookeeper server successfully");
        }
        else if (state == ZOO_EXPIRED_SESSION_STATE)
        {
            LOG_WARN("Zookeeper session expired !");
        }
    }
}

//zoo_acreate,zoo_aset的回调函�?
static void zookeeper_error_msg(INT4 rc, const INT1* opr_name)
{
    //ZOK 操作完成
    //ZNONODE 父节点不存在
    //ZNODEEXISTS 节点已存�?
    //ZNOAUTH 客户端没有权限创建节�?
    //ZNOCHILDRENFOREPHEMERALS 临时节点不能创建子节点�?
    switch(rc)
    {
    case ZOK:
        LOG_WARN_FMT("%s succeed", opr_name);
        break;
    case ZNONODE:
        LOG_WARN_FMT("%s failed, parent node not exist", opr_name);
        break;
    case ZNOAUTH:
        LOG_WARN_FMT("%s failed, no permission to create", opr_name);
        break;
    case ZNOCHILDRENFOREPHEMERALS:
        LOG_WARN_FMT("%s failed, parent node is ephemeral", opr_name);
        break;
    case ZOPERATIONTIMEOUT:
        LOG_WARN_FMT("%s failed, operate timeout", opr_name);
        break;
    case ZCONNECTIONLOSS:
        LOG_WARN_FMT("%s failed, connection loss", opr_name);
        break;
    case ZNODEEXISTS:
        LOG_WARN_FMT("%s failed, node already exist", opr_name);
        break;
    default:
        LOG_WARN_FMT("%s failed. ret_code: %d", opr_name, rc);
    }
}

//zoo_acreate的回调函�?
static void cb_string_completion(INT4 rc, const INT1 *value, const void *data)
{
    zookeeper_error_msg(rc, (const INT1*)data);
}

/*
//zoo_aset的回调函�?
static void cb_stat_completion(int rc, const struct Stat *stat, const void *data)
{
    zookeeper_error_msg(rc, (const INT1*)data);
}
*/

//zoo_aget_children2的回调函�?
static void cb_get_children2(int rc, const struct String_vector *strings, const struct Stat *stat, const void *data)
{
    if ((ZOK == rc) && (NULL != strings) && (strings->count >= 1) && CClusterMgr::getInstance()->isMaster())
        *((INT1*)data) = 1;
    else
        *((INT1*)data) = 0;
}

//观察者函数，当被观察对象发生改变，客户端会收到通知
static void wt_get_children2(zhandle_t *zh, INT4 type, INT4 state, const INT1 *path, void *watcherCtx)
{
    //LOG_WARN_FMT("wt_get_children2 type[%d]state[%d]", type, state);
    if (ZOO_CONNECTED_STATE == state)
    {
        if (ZOO_CHILD_EVENT == type)
        {
            if (CClusterMgr::getInstance()->watchNodes() != BNC_OK)
            {
                LOG_WARN("Start watch children failed !");
                return;
            }
        }
        else if (ZOO_DELETED_EVENT == type)
        {
            LOG_INFO_FMT("Path %s was deleted", path);
        }
        else if (ZOO_CREATED_EVENT == type)
        {
            LOG_INFO_FMT("Path %s was created", path);
        }
    }
	else if (ZOO_EXPIRED_SESSION_STATE == state)
	{
		if (ZOO_SESSION_EVENT == type)
		{
			CClusterMgr::getInstance()->disconnect();

			//连接zookeeper服务�?
            if (CClusterMgr::getInstance()->connect() != BNC_OK)
            {
                LOG_WARN("Connect to zookeeper server failed !");
                return;
            }
			
			//删除"/sdn_root/sdn_node"中旧临时节点
            CClusterMgr::getInstance()->deleteEphemeralNode();
			
            //判断是否选举完毕，若选举完毕，更新master_id 
            CClusterMgr::getInstance()->checkElectionFinished();
			
            if (CClusterMgr::getInstance()->watchNodes() != BNC_OK)
            {
                LOG_WARN("Start watch children failed !");
                return;
            }

#if 0
            if (CClusterMgr::getInstance()->watchMaster() != BNC_OK)
            {
                LOG_WARN("Start watch master failed !");
                return;
            }
#endif

            if (CClusterMgr::getInstance()->createEphemeralNode() != BNC_OK)
            {
                LOG_WARN("Create sdn local node failed !");
            }
		}
	}
}

//回调函数，此处也会切换主�?
static void cb_awget_children2(INT4 rc, const struct String_vector *strings, const struct Stat *stat, const void *data)
{
    //LOG_WARN_FMT("cb_awget_children2 rc[%d]", rc);
    if ((ZOK == rc) && (NULL != strings))
    {
        CClusterMgr::getInstance()->setControllers(strings->count, strings->data);
    }

    zookeeper_error_msg(rc, (const INT1*)data);
}

//观察者函数，当被观察对象发生改变，客户端会收到通知
static void wt_get_master_id(zhandle_t *zh, INT4 type, INT4 state, const INT1 *path, void *watcherCtx)
{
    //LOG_WARN_FMT("wt_get_master_id type[%d]state[%d]", type, state);
    if (ZOO_CONNECTED_STATE == state)
    {
        if (ZOO_CHANGED_EVENT == type)
        {
            LOG_WARN_FMT("wt_get_master_id type[%d]state[%d]", type, state);
            if (CClusterMgr::getInstance()->watchMaster() != BNC_OK)
            {
                LOG_WARN("Start watch master failed !");
                return;
            }
        }
        else if (ZOO_DELETED_EVENT == type)
        {
            LOG_WARN_FMT("Path %s was deleted", path);

        }
        else if (ZOO_CREATED_EVENT == type)
        {
            LOG_WARN_FMT("Path %s was created", path);
        }
        else
        {
            LOG_WARN_FMT("Node %s watch failed", path);
        }
    }
}

//回调函数，主从切换在此执�?
static void cb_awget_master_id (int rc, const char *value, int value_len,const struct Stat *stat, const void *data)
{
    //LOG_WARN_FMT("cb_awget_master_id rc[%d]", rc);
    if (ZOK == rc)
	{
		UINT8 nodeId = CClusterMgr::getInstance()->zkGetMasterId();
        LOG_WARN_FMT("cb_awget_master_id nodeId[%llu][%s]", nodeId, inet_htoa(nodeId));
		if (0 == nodeId)
		{
			LOG_WARN("Master id changed but read from zoo failed !");
			return;
		}
		
		if ((UINT8)-2 != nodeId)
		{
			CClusterMgr::getInstance()->setClusterState(CLUSTER_SETUP);
            if (CClusterMgr::getInstance()->getClusterId() == nodeId)
                CClusterMgr::getInstance()->updateRole(OFPCR_ROLE_MASTER);
            else
                CClusterMgr::getInstance()->updateRole(OFPCR_ROLE_SLAVE);
		}
		else
		{
			CClusterMgr::getInstance()->setClusterState(CLUSTER_STOP);
			CClusterMgr::getInstance()->updateRole(OFPCR_ROLE_EQUAL);
		}
	}

    zookeeper_error_msg(rc, (const INT1*)data);
}

CClusterMgr* CClusterMgr::m_instance = NULL;

CClusterMgr* CClusterMgr::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CClusterMgr();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return m_instance;
}

CClusterMgr::CClusterMgr()
{
    m_clusterOn = FALSE;
    m_clusterState = CLUSTER_STOP;
    m_clusterId = 0;
    m_electionGenerationId = 0;
    m_masterId = 0;
    m_zookeeperServer[0] = 0;
    m_controllerRole = OFPCR_ROLE_EQUAL;
    m_controllerCount = 0;
    memset(m_controllers, 0, MAX_CONTROLLER_COUNT*30);
    m_zkhandler = NULL;
}

CClusterMgr::~CClusterMgr()
{
    if (NULL != m_zkhandler)
    {
        zookeeper_close(m_zkhandler);
        m_zkhandler = NULL;
    }
}

INT4 CClusterMgr::init()
{
    const INT1* pConf = CConf::getInstance()->getConfig("cluster_conf", "cluster_on");
    m_clusterOn = (NULL != pConf) ? (atoi(pConf) > 0) : FALSE;
	LOG_INFO_FMT("cluster_on: %s", m_clusterOn ? "true" : "false");

    if (!m_clusterOn)
        return BNC_OK;

    pConf = CConf::getInstance()->getConfig("controller", "controller_eth");
    UINT4 ctrlIp = 0;
    if (::getControllerIp((NULL!=pConf)?pConf:"eth0", &ctrlIp) != BNC_OK)
    {
        LOG_WARN("getControllerIp failed!");
        return BNC_ERR;
    }
    m_clusterId = ctrlIp;

    pConf = CConf::getInstance()->getConfig("cluster_conf", "zookeeper_server");
    if (NULL != pConf)
        strncpy(m_zookeeperServer, pConf, 300 - 1);
	LOG_INFO_FMT("zookeeper_server: %s", m_zookeeperServer);

    if (0 == m_zookeeperServer[0])
    {
        LOG_WARN("No zookeeper server was set !");
        return BNC_ERR;
    }

    //zookeeper日志级别设置
    setDebugLevel();

    //初始化，连接zookeeper服务�?
    if (connect() != BNC_OK)
    {
        LOG_WARN("Connect to zookeeper server failed !");
        return BNC_ERR;
    }

    //创建sdn_root节点
    if (createRoot() != BNC_OK)
    {
        LOG_WARN("Create sdn_root node failed !");
        //return BNC_ERR;
    }

    //创建sdn_node节点
    if (createNode() != BNC_OK)
    {
        LOG_WARN("Create sdn_node node failed !");
        //return BNC_ERR;
    }

    //创建sdn_data节点
    if (createData() != BNC_OK)
    {
        LOG_WARN("Create sdn_data node failed !");
        //return BNC_ERR;
    }

    //创建sdn_data/election_generation_id节点
    if (createElectionGenerationId() != BNC_OK)
    {
        LOG_WARN("Create election_generation_id node failed !");
        //return BNC_ERR;
    }

    //创建sdn_data/master_id节点
    if (createMasterId() != BNC_OK)
    {
        LOG_WARN("Create master_id node failed !");
        //return BNC_ERR;
    }

	//获取election_generation_id
    getElectionGenerationId();

	//获取master_id
    getMasterId();

    //删除"/sdn_root/sdn_node"中旧临时节点
    deleteEphemeralNode();

    //判断是否选举完毕，若选举完毕，更新master_id 
    checkElectionFinished();

	//主从身份在下述两块中执行	
    //监控所有节�?
    if (watchNodes() != BNC_OK)
    {
        LOG_WARN("Start watch children failed !");
        //return BNC_ERR;
    }

	//监控master
    if (watchMaster() != BNC_OK)
    {
        LOG_WARN("Start watch master failed !");
        //return BNC_ERR;
    }
	
    //创建一个临时节�?
    if (createEphemeralNode() != BNC_OK)
    {
        LOG_WARN("Create sdn local node failed !");
        //return BNC_ERR;
    }

	if (getConnectState() == 0) //connected closed
	{
        LOG_WARN("Connect to zookeeper server failed !");
		return BNC_ERR;
	}

    return BNC_OK;
}

void CClusterMgr::setDebugLevel()
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
}

INT4 CClusterMgr::connect()
{
    UINT4 timeout = 10000; //100ms->10s
    m_zkhandler = zookeeper_init(m_zookeeperServer, wt_zookeeper_init, timeout, 0, 0, 0);
    if (NULL == m_zkhandler)
    {
        LOG_WARN_FMT("Connect to zookeeper server[%s] failed !", m_zookeeperServer);
        return BNC_ERR;
    }

    return BNC_OK;
}

void CClusterMgr::disconnect()
{
    if (NULL != m_zkhandler)
    {
        zookeeper_close(m_zkhandler);
        m_zkhandler = NULL;
    }
}

INT4 CClusterMgr::createRoot()
{
    INT4 ret = zoo_acreate(m_zkhandler, SDN_ROOT_PATH, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create sdn_root node");
    if (ZOK != ret)
    {
        LOG_WARN_FMT("Create sdn_root node failed, ret_code: %d", ret);
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CClusterMgr::createNode()
{
    INT4 ret = zoo_acreate(m_zkhandler, SDN_NODE_PATH, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create sdn_node node");
    if (ZOK != ret)
    {
        LOG_WARN_FMT("Create sdn_node node failed, ret_code: %d", ret);
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CClusterMgr::createData()
{
    INT4 ret = zoo_acreate(m_zkhandler, SDN_DATA_PATH, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create sdn_data node");
    if (ZOK != ret)
    {
        LOG_WARN_FMT("Create sdn_data node failed, ret_code: %d", ret);
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CClusterMgr::createElectionGenerationId()
{
    INT4 ret = zoo_acreate(m_zkhandler, SDN_ELEC_PATH, "0", 1, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create election_generation_id node");
    if (ZOK != ret)
    {
        LOG_WARN_FMT("Create election_generation_id node failed, ret_code: %d", ret);
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CClusterMgr::createMasterId()
{
    INT4 ret = zoo_acreate(m_zkhandler, SDN_MASTER_PATH, "0", 1, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create master_id node");
    if (ZOK != ret)
    {
        LOG_WARN_FMT("Create master_id node failed, ret_code: %d", ret);
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CClusterMgr::createEphemeralNode()
{
    INT1 nodeName[100] = {0};
    snprintf(nodeName, 100, "%s/%llu", SDN_NODE_PATH, m_clusterId);
    LOG_WARN_FMT("Create sdn local node %s", nodeName);

    INT4 ret = zoo_acreate(m_zkhandler, nodeName, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, cb_string_completion, "Create local node");
    if (ZOK != ret)
    {
        LOG_WARN_FMT("Create sdn local node failed, ret_code: %d", ret);
        return BNC_ERR;
    }

    return BNC_OK;
}

void CClusterMgr::deleteEphemeralNode()
{
    INT1 nodeName[100] = {0};
    snprintf(nodeName, 100, "%s/%llu", SDN_NODE_PATH, m_clusterId);
    LOG_WARN_FMT("Delete sdn local node %s", nodeName);

    zoo_delete(m_zkhandler, nodeName, -1);
}

void CClusterMgr::getElectionGenerationId()
{
    INT1 val[TABLE_STRING_LEN] = {0};
    INT4 len = TABLE_STRING_LEN;
    INT4 ret = zoo_get(m_zkhandler, SDN_ELEC_PATH, 0, val, &len, NULL);
    if (ZOK == ret)
    {
        m_electionGenerationId = string2Uint8(val);
    }
    else
    {
        CSyncMgr::getInstance()->queryElectionGenerationId(m_electionGenerationId);
    }
}

INT4 CClusterMgr::updateElectionGenerationId(UINT8 id)
{
    INT1 val[TABLE_STRING_LEN] = {0};
    snprintf(val, TABLE_STRING_LEN, "%llu", id);
    zoo_set(m_zkhandler, SDN_ELEC_PATH, val, strlen(val), -1);

    return CSyncMgr::getInstance()->persistElectionGenerationId(id);
}

void CClusterMgr::getMasterId()
{
    INT1 val[TABLE_STRING_LEN] = {0};
    INT4 len = TABLE_STRING_LEN;
    INT4 ret = zoo_get(m_zkhandler, SDN_MASTER_PATH, 0, val, &len, NULL);
    if (ZOK == ret)
    {
        m_masterId = string2Uint8(val);
    }
    else
    {
        CSyncMgr::getInstance()->queryMasterId(m_masterId);
    }
}

INT4 CClusterMgr::updateMasterId(UINT8 id)
{
    INT1 val[TABLE_STRING_LEN] = {0};
    snprintf(val, TABLE_STRING_LEN, "%llu", id);
    zoo_set(m_zkhandler, SDN_MASTER_PATH, val, strlen(val), -1);

    return CSyncMgr::getInstance()->persistMasterId(id);
}

UINT8 CClusterMgr::zkGetMasterId()
{
    INT1 val[TABLE_STRING_LEN] = {0};
    INT4 len = TABLE_STRING_LEN;
    INT4 ret = zoo_get(m_zkhandler, SDN_MASTER_PATH, 0, val, &len, NULL);
    if (ZOK == ret)
        return string2Uint8(val);

    return 0;
}

INT4 CClusterMgr::watchNodes()
{
    //开始监控所有节点，�?/sdn_root/sdn_node"发生改变时，wt_get_children2会收到通知
    INT4 ret = zoo_awget_children2(m_zkhandler, SDN_NODE_PATH, wt_get_children2, 0, cb_awget_children2, "Get children");
    if (ret)
    {
        LOG_WARN_FMT("Start watch children failed, ret_code: %d", ret);
        return BNC_ERR;
    }
    
    return BNC_OK;
}

INT4 CClusterMgr::watchMaster()
{
	//监控master_id �?/sdn_root/sdn_data/master_id"发生改变�? wt_get_master_id会收到通知
    INT4 ret = zoo_awget(m_zkhandler, SDN_MASTER_PATH, wt_get_master_id, 0, cb_awget_master_id, "Get master");  
	if(ret)
    {
        LOG_WARN_FMT("Start watch master failed, ret_code: %d", ret);
        return BNC_ERR;
    }
    
    return BNC_OK;
}

INT4 CClusterMgr::getConnectState()
{
    return zoo_state(m_zkhandler);
}

void CClusterMgr::checkElectionFinished()
{
    INT1 flag = -1;
    INT4 ret = zoo_aget_children2(m_zkhandler, SDN_NODE_PATH, 0, cb_get_children2, &flag);
    if (ZOK == ret)
    {
        INT4 count = 3000; //等待cb_get_children2结果，最�?00s
        while ((0 != flag) && (1 != flag) && (count > 0))
        {
            usleep(100000);
            count--;
            continue;
        }

        if ((0 == flag) || (1 == flag))
        {
            INT1 val[TABLE_STRING_LEN] = {0};
            INT4 len = TABLE_STRING_LEN;
            zoo_get(m_zkhandler, SDN_MASTER_PATH, 0, val, &len, NULL);
            m_masterId = string2Uint8(val);
        }
    }
    else
    {
        LOG_WARN("Get zookeeper sdn nodes failed !");
    }
}

void CClusterMgr::setControllers(INT4 count, INT1** nodes)
{
    m_controllerCount = (count < MAX_CONTROLLER_COUNT) ? count : MAX_CONTROLLER_COUNT;
    memset(m_controllers, 0, MAX_CONTROLLER_COUNT * 30);    

    UINT8 minNodeId = -1, nodeId = 0;
    
    for (INT4 i = 0; i < m_controllerCount; i++)
    {
        nodeId = string2Uint8(nodes[i]);
        LOG_WARN_FMT("setControllers nodeId[%llu],m_masterId[%llu],m_clusterId[%llu]", 
            nodeId, m_masterId, m_clusterId);
        if (nodeId == m_masterId)
        {
            minNodeId = m_masterId;
            break;
        }
    
        if (nodeId < minNodeId)
            minNodeId = nodeId;
    
        snprintf(m_controllers[i], 30, "tcp:%s:6633", inet_htoa(nodeId));
    }
    
    
    if (minNodeId == m_clusterId)
    {
        LOG_WARN_FMT("Master re-elected, I'm master. masterId[%llu],clusterId[%llu],minNodeId[%llu],controllerCount[%d]", 
            m_masterId, m_clusterId, minNodeId, m_controllerCount);
        CClusterMgr::getInstance()->updateRole(OFPCR_ROLE_MASTER);
        updateControllerInfo(count, nodes);
    }
    else
    {
        LOG_WARN_FMT("Master re-elected, I'm slave. masterId[%llu],clusterId[%llu],minNodeId[%llu],controllerCount[%d]", 
            m_masterId, m_clusterId, minNodeId, m_controllerCount);
        CClusterMgr::getInstance()->updateRole(OFPCR_ROLE_SLAVE);
    }
}

INT4 CClusterMgr::updateRole(INT4 role)
{
    LOG_WARN_FMT("updateRole role[%u]m_clusterId[%llu]m_controllerRole[%d]", role, m_clusterId,m_controllerRole);

    if (!m_clusterOn)
        return BNC_ERR;

    if (role == m_controllerRole)
        return BNC_OK;

    m_controllerRole = role;

    if (OFPCR_ROLE_MASTER == role)
    {
        CSyncMgr::getInstance()->persistTotal();

        m_masterId = m_clusterId;
        updateMasterId(m_masterId);

        //获取最新的generation_id
        getElectionGenerationId();

        updateElectionGenerationId(m_electionGenerationId);
    }
    else if (OFPCR_ROLE_SLAVE == role)
    {
        //�?
    }
    else if (OFPCR_ROLE_EQUAL == role)
    {
        return BNC_OK;
    }
    else
    {
        return BNC_ERR;
    }

	//遍历所有交换机, 更新自己的主备身�?
    CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    STL_FOR_LOOP(map, it)
    {
		CSmartPtr<CSwitch> sw = it->second;
        if (sw.isNull())
            continue;
        
		if ((OFP13_VERSION == sw->getVersion()) &&
            (SW_STATE_STABLE == sw->getState()))
		{
            COfMsgUtil::sendOfp13RoleRequest(sw->getSockfd(), role);
        }
    }

	CControl::getInstance()->getSwitchMgr().unlock();

    return BNC_OK;
}

void CClusterMgr::updateControllerInfo(INT4 count, INT1** nodes)
{
    const INT1* pConf = CConf::getInstance()->getConfig("restful_conf", "http_server_port");
    UINT2 httpPort = (NULL == pConf) ? 8081 : atoi(pConf);
    UINT8 nodeId = 0;

    for (INT4 i = 0; i < count; i++)
    {
        nodeId = string2Uint8(nodes[i]);
        if (nodeId == m_masterId)
            CSyncMgr::getInstance()->persistControllerInfo(nodeId, httpPort, OFPCR_ROLE_MASTER);
        else
            CSyncMgr::getInstance()->persistControllerInfo(nodeId, httpPort, OFPCR_ROLE_SLAVE);
    }
}


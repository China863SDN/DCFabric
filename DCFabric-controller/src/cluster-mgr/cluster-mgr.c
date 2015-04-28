/******************************************************************************
*                                                                             *
*   File Name   : cluster-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "cluster-mgr.h"
#include "hbase_client.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../conn-svr/conn-svr.h"
#include "zookeeper.h"

UINT1 g_controller_role = OFPCR_ROLE_EQUAL;     //当前控制器角色
UINT4 g_controller_cnt = 0;                     //当前集群中控制器数
UINT8 g_election_generation_id = 0;             //master选举版本号
UINT8 g_cluster_id;                             //控制器id, 集群中每个控制器都有一个id,最小id为master
INT1 g_zookeeper_server[300] = {0};             //zookeeper服务器地址  ip:port,ip:port,ip:port,ip:port
INT1 g_controllers[MAX_CONTROLLER][30];         //所有控制器的信息
UINT8 g_master_id;                              //当前master的id

static zhandle_t * g_zkhandler = NULL;         //zookeep client hanlder
//static controller_cluster_t g_controller_cluster[CLUSTER_NUM];     //控制器集群管理结构

const INT1* SDN_ROOT_PATH = "/sdn_root";
const INT1* SDN_NODE_PATH = "/sdn_root/sdn_node";
const INT1* SDN_DATA_PATH = "/sdn_root/sdn_data";
const INT1* SDN_ELEC_PATH = "/sdn_root/sdn_data/election_generation_id";
const INT1* SDN_MASTER_PATH = "/sdn_root/sdn_data/master_id";
const INT1* SDN_TOPO_VER_PATH = "/sdn_root/sdn_data/topology_version_id";

INT4 update_role(UINT4 role)
{
    INT4 ret = 0;
    UINT4 idx_sw = 0;
    role_req_info_t role_req_info;
    gn_switch_t *sw = NULL;

    if((role == g_controller_role) || (NULL == g_zkhandler))
    {
        return GN_OK;
    }

    //获取最新的generation_id
    if(OFPCR_ROLE_MASTER == role)
    {
        INT4 len = TABLE_STRING_LEN;
        INT1 election_generation[TABLE_STRING_LEN] = {0};
        INT1 master_id[TABLE_STRING_LEN] = {0};

        g_master_id = g_cluster_id;
        if(ZOK != zoo_get(g_zkhandler, SDN_ELEC_PATH, 0, election_generation, &len, NULL))
        {
            g_election_generation_id = strtoull(election_generation, 0, 10);
        }
#ifdef HBASE_CLIENT
        else
        {
            query_value(ELECTION_GENERATION_ID, election_generation);
            g_election_generation_id = strtoull(election_generation, 0, 10);
        }
#endif
        snprintf(election_generation, TABLE_STRING_LEN, "%llu", g_election_generation_id);
        snprintf(master_id, TABLE_STRING_LEN, "%llu", g_master_id);

        zoo_set(g_zkhandler, SDN_ELEC_PATH, election_generation, strlen(election_generation), -1);
        zoo_set(g_zkhandler, SDN_MASTER_PATH, master_id, strlen(master_id), -1);

#ifdef HBASE_CLIENT
        persist_value(ELECTION_GENERATION_ID, election_generation);
        persist_value(MASTER_ID, master_id);
#endif
    }
    else if(OFPCR_ROLE_SLAVE == role)
    {

    }
    else if(OFPCR_ROLE_EQUAL == role)
    {

    }
    else
    {
        return EC_ROLE_INVALID;
    }

    g_controller_role = role;
    role_req_info.generation_id = g_election_generation_id;
    role_req_info.role = role;
    for(; idx_sw < g_server.max_switch; idx_sw++)
    {
        if (g_server.switches[idx_sw].state)
        {
            sw = &g_server.switches[idx_sw];
            if(sw && (sw->state))
            {
                ret = sw->msg_driver.msg_handler[OFPT13_ROLE_REQUEST](sw, (UINT1 *)&role_req_info);
            }
        }
    }

    return ret;
}

void zookeeper_error_msg(INT4 rc, const INT1* opr_name)
{
    //ZOK 操作完成
    //ZNONODE 父节点不存在
    //ZNODEEXISTS 节点已存在
    //ZNOAUTH 客户端没有权限创建节点
    //ZNOCHILDRENFOREPHEMERALS 临时节点不能创建子节点。
    switch(rc)
    {
    case ZOK:
        LOG_PROC("INFO", "%s succeed", opr_name);
        break;
    case ZNONODE:
        LOG_PROC("ERROR", "%s failed, parent node not exist", opr_name);
        break;
    case ZNOAUTH:
        LOG_PROC("ERROR", "%s failed, no permission to create", opr_name);
        break;
    case ZNOCHILDRENFOREPHEMERALS:
        LOG_PROC("ERROR", "%s failed, parent node is ephemeral", opr_name);
        break;
    case ZOPERATIONTIMEOUT:
        LOG_PROC("ERROR", "%s failed, operate timeout", opr_name);
        break;
    case ZCONNECTIONLOSS:
        LOG_PROC("ERROR", "%s failed, connection loss", opr_name);
        break;
    case ZNODEEXISTS:
        LOG_PROC("ERROR", "%s failed, node already exist", opr_name);
        break;
    default:
        LOG_PROC("ERROR", "%s failed. ret_code: %d", opr_name, rc);
    }
}

void cb_string_completion(INT4 rc, const INT1 *value, const void *data)
{
    zookeeper_error_msg(rc, data);
}

void wt_zookeeper_init(zhandle_t* zh, INT4 type, INT4 state, const INT1* path, void* watcherCtx)
{
    if(type == ZOO_SESSION_EVENT)
    {
        //已连接:ZOO_CONNECTED_STATE
        //session失效:ZOO_EXPIRED_SESSION_STATE
        if(state == ZOO_CONNECTED_STATE)
        {
            LOG_PROC("INFO", "Connected to zookeeper server[%s] successfully", g_zookeeper_server);
        }
        else if(state == ZOO_EXPIRED_SESSION_STATE)
        {
            LOG_PROC("ERROR", "Zookeeper session expired");
        }
    }
}

void cb_awget_children2 (INT4 rc, const struct String_vector *strings, const struct Stat *stat, const void *data)
{
    if(ZOK == rc)
    {
        if (strings)
        {
            UINT4 idx = 0;
            UINT8 min_node_id = -1;
            UINT8 node_id = 0;

            if (MAX_CONTROLLER < strings->count)
            {
                LOG_PROC("ERROR", "Error: Too many controllers");
                return;
            }
            g_controller_cnt = strings->count;
            memset(g_controllers, 0, MAX_CONTROLLER * 30);

            for (idx=0; idx <strings->count; idx++)
            {
                node_id = strtoull(strings->data[idx], 0, 10);
                if (node_id == g_master_id)
                {
                    min_node_id = g_master_id;
                    break;
                }

                if(node_id < min_node_id)
                {
                    min_node_id = node_id;
                }

                sprintf(g_controllers[idx], "tcp:%s:6633", inet_htoa(ntohl(node_id)));
            }

            if (g_cluster_id == min_node_id)
            {
                LOG_PROC("INFO", "Master re-elected, I'm the master %llu %llu %llu", g_master_id, g_cluster_id, min_node_id);

                update_role(OFPCR_ROLE_MASTER);
            }
            else
            {
                LOG_PROC("INFO", "Master re-elected, I'm the slave");

                update_role(OFPCR_ROLE_SLAVE);
            }
        }
    }

    zookeeper_error_msg(rc, data);
}

void wt_get_children2(zhandle_t *zh, INT4 type, INT4 state, const INT1 *path, void *watcherCtx)
{
    if (ZOO_CONNECTED_STATE == state)
    {
        if (ZOO_CHILD_EVENT == type)
        {
            INT4 ret = 0;
            ret = zoo_awget_children2(g_zkhandler, SDN_NODE_PATH, wt_get_children2, 0, cb_awget_children2, "Get children");
            if (ret)
            {
                LOG_PROC("ERROR", "Start watch children failed. --ret code: %d", ret);
                return;
            }
        }
        else if(ZOO_DELETED_EVENT == type)
        {
            LOG_PROC("INFO", "Path %s was deleted", path);

        }
        else if (ZOO_CREATED_EVENT == type)
        {
            LOG_PROC("INFO", "Path %s was created", path);
        }
    }
}

//初始化
INT4 init_cluster_mgr()
{
    INT1 node_name[100];
    INT1 value[TABLE_STRING_LEN] = {0};
    UINT4 timeout = 100;
    INT4 len = 0;
    INT4 ret = 0;
    INT1 *conf_value = NULL;

    conf_value = get_value(g_controller_configure, "[controller]", "zoo_server");
    NULL == conf_value ? strncpy(g_zookeeper_server, "0", 300 - 1) : strncpy(g_zookeeper_server, conf_value, 300 - 1);

    if (0 == strcmp("0", g_zookeeper_server))
    {
        LOG_PROC("WARNING", "No zookeeper server was set");
        return GN_OK;
    }

    //zookeeper日志级别设置
    zoo_set_debug_level(ZOO_LOG_LEVEL_INFO);

    //初始化，连接zookeeper服务器
    g_zkhandler = zookeeper_init(g_zookeeper_server, wt_zookeeper_init, timeout, 0, 0, 0);
    if (g_zkhandler == NULL)
    {
        LOG_PROC("ERROR", "Connecting to zookeeper servers failed");
        return GN_ERR;
    }

    //创建sdn_root节点
    ret = zoo_acreate(g_zkhandler, SDN_ROOT_PATH, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create sdn_root node");
    if (ret)
    {
        LOG_PROC("ERROR", "Create sdn_root node failed, ret_code: %d", ret);
        return GN_ERR;
    }

    //创建sdn_node节点
    ret = zoo_acreate(g_zkhandler, SDN_NODE_PATH, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create sdn_node node");
    if (ret)
    {
        LOG_PROC("ERROR", "Create sdn_node node failed, ret_code: %d", ret);
        return GN_ERR;
    }

    //创建sdn_data节点
    ret = zoo_acreate(g_zkhandler, SDN_DATA_PATH, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create sdn_data node");
    if (ret)
    {
        LOG_PROC("ERROR", "Create sdn_data node failed, ret_code: %d", ret);
        return GN_ERR;
    }

    //创建sdn_data/election_generation_id节点
    //    ret = zoo_create(g_zkhandler, SDN_ELEC_PATH, "0", 1, &ZOO_OPEN_ACL_UNSAFE, NULL, 0);
    ret = zoo_acreate(g_zkhandler, SDN_ELEC_PATH, "0", 1, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create election_generation_id node");
    if (ret)
    {
        LOG_PROC("ERROR", "Create election_generation_id node failed, ret_code: %d", ret);
        return GN_ERR;
    }

    //创建sdn_data/master_id节点
    ret = zoo_acreate(g_zkhandler, SDN_MASTER_PATH, "0", 1, &ZOO_OPEN_ACL_UNSAFE, 0, cb_string_completion, "Create master_id node");
    if (ret)
    {
        LOG_PROC("ERROR", "Create master_id node failed, ret_code: %d", ret);
        return GN_ERR;
    }

    //read all the important info from hbase or zookeeper
    g_election_generation_id = 0;
    g_master_id = 0;
#ifdef HBASE_CLIENT

    query_value(ELECTION_GENERATION_ID, value);
    g_election_generation_id = strtoull(value, 0, 10);

    query_value(MASTER_ID, value);
    g_master_id = strtoull(value, 0, 10);
#endif

    memset(value, 0, TABLE_STRING_LEN);
    if(ZOK == zoo_get(g_zkhandler, SDN_ELEC_PATH, 0, value, &len, NULL))
    {
        g_election_generation_id = strtoull(value, 0, 10);
    }

    if(0 == g_master_id)
    {
        INT4 len = TABLE_STRING_LEN;
        zoo_get(g_zkhandler, SDN_MASTER_PATH, 0, value, &len, NULL);
        g_master_id = strtoull(value, 0, 10);
    }

    //创建一个临时节点
    snprintf(node_name, 100, "%s/%llu", SDN_NODE_PATH, g_cluster_id);
    zoo_delete(g_zkhandler, node_name, -1);
    ret = zoo_acreate(g_zkhandler, node_name, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, cb_string_completion, "Create local node");
    if (ret)
    {
        LOG_PROC("ERROR", "Create sdn local node failed, ret_code: %d", ret);
        return GN_ERR;
    }

    //开始监控所有节点
    ret = zoo_awget_children2(g_zkhandler, SDN_NODE_PATH, wt_get_children2, 0, cb_awget_children2, "Get children");
    if (ret)
    {
        LOG_PROC("ERROR", "Start watch children failed, ret_code: %d", ret);
        return GN_ERR;
    }

    return GN_OK;
}

//销毁
void fini_cluster_mgr()
{
    zookeeper_close(g_zkhandler);
}

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
*   File Name   : COvsdbMgr.cpp                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-param.h"
#include "bnc-error.h"
#include "log.h"
#include "COvsdbMgr.h"
#include "CConf.h"
#include "comm-util.h"
#include "CServer.h"
#include "openflow-common.h"
#include "CControl.h"

#define  SEARCH_ALL_TABLE_ID        	"01"
#define  ADD_BRIDGE_ID              	"02"
#define  SET_FAILMODE_OF_ID         	"03"
#define  SET_CONTROLLER_ID          	"04"
#define  ADD_PORT_ID                	"05"
#define  ADD_INTERFACE_OPTION_ID    	"06"
#define  SEARCH_HOST_BY_MAC				"07"
#define  CREATE_OVSDB_QUEUE				"08"
#define  SEARCH_INTERFACE_BY_PORT_NO	"09"
#define  SEARCH_PORT_BY_INTERFACE		"10"
#define  SEARCH_PORT_QOS_TABLE			"11"

#define  INTERNAL_BR                    "br-int"
#define  EXTERNAL_BR                    "br-ex"

#define  FAIL_MODE_SECURE               "secure"

#define  OFP_10                         "OpenFlow10"
#define  OFP_13                         "OpenFlow13"

typedef enum
{
    TUNNEL_TYPE_NONE     = 0,
    TUNNEL_TYPE_GRE      = 1,
    TUNNEL_TYPE_VLAN     = 2,
    TUNNEL_TYPE_VXLAN    = 3,
    TUNNEL_TYPE_INTERNAL = 4,
    TUNNEL_TYPE_PATCH    = 5,
    TUNNEL_TYPE_MAX
}tunnel_type_e;
const INT1* tunnelTypeString[TUNNEL_TYPE_MAX] =
{
    "none",
    "gre",
    "vlan",
    "vxlan",
    "internal",
    "patch"
};

COvsdbClient::COvsdbClient():m_sockfd(-1),m_ip(0),m_port(0)
{
    memset(m_mac, 0, MAC_LEN);
    memset(&m_sw, 0, sizeof(open_vswitch_t));
}

COvsdbClient::COvsdbClient(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port):
    m_sockfd(sockfd),m_ip(ip),m_port(port)
{
    memcpy(m_mac, mac, MAC_LEN);
    memset(&m_sw, 0, sizeof(open_vswitch_t));
}

COvsdbClient::~COvsdbClient()
{
}

void COvsdbClient::saveBridge(const ovs_bridge_t& bridge)
{
    m_bridges.push_back(bridge);
}

void COvsdbClient::clearBridges()
{
    m_bridges.clear();
}

COvsdbMgr* COvsdbMgr::m_instance = NULL;

COvsdbMgr* COvsdbMgr::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new COvsdbMgr();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return m_instance;
}

COvsdbMgr::COvsdbMgr():
    m_tunnelOn(FALSE),
    m_tunnelType(TUNNEL_TYPE_NONE),
    m_ofVersion(OFP13_VERSION),
    m_clients(hash_bucket_number),
    m_ipMap(hash_bucket_number)
{
}

COvsdbMgr::~COvsdbMgr()
{
}

INT4 COvsdbMgr::init()
{
    const INT1* pConf = CConf::getInstance()->getConfig("ovsdb_conf", "ovsdb_tunnel_on");
    m_tunnelOn = (NULL != pConf) ? (atoi(pConf) > 0) : FALSE;
	LOG_INFO_FMT("ovsdb_tunnel_on: %s", m_tunnelOn ? "true" : "false");

    pConf = CConf::getInstance()->getConfig("ovsdb_conf", "ovsdb_tunnel_type");
    if (NULL != pConf)
    {
        for (INT4 type = TUNNEL_TYPE_GRE; type < TUNNEL_TYPE_MAX; ++ type)
            if (strcasecmp(pConf, tunnelTypeString[type]) == 0)
            {
                m_tunnelType = type;
                break;
            }
    }
    LOG_INFO_FMT("ovsdb_tunnel_type %s[%d]", tunnelTypeString[m_tunnelType], m_tunnelType);

    pConf = CConf::getInstance()->getConfig("ovsdb_conf", "ovsdb_of_version");
    if (NULL != pConf)
    {
        if (strcasecmp(pConf, "of10") == 0)
            m_ofVersion = OFP10_VERSION;
        else if (strcasecmp(pConf, "of13") == 0)
            m_ofVersion = OFP13_VERSION;
    }
    LOG_INFO_FMT("ovsdb_of_version %d", m_ofVersion);

    return BNC_OK;
}

UINT4 COvsdbMgr::genJson(const INT1* buffer, UINT4 len)
{
    if ((NULL == buffer) || (0 == len) || ('\0' == buffer[0]))
        return 0;

    UINT4 ret = 0;
    UINT4 brace_cnt = 0;

    for (UINT4 i = 0; i < len; ++ i)
    {
        if ('{' == buffer[i])
        {
            ++ brace_cnt;
        }
        else if ('}' == buffer[i])
        {
            -- brace_cnt;
            if (0 == brace_cnt)
            {
                ret = i + 1;
                break;
            }
        }
    }

    return ret;
}

INT4 COvsdbMgr::process(INT4 sockfd, const INT1* msg, UINT4 len)
{
    LOG_INFO_FMT("COvsdbMgr process JSON[%u][%s] ...\n", len, msg);

    //校验OVSDB消息是否为json格式，是则将其构造到root节点�?
    json_t* root = NULL;
    json_error parse = json_parse_document(&root, msg);
    if (NULL == root)
    {
        //LOG_WARN_FMT("json_parse_document failed[%d] !\n", parse);
        return BNC_ERR;
    }
    if (JSON_OK != parse)
    {
        LOG_WARN_FMT("json_parse_document return %d !\n", parse);
        json_free_value_all(&root);
        return BNC_ERR;
    }

    //查找"method"JSON对象
    json_t* method = json_find_first_label(root, "method");
    if (NULL != method)
    {
        if ((NULL != method->child) && 
            (NULL != method->child->text) && 
            (strncmp(method->child->text, "update", 6) == 0))   
        {
            json_t* id = json_find_first_label(root, "id");
            if (NULL != id)
            {
                if ((NULL != id->child) && (JSON_NULL == id->child->type))
                {
                    json_t* params = json_find_first_label(root, "params");
                    if (NULL != params)
                    {
                        if ((NULL != params->child) && 
                            (NULL != params->child->child) &&
                            (NULL != params->child->child->next))
                        {
                            json_t* br = json_find_first_label(params->child->child->next, "Bridge");
                            if (NULL != br)
                            {
                                if ((NULL != br->child) && 
                                    (NULL != br->child->child) &&
                                    (NULL != br->child->child->text))   
                                {
                                    LOG_WARN_FMT("Switch info: name[%s], uuid[%s]", INTERNAL_BR, br->child->child->text);

                                    //保存bridge相关信息
                                    ovs_bridge_t bridge = {0};
                                    memcpy(bridge.name, INTERNAL_BR, strlen(INTERNAL_BR));
                                    memcpy(bridge.uuid, br->child->child->text, strlen(br->child->child->text));

                                    if ((NULL != br->child->child->child) &&
                                        (NULL != br->child->child->child->child) &&
                                        (NULL != br->child->child->child->child->child))   
                                    {
                                        json_t* br_dpid = json_find_first_label(br->child->child->child->child->child, "datapath_id");
                                        if (NULL != br_dpid)
                                        {
                                            if (NULL != br_dpid->text)
                                                bridge.dpid = string2Uint8(br_dpid->text);
                                            json_free_value(&br_dpid);
                                        }

                                        //向ovsdb设置控制器相关信�?
                                        json_t* br_ctrl = json_find_first_label(br->child->child->child->child->child, "controller");
                                        if (NULL != br_ctrl)
                                        {
                                            if ((NULL != br_ctrl->child) &&
                                                (NULL != br_ctrl->child->child) &&
                                                (NULL != br_ctrl->child->child->text))
                                            {
                                                if (strcmp(br_ctrl->child->child->text, "uuid") != 0)
                                                    setController(sockfd, bridge.uuid);
                                            }
                                            json_free_value(&br_ctrl);
                                        }                                 

                                    }

                                    saveBridge(sockfd, bridge);

                                    //向ovsdb设置port,tunnel相关信息
                                    addPort(sockfd, bridge.uuid, bridge.name);
                                    addTunnel(sockfd, bridge);
                                }

                                json_free_value(&br);
                                json_free_value(&params);
                                json_free_value(&id);
                                json_free_value(&method);
                                json_free_value_all(&root);

                                return BNC_OK;
                            }
                             
                            json_t* queue = json_find_first_label(params->child->child->next, "Queue");
                            if (NULL != queue) 
                            {
                                m_qosQueue.notifyReceviceQueueUuid(queue);
                                json_free_value(&queue);
                            }
        
                            json_t* qos = json_find_first_label(params->child->child->next, "QoS");
                            if (NULL != qos) 
                            {
                                m_qosQueue.notifyReceviceQosUuid(queue);
                                json_free_value(&qos);
                            }
                        }
                        json_free_value(&params);
                    }
                }
                json_free_value(&id);
            }
        }
        json_free_value(&method);
    }

    //查找"id"JSON对象
    json_t* id = json_find_first_label(root, "id");
    if (NULL != id)
    {
        if ((NULL != id->child) && (NULL != id->child->text))
        {
            if (strncmp(id->child->text, "echo", 4) == 0)                  
            {
                replyEcho(sockfd);
            }
            else if (strncmp(id->child->text, SEARCH_ALL_TABLE_ID, 2) == 0)
            {
                json_t* result = id->next;
                if ((NULL != result) && (0 == strcmp(result->text, "result")))
                {
                    handleOpenvswitchTable(sockfd, result);
                    BOOL haveController = handleControllerTable(sockfd, result);
                    handleBridgeTable(sockfd, result, haveController);
                }
            }
            else if (strncmp(id->child->text, SEARCH_HOST_BY_MAC, 2) == 0) 
            {
                json_t* result = id->next;
                if ((NULL != result) && (0 == strcmp(result->text, "result"))) 
                {
                    handleSearchHostByMac(sockfd, result);
                }
            }
            else if (strncmp(id->child->text, SEARCH_INTERFACE_BY_PORT_NO, 2) == 0) 
            {
                json_t* result = id->next;
                if ((NULL != result) && (0 == strcmp(result->text, "result"))) 
                {
                    m_qosQueue.notifyReceviceInterfaceUuid(result);
                }
            }
            else if (strncmp(id->child->text, SEARCH_PORT_BY_INTERFACE, 2) == 0) 
            {
                json_t* result = id->next;
                if ((NULL != result) && (0 == strcmp(result->text, "result"))) 
                {
                    m_qosQueue.notifyRecevicePortUuid(result);
                }
            }
            else if (strncmp(id->child->text, SEARCH_PORT_QOS_TABLE, 2) == 0) 
            {
                json_t* result = id->next;
                if ((NULL != result) && (0 == strcmp(result->text, "result"))) 
                {
                    m_qosQueue.notifyReceiveSearchBridgeUuid(sockfd, result);
                }
            }
        }
        json_free_value(&id);
    }
     
    json_free_value_all(&root);
    return BNC_OK;
}

INT4 COvsdbMgr::addClient(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port)
{
    COvsdbClient* client = new COvsdbClient(sockfd, mac, ip, port);
    if (NULL == client)
    {
        LOG_ERROR_FMT("new COvsdbClient failed !");
        return BNC_ERR;
    }

    LOG_INFO_FMT("before add client with sockfd[%d]ip[%s]port[%u], client map size[%lu] ...", 
        sockfd, inet_htoa(client->getIp()), client->getPort(), m_clients.size());
    CSmartPtr<COvsdbClient> sclient(client);
    if (!m_clients.insert(sockfd, sclient))
    {
        LOG_WARN_FMT("add mapping sockfd[%d] to COvsdbClient failed !", sockfd);
        return BNC_ERR;
    }
    LOG_INFO_FMT("after add client with sockfd[%d]ip[%s]port[%u], client map size[%lu] ...", 
        sockfd, inet_htoa(client->getIp()), client->getPort(), m_clients.size());

    LOG_INFO_FMT("before add mapping ip[%x], m_ipMap size[%lu] ...", ip, sockfd, m_ipMap.size());
    if (!m_ipMap.insert(ip, sclient))
    {
        LOG_WARN_FMT("add mapping ip[%x] failed !", ip);
        return BNC_ERR;
    }
    LOG_INFO_FMT("after add mapping ip[%x], m_ipMap size[%lu] ...", ip, sockfd, m_ipMap.size());

    searchAllTableFromOvsdb(sockfd);

    return BNC_OK;
}

void COvsdbMgr::delClient(INT4 sockfd)
{
    CSmartPtr<COvsdbClient> client = COvsdbMgr::getClient(sockfd);
    if (client.isNotNull())
    {
        LOG_INFO_FMT("before delete mapping ip[%x], m_ipMap size[%lu] ...", client->getIp(), m_ipMap.size());
        m_ipMap.erase(client->getIp());
        LOG_INFO_FMT("after delete mapping ip[%x], m_ipMap size[%lu] ...", client->getIp(), m_ipMap.size());
    }

    LOG_INFO_FMT("before delete client with sockfd[%d], client map size[%lu] ...", 
        sockfd, m_clients.size());
    m_clients.erase(sockfd);
    LOG_INFO_FMT("after delete client with sockfd[%d], client map size[%lu] ...", 
        sockfd, m_clients.size());
}

CSmartPtr<COvsdbClient> COvsdbMgr::getClient(INT4 sockfd)
{
    CSmartPtr<COvsdbClient> client;

    COvsdbClientHMap::CPair* item = NULL;
    if (m_clients.find(sockfd, &item))
        client = item->second;

    return client;
}

CSmartPtr<COvsdbClient> COvsdbMgr::getClient(UINT4 ip)
{
    CSmartPtr<COvsdbClient> client;

    COvsdbClientHMap::CPair* item = NULL;
    if (m_ipMap.find(ip, &item))
        client = item->second;

    return client;
}

void COvsdbMgr::saveBridge(INT4 sockfd, const ovs_bridge_t& bridge)
{
    CSmartPtr<COvsdbClient> client = getClient(sockfd);
    if (client.isNotNull())
        client->saveBridge(bridge);
}

//构造控制器相关的JSON数据信息,并发送给ovsdb,对ovsdb进行设置
//{"id":"2e93096c-ab33-4abd-a244-bad919566fac","method":"transact","params":["Open_vSwitch",
//{"op":"mutate","table":"Bridge","where":[["_uuid","==",["uuid","612c720c-52a5-4231-bb51-ff94a0d38671"]]],
// "mutations":[["controller","insert",["named-uuid","new_controller"]]]},
//{"op":"insert","table":"Controller","row":{"target":"tcp:10.8.1.211:6633"},"uuid-name":"new_controller"}
//]}
void COvsdbMgr::setController(INT4 sockfd, const INT1* uuid)
{
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array, *uuid_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(SET_CONTROLLER_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    root_array = json_new_array();
    json_insert_child(key, root_array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(root_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("mutate");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //where
    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(uuid);
    json_insert_child(uuid_array, key);

    //mutations
    key = json_new_string("mutations");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("controller");
    json_insert_child(_uuid_array, key);

    key = json_new_string("insert");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("named-uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string("new_controller");
    json_insert_child(uuid_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Controller");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("target");
    INT1 controllerIp[128] = {0};
    sprintf(controllerIp, "tcp:%s:%d", 
       inet_htoa(CServer::getInstance()->getControllerIp()), 
       CServer::getInstance()->getOfPort());
    value = json_new_string(controllerIp);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_controller");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    INT1* text = NULL;
    json_tree_to_string(obj, &text);
    json_free_value_all(&obj);

    if (NULL != text)
    {
        if (!sendMsgOut(sockfd, text, strlen(text)))
        {
            CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
            if (worker.isNotNull())
                worker->processPeerDisconn(sockfd);
        }

    	free(text);
    }
}

//向句柄对应的ovsdb发送添加port的命�?
//{"id":"d0d14471-5a66-412c-bcae-0e594f5ee9af","method":"transact","params":["Open_vSwitch",
//{"op":"mutate","table":"Bridge","where":[["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],
// "mutations":[["ports","insert",["named-uuid","new_port"]]]},
//{"op":"insert","table":"Port","row":{"name":"gre-10.8.1.212","interfaces":["set",[["named-uuid","new_interface"]]]},"uuid-name":"new_port"},
//{"op":"insert","table":"Interface","row":{"name":"gre-10.8.1.212"},"uuid-name":"new_interface"}]}
void COvsdbMgr::addPort(INT4 sockfd, const INT1* uuid, const INT1* name)
{
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array, *uuid_array, *named_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(ADD_PORT_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    root_array = json_new_array();
    json_insert_child(key, root_array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(root_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("mutate");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //where
    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(uuid);
    json_insert_child(uuid_array, key);

    //mutations
    key = json_new_string("mutations");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("ports");
    json_insert_child(_uuid_array, key);

    key = json_new_string("insert");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("named-uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string("new_port");
    json_insert_child(uuid_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    //Port
    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Port");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    //interfaces
    key = json_new_string("interfaces");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);

    key = json_new_string("named-uuid");
    json_insert_child(named_array, key);

    key = json_new_string("new_interface");
    json_insert_child(named_array, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_port");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //interface
    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    INT1 *text = NULL;
    json_tree_to_string(obj, &text);
    json_free_value_all(&obj);

    if (NULL != text)
    {
        if (!sendMsgOut(sockfd, text, strlen(text)))
        {
            CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
            if (worker.isNotNull())
                worker->processPeerDisconn(sockfd);
        }

    	free(text);
    }
}

//向句柄对应的ovsdb发送添加bridge的命�?
//{"id":"315c15f7-8b78-41ff-880f-fc6211cb5b8e","method":"transact","params":
// ["Open_vSwitch",{"op":"mutate","table":"Open_vSwitch","where":[["_uuid","==",["uuid","17074e89-2ac5-4bba-997a-1a5a3527cf56"]]],
//                  "mutations":[["bridges","insert",["named-uuid","new_bridge"]]]},
//                 {"op":"insert","table":"Bridge","row":{"name":"br-int","fail_mode":["set",["secure"]],
//                  "protocols":["set",["OpenFlow13"]]},"uuid-name":"new_bridge"}]}
void COvsdbMgr::addBridge(INT4 sockfd, const INT1* name, const INT1* uuid, const INT1* failMode, const INT1* ofproto)
{
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array, *uuid_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(ADD_BRIDGE_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    root_array = json_new_array();
    json_insert_child(key, root_array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(root_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("mutate");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Open_vSwitch");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //where
    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(uuid);
    json_insert_child(uuid_array, key);

    //mutations
    key = json_new_string("mutations");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("bridges");
    json_insert_child(_uuid_array, key);

    key = json_new_string("insert");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("named-uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string("new_bridge");
    json_insert_child(uuid_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    //Bridge
    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    //fail_mode
    key = json_new_string("fail_mode");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(failMode);
    json_insert_child(uuid_array, key);

    key = json_new_string("protocols");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(ofproto);
    json_insert_child(uuid_array, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    INT1* text = NULL;
    json_tree_to_string(obj, &text);
    json_free_value_all(&obj);

    if (NULL != text)
    {
        if (!sendMsgOut(sockfd, text, strlen(text)))
        {
            CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
            if (worker.isNotNull())
                worker->processPeerDisconn(sockfd);
        }

    	free(text);
    }
}

void COvsdbMgr::addTunnel(INT4 sockfd, const ovs_bridge_t& bridge)
{
    if (!m_tunnelOn)
        return;    

    CSmartPtr<COvsdbClient> compute = getClient(sockfd);
    if (compute.isNull())
        return;    

    const INT1* computeIp = inet_htoa(compute->getIp());
    const INT1* remoteIp = NULL;
    INT1 controlTunnelPortName[32] = {0};
    INT1 computeTunnelPortName[32] = {0};

    STL_FOR_LOOP(m_clients, clit)
    {
        CSmartPtr<COvsdbClient>& remote = clit->second;
        remoteIp = inet_htoa(remote->getIp());

        if (TUNNEL_TYPE_GRE == m_tunnelType)
        {
            sprintf(controlTunnelPortName, "gre-%s", computeIp);
            sprintf(computeTunnelPortName, "gre-%s", remoteIp);
        }
        else if (TUNNEL_TYPE_VXLAN == m_tunnelType)
        {
            sprintf(controlTunnelPortName, "vxlan-%s", computeIp);
            sprintf(computeTunnelPortName, "vxlan-%s", remoteIp);
        }

        //control---->compute
        STL_FOR_LOOP(remote->getBridges(), brit)
        {
            ovs_bridge_t& remoteBr = *brit;
            if (strcmp(remoteBr.name, INTERNAL_BR) == 0)
            {
                if (TUNNEL_TYPE_GRE == m_tunnelType)
                {
                    addPortAndPortOption(remote->getSockfd(), remoteBr.uuid, controlTunnelPortName, remoteIp, computeIp, "key", tunnelTypeString[TUNNEL_TYPE_GRE]);
                }
                else if (TUNNEL_TYPE_VXLAN == m_tunnelType)
                {
                    addPortAndPortOption(remote->getSockfd(), remoteBr.uuid, controlTunnelPortName, remoteIp, computeIp, "key", tunnelTypeString[TUNNEL_TYPE_VXLAN]);
                }
                break;
            }
        }

        //compute---->control
        if (TUNNEL_TYPE_GRE == m_tunnelType)
        {
            addPortAndPortOption(compute->getSockfd(), bridge.uuid, computeTunnelPortName, computeIp, remoteIp, "key", tunnelTypeString[TUNNEL_TYPE_GRE]);
        }
        else if (TUNNEL_TYPE_VXLAN == m_tunnelType)
        {
            addPortAndPortOption(compute->getSockfd(), bridge.uuid, computeTunnelPortName, computeIp, remoteIp, "key", tunnelTypeString[TUNNEL_TYPE_VXLAN]);
        }
    }
}

//向句柄对应的ovsdb发送添加port和相关配置的命令
//{"id": "d0d14471-5a66-412c-bcae-0e594f5ee9af","method": "transact","params": ["Open_vSwitch",
//{"op": "mutate","table": "Bridge","where": [["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],
// "mutations": [["ports","insert",["named-uuid","new_port"]]]},
//{"op": "insert","table": "Port","row": {"name": "gre-10.8.1.212","interfaces": ["set",[["named-uuid","new_interface"]]]},"uuid-name": "new_port"},
//{"op": "insert","table": "Interface","row": {"name": "gre-10.8.1.212","options": ["map",[["local_ip","10.8.1.211"],["remote_ip","10.8.1.212"],["key","flow"]]],"type": "gre"},
// "uuid-name": "new_interface"}]}
void COvsdbMgr::addPortAndPortOption(INT4 sockfd, const INT1* brUuid, const INT1* name, 
    const INT1* localIp, const INT1* remoteIp, const INT1* optionKey, const INT1* type)
{
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array,*uuid_array, *named_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(ADD_INTERFACE_OPTION_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    root_array = json_new_array();
    json_insert_child(key, root_array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(root_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("mutate");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //where
    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(brUuid);
    json_insert_child(uuid_array, key);

    //mutations
    key = json_new_string("mutations");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("ports");
    json_insert_child(_uuid_array, key);

    key = json_new_string("insert");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("named-uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string("new_port");
    json_insert_child(uuid_array, key);

    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    //Port
    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Port");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    //interfaces
    key = json_new_string("interfaces");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);

    key = json_new_string("named-uuid");
    json_insert_child(named_array, key);

    key = json_new_string("new_interface");
    json_insert_child(named_array, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_port");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    //interface
    db_obj = json_new_object();
    json_insert_child(root_array, db_obj);

    key = json_new_string("op");
    value = json_new_string("insert");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("name");
    value = json_new_string(name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("options");
    _uuid_array = json_new_array();
    json_insert_child(key, _uuid_array);
    json_insert_child(row_obj, key);

    key = json_new_string("map");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);
    key = json_new_string("local_ip");
    json_insert_child(named_array, key);
    key = json_new_string(localIp);
    json_insert_child(named_array, key);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);
    key = json_new_string("remote_ip");
    json_insert_child(named_array, key);
    key = json_new_string(remoteIp);
    json_insert_child(named_array, key);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);
    key = json_new_string("key");
    json_insert_child(named_array, key);
    key = json_new_string(optionKey);
    json_insert_child(named_array, key);

    key = json_new_string("type");
    value = json_new_string(type);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    INT1 *text = NULL;
    json_tree_to_string(obj, &text);
    json_free_value_all(&obj);

    if (NULL != text)
    {
        if (!sendMsgOut(sockfd, text, strlen(text)))
        {
            CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
            if (worker.isNotNull())
                worker->processPeerDisconn(sockfd);
        }

    	free(text);
    }
}


void COvsdbMgr::replyEcho(INT4 sockfd)
{
    json_t *array, *obj, *key, *value;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string("echo");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("result");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(obj, key);

    INT1 *text = NULL;
    json_tree_to_string(obj, &text);
    json_free_value_all(&obj);

    if (NULL != text)
    {
        if (!sendMsgOut(sockfd, text, strlen(text)))
        {
            CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
            if (worker.isNotNull())
                worker->processPeerDisconn(sockfd);
        }

    	free(text);
    }
}

void COvsdbMgr::handleOpenvswitchTable(INT4 sockfd, const json_t* result)
{
    if ((NULL == result) || (NULL == result->child))
        return;    

    CSmartPtr<COvsdbClient> client = getClient(sockfd);
    if (client.isNull())
        return;    

    json_t* open_vswitch = json_find_first_label(result->child, "Open_vSwitch");
    if ((NULL != open_vswitch) && 
        (NULL != open_vswitch->child) && 
        (NULL != open_vswitch->child->child) &&
        (NULL != open_vswitch->child->child->text))
    {
        json_t* open_vswitch_uuid = open_vswitch->child->child;
        open_vswitch_t& sw = client->getOpenVSwitch();
        memcpy(sw.uuid, open_vswitch_uuid->text, strlen(open_vswitch_uuid->text));
		json_free_value(&open_vswitch);
        LOG_INFO_FMT("open_vswitch ip[%s]uuid[%s]", inet_htoa(client->getIp()), sw.uuid);
    }
}

BOOL COvsdbMgr::handleControllerTable(INT4 sockfd, const json_t *result)
{
    if ((NULL == result) || (NULL == result->child))
        return FALSE;    

    CSmartPtr<COvsdbClient> client = getClient(sockfd);
    if (client.isNull())
        return FALSE;    

    json_t* controller = json_find_first_label(result->child, "Controller");
    if (controller)
    {
    	json_free_value(&controller);
        return TRUE;
    }

    return FALSE;    
}

void COvsdbMgr::handleBridgeTable(INT4 sockfd, const json_t* result, BOOL haveController)
{
    CSmartPtr<COvsdbClient> client = getClient(sockfd);
    if (client.isNull())
        return;    

    client->clearBridges();

    BOOL has_br_int = FALSE;

    if ((NULL == result) || (NULL == result->child))
        return;
    
    //Bridge
    json_t* br = json_find_first_label(result->child, "Bridge");
    if (br)
    {
        if (br && br->child && br->child->child)
        {
            for (json_t* br_uuid = br->child->child; br_uuid; br_uuid = br_uuid->next)
            {
                if ((NULL == br_uuid->text) ||
                    (NULL == br_uuid->child) ||
                    (NULL == br_uuid->child->child) ||
                    (NULL == br_uuid->child->child->child))
                    continue;

                //保存bridge相关信息
                ovs_bridge_t bridge = {0};
                memcpy(bridge.uuid, br_uuid->text, strlen(br_uuid->text));

                json_t* br_name = json_find_first_label(br_uuid->child->child->child, "name");
                if (br_name)
                {
                    if (br_name->child && br_name->child->text)
                    {
                        memcpy(bridge.name, br_name->child->text, strlen(br_name->child->text));
                        if (0 == strcmp(INTERNAL_BR, bridge.name))
                            has_br_int = TRUE;                
                    }
                    json_free_value(&br_name);
                }

                json_t* br_dpid = json_find_first_label(br_uuid->child->child->child, "datapath_id");
                if (br_dpid)
                {
                    if (br_dpid->child && br_dpid->child->text)
                        bridge.dpid = string2Uint8(br_dpid->child->text);
                    json_free_value(&br_dpid);
                }

                saveBridge(sockfd, bridge);
                
                json_t* br_ctrl = json_find_first_label(br_uuid->child->child->child, "controller");
                if (br_ctrl)
                {
                    if (br_ctrl->child && br_ctrl->child->child && br_ctrl->child->child->text)
                        if (!haveController && (0 != strcmp(br_ctrl->child->child->text, "uuid")))
                            setController(sockfd, bridge.uuid);
                    json_free_value(&br_ctrl);
                }

                if (m_ofVersion == OFP10_VERSION)
                {
                    setFailmodAndOfver(sockfd, bridge.uuid, FAIL_MODE_SECURE, OFP_10);
                }
                else if (m_ofVersion == OFP13_VERSION)
                {
                    setFailmodAndOfver(sockfd, bridge.uuid, FAIL_MODE_SECURE, OFP_13);
                }

    			json_t* br_port = json_find_first_label(br_uuid->child->child->child, "ports");
    			if (br_port) 
                {
                    if (br_port->child && br_port->child->child && br_port->child->child->next && br_port->child->child->next->child)
                    {
        				for (json_t* br_p = br_port->child->child->next->child; br_p; br_p = br_p->next) 
                        {
        					//clear all qos info
                            if (br_p->child && br_p->child->next && br_p->child->next->text) 
            					m_qosQueue.clearQosInPortTable(sockfd, br_p->child->next->text);
        					m_qosQueue.clearQosInQosTable(sockfd);
        					m_qosQueue.clearQueueInQueueTable(sockfd);
        				}
                    }
    				json_free_value(&br_port);
    			}
            }
        }

		json_free_value(&br);
    }

    if (!has_br_int) //add br-int
    {
        if (OFP10_VERSION == m_ofVersion)
        {
            addBridge(sockfd, INTERNAL_BR, client->getOpenVSwitch().uuid, FAIL_MODE_SECURE, OFP_10);
        }
        else if (OFP13_VERSION == m_ofVersion)
        {
            addBridge(sockfd, INTERNAL_BR, client->getOpenVSwitch().uuid, FAIL_MODE_SECURE, OFP_13);
        }
    }
}

//{"id":"292174f5-5da9-4d7f-b9a9-1f0b787607c6","method":"transact","params":["Open_vSwitch",
//{"op":"update","table":"Bridge","where":[["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],
//"row":{"fail_mode":["set",["secure"]],"protocols":["set",["OpenFlow13"]]}}]}
void COvsdbMgr::setFailmodAndOfver(INT4 sockfd, const INT1* brUuid, const INT1* failMode, const INT1* ofproto)
{
    json_t *array, *obj, *key, *value, *db_obj, *_uuid_array, *uuid_array, *row_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(SET_FAILMODE_OF_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("transact");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(obj, key);

    key = json_new_string("Open_vSwitch");
    json_insert_child(array, key);

    db_obj = json_new_object();
    json_insert_child(array, db_obj);

    key = json_new_string("op");
    value = json_new_string("update");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("table");
    value = json_new_string("Bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    key = json_new_string("where");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(db_obj, key);

    _uuid_array = json_new_array();
    json_insert_child(array, _uuid_array);

    key = json_new_string("_uuid");
    json_insert_child(_uuid_array, key);

    key = json_new_string("==");
    json_insert_child(_uuid_array, key);

    uuid_array = json_new_array();
    json_insert_child(_uuid_array, uuid_array);

    key = json_new_string("uuid");
    json_insert_child(uuid_array, key);

    key = json_new_string(brUuid);
    json_insert_child(uuid_array, key);

    //row
    key = json_new_string("row");
    row_obj = json_new_object();
    json_insert_child(key, row_obj);
    json_insert_child(db_obj, key);

    key = json_new_string("fail_mode");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(failMode);
    json_insert_child(uuid_array, key);

    key = json_new_string("protocols");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(ofproto);
    json_insert_child(uuid_array, key);

    INT1 *text = NULL;
    json_tree_to_string(obj, &text);
    json_free_value_all(&obj);

    if (NULL != text)
    {
        if (!sendMsgOut(sockfd, text, strlen(text)))
        {
            CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
            if (worker.isNotNull())
                worker->processPeerDisconn(sockfd);
        }

    	free(text);
    }
}

void COvsdbMgr::handleSearchHostByMac(INT4 sockfd, const json_t* result)
{
    json_t *tmp, *row, *ofport, *externalids, *in_use_mac, *attach_mac = NULL;
	//UINT4 port = 0;
	UINT1 host_mac[6] = {0};
	UINT1 phy_mac[6] = {0};

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (sw.isNull())
        return;

	if ((result) && (result->child) && (result->child->child)) 
	{
		tmp = result->child->child;
    	row = json_find_first_label(tmp, "rows");
	}
    if (row)
    {	
        if ((row->child) && (row->child->child))
        {
        	tmp = row->child->child;
    		ofport = json_find_first_label(tmp, "ofport");
            if (ofport)
            {
                //if ((ofport->child) && (ofport->child->text)) 
                //    port = string2Uint4(ofport->child->text);
                json_free_value(&ofport);
            }

    		in_use_mac = json_find_first_label(tmp, "mac_in_use");
    		if (in_use_mac) 
            {
                if ((in_use_mac->child) && (in_use_mac->child->text)) 
        			macstr2hex(in_use_mac->child->text, phy_mac);
    			json_free_value(&in_use_mac);
    		}
    		
    		externalids = json_find_first_label(tmp, "external_ids");
    		if (externalids)
    		{
                if ((externalids->child) && (externalids->child->child) && 
                    (externalids->child->child->next) && (externalids->child->child->next->child) && 
                    (externalids->child->child->next->child->child))
                {
        			tmp = externalids->child->child->next->child->child;
        			if (tmp->text) 
                    {
        				if (0 == strcmp(tmp->text, "attached-mac")) 
                        {
        					attach_mac = tmp->next;
        				}
        				if (attach_mac) 
                        {
        					macstr2hex(attach_mac->text, host_mac);
        				}
        			}
                }
    			json_free_value(&externalids);
    		}
        }
		json_free_value(&row);
    }

/*
	if (port > 0) 
        update_openstack_host_port_by_mac(host_mac, sw, port);
*/
}

//向句柄对应的ovsdb发送检索用的json�?
//{"id":"1b5f9ab0-4abc-40c3-97bc-7ea66e663923","method":"monitor","params":["Open_vSwitch",null,
// {"Bridge":{},"Port":{},"Interface":{},"Controller":{},"Manager":{},"Mirror":{},"NetFlow":{},
//  "Open_vSwitch":{},"QoS":{},"Queue":{},"sFlow":{},"SSL":{},"Flow_Sample_Collector_Set":{},
//  "Flow_Table":{},"IPFIX":{}}]}
void COvsdbMgr::searchAllTableFromOvsdb(INT4 sockfd)
{
    json_t *array, *obj, *key, *value, *table_obj;

    obj = json_new_object();

    key = json_new_string("id");
    value = json_new_string(SEARCH_ALL_TABLE_ID);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("method");
    value = json_new_string("monitor");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("params");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(obj, key);

    value = json_new_string("Open_vSwitch");
    json_insert_child(array, value);

    value = json_new_string("null");
    json_insert_child(array, value);

    value = json_new_object();
    json_insert_child(array, value);

    //Open_vSwitch
    key = json_new_string("Open_vSwitch");
    table_obj = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    //bridge
    key = json_new_string("Bridge");
    table_obj = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    //Controller
    key = json_new_string("Controller");
    table_obj = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    //Controller
    key = json_new_string("QoS");
    table_obj = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    //Controller
    key = json_new_string("Queue");
    table_obj = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

#if 0
    //Port
    key  = json_new_string("Port");
    table_obj   = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    //Interface
    key  = json_new_string("Interface");
    table_obj   = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);

    //Manager
    key  = json_new_string("Manager");
    table_obj   = json_new_object();
    json_insert_child(key, table_obj);
    json_insert_child(value, key);
#endif

    INT1* text = NULL;
    json_tree_to_string(obj, &text);
    json_free_value_all(&obj);
   
    if (NULL != text)
    {
        if (!sendMsgOut(sockfd, text, strlen(text)))
        {
            CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
            if (worker.isNotNull())
                worker->processPeerDisconn(sockfd);
        }

    	free(text);
    }
}


/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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
*   File Name   : cluster-mgr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-27           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "cluster-mgr.h"
#include "redis_client.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../conn-svr/conn-svr.h"
#include "zookeeper.h"
#include "redis_sync.h"
#include "../restful-svr/restful-svr.h"



UINT1 g_cluster_state = CLUSTER_STOP;	
UINT1 g_controller_role = OFPCR_ROLE_EQUAL;     //当前控制器角色
UINT4 g_controller_cnt = 0;                     //当前集群中控制器数
UINT8 g_election_generation_id = 0;             //master选举版本号
UINT8 g_cluster_id;                             //by:yhy 控制器id,被初始化成本控制器的IP地址后便不再改变
INT1  g_zookeeper_server[300] = {0};            //zookeeper服务器地址  ip:port,ip:port,ip:port,ip:port
INT1  g_controllers[MAX_CONTROLLER][30];        //所有控制器的信息
UINT8 g_master_id;                              //by:yhy 当前master的id.可以从zookeeper的SDN_MASTER_PATH中读取,也可以从Hbase的MASTER_ID中读取
UINT4 g_startup_time_delay = 10;				//by:yhy master宕机后重启延时时间

static zhandle_t * g_zkhandler = NULL;         //zookeep client hanlder
//static controller_cluster_t g_controller_cluster[CLUSTER_NUM];     //???????????????

const INT1* SDN_ROOT_PATH 		= "/sdn_root";
const INT1* SDN_NODE_PATH 		= "/sdn_root/sdn_node";
const INT1* SDN_DATA_PATH 		= "/sdn_root/sdn_data";
const INT1* SDN_ELEC_PATH 		= "/sdn_root/sdn_data/election_generation_id";
const INT1* SDN_MASTER_PATH 	= "/sdn_root/sdn_data/master_id";
const INT1* SDN_TOPO_VER_PATH 	= "/sdn_root/sdn_data/topology_version_id";

//by:yhy 更新控制器本地,Hbase,Zoo,控制器所接交换机中的与控制器主从身份有关的信息
//       重点是主机身份的菜更新信息
INT4 update_role(UINT4 role)
{
    if (0 == g_is_cluster_on)
    {//by:yhy 若是从机
        return GN_OK;
    }

    if(OFPCR_ROLE_MASTER == role)
    {//by:yhy 若是主机则先备份数据至Hbase
        persist_fabric_all();
    }

    if((role == g_controller_role) || (NULL == g_zkhandler))
    {
        return GN_OK;
    }

    INT4 ret = 0;
    UINT4 idx_sw = 0;
    role_req_info_t role_req_info;
    gn_switch_t *sw = NULL;
    //获取最新的generation_id
    if(OFPCR_ROLE_MASTER == role)
    {//by:yhy 主机身份,更新相关ID
        INT4 len = TABLE_STRING_LEN;
        INT1 election_generation[TABLE_STRING_LEN] = {0};
        INT1 master_id[TABLE_STRING_LEN] = {0};
		//by:yhy 主机身份,更新本地g_election_generation_id,更新Hbase和Zoo中SDN_ELEC_PATH,SDN_MASTER_PATH
        g_master_id = g_cluster_id;
        if(ZOK != zoo_get(g_zkhandler, SDN_ELEC_PATH, 0, election_generation, &len, NULL))
        {
            g_election_generation_id = strtoull(election_generation, 0, 10);
        }
        else
        {
            query_value(ELECTION_GENERATION_ID, election_generation);
            g_election_generation_id = strtoull(election_generation, 0, 10);
        }

        snprintf(election_generation, TABLE_STRING_LEN, "%llu", g_election_generation_id);
        snprintf(master_id, TABLE_STRING_LEN, "%llu", g_master_id);

        zoo_set(g_zkhandler, SDN_ELEC_PATH, election_generation, strlen(election_generation), -1);
        zoo_set(g_zkhandler, SDN_MASTER_PATH, master_id, strlen(master_id), -1);

        persist_value(ELECTION_GENERATION_ID, election_generation);
        persist_value(MASTER_ID, master_id);
    }
    else if(OFPCR_ROLE_SLAVE == role)
    {//无

    }
    else if(OFPCR_ROLE_EQUAL == role)
    {//无

    }
    else
    {//返回 身份错误
        return EC_ROLE_INVALID;
    }

	//by:yhy 只有主机身份的控制器,程序才能运行至此
    g_controller_role = role;
    role_req_info.generation_id = g_election_generation_id;
    role_req_info.role = role;
	//by:yhy 遍历该控制器下所有交换机,更新自己的主机身份
    for(idx_sw = 0; idx_sw < g_server.max_switch; idx_sw++)
    {
        sw = &g_server.switches[idx_sw];
        if(CONNECTED == sw->conn_state)
        {
            ret = sw->msg_driver.msg_handler[OFPT13_ROLE_REQUEST](sw, (UINT1 *)&role_req_info);
        }
    }

    return ret;
}
//by:yhy zoo_acreate,zoo_aset的回调函数
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
        LOG_PROC("WARNING", "%s failed, node already exist", opr_name);
        break;
    default:
        LOG_PROC("ERROR", "%s failed. ret_code: %d", opr_name, rc);
    }
}
//by:yhy zoo_acreate的回调函数
void cb_string_completion(INT4 rc, const INT1 *value, const void *data)
{
    zookeeper_error_msg(rc, data);
}
//by:yhy zoo_aset的回调函数
void cb_stat_completion(int rc, const struct Stat *stat,const void *data)
{
    zookeeper_error_msg(rc, data);
}
//by:yhy zookeeper_init的回调函数
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

void update_controller_info(const struct String_vector *strings)
{
    UINT4 idx = 0;
    UINT8 node_id = 0;
    for (idx=0; idx <strings->count; idx++)
    {
        node_id = strtoull(strings->data[idx], 0, 10);
        if (node_id == g_master_id)
        {
            persist_controller(htonl(node_id), g_restful_port, "Master");
        }
        else
        {
            persist_controller(htonl(node_id), g_restful_port, "Slave");
        }
    }
}

//by:yhy 回调函数 此处也会切换主从
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
			//by:yhy 更新当前集群中控制器数量 
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
                update_controller_info(strings);
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
//by:yhy ?
//观察者函数.当被观察对象发生改变,客户端回收到通知
void wt_get_children2(zhandle_t *zh, INT4 type, INT4 state, const INT1 *path, void *watcherCtx)
{
	INT4 ret = 0;
	INT1 node_name[100];
    if (ZOO_CONNECTED_STATE == state)
    {
        if (ZOO_CHILD_EVENT == type)
        {
			
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
	else if(ZOO_EXPIRED_SESSION_STATE == state)
	{
		if(ZOO_SESSION_EVENT == type)
		{
			zookeeper_close(g_zkhandler);
			//初始化，连接zookeeper服务器
		    g_zkhandler = zookeeper_init(g_zookeeper_server, wt_zookeeper_init, 10000, 0, 0, 0);
		    if (g_zkhandler == NULL)
		    {
		        LOG_PROC("ERROR", "Connecting to zookeeper servers [%s] failed", g_zookeeper_server);
		        return ;
		    } 
			ret = zoo_awget_children2(g_zkhandler, SDN_NODE_PATH, wt_get_children2, 0, cb_awget_children2, "Get children");
            if (ret)
            {
                LOG_PROC("ERROR", "Start watch children failed. --ret code: %d", ret);
                return;
            }
		   
			snprintf(node_name, 100, "%s/%llu", SDN_NODE_PATH, g_cluster_id);
		    ret = zoo_acreate(g_zkhandler, node_name, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, cb_string_completion, "Create local node");
		    if (ret)
		    {
		        LOG_PROC("ERROR", "Create sdn local node failed, ret_code: %d", ret);
		        return ;
		    }
			
		}
	}
}
//by:yhy zookeeper 主从切换在此执行
void cb_awget_master_id (int rc, const char *value, int value_len,const struct Stat *stat, const void *data)
{
	char buff[128] = {0};
	int buff_len = 128;
	INT1 controller_ip[TABLE_STRING_LEN] = {0};

	number2ip(htonl(g_cluster_id), controller_ip);

    if(ZOK == rc)
	{
		UINT8 node_id;

		if(ZOK == zoo_get(g_zkhandler, SDN_MASTER_PATH, 0, buff, &buff_len, 0))
		{
			node_id = strtoull(buff, 0, 10);
		}
		else
		{
			LOG_PROC("WARNING", "master id changed but read from zoo failed");
			return;
		}
		
		if(node_id != -2)
		{
			g_cluster_state = CLUSTER_SETUP;
		}
		else
		{
			g_cluster_state = CLUSTER_STOP;
			update_role(OFPCR_ROLE_EQUAL);
			return;
		}

		if(g_cluster_state == CLUSTER_SETUP)
		{
			if(node_id == g_cluster_id)
			{
				if(OFPCR_ROLE_MASTER != g_controller_role)
				{
					g_master_id = g_cluster_id;
					update_role(OFPCR_ROLE_MASTER);
				}
			}
			else
			{
				update_role(OFPCR_ROLE_SLAVE);
			}
		}	
		
	}
    zookeeper_error_msg(rc, data);
}
//by:yhy zookeeper 主从切换在此执行  注意处理过程
void wt_get_master_id(zhandle_t *zh, INT4 type, INT4 state, const INT1 *path, void *watcherCtx)
{
    if (ZOO_CONNECTED_STATE == state)
    {
        if (ZOO_CHANGED_EVENT == type)
        {
            INT4 ret = 0;
			//by:yhy 处理过程
            ret = zoo_awget(g_zkhandler, SDN_MASTER_PATH, wt_get_master_id, 0, cb_awget_master_id, "Get master");
            if (ret)
            {
                LOG_PROC("ERROR", "Start watch master idfailed. --ret code: %d", ret);
                return;
            }
        }
        else if(ZOO_DELETED_EVENT == type)
        {
            LOG_PROC("WARNING", "Path %s was deleted", path);

        }
        else if (ZOO_CREATED_EVENT == type)
        {
            LOG_PROC("WARNING", "Path %s was created", path);
        }
        else
        {
            LOG_PROC("WARNING", "Node %s watch failed", path);
        }
    }
}

//by:yhy zoo_aget_children2的回调函数
void cb_get_children2(int rc, const struct String_vector *strings, const struct Stat *stat, const void *data)
{
    if(ZOK == rc)
    {
        if (strings)
        {
            if (g_cluster_id == g_master_id && strings->count >= 1)
            {
                *((INT1*)data) = 1;
                return;
            }
        }
    }
    *((INT1*)data) = 0;
}

//by:yhy 判断是否选举完毕
//若选举完毕,则更新g_master_id 
void is_election_finished(zhandle_t * g_zkhandler)
{
    INT1 flag = -1;
    INT4 ret = zoo_aget_children2(g_zkhandler, SDN_NODE_PATH, 0, cb_get_children2, &flag);
    INT4 count = 3000; //等待cb_get_children2结果，最多300s
    if (!ret)
    {
        while (0 != flag && 1 != flag && count > 0)
        {
            usleep(100000);
            count--;
            continue;
        }

        if (flag)
        {
            LOG_PROC("WARNING", "%u seconds delay, waitting for last election to be completed.", g_startup_time_delay);
            //sleep(g_startup_time_delay);
            MsecSleep(g_startup_time_delay*1000);
            
            INT4 len = TABLE_STRING_LEN;
            INT1 value[TABLE_STRING_LEN] = {0};
			//by:yhy 从SDN_MASTER_PATH获取master_id
            zoo_get(g_zkhandler, SDN_MASTER_PATH, 0, value, &len, NULL);
            g_master_id = strtoull(value, 0, 10);
        }
    }
    else
    {
        LOG_PROC("WARNING", "Get zookeeper sdn nodes failure!");
    }
}


UINT4 set_master_id(UINT8 controller_id)
{	
	struct String_vector  *strings = NULL;
	int     i = 0;
	int     flag = 0;
	INT1    node_name[100] = {0};
	INT4    ret = 0;
	UINT8   node_id = 0;

    strings = (struct String_vector *)malloc(sizeof(struct String_vector));
    zoo_get_children(g_zkhandler, SDN_NODE_PATH, 1, strings);  
    if(strings)
    {
        for(i = 0; i < strings->count; i++)
        {
        	node_id = strtoull(strings->data[i], 0, 10);
        	if (node_id == controller_id)
            {
            	flag = 1;
                break;
            }
        }
    }

    free(strings);
    
    if (!flag)
    {
    	return GN_ERR;
    }

    sprintf(node_name,"%llu", controller_id);
	ret = zoo_aset(g_zkhandler, SDN_MASTER_PATH,node_name,strlen(node_name),-1,cb_stat_completion, "Set node");
	if (ret)
    {
        LOG_PROC("ERROR", "Set master id failed, ret_code: %d", ret);
        return GN_ERR;
    }
    
    return GN_OK;
}
//by:yhy 更新控制器当前主从身份
//      北向接口
int update_controllers_role(UINT4 controller_id, UINT1 mode)
{	
	int     i = 0;
    struct String_vector  *strings = NULL;
    UINT4   app_master_id = -1;
    UINT4   min_node_id = -1;
	INT1    controller_ip[32] = {0};
	UINT8   node_id = 0;
	INT1 master_controller_id[TABLE_STRING_LEN] = {0};


	if (0 == g_is_cluster_on)
    {
        return GN_OK;
    }
	query_value(CUSTOM_MASTER_ID, master_controller_id);
	
    strings = (struct String_vector *)malloc(sizeof(struct String_vector));
    zoo_get_children(g_zkhandler, SDN_NODE_PATH, 1, strings);  
    if(strings)
    {
    	if(!strings->count || MAX_CONTROLLER < strings->count)
    	{
    		return GN_ERR;
    	}
    	
    	if(mode == 1)
    	{
    		if(g_cluster_state == CLUSTER_STOP)
    		{
    			for(i=0 ; i< strings->count ; i++)
    			{
    				node_id = strtoull(strings->data[i], 0, 10);
    				number2ip(htonl(node_id), controller_ip);
    				if(node_id == ntohl(controller_id))
    				{
						reset_controller_role(controller_ip, "Equal");					
						zoo_set(g_zkhandler, SDN_MASTER_PATH, "-2", 2, -1);
    				}
    			}
    		}
    		else if(g_cluster_state == CLUSTER_SETUP)
    		{
    			for(i=0 ; i< strings->count ; i++)
    			{
    				node_id = strtoull(strings->data[i], 0, 10);
    				number2ip(htonl(node_id), controller_ip);
					if (GN_ERR == IsInDB(controller_ip))
					{
						continue;
					}

	                if (node_id == g_master_id)
	                {
	                	if(node_id != g_cluster_id)
	                	{
	                        min_node_id = node_id;
	                        break;
	                     }
	                }

	                if(node_id == app_master_id)
	                {
	                    min_node_id = node_id;
	                    break;
	                }

	                if(node_id < min_node_id)
	                {
	                    min_node_id = node_id;
	                }
    			}

    			for(i=0 ; i< strings->count ; i++)
    			{
    				node_id = strtoull(strings->data[i], 0, 10);
    				number2ip(htonl(node_id), controller_ip);
    				if (GN_ERR == IsInDB(controller_ip))
	                {
	                	continue;
	                }

	                if(node_id == min_node_id)
	                {

						reset_controller_role(controller_ip, "Master");
						set_master_id(node_id);
					}
					else
					{
						reset_controller_role(controller_ip, "Slave");
					}
    			}
    		}
    	}
    	else if( mode == 0)
    	{
    		if(g_cluster_state == CLUSTER_SETUP)
    		{
    			for(i=0 ; i< strings->count ; i++)
    			{
					node_id = strtoull(strings->data[i], 0, 10);
    				number2ip(htonl(node_id), controller_ip);
					if (GN_ERR == IsInDB(controller_ip))
	                {
	                	continue;
	                }
                    
	                if(controller_id == node_id)
	                {
	                	continue;
	                }

	                if (node_id == g_master_id)
	                {
	                	if(node_id != g_cluster_id)
	                	{
	                        min_node_id = node_id;
	                        break;
	                     }
	                }

	                if(node_id == app_master_id)
	                {
	                    min_node_id = node_id;
	                    break;
	                }

	                if(node_id < min_node_id)
	                {
	                    min_node_id = node_id;
	                }
    			}

    			for(i=0 ; i< strings->count ; i++)
    			{
    				node_id = strtoull(strings->data[i], 0, 10);
    				number2ip(htonl(node_id), controller_ip);
    				if (GN_ERR == IsInDB(controller_ip))
	                {
	                	continue;
	                }

	                if(controller_id == node_id)
	                {
	                	continue;
	                }
	                

	                if(node_id == min_node_id)
	                {
						reset_controller_role(controller_ip, "Master");
						set_master_id(node_id);
					}
					else
					{
						reset_controller_role(controller_ip, "Slave");
					}
    			}
    		}
    	}
    }
    
    free(strings);
    
    return 0;
}
//by:yhy 设置控制前当前主从身份
//      北向接口
UINT4 set_cluster_role(UINT8 controller_id)
{
	INT4 idx = 0;
	UINT8 node_id = 0;
	struct String_vector  *strings = NULL;
	INT1 controller_ip[32] = {0};

	if (0 == g_is_cluster_on)
    {
        return GN_OK;
    }
	strings = (struct String_vector *)malloc(sizeof(struct String_vector));
	zoo_get_children(g_zkhandler, SDN_NODE_PATH, 1, strings);  
	if (strings  && g_cluster_state ==CLUSTER_SETUP)
	{
    	if(!strings->count) return GN_ERR;
    	
		for(idx = 0; idx < strings->count; idx++)
		{
			node_id = strtoull(strings->data[idx], 0, 10);
			number2ip(htonl(node_id), controller_ip);
			if (node_id == controller_id)
			{			
				reset_controller_role(controller_ip, "Master");
			}
			else
			{
				reset_controller_role(controller_ip, "Slave");		
			}
		}

		if (g_cluster_id == controller_id)
		{
			update_role(OFPCR_ROLE_MASTER);
		}
		else
		{
			update_role(OFPCR_ROLE_SLAVE);
			
		    set_master_id(controller_id);
		}
	}
	else if(g_cluster_state ==CLUSTER_SETUP)
	{
		set_master_id(controller_id);
	}
	free(strings);
	return GN_OK;
}
//by:yhy 开启或关闭控制器主从切换
//      北向接口
UINT4 set_cluster_onoff(UINT4 onoff)
{
	int     i = 0;
    struct String_vector  *strings = NULL;
    UINT4   master_id = 0;
    UINT4   min_node_id = -1;
	UINT8   node_id = 0;	
	INT1    controller_ip[32] = {0};
	
	INT1    master_controller_ip[TABLE_STRING_LEN] = {0};

	if (0 == g_is_cluster_on)
    {
        return GN_OK;
    }
	
	query_value(CUSTOM_MASTER_ID, master_controller_ip);
	master_id = strtoull(master_controller_ip, 0, 10);
	
    strings = (struct String_vector *)malloc(sizeof(struct String_vector));
    zoo_get_children(g_zkhandler, SDN_NODE_PATH, 1, strings);  
    if(strings)
    {
    	if(!strings->count) return GN_ERR;

        for(i = 0; i < strings->count; i++)
        { 
        	node_id = strtoull(strings->data[i], 0, 10);
        	number2ip(htonl(node_id), controller_ip);
	
        	if(onoff == CLUSTER_STOP)
        	{    		
				reset_controller_role(controller_ip, "Equal");
        		zoo_set(g_zkhandler, SDN_MASTER_PATH, "-2", 2, -1);
        	}
        	else if(onoff == CLUSTER_SETUP)
        	{
				if(GN_ERR == IsInDB(controller_ip))
				{
					continue;
				}
                
				if (node_id == master_id || node_id == g_master_id)
				{
					min_node_id = node_id;
	    			break;
				}
	    		else if( node_id < min_node_id)
	    		{
	    			min_node_id = node_id;
	    		}
        	}
        }

		if(onoff == CLUSTER_SETUP)
		{
			for(i = 0; i < strings->count ; i++)
			{
				node_id = strtoull(strings->data[i], 0, 10);
        		number2ip(htonl(node_id), controller_ip);
				if(GN_ERR == IsInDB(controller_ip))
				{
					continue;
				}
 		
				if(min_node_id == node_id)
				{

					reset_controller_role(controller_ip, "Master");
        			set_master_id(node_id);
        		}
        		else
        		{
        			reset_controller_role(controller_ip, "Slave");
        		}
			}

			if(g_cluster_id == min_node_id)
			{
				update_role(OFPCR_ROLE_MASTER);
			}
			else
			{
				update_role(OFPCR_ROLE_SLAVE);
			}
		}
		else if (onoff == CLUSTER_STOP)
		{
			update_role(OFPCR_ROLE_EQUAL);
		}
    }

    free(strings);
	g_cluster_state = onoff;
	return GN_OK;
}
//by:yhy 获取控制器当前主从身份
//      北向接口
INT4 get_controller_status(UINT4 tmp_id)
{
	int     i = 0;
    struct String_vector  *strings = NULL;
    UINT4   node_id = 0;
    strings = (struct String_vector *)malloc(sizeof(struct String_vector));
	memset(strings, 0, sizeof(struct String_vector));
    int flag = 0;

	if (0 == g_is_cluster_on)
    {
        return 0;
    }
    zoo_get_children(g_zkhandler, SDN_NODE_PATH, 1, strings);  
    if(strings)
    {
    	if (!strings->count) return GN_ERR;
    		
        for(i = 0; i < strings->count; i++)
        { 
        	node_id = atoll(strings->data[i]);
        	if(node_id == tmp_id) 
        	{		
        		flag = 1;
        	}
        }
    }

    free(strings);

    return flag;
}


//by:yhy 初始化集群服务
INT4 init_cluster_mgr()
{
    INT1 node_name[100];
    INT1 value[TABLE_STRING_LEN] = {0};
    UINT4 timeout = 10000; //100ms->10s
    INT4 len = 0;
    INT4 ret = 0;
    INT1 *conf_value = NULL;

    g_cluster_id = g_controller_ip;
    conf_value = get_value(g_controller_configure, "[cluster_conf]", "zoo_server");
    NULL == conf_value ? strncpy(g_zookeeper_server, "0", 300 - 1) : strncpy(g_zookeeper_server, conf_value, 300 - 1);

    if (0 == strcmp("0", g_zookeeper_server))
    {
        LOG_PROC("WARNING", "No zookeeper server was set");
        return GN_ERR;
    }

    conf_value = get_value(g_controller_configure, "[cluster_conf]", "startup_time_delay");
    g_startup_time_delay = (NULL == conf_value) ? 10 : atoll(conf_value);
	//by:yhy 下面开始zookeeper的相关操作
    //zookeeper日志级别设置
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);

    //初始化，连接zookeeper服务器
    g_zkhandler = zookeeper_init(g_zookeeper_server, wt_zookeeper_init, timeout, 0, 0, 0);
    if (g_zkhandler == NULL)
    {
        LOG_PROC("ERROR", "Connecting to zookeeper servers [%s] failed", g_zookeeper_server);
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

    //read all the important info from redis or zookeeper
    g_election_generation_id = 0;
    g_master_id = 0;

    query_value(ELECTION_GENERATION_ID, value);
    g_election_generation_id = strtoull(value, 0, 10);

    query_value(MASTER_ID, value);
    g_master_id = strtoull(value, 0, 10);

    memset(value, 0, TABLE_STRING_LEN);
	//获取sdn_data/election_generation_id节点
    if(ZOK == zoo_get(g_zkhandler, SDN_ELEC_PATH, 0, value, &len, NULL))
    {
        g_election_generation_id = strtoull(value, 0, 10);
    }
	//获取"/sdn_root/sdn_data/master_id"节点
    if(0 == g_master_id)
    {//by:yhy 如果未获得主机ID,则获取主机ID
        INT4 len = TABLE_STRING_LEN;
        zoo_get(g_zkhandler, SDN_MASTER_PATH, 0, value, &len, NULL);
        g_master_id = strtoull(value, 0, 10);
    }
    
    //by:yhy 删除"/sdn_root/sdn_node"中旧临时节点
    snprintf(node_name, 100, "%s/%llu", SDN_NODE_PATH, g_cluster_id);
    zoo_delete(g_zkhandler, node_name, -1);
    
    //如果当前master id是自己并且存在正在运行的其他节点，则延迟10s启动
    is_election_finished(g_zkhandler);


	//by:yhy 主从身份在下述两块中执行	
    //by:yhy 开始监控所有节点 当"/sdn_root/sdn_node"发生改变时,wt_get_children2会收到通知
    ret = zoo_awget_children2(g_zkhandler, SDN_NODE_PATH, wt_get_children2, 0, cb_awget_children2, "Get children");
    if (ret)
    {
        LOG_PROC("ERROR", "Start watch children failed, ret_code: %d", ret);
        return GN_ERR;
    }

	//by:yhy 监控master_id 当"/sdn_root/sdn_data/master_id"发生改变时,wt_get_master_id会收到通知
    ret = zoo_awget(g_zkhandler, SDN_MASTER_PATH, wt_get_master_id, 0, cb_awget_master_id, "Get master");  
	if(ret)
    {
    	LOG_PROC("ERROR", "Start watch master id failed, ret_code: %d", ret);
        return GN_ERR;
    }

	
    //创建一个临时节点
    ret = zoo_acreate(g_zkhandler, node_name, "alive", 5, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, cb_string_completion, "Create local node");
    if (ret)
    {
        LOG_PROC("ERROR", "Create sdn local node failed, ret_code: %d", ret);
        return GN_ERR;
    }
	if(0 == zoo_state(g_zkhandler)) //connected closed
	{
		LOG_PROC("ERROR", "Connecting to zookeeper servers [%s] failed",g_zookeeper_server);
		return GN_ERR;
	}
    return GN_OK;
}

//by:yhy 注销主从
void fini_cluster_mgr()
{
    zookeeper_close(g_zkhandler);
}

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
*   File Name   : redis_client.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-20           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "../conn-svr/conn-svr.h"
#include "../topo-mgr/topo-mgr.h"
#include "../../inc/timer.h"
#include "cluster-mgr.h"
#include "redis_client.h"
#include "openflow-common.h"
#include "fabric_impl.h"
#include "redis_sync.h"
#include "hiredis.h"
#include "epoll_svr.h"

#define REDIS_PORT   6379

UINT4 MAX_FILED_NUM = 204800*10;
UINT4 TABLE_DATA_LEN = 204800*10;

field_pad_t* g_filed_pad_master = NULL;
//by:yhy 在reslove_data中更改,表示在reslove_data中提取到有效数据
field_pad_t* g_filed_pad_slave = NULL;
INT1* g_table_data_buf = NULL;
//by:yhy master控制器每次在网HBase中更新一条数据时,就会将此自增1
UINT4 g_filed_num_master = 0;
//by:yhy 在reslove_data中更改,表示在reslove_data中提取到有效数据的个数
UINT4 g_filed_num_slave  = 0;
UINT8 g_trans_seq_master = 0;
UINT8 g_trans_seq_slave = 0;
void* g_sync_timer = NULL;

INT1 g_redis_ip[32];
INT1 g_redis_port[16];

//by:yhy latest version of topology
int g_topology_version = -1;



UINT1   g_method_DBSyn = 0;
pthread_mutex_t g_redisMutex;
redisContext *g_redisCtx = NULL;

INT4 persist_controller(UINT4 ip, UINT2 port, INT1* role)
{
	INT1 key_tmp[TABLE_STRING_LEN] = {0};
	redisReply *reply = NULL;
	stControllerInfo stControlNodeInfo = {0};
	snprintf(key_tmp, TABLE_STRING_LEN, "SFabric_%llu", ip);

	stControlNodeInfo.uiIp = ip;
	stControlNodeInfo.uiPort = port;
	strcpy(stControlNodeInfo.ucRole, role);

	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"SET %s %b",  key_tmp, (UINT1 *)&stControlNodeInfo , (size_t)sizeof(stControllerInfo));
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);
	return GN_OK;
}
INT4 deletet_controller(INT1* nodekey)
{
	INT1 key_tmp[TABLE_STRING_LEN] = {0};
	redisReply *reply = NULL;
	snprintf(key_tmp, TABLE_STRING_LEN, "SFabric_%s", nodekey);
	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"DEL %s", key_tmp);
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	freeReplyObject(reply);	
	pthread_mutex_unlock(&g_redisMutex);
	return GN_OK;
}
INT4 query_controller_all(INT1* value)
{
	UINT4 idx = 0;
	BOOL  bResult = FALSE;
	INT1 *str = NULL;
	UINT1 *strpos_ptr=NULL;  
	INT1 key_tmp[TABLE_STRING_LEN] = {0};
	INT1  value_tmp[TABLE_STRING_LEN] = {0};
	redisReply *reply = NULL;
	stControllerInfo stControlNodeInfo = {0};
	snprintf(key_tmp, TABLE_STRING_LEN , "SFabric");
	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"KEYS *");
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	if(reply->elements)
	{
		str = value;
		for(idx=0; idx < reply->elements; idx ++)
		{
			if((reply->element[idx]->len > strlen(key_tmp))&&(0 == memcmp(key_tmp, reply->element[idx]->str, strlen(key_tmp))))
			{
				if(NULL != strtok_r(reply->element[idx]->str, "_", &strpos_ptr))
				{
					//value_tmp
					sprintf(value_tmp, "%s-", strpos_ptr);
	        		strcat(str, value_tmp);
					bResult = TRUE;
				}
			}
		}
		str[strlen(str)-1] = '\0';
	}
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);
	
	return bResult ? GN_OK: GN_ERR;

}
INT4 query_controller_port(INT1* nodekey, INT1* value)
{
	INT1 key_tmp[TABLE_STRING_LEN] = {0};
	INT1 portvalue_tmp[TABLE_STRING_LEN] = {0};
	redisReply *reply = NULL;
	stControllerInfo stControlNodeInfo = {0};
	snprintf(key_tmp, TABLE_STRING_LEN,  "SFabric_%s", nodekey);
	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"GET %s",key_tmp);
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	
	if( reply->len)
	{
		memcpy(&stControlNodeInfo, reply->str , reply->len);
		snprintf(portvalue_tmp,TABLE_STRING_LEN, "%d",stControlNodeInfo.uiPort);
		strcpy(value, portvalue_tmp); //----
	}
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);
	return GN_OK;
}
INT4 query_controller_role(INT1* nodekey, INT1* value)
{
	INT1 key_tmp[TABLE_STRING_LEN] = {0};
	redisReply *reply = NULL;
	stControllerInfo stControlNodeInfo = {0};
	snprintf(key_tmp, TABLE_STRING_LEN, "SFabric_%s", nodekey);
	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"GET %s",key_tmp );
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	if( reply->len)
	{
		memcpy(&stControlNodeInfo, reply->str , reply->len);
		//memcpy(value, &stControlNodeInfo.ucRole, TABLE_STRING_LEN);
		strcpy(value, stControlNodeInfo.ucRole);
	}
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);
	return GN_OK;
}
INT4 reset_controller_role(INT1* nodekey, INT1* value)
{
	BOOL  bResult = FALSE;
	
	INT1 key_tmp[TABLE_STRING_LEN] = {0};
	redisReply *reply = NULL;
	stControllerInfo stControlNodeInfo = {0};
	snprintf(key_tmp, TABLE_STRING_LEN, "SFabric_%s", nodekey);

	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"GET %s", key_tmp);
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	if( reply->len)
	{
		memcpy(&stControlNodeInfo, reply->str , reply->len);
		memset(stControlNodeInfo.ucRole, 0x00, TABLE_STRING_LEN);
		strcpy(stControlNodeInfo.ucRole, value);
		bResult = TRUE;
	}
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);

	if(bResult)
	{
		
		pthread_mutex_lock(&g_redisMutex);
		reply = redisCommand(g_redisCtx,"SET %s %b",  key_tmp, (UINT1 *)&stControlNodeInfo , (size_t)sizeof(stControllerInfo));
		if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
		{
			if(g_redisCtx->err)
			{
				LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
			}
			if(NULL != reply)
				freeReplyObject(reply);
			pthread_mutex_unlock(&g_redisMutex);
			return GN_ERR;
		}
		
		freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_OK;
	}
	return GN_ERR;
}

//by:yhy 在hbase中保存key项的值value
INT4 persist_value(const INT1* key, const INT1* value)
{
	redisReply *reply = NULL;
	if(!g_method_DBSyn)
	{
		
		LOG_PROC("ERROR", "redis server not start or configure error ! %s", FN);
		return GN_ERR;
	}
	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"SET %s %s",  key, value );
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);
	return GN_OK;
}

//by:yhy 在hbase中查找key项的值并传给value
INT4 query_value(const INT1* key, INT1* value)
{
	redisReply *reply = NULL;
	
	if(!g_method_DBSyn)
	{
		
		LOG_PROC("ERROR", "redis server not start or configure error ! %s", FN);
		return GN_ERR;
	}
	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"GET %s",  key);
	
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	
	if( reply->len)
	{
		memcpy(value, reply->str , reply->len);
	}
	freeReplyObject(reply);
	
	pthread_mutex_unlock(&g_redisMutex);
	return GN_OK;
}



INT4 IsInDB(INT1* nodekey)
{
	UINT4 idx = 0;
	INT1 key_tmp[TABLE_STRING_LEN] = {0};
	redisReply *reply = NULL;
	stControllerInfo stControlNodeInfo = {0};

	if(!g_method_DBSyn)
	{
		
		LOG_PROC("ERROR", "redis server not start or configure error ! %s", FN);
		return GN_ERR;
	}
	
	snprintf(key_tmp, TABLE_STRING_LEN , "SFabric_%s", nodekey);
	
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"KEYS *");
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))	
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	
	if(reply->elements)
	{
		for(idx=0; idx < reply->elements; idx ++)
		{
			if(0 == strcmp(key_tmp, reply->element[idx]->str ))
			{
				freeReplyObject(reply);
				pthread_mutex_unlock(&g_redisMutex);
				return GN_OK;
			}
		}
	}
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);
	return GN_ERR;
}
//by:yhy 数据持久化  将数据存入Hbase
//table name(表名):CONTROLLER_DATA_TABLE 
//column family:CONTROLLER_DATA_TABLE_CF
//row(行):g_trans_seq_master
//value(值):node_type,operate_type,field_pad_p
INT4 persist_data(UINT4 node_type, UINT4 operate_type, INT4 num, field_pad_t* field_pad_p)
{
	tlv_t* pTlv = NULL;
	INT4 idx = 0;
	INT4 status = 0;
	redisReply *reply = NULL;
	stCommMsgNode_header *pstCommHeader = NULL;
	stSynMsgNode_header  *pstSynNodeHeader = NULL;

	if(!g_method_DBSyn)
	{
		
		LOG_PROC("ERROR", "redis server not start or configure error ! %s", FN);
		return GN_ERR;
	}
	
    memset(g_table_data_buf, 0, TABLE_DATA_LEN);
	pstCommHeader = (stCommMsgNode_header *)g_table_data_buf;
	pstCommHeader->uiMsgLen = sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header) + num*sizeof(field_pad_t);
	pstCommHeader->uiMsgNodeType = node_type;

	pstSynNodeHeader= (stSynMsgNode_header *)(g_table_data_buf + sizeof(stCommMsgNode_header) );
	pstSynNodeHeader->ucSynMsgType = node_type;
	pstSynNodeHeader->ucOperMsgType = operate_type;
	pstSynNodeHeader->uiSynMsgLen =  num*sizeof(field_pad_t);
	pTlv = (tlv_t*)(g_table_data_buf + sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header));
	for (idx = 0; idx < num; idx++)
	{
		pTlv->type = field_pad_p[idx].type;
		pTlv->len  = field_pad_p[idx].len;
		memcpy(pTlv->data, field_pad_p[idx].pad, field_pad_p[idx].len);

		if (idx < num-1)  pTlv = (tlv_t*)(pTlv->data + pTlv->len);			
	}
	pthread_mutex_lock(&g_redisMutex);
	reply = redisCommand(g_redisCtx,"SET %d %b", node_type, g_table_data_buf, (size_t) pstCommHeader->uiMsgLen );
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);
	

	pthread_mutex_lock(&g_redisMutex);
	g_trans_seq_master++;
	reply = redisCommand(g_redisCtx,"SET %s %lld", TRANS_SEQ,  g_trans_seq_master);
	if((NULL == reply)||(REDIS_REPLY_NIL == reply->type)||(REDIS_REPLY_ERROR == reply->type))
	{
		if(g_redisCtx->err)
		{
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
		}
		if(NULL != reply)
				freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex);
		return GN_ERR;
	}
	freeReplyObject(reply);
	pthread_mutex_unlock(&g_redisMutex);
	return GN_OK;
}
//by:yhy 覆盖,恢复 当前控制器的核心数据
void recovery_data(UINT4 node_type, UINT4 operate_type, INT4 num, field_pad_t* field_pad_p)
{
	LOG_PROC("HBASE", "%s -- START",FN);
    switch (node_type)
    {
    case FABRIC_SW_NODE:
		LOG_PROC("HBASE", "%s -- FABRIC_SW_NODE",FN);
        recover_fabric_sw_list(num, field_pad_p);
        break;
    case FABRIC_HOST_NODE:
		LOG_PROC("HBASE", "%s -- FABRIC_HOST_NODE",FN);
        recover_fabric_host_list(num, field_pad_p);
        break;
    case OPENSTACK_EXTERNAL_NODE:
		LOG_PROC("HBASE", "%s -- OPENSTACK_EXTERNAL_NODE",FN);
        recover_fabric_openstack_external_list(num, field_pad_p);
        break;
    case NAT_ICMP:
		LOG_PROC("HBASE", "%s -- NAT_ICMP",FN);
        recover_fabric_nat_icmp_iden_list(operate_type, num, field_pad_p);
        break;
    case NAT_HOST:
		LOG_PROC("HBASE", "%s -- NAT_HOST",FN);
        recover_fabric_nat_host_list(operate_type, num, field_pad_p);
        break;
    case OPENSTACK_LBAAS_MEMBERS:
		LOG_PROC("HBASE", "%s -- OPENSTACK_LBAAS_MEMBERS",FN);
        recover_fabric_openstack_lbaas_members_list(operate_type, num, field_pad_p);
        break;
    default:
        break;
    }
	LOG_PROC("HBASE", "%s -- STOP",FN);
}
//by:yhy 将data中的内容转存到g_filed_pad_slave中
INT4 reslove_data(INT1* data)
{	
	tlv_t* tlv_p = (tlv_t *)(data + sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header));
	g_filed_num_slave = 0;

	while (tlv_p->len)
	{	
		g_filed_pad_slave[g_filed_num_slave].type = tlv_p->type;
		g_filed_pad_slave[g_filed_num_slave].len  = tlv_p->len;
        memset(g_filed_pad_slave[g_filed_num_slave].pad, 0, MAX_PAD_LEN);
		strncpy(g_filed_pad_slave[g_filed_num_slave].pad, tlv_p->data, g_filed_pad_slave[g_filed_num_slave].len);
		g_filed_pad_slave[g_filed_num_slave].pad[g_filed_pad_slave[g_filed_num_slave].len] = '\0';
		g_filed_num_slave++;
		tlv_p = (tlv_t *)(tlv_p->data + tlv_p->len);	
	}
	return g_filed_num_slave;
}
//by:yhy 从Hbase中查找TRANS_SEQ字段的值
//  获取HBASE中数据的更新版本号.
UINT8 query_trans_seq()
{
	UINT8 trans_seq = 0;
	INT1 trnas_id[TABLE_STRING_LEN] = {0};

	query_value(TRANS_SEQ, trnas_id);
	trans_seq = strtoull(trnas_id, 0, 10); //10 base
	//printf("trans_seq: %llu\n", trans_seq);
	return trans_seq;
}
//by:yhy 查询并获取Hbase中所有信息(控制器核心数据),并将其恢复(相当于将当前控制器的核心数据覆盖更新)
INT4 query_data()
{

	UINT8    idx = 0;
	UINT4    node_type = 0;
	UINT4    operate_type = 0;
	UINT8    trans_seq = 0;
	UINT8    uiReplyLen = 0;
	redisReply *reply = NULL;
	stSynMsgNode_header  *pstSynNodeHeader = NULL;
	
	if(!g_method_DBSyn)
	{
		
		LOG_PROC("ERROR", "redis server not start or configure error ! %s", FN);
		return GN_ERR;
	}
	
	trans_seq = query_trans_seq();
	LOG_PROC("INFO", "sync data: Master:%llu -> Slave:%llu", trans_seq, g_trans_seq_slave);

	if (trans_seq <= g_trans_seq_slave)
	{//by:yhy 如果HBASE中数据的更新版本号小于本机已经更新的版本号,则数据过期
		LOG_PROC("INFO", "There is`t latest data");
		return  GN_ERR;
	}


	/*
	FABRIC_HOST_NODE         = 0x01,
	FABRIC_SW_NODE 	         = 0x02,
	
	NAT_ICMP			     = 0x03,
	NAT_HOST			     = 0x04,
	
	OPENSTACK_EXTERNAL_NODE  = 0x05,
	OPENSTACK_LBAAS_MEMBERS  = 0x06
	*/
	for(idx=0; idx<OPENSTACK_LBAAS_MEMBERS; idx++)
	{
		pthread_mutex_lock(&g_redisMutex);
		reply = redisCommand(g_redisCtx,"GET %d",  idx+1 );
		
		if((NULL == reply)||(REDIS_REPLY_ERROR == reply->type))		
		{
			if(g_redisCtx->err)
			{
				LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
			}
			if(NULL != reply)
				freeReplyObject(reply);
			pthread_mutex_unlock(&g_redisMutex);
			return GN_ERR;
		}
		if(REDIS_REPLY_NIL == reply->type)
		{
			freeReplyObject(reply);
			pthread_mutex_unlock(&g_redisMutex);
			continue;
		}
		if( reply->len)
		{
			memcpy(g_table_data_buf, reply->str , reply->len);
		}
		uiReplyLen =  reply->len;
		freeReplyObject(reply);
		pthread_mutex_unlock(&g_redisMutex); //reply->len可能较大，处理部分分开
		if(uiReplyLen)
		{
			pstSynNodeHeader = (stSynMsgNode_header *)(g_table_data_buf + sizeof(stCommMsgNode_header));
			node_type = pstSynNodeHeader->ucSynMsgType;
			operate_type = pstSynNodeHeader->ucOperMsgType;
			reslove_data(g_table_data_buf);
			recovery_data(node_type, operate_type, g_filed_num_slave, g_filed_pad_slave);
		}
	}

	//by:yhy 更新slave本地版本号
	g_trans_seq_slave = trans_seq;
	return GN_OK;
}


//by:yhy 初始化用于操作Hbase的jni调用相关的函数的引用,并获取最新的g_topology_version与g_master_id
//		 无明显问题,重点在于JNI的相关操作
INT4 init_redis_client()
{
	
    INT1 value[TABLE_STRING_LEN] = {0};
    INT1 *conf_value = NULL;
	 struct timeval tv = {1,0};
    conf_value = get_value(g_controller_configure, "[cluster_conf]", "hbase_ip");
    NULL == conf_value ? strncpy(g_redis_ip, "127.0.0.1", 32 - 1) : strncpy(g_redis_ip, conf_value, 32 - 1);

    conf_value = get_value(g_controller_configure, "[cluster_conf]", "hbase_port");
    NULL == conf_value ? strncpy(g_redis_port, "6379", 16 - 1) : strncpy(g_redis_port, conf_value, 16 - 1);

	LOG_PROC("INFO", "redis_ip: %s, redis_port: %s", g_redis_ip, g_redis_port);

	g_redisCtx = redisConnectWithTimeout(g_redis_ip, atoi(g_redis_port), tv);
	if (NULL == g_redisCtx || g_redisCtx->err) 
	{        
		if (g_redisCtx) 
		{            
			LOG_PROC("ERROR", "redisCommand failed ! %s %s", FN, g_redisCtx->errstr);
			redisFree(g_redisCtx);  
		} 
		else 
		{            
			LOG_PROC("ERROR","Connection error: can't allocate redis context\n");        
		}
		return GN_ERR;
	}
	pthread_mutex_init(&g_redisMutex, NULL);
	g_method_DBSyn = 1;
	return GN_OK;
}

void fini_redis_client()
{
}
//by:yhy 如果当前主机是slave将Hbase中数据同步至当前控制器,如果当前控制器为master则将控制器数据同步至Hbase
void sync_data()
{
	int tv1,tv2 ;
	tv1= time(NULL);
	LOG_PROC("INFO", "%s - START",FN);
	if (g_controller_role == OFPCR_ROLE_SLAVE)
	{//by:yhy 从机
		LOG_PROC("TIMER", "%s - g_controller_role == OFPCR_ROLE_SLAVE",FN);
		query_data();
	}
    else if (g_controller_role == OFPCR_ROLE_MASTER)
    {//by:yhy 主机
		LOG_PROC("TIMER", "%s - g_controller_role == OFPCR_ROLE_MASTER",FN);
		//by:hy 将主机信息交换机信息存入Hbase
        persist_fabric_host_list();
        persist_fabric_sw_list();
    }
	tv2= time(NULL);
	LOG_PROC("INFO", "%s - STOP tv2-tv1=%d",FN,tv2-tv1);
}

//by:yhy 分配同步所需的相关变量的内存空间,创建同步线程
//       无明显问题,注意几个全局变量的用途
INT4 init_sync_mgr()
{
	pthread_t datasyn_thread;
	
   
    UINT4 len = sizeof(field_pad_t) * MAX_FILED_NUM;
    g_filed_pad_master = (field_pad_t*)gn_malloc(len);
    g_filed_pad_slave = (field_pad_t*)gn_malloc(len);

    TABLE_DATA_LEN = len + sizeof(stCommMsgNode_header) + sizeof(stSynMsgNode_header);
    g_table_data_buf = (INT1*)gn_malloc(TABLE_DATA_LEN);

	void* sync_timerid  = NULL;
    INT1* value = get_value(g_controller_configure, "[cluster_conf]", "sync_interval");
	UINT4 sync_interval = ((NULL == value) ? 30 : atoll(value));;


	sync_timerid = timer_init(1);

	if (NULL == timer_creat(sync_timerid, sync_interval, NULL, &g_sync_timer, sync_data))
	{
		return GN_ERR;
	}

	if (g_controller_role == OFPCR_ROLE_MASTER)
	{
		persist_value(TRANS_SEQ, "0");
	} 
	
	return GN_OK;
}

void fini_sync_mgr()
{
    gn_free((void **)&g_table_data_buf);
    gn_free((void **)&g_filed_pad_master);
    gn_free((void **)&g_filed_pad_slave);
}

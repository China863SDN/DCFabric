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
*   File Name   : hbase_client.c           *
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
#include "hbase_client.h"
#include <jni.h>
#include <dirent.h>
#include "openflow-common.h"
#include "fabric_impl.h"
#include "hbase_sync.h"

UINT4 MAX_FILED_NUM = 204800;
UINT4 TABLE_DATA_LEN = 204800;

field_pad_t* g_filed_pad_master = NULL;
field_pad_t* g_filed_pad_slave = NULL;
INT1* g_table_data_buf = NULL;
UINT4 g_filed_num_master = 0;
UINT4 g_filed_num_slave  = 0;
UINT8 g_trans_seq_master = 0;
UINT8 g_trans_seq_slave = 0;
void* g_sync_timer = NULL;

INT1 g_hbase_ip[32];
INT1 g_hbase_port[16];

JavaVM *g_jvm = NULL;
jclass g_hbase_client = 0;
jmethodID g_method_addrecord = 0;  //AddRecord method id.
jmethodID g_method_deleterecord = 0;  //DeleteRecord method id.
jmethodID g_method_getonecell = 0;  //GetoneCell method id.
jmethodID g_method_getonerecord = 0;  //GetoneRecord method id.
jmethodID g_method_getallrecords = 0;   //GetAllRecords method id.
jmethodID g_method_getrecordsbyfilter = 0;  //GetRecordsByFilter method id.

//static pthread_mutex_t g_jni_mutex = PTHREAD_MUTEX_INITIALIZER; //JNI thread lock

int g_topology_version = -1;

#define HBASE_ADD_RECORD(column, value) \
        arg_column = (*jni_env)->NewStringUTF(jni_env, column);\
        arg_value = (*jni_env)->NewStringUTF(jni_env, value);   \
        (*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_addrecord, arg_tableName, arg_columnFamily, arg_row, arg_column, arg_value);

void persist_topology()
{
    UINT4 idx_sw = 0;
    UINT4 idx_port = 0;
    gn_switch_t *sw = NULL;

    INT1 row[TABLE_STRING_LEN] = {0};
    INT1 src_dpid[TABLE_STRING_LEN] = {0};
    INT1 src_port[TABLE_STRING_LEN] = {0};
    INT1 dst_ppid[TABLE_STRING_LEN] = {0};
    INT1 dst_port[TABLE_STRING_LEN] = {0};
    INT1 current_version[TABLE_STRING_LEN] = {0};

    INT4 status = 0;
    UINT4 record_cnt = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_columnFamily;
    jstring arg_row;
    jstring arg_column;
    jstring arg_value;

    if(0 == g_method_addrecord)
    {
        printf("Hbase client hasn't ready!\n");
        return;
    }

    //Get jni env for current thread
    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
        printf("Update topology, can not get jni env!\n");
        return;
    }

    //Increase the version
    snprintf(current_version, TABLE_STRING_LEN,"%d", ++g_topology_version);

    for(; idx_sw < g_server.max_switch; idx_sw++)
    {
        if (g_server.switches[idx_sw].state)
        {
            sw = &g_server.switches[idx_sw];
            if(sw && (sw->state))
            {
                snprintf(src_dpid, TABLE_STRING_LEN,"%016llx", sw->dpid);
                for(idx_port = 0; idx_port < sw->n_ports; idx_port++)
                {
                    if ((NULL != sw->neighbor[idx_port]) && (NULL != sw->neighbor[idx_port]->sw))
                    {
                        snprintf(src_port, TABLE_STRING_LEN,"%d", sw->ports[idx_port].port_no);
                        snprintf(dst_ppid, TABLE_STRING_LEN,"%016llx", sw->neighbor[idx_port]->sw->dpid);
                        snprintf(dst_port, TABLE_STRING_LEN,"%d", sw->neighbor[idx_port]->port->port_no);
                        snprintf(row, TABLE_STRING_LEN, "%d.%d.%d", idx_sw, idx_port, g_topology_version);

//                        printf("Persist topology, Source: dpid- %016llx(port- %d) -> Destination: dpid- %016llx(port- %d)\n",
//                                sw->datapath_id, idx_port, sw->neighbor[idx_port]->neigh_sw->datapath_id, sw->neighbor[idx_port]->neigh_port->port_no);

                        arg_tableName = (*jni_env)->NewStringUTF(jni_env, TOPO_TABLE);
                        arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, TOPO_TABLE_CF);
                        arg_row = (*jni_env)->NewStringUTF(jni_env, row);

                        //Persist SRC_DPID
                        HBASE_ADD_RECORD("SRC_DPID", src_dpid);

                        //Persist SRC_PORT
                        HBASE_ADD_RECORD("SRC_PORT", src_port);

                        //Persist DST_DPID
                        HBASE_ADD_RECORD("DST_DPID", dst_ppid);

                        //Persist DST_PORT
                        HBASE_ADD_RECORD("DST_PORT", dst_port);

                        //Persist VERSION
                        HBASE_ADD_RECORD("VERSION", current_version);

                        record_cnt++;
                    }
                }
            }
        }
    }

    //refresh the latest version
    if (record_cnt)
    {
        arg_tableName = (*jni_env)->NewStringUTF(jni_env, COMMON_TABLE);
        arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, COMMON_TABLE_CF);
        arg_row = (*jni_env)->NewStringUTF(jni_env, TOPO_VER);

        HBASE_ADD_RECORD("VALUE", current_version);
    }
    else
    {
        g_topology_version--;
    }

    (*g_jvm)->DetachCurrentThread(g_jvm);
}

void query_topology()
{
    INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_row;
    jstring arg_columnFamily;
    jstring arg_column;
    jstring arg_value;

    jarray data;
    UINT4 src_port = 0;
    UINT4 dst_port = 0;
    unsigned long long int src_dpid = 0;
    unsigned long long int dst_dpid = 0;
    gn_switch_t *src_sw;

    if(0 == g_method_addrecord)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    //Get jni env for current thread
    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
        LOG_PROC("ERROR", "Update topology, can not get jni env");
        return;
    }

    arg_value = (*jni_env)->NewStringUTF(jni_env, "0");
	arg_tableName = (*jni_env)->NewStringUTF(jni_env, COMMON_TABLE);
	arg_row = (*jni_env)->NewStringUTF(jni_env, TOPO_VER);
	arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, COMMON_TABLE_CF);
	arg_column = (*jni_env)->NewStringUTF(jni_env, "VALUE");
	data = (jstring)((*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getonecell, arg_tableName, arg_row, arg_columnFamily, arg_column));
    if (NULL != data)
    {
        const char *str = (*jni_env)->GetStringUTFChars(jni_env, data, NULL);
    	arg_value = (*jni_env)->NewStringUTF(jni_env, str);
    }
    arg_tableName = (*jni_env)->NewStringUTF(jni_env, TOPO_TABLE);
    arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, TOPO_TABLE_CF);
    arg_column = (*jni_env)->NewStringUTF(jni_env, "VERSION");
    data = (*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getrecordsbyfilter, arg_tableName, arg_columnFamily, arg_column, arg_value);
    if (NULL != data)
    {
        const jsize length = (*jni_env)->GetArrayLength(jni_env, data);
        jsize index = length;
        while(index--)
        {
            jstring element = (jstring)((*jni_env)->GetObjectArrayElement(jni_env, data, index));
            char const* nativeString = (*jni_env)->GetStringUTFChars(jni_env, element, 0);

            char *p_next = NULL;
            char *value = NULL;
            char *str_cpy = strdup(nativeString);
            char *column = strcut(str_cpy, '|', &p_next);
            while(column)
            {
                value = strcut(NULL, '|', &p_next);
                if (0 == strcmp(column, "SRC_DPID"))
                {
                    src_dpid = strtoull(value, NULL, 16);
                }
                else if(0 == strcmp(column, "SRC_PORT"))
                {
                    src_port = atoll(value);
                }
                else if(0 == strcmp(column, "DST_DPID"))
                {
                    dst_dpid = strtoull(value, NULL, 16);
                }
                else if(0 == strcmp(column, "DST_PORT"))
                {
                    dst_port = atoll(value);
                }

                if (!p_next)
                {
                    break;
                }
                column = strcut(NULL, '|', &p_next);
            }

            free(str_cpy);
            (*jni_env)->ReleaseStringUTFChars(jni_env, element, nativeString);

            src_sw = find_sw_by_dpid(src_dpid);
            if(src_sw)
            {
                LOG_PROC("DEBUG", "----======>>>>> Mapping a new neighbor: src- %016llx.%d, dst: %016llx.%d", src_dpid, src_port, dst_dpid, dst_port);
                mapping_new_neighbor(src_sw, src_port, dst_dpid, dst_port);
            }
            else
            {
                LOG_PROC("DEBUG", "----======>>>>> Mapping a new neighbor failed, can not find src sw: src- %016llx.%d, dst: %016llx.%d", src_dpid, src_port, dst_dpid, dst_port);
            }
        }
    }

    //detach
    (*g_jvm)->DetachCurrentThread(g_jvm);
}

void persist_value(const INT1* key, const INT1* value)
{
    INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_row;
    jstring arg_columnFamily;
    jstring arg_column;
    jstring arg_value;

    if(0 == g_method_addrecord)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
        LOG_PROC("ERROR", "Persist value [%s] failed by key [%s], can not get jni env", value, key);
        return;
    }

    arg_tableName = (*jni_env)->NewStringUTF(jni_env, COMMON_TABLE);
    arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, COMMON_TABLE_CF);
    arg_row = (*jni_env)->NewStringUTF(jni_env, key);

    //Persist VALUE
    HBASE_ADD_RECORD("VALUE", value);

    //detach
    (*g_jvm)->DetachCurrentThread(g_jvm);
}

void query_value(const INT1* key, INT1* value)
{
    INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_columnFamily;
    jstring arg_column;
    jstring arg_row;
    jstring out_res;

    if(0 == g_method_getonecell)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
        LOG_PROC("ERROR", "Query value failed by key [%s], can not get jni env", key);
        return;
    }

    arg_tableName = (*jni_env)->NewStringUTF(jni_env, COMMON_TABLE);
    arg_row = (*jni_env)->NewStringUTF(jni_env, key);
    arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, COMMON_TABLE_CF);
	arg_column = (*jni_env)->NewStringUTF(jni_env, "VALUE");
    out_res = (jstring)((*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getonecell, arg_tableName, arg_row, arg_columnFamily, arg_column));
    if (NULL != out_res)
    {
        const char *str = (*jni_env)->GetStringUTFChars(jni_env, out_res, NULL);
        strncpy(value, str, TABLE_STRING_LEN);
    }

    //detach
    (*g_jvm)->DetachCurrentThread(g_jvm);
}

void delete_record(const INT1 *table_name, const INT1 *row_key)
{
    INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_row;

    if(0 == g_method_deleterecord)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready!\n");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
        LOG_PROC("ERROR", "Delete record, can not get jni env! --RowKey: %s\n", row_key);
        return;
    }

    arg_tableName = (*jni_env)->NewStringUTF(jni_env, table_name);
    arg_row = (*jni_env)->NewStringUTF(jni_env, row_key);
    (*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_deleterecord, arg_tableName, arg_row);
}

void persist_controller(UINT4 ip, UINT2 port, INT1* role)
{
	INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_row;
    jstring arg_columnFamily;
    jstring arg_column;
    jstring arg_value;
    INT1 ip_s[TABLE_STRING_LEN] = {0};
    INT1 port_s[TABLE_STRING_LEN] = {0};

    if(0 == g_method_addrecord)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {	
    	LOG_PROC("ERROR", "Persist Controller Info failed");
        return;
    }

	number2ip(ip, ip_s);
    snprintf(port_s,  TABLE_STRING_LEN, "%d", port);

    arg_tableName    = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE);
    arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE_CF);
    arg_row          = (*jni_env)->NewStringUTF(jni_env, ip_s);

     HBASE_ADD_RECORD("IP",   ip_s);
     HBASE_ADD_RECORD("PORT", port_s);
     HBASE_ADD_RECORD("ROLE", role);

     (*g_jvm)->DetachCurrentThread(g_jvm);
}

void deletet_controller(INT1* key)
{
	INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_row;
	
    if(0 == g_method_deleterecord)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
    	LOG_PROC("ERROR", "Delete Controller failed");
    }

	arg_tableName = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE);
	arg_row       = (*jni_env)->NewStringUTF(jni_env, key);

	(*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_deleterecord, arg_tableName, arg_row);

	(*g_jvm)->DetachCurrentThread(g_jvm);
}

void query_controller_all(INT1* value)
{
	INT1 *str = NULL;
	INT1 tmp[32] = {0};
	INT4 status  = 0;
	JNIEnv *jni_env = NULL;
	jstring arg_tableName;
	jstring arg_columnFamily;
	jarray out_res;

	if(0 == g_method_getallrecords)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
    	LOG_PROC("ERROR", "Query Controller All IP failed");
        return;
    }

	str = value;
	
    arg_tableName    = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE);
	arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE_CF);
    out_res          = (*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getallrecords, arg_tableName, arg_columnFamily);

	if (NULL != out_res)
	{
		const jsize length = (*jni_env)->GetArrayLength(jni_env, out_res);
		jsize index = length;
		while(index--)
		{
			jstring element = (jstring)((*jni_env)->GetObjectArrayElement(jni_env, out_res, index));
            char const* nativeString = (*jni_env)->GetStringUTFChars(jni_env, element, NULL);

            char *p_next  = NULL;
            char *value   = NULL;
            char *str_cpy = strdup(nativeString);
            char *column  = strcut(str_cpy, '|', &p_next);

            while(column)
            {
            	value = strcut(NULL, '|', &p_next);

				if (0 == strcmp(column, "IP"))
                {  
                	sprintf(tmp, "%s-", value);
            		strcat(str, tmp); 
                }
   
            	if (!p_next)
                {
                    break;
                }
                column = strcut(NULL, '|', &p_next);
            }
      
            free(str_cpy);
            (*jni_env)->ReleaseStringUTFChars(jni_env, element, nativeString);  
		}
		str[strlen(str)-1] = '\0';
	}
	(*g_jvm)->DetachCurrentThread(g_jvm);
}

void query_controller_port(INT1* key, INT1* value)
{
	INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_columnFamily;
    jstring arg_column;
    jstring arg_row;
    jstring out_res;

    if(0 == g_method_getonecell)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
    	LOG_PROC("ERROR", "Query Controller Port failed");
        return;
    }

    arg_tableName = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE);
    arg_row = (*jni_env)->NewStringUTF(jni_env, key);
    arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE_CF);
	arg_column = (*jni_env)->NewStringUTF(jni_env, "PORT");
    out_res = (jstring)((*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getonecell, arg_tableName, arg_row, arg_columnFamily, arg_column));
    if (NULL != out_res)
    {
        const char *str = (*jni_env)->GetStringUTFChars(jni_env, out_res, NULL);
        
        strncpy(value, str, TABLE_STRING_LEN);
    }

    //detach
    (*g_jvm)->DetachCurrentThread(g_jvm);    
}

void query_controller_role(INT1* key, INT1* value)
{
	INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_columnFamily;
    jstring arg_column;
    jstring arg_row;
    jstring out_res;

    if(0 == g_method_getonecell)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
    	LOG_PROC("ERROR", "Query Controller Role failed");
        return;
    }

    arg_tableName = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE);
    arg_row = (*jni_env)->NewStringUTF(jni_env, key);
    arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE_CF);
	arg_column = (*jni_env)->NewStringUTF(jni_env, "ROLE");
    out_res = (jstring)((*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getonecell, arg_tableName, arg_row, arg_columnFamily, arg_column));
    if (NULL != out_res)
    {
        const char *str = (*jni_env)->GetStringUTFChars(jni_env, out_res, NULL);
        strncpy(value, str, TABLE_STRING_LEN);
    }

    //detach
    (*g_jvm)->DetachCurrentThread(g_jvm);    
}

void reset_controller_role(INT1* key, INT1* value)
{
	INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_row;
    jstring arg_columnFamily;
    jstring arg_column;
    jstring arg_value;

    if(0 == g_method_addrecord)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {	
    	LOG_PROC("ERROR", "Update Controller Role failed");
        return;
    }

    arg_tableName    = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE);
    arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE_CF);
    arg_row          = (*jni_env)->NewStringUTF(jni_env, key);

     HBASE_ADD_RECORD("ROLE", value);

     (*g_jvm)->DetachCurrentThread(g_jvm);
}

INT4 IsInDB(INT1* key)
{
	INT4 status  = 0;
	JNIEnv *jni_env = NULL;
	jstring arg_tableName;
	jstring arg_columnFamily;
	jarray out_res;

	if(0 == g_method_getallrecords)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return GN_ERR; 
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
    	LOG_PROC("ERROR", "Query Controller All IP failed");
        return GN_ERR;
    }

    arg_tableName    = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE);
	arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_INFO_TABLE_CF);
    out_res          = (*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getallrecords, arg_tableName, arg_columnFamily);

	if (NULL != out_res)
	{
		const jsize length = (*jni_env)->GetArrayLength(jni_env, out_res);
		jsize index = length;
		while(index--)
		{
			jstring element = (jstring)((*jni_env)->GetObjectArrayElement(jni_env, out_res, index));
            char const* nativeString = (*jni_env)->GetStringUTFChars(jni_env, element, NULL);

            char *p_next  = NULL;
            char *value   = NULL;
            char *str_cpy = strdup(nativeString);
            char *column  = strcut(str_cpy, '|', &p_next);

            while(column)
            {
            	value = strcut(NULL, '|', &p_next);

				if (0 == strcmp(column, "IP"))
                {  
                	if (!strncmp(value, key, strlen(key)))
                	{
                		return GN_OK;
                	}
                }
   
            	if (!p_next)
                {
                    break;
                }
                column = strcut(NULL, '|', &p_next);
            }
            
            free(str_cpy);
            (*jni_env)->ReleaseStringUTFChars(jni_env, element, nativeString);  
		}
	}
	(*g_jvm)->DetachCurrentThread(g_jvm);
	
	return GN_ERR;
}

void persist_data(UINT4 node_type, UINT4 operate_type, INT4 num, field_pad_t* field_pad_p)
{
	INT1 ntype[TABLE_STRING_LEN]   = {0};
	INT1 otype[TABLE_STRING_LEN]   = {0};
	INT1 row_key[TABLE_STRING_LEN] = {0};
	INT1 seq[TABLE_STRING_LEN]     = {0};
	tlv_t* pTlv = NULL;
	INT4 idx = 0;
	INT4 status = 0;
    JNIEnv *jni_env = NULL;
    jstring arg_tableName;
    jstring arg_row;
    jstring arg_columnFamily;
    jstring arg_column;
    jstring arg_value;

    if(0 == g_method_addrecord)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
    	LOG_PROC("ERROR", "Persist Data failed");
        return;
    }

    memset(g_table_data_buf, 0, TABLE_DATA_LEN);
	pTlv = (tlv_t*)g_table_data_buf;
	for (idx = 0; idx < num; idx++)
	{
		pTlv->type = field_pad_p[idx].type;
		pTlv->len  = field_pad_p[idx].len;
		memcpy(pTlv->data, field_pad_p[idx].pad, field_pad_p[idx].len);

		if (idx < num-1)  pTlv = (tlv_t*)(pTlv->data + pTlv->len);			
	}

	snprintf(row_key, TABLE_STRING_LEN, "%016llx", g_trans_seq_master);
	snprintf(ntype,   TABLE_STRING_LEN, "%u",      node_type);
	snprintf(otype,   TABLE_STRING_LEN, "%u",      operate_type);
	arg_tableName 	 = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_DATA_TABLE);
    arg_columnFamily = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_DATA_TABLE_CF);
    arg_row          = (*jni_env)->NewStringUTF(jni_env, row_key);

    //Persist VALUE
    HBASE_ADD_RECORD("NODE_TYPE",    ntype);
    HBASE_ADD_RECORD("OPERATE_TYPE", otype);
    HBASE_ADD_RECORD("DATA",         g_table_data_buf);

    //detach
    (*g_jvm)->DetachCurrentThread(g_jvm);

	g_trans_seq_master++;

	snprintf(seq, TABLE_STRING_LEN, "%016llx", g_trans_seq_master);
	persist_value(TRANS_SEQ, seq);
}

void recovery_data(UINT4 node_type, UINT4 operate_type, INT4 num, field_pad_t* field_pad_p)
{
    switch (node_type)
    {
    case FABRIC_SW_NODE:
        recover_fabric_sw_list(num, field_pad_p);
        break;
    case FABRIC_HOST_NODE:
        recover_fabric_host_list(num, field_pad_p);
        break;
    case OPENSTACK_EXTERNAL_NODE:
        recover_fabric_openstack_external_list(num, field_pad_p);
        break;
    case NAT_ICMP:
        recover_fabric_nat_icmp_iden_list(operate_type, num, field_pad_p);
        break;
    case NAT_HOST:
        recover_fabric_nat_host_list(operate_type, num, field_pad_p);
        break;
    case OPENSTACK_LBAAS_MEMBERS:
        recover_fabric_openstack_lbaas_members_list(operate_type, num, field_pad_p);
        break;
    default:
        break;
    }
}

INT4 reslove_data(INT1* data)
{	
	tlv_t* tlv_p = (tlv_t *)data;
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

UINT8 query_trans_seq()
{
	UINT8 trans_seq = 0;
	INT1 trnas_id[TABLE_STRING_LEN] = {0};

	query_value(TRANS_SEQ, trnas_id);
	trans_seq = strtoull(trnas_id, 0, 16);

	//printf("trans_seq: %llu\n", trans_seq);

	return trans_seq;
	
}
void query_data()
{
	JNIEnv *jni_env = NULL;
	jstring arg_tableName;
	jstring arg_row;
	jarray  out_res;
	INT1  row_key[TABLE_STRING_LEN]  = {0};
	INT4  status  = 0;
	UINT8 idx = 0;
	UINT4 node_type = 0;
	UINT4 operate_type = 0;
	UINT8 trans_seq = 0;
	
	trans_seq = query_trans_seq();
	LOG_PROC("INFO", "sync data: Master:%llu -> Slave:%llu", trans_seq, g_trans_seq_slave);

	if (trans_seq <= g_trans_seq_slave)
	{
		LOG_PROC("INFO", "There is`t latest data");
		return ;
	}
	
	if(0 == g_method_getonerecord)
    {
        LOG_PROC("ERROR", "Hbase client hasn't ready");
        return;
    }

    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
        LOG_PROC("ERROR", "Fetch transation, can not get jni env");
        return;
    }

    arg_tableName    = (*jni_env)->NewStringUTF(jni_env, CONTROLLER_DATA_TABLE);
	for (idx = g_trans_seq_slave; idx < trans_seq; idx++)
	{	
	    memset(g_table_data_buf, 0, TABLE_DATA_LEN);
        
		snprintf(row_key, TABLE_STRING_LEN, "%016llx", idx);
    	arg_row = (*jni_env)->NewStringUTF(jni_env, row_key);
    	out_res = (*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getonerecord, arg_tableName, arg_row);

		if (NULL != out_res)
		{
			char const* nativeString = (*jni_env)->GetStringUTFChars(jni_env, out_res, 0);
			char *p_next = NULL;
            char *value = NULL;
            char *str_cpy = strdup(nativeString);
            char *column = strcut(str_cpy, '|', &p_next);
            while(column)
            {
            	value = strcut(NULL, '|', &p_next);

                if(0 == strcmp(column, "NODE_TYPE"))
                {      
                	node_type = atoll(value);   
                }
                else if(0 == strcmp(column, "OPERATE_TYPE"))
                {    
                	operate_type = atoll(value);       
                }
                else if(0 == strcmp(column, "DATA"))
                {
                	strncpy(g_table_data_buf, value, TABLE_DATA_LEN);	
                	reslove_data(g_table_data_buf);
                }   
  
                if (!p_next)
                {
                    break;
                }
                column = strcut(NULL, '|', &p_next);
            }

			recovery_data(node_type, operate_type, g_filed_num_slave, g_filed_pad_slave);
			
            free(str_cpy);	
		}
	}
	
	(*g_jvm)->DetachCurrentThread(g_jvm);

	g_trans_seq_slave = trans_seq;
}

INT4 init_hbase_client()
{
    JNIEnv *jni_env = (JNIEnv *)NULL;
    jclass cls = 0;
    JavaVMInitArgs vm_args;
    JavaVMOption   options[1];
    jmethodID mid = 0;

    INT1 value[TABLE_STRING_LEN] = {0};
    INT1 *conf_value = NULL;
    conf_value = get_value(g_controller_configure, "[cluster_conf]", "hbase_ip");
    NULL == conf_value ? strncpy(g_hbase_ip, "127.0.0.1", 32 - 1) : strncpy(g_hbase_ip, conf_value, 32 - 1);

    conf_value = get_value(g_controller_configure, "[cluster_conf]", "hbase_port");
    NULL == conf_value ? strncpy(g_hbase_port, "60000", 16 - 1) : strncpy(g_hbase_port, conf_value, 16 - 1);

	LOG_PROC("INFO", "hbase_ip: %s, hbase_port: %s", g_hbase_ip, g_hbase_port);

	options[0].optionString = "-Djava.class.path=third/hbase/hbase_client.jar:third/hbase/hbase_lib.jar";
    vm_args.version  = JNI_VERSION_1_6;
    vm_args.nOptions = 1;
    vm_args.options  = options;


    //Create java vm
    if(JNI_CreateJavaVM(&g_jvm, (void **)&jni_env, &vm_args) != JNI_OK)
    {
        LOG_PROC("ERROR", "Create JVM failed");
        return GN_ERR;
    }

    //Find class "hbase_client"
     cls = (*jni_env)->FindClass(jni_env, "hbase_client");
     g_hbase_client = (jclass)((*jni_env)->NewGlobalRef(jni_env, cls));
    if(0 == g_hbase_client)
    {
        LOG_PROC("ERROR", "Jni can not find class named \"hbase_client\"");
        return GN_ERR;
    }

    //Init env conf
    mid = (*jni_env)->GetStaticMethodID(jni_env, g_hbase_client, "Init_conf", "(Ljava/lang/String;Ljava/lang/String;)V");
    if(0 != mid)
    {
        jstring arg_ip = (*jni_env)->NewStringUTF(jni_env, g_hbase_ip);
        jstring arg_port = (*jni_env)->NewStringUTF(jni_env, g_hbase_port);
        (*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, mid, arg_ip, arg_port);
    }
    else
    {
        LOG_PROC("ERROR", "Call method \"Init_conf\" failed");
        return GN_ERR;
    }

    //Get method: add record
    mid = (*jni_env)->GetStaticMethodID(jni_env, g_hbase_client, "addRecord", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if(0 == mid)
    {
        LOG_PROC("ERROR", "Jni can not find method named \"addRecord\"");
        return GN_ERR;
    }
    g_method_addrecord = (jmethodID)((*jni_env)->NewGlobalRef(jni_env, (jobject)mid));

    //Get method: delete records by row key
    mid = (*jni_env)->GetStaticMethodID(jni_env, g_hbase_client, "deleteRecord", "(Ljava/lang/String;Ljava/lang/String;)V");
    if(0 == mid)
    {
        LOG_PROC("ERROR", "Jni can not find method named \"deleteRecord\"");
        return GN_ERR;
    }
    g_method_deleterecord = (jmethodID)((*jni_env)->NewGlobalRef(jni_env, (jobject)mid));

    //Get method: get records by filter
    mid = (*jni_env)->GetStaticMethodID(jni_env, g_hbase_client, "getRecordsByFilter", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;");
    if(0 == mid)
    {
        LOG_PROC("ERROR", "Jni can not find method named \"getRecordsByFilter\"");
        return GN_ERR;
    }
    g_method_getrecordsbyfilter = (jmethodID)((*jni_env)->NewGlobalRef(jni_env, (jobject)mid));

    //Get method: get all records
    mid = (*jni_env)->GetStaticMethodID(jni_env, g_hbase_client, "getAllRecords", "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;");
    if(0 == mid)
    {
        LOG_PROC("ERROR", "Jni can not find method named \"getAllRecords\"");
        return GN_ERR;
    }
    g_method_getallrecords = (jmethodID)((*jni_env)->NewGlobalRef(jni_env, (jobject)mid));

    //Get method: get one record
    mid = (*jni_env)->GetStaticMethodID(jni_env, g_hbase_client, "getOneRecord", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    if(0 == mid)
    {
        LOG_PROC("ERROR", "Jni can not find method named \"getOneRecord\"");
        return GN_ERR;
    }
    g_method_getonerecord = (jmethodID)((*jni_env)->NewGlobalRef(jni_env, (jobject)mid));

    //Get method: get one cell
    mid = (*jni_env)->GetStaticMethodID(jni_env, g_hbase_client, "getOneCell", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    if(0 == mid)
    {
        LOG_PROC("ERROR", "Jni can not find method named \"getOneCell\"");
        return GN_ERR;
    }
    g_method_getonecell = (jmethodID)((*jni_env)->NewGlobalRef(jni_env, (jobject)mid));

    //Get the latest version of topology
    query_value(TOPO_VER, value);
    g_topology_version = (NULL == value) ? 0 : atoi(value);
    LOG_PROC("INFO", "Topology version: %d", g_topology_version);

    //Get the latest master of topology
    query_value(MASTER_ID, value);
    g_master_id = (NULL == value) ? 0 : strtoul(value, 0, 10);
    LOG_PROC("INFO", "User specified master is: %llu", g_master_id);

    return GN_OK;
}

void fini_hbase_client()
{
    JNIEnv *jni_env = (JNIEnv *)NULL;
    INT4 status = 0;

    //Get jni env for current thread
    status = (*g_jvm)->AttachCurrentThread(g_jvm, (void**)&jni_env, NULL);
    if (0 != status)
    {
        LOG_PROC("ERROR", "Fini hbase client, can not get jni env");
        return;
    }

    (*jni_env)->DeleteGlobalRef(jni_env, (jobject)g_method_addrecord);
    (*jni_env)->DeleteGlobalRef(jni_env, (jobject)g_method_getonerecord);
    (*jni_env)->DeleteGlobalRef(jni_env, (jobject)g_method_getallrecords);
    (*jni_env)->DeleteGlobalRef(jni_env, (jobject)g_method_getonecell);
    (*jni_env)->DeleteGlobalRef(jni_env, (jobject)g_method_getrecordsbyfilter);
    (*jni_env)->DeleteGlobalRef(jni_env, g_hbase_client);
    (*g_jvm)->DetachCurrentThread(g_jvm);
    (*g_jvm)->DestroyJavaVM(g_jvm);
}

void sync_data()
{
	if (g_controller_role == OFPCR_ROLE_SLAVE)
	{
		query_data();
	}
    else if (g_controller_role == OFPCR_ROLE_MASTER)
    {
        persist_fabric_host_list();
        persist_fabric_sw_list();
    }
}

INT4 init_sync_mgr()
{
    MAX_FILED_NUM = 204800;
    UINT4 len = sizeof(field_pad_t) * MAX_FILED_NUM;
    g_filed_pad_master = (field_pad_t*)gn_malloc(len);
    g_filed_pad_slave = (field_pad_t*)gn_malloc(len);

    TABLE_DATA_LEN = len;
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

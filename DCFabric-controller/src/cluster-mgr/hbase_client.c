/******************************************************************************
*                                                                             *
*   File Name   : hbase_client.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-20           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "hbase_client.h"
#include <jni.h>
#include <dirent.h>

INT1 g_hbase_ip[16];
INT1 g_hbase_port[6];

JavaVM *g_jvm = NULL;
jclass g_hbase_client = 0;
jmethodID g_method_addrecord = 0;  //AddRecord method id.
jmethodID g_method_deleterecord = 0;  //DeleteRecord method id.
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
    data = (jstring)((*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getonerecord, arg_tableName, arg_row));
    if (NULL != data)
    {
        const char *str = (*jni_env)->GetStringUTFChars(jni_env, data, NULL);
        arg_value = (*jni_env)->NewStringUTF(jni_env, str + 6);
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
                    src_port = atoi(value);
                }
                else if(0 == strcmp(column, "DST_DPID"))
                {
                    dst_dpid = strtoull(value, NULL, 16);
                }
                else if(0 == strcmp(column, "DST_PORT"))
                {
                    dst_port = atoi(value);
                }

                if (!p_next)
                {
                    break;
                }
                column = strcut(NULL, '|', &p_next);
            }

            free(str_cpy);
            (*jni_env)->ReleaseStringUTFChars(jni_env, element, nativeString);

            src_sw = find_sw_from_dpid(src_dpid);
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
    jstring arg_row;
    jstring out_res;

    if(0 == g_method_getonerecord)
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
    out_res = (jstring)((*jni_env)->CallStaticObjectMethod(jni_env, g_hbase_client, g_method_getonerecord, arg_tableName, arg_row));
    if (NULL != out_res)
    {
        const char *str = (*jni_env)->GetStringUTFChars(jni_env, out_res, NULL);

        //6 = strlen("VALUE|")
        strncpy(value, str + 6, TABLE_STRING_LEN);
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

INT4 init_hbase_client()
{
    JNIEnv *jni_env = (JNIEnv *)NULL;
    jclass cls = 0;
    JavaVMInitArgs vm_args;
    JavaVMOption   options[1];
    jmethodID mid = 0;

    INT1 value[TABLE_STRING_LEN] = {0};
    INT1 *conf_value = NULL;
    conf_value = get_value(g_controller_configure, "[controller]", "hbase_ip");
    NULL == conf_value ? strncpy(g_hbase_ip, "127.0.0.1", 16 - 1) : strncpy(g_hbase_ip, conf_value, 300 - 1);

    conf_value = get_value(g_controller_configure, "[controller]", "hbase_port");
    NULL == conf_value ? strncpy(g_hbase_port, "60000", 6 - 1) : strncpy(g_hbase_port, conf_value, 300 - 1);


//    int jarCount = 0;
//    struct dirent **jar_files;
//    char option_str[10240] = {0};
//    strcat(option_str, "-Djava.class.path=");
//    strcat(option_str, g_hbase_client_file_path);
//    if ((jarCount = scandir(g_hbase_lib_path, &jar_files, filter_jar, alphasort)) > 0)
//    {
//        while (jarCount--)
//        {
//            strcat(option_str, ":");
//            strcat(option_str, g_hbase_lib_path);
//            strcat(option_str, "/");
//            strcat(option_str, jar_files[jarCount]->d_name);
//
//            free(jar_files[jarCount]);
//        }
//
//        free(jar_files);
//    }
//    else
//    {
//        printf("No libs found in the given hbase lib path.");
//    }
//
//    options[0].optionString = option_str;
//    printf("Option string: %s\n", option_str);

    options[0].optionString = "-Djava.class.path=../hbase/hbase_client.jar:../hbase/hbase_lib.jar";
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

    //Get the latest version of topology
    query_value(TOPO_VER, value);
    g_topology_version = atoi(value);
    LOG_PROC("INFO", "Topology version: %d", g_topology_version);

    //Get the latest master of topology
    query_value(MASTER_ID, value);
    g_master_id = strtoul(value, 0, 10);
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
    (*jni_env)->DeleteGlobalRef(jni_env, (jobject)g_method_getrecordsbyfilter);
    (*jni_env)->DeleteGlobalRef(jni_env, g_hbase_client);
    (*g_jvm)->DetachCurrentThread(g_jvm);
    (*g_jvm)->DestroyJavaVM(g_jvm);
}

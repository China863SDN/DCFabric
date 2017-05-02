/******************************************************************************
 *                                                                             *
 *   File Name   : ovsdb.c           *
 *   Author      : greenet Administrator           *
 *   Create Date : 2015-3-20           *
 *   Version     : 1.0           *
 *   Function    : .           *
 *                                                                             *
 ******************************************************************************/

#include "ovsdb.h"
#include "openflow-common.h"
#include <stdlib.h>
#include "../conn-svr/conn-svr.h"
#include "openstack_host.h"
#include "../qos-mgr/qos-queue-ovsdb.h"
#include <sys/prctl.h>
#include "epoll_svr.h"

struct sockaddr_in g_ovsdb_addr;
//by:yhy 全局 ovsdb的套接字句柄
INT4 g_ovsdb_sockfd;
fd_set g_ovsdb_recvmask;
pthread_t g_ovsdb_recv_tid;

UINT1 g_ovsdb_of_version = OFP13_VERSION;           //openstack ovs锟斤拷openflow锟芥��
UINT1 g_tunnel_type = NETWORK_TYPE_VXLAN;           //openstack锟斤拷锟斤拷锟斤拷锟斤拷 gre/vxlan
//by:yhy 存放连入控制器的OVSDB的socket句柄
INT4 g_ovsdb_clients[OVSDB_MAX_CONNECTION] = { 0 };
INT4 g_ovsdb_clients_bak[OVSDB_MAX_CONNECTION] = { 0 };    //clients锟侥憋拷锟斤?? 锟斤拷锟斤拷删锟斤拷丝锟斤拷锟斤拷拥慕锟斤拷锟斤拷锟?
UINT4 g_ovsdb_clients_ip[OVSDB_MAX_CONNECTION] = { 0 };

ovsdb_server_t g_ovsdb_nodes[OVSDB_MAX_CONNECTION];
//by:yhy 全局 ovsdb服务器端口
UINT4 g_ovsdb_port = OVSDB_SERVER_PORT;    
//by:yhy 全局 ovsdb通道开启标志
UINT4 g_ovsdb_turnel_on = 1;

INT4  g_iOvsdbEpollFd = 0;



//by:yhy 
INT4 ovsdb_connect_quit()
{
    INT4 index;
    if (g_ovsdb_sockfd > 0)
    {
        close(g_ovsdb_sockfd);
        g_ovsdb_sockfd = 0;
    }

    for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
    {
        if (g_ovsdb_clients[index])
        {
            //FD_CLR(g_ovsdb_clients[index], &g_ovsdb_recvmask);
            close(g_ovsdb_clients[index]);
            g_ovsdb_clients[index] = 0;
        }
    }
    return GN_OK;
}
//by:yhy socket中read出错时,将错误代码存入errno中,EAGAIN表示无数据可读
//       判断socket是否出错或者死亡
static inline BOOL is_socket_dead(INT4 recv_res)
{
    if ((recv_res == 0) || ((recv_res < 0) && (errno != EAGAIN)))
    {
        return TRUE;
    }
    return FALSE;
}


//{"id":"1b5f9ab0-4abc-40c3-97bc-7ea66e663923","method":"monitor","params":["Open_vSwitch",null,{"Bridge":{},"Port":{},"Interface":{},"Controller":{},"Manager":{},"Mirror":{},"NetFlow":{},"Open_vSwitch":{},"QoS":{},"Queue":{},"sFlow":{},"SSL":{},"Flow_Sample_Collector_Set":{},"Flow_Table":{},"IPFIX":{}}]}
//by:yhy 向ovsdb发送检索用的json包
void search_from_ovsdb_table_all(INT4 conn_fd)
{
    INT1 *text;
	INT4 Length_BuffSended =0;
	INT4 Result_Send =0;
	INT4 iErrno =0;
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


    /*
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
     */
    json_tree_to_string(obj, &text);
    json_free_value(&obj);
   
	while(Length_BuffSended !=strlen(text))
	{
		Result_Send = send(conn_fd, (text+Length_BuffSended), (strlen(text)-Length_BuffSended), 0);
		if(Result_Send <0)
		{
			iErrno = errno;
			if((EINTR== iErrno)||(EAGAIN ==iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				Del_OVSDB_by_conn_fd(conn_fd);
				return;
			}
		}
		else
		{
			Length_BuffSended += Result_Send;
		}
	}
}
//by:yhy 在openvswitch数据库中查找对应MAC的host主机
//遍历所有ovsdb,发送查询用的JSON包
void search_host_in_ovsdb_by_mac(UINT1* mac)
{
	INT4 index = 0;
	INT1 mac_str[48] = {0};
	INT4 Length_BuffSended =0;
	INT4 Result_Send =0;
	INT4 iErrno =0;
	if (NULL == mac) 
	{
		return ;
	}
	
	mac2str(mac, mac_str);
	if (0 == strlen(mac_str)) 
	{
		return ;
	}
	
	for (index = 0; index < OVSDB_MAX_CONNECTION; index++) 
	{
		INT4 conn_fd = g_ovsdb_clients[index];

		if (conn_fd)
		{
			INT1 *text;
		    json_t *array, *obj, *key, *value, *table_obj, *named_array, *_named_array, *map_array, *mac_list_array, *mac_array;

		    obj = json_new_object();

		    key = json_new_string("id");
		    value = json_new_string(SEARCH_HOST_BY_MAC);
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

		    value = json_new_string("Open_vSwitch");
		    json_insert_child(array, value);

		    value = json_new_object();
		    json_insert_child(array, value);

		    key = json_new_string("op");
		    table_obj = json_new_string("select");
		    json_insert_child(key, table_obj);
		    json_insert_child(value, key);

		    key = json_new_string("table");
		    table_obj = json_new_string("Interface");
		    json_insert_child(key, table_obj);
		    json_insert_child(value, key);

		    _named_array = json_new_array();
		   	key = json_new_string("where");
			json_insert_child(key, _named_array);
			json_insert_child(value, key);

			named_array = json_new_array();
			json_insert_child(_named_array, named_array);

			key = json_new_string("external_ids");
			json_insert_child(named_array, key);

			key = json_new_string("includes");
			json_insert_child(named_array, key);

			map_array = json_new_array();
			json_insert_child(named_array, map_array);

			key = json_new_string("map");
			json_insert_child(map_array, key);

			mac_list_array = json_new_array();
			json_insert_child(map_array, mac_list_array);

			mac_array = json_new_array();
			json_insert_child(mac_list_array, mac_array);

			key = json_new_string("attached-mac");
			json_insert_child(mac_array, key);
			
			key = json_new_string(mac_str);
			json_insert_child(mac_array, key);

			named_array = json_new_array();
		   	key = json_new_string("columns");
			json_insert_child(key, named_array);
			json_insert_child(value, key);

			key = json_new_string("external_ids");
		    json_insert_child(named_array, key);

			key = json_new_string("ofport");
		    json_insert_child(named_array, key);

			key = json_new_string("mac_in_use");
		    json_insert_child(named_array, key);

			json_tree_to_string(obj, &text);
		    json_free_value(&obj);
		
		    while(Length_BuffSended !=strlen(text))
			{
				Result_Send = send(conn_fd, (text+Length_BuffSended), (strlen(text)-Length_BuffSended), 0);
				if(Result_Send <0)
				{
					iErrno = errno;
					if((EINTR== iErrno)||(EAGAIN ==iErrno))
					{
						continue;
					}
					else
					{
						LOG_PROC("ERROR", "%s,Send Error Code: %d conn_fd=%d",FN,iErrno,conn_fd);
						Del_OVSDB_by_conn_fd(conn_fd);
						return;
					}
				}
				else
				{
					Length_BuffSended += Result_Send;
				}
			}
		}
	}
}


//{"id": "d0d14471-5a66-412c-bcae-0e594f5ee9af","method": "transact","params": ["Open_vSwitch",{"op": "mutate","table": "Bridge","where": [["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],"mutations": [["ports","insert",["named-uuid","new_port"]]]},{"op": "insert","table": "Port","row": {"name": "gre-10.8.1.212","interfaces": ["set",[["named-uuid","new_interface"]]]},"uuid-name": "new_port"},{"op": "insert","table": "Interface","row": {"name": "gre-10.8.1.212","options": ["map",[["local_ip","10.8.1.211"],["remote_ip","10.8.1.212"],["key","flow"]]],"type": "gre"},"uuid-name": "new_interface"}]}
void add_port_and_portoption(INT4 conn_fd, INT1 *_uuid_br, INT1 *port_name,
        INT1 *local_ip, INT1 *remote_ip, INT1 *option_key, INT1 *type)
{
    INT1 *text;
	INT4 Length_BuffSended =0;
	INT4 Result_Send =0;
	INT4 iErrno =0;
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array,
            *uuid_array, *named_array, *row_obj;

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

    key = json_new_string(_uuid_br);
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
    value = json_new_string(port_name);
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
    value = json_new_string(port_name);
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
    key = json_new_string(local_ip);
    json_insert_child(named_array, key);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);
    key = json_new_string("remote_ip");
    json_insert_child(named_array, key);
    key = json_new_string(remote_ip);
    json_insert_child(named_array, key);

    named_array = json_new_array();
    json_insert_child(uuid_array, named_array);
    key = json_new_string("key");
    json_insert_child(named_array, key);
    key = json_new_string(option_key);
    json_insert_child(named_array, key);

    key = json_new_string("type");
    value = json_new_string(type);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    while(Length_BuffSended !=strlen(text))
	{
		Result_Send = send(conn_fd, (text+Length_BuffSended), (strlen(text)-Length_BuffSended), 0);
		if(Result_Send <0)
		{
			iErrno = errno;
			if((EINTR== iErrno)||(EAGAIN ==iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				Del_OVSDB_by_conn_fd(conn_fd);
				return;
			}
		}
		else
		{
			Length_BuffSended += Result_Send;
		}
	}
}

//{"id":"d0d14471-5a66-412c-bcae-0e594f5ee9af","method":"transact","params":["Open_vSwitch",{"op":"mutate","table":"Bridge","where":[["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],"mutations":[["ports","insert",["named-uuid","new_port"]]]},{"op":"insert","table":"Port","row":{"name":"gre-10.8.1.212","interfaces":["set",[["named-uuid","new_interface"]]]},"uuid-name":"new_port"},{"op":"insert","table":"Interface","row":{"name":"gre-10.8.1.212"},"uuid-name":"new_interface"}]}
void add_port(INT4 conn_fd, INT1 *br_uuid, INT1 *port_name)
{
    INT1 *text;
	INT4 Length_BuffSended =0;
	INT4 Result_Send =0;
	INT4 iErrno =0;
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array,
            *uuid_array, *named_array, *row_obj;

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

    key = json_new_string(br_uuid);
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
    value = json_new_string(port_name);
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
    value = json_new_string(port_name);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_interface");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    while(Length_BuffSended !=strlen(text))
	{
		Result_Send = send(conn_fd, (text+Length_BuffSended), (strlen(text)-Length_BuffSended), 0);
		if(Result_Send <0)
		{
			iErrno = errno;
			if((EINTR== iErrno)||(EAGAIN ==iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				Del_OVSDB_by_conn_fd(conn_fd);
				return;
			}
		}
		else
		{
			Length_BuffSended += Result_Send;
		}
	}
}

//{"id":"315c15f7-8b78-41ff-880f-fc6211cb5b8e","method":"transact","params":["Open_vSwitch",{"op":"mutate","table":"Open_vSwitch","where":[["_uuid","==",["uuid","17074e89-2ac5-4bba-997a-1a5a3527cf56"]]],"mutations":[["bridges","insert",["named-uuid","new_bridge"]]]},{"op":"insert","table":"Bridge","row":{"name":"br-int","fail_mode":["set",["secure"]],"protocols":["set",["OpenFlow13"]]},"uuid-name":"new_bridge"}]}
void add_bridge(INT4 conn_fd, INT1 *br_name, INT1 *open_vswitch_uuid, INT1 *fail_mode, INT1 *of_proto)
{
    INT1 *text;
	INT4 Length_BuffSended =0;
	INT4 Result_Send =0;
	INT4 iErrno =0;
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

    key = json_new_string(open_vswitch_uuid);
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
    value = json_new_string(br_name);
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

    key = json_new_string(fail_mode);
    json_insert_child(uuid_array, key);

    key = json_new_string("protocols");
    array = json_new_array();
    json_insert_child(key, array);
    json_insert_child(row_obj, key);

    key = json_new_string("set");
    json_insert_child(array, key);

    uuid_array = json_new_array();
    json_insert_child(array, uuid_array);

    key = json_new_string(of_proto);
    json_insert_child(uuid_array, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_bridge");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    while(Length_BuffSended !=strlen(text))
	{
		Result_Send = send(conn_fd, (text+Length_BuffSended), (strlen(text)-Length_BuffSended), 0);
		if(Result_Send <0)
		{
			iErrno = errno;
			if((EINTR== iErrno)||(EAGAIN ==iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				Del_OVSDB_by_conn_fd(conn_fd);
				return;
			}
		}
		else
		{
			Length_BuffSended += Result_Send;
		}
	}
}

void add_tunnel(ovsdb_server_t *compute_node, ovs_bridge_t *compute_bridge)
{
    INT4 index;
    INT4 index_br;
    INT1 control_tunnel_port_name[32];
    INT1 compute_tunnel_port_name[32];
    INT1 *remote_ip = NULL;
    INT1 *compute_ip = NULL;

    ovsdb_server_t *remote_node = NULL;
    if(g_ovsdb_turnel_on == 0){
    	return;
    }

    for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
    {
        if (g_ovsdb_clients[index])
        {
            remote_node = &g_ovsdb_nodes[index];
            remote_ip = strdup(inet_htoa(ntohl(remote_node->node_ip)));
            compute_ip = strdup(inet_htoa(ntohl(compute_node->node_ip)));

            if (g_tunnel_type == NETWORK_TYPE_GRE)
            {
                sprintf(control_tunnel_port_name, "gre-%s", inet_htoa(ntohl(compute_node->node_ip)));
                sprintf(compute_tunnel_port_name, "gre-%s", inet_htoa(ntohl(remote_node->node_ip)));
            }
            else if(g_tunnel_type == NETWORK_TYPE_VXLAN)
            {
                sprintf(control_tunnel_port_name, "vxlan-%s", inet_htoa(ntohl(compute_node->node_ip)));
                sprintf(compute_tunnel_port_name, "vxlan-%s", inet_htoa(ntohl(remote_node->node_ip)));
            }

            //control---->compute
            for (index_br = 0; index_br < NEUTRON_BRIDGE_MAX_NUM; index_br++)
            {
                if ((g_ovsdb_nodes[index].bridge[index_br].is_using) && (strcmp(remote_node->bridge[index_br].name, INTERNAL_BR) == 0))
                {
                    if (g_tunnel_type == NETWORK_TYPE_GRE)
                    {
                        add_port_and_portoption(remote_node->node_fd, remote_node->bridge[index_br]._uuid,
                                control_tunnel_port_name, remote_ip, compute_ip, "key", INTERFACE_TYPE_GRE);
                    }
                    else if (g_tunnel_type == NETWORK_TYPE_VXLAN)
                    {
                        add_port_and_portoption(remote_node->node_fd, remote_node->bridge[index_br]._uuid,
                                control_tunnel_port_name, remote_ip, compute_ip, "key", INTERFACE_TYPE_VXLAN);
                    }
                    break;
                }
            }

            //compute---->control
            if (g_tunnel_type == NETWORK_TYPE_GRE)
            {
                add_port_and_portoption(compute_node->node_fd, compute_bridge->_uuid, compute_tunnel_port_name,
                        compute_ip, remote_ip, "key", INTERFACE_TYPE_GRE);
            }
            else if (g_tunnel_type == NETWORK_TYPE_VXLAN)
            {
                add_port_and_portoption(compute_node->node_fd, compute_bridge->_uuid, compute_tunnel_port_name,
                        compute_ip, remote_ip, "key", INTERFACE_TYPE_VXLAN);
            }

            free(remote_ip);
            free(compute_ip);
        }
    }
}

//by:yhy 构造控制器相关的JSON数据信息,并发送给ovsdb,对ovsdb进行设置
//{"id":"2e93096c-ab33-4abd-a244-bad919566fac","method":"transact","params":["Open_vSwitch",{"op":"mutate","table":"Bridge","where":[["_uuid","==",["uuid","612c720c-52a5-4231-bb51-ff94a0d38671"]]],"mutations":[["controller","insert",["named-uuid","new_controller"]]]},{"op":"insert","table":"Controller","row":{"target":"tcp:10.8.1.211:6633"},"uuid-name":"new_controller"}]}
void set_controller(INT4 conn_fd, INT1 *br_uuid, INT1 *controller_ip)
{
    INT1 *text;
	INT4 Length_BuffSended =0;
	INT4 Result_Send =0;
	INT4 iErrno =0;
    json_t *root_array, *array, *obj, *key, *value, *db_obj, *_uuid_array,
            *uuid_array, *row_obj;

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

    key = json_new_string(br_uuid);
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

    //
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
    value = json_new_string(controller_ip);
    json_insert_child(key, value);
    json_insert_child(row_obj, key);

    key = json_new_string("uuid-name");
    value = json_new_string("new_controller");
    json_insert_child(key, value);
    json_insert_child(db_obj, key);

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    while(Length_BuffSended !=strlen(text))
	{
		Result_Send = send(conn_fd, (text+Length_BuffSended), (strlen(text)-Length_BuffSended), 0);
		if(Result_Send <0)
		{
			iErrno = errno;
			if((EINTR== iErrno)||(EAGAIN ==iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				Del_OVSDB_by_conn_fd(conn_fd);
				return;
			}
		}
		else
		{
			Length_BuffSended += Result_Send;
		}
	}

}

//{"id":"292174f5-5da9-4d7f-b9a9-1f0b787607c6","method":"transact","params":["Open_vSwitch",{"op":"update","table":"Bridge","where":[["_uuid","==",["uuid","6656bc46-915c-4eef-8e07-3c53cf916387"]]],"row":{"fail_mode":["set",["secure"]],"protocols":["set",["OpenFlow13"]]}}]}
void set_failmod_and_ofver(INT4 conn_fd, INT1 *_uuid_br, INT1 *fail_mode, INT1 *ofproto)
{
    INT1 *text;
	INT4 Length_BuffSended =0;
	INT4 Result_Send =0;
	INT4 iErrno =0;
    json_t *array, *obj, *key, *value, *db_obj, *_uuid_array, *uuid_array,
            *row_obj;

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

    key = json_new_string(_uuid_br);
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

    key = json_new_string(fail_mode);
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

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    while(Length_BuffSended !=strlen(text))
	{
		Result_Send = send(conn_fd, (text+Length_BuffSended), (strlen(text)-Length_BuffSended), 0);
		if(Result_Send <0)
		{
			iErrno = errno;
			if((EINTR== iErrno)||(EAGAIN ==iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				Del_OVSDB_by_conn_fd(conn_fd);
				return;
			}
		}
		else
		{
			Length_BuffSended += Result_Send;
		}
	}
}
//by;yhy socket中echo回复
//{"id":"echo","result":[]}
void echo_reply_ovsdb(INT4 conn_fd)
{
    INT1 *text;
	INT4 Length_BuffSended =0;
	INT4 Result_Send =0;
	INT4 iErrno =0;
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

    json_tree_to_string(obj, &text);
    json_free_value(&obj);
    while(Length_BuffSended !=strlen(text))
	{
		Result_Send = send(conn_fd, (text+Length_BuffSended), (strlen(text)-Length_BuffSended), 0);
		if(Result_Send <0)
		{
			iErrno = errno;
			if((EINTR== iErrno)||(EAGAIN ==iErrno))
			{
				continue;
			}
			else
			{
				LOG_PROC("ERROR", "%s,Send Error Code: %d",FN,iErrno);
				Del_OVSDB_by_conn_fd(conn_fd);
				return;
			}
		}
		else
		{
			Length_BuffSended += Result_Send;
		}
	}
}
//by:yhy why?
BOOL handle_search_host_by_mac(INT4 client_ip, INT4 seq, json_t *result)
{
    json_t *tmp, *row, *ofport, *externalids, *in_use_mac, *attach_mac = NULL;
	UINT4 port = 0;
	UINT1 host_mac[6] = {0};
	UINT1 phy_mac[6] = {0};

	if ((result) && (result->child) && (result->child->child)) 
	{
		tmp = result->child->child;
    	row = json_find_first_label(tmp, "rows");
	}
    if ((row) && (row->child) && (row->child->child))
    {	
    	tmp = row->child->child;
		if (tmp) 
		{
			ofport = json_find_first_label(tmp, "ofport");
			if ((ofport) && (ofport->child)&&(ofport->child->text)) 
			{
				port = strtoul(ofport->child->text, 0, 10);
			}

			in_use_mac = json_find_first_label(tmp, "mac_in_use");
			if ((in_use_mac) && (in_use_mac->child)) {
				macstr2hex(in_use_mac->child->text, phy_mac);
			}
			
			externalids = json_find_first_label(tmp, "external_ids");
			if ((externalids) && (externalids->child) && (externalids->child->child) && (externalids->child->child->next)
				&& (externalids->child->child->next->child) && (externalids->child->child->next->child->child))
			{
				tmp = externalids->child->child->next->child->child;
				if (tmp) {
					if (0 == strcmp(tmp->text, "attached-mac")) {
						attach_mac = tmp->next;
					}
					if (attach_mac) {
						macstr2hex(attach_mac->text, host_mac);
					}
				}
			}
		}
    }

	if (port) 
	{
		gn_switch_t* sw = find_sw_by_port_physical_mac(phy_mac);
		if (sw) 
		{
			update_openstack_host_port_by_mac(host_mac, sw, port);
		}
	}
		
    return TRUE;
}

BOOL handle_controller_table(INT4 client_fd, INT4 seq, json_t *result)
{
    json_t *controller = NULL;

    //锟斤拷锟斤拷Controller
    controller = json_find_first_label(result->child, "Controller");
    if (controller)
    {
        return TRUE;
    }

    return FALSE;
}

//by:yhy why?
void handle_openvswitch_table(INT4 client_fd, INT4 seq, json_t *result)
{
    json_t *open_vswitch = NULL;
    json_t *open_vswitch_uuid = NULL;

    //Open_vSwitch
    open_vswitch = json_find_first_label(result->child, "Open_vSwitch");
    if (open_vswitch)
    {
        open_vswitch_uuid = open_vswitch->child->child;
        if (open_vswitch_uuid)
        {
            //open_vswitch._uuid
            memcpy(g_ovsdb_nodes[seq].open_vswitch._uuid, open_vswitch_uuid->text, strlen(open_vswitch_uuid->text));
            LOG_PROC("INFO", "Hand open_vswitch uuid: %s", g_ovsdb_nodes[seq].open_vswitch._uuid);
        }
    }
}

void handle_bridge_table(INT4 client_fd, INT4 seq, json_t *result, BOOL have_controller)
{
    UINT4 br_idx = 0;
    BOOL has_br_int = FALSE;
    json_t *br = NULL;
    json_t *br_uuid = NULL;
    json_t *br_name = NULL;
    json_t *br_dpid = NULL;
    json_t *br_ctrl = NULL;
	json_t *br_port = NULL;

    INT1 controller_ip[128];

    //Bridge
    br = json_find_first_label(result->child, "Bridge");
    if (br)
    {
        br_uuid = br->child->child;
        while(br_uuid)
        {
            memcpy(g_ovsdb_nodes[seq].bridge[br_idx]._uuid, br_uuid->text, strlen(br_uuid->text));
            br_name = json_find_first_label(br_uuid->child->child->child, "name");
            if(br_name)
            {
                if(0 == strcmp(INTERNAL_BR, br_name->child->text))
                {
                    has_br_int = TRUE;
                }

                memcpy(g_ovsdb_nodes[seq].bridge[br_idx].name, br_name->child->text, strlen(br_name->child->text));
                json_free_value(&br_name);
            }

            g_ovsdb_nodes[seq].bridge[br_idx].dpid = 0;
            br_dpid = json_find_first_label(br_uuid->child->child->child, "datapath_id");
            if(br_dpid)
            {
                INT1 *e = NULL;
				if(NULL != br_dpid->child->text)
				{
                	g_ovsdb_nodes[seq].bridge[br_idx].dpid = strtoul (br_dpid->child->text, &e, 16);
				}
                json_free_value(&br_dpid);
            }

            br_ctrl = json_find_first_label(br_uuid->child->child->child, "controller");
            if(br_ctrl)
            {
                if ((FALSE == have_controller) && (0 != strcmp(br_ctrl->child->child->text, "uuid")))
                {
                    sprintf(controller_ip, "tcp:%s:%d", inet_htoa(g_controller_ip), g_controller_south_port);
                    set_controller(client_fd, g_ovsdb_nodes[seq].bridge[br_idx]._uuid, controller_ip);
                }

                json_free_value(&br_ctrl);
            }

            if (g_ovsdb_of_version == OFP10_VERSION)
            {
                set_failmod_and_ofver(client_fd, g_ovsdb_nodes[seq].bridge[br_idx]._uuid, FAIL_MODE_SECURE, OFP_10);
            }
            else if (g_ovsdb_of_version == OFP13_VERSION)
            {
                set_failmod_and_ofver(client_fd, g_ovsdb_nodes[seq].bridge[br_idx]._uuid, FAIL_MODE_SECURE, OFP_13);
            }

            g_ovsdb_nodes[seq].bridge[br_idx].is_using = TRUE;

			br_port = json_find_first_label(br_uuid->child->child->child, "ports");
			if (br_port) {
				json_t* br_p = br_port->child->child->next->child;
				
				while (br_p) {
					// clear all qos info
					clear_qos_in_port_table(client_fd, br_p->child->next->text);
					clear_qos_in_qos_table(client_fd);
					clear_queue_in_queue_table(client_fd);
					
					br_p = br_p->next;
				}
			}

            br_idx++;
            br_uuid = br_uuid->next;
        }
    }

    if(!has_br_int)    //add br-int
    {
        if (g_ovsdb_of_version == OFP10_VERSION)
        {
            add_bridge(client_fd, INTERNAL_BR, g_ovsdb_nodes[seq].open_vswitch._uuid, FAIL_MODE_SECURE, OFP_10);
        }
        else if (g_ovsdb_of_version == OFP13_VERSION)
        {
            add_bridge(client_fd, INTERNAL_BR, g_ovsdb_nodes[seq].open_vswitch._uuid, FAIL_MODE_SECURE, OFP_13);
        }
    }
}
//by:yhy 处理ovsdb发送过来的json消息
//ovsdb_msg:json数据节点,client_fd:ovsdb的套接字句柄,client_ip:ovsdb的IP,seq:本ovsdb在list中的索引
void proc_ovsdb_msg(INT1 *ovsdb_msg, INT4 client_fd, UINT4 client_ip, INT4 seq)
{
    json_t *id = NULL;
    json_t *result = NULL;
    json_t *method = NULL;
    json_t *params = NULL;
    json_t *br = NULL;
	json_t *queue = NULL;
	json_t *qos = NULL;
    json_t *root = NULL;
    json_t *br_dpid = NULL;
    json_t *br_ctrl = NULL;
    INT1* test = NULL;
    UINT4 br_idx = 0;
    INT4 parse_type = 0;
    INT1 controller_ip[128];
    BOOL have_controller = TRUE;
	//by:yhy 校验ovsdb_msg是否为json格式,是则将其构造到root节点上
    parse_type = json_parse_document (&root, ovsdb_msg);
    if(parse_type != JSON_OK)
    {
        //printf("json_error type:   %d\n", parse_type);
        return;
    }

    if(NULL == root)
    {
        return;
    }
	//by:yhy 此处无用
    json_tree_to_string(root, &test);
	//by:yhy 查找"method"JSON对象
    method = json_find_first_label(root, "method");
    if (method)
    {
        if (strncmp(method->child->text, "update", 6) == 0)   
        {//by:yhy method->child->text
            id = method->next;
            if (id)
            {//by:yhy method->next
                if (id->child->type == JSON_NULL)
                {
                    params = id->next;
                    if (params)
                    {//by:yhy method->next->next
                        br = json_find_first_label(params->child->child->next, "Bridge");
                        if (br)
                        {//by:yhy method->next->next->child->child->next
                            // printf("%s\n", ovsdb_msg);
                            while(br_idx < NEUTRON_BRIDGE_MAX_NUM)
                            {//by:yhy 遍历g_ovsdb_nodes[seq].bridge
                                if (g_ovsdb_nodes[seq].bridge[br_idx].is_using == FALSE)
                                {//by:yhy 判断桥是否启用
									//by:yhy 赋值bridge name
                                    memcpy(g_ovsdb_nodes[seq].bridge[br_idx].name, INTERNAL_BR, strlen(INTERNAL_BR));
                                    //by:yhy 赋值bridge uuid
									memcpy(g_ovsdb_nodes[seq].bridge[br_idx]._uuid, br->child->child->text, strlen(br->child->child->text));
                                    LOG_PROC("INFO", "Switch info: name[%s], uuid[%s]", g_ovsdb_nodes[seq].bridge[br_idx].name, g_ovsdb_nodes[seq].bridge[br_idx]._uuid);
                                    sprintf(controller_ip, "tcp:%s:%d", inet_htoa(g_controller_ip), g_controller_south_port);

									//by:yhy 赋值bridge dpid
                                    g_ovsdb_nodes[seq].bridge[br_idx].dpid = 0;
                                    br_dpid = json_find_first_label(br->child->child->child->child->child, "datapath_id");
                                    if(br_dpid)
                                    {
                                        UINT1 _dpid[8];
                                        memcpy(_dpid, br_dpid->text, 16);
                                        uc8_to_ulli64(_dpid, &g_ovsdb_nodes[seq].bridge[br_idx].dpid);
                                        json_free_value(&br_dpid);
                                    }

                                    //by:yhy 向ovsdb设置控制器相关信息
                                    br_ctrl = json_find_first_label(br->child->child->child->child->child, "controller");
                                    if(br_ctrl)
                                    {
                                        if(0 != strcmp(br_ctrl->child->child->text, "uuid"))
                                        {
                                            sprintf(controller_ip, "tcp:%s:%d", inet_htoa(g_controller_ip), g_controller_south_port);
                                            set_controller(client_fd, g_ovsdb_nodes[seq].bridge[br_idx]._uuid, controller_ip);
                                        }
                                        json_free_value(&br_ctrl);
                                    }

                                    //by:yhy 向ovsdb设置port,tunnel相关信息
                                    add_port(client_fd, g_ovsdb_nodes[seq].bridge[br_idx]._uuid, g_ovsdb_nodes[seq].bridge[br_idx].name);
                                    add_tunnel(&g_ovsdb_nodes[seq], &g_ovsdb_nodes[seq].bridge[br_idx]);
                                    g_ovsdb_nodes[seq].bridge[br_idx].is_using = TRUE;
                                    break;
                                }
								br_idx++;
                            }

                            return;
                        }
						
						queue = json_find_first_label(params->child->child->next, "Queue");
						if (queue) {
							notify_recevice_queue_uuid(queue);
						}

						qos = json_find_first_label(params->child->child->next, "QoS");
						if (qos) {
							notify_recevice_qos_uuid(qos);
						}
						
                    }
                }
            }
        }
    }
	//by:yhy 查找"id"JSON对象
    id = json_find_first_label(root, "id");
    if (id && id->child && id->child->text)
    {
        if (strncmp(id->child->text, "echo", 4) == 0)                  
        {//by:yhy echo回复
            echo_reply_ovsdb(client_fd);
        }
        else if (strncmp(id->child->text, SEARCH_ALL_TABLE_ID, 2) == 0) //bridge
        {
            result = id->next;
            if (result)
            {
                handle_openvswitch_table(client_fd, seq, result);
                have_controller = handle_controller_table(client_fd, seq, result);
                handle_bridge_table(client_fd, seq, result, have_controller);
            }
        }
		else if (strncmp(id->child->text, SEARCH_HOST_BY_MAC, 2) == 0) 
		{
			result = id->next;
			if (result) {
				handle_search_host_by_mac(client_ip, seq, result);
			}
		}
		else if (strncmp(id->child->text, SEARCH_INTERFACE_BY_PORT_NO, 2) == 0) 
		{
			result = id->next;
			if (result) {
				notify_receive_interface_uuid(result);
			}
		}
		else if (strncmp(id->child->text, SEARCH_PORT_BY_INTERFACE, 2) == 0) 
		{
			result = id->next;
			if (result) {
				notify_receive_port_uuid(result);
			}
		}

		
		else {
			}

		
        json_free_value(&id);
    }

    json_free_value(&root);
}

INT4 get_json(const INT1 *json_str)
{
    INT4 parentheses_cnt_1 = 0;
    INT4 parentheses_cnt_2 = 0;
    INT4 length = 0;

    if((NULL == json_str) || (json_str[0] == '\0'))
    {
        return 0;
    }

    for(; json_str[length] != '\0'; length++)
    {
        switch(json_str[length])
        {
            case '{':
                parentheses_cnt_1++;
                break;
            case '[':
                parentheses_cnt_2++;
                break;
            case '}':
                parentheses_cnt_1--;
                if((parentheses_cnt_1 == 0) && (parentheses_cnt_2 == 0))
                {
                    length++;
                    goto EXIT;
                }
                break;
            case ']':
                parentheses_cnt_2--;
                if((parentheses_cnt_1 == 0) && (parentheses_cnt_2 == 0))
                {
                    length++;
                    goto EXIT;
                }
                break;
            default:
                break;
        }

    }

    length = 0;
EXIT:
    return length;
}

INT4 ovsdb_FindClientIndexBySockfd(INT4 iSockFd)
{
	INT4 i = 0;
	for(i=0; i< OVSDB_MAX_CONNECTION; i++)
	{
		if(iSockFd == g_ovsdb_clients[i])
		{
			return i;
		}
	}
	return -1;
}

INT4 ovsdb_new_client(INT4 iSockFd, struct sockaddr_in addr)
{
	INT4 index = 0;
	UINT4 clientip = 0;    
    UINT2 client_port = 0;
	
	clientip = *(UINT4 *) &addr.sin_addr;   
    client_port = *(UINT2 *) &addr.sin_port;

    LOG_PROC("INFO", "New OVSDB connected[%s:%d]", inet_htoa(ntohl(clientip)),ntohs(client_port));
    search_from_ovsdb_table_all(iSockFd);
	for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
	{//by:yhy 遍历g_ovsdb_clients
	    if (g_ovsdb_clients[index] == 0)
	    {//by:yhy 检索到未使用的节点,开始初始化
	        g_ovsdb_clients[index] = iSockFd;
	        g_ovsdb_clients_bak[index] = iSockFd;
	        g_ovsdb_clients_ip[index] = *(UINT4 *) &addr.sin_addr;

	        g_ovsdb_nodes[index].bridge[0].is_using = FALSE;
	        g_ovsdb_nodes[index].bridge[1].is_using = FALSE;
	        g_ovsdb_nodes[index].bridge[2].is_using = FALSE;
	        g_ovsdb_nodes[index].bridge[3].is_using = FALSE;
	        g_ovsdb_nodes[index].bridge[4].is_using = FALSE;

	        g_ovsdb_nodes[index].node_ip =  *(UINT4 *) &addr.sin_addr;
	        g_ovsdb_nodes[index].node_fd = iSockFd;

			EpollSvr_AddSockFd( g_iOvsdbEpollFd, iSockFd, TRUE,  g_ovsdb_nodes[index].funcptr);
			
	        break;
	    }
	}
	if (index >= OVSDB_MAX_CONNECTION)    // exceed max connections.
    {
        LOG_PROC("ERROR", "Max OVSDB connected limited [%d]", OVSDB_MAX_CONNECTION);
        close(iSockFd);
		return GN_ERR;
    }
	return GN_OK;
}
INT4 ovsdb_client_read(INT4 iSockFd)
{
	INT4 index = 0;
	INT1 *p_str = NULL;
	INT4 len = 0;
	INT4 offset = 0;
	INT1 ovsdb_buff[OVSDB_BUFF_LEN + 1] = { 0 };
	INT1 json_string[102400] = {0};

	INT4 recv_len = 0;
	INT4 nErr = 0;
	INT4 ret = 0;


	index = ovsdb_FindClientIndexBySockfd(iSockFd);
	if(index < 0)
	{
		return GN_ERR;
	}

	while(1)
	{
		ret = read(iSockFd, ovsdb_buff + recv_len, OVSDB_BUFF_LEN - recv_len);
    	if(ret > 0)
	    {//by:yhy 消息接收成功
	    	recv_len += ret;
	        ovsdb_buff[recv_len] = '\0';
	        p_str = ovsdb_buff;
	        offset = 0;
			//by:yhy 处理接收到的消息
	        do
	        {
				//by:yhy 提取json{}或者[]的这样一个节点
	           len = get_json(p_str);
	           memcpy(json_string, p_str, len);
	           json_string[len] = '\0';
			   //by:yhy 处理一个json节点数据
	           proc_ovsdb_msg(json_string, g_ovsdb_clients[index], g_ovsdb_clients_ip[index], index);

	           offset += len;
	           p_str += len;
	        }while(len);
			//by:yhy 本次读取的数据中json包不完整,说明人需要从socket中读取信息,接续此次读取做拼接处理
	        if(recv_len != offset)
	        {
	            memmove(ovsdb_buff, ovsdb_buff + offset, recv_len - offset );
	            recv_len = recv_len - offset;
	          
	        }
	    }
		else
		{
			nErr  = errno;
			
			if(EAGAIN == nErr) // 缓冲区数据读完
			{
				break;
			}
				
			if(EINTR == nErr)
			{
				continue;
			}
			LOG_PROC("DEBUG", "%s : len=%d nErr=%d",FN,len, nErr);
			break;
		}
	}
	return GN_OK;
}
INT4 ovsdb_client_write(INT4 iSockFd)
{
	return GN_OK;
}

INT4 ovsdb_svr_read(INT4 iSockFd)
{
	INT4 conn_fd = 0;
	INT4 iErrno = 0;
	socklen_t addrlen;
    struct sockaddr_in addr;


	addrlen = sizeof(struct sockaddr);
	while(1)
	{
		conn_fd = accept(iSockFd, (struct sockaddr *) &addr, &addrlen); 
		if(conn_fd <= 0)
		{
			iErrno = errno;
			if(EINTR == iErrno)
			{
				continue;
			}
			break;
		}
		
		ovsdb_new_client(conn_fd, addr);
	}
	return GN_OK;
}
INT4 ovsdb_err_proc(INT4 iSockFd)
{
	INT4 index = 0;
	
	LOG_PROC("DEBUG","func %s iSockFd=%d", FN, iSockFd);
	
	EpollSvr_DelSockFd( g_iOvsdbEpollFd, iSockFd, TRUE );
	close(iSockFd);
	
	index = ovsdb_FindClientIndexBySockfd(iSockFd);
	if(index < 0)
	{
		LOG_PROC("ERROR","can't find sockfd [%d] in ovsdb", iSockFd);
		return GN_ERR;
	} 
	if (g_ovsdb_clients[index])
	{
	    g_ovsdb_clients[index] = 0;
	}
	return GN_OK;
}
#if 0
//by:yhy ovsdb接收消息
void *ovsdb_recv_msg(void *para)
{
    INT4 index;
    INT4 ret = 0;
	//by:yhy 套接字描述符
    INT4 conn_fd = 0;
    struct timeval wait;
    wait.tv_sec = 5;
    wait.tv_usec = 0;
    UINT4 clientip = 0;    
    UINT2 client_port = 0;
    INT1 *p_str = NULL;
    INT4 len = 0;
    INT4 offset = 0;
    INT1 ovsdb_buff[OVSDB_BUFF_LEN + 1] = { 0 };
    INT1 json_string[102400] = {0};

    BOOL recv_flag = FALSE;
    INT4 recv_len = 0;

	//by:yhy 初始化系统监视g_ovsdb_sockfd文件描述符
    fd_set recvmask;
    FD_ZERO(&g_ovsdb_recvmask);
    FD_SET(g_ovsdb_sockfd, &g_ovsdb_recvmask);

	
	prctl(PR_SET_NAME, (unsigned long) "OvsdbRecvThread" ) ;  
    while (1)
    {
        wait.tv_sec = 5;
        wait.tv_usec = 0;
        memcpy(&recvmask, &g_ovsdb_recvmask, sizeof(fd_set));
		//by:yhy select机制
        ret = select(FD_SETSIZE, &recvmask, NULL, NULL, &wait);
        if (ret <= 0)
		{
            continue;
		}
		//by:yhy 检查是否有新的ovsdb连接控制器
        if (FD_ISSET(g_ovsdb_sockfd, &recvmask))
        {//by:yhy 检查g_ovsdb_sockfd是否在recvmask中
            memset(&g_ovsdb_addr, 0, sizeof(struct sockaddr_in));
            socklen_t size = sizeof(struct sockaddr);

            conn_fd = accept(g_ovsdb_sockfd, (struct sockaddr *) &g_ovsdb_addr, &size);    
            clientip = *(UINT4 *) &g_ovsdb_addr.sin_addr;   
            client_port = *(UINT2 *) &g_ovsdb_addr.sin_port;

            LOG_PROC("INFO", "New OVSDB connected[%s:%d] conn_fd=%d", inet_htoa(ntohl(clientip)),ntohs(client_port), conn_fd);
            search_from_ovsdb_table_all(conn_fd);

            for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
            {//by:yhy 遍历g_ovsdb_clients
                if (g_ovsdb_clients[index] == 0)
                {//by:yhy 检索到未使用的节点,开始初始化
                    g_ovsdb_clients[index] = conn_fd;
                    g_ovsdb_clients_bak[index] = conn_fd;
                    g_ovsdb_clients_ip[index] = clientip;

                    g_ovsdb_nodes[index].bridge[0].is_using = FALSE;
                    g_ovsdb_nodes[index].bridge[1].is_using = FALSE;
                    g_ovsdb_nodes[index].bridge[2].is_using = FALSE;
                    g_ovsdb_nodes[index].bridge[3].is_using = FALSE;
                    g_ovsdb_nodes[index].bridge[4].is_using = FALSE;

                    g_ovsdb_nodes[index].node_ip = clientip;
                    g_ovsdb_nodes[index].node_fd = conn_fd;

					//by:yhy将conn_fd这个套接字描述符加入g_ovsdb_recvmask
                    FD_SET(conn_fd, &g_ovsdb_recvmask);

                    break;
                }
            }

            if (index >= OVSDB_MAX_CONNECTION)    // exceed max connections.
            {
                LOG_PROC("ERROR", "Max OVSDB connected limited [%d]", OVSDB_MAX_CONNECTION);
                close(conn_fd);
            }
        }

		//by:yhy 检查是否有ovsdb发送信息给控制器
        for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
        {//by:yhy 遍历g_ovsdb_clients
            recv_len = 0;
            recv_flag = FALSE;

            if (g_ovsdb_clients[index])
            {//by:yhy 遍历查找是哪个套接字描述符出现在recvmask中
                if (FD_ISSET(g_ovsdb_clients[index], &recvmask))
                {//by:yhy 当前ovsdb发送消息,接收套接字中数据并处理	
                    do
                    {
                        recv_len += read(g_ovsdb_clients[index], ovsdb_buff + recv_len, OVSDB_BUFF_LEN - recv_len);
                        if(recv_len > 0)
                        {//by:yhy 消息接收成功
                            ovsdb_buff[recv_len] = '\0';
                            p_str = ovsdb_buff;
                            offset = 0;
                            recv_flag = FALSE;
							//by:yhy 处理接收到的消息
                            do
                            {
								//by:yhy 提取json{}或者[]的这样一个节点
                               len = get_json(p_str);
                               memcpy(json_string, p_str, len);
                               json_string[len] = '\0';
							   //by:yhy 处理一个json节点数据
                               proc_ovsdb_msg(json_string, g_ovsdb_clients[index], g_ovsdb_clients_ip[index], index);

                               offset += len;
                               p_str += len;
                            }while(len);
							//by:yhy 本次读取的数据中json包不完整,说明人需要从socket中读取信息,接续此次读取做拼接处理
                            if(recv_len != offset)
                            {
                                memmove(ovsdb_buff, ovsdb_buff + offset, recv_len - offset );
                                recv_len = recv_len - offset;
                                recv_flag = TRUE;
                            }
                        }
                        else
                        {//by:yhy 套接口死亡或者出错	
                            if (is_socket_dead(recv_len))
                            {
                                if (g_ovsdb_clients[index])
                                {
									//by:yhy 清除该套接字描述符
                                    FD_CLR(g_ovsdb_clients[index], &g_ovsdb_recvmask);
                                    close(g_ovsdb_clients[index]);
                                    g_ovsdb_clients[index] = 0;
                                }
                                recv_flag = FALSE;
                            }
                        }
                    }while(recv_flag);
                }
            }
        }
    }
}
#else
void *ovsdb_recv_msg(void *para)
{
	INT4 ret = 0;
	stEpollsvr_Sockfuncptr *pstEpollsvr_SockfuncAddr = NULL;

	prctl(PR_SET_NAME, (unsigned long) "OvsdbRecvThread" ) ;  
	ret = EpollSvr_Create(&g_iOvsdbEpollFd);
	if (GN_OK != ret) 
	{
		return NULL;
	}
	
	pstEpollsvr_SockfuncAddr =  register_EpollSock_Handle(ovsdb_svr_read, NULL, ovsdb_err_proc);
	if (NULL == pstEpollsvr_SockfuncAddr) 
	{
		return NULL;
	}
	EpollSvr_AddSockFd( g_iOvsdbEpollFd, g_ovsdb_sockfd, TRUE,  pstEpollsvr_SockfuncAddr );

	EpollSvr_Loop(g_iOvsdbEpollFd, TRUE);
	return NULL;
}
#endif
//by:yhy ovsdb的tcp sever初始化
INT4 ovsdb_connect_init()
{
    struct timeval wait;

    wait.tv_sec = 1;
    wait.tv_usec = 0;
	INT4 sockopt = 1;
    char * value = NULL;
    INT4 tcp_serv_fd = 0;
    UINT4 ssize = sizeof(struct sockaddr_in);

    struct sockaddr_in tcpsaddr;
    memset(&tcpsaddr, 0, sizeof(struct sockaddr_in));
    tcpsaddr.sin_family = AF_INET;
    tcpsaddr.sin_port = htons(g_ovsdb_port);   
    tcpsaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    value = get_value(g_controller_configure, "[ovsdb_conf]", "ovsdb_tunnel_on");
    g_ovsdb_turnel_on = (NULL == value)?1:atoll(value);

    tcp_serv_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcp_serv_fd == -1)
    {
        LOG_PROC("ERROR", "Create OVSDB server failed");
        return GN_ERR;
    }

	//by:yhy 设置套接口 接收超时,发送超时,允许套接口和一个已在使用中的地址捆绑
    setsockopt(tcp_serv_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &wait, sizeof(wait));
    setsockopt(tcp_serv_fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &wait, sizeof(wait));
    setsockopt(tcp_serv_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &sockopt, sizeof(sockopt));
	setsockopt(tcp_serv_fd, SOL_SOCKET, SO_REUSEPORT, (char *) &sockopt, sizeof(sockopt));
	//by:yhy 绑定tcp sever
    if (bind(tcp_serv_fd, (struct sockaddr *) &tcpsaddr, ssize) == -1)
    {
        LOG_PROC("ERROR", "Create OVSDB server failed");
        close(tcp_serv_fd);
    }
    //by:yhy 启动tcp sever监听
    if (listen(tcp_serv_fd, OVSDB_MAX_CONNECTION) == -1)
    {
        LOG_PROC("ERROR", "Create OVSDB server failed");
        close(tcp_serv_fd);
    }

    LOG_PROC("INFO", "Create OVSDB server succeed, listening at [%d]", g_ovsdb_port);
    return tcp_serv_fd;
}
//by:yhy why?
INT4 init_ovsdb()
{
	INT4 index = 0;  
	stEpollsvr_Sockfuncptr *pstOvsdbEpollsvr_SockfuncAddr = NULL;
	
    g_ovsdb_sockfd = ovsdb_connect_init();
    if (g_ovsdb_sockfd < 0)
    {
        return GN_ERR;
    }

	for(index=0; index< OVSDB_MAX_CONNECTION; index++)
	{
		pstOvsdbEpollsvr_SockfuncAddr = register_EpollSock_Handle(ovsdb_client_read, ovsdb_client_write, ovsdb_err_proc);
		if (NULL == pstOvsdbEpollsvr_SockfuncAddr) 
		{
			LOG_PROC("ERROR", "init_ovsdb -- register_EpollSock_Handle gn_malloc  Finall return GN_ERR");
			return GN_ERR;
		}
		g_ovsdb_clients[index] = 0;
		g_ovsdb_nodes[index].node_fd = 0;
		g_ovsdb_nodes[index].funcptr = pstOvsdbEpollsvr_SockfuncAddr ;
	}
    //by:yhy 多线程处理ovsdb中套接字消息处理
    if (pthread_create(&g_ovsdb_recv_tid, NULL, ovsdb_recv_msg, NULL))
    {
        ovsdb_connect_quit();
        return GN_ERR;
    }

    return GN_OK;
}

// get conn fd by sw ip
INT4 get_conn_fd_by_sw_ip(UINT4 sw_ip)
{		
	INT4 index = 0;
	
	for (index = 0; index < OVSDB_MAX_CONNECTION; index++) {		
		if (g_ovsdb_clients_ip[index] == sw_ip) {
			
			return g_ovsdb_clients[index];
		}
	}

	return 0;
}

// get sw ip by conn fd
UINT4 get_sw_ip_by_conn_fd(INT4 conn_fd)
{
	INT4 index = 0;
	
	for (index = 0; index < OVSDB_MAX_CONNECTION; index++) {		
		if (g_ovsdb_clients[index] == conn_fd) {
			
			return g_ovsdb_clients_ip[index];
		}
	}

	return 0;
}
//根据套接字句柄删除OVSDB的相关操作
//成功返回1,失败返回0
UINT1 Del_OVSDB_by_conn_fd(INT4 conn_fd)
{
	INT4 index = 0;

    for (index = 0; index < OVSDB_MAX_CONNECTION; index++)
    {
        if (g_ovsdb_clients[index]== conn_fd)
        {
            close(g_ovsdb_clients[index]);
            g_ovsdb_clients[index] = 0;
			return 1;
        }
    }
	return 0;
}



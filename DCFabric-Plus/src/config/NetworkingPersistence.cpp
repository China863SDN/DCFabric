#include "NetworkingPersistence.h"
#include "NetworkingEventConsumer.h"
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <arpa/inet.h>
#include <assert.h>
#include "log.h"

extern UINT1 MACStringtoArray(string&p_StringMAC,UINT1 p_MACArray[]);
extern UINT1 MACArraytoString(string&p_StringMAC,UINT1 p_MACArray[]);

string IPtoString(UINT4 p_IP)
{
    char strTemp[20];
    sprintf(strTemp,"%d.%d.%d.%d",(p_IP&0x000000ff),(p_IP&0x0000ff00)>>8,(p_IP&0x00ff0000)>>16, (p_IP&0xff000000)>>24 );
    return string(strTemp);
}

UINT4  StringtoIP(string& p_IP)
{
	char ip_charArray[20]={0};
	UINT4 IP_UINT4 =INADDR_NONE;
	strcpy(ip_charArray,p_IP.c_str());
	IP_UINT4=inet_addr(ip_charArray);
    return IP_UINT4;
}

string UINT1toString(UINT1 p_value)
{
    string string_value;
    INT1 charArray[10]={0};
    sprintf(charArray,"%d",p_value);
    string_value=charArray;
    return string_value;
}

UINT1  StringtoUINT1(string& p_value)
{
    return (UINT1)strtoul(p_value.c_str(),NULL,10);
}

NetworkingPersistence* NetworkingPersistence::Instance =NULL;

NetworkingPersistence::NetworkingPersistence(INT1* p_DatabasePath)
{
    INT4 result =0;
    DatabasePath=p_DatabasePath;
    result=sqlite3_open(DatabasePath,&ConfigDB);
    if( result )
    {
        LOG_WARN_FMT("Can't open database: %s\n", sqlite3_errmsg(ConfigDB));
    }
    else
    {
        LOG_WARN("Opened database successfully\n");
    }
}

NetworkingPersistence::~NetworkingPersistence()
{
    if (NULL != ConfigDB)
    {
        sqlite3_close(ConfigDB);
        ConfigDB = NULL;
    }
}

NetworkingPersistence* NetworkingPersistence::Get_Instance(void)
{
    if(Instance == NULL)
    {
        Instance =new NetworkingPersistence(DBPath);
		if(NULL == Instance)
		{
			exit(-1);
		}
	
		CEventNotifier* Notifier = new NetworkingEventConsumer(&G_FloatingMgr,&G_NetworkMgr,&G_RouterMgr,&G_SubnetMgr,&G_PortMgr,&G_ExternalMgr);
		if (NULL != Notifier)
		{
			if (Notifier->onregister() != BNC_OK)
			{
				delete Notifier;
			}
		}
    }

    return Instance;
}

INT4 NetworkingPersistence::init(void)
{   
	assert (RETURN_OK==floating_R());
	assert (RETURN_OK==network_R());
	assert (RETURN_OK==router_R());
	assert (RETURN_OK==subnet_R());
	assert (RETURN_OK==port_R());

	return BNC_OK;
}

int NetworkingPersistence::floating_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::network_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::router_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::subnet_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::port_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}


int NetworkingPersistence::floating_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    int i;

    string string_ID                ="";
    string string_name              ="";
    string string_tenant_ID         ="";
    string string_network_ID        ="";
    string string_router_ID         ="";
    string string_port_ID           ="";
    string string_status            ="";
    string string_fixed_IP          ="";
    string string_floating_IP       ="";
    UINT1  UINT1_status             =0;
    UINT4  UINT4_floating_IP        =INADDR_NONE;
    UINT4  UINT4_fixed_IP           =INADDR_NONE;

    for(i=0; i<argc; i++)
    {
        string ColumnName  = azColName[i];
        string ColumnValue = (argv[i] ? argv[i] : "");
        //LOG_WARN_FMT("%s = %s\n", ColumnName.c_str(), ColumnValue.c_str());
        if(ColumnName =="ID")
        {
            string_ID           =ColumnValue;
        }
        if(ColumnName =="name")
        {
            string_name         =ColumnValue;
        }
        if(ColumnName =="tenant_ID")
        {
            string_tenant_ID    =ColumnValue;
        }
        if(ColumnName =="network_ID")
        {
            string_network_ID   =ColumnValue;
        }
        if(ColumnName =="router_ID")
        {
            string_router_ID    =ColumnValue;
        }
        if(ColumnName =="port_ID")
        {
            string_port_ID      =ColumnValue;
        }
        if(ColumnName =="status")
        {
            string_status       =ColumnValue;
            if(string_status != "")
            {
                UINT1_status =(UINT1)strtoul(string_status.c_str(),NULL,10);
            }
        }
        if(ColumnName =="fixed_IP")
        {
            string_fixed_IP     =ColumnValue;
            if(string_fixed_IP!="")
            {
                UINT4_fixed_IP =StringtoIP(string_fixed_IP);
            }
        }
        if(ColumnName =="floating_IP")
        {
            string_floating_IP  =ColumnValue;
            if(string_floating_IP!="")
            {
                UINT4_floating_IP =StringtoIP(string_floating_IP);
            }
        }
    }

    Base_Floating *CreatedFloating =new Base_Floating(string_ID,string_name,string_tenant_ID,string_network_ID,UINT4_floating_IP);
    CreatedFloating->set_status(UINT1_status);
    CreatedFloating->set_port_ID(string_port_ID);
    CreatedFloating->set_router_ID(string_router_ID);
    CreatedFloating->set_fixed_IP(UINT4_fixed_IP);

    G_NetworkingEventReporter.report_C_Floating(CreatedFloating,PERSTANCE_FALSE);

    return SQLITE_OK;
}
int NetworkingPersistence::network_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    int i;
    string string_ID                ="";
    string string_name              ="";
    string string_tenant_ID         ="";
    string string_status            ="";
    string string_router_external   ="";
    string string_shared            ="";
    string string_subnets           ="";
    UINT1  UINT1_status             =BASE_NETWORK_STATUS_ERROR;
    UINT1  UINT1_router_external    =BASE_NETWORK_NOT_ROUTEREXTERNAL;
    UINT1  UINT1_shared             =BASE_NETWORK_NOT_SHARED;


    for(i=0; i<argc; i++)
    {
        string ColumnName  = azColName[i];
        string ColumnValue = (argv[i] ? argv[i] : "");
        //LOG_WARN_FMT("%s = %s\n", ColumnName.c_str(), ColumnValue.c_str());
        if(ColumnName =="ID")
        {
            string_ID           =ColumnValue;
        }
        if(ColumnName =="name")
        {
            string_name         =ColumnValue;
        }
        if(ColumnName =="tenant_ID")
        {
            string_tenant_ID    =ColumnValue;
        }
        if(ColumnName =="status")
        {
            string_status       =ColumnValue;
            if(string_status != "")
            {
                UINT1_status =(UINT1)strtoul(string_status.c_str(),NULL,10);
            }
        }
        if(ColumnName =="router_external")
        {
            string_router_external     =ColumnValue;
            if(string_router_external!="")
            {
                UINT1_router_external =(UINT1)strtoul(string_router_external.c_str(),NULL,10);
            }
        }
        if(ColumnName =="shared")
        {
            string_shared  =ColumnValue;
            if(string_shared!="")
            {
                UINT1_shared =(UINT1)strtoul(string_shared.c_str(),NULL,10);
            }
        }
        if(ColumnName =="subnets")
        {
            string_subnets    =ColumnValue;
        }
    }

    Base_Network* CreatedNetwork =new Base_Network(string_ID,string_name,string_tenant_ID,UINT1_status,UINT1_router_external,UINT1_shared);

    UINT4 StartPos =0;
    UINT4 TargetPos =0;
    string subnet ="";
    while(StartPos<string_subnets.size())
    {

        TargetPos=string_subnets.find('=',StartPos);
        if(TargetPos)
        {
            subnet =string_subnets.substr(StartPos,TargetPos-StartPos);
            CreatedNetwork->add_subnet(subnet);
            StartPos=TargetPos+1;
            TargetPos=0;

            if(StartPos>=string_subnets.size())
            {
                break;
            }
        }
    }
    G_NetworkingEventReporter.report_C_Network(CreatedNetwork,PERSTANCE_FALSE);

    return SQLITE_OK;
}
int NetworkingPersistence::router_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    int i;
    string string_ID="";
    string string_name ="";
    string string_tenant_ID ="";
    string string_status ="";
    string string_external_network_ID ="";
    string string_external_enable_snat ="";
    string string_external_subnet_ID ="";
    string string_external_IP ="";
    UINT1  UINT1_status=BASE_ROUTER_STATUS_ERROR;
    UINT1  UINT1_external_enable_snat =BASE_ROUTER_SNAT_DISABLED;
    UINT4  UINT4_external_IP=INADDR_NONE;

    for(i=0; i<argc; i++)
    {
        string ColumnName  = azColName[i];
        string ColumnValue = (argv[i] ? argv[i] : "");
        //LOG_WARN_FMT("%s = %s\n", ColumnName.c_str(), ColumnValue.c_str());
        if(ColumnName =="ID")
        {
            string_ID           =ColumnValue;
        }
        if(ColumnName =="name")
        {
            string_name         =ColumnValue;
        }
        if(ColumnName =="tenant_ID")
        {
            string_tenant_ID    =ColumnValue;
        }
        if(ColumnName =="status")
        {
            string_status       =ColumnValue;
            if(string_status != "")
            {
                UINT1_status =(UINT1)strtoul(string_status.c_str(),NULL,10);
            }
        }
        if(ColumnName =="external_network_ID")
        {
            string_external_network_ID =ColumnValue;
        }
        if(ColumnName =="external_enabled_snat")
        {
            string_external_enable_snat =ColumnValue;
            if(string_external_enable_snat != "")
            {
                UINT1_external_enable_snat =StringtoUINT1(string_external_enable_snat);
            }
        }
        if(ColumnName =="external_subnet_ID")
        {
            string_external_subnet_ID  =ColumnValue;
        }
        if(ColumnName =="external_IP")
        {
			
            string_external_IP    =ColumnValue;
            UINT4_external_IP=  StringtoIP(string_external_IP);
        }
    }

    Base_Router * CreatedRouter =new Base_Router(string_ID,string_name,string_tenant_ID);
    CreatedRouter->set_status(UINT1_status);
    CreatedRouter->set__external_gateway_info__network_ID(string_external_network_ID);
    CreatedRouter->set__external_gateway_info__enabled_snat(UINT1_external_enable_snat);
    CreatedRouter->set__external_gateway_info__subnet_ID(string_external_subnet_ID);
    CreatedRouter->set__external_gateway_info__IP(UINT4_external_IP);

    G_NetworkingEventReporter.report_C_Router(CreatedRouter,PERSTANCE_FALSE);

    return SQLITE_OK;
}
int NetworkingPersistence::subnet_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    int i;
    string string_name      ="";
    string string_tenant_ID ="";
    string string_network_ID="";
    string string_cidr      ="";
    string string_gateway_IP ="";
    string string_status    ="";
    string string_IP_start  ="";
    string string_IP_end    ="";
    string string_ID        ="";

    UINT1 UINT1_status      =BASE_SUBNET_STATUS_ERROR;
    UINT4 UINT4_gateway_IP  =INADDR_NONE;
    UINT4 UINT4_IP_start    =INADDR_NONE;
    UINT4 UINT4_IP_end      =INADDR_NONE;


    for(i=0; i<argc; i++)
    {
        string ColumnName  = azColName[i];
        string ColumnValue = (argv[i] ? argv[i] : "");
        //LOG_WARN_FMT("%s = %s\n", ColumnName.c_str(), ColumnValue.c_str());
        if(ColumnName =="ID")
        {
            string_ID           =ColumnValue;
        }
        if(ColumnName =="name")
        {
            string_name         =ColumnValue;
        }
        if(ColumnName =="tenant_ID")
        {
            string_tenant_ID    =ColumnValue;
        }
        if(ColumnName =="status")
        {
            string_status       =ColumnValue;
            if(string_status != "")
            {
                UINT1_status =(UINT1)strtoul(string_status.c_str(),NULL,10);
            }
        }
        if(ColumnName =="cidr")
        {
            string_cidr  =ColumnValue;
        }
        if(ColumnName =="network_ID")
        {
            string_network_ID    =ColumnValue;
        }
        if(ColumnName =="gateway_IP")
        {
            string_gateway_IP    =ColumnValue;
            UINT4_gateway_IP=  StringtoIP(string_gateway_IP);
        }
        if(ColumnName =="IP_start")
        {
            string_IP_start    =ColumnValue;
            UINT4_IP_start=  StringtoIP(string_IP_start);
        }
        if(ColumnName =="IP_end")
        {
            string_IP_end    =ColumnValue;
            UINT4_IP_end=  StringtoIP(string_IP_end);
        }
    }
    Base_Subnet* CreatedSubnet =new Base_Subnet(string_ID,string_name,string_tenant_ID,string_network_ID,string_cidr,UINT4_gateway_IP,UINT4_IP_start,UINT4_IP_end);
    CreatedSubnet->change_status(UINT1_status);
    G_NetworkingEventReporter.report_C_Subnet(CreatedSubnet,PERSTANCE_FALSE);


    return SQLITE_OK;
}
int NetworkingPersistence::port_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    int i;
    string string_ID                ="";
    string string_name              ="";
    string string_tenant_ID         ="";
    string string_status            ="";
    string string_device_owner      ="";
	string string_device_ID         ="";
    string string_network_ID        ="";
    string string_MAC               ="";
    string string_Fixed_IPs         ="";
    string string_subnet_ID         ="";
    string string_IP                ="";

    UINT1 UINT1_status =BASE_PORT_STATUS_ERROR;
    UINT1 UINT1_MAC[6] ={0};
    UINT4 UINT4_IP     =INADDR_NONE;

    for(i=0; i<argc; i++)
    {
        string ColumnName  = azColName[i];
        string ColumnValue = (argv[i] ? argv[i] : "");
        //LOG_WARN_FMT("%s = %s\n", ColumnName.c_str(), ColumnValue.c_str());
        if(ColumnName =="ID")
        {
            string_ID           =ColumnValue;
        }
        if(ColumnName =="name")
        {
            string_name         =ColumnValue;
        }
        if(ColumnName =="tenant_ID")
        {
            string_tenant_ID    =ColumnValue;
        }
        if(ColumnName =="status")
        {
            string_status       =ColumnValue;
            if(string_status != "")
            {
                UINT1_status =(UINT1)strtoul(string_status.c_str(),NULL,10);
            }
        }
        if(ColumnName =="device_owner")
        {
            string_device_owner     =ColumnValue;
        }
        if(ColumnName =="network_ID")
        {
            string_network_ID  =ColumnValue;
        }
		if(ColumnName =="device_ID")
        {
            string_device_ID  =ColumnValue;
        }
        if(ColumnName =="MAC")
        {
            string_MAC    =ColumnValue;
            MACStringtoArray(string_MAC,UINT1_MAC);
        }
        if(ColumnName =="Fixed_IPs")
        {
            string_Fixed_IPs    =ColumnValue;
        }
    }

    Base_Port* CreatedPort =new Base_Port(string_ID,string_name,string_tenant_ID,string_network_ID,string_device_ID,string_device_owner,UINT1_status,UINT1_MAC);


    UINT4 StartPos =0;
    UINT4 TargetPos =0;
    UINT4 TargetPosMark2 =0;
    string string_Fixed_IP ="";
    while(StartPos<string_Fixed_IPs.size())
    {
        TargetPos=string_Fixed_IPs.find('=',StartPos);
        if(TargetPos)
        {
            string_Fixed_IP =string_Fixed_IPs.substr(StartPos,TargetPos-StartPos);
            TargetPosMark2=string_Fixed_IP.find('|',0);
            string_IP=string_Fixed_IP.substr(0,TargetPosMark2);
            string_subnet_ID =string_Fixed_IP.substr(TargetPosMark2+1,string_Fixed_IP.size());
            UINT4_IP =StringtoIP(string_IP);
            CreatedPort->add_fixed_IP(UINT4_IP,string_subnet_ID);

            StartPos=TargetPos+1;
            TargetPos=0;

            if(StartPos>=string_Fixed_IPs.size())
            {
                break;
            }
        }
    }
    G_NetworkingEventReporter.report_C_Port(CreatedPort,PERSTANCE_FALSE);
    return SQLITE_OK;
}


int NetworkingPersistence::floating_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::network_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::router_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::subnet_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::port_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}


int NetworkingPersistence::floating_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::network_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::router_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::subnet_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}
int NetworkingPersistence::port_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName)
{
    return SQLITE_OK;
}




UINT1 NetworkingPersistence::floating_C( Base_Floating & p_floating)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="INSERT INTO floating (ID,name,tenant_ID,network_ID,router_ID,port_ID,status,fixed_IP,floating_IP) VALUES(";

    string string_ID                =p_floating.get_ID();
    string string_name              =p_floating.get_name();
    string string_tenant_ID         =p_floating.get_tenant_ID();
    string string_network_ID        =p_floating.get_network_ID();
    string string_router_ID         =p_floating.get_router_ID();
    string string_port_ID           =p_floating.get_port_ID();
    string string_status            =UINT1toString(p_floating.get_status());
    string string_fixed_IP          =IPtoString(p_floating.get_fixed_IP());
    string string_floating_IP       =IPtoString(p_floating.get_floating_IP());

    SQL.append("'");    SQL.append(string_ID);          SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_name);        SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_tenant_ID);   SQL.append("'");
    SQL.append(",");
	SQL.append("'");    SQL.append(string_network_ID);  SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_router_ID);   SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_port_ID);     SQL.append("'");
    SQL.append(",");
    SQL.append(" ");    SQL.append(string_status);      SQL.append(" ");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_fixed_IP);    SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_floating_IP); SQL.append("'");
    SQL.append(");");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),floating_C_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::floating_R(void)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="SELECT * from floating";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),floating_R_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::floating_U(Base_Floating & p_floating)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="UPDATE floating SET ";


    string string_ID                =p_floating.get_ID();
    string string_name              =p_floating.get_name();
    string string_tenant_ID         =p_floating.get_tenant_ID();
    string string_network_ID        =p_floating.get_network_ID();
    string string_router_ID         =p_floating.get_router_ID();
    string string_port_ID           =p_floating.get_port_ID();
    string string_status            =UINT1toString(p_floating.get_status());
    string string_fixed_IP          =IPtoString(p_floating.get_fixed_IP());
    string string_floating_IP       =IPtoString(p_floating.get_floating_IP());

    SQL.append(" name          ='");SQL.append(string_name);
    SQL.append("',tenant_ID     ='");SQL.append(string_tenant_ID);
    SQL.append("',network_ID    ='");SQL.append(string_network_ID);
    SQL.append("',router_ID     ='");SQL.append(string_router_ID);
    SQL.append("',port_ID       ='");SQL.append(string_port_ID);
    SQL.append("',status        =" );SQL.append(string_status);
    SQL.append(" ,fixed_IP      ='");SQL.append(string_fixed_IP);
    SQL.append("',floating_IP   ='");SQL.append(string_floating_IP);
    SQL.append("' WHERE ID      ='");SQL.append(string_ID);
    SQL.append("';");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),floating_U_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::floating_D(string p_ID)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="DELETE from floating where ID =\"" +p_ID +"\"";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),floating_D_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}

UINT1 NetworkingPersistence::network_C( Base_Network & p_network)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="INSERT INTO network (ID,name,tenant_ID,status,router_external,shared,subnets) VALUES(";

    string string_ID                =p_network.get_ID();
    string string_name              =p_network.get_name();
    string string_tenant_ID         =p_network.get_tenant_ID();
    string string_status            =UINT1toString(p_network.get_status());
    string string_router_external   =UINT1toString(p_network.get_router_external());
    string string_shared            =UINT1toString(p_network.get_shared());
    string string_subnets           ="";
    list<string> subnets;
    list<string>::iterator itor;
    p_network.get_subnets(subnets);
    itor = subnets.begin();
    while (itor != subnets.end())
    {
        string_subnets.append(*itor);
        string_subnets.append("=");
        itor++;
    }

    SQL.append("'");    SQL.append(string_ID);          SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_name);        SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_tenant_ID);   SQL.append("'");
    SQL.append(",");
    SQL.append(" ");    SQL.append(string_status);      SQL.append(" ");
    SQL.append(",");
    SQL.append(" ");    SQL.append(string_router_external);      SQL.append(" ");
    SQL.append(",");
    SQL.append(" ");    SQL.append(string_shared);      SQL.append(" ");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_subnets);     SQL.append("'");
    SQL.append(");");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),network_C_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::network_R(void)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="SELECT * from network";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),network_R_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::network_U(Base_Network & p_network)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="UPDATE network SET ";

    string string_ID                =p_network.get_ID();
    string string_name              =p_network.get_name();
    string string_tenant_ID         =p_network.get_tenant_ID();
    string string_status            =UINT1toString(p_network.get_status());
    string string_router_external   =UINT1toString(p_network.get_router_external());
    string string_shared            =UINT1toString(p_network.get_shared());
    string string_subnets           ="";
    list<string> subnets;
    list<string>::iterator itor;
    p_network.get_subnets(subnets);
    itor = subnets.begin();
    while (itor != subnets.end())
    {
        string_subnets.append(*itor);
        string_subnets.append("=");
        itor++;
    }

    SQL.append("  name            ='");SQL.append(string_name);
    SQL.append("',tenant_ID       ='");SQL.append(string_tenant_ID);
    SQL.append("',status          =" );SQL.append(string_status);
    SQL.append(" ,router_external =" );SQL.append(string_router_external);
    SQL.append(" ,shared          =" );SQL.append(string_shared);
    SQL.append(" ,subnets         ='");SQL.append(string_subnets);
    SQL.append("' WHERE ID        ='");SQL.append(string_ID);
    SQL.append("';");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),network_U_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::network_D(string p_ID)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="DELETE from network where ID =\"" +p_ID +"\"";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),network_D_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}

UINT1 NetworkingPersistence::router_C( Base_Router & p_router)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="INSERT INTO router (ID,name,tenant_ID,status,external_network_ID,external_enabled_snat,external_subnet_ID,external_IP) VALUES(";

    string string_ID=p_router.get_ID();
    string string_name =p_router.get_name();
    string string_tenant_ID =p_router.get_tenant_ID();
    string string_status =UINT1toString(p_router.get_status());
    string string_external_network_ID =p_router.get_external_gateway_info().get_network_ID();
    string string_external_enable_snat =UINT1toString(p_router.get_external_gateway_info().get_enabled_snat());
    string string_external_subnet_ID =p_router.get_external_gateway_info().get_external_IP().get_subnet_ID();
    string string_external_IP =IPtoString(p_router.get_external_gateway_info().get_external_IP().get_IP());


    SQL.append("'");    SQL.append(string_ID);          SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_name);        SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_tenant_ID);   SQL.append("'");
    SQL.append(",");
    SQL.append(" ");    SQL.append(string_status);      SQL.append(" ");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_external_network_ID);         SQL.append("'");
    SQL.append(",");
    SQL.append(" ");    SQL.append(string_external_enable_snat);        SQL.append(" ");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_external_subnet_ID);          SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_external_IP);                 SQL.append("'");
    SQL.append(");");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),router_C_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::router_R(void)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="SELECT * from router";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),router_R_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::router_U(Base_Router & p_router)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="UPDATE router SET ";

    string string_ID=p_router.get_ID();
    string string_name =p_router.get_name();
    string string_tenant_ID =p_router.get_tenant_ID();
    string string_status =UINT1toString(p_router.get_status());
    string string_external_network_ID =p_router.get_external_gateway_info().get_network_ID();
    string string_external_enable_snat =UINT1toString(p_router.get_external_gateway_info().get_enabled_snat());
    string string_external_subnet_ID =p_router.get_external_gateway_info().get_external_IP().get_subnet_ID();
    string string_external_IP =IPtoString(p_router.get_external_gateway_info().get_external_IP().get_IP());

    SQL.append("  name          ='");SQL.append(string_name);
    SQL.append("',tenant_ID     ='");SQL.append(string_tenant_ID);
    SQL.append("',status        =" );SQL.append(string_status);
    SQL.append(" ,external_network_ID      ='");SQL.append(string_external_network_ID);
    SQL.append("',external_enabled_snat     =" );SQL.append(string_external_enable_snat);
    SQL.append(" ,external_subnet_ID       ='");SQL.append(string_external_subnet_ID);
    SQL.append("',external_IP              ='");SQL.append(string_external_IP);
    SQL.append("' WHERE ID      ='");SQL.append(string_ID);
    SQL.append("';");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),router_U_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::router_D(string p_ID)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="DELETE from router where ID =\"" +p_ID +"\"";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),router_D_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}

UINT1 NetworkingPersistence::subnet_C( Base_Subnet & p_subnet)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="INSERT INTO subnet (ID,name,tenant_ID,network_ID,cidr,gateway_IP,status,IP_start,IP_end) VALUES(";

    string string_name      =p_subnet.get_name();
    string string_tenant_ID =p_subnet.get_tenant_ID();
    string string_network_ID =p_subnet.get_network_ID();
    string string_cidr      =p_subnet.get_cidr();
    string string_gateway_IP =IPtoString(p_subnet.get_gateway_IP());
    string string_status    =UINT1toString(p_subnet.get_status());
    string string_IP_start  ="";
    string string_IP_end    ="";
    string string_ID        =p_subnet.get_ID();

    list<IP_pool *> ip_pools;
    list<IP_pool *>::iterator itor;
    p_subnet.get_allocation_pools(ip_pools);
    itor = ip_pools.begin();
    if(itor != ip_pools.end())
    {
        if(*itor != NULL)
        {
            string_IP_start.append(IPtoString((*itor)->get_IP_start()));
            string_IP_end.append(IPtoString((*itor)->get_IP_end()));
        }
    }

    SQL.append("'");    SQL.append(string_ID);          SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_name);        SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_tenant_ID);   SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_network_ID);   SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_cidr);        SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_gateway_IP);  SQL.append("'");
    SQL.append(",");
    SQL.append(" ");    SQL.append(string_status);      SQL.append(" ");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_IP_start);    SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_IP_end);      SQL.append("'");
    SQL.append(");");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),subnet_C_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::subnet_R(void)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="SELECT * from subnet";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),subnet_R_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::subnet_U(Base_Subnet & p_subnet)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="UPDATE subnet SET ";


    string string_name      =p_subnet.get_name();
    string string_tenant_ID =p_subnet.get_tenant_ID();
    string string_network_ID =p_subnet.get_network_ID();
    string string_cidr      =p_subnet.get_cidr();
    string string_gateway_IP =IPtoString(p_subnet.get_gateway_IP());
    string string_status    =UINT1toString(p_subnet.get_status());
    string string_IP_start  ="";
    string string_IP_end    ="";
    string string_ID        =p_subnet.get_ID();

    list<IP_pool *> ip_pools;
    list<IP_pool *>::iterator itor;
    p_subnet.get_allocation_pools(ip_pools);
    itor = ip_pools.begin();
    if(itor != ip_pools.end())
    {
        if(*itor != NULL)
        {
            string_IP_start.append(IPtoString((*itor)->get_IP_start()));
            string_IP_end.append(IPtoString((*itor)->get_IP_end()));
        }
    }

    SQL.append("  name          ='");SQL.append(string_name);
    SQL.append("',tenant_ID     ='");SQL.append(string_tenant_ID);
    SQL.append("',network_ID    ='");SQL.append(string_network_ID);
    SQL.append("',cidr          ='");SQL.append(string_cidr);
    SQL.append("',gateway_IP    ='");SQL.append(string_gateway_IP);
    SQL.append("',status        ="); SQL.append(string_status);
    SQL.append(" ,IP_start      ='");SQL.append(string_IP_start);
    SQL.append("',IP_end        ='");SQL.append(string_IP_end);
    SQL.append("' WHERE ID      ='");SQL.append(string_ID);
    SQL.append("';");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),subnet_U_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::subnet_D(string p_ID)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="DELETE from subnet where ID =\"" +p_ID +"\"";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),subnet_D_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}

UINT1 NetworkingPersistence::port_C( Base_Port & p_port)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="INSERT INTO port (ID,name,tenant_ID,status,device_ID,device_owner,network_ID,MAC,Fixed_IPs) VALUES(";

    string string_ID                =p_port.get_ID();
    string string_name              =p_port.get_name();
    string string_tenant_ID         =p_port.get_tenant_ID();
    string string_status            =UINT1toString(p_port.get_status());
    string string_device_owner      =p_port.get_device_owner();
	string string_device_ID         =p_port.get_device_ID();
    string string_network_ID        =p_port.get_network_ID();
    string string_MAC               ="";
    string string_Fixed_IPs         ="";

    MACArraytoString(string_MAC,p_port.get_MAC());

    list<Fixed_IP*> Fixed_IPs;
    list<Fixed_IP*>::iterator itor;
    p_port.get_Fixed_IPs(Fixed_IPs);
    itor = Fixed_IPs.begin();
    while (itor != Fixed_IPs.end())
    {
        string_Fixed_IPs.append(IPtoString((*itor)->get_IP()));
        string_Fixed_IPs.append("|");
        string_Fixed_IPs.append(((*itor)->get_subnet_ID()));
        string_Fixed_IPs.append("=");
        itor++;
    }

    SQL.append("'");    SQL.append(string_ID);          SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_name);        SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_tenant_ID);   SQL.append("'");
    SQL.append(",");
    SQL.append("");     SQL.append(string_status);      SQL.append("");
    SQL.append(",");
	SQL.append("'");    SQL.append(string_device_ID);   SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_device_owner);SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_network_ID);  SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_MAC);         SQL.append("'");
    SQL.append(",");
    SQL.append("'");    SQL.append(string_Fixed_IPs);   SQL.append("'");
    SQL.append(");");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),port_C_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::port_R(void)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="SELECT * from port";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),port_R_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::port_U(Base_Port & p_port)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="UPDATE port SET ";
	
    string string_ID                =p_port.get_ID();
    string string_name              =p_port.get_name();
    string string_tenant_ID         =p_port.get_tenant_ID();
    string string_status            =UINT1toString(p_port.get_status());
    string string_device_owner      =p_port.get_device_owner();
	string string_device_ID         =p_port.get_device_ID();
    string string_network_ID        =p_port.get_network_ID();
    string string_MAC               ="";
    string string_Fixed_IPs         ="";

    MACArraytoString(string_MAC,p_port.get_MAC());

    list<Fixed_IP*> Fixed_IPs;
    list<Fixed_IP*>::iterator itor;
    p_port.get_Fixed_IPs(Fixed_IPs);
    itor = Fixed_IPs.begin();
    while (itor != Fixed_IPs.end())
    {
        string_Fixed_IPs.append(IPtoString((*itor)->get_IP()));
        string_Fixed_IPs.append("|");
        string_Fixed_IPs.append(((*itor)->get_subnet_ID()));
        string_Fixed_IPs.append("=");
        itor++;
    }

    SQL.append("  name            ='");SQL.append(string_name);
    SQL.append("',tenant_ID       ='");SQL.append(string_tenant_ID);
    SQL.append("',status          =" );SQL.append(string_status);
    SQL.append(" ,device_owner    ='");SQL.append(string_device_owner);
	SQL.append("',device_ID       ='");SQL.append(string_device_ID);
    SQL.append("',network_ID      ='");SQL.append(string_network_ID);
    SQL.append("',MAC             ='");SQL.append(string_MAC);
    SQL.append("',Fixed_IPs       ='");SQL.append(string_Fixed_IPs);
    SQL.append("' WHERE ID        ='");SQL.append(string_ID);
    SQL.append("';");

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),port_U_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}
UINT1 NetworkingPersistence::port_D(string p_ID)
{
    INT4 result=0;
    const INT1* data ="Callback function called";
    INT1 * ErrMsg = NULL;
    string SQL ="DELETE from port where ID =\"" +p_ID +"\"";

    LOG_WARN(SQL.c_str());
    result=sqlite3_exec(ConfigDB,SQL.data(),port_D_callback,(void*)data,&ErrMsg);

    if(result != SQLITE_OK)
    {
        //TBD print error
        fprintf(stderr, "SQL error: %s\n", ErrMsg);
        sqlite3_free(ErrMsg);
        return RETURN_ERR;
    }
    else
    {
        //TBD print success
        return RETURN_OK;
    }
}






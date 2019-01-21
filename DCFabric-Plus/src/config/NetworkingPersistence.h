#ifndef NETWORKINGPERSISTENCE_H
#define NETWORKINGPERSISTENCE_H

#include "comm-util.h"
#include "BaseFloating.h"
#include "BaseNetwork.h"
#include "BaseRouter.h"
#include "BaseSubnet.h"
#include "BasePort.h"
#include <sqlite3.h>
#include <assert.h>
#include <string>
#include <stdio.h>
#include "NetworkingEventReporter.h"
#include "NetworkingEventConsumer.h"

#define DBPath "conf/NetworkingConf.db"


#define RETURN_OK       1
#define RETURN_ERR      0
class NetworkingPersistence
{
    public:
        static NetworkingPersistence* Get_Instance(void);

        ~NetworkingPersistence();

		INT4 init(void);

        UINT1 floating_C( Base_Floating & p_floating);
        UINT1 floating_R(void);
        UINT1 floating_U(Base_Floating & p_floating);
        UINT1 floating_D(string p_ID);

        UINT1 network_C( Base_Network & p_network);
        UINT1 network_R(void);
        UINT1 network_U(Base_Network & p_network);
        UINT1 network_D(string p_ID);

        UINT1 router_C( Base_Router & p_router);
        UINT1 router_R(void);
        UINT1 router_U(Base_Router & p_router);
        UINT1 router_D(string p_ID);

        UINT1 subnet_C( Base_Subnet & p_subnet);
        UINT1 subnet_R(void);
        UINT1 subnet_U(Base_Subnet & p_subnet);
        UINT1 subnet_D(string p_ID);

        UINT1 port_C(Base_Port & p_port);
        UINT1 port_R(void);
        UINT1 port_U(Base_Port & p_port);
        UINT1 port_D(string p_ID);

    private:
        NetworkingPersistence(INT1* p_DatabasePath);
        NetworkingPersistence(const NetworkingPersistence&);
        NetworkingPersistence& operator=(const NetworkingPersistence&);

    private:
        static int floating_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int network_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int router_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int subnet_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int port_C_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);

        static int floating_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int network_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int router_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int subnet_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int port_R_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);

        static int floating_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int network_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int router_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int subnet_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int port_U_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);

        static int floating_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int network_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int router_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int subnet_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);
        static int port_D_callback(void * data,INT4 argc,INT1 **argv,INT1 **azColName);

    private:
        static NetworkingPersistence* Instance;

        sqlite3* ConfigDB;
        INT1* DatabasePath;
};

#endif // NETWORKINGPERSISTENCE_H

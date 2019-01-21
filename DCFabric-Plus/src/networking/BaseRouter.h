#ifndef BASE_ROUTER_H
#define BASE_ROUTER_H


#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0

#define BASE_ROUTER_STATUS_ACTIVE      3
#define BASE_ROUTER_STATUS_DOWN        2
#define BASE_ROUTER_STATUS_BUILD       1
#define BASE_ROUTER_STATUS_ERROR       0

#define BASE_ROUTER_SNAT_ENABLED    1
#define BASE_ROUTER_SNAT_DISABLED   0

using namespace std;

class Fixed_IP
{
public:
        const string& get_subnet_ID(void);
        UINT4  get_IP(void);

        UINT1 set_subnet_ID(string p_subnet_ID);
        UINT1 set_IP(UINT4  p_IP);

private:
        string subnet_ID;
        UINT4  IP;
};


class RouterExternalGateway
{
public:
        const string&     get_network_ID(void);
        UINT1      get_enabled_snat(void);
        Fixed_IP & get_external_IP(void);

        UINT1  set_network_ID(string p_network_ID);
        UINT1  set_enabled_snat(UINT1 p_enabled_snat);

private:
        string          network_ID;
        UINT1           enabled_snat;
        Fixed_IP        external_IP;
};


class Base_Router
{
public:
        Base_Router(string p_ID, string p_name, string p_tenant_ID);
        ~Base_Router();

        const string& get_ID(void);
        const string& get_name(void);
        const string& get_tenant_ID(void);
        UINT1  get_status(void);
        RouterExternalGateway & get_external_gateway_info(void);

        UINT1  set_status(UINT1 p_status);
        UINT1  set__external_gateway_info__network_ID(string p_network_ID);
        UINT1  set__external_gateway_info__enabled_snat(UINT1 p_enabled_snat);
        UINT1  set__external_gateway_info__subnet_ID(string p_subnet_ID);
        UINT1  set__external_gateway_info__IP(UINT4  p_IP);

private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

private:
        string ID;                                          //by:yhy    The ID of the subnet.
        string name;                                        //by:yhy    Human-readable name of the subnet.
        string tenant_ID;                                   //by:yhy    The ID of the project.

        UINT1 status;                                       //by:yhy    The subnet status. Values are ACTIVE, DOWN, BUILD or ERROR.
        RouterExternalGateway   external_gateway_info;      //by:yhy    The external gateway information of the router. If the router has an external gateway, this would be a dict with network_id, enable_snat and external_fixed_ips. Otherwise, this would be null.
        pthread_mutex_t BaseRouterMutex;
};

#endif // BASE_ROUTER_H

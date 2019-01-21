#ifndef BASE_NETWORK_H
#define BASE_NETWORK_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0

#define BASE_NETWORK_STATUS_ACTIVE      3
#define BASE_NETWORK_STATUS_DOWN        2
#define BASE_NETWORK_STATUS_BUILD       1
#define BASE_NETWORK_STATUS_ERROR       0

#define BASE_NETWORK_NOT_ROUTEREXTERNAL     0
#define BASE_NETWORK_ROUTEREXTERNAL         1

#define BASE_NETWORK_NOT_SHARED     0
#define BASE_NETWORK_SHARED         1

using namespace std;

class Base_Network
{
public:
        Base_Network(string p_ID, string p_name, string p_tenant_ID, UINT1 p_status, UINT1 p_router_external, UINT1 p_shared);
        ~Base_Network();

        UINT1 change_status(UINT1 p_status);
        UINT1 change_shared(UINT1 p_shared);
        UINT1 change_router_external(UINT1 p_router_external);

        const string& get_ID(void);
        const string& get_name(void);
        const string& get_tenant_ID(void);
        UINT1 get_status(void);
        UINT1 get_router_external(void);
        UINT1 get_shared(void);
        UINT1 get_subnets(list<string>& p_subnets);

        UINT1 add_subnet(string p_subnet_ID);
        UINT1 del_subnet(string p_subnet_ID);
        UINT1 search_subnet(string p_subnet_ID);

private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

private:
        string ID;                      //by:yhy        The ID of the network.
        string name;                    //by:yhy        Human-readable name of the network.
        string tenant_ID;               //by:yhy        The ID of the project.

        UINT1 status;                   //by:yhy        The network status. Values are ACTIVE, DOWN, BUILD or ERROR.
        UINT1 router_external;          //by:yhy        true/false
        UINT1 shared;                   //by:yhy        true/false

        list<string> subnets;           //by:yhy        The associated subnets.

        pthread_mutex_t BaseNetworkMutex;
};
#endif // BASE_NETWORK_H

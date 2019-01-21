#ifndef BASE_FLOATING_H
#define BASE_FLOATING_H


#include "comm-util.h"
#include <string>
#include <vector>

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0


using namespace std;


class Base_Floating
{
public:
        Base_Floating(string p_ID, string p_name, string p_tenant_ID, string p_network_ID, UINT4 p_floating_IP);
        ~Base_Floating();

        const string& get_ID(void);
        const string& get_name(void);
        const string& get_tenant_ID(void);
        const string& get_network_ID(void);
        const string& get_router_ID(void);
        const string& get_port_ID(void);
        UINT1  get_status(void);
        UINT4  get_fixed_IP(void);
        UINT4  get_floating_IP(void);


        UINT1  set_status(UINT1 p_status);
        UINT1  set_port_ID(string p_port_ID);
        UINT1  set_router_ID(string p_router_ID);
        UINT1  set_fixed_IP(UINT4 p_fixed_IP);

private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

private:
        string ID;                                      //by:yhy        The ID of the subnet.
        string name;                                    //by:yhy        Human-readable name of the subnet.
        string tenant_ID;                               //by:yhy        The ID of the project.
        string network_ID;                              //by:yhy        The ID of the subnet to which the subnet belongs.
        string router_ID;
        string port_ID;


        UINT1 status;
        UINT4 fixed_IP;
        UINT4 floating_IP;

        pthread_mutex_t BaseFloatingMutex;
};

#endif // BASE_FLOATING_H

#ifndef BASEPORT_H
#define BASEPORT_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>
#include <cstring>
#include <arpa/inet.h>
#include "BaseRouter.h"
#include "CHostDefine.h"

#define BASE_PORT_STATUS_ACTIVE      3
#define BASE_PORT_STATUS_DOWN        2
#define BASE_PORT_STATUS_BUILD       1
#define BASE_PORT_STATUS_ERROR       0

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0


#define DO_unknown           "unknown"
#define DO_floating          "network:floatingip"
#define DO_compute           "compute:None"
#define DO_dhcp              "network:dhcp"
#define DO_routerInterface   "network:router_interface"
#define DO_routerGateway     "network:router_gateway"
#define DO_LB_HA             "network:LOADBALANCER_HA"
#define DO_LB_VIP            "network:LOADBALANCER_VIP"

using namespace std;



class Base_Port
{
    public:
        Base_Port(string p_ID, string p_name, string p_tenant_ID, string p_network_ID, string p_device_ID,string p_device_owner, UINT1 p_status,UINT1 * p_MAC);
        ~Base_Port();

        const string& get_ID(void);
        const string& get_name(void);
        const string& get_tenant_ID(void);
        const string& get_device_owner(void);
        const string& get_network_ID(void);
		const string& get_device_ID(void);
        UINT1* get_MAC(void);
        UINT1 get_status(void);
        UINT1 get_Fixed_IPs(list<Fixed_IP*>& p_Fixed_IPs);

        UINT1 set_status(UINT1 p_status);
        UINT1 add_fixed_IP(UINT4 p_IP,string p_subnet_ID);
        UINT1 del_fixed_IP(UINT4 p_IP,string p_subnet_ID);
        UINT1 search_IP(UINT4 p_IP);
        UINT1 search_fixed_IP(UINT4 p_IP,string p_subnet_ID);
        UINT4 search_IP_by_subnetID(string p_subnet_ID);

		bnc::host::host_type getDeviceType();
		UINT4 getFirstFixedIp();
		const string getFirstSubnet();
    private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

    private:
        string ID;                      //by:yhy        The ID of the network.
        string name;                    //by:yhy        Human-readable name of the network.
        string tenant_ID;               //by:yhy        The ID of the project.
        string device_owner;
        string network_ID;


		string device_ID;
        UINT1 status;                   //by:yhy        The network status. Values are ACTIVE, DOWN, BUILD or ERROR.

        UINT1 MAC[6];
        list<Fixed_IP*> Fixed_IPs;

        pthread_mutex_t BasePortMutex;
};

#endif // BASEPORT_H

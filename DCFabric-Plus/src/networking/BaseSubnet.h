#ifndef BASE_SUBNET_H
#define BASE_SUBNET_H


#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0

#define BASE_SUBNET_STATUS_ACTIVE       3
#define BASE_SUBNET_STATUS_DOWN         2
#define BASE_SUBNET_STATUS_BUILD        1
#define BASE_SUBNET_STATUS_ERROR        0

using namespace std;

class IP_pool
{
public:
        IP_pool(UINT4 p_IP_start, UINT4 p_IP_end);

        UINT4 get_IP_start(void);
        UINT4 get_IP_end(void);

        UINT1 check_IPinpool(UINT4 p_IP);

private:
        UINT4 IP_start;         //MIN
        UINT4 IP_end;           //MAX
};


class Base_Subnet
{
public:
        Base_Subnet(string p_ID, string p_name, string p_tenant_ID, string p_network_ID, string p_cidr, UINT4 p_gateway_IP, UINT4 p_IP_start, UINT4 p_IP_end);
		~Base_Subnet();
        const string& get_ID(void);
        const string& get_name(void);
        const string& get_tenant_ID(void);
        const string& get_network_ID(void);
        const string& get_cidr(void);
        UINT4  get_gateway_IP(void);
        UINT1  get_status(void);
        UINT1  get_allocation_pools(list< IP_pool *> & p_allocation_pools);

		
		void  set_network_id(const string& str_network_id);
        UINT1 change_status(UINT1 p_status);
        UINT1 change_cidr(string p_cidr);
        UINT1 change_gateway_IP(UINT4 p_gateway_IP);

        UINT1 add_allocation_pool(UINT4 p_IP_start, UINT4 p_IP_end);
        UINT1 del_allocation_pool(UINT4 p_IP_start, UINT4 p_IP_end);

        UINT1 check_IPexisted_in_allocation_pools(UINT4 p_IP);

		UINT4 getFirstStartIp();
		UINT4 getFirstEndIp();

private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

private:
        string ID;                              //by:yhy        The ID of the subnet.
        string name;                            //by:yhy        Human-readable name of the subnet.
        string tenant_ID;                       //by:yhy        The ID of the project.
        string network_ID;                      //by:yhy        The ID of the subnet to which the subnet belongs.
        string cidr;                            //by:yhy        The CIDR of the subnet.

        UINT4 gateway_IP;                       //by:yhy        Gateway IP of this subnet. If the value is?null?that implies no gateway is associated with the subnet
        UINT1 status;                           //by:yhy        The subnet status. Values are?ACTIVE,?DOWN,?BUILD?or?ERROR.

        list<IP_pool *> allocation_pools;               //by:yhy        Allocation pools with?start?and?end?IP addresses for this subnet.

        pthread_mutex_t BaseSubnetMutex;
};

#endif // BASE_SUBNET_H

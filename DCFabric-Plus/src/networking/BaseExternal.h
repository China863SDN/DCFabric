#ifndef BASE_EXTERNAL_H
#define BASE_EXTERNAL_H
#include "comm-util.h"
#include <string>
#include <cstring>
#define RETURN_OK       1
#define RETURN_ERR      0



#define BASE_EXTERNAL_STATUS_PORT_FINDED            1
#define BASE_EXTERNAL_STATUS_PORT_UNFINDED          0

using namespace std;
class Base_External
{
    public:
        Base_External(string p_network_ID,string p_subnet_ID,UINT4 p_gateway_IP);
        ~Base_External();

        const string& get_network_ID(void);
        const string& get_subnet_ID(void);
        UINT4 get_gateway_IP(void);
        UINT1* get_gateway_MAC(void);
        UINT8 get_switch_DPID(void);
        UINT4 get_switch_port(void);
        UINT1 get_status(void);
		
		UINT1 set_gateway_IP(UINT4 p_gateway_IP);
        UINT1 set_gateway_MAC(UINT1 * p_gateway_MAC);
        UINT1 set_switch_DPID(UINT8 p_switch_DPID);
        UINT1 set_switch_port(UINT4 p_switch_port);
        UINT1 set_status(UINT1 p_status);

    private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

    private:
        string network_ID;
        string subnet_ID;
        UINT4 gateway_IP;
        UINT1 gateway_MAC[6];
        UINT8 switch_DPID;
        UINT4 switch_port;
        UINT1 status;

        pthread_mutex_t BaseMutex;
};

#endif // BASE_EXTERNAL_H

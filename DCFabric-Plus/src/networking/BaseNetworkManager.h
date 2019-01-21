#ifndef BASENETWORKMGR_H
#define BASENETWORKMGR_H


#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseNetwork.h"

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0

#define MAX_NETWORK_COUNT       100

using namespace std;

//======================================================================
//by:yhy 考虑换成单例
//======================================================================

class BaseNetworkMgr
{
public:
        BaseNetworkMgr();
        ~BaseNetworkMgr();

        Base_Network * targetNetwork_ByID(const string p_ID);
		
		Base_Network* targetNetwork_Bysubnet_ID(const string&   p_subnet_ID);
        UINT1 targetNetwork_Byname(const string   p_name,list<Base_Network *> & p_networks);
		UINT1 targetNetwork_Bytenant_ID(const string   p_tenant_ID,list<Base_Network *> & p_networks);
        UINT1 targetNetwork_Bystatus(const UINT1    p_status,list<Base_Network *> & p_networks);
        UINT1 targetNetwork_Byrouter_external(const UINT1    p_router_external,list<Base_Network *> & p_networks);
        UINT1 targetNetwork_Byshared(const UINT1    p_shared,list<Base_Network *> & p_networks);

        UINT1 listNetworks(list<Base_Network *> & p_networks);

        UINT1 insertNode_ByNetwork(Base_Network * p_network);
        UINT1 deleteNode_ByNetwork(Base_Network * p_network);
        UINT1 deleteNode_ByNetworkID(const string p_ID);

private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

private:
        pthread_mutex_t BaseNetworkMgrMutex;
        list<Base_Network *> networks;
};


extern BaseNetworkMgr G_NetworkMgr;
#endif // BASENETWORKMGR_H

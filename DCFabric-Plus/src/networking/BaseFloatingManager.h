#ifndef BASEFLOATINGMGR_H
#define BASEFLOATINGMGR_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseFloating.h"

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE      1
#define RETURN_FALSE 0

using namespace std;




//======================================================================
//by:yhy 考虑换成单例
//======================================================================
class BaseFloatingMgr
{
public:
        BaseFloatingMgr();
        ~BaseFloatingMgr();

        Base_Floating * targetFloating_ByID(const string p_ID);


        UINT1 targetFloating_Byname(const string        p_name,list<Base_Floating *> &p_floatings);
        UINT1 targetFloating_Bytenant_ID(const string   p_tenant_ID,list<Base_Floating *> &p_floatings);
        UINT1 targetFloating_Bynetwork_ID(const string  p_network_ID,list<Base_Floating *> &p_floatings);
        UINT1 targetFloating_Byrouter_ID(const string   p_router_ID,list<Base_Floating *> &p_floatings);
        UINT1 targetFloating_Byport_ID(const string     p_port_ID,list<Base_Floating *> &p_floatings);

        UINT1 targetFloating_Bystatus(const UINT1       p_status,list<Base_Floating *> &p_floatings);
        UINT1 targetFloating_Byfixed_IP(const UINT4     p_fixed_IP,list<Base_Floating *> &p_floatings);
        UINT1 targetFloating_Byfloating_IP(const UINT4  p_floating_IP,list<Base_Floating *> &p_floatings);

        UINT1 listFloatings(list<Base_Floating *> &p_floatings);

        UINT1 insertNode_ByFloating(Base_Floating * p_Floating);
        UINT1 deleteNode_ByFloating(Base_Floating * p_Floating);
        UINT1 deleteNode_ByFloatingID(const string p_ID);

private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

private:
        pthread_mutex_t BaseFloatingMgrMutex;
        list<Base_Floating *> floatings;
};

extern BaseFloatingMgr G_FloatingMgr;


#endif // BASEFLOATINGMGR_H

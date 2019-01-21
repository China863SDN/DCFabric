#ifndef BASEROUTERMGR_H
#define BASEROUTERMGR_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseRouter.h"

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE         1
#define RETURN_FALSE    0

using namespace std;

//======================================================================
//by:yhy 考虑换成单例
//======================================================================

class BaseRouterMgr
{
public:
        BaseRouterMgr();
        ~BaseRouterMgr();

        Base_Router * targetRouter_ByID(const string p_ID);
        Base_Router * targetRouter_Byexternal_info(const string p_network_ID, const string p_subnet_ID, const UINT4 p_external_IP);

        UINT1 targetRouter_Byname(const string    p_name,list<Base_Router *> & p_routers);
        UINT1 targetRouter_Bytenant_ID(const string       p_tenant_ID,list<Base_Router *> & p_routers);
        UINT1 targetRouter_Bystatus(const UINT1   p_status,list<Base_Router *> & p_routers);

        UINT1 targetRouter_Byexternal_network_ID(const string     p_network_ID,list<Base_Router *> & p_routers);
        UINT1 targetRouter_Byexternal_subnet_ID(const string      p_subnet_ID,list<Base_Router *> & p_routers);
        UINT1 targetRouter_Byexternal_IP(const UINT4      p_external_IP,list<Base_Router *> & p_routers);

        UINT1 listRouters(list<Base_Router *> & p_routers);

        UINT1 insertNode_ByRouter(Base_Router * p_router);
        UINT1 deleteNode_ByRouter(Base_Router * p_router);
        UINT1 deleteNode_ByRouterID(const string p_ID);

private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

private:
        pthread_mutex_t BaseRouterMgrMutex;
        list<Base_Router *> routers;
};


extern BaseRouterMgr G_RouterMgr;
#endif // BASEROUTERMGR_H

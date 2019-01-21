#include "BaseRouterManager.h"

BaseRouterMgr G_RouterMgr;

BaseRouterMgr::BaseRouterMgr()
{
        pthread_mutex_init(&BaseRouterMgrMutex, NULL);
}

BaseRouterMgr::~BaseRouterMgr()
{
        pthread_mutex_destroy(&BaseRouterMgrMutex);
}

Base_Router * BaseRouterMgr::targetRouter_ByID(const string p_ID)
{
        Base_Router * target = NULL;
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                if ((*itor)->get_ID() == p_ID)
                {
                        target = *itor;
                        break;
                }
                itor++;
        }
        unlock_Mutex();
        return target;
}

Base_Router * BaseRouterMgr::targetRouter_Byexternal_info(const string p_network_ID, const string p_subnet_ID, const UINT4 p_external_IP)
{
        Base_Router * target = NULL;
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                RouterExternalGateway & externalGateway = (*itor)->get_external_gateway_info();
                Fixed_IP & IP_info = externalGateway.get_external_IP();
                if ((externalGateway.get_network_ID() == p_network_ID) && (IP_info.get_subnet_ID() == p_subnet_ID) && (IP_info.get_IP() == p_external_IP))
                {
                        target = *itor;
                        break;
                }
                itor++;
        }
        unlock_Mutex();
        return target;
}

UINT1 BaseRouterMgr::targetRouter_Byname(const string     p_name,list<Base_Router *> & p_routers)
{
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                if ((*itor)->get_name() == p_name)
                {
                        p_routers.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseRouterMgr::targetRouter_Bytenant_ID(const string        p_tenant_ID,list<Base_Router *> & p_routers)
{
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                if ((*itor)->get_tenant_ID() == p_tenant_ID)
                {
                        p_routers.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseRouterMgr::targetRouter_Bystatus(const UINT1    p_status,list<Base_Router *> & p_routers)
{
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                if ((*itor)->get_status() == p_status)
                {
                        p_routers.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseRouterMgr::targetRouter_Byexternal_network_ID(const string      p_network_ID,list<Base_Router *> & p_routers)
{
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                RouterExternalGateway & externalGateway = (*itor)->get_external_gateway_info();
                if (externalGateway.get_network_ID() == p_network_ID)
                {
                        p_routers.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseRouterMgr::targetRouter_Byexternal_subnet_ID(const string       p_subnet_ID,list<Base_Router *> & p_routers)
{
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                RouterExternalGateway & externalGateway = (*itor)->get_external_gateway_info();
                Fixed_IP & IP_info = externalGateway.get_external_IP();
                if (IP_info.get_subnet_ID() == p_subnet_ID)
                {
                        p_routers.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseRouterMgr::targetRouter_Byexternal_IP(const UINT4       p_external_IP,list<Base_Router *> & p_routers)
{
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                RouterExternalGateway & externalGateway = (*itor)->get_external_gateway_info();
                Fixed_IP & IP_info = externalGateway.get_external_IP();
                if (IP_info.get_IP() == p_external_IP)
                {
                        p_routers.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseRouterMgr::listRouters(list<Base_Router *> & p_routers)
{
        list<Base_Router *>::iterator itor;
        lock_Mutex();
        itor = routers.begin();
        while (itor != routers.end())
        {
                p_routers.push_front(*itor);
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseRouterMgr::insertNode_ByRouter(Base_Router * p_router)
{
        lock_Mutex();
        routers.push_front(p_router);
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseRouterMgr::deleteNode_ByRouter(Base_Router * p_router)
{
    if(p_router)
    {
        lock_Mutex();
        routers.remove(p_router);
        unlock_Mutex();
        delete p_router;
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }

}

UINT1 BaseRouterMgr::deleteNode_ByRouterID(const string p_ID)
{
        Base_Router * p_router = targetRouter_ByID(p_ID);
        if (p_router)
        {
                lock_Mutex();
                routers.remove(p_router);
                unlock_Mutex();
                delete p_router;
        }
        return RETURN_OK;
}

void BaseRouterMgr::lock_Mutex(void)
{
        pthread_mutex_lock(&BaseRouterMgrMutex);
}

void BaseRouterMgr::unlock_Mutex(void)
{
        pthread_mutex_unlock(&BaseRouterMgrMutex);
}


#include "BaseNetwork.h"

Base_Network::Base_Network(string p_ID, string p_name, string p_tenant_ID, UINT1 p_status, UINT1 p_router_external, UINT1 p_shared)
{
        ID = p_ID;
        name = p_name;
        tenant_ID = p_tenant_ID;
        status = p_status;
        router_external = p_router_external;
        shared = p_shared;

        pthread_mutex_init(&BaseNetworkMutex, NULL);
}

Base_Network::~Base_Network()
{
        pthread_mutex_destroy(&BaseNetworkMutex);
}

UINT1 Base_Network::change_status(UINT1 p_status)
{
        status = p_status;
        return RETURN_OK;
}

UINT1 Base_Network::change_shared(UINT1 p_shared)
{
        shared = p_shared;
        return RETURN_OK;
}

UINT1 Base_Network::change_router_external(UINT1 p_router_external)
{
        router_external = p_router_external;
        return RETURN_OK;
}

const string& Base_Network::get_ID(void)
{
        return ID;
}

const string& Base_Network::get_name(void)
{
        return name;
}

const string& Base_Network::get_tenant_ID(void)
{
        return tenant_ID;
}

UINT1 Base_Network::get_status(void)
{
        return status;
}

UINT1 Base_Network::get_router_external(void)
{
        return router_external;
}

UINT1 Base_Network::get_shared(void)
{
        return shared;
}

UINT1 Base_Network::get_subnets(list<string>& p_subnets)
{
    list<string>::iterator itor;

    lock_Mutex();
    itor = subnets.begin();
    while (itor != subnets.end())
    {
        p_subnets.push_front((*itor));
        itor++;
    }
    unlock_Mutex();

    return RETURN_OK;

}

UINT1 Base_Network::add_subnet(string p_subnet_ID)
{
        lock_Mutex();
        subnets.push_front(p_subnet_ID);
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 Base_Network::del_subnet(string p_subnet_ID)
{
        lock_Mutex();
        subnets.remove(p_subnet_ID);
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 Base_Network::search_subnet(string p_subnet_ID)
{
        UINT1 result = RETURN_FALSE;
        list<string>::iterator itor;

        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
        {
                if ((*itor) == p_subnet_ID)
                {
                        result = RETURN_TRUE;
                        break;
                }
                itor++;
        }
        unlock_Mutex();

        return result;
}


void Base_Network::lock_Mutex(void)
{
        pthread_mutex_lock(&BaseNetworkMutex);
}

void Base_Network::unlock_Mutex(void)
{
        pthread_mutex_unlock(&BaseNetworkMutex);
}


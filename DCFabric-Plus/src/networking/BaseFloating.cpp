#include "BaseFloating.h"

Base_Floating::Base_Floating(string p_ID, string p_name, string p_tenant_ID, string p_network_ID, UINT4 p_floating_IP)
{
        ID = p_ID;
        name = p_name;
        tenant_ID = p_tenant_ID;
        network_ID = p_network_ID;
        floating_IP = p_floating_IP;

        pthread_mutex_init(&BaseFloatingMutex, NULL);
}

Base_Floating::~Base_Floating()
{
        pthread_mutex_destroy(&BaseFloatingMutex);
}

const string& Base_Floating::get_ID(void)
{
        return ID;
}

const string& Base_Floating::get_name(void)
{
        return name;
}

const string& Base_Floating::get_tenant_ID(void)
{
        return tenant_ID;
}

const string& Base_Floating::get_network_ID(void)
{
        return network_ID;
}

const string& Base_Floating::get_router_ID(void)
{
        return router_ID;
}

const string& Base_Floating::get_port_ID(void)
{
        return port_ID;
}

UINT1  Base_Floating::get_status(void)
{
        return status;
}

UINT4  Base_Floating::get_fixed_IP(void)
{
        return fixed_IP;
}

UINT4  Base_Floating::get_floating_IP(void)
{
        return floating_IP;
}


UINT1  Base_Floating::set_status(UINT1 p_status)
{
        status = p_status;
        return RETURN_OK;
}

UINT1  Base_Floating::set_port_ID(string p_port_ID)
{
        port_ID = p_port_ID;
        return RETURN_OK;
}

UINT1  Base_Floating::set_router_ID(string p_router_ID)
{
        router_ID = p_router_ID;
        return RETURN_OK;
}

UINT1  Base_Floating::set_fixed_IP(UINT4 p_fixed_IP)
{
        fixed_IP = p_fixed_IP;
        return RETURN_OK;
}

void Base_Floating::lock_Mutex(void)
{
        pthread_mutex_lock(&BaseFloatingMutex);
}

void Base_Floating::unlock_Mutex(void)
{
        pthread_mutex_unlock(&BaseFloatingMutex);
}


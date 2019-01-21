#include "BaseRouter.h"

const string& Fixed_IP::get_subnet_ID(void)
{
        return subnet_ID;
}

UINT4  Fixed_IP::get_IP(void)
{
        return IP;
}

UINT1 Fixed_IP::set_subnet_ID(string p_subnet_ID)
{
        subnet_ID = p_subnet_ID;
        return RETURN_OK;
}

UINT1 Fixed_IP::set_IP(UINT4  p_IP)
{
        IP = p_IP;
        return RETURN_OK;
}




const string&     RouterExternalGateway::get_network_ID(void)
{
        return network_ID;
}

UINT1      RouterExternalGateway::get_enabled_snat(void)
{
        return enabled_snat;
}

Fixed_IP & RouterExternalGateway::get_external_IP(void)
{
        return external_IP;
}

UINT1  RouterExternalGateway::set_network_ID(string p_network_ID)
{
        network_ID = p_network_ID;
        return RETURN_OK;
}

UINT1  RouterExternalGateway::set_enabled_snat(UINT1 p_enabled_snat)
{
        enabled_snat = p_enabled_snat;
        return RETURN_OK;
}






Base_Router::Base_Router(string p_ID, string p_name, string p_tenant_ID)
{
        ID = p_ID;
        name = p_name;
        tenant_ID = p_tenant_ID;
        pthread_mutex_init(&BaseRouterMutex, NULL);
}

Base_Router::~Base_Router()
{
        pthread_mutex_destroy(&BaseRouterMutex);
}

const string& Base_Router::get_ID(void)
{
        return ID;
}

const string& Base_Router::get_name(void)
{
        return name;
}

const string& Base_Router::get_tenant_ID(void)
{
        return tenant_ID;
}

UINT1  Base_Router::get_status(void)
{
        return status;
}

RouterExternalGateway & Base_Router::get_external_gateway_info(void)
{
        return external_gateway_info;
}

UINT1  Base_Router::set_status(UINT1 p_status)
{
        status = p_status;
        return RETURN_OK;
}

UINT1  Base_Router::set__external_gateway_info__network_ID(string p_network_ID)
{
        RouterExternalGateway & externalGateway = get_external_gateway_info();
        externalGateway.set_network_ID(p_network_ID);
        return RETURN_OK;

}

UINT1  Base_Router::set__external_gateway_info__enabled_snat(UINT1 p_enabled_snat)
{
        RouterExternalGateway & externalGateway = get_external_gateway_info();
        externalGateway.set_enabled_snat(p_enabled_snat);
        return RETURN_OK;

}

UINT1  Base_Router::set__external_gateway_info__subnet_ID(string p_subnet_ID)
{
        RouterExternalGateway & externalGateway = get_external_gateway_info();
        Fixed_IP & IP_info = externalGateway.get_external_IP();
        IP_info.set_subnet_ID(p_subnet_ID);
        return RETURN_OK;

}

UINT1  Base_Router::set__external_gateway_info__IP(UINT4  p_IP)
{
        RouterExternalGateway & externalGateway = get_external_gateway_info();
        Fixed_IP & IP_info = externalGateway.get_external_IP();
        IP_info.set_IP(p_IP);
        return RETURN_OK;
}





void Base_Router::lock_Mutex(void)
{
        pthread_mutex_lock(&BaseRouterMutex);
}

void Base_Router::unlock_Mutex(void)
{
        pthread_mutex_unlock(&BaseRouterMutex);
}



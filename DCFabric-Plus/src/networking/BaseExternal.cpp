#include "BaseExternal.h"

Base_External::Base_External(string p_network_ID,string p_subnet_ID,UINT4 p_gateway_IP)
{
    network_ID=p_network_ID;
    subnet_ID=p_subnet_ID;
    gateway_IP=p_gateway_IP;
    status=BASE_EXTERNAL_STATUS_PORT_UNFINDED;

    pthread_mutex_init(&BaseMutex, NULL);
}

Base_External::~Base_External()
{
    pthread_mutex_destroy(&BaseMutex);
}

const string& Base_External::get_network_ID(void)
{
    return network_ID;
}
const string& Base_External::get_subnet_ID(void)
{
    return subnet_ID;
}
UINT4 Base_External::get_gateway_IP(void)
{
    return gateway_IP;
}
UINT1* Base_External::get_gateway_MAC(void)
{
    return gateway_MAC;
}
UINT8 Base_External::get_switch_DPID(void)
{
    return switch_DPID;
}
UINT4 Base_External::get_switch_port(void)
{
    return switch_port;
}
UINT1 Base_External::get_status(void)
{
    return status;
}
UINT1 Base_External::set_gateway_IP(UINT4 p_gateway_IP)
{
	gateway_IP=p_gateway_IP;
	memset(gateway_MAC,0,6);
	switch_DPID=0;
	switch_port=0;
    return RETURN_OK;
}
UINT1 Base_External::set_gateway_MAC(UINT1 * p_gateway_MAC)
{
    memcpy(gateway_MAC,p_gateway_MAC,6);
    return RETURN_OK;
}
UINT1 Base_External::set_switch_DPID(UINT8 p_switch_DPID)
{
    switch_DPID=p_switch_DPID;
    return RETURN_OK;
}
UINT1 Base_External::set_switch_port(UINT4 p_switch_port)
{
    switch_port=p_switch_port;
    return RETURN_OK;
}
UINT1 Base_External::set_status(UINT1 p_status)
{
    status=p_status;
    return RETURN_OK;
}

void Base_External::lock_Mutex(void)
{
    pthread_mutex_lock(&BaseMutex);
}
void Base_External::unlock_Mutex(void)
{
    pthread_mutex_unlock(&BaseMutex);
}


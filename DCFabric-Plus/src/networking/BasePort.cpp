#include "BasePort.h"



Base_Port::Base_Port(string p_ID, string p_name, string p_tenant_ID, string p_network_ID, string p_device_ID, string p_device_owner, UINT1 p_status,UINT1 * p_MAC)
{
    ID=p_ID;
    name=p_name;
    tenant_ID=p_tenant_ID;
    network_ID=p_network_ID;
	device_ID=p_device_ID;
    device_owner=p_device_owner;
    status=p_status;
	
    memcpy(MAC,p_MAC,6);

    pthread_mutex_init(&BasePortMutex, NULL);
}
Base_Port::~Base_Port()
{
	list<Fixed_IP*>::iterator itor;
    lock_Mutex();
    itor = Fixed_IPs.begin();
    while (itor != Fixed_IPs.end())
    {
		delete (*itor);
		itor++;
    }
    unlock_Mutex();
    pthread_mutex_destroy(&BasePortMutex);
}

const string& Base_Port::get_ID(void)
{
    return ID;
}
const string& Base_Port::get_name(void)
{
    return name;
}
const string& Base_Port::get_tenant_ID(void)
{
    return tenant_ID;
}
const string& Base_Port::get_device_owner(void)
{
    return device_owner;
}
const string& Base_Port::get_network_ID(void)
{
    return network_ID;
}
const string& Base_Port::get_device_ID(void)
{
	return device_ID;
}
UINT1* Base_Port::get_MAC(void)
{
    return MAC;
}
UINT1 Base_Port::get_status(void)
{
    return status;
}
UINT4 Base_Port::getFirstFixedIp()
{
	UINT4 ret = 0;
	if(NULL != Fixed_IPs.front())
		ret = Fixed_IPs.front()->get_IP();
    return ret;
}
const string Base_Port::getFirstSubnet()
{
	string str="";
	if(NULL != Fixed_IPs.front())
		str = Fixed_IPs.front()->get_subnet_ID();
	return str;
}

bnc::host::host_type Base_Port::getDeviceType()
{
	bnc::host::host_type return_type = bnc::host::HOST_UNKNOWN;

    if ((device_owner == std::string("compute:None"))||(device_owner == std::string("compute:nova")))
    {
        return_type =  bnc::host::HOST_NORMAL;
    }
    else if (device_owner == std::string("network:dhcp"))
    {
        return_type =  bnc::host::HOST_DHCP;
    }
    else if (device_owner == std::string("network:router_interface"))
    {
        return_type = bnc::host::HOST_ROUTER;
    }
    else if (device_owner == std::string("network:floatingip"))
    {
        return_type = bnc::host::HOST_FLOATINGIP;
    }
    else if (device_owner == std::string("network:router_gateway"))
    {
        return_type = bnc::host::HOST_GATEWAY;
    }
	else if(device_owner == std::string("network:LOADBALANCER_HA"))
	{
		return_type = bnc::host::HOST_CLBHA;
	}
	else if(device_owner == std::string("network:LOADBALANCER_VIP"))
	{
		return_type = bnc::host::HOST_CLBVIP;
	}
    else
    {
        return_type = bnc::host::HOST_UNKNOWN;
    }

    return return_type;
}


UINT1 Base_Port::get_Fixed_IPs(list<Fixed_IP*>& p_Fixed_IPs)
{
    list<Fixed_IP*>::iterator itor;

    lock_Mutex();
    itor = Fixed_IPs.begin();
    while (itor != Fixed_IPs.end())
    {
        p_Fixed_IPs.push_front((*itor));
        itor++;
    }
    unlock_Mutex();

    return RETURN_OK;
}

UINT1 Base_Port::set_status(UINT1 p_status)
{
    status = p_status;
    return RETURN_OK;
}
UINT1 Base_Port::add_fixed_IP(UINT4 p_IP,string p_subnet_ID)
{
    Fixed_IP * FixedIP = new Fixed_IP();
    FixedIP->set_IP(p_IP);
    FixedIP->set_subnet_ID(p_subnet_ID);
    lock_Mutex();
    Fixed_IPs.push_front(FixedIP);
    unlock_Mutex();
    return RETURN_TRUE;
}
UINT1 Base_Port::del_fixed_IP(UINT4 p_IP,string p_subnet_ID)
{
    list<Fixed_IP *>::iterator itor;
    Fixed_IP * target_Fixed_IP = NULL;
    lock_Mutex();
    itor = Fixed_IPs.begin();
    while (itor != Fixed_IPs.end())
    {
            if (((*itor)->get_subnet_ID() == p_subnet_ID) && ((*itor)->get_IP() == p_IP))
            {
				target_Fixed_IP = (*itor);
				itor++;
				Fixed_IPs.remove(target_Fixed_IP);
				delete target_Fixed_IP;
            }
			else
			{
				itor++;
			}
    }
    unlock_Mutex();
    return RETURN_TRUE;
}
UINT1 Base_Port::search_IP(UINT4 p_IP)
{
    UINT1 result = RETURN_FALSE;
    list<Fixed_IP*>::iterator itor;

    lock_Mutex();
    itor = Fixed_IPs.begin();
    while (itor != Fixed_IPs.end())
    {
            if (((*itor)->get_IP()) == p_IP)
            {
                    result = RETURN_TRUE;
                    break;
            }
            itor++;
    }
    unlock_Mutex();

    return result;
}
UINT1 Base_Port::search_fixed_IP(UINT4 p_IP,string p_subnet_ID)
{
    UINT1 result = RETURN_FALSE;
    list<Fixed_IP*>::iterator itor;

    lock_Mutex();
    itor = Fixed_IPs.begin();
    while (itor != Fixed_IPs.end())
    {
            if (((*itor)->get_IP()) == p_IP&&((*itor)->get_subnet_ID()) == p_subnet_ID)
            {
                    result = RETURN_TRUE;
                    break;
            }
            itor++;
    }
    unlock_Mutex();

    return result;
}


UINT4 Base_Port::search_IP_by_subnetID(string p_subnet_ID)
{
    UINT1 result = RETURN_FALSE;
    list<Fixed_IP*>::iterator itor;

    lock_Mutex();
    itor = Fixed_IPs.begin();
    while (itor != Fixed_IPs.end())
    {
            if (((*itor)->get_subnet_ID()) == p_subnet_ID)
            {
                    result = RETURN_TRUE;
                    break;
            }
            itor++;
    }
    unlock_Mutex();
    if(result == RETURN_TRUE)
    {
        return (*itor)->get_IP();
    }
    return INADDR_NONE;
}

void Base_Port::lock_Mutex(void)
{
    pthread_mutex_lock(&BasePortMutex);
}
void Base_Port::unlock_Mutex(void)
{
    pthread_mutex_unlock(&BasePortMutex);
}

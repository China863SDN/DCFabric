#include "BaseSubnet.h"

IP_pool::IP_pool(UINT4 p_IP_start, UINT4 p_IP_end)
{
        IP_start = p_IP_start;
        IP_end = p_IP_end;
}

UINT4 IP_pool::get_IP_start(void)
{
        return IP_start;
}

UINT4 IP_pool::get_IP_end(void)
{
        return IP_end;
}

UINT1 IP_pool::check_IPinpool(UINT4 p_IP)
{
        if ((p_IP >= IP_start) && (p_IP <= IP_end))
        {
                return RETURN_TRUE;
        }
        else
        {
                return RETURN_FALSE;
        }
}

Base_Subnet::Base_Subnet(string p_ID, string p_name, string p_tenant_ID, string p_network_ID, string p_cidr, UINT4 p_gateway_IP, UINT4 p_IP_start, UINT4 p_IP_end)
{
	ID = p_ID;
	name = p_name;
	tenant_ID = p_tenant_ID;
	network_ID = p_network_ID;
	cidr = p_cidr;
	gateway_IP = p_gateway_IP;
	
	add_allocation_pool(p_IP_start,p_IP_end);
	pthread_mutex_init(&BaseSubnetMutex, NULL);
}
Base_Subnet::~Base_Subnet()
{
	list<IP_pool *>::iterator itor;
	itor = allocation_pools.begin();
	while (itor != allocation_pools.end())
	{
		delete (*itor);
		itor++;
	}
	pthread_mutex_destroy(&BaseSubnetMutex);
}
const string& Base_Subnet::get_ID(void)
{
        return ID;
}

const string& Base_Subnet::get_name(void)
{
        return name;
}

const string& Base_Subnet::get_tenant_ID(void)
{
        return tenant_ID;
}

const string& Base_Subnet::get_network_ID(void)
{
        return network_ID;
}

const string& Base_Subnet::get_cidr(void)
{
        return cidr;
}

UINT4  Base_Subnet::get_gateway_IP(void)
{
        return gateway_IP;
}

UINT1  Base_Subnet::get_status(void)
{
        return status;
}

UINT4 Base_Subnet::getFirstStartIp()
{
	UINT4 ret = 0;
	if(NULL != allocation_pools.front())
		ret = allocation_pools.front()->get_IP_start();
	return ret;
}
UINT4 Base_Subnet::getFirstEndIp()
{
	UINT4 ret = 0;
	if(NULL != allocation_pools.front())
		ret = allocation_pools.front()->get_IP_end();
	return ret;
}

UINT1  Base_Subnet::get_allocation_pools(list< IP_pool *> & p_allocation_pools)
{
        list<IP_pool *>::iterator itor;

        itor = allocation_pools.begin();
        while (itor != allocation_pools.end())
        {
                p_allocation_pools.push_front(*itor);
                itor++;
        }

        return RETURN_OK;
}

void  Base_Subnet::set_network_id(const string& str_network_id)
{
	network_ID = str_network_id;
}

UINT1 Base_Subnet::change_status(UINT1 p_status)
{
        status = p_status;
        return RETURN_TRUE;
}

UINT1 Base_Subnet::change_cidr(string p_cidr)
{
        cidr = p_cidr;
        return RETURN_TRUE;
}

UINT1 Base_Subnet::change_gateway_IP(UINT4 p_gateway_IP)
{
        gateway_IP = p_gateway_IP;
        return RETURN_TRUE;
}

UINT1 Base_Subnet::add_allocation_pool(UINT4 p_IP_start, UINT4 p_IP_end)
{
        IP_pool * new_IP_pool = new IP_pool(p_IP_start, p_IP_end);
        allocation_pools.push_front(new_IP_pool);
        return RETURN_TRUE;
}

UINT1 Base_Subnet::del_allocation_pool(UINT4 p_IP_start, UINT4 p_IP_end)
{
        list<IP_pool *>::iterator itor;
        IP_pool * new_IP_pool = NULL;
        itor = allocation_pools.begin();
        while (itor != allocation_pools.end())
        {
			if(*itor)
			{
                if (((*itor)->get_IP_start() == p_IP_start) && ((*itor)->get_IP_end() == p_IP_end))
                {
                        new_IP_pool = (*itor);
						itor++;
                        allocation_pools.remove(new_IP_pool);
						delete new_IP_pool;
                }
				else
				{
					itor++;
				}
			}
        }
        return RETURN_TRUE;
}

UINT1 Base_Subnet::check_IPexisted_in_allocation_pools(UINT4 p_IP)
{
        list<IP_pool *>::iterator itor;
        UINT1 check_result = RETURN_FALSE;
        itor = allocation_pools.begin();
        while (itor != allocation_pools.end())
        {
                if (RETURN_TRUE == (*itor)->check_IPinpool(p_IP))
                {
                        check_result = RETURN_TRUE;
                        break;
                }
                itor++;
        }
        return check_result;
}


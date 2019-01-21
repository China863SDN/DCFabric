#include "CNotifyMgr.h"
#include "BaseSubnetManager.h"

BaseSubnetMgr G_SubnetMgr;

BaseSubnetMgr::BaseSubnetMgr()
{
        pthread_mutex_init(&BaseSubnetMgrMutex, NULL);
}

BaseSubnetMgr::~BaseSubnetMgr()
{
        pthread_mutex_destroy(&BaseSubnetMgrMutex);
}

Base_Subnet * BaseSubnetMgr::targetSubnet_ByID(const string p_ID)
{
        Base_Subnet * target = NULL;
        list<Base_Subnet *>::iterator itor;
        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
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

UINT1 BaseSubnetMgr::targetSubnet_Byname(const string     p_name,list<Base_Subnet*>&p_subnets)
{
        list<Base_Subnet *>::iterator itor;
        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
        {
                if ((*itor)->get_name() == p_name)
                {
                        p_subnets.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseSubnetMgr::targetSubnet_Bytenant_ID(const string        p_tenant_ID,list<Base_Subnet*>&p_subnets)
{
        list<Base_Subnet *>::iterator itor;
        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
        {
                if ((*itor)->get_tenant_ID() == p_tenant_ID)
                {
                        p_subnets.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseSubnetMgr::targetSubnet_Bynetwork_ID(const string       p_network_ID,list<Base_Subnet*>&p_subnets)
{
        list<Base_Subnet *>::iterator itor;
        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
        {
                if ((*itor)->get_network_ID() == p_network_ID)
                {
                        p_subnets.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseSubnetMgr::targetSubnet_Bystatus(const UINT1    p_status,list<Base_Subnet*>&p_subnets)
{
        list<Base_Subnet *>::iterator itor;
        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
        {
                if ((*itor)->get_status() == p_status)
                {
                        p_subnets.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseSubnetMgr::targetSubnet_Bygateway_IP(const UINT4        p_gateway_IP,list<Base_Subnet*>&p_subnets)
{
        list<Base_Subnet *>::iterator itor;
        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
        {
                if ((*itor)->get_gateway_IP() == p_gateway_IP)
                {
                        p_subnets.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseSubnetMgr::targetSubnet_Bycidr(const string     p_cidr,list<Base_Subnet*>&p_subnets)
{
        list<Base_Subnet *>::iterator itor;
        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
        {
                if ((*itor)->get_cidr() == p_cidr)
                {
                        p_subnets.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseSubnetMgr::defineSubnet_ByIP	(const UINT4   p_IP,string& p_SubnetID)
{
	UINT1 matchedSubnetCount	=0;
	UINT1 matchedSubnetResult   =RETURN_FALSE;
	list<Base_Subnet *>::iterator itor;
	lock_Mutex();
	itor = subnets.begin();
	while (itor != subnets.end())
	{
		if((*itor)->check_IPexisted_in_allocation_pools(p_IP))
		{
			p_SubnetID=(*itor)->get_ID();
			matchedSubnetCount++;
		}
		itor++;
	}
	unlock_Mutex();
	if(matchedSubnetCount == 1)
	{
		matchedSubnetResult=RETURN_TRUE;
	}
	else
	{
		matchedSubnetResult=RETURN_FALSE;
	}
	return matchedSubnetResult;
}

UINT1 BaseSubnetMgr::listSubnets   (list<Base_Subnet*>&p_subnets)
{
        list<Base_Subnet *>::iterator itor;
        lock_Mutex();
        itor = subnets.begin();
        while (itor != subnets.end())
        {
                p_subnets.push_front(*itor);
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseSubnetMgr::insertNode_BySubnet(Base_Subnet * p_subnet)
{
		CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyAddSubnet(p_subnet);
        lock_Mutex();
        subnets.push_front(p_subnet);
        unlock_Mutex();
		
        return RETURN_OK;
}

UINT1 BaseSubnetMgr::deleteNode_BySubnet(Base_Subnet * p_subnet)
{
    if(p_subnet)
    {
		CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelSubnet(p_subnet->get_ID());
        lock_Mutex();
        subnets.remove(p_subnet);
        unlock_Mutex();
		
        delete p_subnet;
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }
}

UINT1 BaseSubnetMgr::deleteNode_BySubnetID(const string p_ID)
{
        Base_Subnet * p_subnet = targetSubnet_ByID(p_ID);
        if (p_subnet)
        {
                lock_Mutex();
                subnets.remove(p_subnet);
                unlock_Mutex();
				
				CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelSubnet(p_ID);
                delete p_subnet;
        }
        return RETURN_OK;
}


void BaseSubnetMgr::lock_Mutex(void)
{
        pthread_mutex_lock(&BaseSubnetMgrMutex);
}

void BaseSubnetMgr::unlock_Mutex(void)
{
        pthread_mutex_unlock(&BaseSubnetMgrMutex);
}

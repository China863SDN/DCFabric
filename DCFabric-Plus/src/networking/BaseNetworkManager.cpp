#include "CNotifyMgr.h"
#include "BaseNetworkManager.h"

BaseNetworkMgr G_NetworkMgr;


BaseNetworkMgr::BaseNetworkMgr()
{
        pthread_mutex_init(&BaseNetworkMgrMutex, NULL);
}

BaseNetworkMgr::~BaseNetworkMgr()
{
        pthread_mutex_destroy(&BaseNetworkMgrMutex);
}

Base_Network * BaseNetworkMgr::targetNetwork_ByID(const string p_ID)
{
        Base_Network * target = NULL;
        list<Base_Network *>::iterator itor;
        lock_Mutex();
        itor = networks.begin();
        while (itor != networks.end())
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


UINT1 BaseNetworkMgr::targetNetwork_Byname(const string   p_name,list<Base_Network *> & p_networks)
{
        list<Base_Network *>::iterator itor;
        lock_Mutex();
        itor = networks.begin();
        while (itor != networks.end())
        {
                if ((*itor)->get_name() == p_name)
                {
                        p_networks.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

Base_Network* BaseNetworkMgr::targetNetwork_Bysubnet_ID(const string&   p_subnet_ID)
{
        list<Base_Network *>::iterator itor;
        lock_Mutex();
        itor = networks.begin();
        while (itor != networks.end())
        {
                if (RETURN_TRUE == (*itor)->search_subnet(p_subnet_ID))
                {
                      unlock_Mutex();
					  return *itor;
                }
                itor++;
        }
        unlock_Mutex();
        return NULL;
}

UINT1 BaseNetworkMgr::targetNetwork_Bytenant_ID(const string   p_tenant_ID,list<Base_Network *> & p_networks)
{
        list<Base_Network *>::iterator itor;
        lock_Mutex();
        itor = networks.begin();
        while (itor != networks.end())
        {
                if ((*itor)->get_tenant_ID() == p_tenant_ID)
                {
                        p_networks.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseNetworkMgr::targetNetwork_Bystatus(const UINT1    p_status,list<Base_Network *> & p_networks)
{
        list<Base_Network *>::iterator itor;
        lock_Mutex();
        itor = networks.begin();
        while (itor != networks.end())
        {
                if ((*itor)->get_status() == p_status)
                {
                        p_networks.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseNetworkMgr::targetNetwork_Byrouter_external(const UINT1    p_router_external,list<Base_Network *> & p_networks)
{
        list<Base_Network *>::iterator itor;
        lock_Mutex();
        itor = networks.begin();
        while (itor != networks.end())
        {
                if ((*itor)->get_router_external() == p_router_external)
                {
                        p_networks.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseNetworkMgr::targetNetwork_Byshared(const UINT1    p_shared,list<Base_Network *> & p_networks)
{
        list<Base_Network *>::iterator itor;
        lock_Mutex();
        itor = networks.begin();
        while (itor != networks.end())
        {
                if ((*itor)->get_shared() == p_shared)
                {
                        p_networks.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseNetworkMgr::listNetworks(list<Base_Network *> & p_networks)
{
        list<Base_Network *>::iterator itor;
        lock_Mutex();
        itor = networks.begin();
        while (itor != networks.end())
        {
                p_networks.push_front(*itor);
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;

}

UINT1 BaseNetworkMgr::insertNode_ByNetwork(Base_Network * p_network)
{
        lock_Mutex();
        networks.push_front(p_network);
        unlock_Mutex();
		
		CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyAddNetwork(p_network);
        return RETURN_OK;
}

UINT1 BaseNetworkMgr::deleteNode_ByNetwork(Base_Network * p_network)
{
    if(p_network)
    {
		CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelNetwork(p_network->get_ID());
        lock_Mutex();
        networks.remove(p_network);
        unlock_Mutex();
		
        delete p_network;
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }

}

UINT1 BaseNetworkMgr::deleteNode_ByNetworkID(const string p_ID)
{
        Base_Network * p_network = targetNetwork_ByID(p_ID);
        if (p_network)
        {
				CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelNetwork(p_ID);
                lock_Mutex();
                networks.remove(p_network);
                unlock_Mutex();
				
                delete p_network;
        }
        return RETURN_OK;
}


void BaseNetworkMgr::lock_Mutex(void)
{
        pthread_mutex_lock(&BaseNetworkMgrMutex);
}

void BaseNetworkMgr::unlock_Mutex(void)
{
        pthread_mutex_unlock(&BaseNetworkMgrMutex);
}


#include "CNotifyMgr.h"
#include "BaseFloatingManager.h"

BaseFloatingMgr G_FloatingMgr;

BaseFloatingMgr::BaseFloatingMgr()
{
        pthread_mutex_init(&BaseFloatingMgrMutex, NULL);
}

BaseFloatingMgr::~BaseFloatingMgr()
{
        pthread_mutex_destroy(&BaseFloatingMgrMutex);
}

Base_Floating * BaseFloatingMgr::targetFloating_ByID(const string p_ID)
{
        Base_Floating * target = NULL;
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
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


UINT1 BaseFloatingMgr::targetFloating_Byname(const string       p_name,list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {
                if ((*itor)->get_name() == p_name)
                {
                        p_floatings.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseFloatingMgr::targetFloating_Bytenant_ID(const string  p_tenant_ID,list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {
                if ((*itor)->get_tenant_ID() == p_tenant_ID)
                {
                        p_floatings.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseFloatingMgr::targetFloating_Bynetwork_ID(const string         p_network_ID,list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {
                if ((*itor)->get_network_ID() == p_network_ID)
                {
                        p_floatings.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseFloatingMgr::targetFloating_Byrouter_ID(const string  p_router_ID,list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {
                if ((*itor)->get_router_ID() == p_router_ID)
                {
                        p_floatings.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseFloatingMgr::targetFloating_Byport_ID(const string    p_port_ID,list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {
                if ((*itor)->get_port_ID() == p_port_ID)
                {
                        p_floatings.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseFloatingMgr::targetFloating_Bystatus(const UINT1      p_status,list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {
                if ((*itor)->get_status() == p_status)
                {
                        p_floatings.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseFloatingMgr::targetFloating_Byfixed_IP(const UINT4    p_fixed_IP,list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {
                if ((*itor)->get_fixed_IP() == p_fixed_IP)
                {
                        p_floatings.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}

UINT1 BaseFloatingMgr::targetFloating_Byfloating_IP(const UINT4         p_floating_IP,list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {
                if ((*itor)->get_floating_IP() == p_floating_IP)
                {
                        p_floatings.push_front(*itor);
                }
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}


UINT1 BaseFloatingMgr::listFloatings(list<Base_Floating *> &p_floatings)
{
        list<Base_Floating *>::iterator itor;
        lock_Mutex();
        itor = floatings.begin();
        while (itor != floatings.end())
        {

                p_floatings.push_front(*itor);
                itor++;
        }
        unlock_Mutex();
        return RETURN_OK;
}


UINT1 BaseFloatingMgr::insertNode_ByFloating(Base_Floating * p_Floating)
{
        lock_Mutex();
        floatings.push_front(p_Floating);
        unlock_Mutex();
		
		CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyAddFloatingIp(p_Floating);
        return RETURN_OK;
}

UINT1 BaseFloatingMgr::deleteNode_ByFloating(Base_Floating * p_Floating)
{
    if(p_Floating)
    {
    	CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelFloatingIp(p_Floating->get_ID());
        lock_Mutex();
        floatings.remove(p_Floating);
        unlock_Mutex();
		
        delete p_Floating;
        return RETURN_OK;
    }
    else
    {
        return  RETURN_ERR;
    }
}

UINT1 BaseFloatingMgr::deleteNode_ByFloatingID(const string p_ID)
{
        Base_Floating * p_Floating = targetFloating_ByID(p_ID);
        if (p_Floating)
        {
        		CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelFloatingIp(p_ID);
                lock_Mutex();
                floatings.remove(p_Floating);
                unlock_Mutex();
                delete p_Floating;
        }
        return RETURN_OK;
}

void BaseFloatingMgr::lock_Mutex(void)
{
        pthread_mutex_lock(&BaseFloatingMgrMutex);
}

void BaseFloatingMgr::unlock_Mutex(void)
{
        pthread_mutex_unlock(&BaseFloatingMgrMutex);
}




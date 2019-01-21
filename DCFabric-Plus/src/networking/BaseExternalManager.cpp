#include "log.h"
#include "bnc-error.h"
#include "BaseExternalManager.h"
#include "CRouter.h"
#include "CRouterGateMgr.h"

BaseExternalMgr G_ExternalMgr ;

BaseExternalMgr::BaseExternalMgr()
{
    pthread_mutex_init(&BaseMgrMutex, NULL);
}

BaseExternalMgr::~BaseExternalMgr()
{
    pthread_mutex_destroy(&BaseMgrMutex);
}

Base_External * BaseExternalMgr::targetExternal_Bysubnet_ID(const string p_subnet_ID)
{
    Base_External * target = NULL;
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
            if ((*itor)->get_subnet_ID() == p_subnet_ID)
            {
                    target = *itor;
                    break;
            }
            itor++;
    }
    unlock_Mutex();
    return target;
}

UINT1 BaseExternalMgr::targetExternal_Bynetwork_ID(const string p_network_ID,list<Base_External *>& p_externals)
{
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
        if ((*itor)->get_network_ID() == p_network_ID)
        {
            p_externals.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}
UINT1 BaseExternalMgr::targetExternal_Bygateway_IP(UINT4 p_gateway_IP,list<Base_External *>& p_externals)
{
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
        if ((*itor)->get_gateway_IP() == p_gateway_IP)
        {
            p_externals.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}
UINT1 BaseExternalMgr::targetExternal_Byswitch(UINT8 p_switch_DPID,list<Base_External *>& p_externals)
{
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
        if ((*itor)->get_switch_DPID() == p_switch_DPID)
        {
            p_externals.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}
UINT1 BaseExternalMgr::targetExternal_Byswitch_port(UINT8 p_switch_DPID,UINT4 p_switch_port,list<Base_External *>& p_externals)
{
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
        if (((*itor)->get_switch_DPID() == p_switch_DPID)&&((*itor)->get_switch_port() == p_switch_port))
        {
            p_externals.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}
UINT1 BaseExternalMgr::targetExternal_Bystatus(UINT1 p_status,list<Base_External *>& p_externals)
{
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
        if ((*itor)->get_status() == p_status)
        {
            p_externals.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}

UINT1 BaseExternalMgr::updateExternal_Bygateway_IP(UINT4 p_gateway_IP,UINT1 * p_gateway_MAC,UINT8 p_switch_DPID,UINT4 p_switch_port, BOOL *p_match)
{
	list<Base_External *>::iterator itor;
	*p_match = FALSE;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
        if((*itor)->get_gateway_IP()==p_gateway_IP)
		{
			if((memcmp((*itor)->get_gateway_MAC(),p_gateway_MAC,6)==0)
			   &&((*itor)->get_switch_DPID()==p_switch_DPID)
			   &&((*itor)->get_switch_port()==p_switch_port)
			  )
			{
				//all matched , do nothing
				*p_match = TRUE;
			}
			else
			{
				(*itor)->set_gateway_MAC(p_gateway_MAC);
				(*itor)->set_switch_DPID(p_switch_DPID);
				(*itor)->set_switch_port(p_switch_port);
				//TBD here need do something with related flow
			}
		}
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}

UINT1 BaseExternalMgr::listExternal(list<Base_External *>& p_externals)
{
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
        p_externals.push_front(*itor);
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}

UINT1 BaseExternalMgr::insertNode_ByExternal(Base_External * p_external)
{
    if(p_external)
    {
        lock_Mutex();
        externals.push_front(p_external);
        unlock_Mutex();
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }
}
UINT1 BaseExternalMgr::deleteNode_ByExternal(Base_External * p_external)
{
    if(p_external)
    {
        lock_Mutex();
        externals.remove(p_external);
        unlock_Mutex();
        delete p_external;
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }
}
UINT1 BaseExternalMgr::deleteNode_Bysubnet_ID(const string p_subnet_ID)
{
    Base_External * p_external = targetExternal_Bysubnet_ID(p_subnet_ID);
    if (p_external)
    {
            lock_Mutex();
            externals.remove(p_external);
            unlock_Mutex();
            delete p_external;
    }
    return RETURN_OK;
}
UINT1 BaseExternalMgr::deleteNode_Bynetwork_ID(const string p_network_ID)
{
	Base_External * Target_ExternalInfo =NULL;
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
        if ((*itor)->get_network_ID() == p_network_ID)
        {
			Target_ExternalInfo=*itor;
			itor++;
            externals.remove(Target_ExternalInfo);
            delete(Target_ExternalInfo);
        }
		else
		{
			itor++;
		}
    }
    unlock_Mutex();
    return RETURN_OK;
}

BOOL  BaseExternalMgr::checkSwitch_isExternal(UINT8 switch_dpid)
{
    list<Base_External *>::iterator itor;
    lock_Mutex();
    itor = externals.begin();
    while (itor != externals.end())
    {
            if (switch_dpid&&(NULL != *itor)&&((*itor)->get_switch_DPID() == switch_dpid))
            {
            	 unlock_Mutex();
                 return TRUE;
            }
            itor++;
    }
    unlock_Mutex();
    return FALSE;
}


Base_External * BaseExternalMgr::getExternalPortByInternalIp(UINT4 internal_ip)
{
	std::list<Base_External *>	baseExternalPortList;
	CRouter* router = CRouterGateMgr::getInstance()->FindRouterNodeByHostIp(internal_ip);
	if(NULL == router)
	{
		LOG_ERROR(" Can't get router!!!");
		return NULL;
	}
	G_ExternalMgr.listExternal(baseExternalPortList);
	if(!baseExternalPortList.empty())
	{
		STL_FOR_LOOP(baseExternalPortList, iter)
		{				
			if(*iter&&((*iter)->get_subnet_ID() == router->getSubnetid()))
			{
				return *iter;
			}
		}
	}
	return NULL;	
}
Base_External * BaseExternalMgr::getExternalPortByInternalMac(UINT1* internal_mac)
{
	std::list<Base_External *>	baseExternalPortList;
	CRouter* router = CRouterGateMgr::getInstance()->FindRouterNodeByHostMac(internal_mac);
	if(NULL == router)
	{
		LOG_ERROR(" Can't get router!!!");
		return NULL;
	}
	G_ExternalMgr.listExternal(baseExternalPortList);
	if(!baseExternalPortList.empty())
	{
		STL_FOR_LOOP(baseExternalPortList, iter)
		{				
			if(*iter&&((*iter)->get_subnet_ID() == router->getSubnetid()))
			{
				return *iter;
			}
		}
	}
	return NULL;	
}


void BaseExternalMgr::lock_Mutex(void)
{
    pthread_mutex_lock(&BaseMgrMutex);
}
void BaseExternalMgr::unlock_Mutex(void)
{
    pthread_mutex_unlock(&BaseMgrMutex);
}


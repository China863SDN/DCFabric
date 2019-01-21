#include "CNotifyMgr.h"
#include "BasePortManager.h"

BasePortMgr G_PortMgr;


BasePortMgr::BasePortMgr()
{
    pthread_mutex_init(&BasePortMgrMutex, NULL);
}
BasePortMgr::~BasePortMgr()
{
    pthread_mutex_destroy(&BasePortMgrMutex);
}

Base_Port * BasePortMgr::targetPort_ByID(const string p_ID)
{
    Base_Port * target = NULL;
    list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
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
Base_Port * BasePortMgr::targetPort_ByMAC(const UINT1* p_MAC)
{
    Base_Port * target = NULL;
    list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
        if (memcmp((*itor)->get_MAC(),p_MAC,6)==0)
        {
            target = *itor;
            break;
        }
        itor++;
    }
    unlock_Mutex();
    return target;
}
Base_Port * BasePortMgr::targetPort_ByFixed_IP(UINT4 p_IP,string p_subnet_ID)
{
    Base_Port * target = NULL;
    list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
        if ((*itor)->search_fixed_IP(p_IP,p_subnet_ID) == RETURN_TRUE)
        {
            target = *itor;
            break;
        }
        itor++;
    }
    unlock_Mutex();
    return target;
}
UINT1 BasePortMgr::targetPort_ByIP(UINT4 p_IP,list<Base_Port *> & p_ports)
{
    list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
        if ((*itor)->search_IP(p_IP) == RETURN_TRUE)
        {
            p_ports.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}

UINT1 BasePortMgr::targetPort_Bytenant_ID(const string   p_tenant_ID,list<Base_Port *> & p_ports)
{
    list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
            if ((*itor)->get_tenant_ID() == p_tenant_ID)
            {
                    p_ports.push_front(*itor);
            }
            itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}
UINT1 BasePortMgr::targetPort_Bynetwork_ID(const string   p_network_ID,list<Base_Port *> & p_ports)
{
    list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
            if ((*itor)->get_network_ID() == p_network_ID)
            {
                    p_ports.push_front(*itor);
            }
            itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}
UINT1 BasePortMgr::targetPort_Bydevice_owner(const string   p_device_owner,list<Base_Port *> & p_ports)
{
    list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
        if ((*itor)->get_device_owner() == p_device_owner)
        {
                p_ports.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}
UINT1 BasePortMgr::targetPort_Bydevice_ID(const string   p_device_ID,list<Base_Port *> & p_ports)
{
	list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
        if ((*itor)->get_device_ID() == p_device_ID)
        {
			p_ports.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}
UINT1 BasePortMgr::targetPort_Bydevice_ID_owner(const string   p_device_ID,const string   p_device_owner,list<Base_Port *> & p_ports)
{
	list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
        if (((*itor)->get_device_ID() == p_device_ID)&&((*itor)->get_device_owner() == p_device_owner))
        {
			p_ports.push_front(*itor);
        }
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}

UINT1 BasePortMgr::targetGatewayPort_Byport(Base_Port&  p_srcPort,list<Base_Port *> & p_GatewayPorts)
{
	UINT1 targetGatewayPort_count =0;
	list<Base_Port *>::iterator itor;
	string Device_routerInterface =DO_routerInterface;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
        if (((*itor)->get_device_owner() == Device_routerInterface)&&((*itor)->get_network_ID() ==p_srcPort.get_network_ID() ))
        {
			p_GatewayPorts.push_front(*itor);
			targetGatewayPort_count++;
        }
        itor++;
    }
    unlock_Mutex();
    return targetGatewayPort_count;
}

UINT1 BasePortMgr::listPorts(list<Base_Port *> & p_ports)
{
    list<Base_Port *>::iterator itor;
    lock_Mutex();
    itor = ports.begin();
    while (itor != ports.end())
    {
        p_ports.push_front(*itor);
        itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}

UINT1 BasePortMgr::insertNode_ByPort(Base_Port * p_port)
{
    if(p_port)
    {
        lock_Mutex();
        ports.push_front(p_port);
        unlock_Mutex();
		CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyAddPort(p_port);
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }

}
UINT1 BasePortMgr::deleteNode_ByPort(Base_Port * p_port)
{
    if(p_port)
    {
    	
		CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelPort(p_port->get_ID());
        lock_Mutex();
        ports.remove(p_port);
        unlock_Mutex();
		
        delete p_port;
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }
}
UINT1 BasePortMgr::deleteNode_ByPort(const string p_ID)
{
    Base_Port * p_port = targetPort_ByID(p_ID);
    if (p_port)
    {
    		
			CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelPort(p_port->get_ID());
            lock_Mutex();
            ports.remove(p_port);
            unlock_Mutex();
			
            delete p_port;
			
    }
    return RETURN_OK;
}

UINT1 BasePortMgr::deleteNode_ByMac(const UINT1* p_Mac)
{
    Base_Port * p_port = targetPort_ByMAC(p_Mac);
    if (p_port)
    {
    		
			CNotifyMgr::getInstance()->getNotifyBaseNetworking()->notifyDelPort(p_port->get_ID());
            lock_Mutex();
            ports.remove(p_port);
            unlock_Mutex();
			
            delete p_port;
			
    }
    return RETURN_OK;
}

void BasePortMgr::lock_Mutex(void)
{
        pthread_mutex_lock(&BasePortMgrMutex);
}

void BasePortMgr::unlock_Mutex(void)
{
        pthread_mutex_unlock(&BasePortMgrMutex);
}

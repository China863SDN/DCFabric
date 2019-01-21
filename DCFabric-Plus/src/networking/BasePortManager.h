#ifndef BASEPORTMANAGER_H
#define BASEPORTMANAGER_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>
#include "BasePort.h"

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0




class BasePortMgr
{
public:
        BasePortMgr();
        ~BasePortMgr();

        Base_Port * targetPort_ByID(const string p_ID);
        Base_Port * targetPort_ByMAC(const UINT1* p_MAC);
        Base_Port * targetPort_ByFixed_IP(UINT4 p_IP,string p_subnet_ID);

        UINT1 targetPort_ByIP(UINT4 p_IP,list<Base_Port *> & p_ports);
        UINT1 targetPort_Bytenant_ID(const string   p_tenant_ID,list<Base_Port *> & p_ports);
        UINT1 targetPort_Bynetwork_ID(const string   p_network_ID,list<Base_Port *> & p_ports);
		UINT1 targetPort_Bydevice_ID(const string   p_device_ID,list<Base_Port *> & p_ports);
        UINT1 targetPort_Bydevice_owner(const string   p_device_owner,list<Base_Port *> & p_ports);
		UINT1 targetPort_Bydevice_ID_owner(const string   p_device_ID,const string   p_device_owner,list<Base_Port *> & p_ports);
		UINT1 targetGatewayPort_Byport(Base_Port&  p_srcPort,list<Base_Port *> & p_GatewayPorts);

        UINT1 listPorts(list<Base_Port *> & p_ports);
		
        UINT1 insertNode_ByPort(Base_Port * p_port);
        UINT1 deleteNode_ByPort(Base_Port * p_port);
        UINT1 deleteNode_ByPort(const string p_ID);
		UINT1 deleteNode_ByMac(const UINT1* p_Mac);

private:
        void lock_Mutex(void);
        void unlock_Mutex(void);

private:
        pthread_mutex_t BasePortMgrMutex;
        list<Base_Port *> ports;
};


extern BasePortMgr G_PortMgr;
#endif // BASEPORTMANAGER_H

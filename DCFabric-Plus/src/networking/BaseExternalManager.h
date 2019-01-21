#ifndef BASEEXTERNALMGR_H
#define BASEEXTERNALMGR_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>
#include"BaseExternal.h"

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0



class BaseExternalMgr
{
    public:
        BaseExternalMgr();
        ~BaseExternalMgr();

        Base_External * targetExternal_Bysubnet_ID(const string p_subnet_ID);

        UINT1 targetExternal_Bynetwork_ID(const string p_network_ID,list<Base_External *>& p_externals);
        UINT1 targetExternal_Bygateway_IP(UINT4 p_gateway_IP,list<Base_External *>& p_externals);
        UINT1 targetExternal_Byswitch(UINT8 p_switch_DPID,list<Base_External *>& p_externals);
        UINT1 targetExternal_Byswitch_port(UINT8 p_switch_DPID,UINT4 p_switch_port,list<Base_External *>& p_externals);
        UINT1 targetExternal_Bystatus(UINT1 p_status,list<Base_External *>& p_externals);

		UINT1 updateExternal_Bygateway_IP(UINT4 p_gateway_IP,UINT1 * p_gateway_MAC,UINT8 p_switch_DPID,UINT4 p_switch_port, BOOL *p_match);
		
        UINT1 listExternal(list<Base_External *>& p_externals);

        UINT1 insertNode_ByExternal(Base_External * p_external);
        UINT1 deleteNode_ByExternal(Base_External * p_external);
        UINT1 deleteNode_Bysubnet_ID(const string p_subnet_ID);
        UINT1 deleteNode_Bynetwork_ID(const string p_network_ID);
		BOOL  checkSwitch_isExternal(UINT8 switch_dpid);
		Base_External* getExternalPortByInternalIp(UINT4 internal_ip);
		Base_External* getExternalPortByInternalMac(UINT1* internal_mac);
		std::list<Base_External *>& getexternalListHead() {return externals;}
    private:
        void lock_Mutex(void);
        void unlock_Mutex(void);
    private:
        pthread_mutex_t BaseMgrMutex;
        list<Base_External *> externals;
};
extern BaseExternalMgr G_ExternalMgr;
#endif // BASEEXTERNALMGR_H

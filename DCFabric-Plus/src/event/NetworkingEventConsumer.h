#ifndef NETWORKINGEVENTCONSUMER_H
#define NETWORKINGEVENTCONSUMER_H

#define THREAD_COUNT_ONE    1

#include "bnc-error.h"
#include "CEventNotifier.h"
#include "NetworkingEvent.h"
#include "BaseFloatingManager.h"
#include "BaseNetworkManager.h"
#include "BaseRouterManager.h"
#include "BaseSubnetManager.h"
#include "BasePortManager.h"
#include "BaseExternalManager.h"
#include "NetworkingPersistence.h"

using namespace std;

class NetworkingEventConsumer : public CEventNotifier
{
    public:
        NetworkingEventConsumer();
        NetworkingEventConsumer(BaseFloatingMgr* p_FloatingMgr,BaseNetworkMgr* p_NetworkMgr,BaseRouterMgr* p_RouterMgr,BaseSubnetMgr* p_SubnetMgr,BasePortMgr* 	p_PortMgr,BaseExternalMgr* p_ExternalMgr);
        ~NetworkingEventConsumer();
        
        INT4 onregister(void);
        void deregister(void);

        const char* toString(void) {return "NetworkingEventConsumer";}

    private:
		INT4 consume(CSmartPtr<CMsgCommon> evt);

        UINT1 floating_C(NetworkingEvent &p_event);
        UINT1 floating_R(NetworkingEvent &p_event);
        UINT1 floating_U(NetworkingEvent &p_event);
        UINT1 floating_D(NetworkingEvent &p_event);
        UINT1 floating_DS(NetworkingEvent &p_event);

        UINT1 network_C(NetworkingEvent &p_event);
        UINT1 network_R(NetworkingEvent &p_event);
        UINT1 network_U(NetworkingEvent &p_event);
        UINT1 network_D(NetworkingEvent &p_event);
        UINT1 network_DS(NetworkingEvent &p_event);

        UINT1 router_C(NetworkingEvent &p_event);
        UINT1 router_R(NetworkingEvent &p_event);
        UINT1 router_U(NetworkingEvent &p_event);
        UINT1 router_D(NetworkingEvent &p_event);
        UINT1 router_DS(NetworkingEvent &p_event);

        UINT1 subnet_C(NetworkingEvent &p_event);
        UINT1 subnet_R(NetworkingEvent &p_event);
        UINT1 subnet_U(NetworkingEvent &p_event);
        UINT1 subnet_D(NetworkingEvent &p_event);
        UINT1 subnet_DS(NetworkingEvent &p_event);
		
		UINT1 port_C(NetworkingEvent &p_event);
        UINT1 port_R(NetworkingEvent &p_event);
        UINT1 port_U(NetworkingEvent &p_event);
        UINT1 port_D(NetworkingEvent &p_event);
        UINT1 port_DS(NetworkingEvent &p_event);
		
        UINT1 network_U_ExternalRelatedOperation(Base_Network * UpdatedNetwork);
        UINT1 network_D_ExternalRelatedOperation(Base_Network * DeletedNetwork);
        UINT1 network_DS_ExternalRelatedOperation(NetworkingEvent &p_event);//TBD
		UINT1 subnet_C_ExternalRelatedOperation(Base_Subnet * CreatedSubnet);
        UINT1 subnet_U_ExternalRelatedOperation(Base_Subnet * UpdatedSubnet);
        UINT1 subnet_D_ExternalRelatedOperation(Base_Subnet * DeletedSubnet);
        UINT1 subnet_DS_ExternalRelatedOperation(NetworkingEvent &p_event);

    private:
        BaseFloatingMgr* 	FloatingMgr;
        BaseNetworkMgr* 	NetworkMgr;
        BaseRouterMgr* 		RouterMgr;
        BaseSubnetMgr* 		SubnetMgr;
		BasePortMgr* 		PortMgr;
		BaseExternalMgr*    ExternalMgr;
};

#endif // NETWORKINGEVENTCONSUMER_H

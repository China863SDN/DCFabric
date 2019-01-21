#ifndef NETWORKINGEVENTREPORTER_H
#define NETWORKINGEVENTREPORTER_H

#include "../networking/BaseSubnetManager.h"
#include "../networking/BaseFloatingManager.h"
#include "../networking/BaseNetworkManager.h"
#include "../networking/BaseRouterManager.h"
#include "../networking/BasePortManager.h"

#include "NetworkingEvent.h"
#include "CEventReportor.h"
#include "bnc-error.h"


#define PERSTANCE_FALSE	0
#define PERSTANCE_TRUE	1

class NetworkingEventReporter : public CEventReportor
{
public:
    NetworkingEventReporter() {}
    ~NetworkingEventReporter() {}
    
    INT4 report_C_Floating(Base_Floating * p_FloatingRelated,UINT1 p_PerstanceFlag);
    INT4 report_R_Floating(void);
    INT4 report_U_Floating(Base_Floating * p_FloatingRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Floating(Base_Floating * p_FloatingRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Floatings(void);

    INT4 report_C_Network(Base_Network  * p_NetworkRelated,UINT1 p_PerstanceFlag);
    INT4 report_R_Network(void);
    INT4 report_U_Network(Base_Network  * p_NetworkRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Network(Base_Network  * p_NetworkRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Networks(void);

    INT4 report_C_Router(Base_Router   * p_RouterRelated,UINT1 p_PerstanceFlag);
    INT4 report_R_Router(void);
    INT4 report_U_Router(Base_Router   * p_RouterRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Router(Base_Router   * p_RouterRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Routers(void);

    INT4 report_C_Subnet(Base_Subnet   * p_SubnetRelated,UINT1 p_PerstanceFlag);
    INT4 report_R_Subnet(void);
    INT4 report_U_Subnet(Base_Subnet   * p_SubnetRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Subnet(Base_Subnet   * p_SubnetRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Subnets(void);
	
	INT4 report_C_Port(Base_Port   * p_PortRelated,UINT1 p_PerstanceFlag);
    INT4 report_R_Port(void);
    INT4 report_U_Port(Base_Port   * p_PortRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Port(Base_Port   * p_PortRelated,UINT1 p_PerstanceFlag);
    INT4 report_D_Ports(void);

    const char* toString() {return "NetworkingEventReporter";}

private:
    CMsgPath getMsgPath(INT4 event);
};


extern NetworkingEventReporter G_NetworkingEventReporter;
#endif // NETWORKINGEVENTREPORTER_H

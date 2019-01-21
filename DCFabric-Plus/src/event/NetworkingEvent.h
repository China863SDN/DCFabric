#ifndef NETWORKINGEVENT_H
#define NETWORKINGEVENT_H

#include "../networking/BaseSubnetManager.h"
#include "../networking/BaseFloatingManager.h"
#include "../networking/BaseNetworkManager.h"
#include "../networking/BaseRouterManager.h"
#include "../networking/BasePortManager.h"
#include "CEvent.h"

//======================================================================
//by:yhy networking事件
//======================================================================
class NetworkingEvent : public CEvent
{
    public:
        NetworkingEvent();
        NetworkingEvent(INT4 event, INT4 reason);
        ~NetworkingEvent();

		UINT1 get_Persistence(void);
        Base_Floating * get_FloatingRelated(void);
        Base_Network  * get_NetworkRelated(void);
        Base_Router   * get_RouterRelated(void);
        Base_Subnet   * get_SubnetRelated(void);
		Base_Port     * get_PortRelated(void);

		void set_Persistence(UINT1 p_PersistenceFlag);
        UINT4 set_FloatingRelated(Base_Floating * p_FloatingRelated);
        UINT4 set_NetworkRelated(Base_Network  * p_NetworkRelated);
        UINT4 set_RouterRelated(Base_Router   * p_RouterRelated);
        UINT4 set_SubnetRelated(Base_Subnet   * p_SubnetRelated);
		UINT4 set_PortRelated(Base_Port   * p_PortRelated);

    private:
        Base_Floating * FloatingRelated;
        Base_Network  * NetworkRelated;
        Base_Router   * RouterRelated;
        Base_Subnet   * SubnetRelated;
		Base_Port     * PortRelated;
		UINT1 PerstanceFlag;
};

#endif // NETWORKINGEVENT_H

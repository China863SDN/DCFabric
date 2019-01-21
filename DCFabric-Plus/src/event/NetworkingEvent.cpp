#include "NetworkingEvent.h"

NetworkingEvent::NetworkingEvent():
    FloatingRelated(NULL),
    NetworkRelated(NULL),
    RouterRelated(NULL),
    SubnetRelated(NULL),
    PortRelated(NULL),
    PerstanceFlag(0)
{
}

NetworkingEvent::NetworkingEvent(INT4 event, INT4 reason):
    CEvent(MSG_OPER_PASSON, event, reason),
    FloatingRelated(NULL),
    NetworkRelated(NULL),
    RouterRelated(NULL),
    SubnetRelated(NULL),
    PortRelated(NULL),
    PerstanceFlag(0)
{
}

NetworkingEvent::~NetworkingEvent()
{
}

UINT1 NetworkingEvent::get_Persistence(void)
{
	return PerstanceFlag;
}
void NetworkingEvent::set_Persistence(UINT1 p_PersistenceFlag)
{
	PerstanceFlag=p_PersistenceFlag;
}

Base_Floating * NetworkingEvent::get_FloatingRelated(void)
{
    return FloatingRelated;
}
Base_Network  * NetworkingEvent::get_NetworkRelated(void)
{
    return NetworkRelated;
}
Base_Router   * NetworkingEvent::get_RouterRelated(void)
{
    return RouterRelated;
}
Base_Subnet   * NetworkingEvent::get_SubnetRelated(void)
{
    return SubnetRelated;
}
Base_Port   * NetworkingEvent::get_PortRelated(void)
{
    return PortRelated;
}

UINT4 NetworkingEvent::set_FloatingRelated(Base_Floating * p_FloatingRelated)
{
    FloatingRelated=p_FloatingRelated;
    return RETURN_OK;
}
UINT4 NetworkingEvent::set_NetworkRelated(Base_Network  * p_NetworkRelated)
{
    NetworkRelated=p_NetworkRelated;
    return RETURN_OK;
}
UINT4 NetworkingEvent::set_RouterRelated(Base_Router   * p_RouterRelated)
{
    RouterRelated=p_RouterRelated;
    return RETURN_OK;
}
UINT4 NetworkingEvent::set_SubnetRelated(Base_Subnet   * p_SubnetRelated)
{
    SubnetRelated=p_SubnetRelated;
    return RETURN_OK;
}

UINT4 NetworkingEvent::set_PortRelated(Base_Port   * p_PortRelated)
{
    PortRelated=p_PortRelated;
    return RETURN_OK;
}


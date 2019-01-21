#include "NetworkingEventReporter.h"
#include "bnc-error.h"
#include "log.h"

NetworkingEventReporter G_NetworkingEventReporter;

INT4 NetworkingEventReporter::report_C_Floating(Base_Floating * p_FloatingRelated,UINT1 p_PerstanceFlag)
{
    if (NULL == p_FloatingRelated)
        return BNC_ERR;

    NetworkingEvent* evt = new NetworkingEvent(NETWORKING_EVENT_FLOATING_C, EVENT_REASON_NETWORKING_FLOATING_C);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new NetworkingEvent failed!");
        return BNC_ERR;
    }

    evt->set_FloatingRelated(p_FloatingRelated);
    evt->set_Persistence(p_PerstanceFlag);
    
    CMsgPath path = getMsgPath(NETWORKING_EVENT_FLOATING_C);
    evt->setPath(path);
    
    INT1 descStr[1024];
    INT1 fixedIpStr[20] = {0}, floatIpStr[20] = {0};
    number2ip(p_FloatingRelated->get_fixed_IP(), fixedIpStr);
    number2ip(p_FloatingRelated->get_floating_IP(), floatIpStr);
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], CREATE floating"
             "(ID[%s],networkID[%s],routerID[%s],portID[%s],fixedIP[%s],floatingIP[%s])",
             NETWORKING_EVENT_FLOATING_C, path.c_str(), EVENT_REASON_NETWORKING_FLOATING_C, 
             p_FloatingRelated->get_ID().c_str(), p_FloatingRelated->get_network_ID().c_str(),
             p_FloatingRelated->get_router_ID().c_str(), p_FloatingRelated->get_port_ID().c_str(),
             fixedIpStr, floatIpStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);

    return CEventReportor::report(evt);
}
INT4 NetworkingEventReporter::report_R_Floating(void)
{
	return BNC_OK;
}
INT4 NetworkingEventReporter::report_U_Floating(Base_Floating * p_FloatingRelated,UINT1 p_PerstanceFlag)
{
    if (NULL == p_FloatingRelated)
        return BNC_ERR;

    NetworkingEvent* evt = new NetworkingEvent(NETWORKING_EVENT_FLOATING_U, EVENT_REASON_NETWORKING_FLOATING_U);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new NetworkingEvent failed!");
        return BNC_ERR;
    }

    evt->set_FloatingRelated(p_FloatingRelated);
    evt->set_Persistence(p_PerstanceFlag);
    
    CMsgPath path = getMsgPath(NETWORKING_EVENT_FLOATING_U);
    evt->setPath(path);
    
    INT1 fixedIpStr[20] = {0}, floatIpStr[20] = {0};
    number2ip(p_FloatingRelated->get_fixed_IP(), fixedIpStr);
    number2ip(p_FloatingRelated->get_floating_IP(), floatIpStr);

    INT1 descStr[1024];
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], UPDATE floating"
             "(ID[%s],networkID[%s],routerID[%s],portID[%s],fixedIP[%s],floatingIP[%s])",
             NETWORKING_EVENT_FLOATING_U, path.c_str(), EVENT_REASON_NETWORKING_FLOATING_U, 
             p_FloatingRelated->get_ID().c_str(), p_FloatingRelated->get_network_ID().c_str(),
             p_FloatingRelated->get_router_ID().c_str(), p_FloatingRelated->get_port_ID().c_str(),
             fixedIpStr, floatIpStr);
    //LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);

    return CEventReportor::report(evt);
}
INT4 NetworkingEventReporter::report_D_Floating(Base_Floating * p_FloatingRelated,UINT1 p_PerstanceFlag)
{
    if (NULL == p_FloatingRelated)
        return BNC_ERR;

    NetworkingEvent* evt = new NetworkingEvent(NETWORKING_EVENT_FLOATING_D, EVENT_REASON_NETWORKING_FLOATING_D);
    if (NULL == evt)
    {
        LOG_ERROR_FMT("new NetworkingEvent failed!");
        return BNC_ERR;
    }

    evt->set_FloatingRelated(p_FloatingRelated);
    evt->set_Persistence(p_PerstanceFlag);
    
    CMsgPath path = getMsgPath(NETWORKING_EVENT_FLOATING_D);
    evt->setPath(path);
    
    INT1 fixedIpStr[20] = {0}, floatIpStr[20] = {0};
    number2ip(p_FloatingRelated->get_fixed_IP(), fixedIpStr);
    number2ip(p_FloatingRelated->get_floating_IP(), floatIpStr);

    INT1 descStr[1024];
    snprintf(descStr, sizeof(descStr), 
             "Reported event[0x%x][%s] with reason[0x%x], DELETE floating"
             "(ID[%s],networkID[%s],routerID[%s],portID[%s],fixedIP[%s],floatingIP[%s])",
             NETWORKING_EVENT_FLOATING_D, path.c_str(), EVENT_REASON_NETWORKING_FLOATING_D, 
             p_FloatingRelated->get_ID().c_str(), p_FloatingRelated->get_network_ID().c_str(),
             p_FloatingRelated->get_router_ID().c_str(), p_FloatingRelated->get_port_ID().c_str(),
             fixedIpStr, floatIpStr);
    //LOG_WARN(descStr);

    CEventDesc desc(descStr);
    evt->setDesc(desc);

    return CEventReportor::report(evt);
}
INT4 NetworkingEventReporter::report_D_Floatings(void)
{
    return BNC_OK;
}

INT4 NetworkingEventReporter::report_C_Network(Base_Network * p_NetworkRelated,UINT1 p_PerstanceFlag)
{
    if(p_NetworkRelated!=NULL)
    {
        NetworkingEvent* NetworkEvent =new NetworkingEvent();
        if(NetworkEvent !=NULL)
        {
            NetworkEvent->set_NetworkRelated(p_NetworkRelated);
			NetworkEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_NETWORK_C);
            NetworkEvent->setPath(path);

            INT1 descStr[1024];
            list<string> subnets;
            p_NetworkRelated->get_subnets(subnets);
            list<string>::iterator it = subnets.begin();
            snprintf(descStr, sizeof(descStr), 
                     "Reported event[0x%x][%s] with reason[0x%x], CREATE network"
                     "(ID[%s],router_external[%u],subnets[size=%lu][%s])",
                     NETWORKING_EVENT_NETWORK_C, path.c_str(), EVENT_REASON_NETWORKING_NETWORK_C, 
                     p_NetworkRelated->get_ID().c_str(), p_NetworkRelated->get_router_external(),
                     subnets.size(), (it!=subnets.end())?(*it).c_str():"");
            CEventDesc desc(descStr);

            NetworkEvent->setPath(path);
            NetworkEvent->setEvent(NETWORKING_EVENT_NETWORK_C);
            NetworkEvent->setReason(EVENT_REASON_NETWORKING_NETWORK_C);
            NetworkEvent->setDesc(desc);

            return CEventReportor::report(NetworkEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_R_Network(void)
{
	NetworkingEvent* NetworkEvent =new NetworkingEvent();
	if(NetworkEvent !=NULL)
	{
        CMsgPath path = getMsgPath(NETWORKING_EVENT_NETWORK_R);
        NetworkEvent->setPath(path);
        
		char str[1024];
		snprintf(str,sizeof(str),"network research");
		CEventDesc desc(str);

		NetworkEvent->setPath(path);
		NetworkEvent->setEvent(NETWORKING_EVENT_NETWORK_R);
		NetworkEvent->setReason(EVENT_REASON_NETWORKING_NETWORK_R);
		NetworkEvent->setDesc(desc);
		LOG_DEBUG(str);

		return CEventReportor::report(NetworkEvent);
	}
	else
	{
		return RETURN_ERR;
	}
}
INT4 NetworkingEventReporter::report_U_Network(Base_Network * p_NetworkRelated,UINT1 p_PerstanceFlag)
{
    if(p_NetworkRelated!=NULL)
    {
        NetworkingEvent* NetworkEvent =new NetworkingEvent();
        if(NetworkEvent !=NULL)
        {
            NetworkEvent->set_NetworkRelated(p_NetworkRelated);
			NetworkEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_NETWORK_U);
            NetworkEvent->setPath(path);
            
            char str[1024];
            snprintf(str,sizeof(str),"network update");
            CEventDesc desc(str);

            NetworkEvent->setPath(path);
            NetworkEvent->setEvent(NETWORKING_EVENT_NETWORK_U);
            NetworkEvent->setReason(EVENT_REASON_NETWORKING_NETWORK_U);
            NetworkEvent->setDesc(desc);
            LOG_DEBUG(str);

            return CEventReportor::report(NetworkEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_D_Network(Base_Network * p_NetworkRelated,UINT1 p_PerstanceFlag)
{
    if(p_NetworkRelated!=NULL)
    {
        NetworkingEvent* NetworkEvent =new NetworkingEvent();
        if(NetworkEvent !=NULL)
        {
            NetworkEvent->set_NetworkRelated(p_NetworkRelated);
			NetworkEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_NETWORK_D);
            NetworkEvent->setPath(path);
            
            char str[1024];
            snprintf(str,sizeof(str),"network delete");
            CEventDesc desc(str);

            NetworkEvent->setPath(path);
            NetworkEvent->setEvent(NETWORKING_EVENT_NETWORK_D);
            NetworkEvent->setReason(EVENT_REASON_NETWORKING_NETWORK_D);
            NetworkEvent->setDesc(desc);
            LOG_DEBUG(str);

            return CEventReportor::report(NetworkEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_D_Networks(void)
{
	return BNC_OK;
}


INT4 NetworkingEventReporter::report_C_Router(Base_Router * p_RouterRelated,UINT1 p_PerstanceFlag)
{
    if(p_RouterRelated!=NULL)
    {
        NetworkingEvent* RouterEvent =new NetworkingEvent();
        if(RouterEvent !=NULL)
        {
            RouterEvent->set_RouterRelated(p_RouterRelated);
			RouterEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_ROUTER_C);
            RouterEvent->setPath(path);
            
            INT1 descStr[1024];
            INT1 ipStr[20] = {0};
            RouterExternalGateway& extGw = p_RouterRelated->get_external_gateway_info();
            Fixed_IP& fixedIp = extGw.get_external_IP();
            number2ip(fixedIp.get_IP(), ipStr);
            snprintf(descStr, sizeof(descStr), 
                     "Reported event[0x%x][%s] with reason[0x%x], CREATE router"
                     "(ID[%s],external_network_ID[%s],external_enabled_snat[%d],external_subnet_ID[%s],external_IP[%s])",
                     NETWORKING_EVENT_ROUTER_C, path.c_str(), EVENT_REASON_NETWORKING_ROUTER_C, 
                     p_RouterRelated->get_ID().c_str(), extGw.get_network_ID().c_str(),
                     extGw.get_enabled_snat(), fixedIp.get_subnet_ID().c_str(), ipStr);
            CEventDesc desc(descStr);

            RouterEvent->setPath(path);
            RouterEvent->setEvent(NETWORKING_EVENT_ROUTER_C);
            RouterEvent->setReason(EVENT_REASON_NETWORKING_ROUTER_C);
            RouterEvent->setDesc(desc);

            return CEventReportor::report(RouterEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_R_Router(void)
{
	NetworkingEvent* RouterEvent =new NetworkingEvent();
	if(RouterEvent !=NULL)
	{
        CMsgPath path = getMsgPath(NETWORKING_EVENT_ROUTER_R);
        RouterEvent->setPath(path);
        
		char str[1024];
		snprintf(str,sizeof(str),"router research");
		CEventDesc desc(str);

		RouterEvent->setPath(path);
		RouterEvent->setEvent(NETWORKING_EVENT_ROUTER_R);
		RouterEvent->setReason(EVENT_REASON_NETWORKING_ROUTER_R);
		RouterEvent->setDesc(desc);
		LOG_DEBUG(str);

		return CEventReportor::report(RouterEvent);
	}
	else
	{
		return RETURN_ERR;
	}
}
INT4 NetworkingEventReporter::report_U_Router(Base_Router * p_RouterRelated,UINT1 p_PerstanceFlag)
{
    if(p_RouterRelated!=NULL)
    {
        NetworkingEvent* RouterEvent =new NetworkingEvent();
        if(RouterEvent !=NULL)
        {
            RouterEvent->set_RouterRelated(p_RouterRelated);
			RouterEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_ROUTER_U);
            RouterEvent->setPath(path);
            
            char str[1024];
            snprintf(str,sizeof(str),"router update");
            CEventDesc desc(str);

            RouterEvent->setPath(path);
            RouterEvent->setEvent(NETWORKING_EVENT_ROUTER_U);
            RouterEvent->setReason(EVENT_REASON_NETWORKING_ROUTER_U);
            RouterEvent->setDesc(desc);
            LOG_DEBUG(str);

            return CEventReportor::report(RouterEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_D_Router(Base_Router * p_RouterRelated,UINT1 p_PerstanceFlag)
{
    if(p_RouterRelated!=NULL)
    {
        NetworkingEvent* RouterEvent =new NetworkingEvent();
        if(RouterEvent !=NULL)
        {
            RouterEvent->set_RouterRelated(p_RouterRelated);
			RouterEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_ROUTER_D);
            RouterEvent->setPath(path);
            
            char str[1024];
            snprintf(str,sizeof(str),"router delete");
            CEventDesc desc(str);

            RouterEvent->setPath(path);
            RouterEvent->setEvent(NETWORKING_EVENT_ROUTER_D);
            RouterEvent->setReason(EVENT_REASON_NETWORKING_ROUTER_D);
            RouterEvent->setDesc(desc);
            LOG_DEBUG(str);

            return CEventReportor::report(RouterEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_D_Routers(void)
{
	return BNC_OK;
}

INT4 NetworkingEventReporter::report_C_Subnet(Base_Subnet * p_SubnetRelated,UINT1 p_PerstanceFlag)
{
    if(p_SubnetRelated!=NULL)
    {
        NetworkingEvent* SubnetEvent =new NetworkingEvent();
        if(SubnetEvent !=NULL)
        {
            SubnetEvent->set_SubnetRelated(p_SubnetRelated);
			SubnetEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_SUBNET_C);
            SubnetEvent->setPath(path);
            
            INT1 descStr[1024];
            INT1 gwIpStr[20] = {0}, startIpStr[20] = {0}, endIpStr[20] = {0};
            number2ip(p_SubnetRelated->get_gateway_IP(), gwIpStr);
            number2ip(p_SubnetRelated->getFirstStartIp(), startIpStr);
            number2ip(p_SubnetRelated->getFirstEndIp(), endIpStr);
            snprintf(descStr, sizeof(descStr), 
                     "Reported event[0x%x][%s] with reason[0x%x], CREATE subnet"
                     "(ID[%s],network_ID[%s],cidr[%s],gateway_IP[%s],IP_range[%s-%s])",
                     NETWORKING_EVENT_SUBNET_C, path.c_str(), EVENT_REASON_NETWORKING_SUBNET_C, 
                     p_SubnetRelated->get_ID().c_str(), p_SubnetRelated->get_network_ID().c_str(),
                     p_SubnetRelated->get_cidr().c_str(), gwIpStr, startIpStr, endIpStr);
            CEventDesc desc(descStr);

            SubnetEvent->setPath(path);
            SubnetEvent->setEvent(NETWORKING_EVENT_SUBNET_C);
            SubnetEvent->setReason(EVENT_REASON_NETWORKING_SUBNET_C);
            SubnetEvent->setDesc(desc);

            return CEventReportor::report(SubnetEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_R_Subnet(void)
{
	NetworkingEvent* SubnetEvent =new NetworkingEvent();
	if(SubnetEvent !=NULL)
	{
        CMsgPath path = getMsgPath(NETWORKING_EVENT_SUBNET_R);
        SubnetEvent->setPath(path);
        
		char str[1024];
		snprintf(str,sizeof(str),"subnet research");
		CEventDesc desc(str);

		SubnetEvent->setPath(path);
		SubnetEvent->setEvent(NETWORKING_EVENT_SUBNET_R);
		SubnetEvent->setReason(EVENT_REASON_NETWORKING_SUBNET_R);
		SubnetEvent->setDesc(desc);
		LOG_DEBUG(str);

		return CEventReportor::report(SubnetEvent);
	}
	else
	{
		return RETURN_ERR;
	}
}
INT4 NetworkingEventReporter::report_U_Subnet(Base_Subnet * p_SubnetRelated,UINT1 p_PerstanceFlag)
{
    if(p_SubnetRelated!=NULL)
    {
        NetworkingEvent* SubnetEvent =new NetworkingEvent();
        if(SubnetEvent !=NULL)
        {
            SubnetEvent->set_SubnetRelated(p_SubnetRelated);
			SubnetEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_SUBNET_U);
            SubnetEvent->setPath(path);
            
            char str[1024];
            snprintf(str,sizeof(str),"subnet update");
            CEventDesc desc(str);

            SubnetEvent->setPath(path);
            SubnetEvent->setEvent(NETWORKING_EVENT_SUBNET_U);
            SubnetEvent->setReason(EVENT_REASON_NETWORKING_SUBNET_U);
            SubnetEvent->setDesc(desc);
            LOG_DEBUG(str);

            return CEventReportor::report(SubnetEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_D_Subnet(Base_Subnet * p_SubnetRelated,UINT1 p_PerstanceFlag)
{
    if(p_SubnetRelated!=NULL)
    {
        NetworkingEvent* SubnetEvent =new NetworkingEvent();
        if(SubnetEvent !=NULL)
        {
            SubnetEvent->set_SubnetRelated(p_SubnetRelated);
			SubnetEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_SUBNET_D);
            SubnetEvent->setPath(path);
            
            char str[1024];
            snprintf(str,sizeof(str),"subnet delete");
            CEventDesc desc(str);

            SubnetEvent->setPath(path);
            SubnetEvent->setEvent(NETWORKING_EVENT_SUBNET_D);
            SubnetEvent->setReason(EVENT_REASON_NETWORKING_SUBNET_D);
            SubnetEvent->setDesc(desc);
            LOG_DEBUG(str);

            return CEventReportor::report(SubnetEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_D_Subnets(void)
{
	return BNC_OK;
}



INT4 NetworkingEventReporter::report_C_Port(Base_Port * p_PortRelated,UINT1 p_PerstanceFlag)
{
    if(p_PortRelated!=NULL)
    {
        NetworkingEvent* PortEvent =new NetworkingEvent();
        if(PortEvent !=NULL)
        {
            PortEvent->set_PortRelated(p_PortRelated);
			PortEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_PORT_C);
            PortEvent->setPath(path);
            
            INT1 descStr[1024];
            INT1 macStr[48] = {0};
            mac2str(p_PortRelated->get_MAC(), macStr);
            INT1 ipStr[20] = {0};
            number2ip(p_PortRelated->getFirstFixedIp(), ipStr);
            snprintf(descStr, sizeof(descStr), 
                     "Reported event[0x%x][%s] with reason[0x%x], CREATE port"
                     "(ID[%s],device_owner[%s],device_ID[%s],network_ID[%s],MAC[%s],Fixed_IPs[%s|%s])",
                     NETWORKING_EVENT_PORT_C, path.c_str(), EVENT_REASON_NETWORKING_PORT_C, 
                     p_PortRelated->get_ID().c_str(), p_PortRelated->get_device_owner().c_str(), 
                     p_PortRelated->get_device_ID().c_str(), p_PortRelated->get_network_ID().c_str(),
                     macStr, ipStr, p_PortRelated->getFirstSubnet().c_str());
            CEventDesc desc(descStr);

            PortEvent->setPath(path);
            PortEvent->setEvent(NETWORKING_EVENT_PORT_C);
            PortEvent->setReason(EVENT_REASON_NETWORKING_PORT_C);
            PortEvent->setDesc(desc);

            return CEventReportor::report(PortEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_R_Port(void)
{
	NetworkingEvent* PortEvent =new NetworkingEvent();
	if(PortEvent !=NULL)
	{
        CMsgPath path = getMsgPath(NETWORKING_EVENT_PORT_R);
        PortEvent->setPath(path);
        
		char str[1024];
		snprintf(str,sizeof(str),"port research");
		CEventDesc desc(str);

		PortEvent->setPath(path);
		PortEvent->setEvent(NETWORKING_EVENT_PORT_R);
		PortEvent->setReason(EVENT_REASON_NETWORKING_PORT_R);
		PortEvent->setDesc(desc);
		LOG_DEBUG(str);

		return CEventReportor::report(PortEvent);
	}
	else
	{
		return RETURN_ERR;
	}
}
INT4 NetworkingEventReporter::report_U_Port(Base_Port * p_PortRelated,UINT1 p_PerstanceFlag)
{
    if(p_PortRelated!=NULL)
    {
        NetworkingEvent* PortEvent =new NetworkingEvent();
        if(PortEvent !=NULL)
        {
            PortEvent->set_PortRelated(p_PortRelated);
			PortEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_PORT_U);
            PortEvent->setPath(path);
            
            char str[1024];
            snprintf(str,sizeof(str),"port update");
            CEventDesc desc(str);

            PortEvent->setPath(path);
            PortEvent->setEvent(NETWORKING_EVENT_PORT_U);
            PortEvent->setReason(EVENT_REASON_NETWORKING_PORT_U);
            PortEvent->setDesc(desc);
            LOG_DEBUG(str);

            return CEventReportor::report(PortEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_D_Port(Base_Port * p_PortRelated,UINT1 p_PerstanceFlag)
{
    if(p_PortRelated!=NULL)
    {
        NetworkingEvent* PortEvent =new NetworkingEvent();
        if(PortEvent !=NULL)
        {
            PortEvent->set_PortRelated(p_PortRelated);
			PortEvent->set_Persistence(p_PerstanceFlag);

            CMsgPath path = getMsgPath(NETWORKING_EVENT_PORT_D);
            PortEvent->setPath(path);
            
            char str[1024];
            snprintf(str,sizeof(str),"port delete");
            CEventDesc desc(str);

            PortEvent->setPath(path);
            PortEvent->setEvent(NETWORKING_EVENT_PORT_D);
            PortEvent->setReason(EVENT_REASON_NETWORKING_PORT_D);
            PortEvent->setDesc(desc);
            LOG_DEBUG(str);

            return CEventReportor::report(PortEvent);
        }
        else
        {
            return RETURN_ERR;
        }
    }
    else
    {
        return RETURN_ERR;
    }
}
INT4 NetworkingEventReporter::report_D_Ports(void)
{
	return BNC_OK;
}

CMsgPath NetworkingEventReporter::getMsgPath(INT4 event)
{
    return g_NetworkingEventPath[0];
}



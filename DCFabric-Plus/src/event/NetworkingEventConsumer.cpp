#include "NetworkingEventConsumer.h"
#include "log.h"

NetworkingEventConsumer::NetworkingEventConsumer():
    FloatingMgr(NULL),
    NetworkMgr(NULL),
    RouterMgr(NULL),
    SubnetMgr(NULL),
    PortMgr(NULL),
    ExternalMgr(NULL)
{
}

NetworkingEventConsumer::NetworkingEventConsumer(BaseFloatingMgr* p_FloatingMgr,BaseNetworkMgr* p_NetworkMgr,BaseRouterMgr* p_RouterMgr,BaseSubnetMgr* p_SubnetMgr,BasePortMgr* p_PortMgr,BaseExternalMgr* p_ExternalMgr)
{
    FloatingMgr=p_FloatingMgr;
    NetworkMgr=p_NetworkMgr;
    RouterMgr=p_RouterMgr;
    SubnetMgr=p_SubnetMgr;
	PortMgr=p_PortMgr;
	ExternalMgr=p_ExternalMgr;
}

NetworkingEventConsumer::~NetworkingEventConsumer()
{
}

INT4 NetworkingEventConsumer::onregister(void)
{
    CMsgPath path(g_NetworkingEventPath[0]);
    return CEventNotifier::onregister(path);
}
void NetworkingEventConsumer::deregister(void)
{
    CMsgPath path(g_NetworkingEventPath[0]);
    CEventNotifier::deregister(path);
}

INT4 NetworkingEventConsumer::consume(CSmartPtr<CMsgCommon> evt)
{
    INT4 ret = BNC_ERR;
    
    if (evt.isNull())
        return ret;

    NetworkingEvent* event = (NetworkingEvent*)evt.getPtr();
    LOG_INFO(event->getDesc().c_str());

    switch (event->getEvent())
    {
        case NETWORKING_EVENT_FLOATING_C:
            LOG_DEBUG("NETWORKING_EVENT_FLOATING_C");
            ret = floating_C(* event);
            break;
        case NETWORKING_EVENT_FLOATING_R:
            LOG_DEBUG("NETWORKING_EVENT_FLOATING_R");
            ret = floating_R(* event);
            break;
        case NETWORKING_EVENT_FLOATING_U:
            LOG_DEBUG("NETWORKING_EVENT_FLOATING_U");
            ret = floating_U(* event);
            break;
        case NETWORKING_EVENT_FLOATING_D:
            LOG_DEBUG("NETWORKING_EVENT_FLOATING_D");
            ret = floating_D(* event);
            break;
        case NETWORKING_EVENT_FLOATING_DS:
            LOG_DEBUG("NETWORKING_EVENT_FLOATING_DS");
            ret = floating_DS(* event);
            break;

        case NETWORKING_EVENT_NETWORK_C:
            LOG_DEBUG("NETWORKING_EVENT_NETWORK_C");
            ret = network_C(* event);
            break;
        case NETWORKING_EVENT_NETWORK_R:
            LOG_DEBUG("NETWORKING_EVENT_NETWORK_R");
            ret = network_R(* event);
            break;
        case NETWORKING_EVENT_NETWORK_U:
            LOG_DEBUG("NETWORKING_EVENT_NETWORK_U");
            ret = network_U(* event);
            break;
        case NETWORKING_EVENT_NETWORK_D:
            LOG_DEBUG("NETWORKING_EVENT_NETWORK_D");
            ret = network_D(* event);
            break;
        case NETWORKING_EVENT_NETWORK_DS:
            LOG_DEBUG("NETWORKING_EVENT_NETWORK_DS");
            ret = network_DS(* event);
            break;

        case NETWORKING_EVENT_ROUTER_C:
            LOG_DEBUG("NETWORKING_EVENT_ROUTER_C");
            ret = router_C(* event);
            break;
        case NETWORKING_EVENT_ROUTER_R:
            LOG_DEBUG("NETWORKING_EVENT_ROUTER_R");
            ret = router_R(* event);
            break;
        case NETWORKING_EVENT_ROUTER_U:
            LOG_DEBUG("NETWORKING_EVENT_ROUTER_U");
            ret = router_U(* event);
            break;
        case NETWORKING_EVENT_ROUTER_D:
            LOG_DEBUG("NETWORKING_EVENT_ROUTER_D");
            ret = router_D(* event);
            break;
        case NETWORKING_EVENT_ROUTER_DS:
            LOG_DEBUG("NETWORKING_EVENT_ROUTER_DS");
            ret = router_DS(* event);
            break;

        case NETWORKING_EVENT_SUBNET_C:
            LOG_DEBUG("NETWORKING_EVENT_SUBNET_C");
            ret = subnet_C(* event);
            break;
        case NETWORKING_EVENT_SUBNET_R:
            LOG_DEBUG("NETWORKING_EVENT_SUBNET_R");
            ret = subnet_R(* event);
            break;
        case NETWORKING_EVENT_SUBNET_U:
            LOG_DEBUG("NETWORKING_EVENT_SUBNET_U");
            ret = subnet_U(* event);
            break;
        case NETWORKING_EVENT_SUBNET_D:
            LOG_DEBUG("NETWORKING_EVENT_SUBNET_D");
            ret = subnet_D(* event);
            break;
        case NETWORKING_EVENT_SUBNET_DS:
            LOG_DEBUG("NETWORKING_EVENT_SUBNET_DS");
            ret = subnet_DS(* event);
            break;

        case NETWORKING_EVENT_PORT_C:
            LOG_DEBUG("NETWORKING_EVENT_PORT_C");
            ret = port_C(* event);
            break;
        case NETWORKING_EVENT_PORT_R:
            LOG_DEBUG("NETWORKING_EVENT_PORT_R");
            ret = port_R(* event);
            break;
        case NETWORKING_EVENT_PORT_U:
            LOG_DEBUG("NETWORKING_EVENT_PORT_U");
            ret = port_U(* event);
            break;
        case NETWORKING_EVENT_PORT_D:
            LOG_DEBUG("NETWORKING_EVENT_PORT_D");
            ret = port_D(* event);
            break;
        case NETWORKING_EVENT_PORT_DS:
            LOG_DEBUG("NETWORKING_EVENT_PORT_DS");
            ret = port_DS(* event);
            break;

        default:
            LOG_DEBUG_FMT("%s consume invalid event[%d] !", toString(), event->getEvent());
            break;
    }

	return ret;
}

//===========================================================================================

UINT1 NetworkingEventConsumer::floating_C(NetworkingEvent &p_event)
{
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->floating_C(*(p_event.get_FloatingRelated()));
	}
    FloatingMgr->insertNode_ByFloating(p_event.get_FloatingRelated());
    //TBD   other operation need to be done after create
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::floating_R(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::floating_U(NetworkingEvent &p_event)
{
#if  0  //modify by ycy
    Base_Floating * UpdatedFloating =p_event.get_FloatingRelated();
    if(UpdatedFloating)
    {
		if(p_event.get_Persistence())
		{
			NetworkingPersistence::Get_Instance()->floating_U(*UpdatedFloating);
		}
        Base_Floating * TargetFloating = FloatingMgr->targetFloating_ByID(UpdatedFloating->get_ID());
        if(TargetFloating)
        {
            //TBD   other operation need to be done before update
            TargetFloating->set_status(UpdatedFloating->get_status());
            TargetFloating->set_port_ID(UpdatedFloating->get_port_ID());
            TargetFloating->set_router_ID(UpdatedFloating->get_router_ID());
            TargetFloating->set_fixed_IP(UpdatedFloating->get_fixed_IP());
        }
        delete UpdatedFloating;
    }
#endif

	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->floating_D((p_event.get_FloatingRelated())->get_ID());
	}
    FloatingMgr->deleteNode_ByFloatingID(p_event.get_FloatingRelated()->get_ID());
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->floating_C(*(p_event.get_FloatingRelated()));
	}
    FloatingMgr->insertNode_ByFloating(p_event.get_FloatingRelated());
	
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::floating_D(NetworkingEvent &p_event)
{
    //TBD   other operation need to be done before delete
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->floating_D((p_event.get_FloatingRelated())->get_ID());
	}
    FloatingMgr->deleteNode_ByFloating(p_event.get_FloatingRelated());
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::floating_DS(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}

//===========================================================================================

UINT1 NetworkingEventConsumer::network_C(NetworkingEvent &p_event)
{
	if(p_event.get_Persistence())
	{
	NetworkingPersistence::Get_Instance()->network_C(*(p_event.get_NetworkRelated()));
	}
    NetworkMgr->insertNode_ByNetwork(p_event.get_NetworkRelated());
    //TBD   other operation need to be done after create
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::network_R(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::network_U(NetworkingEvent &p_event)
{
    Base_Network * UpdatedNetwork =p_event.get_NetworkRelated();
	list<string> old_subnets;
	list<string> new_subnets;
	list<string> ::iterator old_subnetsItor;
	list<string> ::iterator new_subnetsItor;	
	
    if(UpdatedNetwork)
    {
		network_U_ExternalRelatedOperation(UpdatedNetwork);
		if(p_event.get_Persistence())
		{
			NetworkingPersistence::Get_Instance()->network_U(*UpdatedNetwork);
		}
        Base_Network * TargetNetwork = NetworkMgr->targetNetwork_ByID(UpdatedNetwork->get_ID());
        if(TargetNetwork)
        {
            //TBD   other operation need to be done before update
            TargetNetwork->change_status(UpdatedNetwork->get_status());
            TargetNetwork->change_shared(UpdatedNetwork->get_shared());
            TargetNetwork->change_router_external(UpdatedNetwork->get_router_external());
			
			TargetNetwork->get_subnets(old_subnets);
			UpdatedNetwork->get_subnets(new_subnets);
			old_subnetsItor=old_subnets.begin();
			new_subnetsItor=new_subnets.begin();
			while(old_subnetsItor!=old_subnets.end())
			{
				TargetNetwork->del_subnet(*old_subnetsItor);
				old_subnetsItor++;
			}
			while(new_subnetsItor!=new_subnets.end())
			{
				TargetNetwork->add_subnet(*new_subnetsItor);
				new_subnetsItor++;
			}
        }
        delete UpdatedNetwork;
    }

    return BNC_OK;
}
UINT1 NetworkingEventConsumer::network_D(NetworkingEvent &p_event)
{
    //TBD   other operation need to be done before delete
	Base_Network * DeletedNetwork =p_event.get_NetworkRelated();
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->network_D((p_event.get_NetworkRelated())->get_ID());
	}
	network_D_ExternalRelatedOperation(DeletedNetwork);
    NetworkMgr->deleteNode_ByNetwork(DeletedNetwork);
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::network_DS(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}


//===========================================================================================

UINT1 NetworkingEventConsumer::router_C(NetworkingEvent &p_event)
{
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->router_C(*(p_event.get_RouterRelated()));
	}
    RouterMgr->insertNode_ByRouter(p_event.get_RouterRelated());
    //TBD   other operation need to be done after create
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::router_R(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::router_U(NetworkingEvent &p_event)
{
    Base_Router * UpdatedRouter =p_event.get_RouterRelated();
    if(UpdatedRouter)
    {
		if(p_event.get_Persistence())
		{
			NetworkingPersistence::Get_Instance()->router_U(*UpdatedRouter);
		}
		
        RouterExternalGateway & UpdatedExternalGateway =UpdatedRouter->get_external_gateway_info();
        Fixed_IP & UpdatedFixedIP =UpdatedExternalGateway.get_external_IP();

        Base_Router * TargetRouter = RouterMgr->targetRouter_ByID(UpdatedRouter->get_ID());
        if(TargetRouter)
        {
            //TBD   other operation need to be done before update
            TargetRouter->set__external_gateway_info__network_ID(UpdatedExternalGateway.get_network_ID());
            TargetRouter->set__external_gateway_info__enabled_snat(UpdatedExternalGateway.get_enabled_snat());
            TargetRouter->set__external_gateway_info__subnet_ID(UpdatedFixedIP.get_subnet_ID());
            TargetRouter->set__external_gateway_info__IP(UpdatedFixedIP.get_IP());
        }
        delete UpdatedRouter;
    }

    return BNC_OK;
}
UINT1 NetworkingEventConsumer::router_D(NetworkingEvent &p_event)
{
    //TBD   other operation need to be done before delete
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->router_D((p_event.get_RouterRelated())->get_ID());
	}
    RouterMgr->deleteNode_ByRouter(p_event.get_RouterRelated());
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::router_DS(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}

//===========================================================================================

UINT1 NetworkingEventConsumer::subnet_C(NetworkingEvent &p_event)
{
	Base_Subnet * CreatedSubnet =p_event.get_SubnetRelated();
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->subnet_C(*(p_event.get_SubnetRelated()));
	}
	subnet_C_ExternalRelatedOperation(CreatedSubnet);
    SubnetMgr->insertNode_BySubnet(CreatedSubnet);
    //TBD   other operation need to be done after create
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::subnet_R(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::subnet_U(NetworkingEvent &p_event)
{
    Base_Subnet * UpdatedSubnet =p_event.get_SubnetRelated();
    list<IP_pool *> TargetPools;
    list<IP_pool *> UpdatePools;
    //IP_pool *TargetIPPool;
    //IP_pool *UpdateIPPool;
    list<IP_pool *>::iterator TargetItor;
    list<IP_pool *>::iterator UpdateItor;
    if(UpdatedSubnet)
    {
		subnet_U_ExternalRelatedOperation(UpdatedSubnet);
		
		if(p_event.get_Persistence())
		{
			NetworkingPersistence::Get_Instance()->subnet_U(*UpdatedSubnet);
		}
		
        Base_Subnet * TargetSubnet = SubnetMgr->targetSubnet_ByID(UpdatedSubnet->get_ID());
        if(TargetSubnet)
        {
            //TBD   other operation need to be done before update
            TargetSubnet->change_status(UpdatedSubnet->get_status());
            TargetSubnet->change_cidr(UpdatedSubnet->get_cidr());
            TargetSubnet->change_gateway_IP(UpdatedSubnet->get_gateway_IP());

            TargetSubnet->get_allocation_pools(TargetPools);
            UpdatedSubnet->get_allocation_pools(UpdatePools);
            TargetItor=TargetPools.begin();
            UpdateItor=UpdatePools.begin();
            while (TargetItor != TargetPools.end())
            {
                TargetSubnet->del_allocation_pool((*TargetItor)->get_IP_start(),(*TargetItor)->get_IP_end());
                TargetItor++;
            }
            while (UpdateItor != UpdatePools.end())
            {
                TargetSubnet->add_allocation_pool((*UpdateItor)->get_IP_start(),(*UpdateItor)->get_IP_end());
                UpdateItor++;
            }
        }
        delete UpdatedSubnet;
    }
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::subnet_D(NetworkingEvent &p_event)
{
    //TBD   other operation need to be done before delete
	Base_Subnet * DeletedSubnet =p_event.get_SubnetRelated();
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->subnet_D((p_event.get_SubnetRelated())->get_ID());
	}
	subnet_D_ExternalRelatedOperation(DeletedSubnet);
    SubnetMgr->deleteNode_BySubnet(DeletedSubnet);
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::subnet_DS(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}

//===========================================================================================

UINT1 NetworkingEventConsumer::port_C(NetworkingEvent &p_event)
{
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->port_C(*(p_event.get_PortRelated()));
	}
    PortMgr->insertNode_ByPort(p_event.get_PortRelated());
    //TBD   other operation need to be done after create
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::port_R(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::port_U(NetworkingEvent &p_event)
{
    Base_Port * UpdatedPort =p_event.get_PortRelated();
	list<Fixed_IP*> old_Fixed_IPs;
	list<Fixed_IP*> new_Fixed_IPs;
	list<Fixed_IP *>::iterator OldItor;
    list<Fixed_IP *>::iterator NewItor;
	if(UpdatedPort)
	{
		if(p_event.get_Persistence())
		{
			NetworkingPersistence::Get_Instance()->port_U(*UpdatedPort);
		}
		Base_Port * TargetPort =PortMgr->targetPort_ByID(UpdatedPort->get_ID());
		if(TargetPort)
		{
			//TBD other operation need to be done
			UpdatedPort->get_Fixed_IPs(new_Fixed_IPs);
			TargetPort->get_Fixed_IPs(old_Fixed_IPs);
			OldItor=old_Fixed_IPs.begin();
			NewItor=new_Fixed_IPs.begin();
			while(OldItor!=old_Fixed_IPs.end())
			{
				TargetPort->del_fixed_IP((*OldItor)->get_IP(),(*OldItor)->get_subnet_ID());
				OldItor++;
			}
			while(NewItor!=new_Fixed_IPs.end())
			{
				TargetPort->add_fixed_IP((*NewItor)->get_IP(),(*NewItor)->get_subnet_ID());
				NewItor++;
			}
		}	
		delete UpdatedPort;
	}
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::port_D(NetworkingEvent &p_event)
{
    //TBD   other operation need to be done before delete
	if(p_event.get_Persistence())
	{
		NetworkingPersistence::Get_Instance()->port_D((p_event.get_PortRelated())->get_ID());
	}
    PortMgr->deleteNode_ByPort(p_event.get_PortRelated());
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::port_DS(NetworkingEvent &p_event)
{
    //do nothing
    return BNC_OK;
}




UINT1 NetworkingEventConsumer::network_U_ExternalRelatedOperation(Base_Network * UpdatedNetwork)
{
	list<Base_External *> TargetExternals;
	ExternalMgr->targetExternal_Bynetwork_ID(UpdatedNetwork->get_ID(),TargetExternals);
	if(TargetExternals.empty())
	{
		if(UpdatedNetwork->get_router_external())
		{// the network updated changed router_external state from false to true
			//visit subnet list and create new external 
			list<string> subnets;
			list<string>::iterator itor;
			Base_Subnet* TargetSubnet=NULL;
			UpdatedNetwork->get_subnets(subnets);
			itor=subnets.begin();
			while(itor!=subnets.end())
			{		
				TargetSubnet=SubnetMgr->targetSubnet_ByID(*itor);
				if((TargetSubnet)&&(TargetSubnet->get_network_ID()==UpdatedNetwork->get_ID()))
				{
					Base_External* CreatedExternal =new Base_External(TargetSubnet->get_network_ID(),TargetSubnet->get_ID(),TargetSubnet->get_gateway_IP());
					ExternalMgr->insertNode_ByExternal(CreatedExternal);
				}
				itor++;
			}
		}
		else
		{
			//do nothing
		}
	}
	else
	{
		if(UpdatedNetwork->get_router_external())
		{
			//do nothing
		}
		else
		{// the network updated changed router_external state from true to false
			//delete all related external
			ExternalMgr->deleteNode_Bynetwork_ID(UpdatedNetwork->get_ID());
		}
	}
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::network_D_ExternalRelatedOperation(Base_Network * DeletedNetwork)
{
	list<Base_External *> TargetExternals;
	ExternalMgr->targetExternal_Bynetwork_ID(DeletedNetwork->get_ID(),TargetExternals);
	if(TargetExternals.empty())
	{
		//do nothing
	}
	else
	{
		//delete all related external
		ExternalMgr->deleteNode_Bynetwork_ID(DeletedNetwork->get_ID());
	}
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::network_DS_ExternalRelatedOperation(NetworkingEvent &p_event)
{
	//TBD
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::subnet_C_ExternalRelatedOperation(Base_Subnet * CreatedSubnet)
{
	Base_Network* TargetNetwork=NULL;
	TargetNetwork=NetworkMgr->targetNetwork_ByID(CreatedSubnet->get_network_ID());
	if(TargetNetwork)
	{
		if(TargetNetwork->get_router_external())
		{//subnet created is external subnet
			//need create new external
			Base_External* CreatedExternal =new Base_External(CreatedSubnet->get_network_ID(),CreatedSubnet->get_ID(),CreatedSubnet->get_gateway_IP());
			ExternalMgr->insertNode_ByExternal(CreatedExternal);
		}
		else
		{
			//do nothing
		}
	}
	else
	{
		//do nothing
	}
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::subnet_U_ExternalRelatedOperation(Base_Subnet * UpdatedSubnet)
{
	Base_External * TargetExternal =NULL;
	TargetExternal =ExternalMgr->targetExternal_Bysubnet_ID(UpdatedSubnet->get_ID());
	if(TargetExternal)
	{
		if((TargetExternal->get_gateway_IP())==(UpdatedSubnet->get_gateway_IP()))
		{
			//do nothing 
		}
		else
		{//the subnet updated has changed gateway_IP and is external subnet
			//need update gateway ip
			//TBD here need to consider how to deal with old gateway mac and related flow
			TargetExternal->set_gateway_IP(UpdatedSubnet->get_gateway_IP());
		}
	}
	else
	{
		Base_Network* TargetNetwork=NULL;
		TargetNetwork=NetworkMgr->targetNetwork_ByID(UpdatedSubnet->get_network_ID());
		if(TargetNetwork)
		{
			if(TargetNetwork->get_router_external())
			{// the subnet updated has changed related network and related network is external network
				//need create new external
				Base_External* CreatedExternal =new Base_External(UpdatedSubnet->get_network_ID(),UpdatedSubnet->get_ID(),UpdatedSubnet->get_gateway_IP());
				ExternalMgr->insertNode_ByExternal(CreatedExternal);
			}
			else
			{
				//do nothing
			}
		}
		else
		{
			//do nothing
		}
	}
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::subnet_D_ExternalRelatedOperation(Base_Subnet * DeletedSubnet)
{
	Base_External * TargetExternal =NULL;
	TargetExternal =ExternalMgr->targetExternal_Bysubnet_ID(DeletedSubnet->get_ID());
	if(TargetExternal)
	{// the subnet deleted is external subnet
		ExternalMgr->deleteNode_ByExternal(TargetExternal);
	}
	else
	{
		//do nothing 
	}
    return BNC_OK;
}
UINT1 NetworkingEventConsumer::subnet_DS_ExternalRelatedOperation(NetworkingEvent &p_event)
{
	//TBD
    return BNC_OK;
}


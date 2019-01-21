#include "BaseFloatingVerification.h"
#include "log.h"

UINT4 Verify_ID         (Base_Floating& p_Floating);
UINT4 Verify_name       (Base_Floating& p_Floating);
UINT4 Verify_tenant_ID  (Base_Floating& p_Floating);
UINT4 Verify_network_ID (Base_Floating& p_Floating);
UINT4 Verify_router_ID  (Base_Floating& p_Floating);
UINT4 Verify_port_ID    (Base_Floating& p_Floating);
UINT4 Verify_status     (Base_Floating& p_Floating);
UINT4 Verify_fixed_IP   (Base_Floating& p_Floating);
UINT4 Verify_floating_IP(Base_Floating& p_Floating);

UINT4  Verify_CreatedFloating   (Base_Floating& p_Floating)
{
    UINT4 VerifyResult =Verify_ID(p_Floating);
	if(FLOATING_VERIFY_SUCCESS != VerifyResult)
	{
		LOG_ERROR("Verify_ID FAIL");
		goto ErrPro;
		
	}
	VerifyResult =Verify_name(p_Floating);
    if(VerifyResult != FLOATING_VERIFY_SUCCESS)
	{
		LOG_ERROR("Verify_name FAIL");
		goto ErrPro;
	}
	VerifyResult =Verify_tenant_ID(p_Floating);
    if(VerifyResult != FLOATING_VERIFY_SUCCESS)
	{
		LOG_ERROR("Verify_tenant_ID FAIL");
		goto ErrPro;
	}
	VerifyResult =Verify_network_ID(p_Floating);
    if(VerifyResult != FLOATING_VERIFY_SUCCESS)
	{
		LOG_ERROR("Verify_network_ID FAIL");
		goto ErrPro;
	}
	VerifyResult =Verify_floating_IP(p_Floating);
    if(VerifyResult != FLOATING_VERIFY_SUCCESS)
	{
		LOG_ERROR("Verify_floating_IP FAIL");
		goto ErrPro;
	}
	if((p_Floating.get_port_ID()=="")&&(p_Floating.get_router_ID()=="")&&(p_Floating.get_fixed_IP()==INADDR_NONE))
	{
	    LOG_ERROR("Verify_router_ID FAIL");
		goto ErrPro;
	}
	VerifyResult =Verify_router_ID(p_Floating);
    if(VerifyResult != FLOATING_VERIFY_SUCCESS)
	{
		LOG_ERROR("Verify_router_ID FAIL");
		goto ErrPro;
	}
	VerifyResult =Verify_port_ID(p_Floating);
    if(VerifyResult != FLOATING_VERIFY_SUCCESS)
    {
		LOG_ERROR("Verify_port_ID FAIL");
		goto ErrPro;
	}
	VerifyResult =Verify_fixed_IP(p_Floating);
    if(VerifyResult != FLOATING_VERIFY_SUCCESS)
    {
    	LOG_ERROR("Verify_fixed_IP FAIL");
        goto ErrPro;
    }

	return FLOATING_VERIFY_SUCCESS;

ErrPro:
	LOG_WARN_FMT(">>>> VerifyResult[%d]! <<<<", VerifyResult);
	return VerifyResult;
}

UINT4  Verify_UpdatedFloating   (Base_Floating& p_Floating)
{
    UINT4 VerifyResult =Verify_ID(p_Floating);
    if(VerifyResult == FLOATING_VERIFY_ID_ERROR_CONFLICT)
    {
        VerifyResult =Verify_name(p_Floating);
        if(VerifyResult == FLOATING_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID(p_Floating);
            if(VerifyResult == FLOATING_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_network_ID(p_Floating);
                if(VerifyResult == FLOATING_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_floating_IP(p_Floating);
                    if(VerifyResult == FLOATING_VERIFY_SUCCESS)
                    {
                        if((p_Floating.get_port_ID()=="")&&(p_Floating.get_router_ID()=="")&&(p_Floating.get_fixed_IP()==INADDR_NONE))
                        {
                            return VerifyResult;
                        }
                        else
                        {
                            VerifyResult =Verify_router_ID(p_Floating);
                            if(VerifyResult == FLOATING_VERIFY_SUCCESS)
                            {
                                VerifyResult =Verify_port_ID(p_Floating);
                                if(VerifyResult == FLOATING_VERIFY_SUCCESS)
                                {
                                    VerifyResult =Verify_fixed_IP(p_Floating);
                                    if(VerifyResult == FLOATING_VERIFY_SUCCESS)
                                    {
                                        return VerifyResult;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
	else
	{
		return FLOATING_VERIFY_ERROR;
	}
    return VerifyResult;
}

UINT4  Verify_DeletedFloating   (Base_Floating& p_Floating)
{
	return FLOATING_VERIFY_SUCCESS;
}


UINT4  Verify_ID         (Base_Floating& p_Floating)
{
    if((p_Floating.get_ID())!="")
    {
        if(G_FloatingMgr.targetFloating_ByID(p_Floating.get_ID()))
        {
            return FLOATING_VERIFY_ID_ERROR_CONFLICT;
        }
        else
        {
            return FLOATING_VERIFY_ID_SUCCESS;
        }

    }
    else
    {
        return FLOATING_VERIFY_ID_ERROR_EMPTY;
    }
}

UINT4  Verify_name       (Base_Floating& p_Floating)
{
    return FLOATING_VERIFY_NAME_SUCCESS;
}

UINT4  Verify_tenant_ID  (Base_Floating& p_Floating)
{
    if((p_Floating.get_tenant_ID())=="")
    {
        return FLOATING_VERIFY_TENANT_ID_ERROR;
    }
    else
    {
        return FLOATING_VERIFY_TENANT_ID_SUCCESS;
    }

}

UINT4  Verify_network_ID (Base_Floating& p_Floating)
{
    if((p_Floating.get_network_ID())!="")
    {
        Base_Network * target_network =G_NetworkMgr.targetNetwork_ByID(p_Floating.get_network_ID());
        if(!target_network)
        {
            return FLOATING_VERIFY_NETWORK_ID_ERROR_NOTEXIST;
        }
        else
        {
            if(target_network->get_router_external())
            {
                return FLOATING_VERIFY_NETWORK_ID_SUCCESS;
            }
            else
            {
                return FLOATING_VERIFY_NETWORK_ID_ERROR_NOTEXTERNAL;
            }
        }
    }
    else
    {
         return FLOATING_VERIFY_NETWORK_ID_SUCCESS;
    }
}

UINT4  Verify_router_ID  (Base_Floating& p_Floating)
{
    if((p_Floating.get_router_ID())=="")
    {
        return FLOATING_VERIFY_ROUTER_ID_SUCCESS;
    }
    else
    {
        Base_Router * target_router =G_RouterMgr.targetRouter_ByID(p_Floating.get_router_ID());
        if(target_router)
        {
            if((target_router->get_tenant_ID())==(p_Floating.get_tenant_ID()))
            {
                if((target_router->get_external_gateway_info()).get_network_ID() == (p_Floating.get_network_ID()))
                {
                    return FLOATING_VERIFY_ROUTER_ID_SUCCESS;
                }
                else
                {
                    return FLOATING_VERIFY_ROUTER_ID_ERROR_NETWORKNOTMATCH;
                }
            }
            else
            {
                return FLOATING_VERIFY_ROUTER_ID_ERROR_TENANTNOTMATCH;
            }
        }
        else
        {
            return FLOATING_VERIFY_ROUTER_ID_ERROR_NOTEXIST;
        }
    }
}

UINT4  Verify_port_ID    (Base_Floating& p_Floating)
{
    if((p_Floating.get_router_ID())=="")
    {
        if((p_Floating.get_port_ID())=="")
        {
            return FLOATING_VERIFY_PORT_ID_SUCCESS;
        }
        else
        {
            return FLOATING_VERIFY_PORT_ID_ERROR_NOTNULL;
        }
    }
    else
    {
        if((p_Floating.get_port_ID())=="")
        {
            return FLOATING_VERIFY_PORT_ID_ERROR_NULL;
        }
        else
        {
            Base_Port * target_port = G_PortMgr.targetPort_ByID(p_Floating.get_port_ID());
            if(target_port)
            {
                if(target_port->get_device_owner() == DO_compute)
                {
                    return FLOATING_VERIFY_PORT_ID_SUCCESS;
                }
                else
                {
                    return FLOATING_VERIFY_PORT_ID_ERROR_PORTNOTCOMPUTE;
                }
            }
            else
            {
                return FLOATING_VERIFY_PORT_ID_ERROR_PORTNOTEXIST;
            }
        }
    }

}

UINT4  Verify_status     (Base_Floating& p_Floating)
{
    return FLOATING_VERIFY_STATUS_SUCCESS;
}

UINT4  Verify_fixed_IP   (Base_Floating& p_Floating)
{
    if(((p_Floating.get_router_ID())=="")&&((p_Floating.get_port_ID())==""))
    {
        if(p_Floating.get_fixed_IP()==INADDR_NONE)
        {
            return FLOATING_VERIFY_FIXED_IP_SUCCESS;
        }
        else
        {
            return FLOATING_VERIFY_FIXED_IP_ERROR_NOTNULL;
        }
    }
    else
    {
        if(p_Floating.get_fixed_IP()==INADDR_NONE)
        {
            return FLOATING_VERIFY_FIXED_IP_ERROR_NULL;
        }
        else
        {
            Base_Port * target_port = G_PortMgr.targetPort_ByID(p_Floating.get_port_ID());
            if(target_port)
            {
                if(RETURN_TRUE==target_port->search_IP(p_Floating.get_fixed_IP()))
                {
                    return FLOATING_VERIFY_FIXED_IP_SUCCESS;
                }
                else
                {
                    return FLOATING_VERIFY_FIXED_IP_ERROR_NOTMATCHPORT;
                }
            }
            else
            {
                return FLOATING_VERIFY_PORT_ID_ERROR_PORTNOTEXIST;
            }
        }
    }
}

UINT4  Verify_floating_IP(Base_Floating& p_Floating)
{
    if(p_Floating.get_floating_IP()==INADDR_NONE)
    {
        return FLOATING_VERIFY_FLOATING_IP_ERROR_NULL;
    }
    else
    {
        Base_Network * target_network =G_NetworkMgr.targetNetwork_ByID(p_Floating.get_network_ID());
        list<string> subnets ;
        string matchedSubnetID ="";
        target_network->get_subnets(subnets);
        list<string>::iterator itor;
        itor =subnets.begin();
        while(itor!= subnets.end())
        {
            Base_Subnet * target_subnet = G_SubnetMgr.targetSubnet_ByID(*(itor));
            if(target_subnet)
            {
                if(target_subnet->check_IPexisted_in_allocation_pools(p_Floating.get_floating_IP()))
                {
                    matchedSubnetID=(*itor);
                    break;
                }
                else
                {
                    return FLOATING_VERIFY_FLOATING_IP_ERROR_NOTINPOOL;
                }
            }
            itor++;
        }


        if(matchedSubnetID !="")
        {
            Base_Port * targetPort =G_PortMgr.targetPort_ByFixed_IP(p_Floating.get_floating_IP(),matchedSubnetID);
            if(targetPort)
            {
                if((targetPort->get_device_ID()==p_Floating.get_ID())&&(targetPort->get_device_owner()==DO_floating))
                {
                    return FLOATING_VERIFY_FLOATING_IP_SUCCESS;
                }
                else
                {
                    return FLOATING_VERIFY_FLOATING_IP_ERROR_CONFLICT;
                }
            }
            else
            {
                return FLOATING_VERIFY_FLOATING_IP_SUCCESS;
            }
        }
        else
        {
            return FLOATING_VERIFY_FLOATING_IP_ERROR_NOMATCHEDSUBNET;
        }
    }

    return FLOATING_VERIFY_FLOATING_IP_ERROR;
}


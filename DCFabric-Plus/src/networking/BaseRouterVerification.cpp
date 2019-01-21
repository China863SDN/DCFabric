#include "BaseRouterVerification.h"

UINT4 Verify_ID                 (Base_Router& p_Router);
UINT4 Verify_name               (Base_Router& p_Router);
UINT4 Verify_tenant_ID          (Base_Router& p_Router);
UINT4 Verify_network_ID         (Base_Router& p_Router);
UINT4 Verify_enable_snat        (Base_Router& p_Router);
UINT4 Verify_subnet_ID          (Base_Router& p_Router);
UINT4 Verify_IP                 (Base_Router& p_Router);

UINT4  Verify_CreatedRouter      (Base_Router& p_Router)
{
    UINT4 VerifyResult =Verify_ID                 (p_Router);
    if(VerifyResult==ROUTER_VERIFY_SUCCESS)
    {
        VerifyResult =Verify_name                 (p_Router);
        if(VerifyResult==ROUTER_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID                 (p_Router);
            if(VerifyResult==ROUTER_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_network_ID                 (p_Router);
                if(VerifyResult==ROUTER_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_enable_snat                 (p_Router);
                    if(VerifyResult==ROUTER_VERIFY_SUCCESS)
                    {
                        VerifyResult =Verify_subnet_ID                 (p_Router);
                        if(VerifyResult==ROUTER_VERIFY_SUCCESS)
                        {
                            VerifyResult =Verify_IP                 (p_Router);
                        }
                    }
                }
            }
        }
    }
    return VerifyResult;
}

UINT4  Verify_UpdatedRouter      (Base_Router& p_Router)
{
    UINT4 VerifyResult =Verify_ID                 (p_Router);
    if(VerifyResult==ROUTER_VERIFY_SUCCESS)
    {
        VerifyResult =Verify_name                 (p_Router);
        if(VerifyResult==ROUTER_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID                 (p_Router);
            if(VerifyResult==ROUTER_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_network_ID                 (p_Router);
                if(VerifyResult==ROUTER_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_enable_snat                 (p_Router);
                    if(VerifyResult==ROUTER_VERIFY_SUCCESS)
                    {
                        VerifyResult =Verify_subnet_ID                 (p_Router);
                        if(VerifyResult==ROUTER_VERIFY_SUCCESS)
                        {
                            VerifyResult =Verify_IP                 (p_Router);
                        }
                    }
                }
            }
        }
    }

    return VerifyResult;
}

UINT4  Verify_DeletedRouter      (Base_Router& p_Router)
{
    return ROUTER_VERIFY_SUCCESS;
}


UINT4  Verify_ID                 (Base_Router& p_Router)
{
    if(p_Router.get_ID()!="")
    {
        if(G_RouterMgr.targetRouter_ByID(p_Router.get_ID()))
        {
            return ROUTER_VERIFY_ID_ERROR_CONFLICT;
        }
        else
        {
            return ROUTER_VERIFY_ID_SUCCESS;
        }
    }
    else
    {
        return ROUTER_VERIFY_ID_ERROR_EMPTY;
    }
}

UINT4  Verify_name               (Base_Router& p_Router)
{
    return ROUTER_VERIFY_NAME_SUCCESS;
}

UINT4  Verify_tenant_ID          (Base_Router& p_Router)
{
    if(p_Router.get_tenant_ID()=="")
    {
        return ROUTER_VERIFY_TENANT_ID_ERROR;
    }
    else
    {
        return ROUTER_VERIFY_TENANT_ID_SUCCESS;
    }
}

UINT4  Verify_network_ID         (Base_Router& p_Router)
{
    RouterExternalGateway ExternalInfo =p_Router.get_external_gateway_info();
    if(ExternalInfo.get_network_ID()=="")
    {
        return ROUTER_VERIFY_NETWORK_ID_ERROR_EMPTY;
    }
    else
    {
        Base_Network * targetNetwork =G_NetworkMgr.targetNetwork_ByID(ExternalInfo.get_network_ID());
        if(targetNetwork)
        {
            if(targetNetwork->get_router_external()==BASE_NETWORK_ROUTEREXTERNAL)
            {
                return ROUTER_VERIFY_NETWORK_ID_SUCCESS;
            }
            else
            {
                return ROUTER_VERIFY_NETWORK_ID_ERROR_NOTEXTERNAL;
            }
        }
        else
        {
            return ROUTER_VERIFY_NETWORK_ID_ERROR_NOTEXIST;
        }
    }
}

UINT4  Verify_enable_snat        (Base_Router& p_Router)
{
    return ROUTER_VERIFY_ENABLE_SNAT_SUCCESS;
}

UINT4  Verify_subnet_ID          (Base_Router& p_Router)
{
    RouterExternalGateway ExternalInfo =p_Router.get_external_gateway_info();
    Fixed_IP IPinfo =ExternalInfo.get_external_IP();
    if(IPinfo.get_subnet_ID()=="")
    {
        return ROUTER_VERIFY_SUBNET_ID_ERROR_EMPTY;
    }
    else
    {
        Base_Subnet * targetSubnet = G_SubnetMgr.targetSubnet_ByID(IPinfo.get_subnet_ID());
        if(!(targetSubnet))
        {
            return ROUTER_VERIFY_SUBNET_ID_ERROR_NOTEXIST;
        }
        else
        {
            if(targetSubnet->get_network_ID()==ExternalInfo.get_network_ID())
            {
                return ROUTER_VERIFY_SUBNET_ID_SUCCESS;
            }
            else
            {
                return ROUTER_VERIFY_SUBNET_ID_ERROR_NETWORKNOTMATCH;
            }
        }
    }
}

UINT4  Verify_IP                 (Base_Router& p_Router)
{
    RouterExternalGateway ExternalInfo =p_Router.get_external_gateway_info();
    Fixed_IP IPinfo =ExternalInfo.get_external_IP();

    Base_Subnet* targetSubnet =G_SubnetMgr.targetSubnet_ByID (IPinfo.get_subnet_ID());
    if(targetSubnet)
    {
        if(targetSubnet->check_IPexisted_in_allocation_pools(IPinfo.get_IP())==RETURN_TRUE)
        {
            Base_Port * targetPort =G_PortMgr.targetPort_ByFixed_IP(IPinfo.get_IP(),IPinfo.get_subnet_ID());
            if(targetPort)
            {
                if((targetPort->get_device_ID()==p_Router.get_ID())&&(targetPort->get_device_owner()==DO_routerGateway))
                {
                    return ROUTER_VERIFY_IP_SUCCESS;
                }
                else
                {
                    return ROUTER_VERIFY_IP_ERROR_CONFLICT;
                }
            }
            else
            {
                return ROUTER_VERIFY_IP_SUCCESS;
            }
        }
        else
        {
            return ROUTER_VERIFY_IP_ERROR_IPNOTINSUBNET;
        }
    }
    else
    {
        return ROUTER_VERIFY_IP_ERROR;
    }
}







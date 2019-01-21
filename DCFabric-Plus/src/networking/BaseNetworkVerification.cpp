#include "BaseNetworkVerification.h"

UINT4 Verify_ID                 (Base_Network& p_Network);
UINT4 Verify_name               (Base_Network& p_Network);
UINT4 Verify_tenant_ID          (Base_Network& p_Network);
UINT4 Verify_router_external    (Base_Network& p_Network);
UINT4 Verify_shared             (Base_Network& p_Network);
UINT4 Verify_subnets            (Base_Network& p_Network);

UINT4  Verify_CreatedNetwork     (Base_Network& p_Network)
{
    UINT4 VerifyResult =Verify_ID                 (p_Network);
    if(VerifyResult==NETWORK_VERIFY_SUCCESS)
    {
        VerifyResult =Verify_name               (p_Network);
        if(VerifyResult==NETWORK_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID               (p_Network);
            if(VerifyResult==NETWORK_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_router_external               (p_Network);
                if(VerifyResult==NETWORK_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_shared               (p_Network);
                    if(VerifyResult==NETWORK_VERIFY_SUCCESS)
                    {
                        VerifyResult =Verify_subnets               (p_Network);
                        if(VerifyResult==NETWORK_VERIFY_SUCCESS)
                        {
                            return NETWORK_VERIFY_SUCCESS;
                        }
                    }
                }
            }
        }
    }
    return VerifyResult;
}

UINT4  Verify_UpdatedNetwork     (Base_Network& p_Network)
{
    UINT4 VerifyResult =Verify_ID                 (p_Network);
    if(VerifyResult==NETWORK_VERIFY_SUCCESS)
    {
        VerifyResult =Verify_name               (p_Network);
        if(VerifyResult==NETWORK_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID               (p_Network);
            if(VerifyResult==NETWORK_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_router_external               (p_Network);
                if(VerifyResult==NETWORK_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_shared               (p_Network);
                    if(VerifyResult==NETWORK_VERIFY_SUCCESS)
                    {
                        VerifyResult =Verify_subnets               (p_Network);
                        if(VerifyResult==NETWORK_VERIFY_SUCCESS)
                        {
                            return NETWORK_VERIFY_SUCCESS;
                        }
                    }
                }
            }
        }
    }
	else
	{
		return NETWORK_VERIFY_ERROR;
	}
    return VerifyResult;
}

UINT4  Verify_DeletedNetwork     (Base_Network& p_Network)
{
    return NETWORK_VERIFY_SUCCESS;
}

UINT4  Verify_ID                 (Base_Network& p_Network)
{
    if(p_Network.get_ID()!="")
    {
        if(G_NetworkMgr.targetNetwork_ByID(p_Network.get_ID()))
        {
            return NETWORK_VERIFY_ID_ERROR_CONFLICT;
        }
        else
        {
            return NETWORK_VERIFY_ID_SUCCESS;
        }
    }
    else
    {
        return NETWORK_VERIFY_ID_ERROR_EMPTY;
    }
}

UINT4  Verify_name               (Base_Network& p_Network)
{
    return NETWORK_VERIFY_NAME_SUCCESS;
}

UINT4  Verify_tenant_ID          (Base_Network& p_Network)
{
    if(p_Network.get_tenant_ID()=="")
    {
        return NETWORK_VERIFY_TENANT_ID_ERROR;
    }
    else
    {
        return NETWORK_VERIFY_TENANT_ID_SUCCESS;
    }
}

UINT4  Verify_router_external    (Base_Network& p_Network)
{
    return NETWORK_VERIFY_ROUTER_EXTERNAL_SUCCESS;
}

UINT4  Verify_shared             (Base_Network& p_Network)
{
    if(p_Network.get_router_external()==BASE_NETWORK_ROUTEREXTERNAL)
    {
        if(p_Network.get_shared()==BASE_NETWORK_SHARED)
        {
            return NETWORK_VERIFY_SHARED_SUCCESS;
        }
        else
        {
            return NETWORK_VERIFY_SHARED_ERROR;
        }
    }
    else
    {
        return NETWORK_VERIFY_SHARED_SUCCESS;
    }
}

UINT4  Verify_subnets            (Base_Network& p_Network)
{
    list<string> subnets;
    p_Network.get_subnets(subnets);
    if(subnets.empty())
    {
        return NETWORK_VERIFY_SUBNETS_SUCCESS;
    }
    else
    {
        list<string>::iterator subnet;
        subnet=subnets.begin();
        while(subnet!=subnets.end())
        {
            Base_Subnet * target_subnet = G_SubnetMgr.targetSubnet_ByID((*subnet));
            if(target_subnet)
            {
                if(target_subnet->get_network_ID() == p_Network.get_ID())
                {
                    return NETWORK_VERIFY_SUBNETS_SUCCESS;
                }
            }
            subnet++;
        }
        return NETWORK_VERIFY_SUBNETS_ERROR_SUBNETNOTRELATED;
    }
}

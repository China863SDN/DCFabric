#include "BasePortVerification.h"
#include "log.h"

UINT4 Verify_ID                 (Base_Port& p_Port);
UINT4 Verify_name               (Base_Port& p_Port);
UINT4 Verify_tenant_ID          (Base_Port& p_Port);
UINT4 Verify_device_owner       (Base_Port& p_Port);
UINT4 Verify_device_ID          (Base_Port& p_Port);
UINT4 Verify_network_ID         (Base_Port& p_Port);
UINT4 Verify_MAC                (Base_Port& p_Port);
UINT4 Verify_subnet_ID          (Base_Port& p_Port);
UINT4 Verify_IP                 (Base_Port& p_Port);

UINT4  Verify_CreatedPort        (Base_Port& p_Port)
{
    UINT4 VerifyResult =Verify_ID                   (p_Port);
    if(VerifyResult==PORT_VERIFY_SUCCESS)
    {
        VerifyResult =Verify_name                   (p_Port);
        if(VerifyResult==PORT_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID          (p_Port);
            if(VerifyResult==PORT_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_device_owner   (p_Port);
                if(VerifyResult==PORT_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_device_ID  (p_Port);
                    if(VerifyResult==PORT_VERIFY_SUCCESS)
                    {
                        VerifyResult =Verify_network_ID             (p_Port);
                        if(VerifyResult==PORT_VERIFY_SUCCESS)
                        {
                            VerifyResult =Verify_MAC                (p_Port);
                            if(VerifyResult==PORT_VERIFY_SUCCESS)
                            {
                                VerifyResult =Verify_subnet_ID      (p_Port);
                                if(VerifyResult==PORT_VERIFY_SUCCESS)
                                {
                                    VerifyResult =Verify_IP         (p_Port);
                                    if(VerifyResult==PORT_VERIFY_SUCCESS)
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

    if(VerifyResult!=PORT_VERIFY_SUCCESS)
        LOG_WARN_FMT(">>>> VerifyResult[%d]! <<<<", VerifyResult);

    return VerifyResult;
}

UINT4  Verify_UpdatedPort        (Base_Port& p_Port)
{
    UINT4 VerifyResult =Verify_ID                   (p_Port);
    if(VerifyResult==PORT_VERIFY_ID_ERROR_CONFLICT)
    {
        VerifyResult =Verify_name                   (p_Port);
        if(VerifyResult==PORT_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID          (p_Port);
            if(VerifyResult==PORT_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_device_owner   (p_Port);
                if(VerifyResult==PORT_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_device_ID  (p_Port);
                    if(VerifyResult==PORT_VERIFY_SUCCESS)
                    {
                        VerifyResult =Verify_network_ID             (p_Port);
                        if(VerifyResult==PORT_VERIFY_SUCCESS)
                        {
                            VerifyResult =Verify_MAC                (p_Port);
                            if(VerifyResult==PORT_VERIFY_SUCCESS)
                            {
                                VerifyResult =Verify_subnet_ID      (p_Port);
                                if(VerifyResult==PORT_VERIFY_SUCCESS)
                                {
                                    VerifyResult =Verify_IP         (p_Port);
                                    if(VerifyResult==PORT_VERIFY_SUCCESS)
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
		return PORT_VERIFY_ERROR;
	}
    return VerifyResult;
}

UINT4  Verify_DeletedPort        (Base_Port& p_Port)
{
    return PORT_VERIFY_SUCCESS;
}


UINT4  Verify_ID                 (Base_Port& p_Port)
{
    if(p_Port.get_ID()!="")
    {
        if(G_PortMgr.targetPort_ByID(p_Port.get_ID()))
        {
            return PORT_VERIFY_ID_ERROR_CONFLICT;
        }
        else
        {
            return PORT_VERIFY_ID_SUCCESS;
        }
    }
    else
    {
        return PORT_VERIFY_ID_ERROR_EMPTY;
    }
}

UINT4  Verify_name               (Base_Port& p_Port)
{
    return PORT_VERIFY_NAME_SUCCESS;
}

UINT4  Verify_tenant_ID          (Base_Port& p_Port)
{
    if(p_Port.get_tenant_ID()=="")
    {
        return PORT_VERIFY_TENANT_ID_ERROR;
    }
    else
    {
        return PORT_VERIFY_TENANT_ID_SUCCESS;
    }
}

UINT4  Verify_device_owner       (Base_Port& p_Port)
{
    if(p_Port.get_device_owner()=="")
    {
        return PORT_VERIFY_DEVICE_OWNER_ERROR;
    }
    else
    {
        return PORT_VERIFY_DEVICE_OWNER_SUCCESS;
    }
}

UINT4  Verify_device_ID          (Base_Port& p_Port)
{
    if((p_Port.get_device_owner()==DO_routerInterface)||(p_Port.get_device_owner()==DO_routerGateway))
    {
        Base_Router * targetRouter =G_RouterMgr.targetRouter_ByID(p_Port.get_device_ID());
        if(targetRouter)
        {
            if(targetRouter->get_tenant_ID()==p_Port.get_tenant_ID())
            {
                return PORT_VERIFY_DEVICE_ID_SUCCESS;
            }
            else
            {
               return PORT_VERIFY_DEVICE_ID_ERROR_ROUTERNOTMATCHTENANT;
            }
        }
        else
        {
            return PORT_VERIFY_DEVICE_ID_ERROR_ROUTERNOTEXIST;
        }
    }
    else if(p_Port.get_device_owner()==DO_floating)
    {
        Base_Floating * targetFloating =G_FloatingMgr.targetFloating_ByID(p_Port.get_device_ID());
        if(targetFloating)
        {
            return PORT_VERIFY_DEVICE_ID_SUCCESS;
        }
        else
        {
            return PORT_VERIFY_DEVICE_ID_ERROR_FLOATINGNOTEXIST;
        }
    }
    else
    {
        return PORT_VERIFY_DEVICE_ID_SUCCESS;
    }
}

UINT4  Verify_network_ID         (Base_Port& p_Port)
{
    if(p_Port.get_network_ID()!="")
    {
        Base_Network * targetNetwork = G_NetworkMgr.targetNetwork_ByID(p_Port.get_network_ID());
        if(targetNetwork)
        {
            if(targetNetwork->get_shared()==BASE_NETWORK_SHARED)
            {
                //Do nothing
            }
            else
            {
                if(targetNetwork->get_tenant_ID()==p_Port.get_tenant_ID())
                {
                    //Do nothing
                }
                else
                {
                    return PORT_VERIFY_NETWORK_ID_ERROR_TENANTNOTMATCH;
                }
            }
        }
        else
        {
            return PORT_VERIFY_NETWORK_ID_ERROR_NOTEXIST;
        }
    }
    else
    {
        return PORT_VERIFY_NETWORK_ID_ERROR_EMPTY;
    }



    if((p_Port.get_device_owner()==DO_routerGateway))
    {
        Base_Router * targetRouter =G_RouterMgr.targetRouter_ByID(p_Port.get_device_ID());
        if(targetRouter)
        {
            RouterExternalGateway externalInfo =targetRouter->get_external_gateway_info();
            if((externalInfo.get_network_ID()==p_Port.get_network_ID()))
            {
                return PORT_VERIFY_NETWORK_ID_SUCCESS;
            }
            else
            {
                return PORT_VERIFY_NETWORK_ID_ERROR_NOTMATCHROUTERNETWORK;
            }
        }
        else
        {
            return PORT_VERIFY_NETWORK_ID_ERROR_ROUTERNOTEXIST;
        }
    }
    else if(p_Port.get_device_owner()==DO_floating)
    {
        Base_Floating * targetFloating =G_FloatingMgr.targetFloating_ByID(p_Port.get_device_ID());
        if(targetFloating)
        {
            if(targetFloating->get_network_ID()==p_Port.get_network_ID())
            {
                return PORT_VERIFY_NETWORK_ID_SUCCESS;
            }
            else
            {
                return PORT_VERIFY_NETWORK_ID_ERROR_NOTMATCHFLOATINGNETWORK;
            }
        }
        else
        {
            return PORT_VERIFY_NETWORK_ID_ERROR_FLOATINGNOTEXIST;
        }
    }
    else
    {
        return PORT_VERIFY_NETWORK_ID_SUCCESS;
    }
}

UINT4  Verify_MAC                (Base_Port& p_Port)
{
    UINT1 *mac=p_Port.get_MAC();
    if((*mac==0x01)&&(*(mac+1)==0x80)&&(*(mac+2)==0xc2)&&(*(mac+3)==0x00))
    {
        return PORT_VERIFY_MAC_ERROR_RESERVEDMAC;
    }
    else if((*mac==0x03)&&(*(mac+1)==0x00)&&(*(mac+2)==0x00)&&(*(mac+3)==0x00))
    {
        return PORT_VERIFY_MAC_ERROR_RESERVEDMAC;
    }
    else if((*mac==0x09)&&(*(mac+1)==0x00)&&(*(mac+2)==0x2b)&&(*(mac+3)==0x00)&&(*(mac+4)==0x00))
    {
        return PORT_VERIFY_MAC_ERROR_RESERVEDMAC;
    }
    else if((*mac==0x00)&&(*(mac+1)==0x00)&&(*(mac+2)==0x00)&&(*(mac+3)==0x00)&&(*(mac+4)==0x00)&&(*(mac+5)==0x00))
    {
        return PORT_VERIFY_MAC_ERROR_RESERVEDMAC;
    }
    else if((*mac==0xFF)&&(*(mac+1)==0xFF)&&(*(mac+2)==0xFF)&&(*(mac+3)==0xFF)&&(*(mac+4)==0xFF)&&(*(mac+5)==0xFF))
    {
        return PORT_VERIFY_MAC_ERROR_RESERVEDMAC;
    }
    else
    {
        return PORT_VERIFY_MAC_SUCCESS;
    }
}

UINT4  Verify_subnet_ID          (Base_Port& p_Port)
{
    list<Fixed_IP*> IP_Infos ;
    p_Port.get_Fixed_IPs(IP_Infos);
    if(!(IP_Infos.empty()))
    {
        list<Fixed_IP*>::iterator IP_Info;
        IP_Info=IP_Infos.begin();
        while(IP_Info!=IP_Infos.end())
        {
            Base_Subnet* targetSubnet = G_SubnetMgr.targetSubnet_ByID((*IP_Info)->get_subnet_ID());
            if(targetSubnet)
            {
                if(targetSubnet->get_network_ID()==p_Port.get_network_ID())
                {
                    //do nothing
                }
                else
                {
                    return PORT_VERIFY_SUBNET_ID_ERROR_NETWORKNOTMATCH;
                }
            }
            else
            {
                return PORT_VERIFY_SUBNET_ID_ERROR_EMPTY;
            }
			IP_Info++;
        }

    }
    else
    {
        return PORT_VERIFY_FIXED_IP_ERROR_EMPTY;
    }

    if(p_Port.get_device_owner()!=DO_compute)
    {
        if(IP_Infos.size()>1)
        {
            return PORT_VERIFY_FIXED_IP_ERROR_TOOMUCHIPS;
        }
		else
		{
			return PORT_VERIFY_SUBNET_ID_SUCCESS;
		}
    }
    else
    {
        return PORT_VERIFY_SUBNET_ID_SUCCESS;
    }
	return PORT_VERIFY_SUBNET_ID_SUCCESS;
}

UINT4  Verify_IP                 (Base_Port& p_Port)
{
    if(p_Port.get_device_owner()==DO_routerInterface)
    {
        list<Fixed_IP*> IP_Infos ;
        p_Port.get_Fixed_IPs(IP_Infos);
        if(!(IP_Infos.empty()))
        {
            list<Fixed_IP*>::iterator IP_Info;
            IP_Info=IP_Infos.begin();
            while(IP_Info!=IP_Infos.end())
            {
                Base_Subnet* targetSubnet = G_SubnetMgr.targetSubnet_ByID((*IP_Info)->get_subnet_ID());
                if(targetSubnet)
                {
                    if(targetSubnet->get_gateway_IP()==(*IP_Info)->get_IP())
                    {
                        //DO NOTHING
                    }
                    else
                    {
                        return PORT_VERIFY_IP_ERROR_NOTMATCHGATEWAY;
                    }
                }
                else
                {
                    return PORT_VERIFY_IP_ERROR_SUBNETNOTEXIST;
                }
				IP_Info++;
            }
            return PORT_VERIFY_IP_SUCCESS;
        }
        return PORT_VERIFY_IP_ERROR;
    }
    else
    {
        list<Fixed_IP*> IP_Infos ;
        p_Port.get_Fixed_IPs(IP_Infos);
        if(!(IP_Infos.empty()))
        {
            list<Fixed_IP*>::iterator IP_Info;
            IP_Info=IP_Infos.begin();
            while(IP_Info!=IP_Infos.end())
            {
                Base_Subnet* targetSubnet = G_SubnetMgr.targetSubnet_ByID((*IP_Info)->get_subnet_ID());
                if(targetSubnet)
                {
                    if(targetSubnet->check_IPexisted_in_allocation_pools((*IP_Info)->get_IP())==RETURN_TRUE)
                    {
                        Base_Port * targetPort =G_PortMgr.targetPort_ByFixed_IP((*IP_Info)->get_IP(),(*IP_Info)->get_subnet_ID());
                        if(targetPort)
                        {
                            return PORT_VERIFY_IP_ERROR_CONFLICT;
                        }
                        else
                        {
                            //DO NOTHING
                        }
                    }
                    else
                    {
                        return PORT_VERIFY_IP_ERROR_IPNOTINSUBNET;
                    }
                }
                else
                {
                    return PORT_VERIFY_IP_ERROR_SUBNETNOTEXIST;
                }
				IP_Info++;
            }
            return PORT_VERIFY_IP_SUCCESS;
        }
    }
    return PORT_VERIFY_IP_ERROR;
}





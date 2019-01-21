#include "BaseSubnetVerification.h"

UINT4 Verify_ID                 (Base_Subnet& p_Subnet);
UINT4 Verify_name               (Base_Subnet& p_Subnet);
UINT4 Verify_tenant_ID          (Base_Subnet& p_Subnet);
UINT4 Verify_network_ID         (Base_Subnet& p_Subnet);
UINT4 Verify_cidr               (Base_Subnet& p_Subnet);
UINT4 Verify_gateway_IP         (Base_Subnet& p_Subnet);
UINT4 Verify_IP_pool            (Base_Subnet& p_Subnet);

UINT4 GetIPAndMask_by_cidr(const char* p_cidr, UINT4* p_ip, UINT1* p_mask)
{
	char cidr[48] = {0};
	memcpy(cidr, p_cidr, 48);
    * p_ip=INADDR_NONE;
    * p_mask=0;

	char* token = NULL;
	char* buf = cidr;
	INT4 count = 0;
	if(cidr[0]==0)
	{
		*p_ip=INADDR_NONE;
		*p_mask=0;
		return SUBNET_VERIFY_CIDR_ERROR;
	}
	while((token = strsep(&buf , "/")) != NULL)
	{
		if (0 == count)
		{
			*p_ip = inet_addr(token);
		}
		else
		{
			*p_mask = (0 == strcmp("0", token)) ? 0:atoll(token);
		}
		count++;
	}

	if((*p_ip!=INADDR_NONE)&&(*p_mask>0 && *p_mask<=32))
    {
        return SUBNET_VERIFY_CIDR_SUCCESS;
    }
    else
    {
        return SUBNET_VERIFY_CIDR_ERROR;
    }
}




UINT4 CheckIsIPMatchCIDR(UINT4 TargetIP,UINT4 CIDR_IP,UINT1 CIDR_Mask)
{
	UINT4 Mask =0;
	UINT1 i=32;
	UINT1 j=CIDR_Mask;
	for(i=32;i>0;i--)
	{
		if(j>0)
		{
			Mask = (Mask|0x0001);
			j--;
		}
		if(i>1)
        {
            Mask =(Mask <<1);
        }
	}

	if((ntohl(TargetIP) & Mask )==(ntohl(CIDR_IP) & Mask ))
	{
		return 1;
	}
	return 0;
}




UINT4  Verify_CreatedSubnet      (Base_Subnet& p_Subnet)
{
    UINT4 VerifyResult =Verify_ID                 (p_Subnet);
    if(VerifyResult==SUBNET_VERIFY_SUCCESS)
    {
        VerifyResult =Verify_name                 (p_Subnet);
        if(VerifyResult==SUBNET_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID                 (p_Subnet);
            if(VerifyResult==SUBNET_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_network_ID                 (p_Subnet);
                if(VerifyResult==SUBNET_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_cidr                 (p_Subnet);
                    if(VerifyResult==SUBNET_VERIFY_SUCCESS)
                    {
                        VerifyResult =Verify_gateway_IP                 (p_Subnet);
                        if(VerifyResult==SUBNET_VERIFY_SUCCESS)
                        {
                            VerifyResult =Verify_IP_pool                 (p_Subnet);
                        }
                    }
                }
            }
        }
    }
    return VerifyResult;
}

UINT4  Verify_UpdatedSubnet      (Base_Subnet& p_Subnet)
{
    UINT4 VerifyResult =Verify_ID                 (p_Subnet);
    if(VerifyResult==SUBNET_VERIFY_SUCCESS)
    {
        VerifyResult =Verify_name                 (p_Subnet);
        if(VerifyResult==SUBNET_VERIFY_SUCCESS)
        {
            VerifyResult =Verify_tenant_ID                 (p_Subnet);
            if(VerifyResult==SUBNET_VERIFY_SUCCESS)
            {
                VerifyResult =Verify_network_ID                 (p_Subnet);
                if(VerifyResult==SUBNET_VERIFY_SUCCESS)
                {
                    VerifyResult =Verify_cidr                 (p_Subnet);
                    if(VerifyResult==SUBNET_VERIFY_SUCCESS)
                    {
                        VerifyResult =Verify_gateway_IP                 (p_Subnet);
                        if(VerifyResult==SUBNET_VERIFY_SUCCESS)
                        {
                            VerifyResult =Verify_IP_pool                 (p_Subnet);
                        }
                    }
                }
            }
        }
    }

    return VerifyResult;
}

UINT4  Verify_DeletedSubnet      (Base_Subnet& p_Subnet)
{
    return SUBNET_VERIFY_SUCCESS;
}


UINT4  Verify_ID                 (Base_Subnet& p_Subnet)
{
    if(p_Subnet.get_ID()!="")
    {
        if(G_SubnetMgr.targetSubnet_ByID(p_Subnet.get_ID()))
        {
            return SUBNET_VERIFY_ID_ERROR_CONFLICT;
        }
        else
        {
            return SUBNET_VERIFY_ID_SUCCESS;
        }
    }
    else
    {
        return SUBNET_VERIFY_ID_ERROR_EMPTY;
    }
}

UINT4  Verify_name               (Base_Subnet& p_Subnet)
{
    return SUBNET_VERIFY_NAME_SUCCESS;
}

UINT4  Verify_tenant_ID          (Base_Subnet& p_Subnet)
{
    if(p_Subnet.get_tenant_ID()=="")
    {
        return SUBNET_VERIFY_TENANT_ID_ERROR;
    }
    else
    {
        return SUBNET_VERIFY_TENANT_ID_SUCCESS;
    }
}

UINT4  Verify_network_ID         (Base_Subnet& p_Subnet)
{
    if(p_Subnet.get_network_ID()!="")
    {
        Base_Network * target_network =G_NetworkMgr.targetNetwork_ByID(p_Subnet.get_network_ID());
        if(target_network)
        {
            if(target_network->get_shared())
            {
                return SUBNET_VERIFY_NETWORK_ID_SUCCESS;
            }
            else
            {
                if(target_network->get_tenant_ID()==p_Subnet.get_tenant_ID())
                {
                    return SUBNET_VERIFY_NETWORK_ID_SUCCESS;
                }
                else
                {
                    return SUBNET_VERIFY_NETWORK_ID_ERROR_TENANTNOTMATCH;
                }
            }
        }
        else
        {
            return SUBNET_VERIFY_NETWORK_ID_ERROR_NOTEXIST;
        }
    }
    else
    {
        return SUBNET_VERIFY_NETWORK_ID_ERROR_EMPTY;
    }
}

UINT4  Verify_cidr               (Base_Subnet& p_Subnet)
{
    string cidr=p_Subnet.get_cidr();
    UINT4 IP;
    UINT1 MASK;
    return GetIPAndMask_by_cidr(cidr.c_str(),&IP,&MASK);
}

UINT4  Verify_gateway_IP         (Base_Subnet& p_Subnet)
{
    string cidr=p_Subnet.get_cidr();
    UINT4 CIDR_IP;
    UINT1 CIDR_MASK;
    GetIPAndMask_by_cidr(cidr.c_str(),&CIDR_IP,&CIDR_MASK);
    if(CheckIsIPMatchCIDR(p_Subnet.get_gateway_IP(),CIDR_IP,CIDR_MASK))
    {
        return SUBNET_VERIFY_GATEWAY_IP_SUCCESS;
    }
    else
    {
        return SUBNET_VERIFY_GATEWAY_IP_ERROR_NOTMATCHCIDR;
    }

}

UINT4  Verify_IP_pool           (Base_Subnet& p_Subnet)
{
    string cidr=p_Subnet.get_cidr();
    UINT4 CIDR_IP;
    UINT1 CIDR_MASK;
    GetIPAndMask_by_cidr(cidr.c_str(),&CIDR_IP,&CIDR_MASK);

    list<IP_pool *> pools;
    p_Subnet.get_allocation_pools(pools);
    if(pools.empty())
    {
        return SUBNET_VERIFY_IP_POOL_ERROR_EMPTY;
    }
    else
    {
        list<IP_pool *>::iterator pool=pools.begin();
        while(pool!=pools.end())
        {
            UINT4 IP_start =(*pool)->get_IP_start();
            UINT4 IP_end =(*pool)->get_IP_end();
            if((CheckIsIPMatchCIDR(IP_start,CIDR_IP,CIDR_MASK))&&(CheckIsIPMatchCIDR(IP_start,CIDR_IP,CIDR_MASK)))
            {
                if(IP_start<IP_end)
                {
                    if((IP_start<=p_Subnet.get_gateway_IP())&&(p_Subnet.get_gateway_IP()<=IP_end))
                    {
                        return SUBNET_VERIFY_IP_POOL_ERROR_GATEWAYINPOOL;
                    }
                }
                else
                {
                    return SUBNET_VERIFY_IP_POOL_ERROR_STARTENDNOTMATCH;
                }
            }
            else
            {
                return SUBNET_VERIFY_IP_POOL_ERROR_NOTMATCHCIDR;
            }
            pool++;
        }
        return SUBNET_VERIFY_IP_POOL_SUCCESS;
    }
}




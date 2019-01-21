#include "BaseSubnetConventor.h"

Base_Subnet * BaseSubnetConventor_fromCOpenstackSubnet(COpenstackSubnet* subnet)
{
	string ID=subnet->getId();
    string name="";
    string tenant_ID=subnet->getTenantId();
    string network_ID=subnet->getNetworkId();
    string cidr=subnet->getCidr();
    UINT4 gateway_IP=ip2number(subnet->getGatewayIp().c_str());
    UINT1 status=BASE_SUBNET_STATUS_ACTIVE;
	
    UINT4 IP_start=ip2number(subnet->getStart().c_str());
    UINT4 IP_end=ip2number(subnet->getEnd().c_str());
	
	Base_Subnet* CreatedSubnet =new Base_Subnet(ID,name,tenant_ID,network_ID,cidr,gateway_IP,IP_start,IP_end);
	CreatedSubnet->change_status(status);

	return CreatedSubnet;
}

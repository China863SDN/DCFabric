#include "BaseFloatingConventor.h"

Base_Floating * BaseFloatingConventor_fromCOpenstackFloatingip(const COpenstackFloatingip* floatingip)
{
	string ID			=floatingip->getId();
	string name			="";
	string tenant_ID	=floatingip->getTenantId();
	string network_ID	=floatingip->getFloatingNetId();
	string router_ID	=floatingip->getRouterId();
	string port_ID		=floatingip->getPortId();
	UINT1 status		=0;//floatingip->getStatus();
	UINT4 fixed_IP		=ip2number(floatingip->getFixedIp().c_str());
	UINT4 floating_IP	=ip2number(floatingip->getFloatingIp().c_str());
	
	Base_Floating *CreatedFloating =new Base_Floating(ID,name,tenant_ID,network_ID,floating_IP);
	CreatedFloating->set_status(status);
	CreatedFloating->set_port_ID(port_ID);
	CreatedFloating->set_router_ID(router_ID);
	CreatedFloating->set_fixed_IP(fixed_IP);
	
	return CreatedFloating;
}


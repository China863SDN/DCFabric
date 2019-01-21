#include "BaseNetworkConventor.h"
Base_Network * BaseNetworkConventor_fromCOpenstackNetwork(COpenstackNetwork* network)
{
	string 	ID				=network->getId();
	string 	name			="";
	string 	tenant_ID		=network->getTenantId();
	UINT1 	status			=BASE_NETWORK_STATUS_ACTIVE;
	UINT1 	router_external	=(network->getRouterExternal())?BASE_NETWORK_ROUTEREXTERNAL:BASE_NETWORK_NOT_ROUTEREXTERNAL;
	UINT1 	shared			=(network->getShared())?BASE_NETWORK_SHARED:BASE_NETWORK_NOT_SHARED;
	list<string> subnets 	=network->getSubnetsList();

	Base_Network* CreatedNetwork =new Base_Network(ID,name,tenant_ID,status,router_external,shared);
	list<string>::iterator subnet =subnets.begin();
	while(subnet!= subnets.end())
	{
		CreatedNetwork->add_subnet(*subnet);
		subnet++;
	}
	return CreatedNetwork;
}

#include "COpenstackSubnet.h"
#include "CNotifyNetwork.h"
#include "CNetwork.h"
#include "CNetworkMgr.h"
#include "BaseNetwork.h"
#include "BaseNetworkManager.h"

CNotifyNetwork::CNotifyNetwork()
{
	
}

CNotifyNetwork::~CNotifyNetwork()
{
	
}


void CNotifyNetwork::notifyAddNetwork(COpenstackNetwork * network)
{
	if(NULL == network)
		return;
	CNetwork*  networkNode = new CNetwork(network->getId(), network->getTenantId(), network->getRouterExternal(), network->getShared());  //std::string networkid, std::string tenantid, BOOL bexternal , BOOL bshared
	CNetworkMgr::getInstance()->addNetworkNode(networkNode);
}

void CNotifyNetwork::notifyDelNetwork(const  string & network_id)
{
	CNetworkMgr::getInstance()->delNetworkNode( network_id);
}

void CNotifyNetwork::notifyUpdateNetwork(COpenstackNetwork * network)
{
	if(NULL == network)
		return;
	CNetwork*  networkNode = CNetworkMgr::getInstance()->findNetworkById( network->getId());
	if(NULL != networkNode)
	{
		networkNode->set_tenantid( network->getTenantId());
		networkNode->set_external(network->getRouterExternal());
		networkNode->set_shared(network->getShared());
	}
}
void CNotifyNetwork::notifyAddNetwork(Base_Network* network)
{
	if(NULL == network)
		return;
	CNetwork*  networkNode = new CNetwork(network->get_ID(), network->get_tenant_ID(), network->get_router_external(), network->get_shared());  //std::string networkid, std::string tenantid, BOOL bexternal , BOOL bshared
	CNetworkMgr::getInstance()->addNetworkNode(networkNode);
}

void CNotifyNetwork::notifyUpdateNetwork(Base_Network* network)
{
	if(NULL == network)
		return;
	CNetwork*  networkNode = CNetworkMgr::getInstance()->findNetworkById( network->get_ID());
	if(NULL != networkNode)
	{
		networkNode->set_tenantid( network->get_tenant_ID());
		networkNode->set_external(network->get_router_external());
		networkNode->set_shared(network->get_shared());
	}
}




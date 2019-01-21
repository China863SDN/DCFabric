#include "comm-util.h"
#include "COpenstackSubnet.h"
#include "CNotifySubnet.h"
#include "CSubnet.h"
#include "CSubnetMgr.h"
#include "BaseSubnet.h"
#include "BaseSubnetManager.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "BaseNetwork.h"
#include "BaseNetworkManager.h"

CNotifySubnet::CNotifySubnet()
{
}

CNotifySubnet::~CNotifySubnet()
{
}

void CNotifySubnet::notifyAddSubnet(COpenstackSubnet * subnet)
{
	if(NULL == subnet)
		return ;
	CSubnet*  subnetNode = new CSubnet(subnet->getNetworkId(), subnet->getId(), 0, ip2number(subnet->getGatewayIp().c_str()));  //std::string networkid, std::string tenantid, BOOL bexternal , BOOL bshared
	subnetNode->set_subnetIpRange(ip2number(subnet->getStart().c_str()), ip2number(subnet->getEnd().c_str()));
	subnetNode->set_tenantid(subnet->getTenantId());
	subnetNode->set_cidr(subnet->getCidr());
	CSubnetMgr::getInstance()->addSubnetNode(subnetNode); 

	
	Base_External* CreatedExternal =new Base_External(subnet->getNetworkId(),subnet->getId(),ip2number(subnet->getGatewayIp().c_str()));
	G_ExternalMgr.insertNode_ByExternal(CreatedExternal);
}

void CNotifySubnet::notifyDelSubnet(const  string & subnet_id)
{
	CSubnetMgr::getInstance()->delSubnetNode( subnet_id);
}

void CNotifySubnet::notifyUpdateSubnet(COpenstackSubnet * subnet)
{
	if(NULL == subnet)
		return ;
	CSubnet*  subnetNode = CSubnetMgr::getInstance()->findSubnetById( subnet->getId());
	if(NULL != subnetNode)
	{
		subnetNode->set_networkid( subnet->getNetworkId());
		subnetNode->set_tenantid(subnet->getTenantId());
		subnetNode->set_cidr(subnet->getCidr());
		subnetNode->set_subnetIpRange(ip2number(subnet->getStart().c_str()), ip2number(subnet->getEnd().c_str()));
		//subnetNode->set_subnetGateway(subnet->getGatewayIp());
		//subnetNode->set_subnetDhcp(subnet->get);
	}
}

void CNotifySubnet::notifyAddSubnet(Base_Subnet* subnet)
{
	if(NULL == subnet)
		return ;
	if(""== subnet->get_network_ID())
	{
		Base_Network* network_node = G_NetworkMgr.targetNetwork_Bysubnet_ID(subnet->get_ID());
		if(NULL != network_node)
		{
			subnet->set_network_id(network_node->get_ID());
		}
	}
	CSubnet*  subnetNode = new CSubnet(subnet->get_network_ID(), subnet->get_ID(), 0, subnet->get_gateway_IP());	//std::string networkid, std::string tenantid, BOOL bexternal , BOOL bshared
	if(NULL != subnetNode)
	{
		subnetNode->set_tenantid(subnet->get_tenant_ID());
		subnetNode->set_cidr(subnet->get_cidr());
		subnetNode->set_subnetIpRange(subnet->getFirstStartIp(), subnet->getFirstEndIp());
		CSubnetMgr::getInstance()->addSubnetNode(subnetNode); 
	}

	
	
}

void CNotifySubnet::notifyUpdateSubnet(Base_Subnet* subnet)
{
	if(NULL == subnet)
		return ;
	CSubnet*  subnetNode = CSubnetMgr::getInstance()->findSubnetById( subnet->get_ID());
	if(NULL != subnetNode)
	{
		subnetNode->set_networkid( subnet->get_network_ID());
		subnetNode->set_tenantid(subnet->get_tenant_ID());
		subnetNode->set_cidr(subnet->get_cidr());
		subnetNode->set_subnetIpRange(subnet->getFirstStartIp(), subnet->getFirstEndIp());	
		//subnetNode->set_subnetGateway(subnet->getGatewayIp());
		//subnetNode->set_subnetDhcp(subnet->get);
	}
}




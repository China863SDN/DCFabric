#ifndef BASESUBNETMGR_H
#define BASESUBNETMGR_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseSubnet.h"

#define RETURN_OK               1
#define RETURN_ERR              0

#define RETURN_TRUE             1
#define RETURN_FALSE    0

using namespace std;

//======================================================================
//by:yhy 考虑换成单例
//======================================================================

class BaseSubnetMgr
{
public:
	BaseSubnetMgr();
	~BaseSubnetMgr();

	Base_Subnet* targetSubnet_ByID (const string p_ID);

	UINT1 targetSubnet_Byname                   (const string   p_name,list<Base_Subnet*>&p_subnets);
	UINT1 targetSubnet_Bytenant_ID              (const string   p_tenant_ID,list<Base_Subnet*>&p_subnets);
	UINT1 targetSubnet_Bynetwork_ID     (const string   p_network_ID,list<Base_Subnet*>&p_subnets);
	UINT1 targetSubnet_Bystatus         (const UINT1    p_status,list<Base_Subnet*>&p_subnets);
	UINT1 targetSubnet_Bygateway_IP     (const UINT4    p_gateway_IP,list<Base_Subnet*>&p_subnets);
	UINT1 targetSubnet_Bycidr                   (const string   p_cidr,list<Base_Subnet*>&p_subnets);

	UINT1 listSubnets   (list<Base_Subnet*>&p_subnets);
	UINT1 defineSubnet_ByIP				(const UINT4    p_IP,string& p_SubnetID);
	
	UINT1 insertNode_BySubnet       (Base_Subnet* p_network);
	UINT1 deleteNode_BySubnet       (Base_Subnet* p_network);
	UINT1 deleteNode_BySubnetID     (const string p_ID);

private:
	void lock_Mutex(void);
	void unlock_Mutex(void);

private:
	pthread_mutex_t BaseSubnetMgrMutex;
	list<Base_Subnet*> subnets;
};



extern BaseSubnetMgr G_SubnetMgr;

#endif // BASESUBNETMGR_H

#ifndef BASESUBNETVERIFICATION_H
#define BASESUBNETVERIFICATION_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseSubnet.h"
#include "BaseSubnetManager.h"
#include "BaseNetwork.h"
#include "BaseNetworkManager.h"


#define RETURN_OK               1
#define RETURN_ERR              0

#define RETURN_TRUE             1
#define RETURN_FALSE    0

using namespace std;

#define SUBNET_VERIFY_SUCCESS               0x0000
#define SUBNET_VERIFY_ERROR                 0x0001

#define SUBNET_VERIFY_CREATED_SUCCESS       0x0000
#define SUBNET_VERIFY_UPDATED_SUCCESS       0x0000
#define SUBNET_VERIFY_DELETED_SUCCESS       0x0000
#define SUBNET_VERIFY_CREATED_ERROR         0x0001
#define SUBNET_VERIFY_UPDATED_ERROR         0x0001
#define SUBNET_VERIFY_DELETED_ERROR         0x0001

#define SUBNET_VERIFY_ID_SUCCESS              		0x0000
#define SUBNET_VERIFY_ID_ERROR             			0x0511
#define SUBNET_VERIFY_ID_ERROR_EMPTY            	0x0512
#define SUBNET_VERIFY_ID_ERROR_CONFLICT            	0x0513

#define SUBNET_VERIFY_NAME_SUCCESS            	0x0000
#define SUBNET_VERIFY_NAME_ERROR            	0x0521

#define SUBNET_VERIFY_TENANT_ID_SUCCESS       	0x0000
#define SUBNET_VERIFY_TENANT_ID_ERROR       	0x0531

#define SUBNET_VERIFY_NETWORK_ID_SUCCESS 				0x0000
#define SUBNET_VERIFY_NETWORK_ID_ERROR 					0x0541
#define SUBNET_VERIFY_NETWORK_ID_ERROR_EMPTY 			0x0542
#define SUBNET_VERIFY_NETWORK_ID_ERROR_TENANTNOTMATCH 	0x0543
#define SUBNET_VERIFY_NETWORK_ID_ERROR_NOTEXIST 		0x0544

#define SUBNET_VERIFY_CIDR_SUCCESS       0x0000
#define SUBNET_VERIFY_CIDR_ERROR         0x0551

#define SUBNET_VERIFY_GATEWAY_IP_SUCCESS         			0x0000
#define SUBNET_VERIFY_GATEWAY_IP_ERROR         				0x0561
#define SUBNET_VERIFY_GATEWAY_IP_ERROR_NOTMATCHCIDR         0x0562


#define SUBNET_VERIFY_IP_POOL_SUCCESS         				0x0000
#define SUBNET_VERIFY_IP_POOL_ERROR         				0x0571
#define SUBNET_VERIFY_IP_POOL_ERROR_EMPTY         			0x0572
#define SUBNET_VERIFY_IP_POOL_ERROR_NOTMATCHCIDR         	0x0573
#define SUBNET_VERIFY_IP_POOL_ERROR_STARTENDNOTMATCH      	0x0574
#define SUBNET_VERIFY_IP_POOL_ERROR_GATEWAYINPOOL         	0x0575



        UINT4 Verify_CreatedSubnet      (Base_Subnet& p_Subnet);
        UINT4 Verify_UpdatedSubnet      (Base_Subnet& p_Subnet);
        UINT4 Verify_DeletedSubnet      (Base_Subnet& p_Subnet);


#endif // BASESUBNETVERIFICATION_H

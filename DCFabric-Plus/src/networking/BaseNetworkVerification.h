#ifndef BASENETWORKVERIFICATION_H
#define BASENETWORKVERIFICATION_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseNetwork.h"
#include "BaseNetworkManager.h"
#include "BaseSubnet.h"
#include "BaseSubnetManager.h"

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE     1
#define RETURN_FALSE    0

#define MAX_NETWORK_COUNT       100

using namespace std;

#define NETWORK_VERIFY_ERROR                 	0x0001
#define NETWORK_VERIFY_SUCCESS                 	0x0000
#define NETWORK_VERIFY_CREATED_SUCCESS                 	0x0000
#define NETWORK_VERIFY_UPDATED_SUCCESS                 	0x0000

#define NETWORK_VERIFY_ID_SUCCESS              	0x0000
#define NETWORK_VERIFY_ID_ERROR             	0x0211
#define NETWORK_VERIFY_ID_ERROR_EMPTY           0x0212
#define NETWORK_VERIFY_ID_ERROR_CONFLICT        0x0213

#define NETWORK_VERIFY_NAME_SUCCESS            	0x0000
#define NETWORK_VERIFY_NAME_ERROR            	0x0221

#define NETWORK_VERIFY_TENANT_ID_SUCCESS       	0x0000
#define NETWORK_VERIFY_TENANT_ID_ERROR       	0x0231

#define NETWORK_VERIFY_ROUTER_EXTERNAL_SUCCESS 	0x0000
#define NETWORK_VERIFY_ROUTER_EXTERNAL_ERROR 	0x0241

#define NETWORK_VERIFY_SHARED_SUCCESS          	0x0000
#define NETWORK_VERIFY_SHARED_ERROR         	0x0251

#define NETWORK_VERIFY_SUBNETS_SUCCESS         					0x0000
#define NETWORK_VERIFY_SUBNETS_ERROR         					0x0261
#define NETWORK_VERIFY_SUBNETS_ERROR_SUBNETNOTRELATED        	0x0262


        UINT4 Verify_CreatedNetwork     (Base_Network& p_Network);
        UINT4 Verify_UpdatedNetwork     (Base_Network& p_Network);
        UINT4 Verify_DeletedNetwork     (Base_Network& p_Network);


#endif // BASENETWORKVERIFICATION_H

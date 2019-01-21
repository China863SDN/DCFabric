#ifndef BASEROUTERVERIFICATION_H
#define BASEROUTERVERIFICATION_H

#include "comm-util.h"
#include <string>
#include <vector>
#include <list>

#include "BaseRouter.h"
#include "BaseRouterManager.h"
#include "BaseNetwork.h"
#include "BaseNetworkManager.h"
#include "BaseSubnet.h"
#include "BaseSubnetManager.h"
#include "BasePort.h"
#include "BasePortManager.h"

#define RETURN_OK       1
#define RETURN_ERR      0

#define RETURN_TRUE         1
#define RETURN_FALSE    0

using namespace std;


#define ROUTER_VERIFY_SUCCESS        		0x0000
#define ROUTER_VERIFY_ERROR          		0x0001

#define ROUTER_VERIFY_CREATED_SUCCESS         0x0000
#define ROUTER_VERIFY_UPDATED_SUCCESS         0x0000
#define ROUTER_VERIFY_DELETED_SUCCESS         0x0000
#define ROUTER_VERIFY_CREATED_ERROR         0x0000
#define ROUTER_VERIFY_UPDATED_ERROR         0x0000
#define ROUTER_VERIFY_DELETED_ERROR         0x0000

#define ROUTER_VERIFY_ID_SUCCESS        	0x0000
#define ROUTER_VERIFY_ID_ERROR          	0x0411
#define ROUTER_VERIFY_ID_ERROR_EMPTY     	0x0412
#define ROUTER_VERIFY_ID_ERROR_CONFLICT     0x0413

#define ROUTER_VERIFY_NAME_SUCCESS  0x0000
#define ROUTER_VERIFY_NAME_ERROR  	0x0421

#define ROUTER_VERIFY_TENANT_ID_SUCCESS  		0x0000
#define ROUTER_VERIFY_TENANT_ID_ERROR  			0x0431
#define ROUTER_VERIFY_TENANT_ID_ERROR_EMPTY  	0x0432


#define ROUTER_VERIFY_NETWORK_ID_SUCCESS  				0x0000
#define ROUTER_VERIFY_NETWORK_ID_ERROR  				0x0441
#define ROUTER_VERIFY_NETWORK_ID_ERROR_EMPTY  			0x0442
#define ROUTER_VERIFY_NETWORK_ID_ERROR_NOTEXIST  		0x0443
#define ROUTER_VERIFY_NETWORK_ID_ERROR_NOTEXTERNAL  	0x0444


#define ROUTER_VERIFY_ENABLE_SNAT_SUCCESS  	0x0000
#define ROUTER_VERIFY_ENABLE_SNAT_ERROR  	0x0451

#define ROUTER_VERIFY_SUBNET_ID_SUCCESS  				0x0000
#define ROUTER_VERIFY_SUBNET_ID_ERROR  					0x0461
#define ROUTER_VERIFY_SUBNET_ID_ERROR_EMPTY  			0x0462
#define ROUTER_VERIFY_SUBNET_ID_ERROR_NOTEXIST 			0x0463
#define ROUTER_VERIFY_SUBNET_ID_ERROR_NETWORKNOTMATCH  	0x0464

#define ROUTER_VERIFY_IP_SUCCESS  				0x0000
#define ROUTER_VERIFY_IP_ERROR  				0x0471
#define ROUTER_VERIFY_IP_ERROR_CONFLICT  		0x0472
#define ROUTER_VERIFY_IP_ERROR_IPNOTINSUBNET  	0x0473

    UINT4 Verify_CreatedRouter      (Base_Router& p_Router);
        UINT4 Verify_UpdatedRouter      (Base_Router& p_Router);
        UINT4 Verify_DeletedRouter      (Base_Router& p_Router);


#endif // BASEROUTERVERIFICATION_H

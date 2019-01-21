#ifndef SERVICE_NETWORKINGCONFIG_BYRESTFUL_H
#define SERVICE_NETWORKINGCONFIG_BYRESTFUL_H

#include "CRestApi.h"

class CNetworkingRestApi
{
public:
    static void POST_C_Floating(CRestRequest* p_request, CRestResponse* p_response);
    static void GET_R_Floating(CRestRequest* p_request, CRestResponse* p_response);
    static void PUT_U_Floating(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Floating(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Floatings(CRestRequest* p_request, CRestResponse* p_response);

    static void POST_C_Network(CRestRequest* p_request, CRestResponse* p_response);
    static void GET_R_Network(CRestRequest* p_request, CRestResponse* p_response);
    static void PUT_U_Network(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Network(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Networks(CRestRequest* p_request, CRestResponse* p_response);

    static void POST_C_Router(CRestRequest* p_request, CRestResponse* p_response);
    static void GET_R_Router(CRestRequest* p_request, CRestResponse* p_response);
    static void PUT_U_Router(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Router(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Routers(CRestRequest* p_request, CRestResponse* p_response);

    static void POST_C_Subnet(CRestRequest* p_request, CRestResponse* p_response);
    static void GET_R_Subnet(CRestRequest* p_request, CRestResponse* p_response);
    static void PUT_U_Subnet(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Subnet(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Subnets(CRestRequest* p_request, CRestResponse* p_response);

    static void POST_C_Port(CRestRequest* p_request, CRestResponse* p_response);
    static void GET_R_Port(CRestRequest* p_request, CRestResponse* p_response);
    static void PUT_U_Port(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Port(CRestRequest* p_request, CRestResponse* p_response);
    static void DELETE_D_Ports(CRestRequest* p_request, CRestResponse* p_response);
};

#endif // SERVICE_NETWORKINGCONFIG_BYRESTFUL_H

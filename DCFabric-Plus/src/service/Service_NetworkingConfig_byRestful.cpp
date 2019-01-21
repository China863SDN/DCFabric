#include "Service_NetworkingConfig_byRestful.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "comm-util.h"
#include "log.h"

#include "BaseFloatingVerification.h"
#include "BasePortVerification.h"
#include "BaseNetworkVerification.h"
#include "BaseSubnetVerification.h"
#include "BaseRouterVerification.h"
#include "BaseSubnetManager.h"
#include "BaseFloatingManager.h"
#include "BaseNetworkManager.h"
#include "BaseRouterManager.h"
#include "NetworkingEventReporter.h"

#include <arpa/inet.h>
#include <iostream>

using namespace rapidjson;
using namespace std;

UINT1 MACStringtoArray(string&p_StringMAC,UINT1 p_MACArray[])
{
    UINT4 StartPos =0;
    UINT4 TargetPos =0;
    string MACMember ="";
    UINT1 MACMember_UINT1 =0;
    UINT1 MACArrayIndex=0;
    while(StartPos<p_StringMAC.size())
    {
        TargetPos=p_StringMAC.find(':',StartPos);
        if(TargetPos)
        {
            MACMember =p_StringMAC.substr(StartPos,TargetPos-StartPos);
            MACMember_UINT1=(UINT1)strtoul(MACMember.c_str(),NULL,16);
            p_MACArray[MACArrayIndex]=MACMember_UINT1;
            StartPos=TargetPos+1;
            TargetPos=0;
            MACArrayIndex ++;

            if(MACArrayIndex==5)
            {
                MACMember =p_StringMAC.substr(StartPos,p_StringMAC.size());
                MACMember_UINT1=(UINT1)strtoul(MACMember.c_str(),NULL,16);
                p_MACArray[MACArrayIndex]=MACMember_UINT1;
                return RETURN_OK;
            }
        }
    }
    return RETURN_ERR;
}
UINT1 MACArraytoString(string&p_StringMAC,UINT1 p_MACArray[])
{
    INT1 MAC_charArray[256]={0};
    sprintf(MAC_charArray,"%02X:%02X:%02X:%02X:%02X:%02X",p_MACArray[0],p_MACArray[1],p_MACArray[2],p_MACArray[3],p_MACArray[4],p_MACArray[5]);
    p_StringMAC=MAC_charArray;
    return RETURN_OK;
}


//========================================================================================================================================================================================
//Floating
//========================================================================================================================================================================================
void CNetworkingRestApi::POST_C_Floating(CRestRequest* p_request, CRestResponse* p_response)
{
    string body = p_request->getBody();
    LOG_WARN(body.c_str());

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        string rspStr("ParseErrorCode: ");
        rspStr.append(to_string(document.Parse(body.c_str()).GetParseError()));
        LOG_WARN(rspStr.c_str());
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, rspStr);
        return;
    }

    string ID;
    string name;
    string tenant_ID;
    string network_ID;
    string router_ID;
    string port_ID;
    UINT1 status;
    UINT4 fixed_IP;
    UINT4 floating_IP;

    Value& FloatingList = document["FloatingList"];
    if (FloatingList.IsArray())
    {
        for(SizeType i = 0; i < FloatingList.Size(); i++)
        {
            const Value& Floating = FloatingList[i];
            if (Floating.IsObject())
            {
                ID="";
                name="";
                tenant_ID="";
                network_ID="";
                router_ID="";
                port_ID="";
                status=0;
                fixed_IP=INADDR_NONE;
                floating_IP=INADDR_NONE;

                if (Floating.HasMember("ID") && Floating["ID"].IsString())
                {
                    ID=Floating["ID"].GetString();
                }
                if (Floating.HasMember("name")&&Floating["name"].IsString())
                {
                    name=Floating["name"].GetString();
                }
                if (Floating.HasMember("tenant_ID")&&Floating["tenant_ID"].IsString())
                {
                    tenant_ID=Floating["tenant_ID"].GetString();
                }
                if (Floating.HasMember("network_ID")&&Floating["network_ID"].IsString())
                {
                    network_ID=Floating["network_ID"].GetString();
                }
                if (Floating.HasMember("router_ID")&&Floating["router_ID"].IsString())
                {
                    router_ID=Floating["router_ID"].GetString();
                }
                if (Floating.HasMember("port_ID")&&Floating["port_ID"].IsString())
                {
                    port_ID=Floating["port_ID"].GetString();
                }
                if (Floating.HasMember("status"))
                {
                    //TBD
                }
                if (Floating.HasMember("fixed_IP")&&Floating["fixed_IP"].IsString())
                {
                   fixed_IP=ip2number(Floating["fixed_IP"].GetString());
                }
                if (Floating.HasMember("floating_IP")&&Floating["floating_IP"].IsString())
                {
                   floating_IP=ip2number(Floating["floating_IP"].GetString());
                }
				
                if (ID.empty())
                {
                    string rspStr("floating ID is needed !");
                    LOG_WARN(rspStr.c_str());
                    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, rspStr);
                    return;
				}     

				Base_Floating *CreatedFloating = new Base_Floating(ID,name,tenant_ID,network_ID,floating_IP);
                if (NULL == CreatedFloating)
                {
                    string rspStr("Internal memory allocation failed !");
                    LOG_WARN(rspStr.c_str());
                    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, rspStr);
                    return;
				}     

				CreatedFloating->set_status(status);
				CreatedFloating->set_port_ID(port_ID);
				CreatedFloating->set_router_ID(router_ID);
				CreatedFloating->set_fixed_IP(fixed_IP);
				
                UINT4 VerifyResult = Verify_CreatedFloating(*CreatedFloating);
				if (FLOATING_VERIFY_CREATED_SUCCESS != VerifyResult)
				{
                    string rspStr("Verify_CreatedFloating return: ");
                    rspStr.append(to_string(VerifyResult));
                    LOG_WARN(rspStr.c_str());
                    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, rspStr);
					delete CreatedFloating;
                    return;
				}

                G_NetworkingEventReporter.report_C_Floating(CreatedFloating, PERSTANCE_TRUE);

                p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
            }
        }
    }
	else
	{
        string rspStr("FloatingList is not array !");
        LOG_WARN(rspStr.c_str());
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, rspStr);
	}
}
void CNetworkingRestApi::GET_R_Floating(CRestRequest* p_request, CRestResponse* p_response)
{
    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    list<Base_Floating *> floatingList;
    G_FloatingMgr.listFloatings(floatingList);

    writer.StartObject();
    writer.Key("FloatingList");

    writer.StartArray();

    INT1 json_tmp[1024]={0};
    STL_FOR_LOOP(floatingList, it)
    {
        writer.StartObject();

        writer.Key("ID");
        memset(json_tmp,0,1024);
        memcpy(json_tmp,(*it)->get_ID().c_str(),(*it)->get_ID().size());
        writer.String(json_tmp);

        writer.Key("name");
        memset(json_tmp,0,1024);
        memcpy(json_tmp,(*it)->get_name().c_str(),(*it)->get_name().size());
        writer.String(json_tmp);

        writer.Key("tenant_ID");
        memset(json_tmp,0,1024);
        memcpy(json_tmp,(*it)->get_tenant_ID().c_str(),(*it)->get_tenant_ID().size());
        writer.String(json_tmp);

        writer.Key("network_ID");
        memset(json_tmp,0,1024);
        memcpy(json_tmp,(*it)->get_network_ID().c_str(),(*it)->get_network_ID().size());
        writer.String(json_tmp);

        writer.Key("router_ID");
        memset(json_tmp,0,1024);
        memcpy(json_tmp,(*it)->get_router_ID().c_str(),(*it)->get_router_ID().size());
        writer.String(json_tmp);

        writer.Key("port_ID");
        memset(json_tmp,0,1024);
        memcpy(json_tmp,(*it)->get_port_ID().c_str(),(*it)->get_port_ID().size());
        writer.String(json_tmp);

        writer.Key("status");
        //TBD
        writer.String(json_tmp);

        writer.Key("fixed_IP");
        memset(json_tmp,0,1024);
        number2ip(((*it)->get_fixed_IP()),json_tmp);
        writer.String(json_tmp);

        writer.Key("floating_IP");
        memset(json_tmp,0,1024);
        number2ip(((*it)->get_floating_IP()),json_tmp);
        writer.String(json_tmp);

        writer.EndObject();
    }

    writer.EndArray();

    writer.Key("retCode");
    writer.Int(0);
    writer.Key("retMsg");
    writer.String("OK");

    writer.EndObject();
	
    string body(strBuff.GetString());
    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
}
void CNetworkingRestApi::PUT_U_Floating(CRestRequest* p_request, CRestResponse* p_response)
{
    string body = p_request->getBody();
    LOG_WARN(body.c_str());

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        string rspStr("ParseErrorCode: ");
        rspStr.append(to_string(document.Parse(body.c_str()).GetParseError()));
        LOG_WARN(rspStr.c_str());
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, rspStr);
        return;
    }

    string ID="";
    string name="";
    string tenant_ID="";
    string network_ID="";
    string router_ID="";
    string port_ID="";
    UINT1 status=0;
    UINT4 fixed_IP=INADDR_NONE;
    UINT4 floating_IP=INADDR_NONE;

    if (document.HasMember("ID") && document["ID"].IsString())
    {
        ID=document["ID"].GetString();
    }
    if (document.HasMember("name") && document["name"].IsString())
    {
        name=document["name"].GetString();
    }
    if (document.HasMember("tenant_ID") && document["tenant_ID"].IsString())
    {
        tenant_ID=document["tenant_ID"].GetString();
    }
    if (document.HasMember("network_ID") && document["network_ID"].IsString())
    {
        network_ID=document["network_ID"].GetString();
    }
    if (document.HasMember("router_ID") && document["router_ID"].IsString())
    {
        router_ID=document["router_ID"].GetString();
    }
    if (document.HasMember("port_ID") && document["port_ID"].IsString())
    {
        port_ID=document["port_ID"].GetString();
    }
    if (document.HasMember("status"))
    {
        //TBD
    }
    if (document.HasMember("fixed_IP") && document["fixed_IP"].IsString())
    {
       fixed_IP=ip2number(document["fixed_IP"].GetString());
    }
    if (document.HasMember("floating_IP") && document["floating_IP"].IsString())
    {
       floating_IP=ip2number(document["floating_IP"].GetString());
    }
    //need to find the target by ID,then change the param

    Base_Floating *UpdatedFloating = new Base_Floating(ID,name,tenant_ID,network_ID,floating_IP);
    if (NULL == UpdatedFloating)
    {
        string rspStr("Internal memory allocation failed !");
        LOG_WARN(rspStr.c_str());
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, rspStr);
        return;
    }

    UpdatedFloating->set_status(status);
    UpdatedFloating->set_port_ID(port_ID);
    UpdatedFloating->set_router_ID(router_ID);
    UpdatedFloating->set_fixed_IP(fixed_IP);
    
    UINT4 VerifyResult = Verify_UpdatedFloating(*UpdatedFloating);
    if (FLOATING_VERIFY_CREATED_SUCCESS != VerifyResult)
    {
        string rspStr("Verify_UpdatedFloating return: ");
        rspStr.append(to_string(VerifyResult));
        LOG_WARN(rspStr.c_str());
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, rspStr);
        delete UpdatedFloating;
        return;
    }
    
    G_NetworkingEventReporter.report_U_Floating(UpdatedFloating, PERSTANCE_TRUE);

    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
}
void CNetworkingRestApi::DELETE_D_Floating(CRestRequest* p_request, CRestResponse* p_response)
{
    string body = p_request->getBody();
    LOG_WARN(body.c_str());

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        string rspStr("ParseErrorCode: ");
        rspStr.append(to_string(document.Parse(body.c_str()).GetParseError()));
        LOG_WARN(rspStr.c_str());
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, rspStr);
        return;
    }

	if (document.HasMember("ID") && document["ID"].IsString())
	{
        const string& floatingId = document["ID"].GetString();
		if (floatingId.empty())
        {
            string rspStr("floating ID is needed !");
            LOG_WARN(rspStr.c_str());
            p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_BAD_REQUEST, rspStr);
            return;
        }

		Base_Floating* NeedDeleteFloating = G_FloatingMgr.targetFloating_ByID(floatingId);
		if (NULL == NeedDeleteFloating)
        {
            string rspStr("floating doesn't exist !");
            LOG_WARN(rspStr.c_str());
            p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_NOT_FOUND, rspStr);
            return;
        }

		G_NetworkingEventReporter.report_D_Floating(NeedDeleteFloating, PERSTANCE_TRUE);

        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
}
void CNetworkingRestApi::DELETE_D_Floatings(CRestRequest* p_request, CRestResponse* p_response)
{
    
}

//========================================================================================================================================================================================
//Network
//========================================================================================================================================================================================
void CNetworkingRestApi::POST_C_Network(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;

    string ID;
    string name;
    string tenant_ID;
    UINT1 status;
    UINT1 router_external;
    UINT1 shared;
    string subnet;

    string body = p_request->getBody();

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult =RETURN_ERR;
    }
    else
    {
        Value & NetworkList=document["NetworkList"];
        if(NetworkList.IsArray())
        {
            for(SizeType i = 0; i < NetworkList.Size(); i++)
            {
                const Value & Network =NetworkList[i];
                if(Network.IsObject())
                {
                    ID="";
                    name="";
                    tenant_ID="";
                    status=BASE_NETWORK_STATUS_ERROR;
                    router_external=BASE_NETWORK_NOT_ROUTEREXTERNAL;
                    shared=BASE_NETWORK_NOT_SHARED;
                    subnet="";

                    if(Network.HasMember("ID")&&Network["ID"].IsString())
                    {
                        ID=Network["ID"].GetString();
                    }
                    if(Network.HasMember("name")&&Network["name"].IsString())
                    {
                        name=Network["name"].GetString();
                    }
                    if(Network.HasMember("tenant_ID")&&Network["tenant_ID"].IsString())
                    {
                        tenant_ID=Network["tenant_ID"].GetString();
                    }
                    if(Network.HasMember("status"))//TBD
                    {}
                    if(Network.HasMember("router_external")&&Network["router_external"].IsBool())
                    {
                        if(Network["router_external"].GetBool())
                        {
                            router_external=BASE_NETWORK_ROUTEREXTERNAL;
                        }
                        else
                        {
                            router_external=BASE_NETWORK_NOT_ROUTEREXTERNAL;
                        }
                    }
                    if(Network.HasMember("shared")&&Network["shared"].IsBool())
                    {
                        if(Network["shared"].GetBool())
                        {
                            shared=BASE_NETWORK_SHARED;
                        }
                        else
                        {
                            shared=BASE_NETWORK_NOT_SHARED;
                        }
                    }
                    Base_Network* CreatedNetwork =new Base_Network(ID,name,tenant_ID,status,router_external,shared);
                    if(Network.HasMember("subnets")&&Network["subnets"].IsArray())
                    {
                        const Value &Subnets =Network["subnets"];
                        for (SizeType j = 0; j < Subnets.Size(); j++)
                        {
                            subnet=Subnets[j].GetString();
							if(subnet.length()>0)
							{
								CreatedNetwork->add_subnet(subnet);
							}
                        }
                    }
					if(Verify_CreatedNetwork(*CreatedNetwork)==NETWORK_VERIFY_CREATED_SUCCESS)
					{
						G_NetworkingEventReporter.report_C_Network(CreatedNetwork,PERSTANCE_TRUE);
						returnResult =RETURN_OK;
					}
					else
					{
						delete CreatedNetwork;
						returnResult =RETURN_ERR;
					}
                    
                }
            }
        }
        else
        {
            returnResult =RETURN_ERR;
        }
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::GET_R_Network(CRestRequest* p_request, CRestResponse* p_response)
{
    string body;
    INT1 json_tmp[1024]={0};
    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    list<Base_Network *> networkList ;
    G_NetworkMgr.listNetworks(networkList);
    list<Base_Network *>::iterator itor = networkList.begin();

    writer.StartObject();
    writer.Key("NetworkList");
    {
        writer.StartArray();
        {
             while(itor != networkList.end())
            {
                writer.StartObject();
                {
//-------------------------------------------------------------------------------------------------------
                    writer.Key("ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_ID().c_str(),(*itor)->get_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("name");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_name().c_str(),(*itor)->get_name().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("tenant_ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_tenant_ID().c_str(),(*itor)->get_tenant_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("status");
                    memset(json_tmp,0,1024);
                    //TBD
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("router_external");
                    writer.Bool((*itor)->get_router_external());
//-------------------------------------------------------------------------------------------------------
                    writer.Key("shared");
                    writer.Bool((*itor)->get_shared());
//-------------------------------------------------------------------------------------------------------
                    writer.Key("subnets");
                    {
                        memset(json_tmp,0,1024);
                        list<string> subnets ;
                        if((*itor)->get_subnets(subnets))
                        {
                            list<string>::iterator itor_subnet = subnets.begin();
                            writer.StartArray();
                            {
                                while(itor_subnet != subnets.end())
                                {
                                    //writer.StartObject();
                                    {
                                        memset(json_tmp,0,1024);
                                        memcpy(json_tmp,(*itor_subnet).c_str(),(*itor_subnet).size());
                                        writer.String(json_tmp);
                                    }
                                    //writer.EndObject();
                                    itor_subnet ++;
                                }
                            }
                            writer.EndArray();
                        }
                    }
//-------------------------------------------------------------------------------------------------------
                }
                writer.EndObject();
                itor++;
            }
        }
        writer.EndArray();

        writer.Key("retCode");
        writer.Int(0);
        writer.Key("retMsg");
        writer.String("OK");
    }
    writer.EndObject();
	
    body.append(strBuff.GetString());
    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
}
void CNetworkingRestApi::PUT_U_Network(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;

    string ID="";
    string name="";
    string tenant_ID="";
    UINT1 status=BASE_NETWORK_STATUS_ERROR;
    UINT1 router_external=BASE_NETWORK_NOT_ROUTEREXTERNAL;
    UINT1 shared=BASE_NETWORK_NOT_SHARED;
    string subnet="";

    string body = p_request->getBody();

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult =RETURN_ERR;
    }
    else
    {
        if(document.HasMember("ID")&&document["ID"].IsString())
        {
            ID=document["ID"].GetString();
        }
        if(document.HasMember("name")&&document["name"].IsString())
        {
            name=document["name"].GetString();
        }
        if(document.HasMember("tenant_ID")&&document["tenant_ID"].IsString())
        {
            tenant_ID=document["tenant_ID"].GetString();
        }
        if(document.HasMember("status"))//TBD
        {}
        if(document.HasMember("router_external")&&document["router_external"].IsBool())
        {
            if(document["router_external"].GetBool())
            {
                router_external=BASE_NETWORK_ROUTEREXTERNAL;
            }
            else
            {
                router_external=BASE_NETWORK_NOT_ROUTEREXTERNAL;
            }
        }
        if(document.HasMember("shared")&&document["shared"].IsBool())
        {
            if(document["shared"].GetBool())
            {
                shared=BASE_NETWORK_SHARED;
            }
            else
            {
                shared=BASE_NETWORK_NOT_SHARED;
            }
        }
        Base_Network* UpdatedNetwork =new Base_Network(ID,name,tenant_ID,status,router_external,shared);
        if(document.HasMember("subnets")&&document["subnets"].IsArray())
        {
            const Value &Subnets =document["subnets"];
            for (SizeType j = 0; j < Subnets.Size(); j++)
            {
                subnet=Subnets[j].GetString();
				if(subnet.length()>0)
				{
					UpdatedNetwork->add_subnet(subnet);
				}
            }
        }
        
		if(Verify_UpdatedNetwork(*UpdatedNetwork)==NETWORK_VERIFY_UPDATED_SUCCESS)
		{
			G_NetworkingEventReporter.report_U_Network(UpdatedNetwork,PERSTANCE_TRUE);
			returnResult =RETURN_OK;
		}
		else
		{
			delete UpdatedNetwork;
			returnResult =RETURN_ERR;
		}
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::DELETE_D_Network(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;
    string body = p_request->getBody();
    Document document;

    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult= RETURN_ERR;
    }
    else
    {
        if (document.HasMember("ID") && document["ID"].IsString())
        {
            if (document["ID"].GetString() != NULL)
            {
                Base_Network* NeedDeleteNetwork =G_NetworkMgr.targetNetwork_ByID(document["ID"].GetString());
                if(NeedDeleteNetwork)
                {
                    G_NetworkingEventReporter.report_D_Network(NeedDeleteNetwork,PERSTANCE_TRUE);
                    returnResult =RETURN_OK;
                }
                else
                {
                    returnResult= RETURN_OK;
                }
            }
            else
            {
                returnResult= RETURN_ERR;
            }
        }
        else
        {
            returnResult= RETURN_ERR;
        }
    }

	body.append("");
    if(returnResult==RETURN_OK)
	{
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::DELETE_D_Networks(CRestRequest* p_request, CRestResponse* p_response)
{

}

//========================================================================================================================================================================================
//Router
//========================================================================================================================================================================================
void CNetworkingRestApi::POST_C_Router(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;

    string ID;
    string name;
    string tenant_ID;
    string network_ID;
    UINT1  status;
    UINT1  enabled_snat;
    string subnet_ID;
    UINT4  IP;

    string body = p_request->getBody();

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult =RETURN_ERR;
    }
    else
    {
        Value & RouterList=document["RouterList"];
        if(RouterList.IsArray())
        {
            for(SizeType i = 0; i < RouterList.Size(); i++)
            {
                const Value & Router =RouterList[i];
                if(Router.IsObject())
                {
                    ID="";
                    name="";
                    tenant_ID="";
                    status=BASE_ROUTER_STATUS_ERROR;
                    network_ID="";
                    enabled_snat=BASE_ROUTER_SNAT_DISABLED;
                    subnet_ID="";
                    IP=INADDR_NONE;

                    if(Router.HasMember("ID")&&Router["ID"].IsString())
                    {
                        ID=Router["ID"].GetString();
                    }
                    if(Router.HasMember("name")&&Router["name"].IsString())
                    {
                        name=Router["name"].GetString();
                    }
                    if(Router.HasMember("tenant_ID")&&Router["tenant_ID"].IsString())
                    {
                        tenant_ID=Router["tenant_ID"].GetString();
                    }
                    if(Router.HasMember("status"))//TBD
                    {}
                    if(Router.HasMember("external_gateway_info")&&Router["external_gateway_info"].IsObject())
                    {
                        const Value & ExternalGatewayInfo = Router["external_gateway_info"];
                        if(ExternalGatewayInfo.HasMember("network_ID")&&ExternalGatewayInfo["network_ID"].IsString())
                        {
                            network_ID=ExternalGatewayInfo["network_ID"].GetString();
                        }
                        if(ExternalGatewayInfo.HasMember("enabled_snat")&&ExternalGatewayInfo["enabled_snat"].IsBool())
                        {
                            if(ExternalGatewayInfo["enabled_snat"].GetBool())
                            {
                                enabled_snat =BASE_ROUTER_SNAT_ENABLED;
                            }
                            else
                            {
                                enabled_snat =BASE_ROUTER_SNAT_DISABLED;
                            }
                        }
                        if(ExternalGatewayInfo.HasMember("external_IP")&&ExternalGatewayInfo["external_IP"].IsObject())
                        {
                            const Value & ExternalIP = ExternalGatewayInfo["external_IP"];
                            if(ExternalIP.HasMember("subnet_ID")&&ExternalIP["subnet_ID"].IsString())
                            {
                                subnet_ID=ExternalIP["subnet_ID"].GetString();
                            }
                            if(ExternalIP.HasMember("IP")&&ExternalIP["IP"].IsString())
                            {
                                IP=ip2number(ExternalIP["IP"].GetString());
                            }
                        }
                    }
		
                    Base_Router * CreatedRouter =new Base_Router(ID,name,tenant_ID);
                    CreatedRouter->set_status(status);
                    CreatedRouter->set__external_gateway_info__network_ID(network_ID);
                    CreatedRouter->set__external_gateway_info__enabled_snat(enabled_snat);
                    CreatedRouter->set__external_gateway_info__subnet_ID(subnet_ID);
                    CreatedRouter->set__external_gateway_info__IP(IP);
					
					if(Verify_CreatedRouter(*CreatedRouter)==ROUTER_VERIFY_CREATED_SUCCESS)
					{
						G_NetworkingEventReporter.report_C_Router(CreatedRouter,PERSTANCE_TRUE);
						returnResult = RETURN_OK;
					}
					else
					{
						delete CreatedRouter;
						returnResult = RETURN_ERR;
					}
                    
                }
            }
        }
        else
        {
            returnResult =RETURN_ERR;
        }
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::GET_R_Router(CRestRequest* p_request, CRestResponse* p_response)
{
    string body;
    INT1 json_tmp[1024]={0};
    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    list< Base_Router *> routerList;
    G_RouterMgr.listRouters(routerList);
    list<Base_Router *>::iterator itor = routerList.begin();

    writer.StartObject();
    writer.Key("RouterList");
    {
        writer.StartArray();
        {
            while(itor != routerList.end())
            {
                writer.StartObject();
                {
//-------------------------------------------------------------------------------------------------------
                    writer.Key("ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_ID().c_str(),(*itor)->get_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("name");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_name().c_str(),(*itor)->get_name().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("tenant_ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_tenant_ID().c_str(),(*itor)->get_tenant_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("status");
                    memset(json_tmp,0,1024);
                    //TBD
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("external_gateway_info");
                    writer.StartObject();
                    {
                        RouterExternalGateway ExternalGateway =(*itor)->get_external_gateway_info();
                        writer.Key("network_ID");
                        memset(json_tmp,0,1024);
                        memcpy(json_tmp,ExternalGateway.get_network_ID().c_str(),ExternalGateway.get_network_ID().size());
                        writer.String(json_tmp);

                        writer.Key("enabled_snat");
                        writer.Bool(ExternalGateway.get_enabled_snat());

                        writer.Key("external_IP");
                        writer.StartObject();
                        {
                            Fixed_IP Gateway_IP =ExternalGateway.get_external_IP();

                            writer.Key("subnet_ID");
                                memset(json_tmp,0,1024);
                                memcpy(json_tmp,Gateway_IP.get_subnet_ID().c_str(),Gateway_IP.get_subnet_ID().size());
                            writer.String(json_tmp);

                            writer.Key("IP");
                                memset(json_tmp,0,1024);
                                number2ip((Gateway_IP.get_IP()),json_tmp);
                            writer.String(json_tmp);
                        }
                        writer.EndObject();

                    }
                    writer.EndObject();
//-------------------------------------------------------------------------------------------------------
                }
                writer.EndObject();
                itor++;
            }
        }
        writer.EndArray();

        writer.Key("retCode");
        writer.Int(0);
        writer.Key("retMsg");
        writer.String("OK");
    }
    writer.EndObject();

	body.append(strBuff.GetString());
    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
}
void CNetworkingRestApi::PUT_U_Router(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;

    string ID="";
    string name="";
    string tenant_ID="";
    string network_ID="";
    UINT1  status=BASE_ROUTER_STATUS_ERROR;
    UINT1  enabled_snat=BASE_ROUTER_SNAT_DISABLED;
    string subnet_ID="";
    UINT4  IP=INADDR_NONE;

    string body = p_request->getBody();

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult =RETURN_ERR;
    }
    else
    {
        if(document.HasMember("ID")&&document["ID"].IsString())
        {
            ID=document["ID"].GetString();
        }
        if(document.HasMember("name")&&document["name"].IsString())
        {
            name=document["name"].GetString();
        }
        if(document.HasMember("tenant_ID")&&document["tenant_ID"].IsString())
        {
            tenant_ID=document["tenant_ID"].GetString();
        }
        if(document.HasMember("status"))//TBD
        {}
        if(document.HasMember("external_gateway_info")&&document["external_gateway_info"].IsObject())
        {
            const Value & ExternalGatewayInfo = document["external_gateway_info"];
            if(ExternalGatewayInfo.HasMember("network_ID")&&ExternalGatewayInfo["network_ID"].IsString())
            {
                network_ID=ExternalGatewayInfo["network_ID"].GetString();
            }
            if(ExternalGatewayInfo.HasMember("enabled_snat")&&ExternalGatewayInfo["enabled_snat"].IsBool())
            {
                if(ExternalGatewayInfo["enabled_snat"].GetBool())
                {
                    enabled_snat =BASE_ROUTER_SNAT_ENABLED;
                }
                else
                {
                    enabled_snat =BASE_ROUTER_SNAT_DISABLED;
                }
            }
            if(ExternalGatewayInfo.HasMember("external_IP")&&ExternalGatewayInfo["external_IP"].IsObject())
            {
                const Value & ExternalIP = ExternalGatewayInfo["external_IP"];
                if(ExternalIP.HasMember("subnet_ID")&&ExternalIP["subnet_ID"].IsString())
                {
                    subnet_ID=ExternalIP["subnet_ID"].GetString();
                }
                if(ExternalIP.HasMember("IP")&&ExternalIP["IP"].IsString())
                {
                    IP=ip2number(ExternalIP["IP"].GetString());
                }
            }
        }

        Base_Router * UpdatedRouter =new Base_Router(ID,name,tenant_ID);
        UpdatedRouter->set_status(status);
        UpdatedRouter->set__external_gateway_info__network_ID(network_ID);
        UpdatedRouter->set__external_gateway_info__enabled_snat(enabled_snat);
        UpdatedRouter->set__external_gateway_info__subnet_ID(subnet_ID);
        UpdatedRouter->set__external_gateway_info__IP(IP);
		
		if(Verify_UpdatedRouter(*UpdatedRouter)==ROUTER_VERIFY_UPDATED_SUCCESS)
		{
			G_NetworkingEventReporter.report_U_Router(UpdatedRouter,PERSTANCE_TRUE);
			returnResult = RETURN_OK;
		}
		else
		{
			delete UpdatedRouter;
			returnResult = RETURN_ERR;
		}
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::DELETE_D_Router(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;
    string body = p_request->getBody();
    Document document;

    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult= RETURN_ERR;
    }
    else
    {
        if (document.HasMember("ID") && document["ID"].IsString())
        {
            if (document["ID"].GetString() != NULL)
            {
                Base_Router * NeedDeleteRouter =G_RouterMgr.targetRouter_ByID(document["ID"].GetString());
                G_NetworkingEventReporter.report_D_Router(NeedDeleteRouter,PERSTANCE_TRUE);
                returnResult = RETURN_OK;
            }
            else
            {
                returnResult= RETURN_ERR;
            }
        }
        else
        {
            returnResult= RETURN_ERR;
        }
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::DELETE_D_Routers(CRestRequest* p_request, CRestResponse* p_response)
{
}

//========================================================================================================================================================================================
//Subnet
//========================================================================================================================================================================================
void CNetworkingRestApi::POST_C_Subnet(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;

    string ID;
    string name;
    string tenant_ID;
    string network_ID;
    string cidr;
    UINT4 gateway_IP;
    UINT1 status;
    UINT4 IP_start;
    UINT4 IP_end;

    string body = p_request->getBody();

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult =RETURN_ERR;
    }
    else
    {
        Value & SubnetList=document["SubnetList"];
        if(SubnetList.IsArray())
        {
            for(SizeType i = 0; i < SubnetList.Size(); i++)
            {
                const Value & Subnet =SubnetList[i];
                if(Subnet.IsObject())
                {
                    ID="";
                    name="";
                    tenant_ID="";
                    network_ID="";
                    cidr="";
                    gateway_IP=INADDR_NONE;
                    status=BASE_SUBNET_STATUS_ERROR;
                    IP_start=INADDR_NONE;
                    IP_end=INADDR_NONE;

                    if(Subnet.HasMember("ID")&&Subnet["ID"].IsString())
                    {
                        ID=Subnet["ID"].GetString();
                    }
                    if(Subnet.HasMember("name")&&Subnet["name"].IsString())
                    {
                        name=Subnet["name"].GetString();
                    }
                    if(Subnet.HasMember("tenant_ID")&&Subnet["tenant_ID"].IsString())
                    {
                        tenant_ID=Subnet["tenant_ID"].GetString();
                    }
                    if(Subnet.HasMember("network_ID")&&Subnet["network_ID"].IsString())
                    {
                        network_ID=Subnet["network_ID"].GetString();
                    }
                    if(Subnet.HasMember("cidr")&&Subnet["cidr"].IsString())
                    {
                        cidr=Subnet["cidr"].GetString();
                    }
                    if(Subnet.HasMember("status"))//TBD
                    {}
                    if(Subnet.HasMember("gateway_IP")&&Subnet["gateway_IP"].IsString())
                    {
                        gateway_IP=ip2number(Subnet["gateway_IP"].GetString());
                    }
                    if(Subnet.HasMember("allocation_pools")&&Subnet["allocation_pools"].IsArray())
                    {
                        const Value & allocation_pools =Subnet["allocation_pools"];
                        if(allocation_pools.IsArray())
                        {
                            for(SizeType j = 0; j < allocation_pools.Size(); j++)
                            {
                                 const Value & allocation_pool =allocation_pools[j];
                                 IP_start=INADDR_NONE;
                                 IP_end=INADDR_NONE;
                                 if(allocation_pool.HasMember("IP_start")&&allocation_pool["IP_start"].IsString())
                                 {
                                     IP_start=ip2number(allocation_pool["IP_start"].GetString());
                                 }
                                 if(allocation_pool.HasMember("IP_end")&&allocation_pool["IP_end"].IsString())
                                 {
                                     IP_end=ip2number(allocation_pool["IP_end"].GetString());
                                 }
                                 //TBD
                            }
                        }

                    }
                    Base_Subnet* CreatedSubnet =new Base_Subnet(ID,name,tenant_ID,network_ID,cidr,gateway_IP,IP_start,IP_end);
					CreatedSubnet->change_status(status);
					
					if(Verify_CreatedSubnet(*CreatedSubnet)==SUBNET_VERIFY_CREATED_SUCCESS)
					{
						G_NetworkingEventReporter.report_C_Subnet(CreatedSubnet,PERSTANCE_TRUE);
						returnResult = RETURN_OK;
					}
					else
					{
						delete CreatedSubnet;
						returnResult =RETURN_ERR;
					}
				
                }
            }
        }
        else
        {
            returnResult =RETURN_ERR;
        }
    }
	
	body.append("");
    if(returnResult==RETURN_OK)
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::GET_R_Subnet(CRestRequest* p_request, CRestResponse* p_response)
{
     string body;
    INT1 json_tmp[1024]={0};
    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    list< Base_Subnet *> subnetList ;
    G_SubnetMgr.listSubnets(subnetList);
    list<Base_Subnet *>::iterator itor = subnetList.begin();

    writer.StartObject();
    writer.Key("SubnetList");
    {
        writer.StartArray();
        {
             while(itor != subnetList.end())
            {
                writer.StartObject();
                {
//-------------------------------------------------------------------------------------------------------
                    writer.Key("ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_ID().c_str(),(*itor)->get_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("name");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_name().c_str(),(*itor)->get_name().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("tenant_ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_tenant_ID().c_str(),(*itor)->get_tenant_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("network_ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_network_ID().c_str(),(*itor)->get_network_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("cidr");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_cidr().c_str(),(*itor)->get_cidr().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("status");
                    memset(json_tmp,0,1024);
                    //TBD
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("gateway_IP");
                    memset(json_tmp,0,1024);
                    number2ip(((*itor)->get_gateway_IP()),json_tmp);
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("allocation_pools");
                    {
                        memset(json_tmp,0,1024);
                        list<IP_pool *> allocation_pool ;
                        if((*itor)->get_allocation_pools(allocation_pool))
                        {
                            list<IP_pool *>::iterator itor_pool = allocation_pool.begin();
                            writer.StartArray();
                            {
                                while(itor_pool != allocation_pool.end())
                                {
                                    writer.StartObject();
                                    {
                                        writer.Key("IP_start");
                                        memset(json_tmp,0,1024);
                                        number2ip(((*itor_pool)->get_IP_start()),json_tmp);
                                        writer.String(json_tmp);

                                        writer.Key("IP_end");
                                        memset(json_tmp,0,1024);
                                        number2ip(((*itor_pool)->get_IP_end()),json_tmp);
                                        writer.String(json_tmp);
                                    }
                                    writer.EndObject();
                                    itor_pool ++;
                                }
                            }
                            writer.EndArray();
                        }
                    }
//-------------------------------------------------------------------------------------------------------
                }
                writer.EndObject();
                itor ++;
            }
        }
        writer.EndArray();

        writer.Key("retCode");
        writer.Int(0);
        writer.Key("retMsg");
        writer.String("OK");
    }
    writer.EndObject();
	
	body.append(strBuff.GetString());
    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
}
void CNetworkingRestApi::PUT_U_Subnet(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;

    string ID="";
    string name="";
    string tenant_ID="";
    string network_ID="";
    string cidr="";
    UINT4 gateway_IP=INADDR_NONE;
    UINT1 status=BASE_SUBNET_STATUS_ERROR;
    UINT4 IP_start=INADDR_NONE;
    UINT4 IP_end=INADDR_NONE;

    string body = p_request->getBody();

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult =RETURN_ERR;
    }
    else
    {
        if(document.HasMember("ID")&&document["ID"].IsString())
        {
            ID=document["ID"].GetString();
        }
        if(document.HasMember("name")&&document["name"].IsString())
        {
            name=document["name"].GetString();
        }
        if(document.HasMember("tenant_ID")&&document["tenant_ID"].IsString())
        {
            tenant_ID=document["tenant_ID"].GetString();
        }
        if(document.HasMember("network_ID")&&document["network_ID"].IsString())
        {
            network_ID=document["network_ID"].GetString();
        }
        if(document.HasMember("cidr")&&document["cidr"].IsString())
        {
            cidr=document["cidr"].GetString();
        }
        if(document.HasMember("status"))//TBD
        {}
        if(document.HasMember("gateway_IP")&&document["gateway_IP"].IsString())
        {
            gateway_IP=ip2number(document["gateway_IP"].GetString());
        }
        if(document.HasMember("allocation_pools")&&document["allocation_pools"].IsArray())
        {
            const Value & allocation_pools =document["allocation_pools"];
            if(allocation_pools.IsArray())
            {
                for(SizeType j = 0; j < allocation_pools.Size(); j++)
                {
                     const Value & allocation_pool =allocation_pools[j];
                     IP_start=INADDR_NONE;
                     IP_end=INADDR_NONE;
                     if(allocation_pool.HasMember("IP_start")&&allocation_pool["IP_start"].IsString())
                     {
                         IP_start=ip2number(allocation_pool["IP_start"].GetString());
                     }
                     if(allocation_pool.HasMember("IP_end")&&allocation_pool["IP_end"].IsString())
                     {
                         IP_end=ip2number(allocation_pool["IP_end"].GetString());
                     }
                     //TBD
                }
            }

        }
        Base_Subnet* UpdatedSubnet =new Base_Subnet(ID,name,tenant_ID,network_ID,cidr,gateway_IP,IP_start,IP_end);
		UpdatedSubnet->change_status(status);
		
		if(Verify_UpdatedSubnet(*UpdatedSubnet)==SUBNET_VERIFY_UPDATED_SUCCESS)
		{
			G_NetworkingEventReporter.report_U_Subnet(UpdatedSubnet,PERSTANCE_TRUE);
			returnResult = RETURN_OK;
		}
		else
		{
			delete UpdatedSubnet;
			returnResult = RETURN_ERR;
		}
    }
	
	body.append("");
    if(returnResult==RETURN_OK)
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::DELETE_D_Subnet(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;
    string body = p_request->getBody();
    Document document;

    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult= RETURN_ERR;
    }
    else
    {
        if (document.HasMember("ID") && document["ID"].IsString())
        {
            if (document["ID"].GetString() != NULL)
            {
                Base_Subnet* NeedDeleteSubnet =G_SubnetMgr.targetSubnet_ByID(document["ID"].GetString());
                G_NetworkingEventReporter.report_D_Subnet(NeedDeleteSubnet,PERSTANCE_TRUE);
                returnResult = RETURN_OK;
            }
            else
            {
                returnResult= RETURN_ERR;
            }
        }
        else
        {
            returnResult= RETURN_ERR;
        }
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::DELETE_D_Subnets(CRestRequest* p_request, CRestResponse* p_response)
{

}

void CNetworkingRestApi::POST_C_Port(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;

    string ID;
    string name;
    string tenant_ID;
    string device_owner;
	string device_ID;
    string network_ID;
    string string_MAC;
    string subnet_ID;
    UINT4 IP;
    UINT1 status;
    UINT1 MAC[6]={0,0,0,0,0,0};


    string body = p_request->getBody();

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult =RETURN_ERR;
    }
    else
    {
        Value & PortList=document["PortList"];
        if(PortList.IsArray())
        {
            for(SizeType i = 0; i < PortList.Size(); i++)
            {
                const Value & Port =PortList[i];
                if(Port.IsObject())
                {
                    ID="";
                    name="";
                    tenant_ID="";
                    device_owner="";
                    network_ID="";
					device_ID="";
                    string_MAC="";
                    subnet_ID="";
                    IP=INADDR_NONE;
                    status=BASE_PORT_STATUS_ERROR;
                    MAC[0]=0;MAC[1]=0;MAC[2]=0;MAC[3]=0;MAC[4]=0;MAC[5]=0;

                    if(Port.HasMember("ID")&&Port["ID"].IsString())
                    {
                        ID=Port["ID"].GetString();
                    }
                    if(Port.HasMember("name")&&Port["name"].IsString())
                    {
                        name=Port["name"].GetString();
                    }
                    if(Port.HasMember("tenant_ID")&&Port["tenant_ID"].IsString())
                    {
                        tenant_ID=Port["tenant_ID"].GetString();
                    }
                    if(Port.HasMember("device_owner")&&Port["device_owner"].IsString())
                    {
                        device_owner=Port["device_owner"].GetString();
                    }
                    if(Port.HasMember("network_ID")&&Port["network_ID"].IsString())
                    {
                        network_ID=Port["network_ID"].GetString();
                    }
					if(Port.HasMember("device_ID")&&Port["device_ID"].IsString())
                    {
                        device_ID=Port["device_ID"].GetString();
                    }
                    if(Port.HasMember("MAC")&&Port["MAC"].IsString())
                    {
                        string_MAC=Port["MAC"].GetString();
                        MACStringtoArray(string_MAC,MAC);
                    }
                    if(Port.HasMember("status"))//TBD
                    {}
          
                    Base_Port* CreatedPort =new Base_Port(ID,name,tenant_ID,network_ID,device_ID,device_owner,status,MAC);
                    if(Port.HasMember("Fixed_IPs")&&Port["Fixed_IPs"].IsArray())
                    {
                        const Value &Fixed_IPs =Port["Fixed_IPs"];
                        for (SizeType j = 0; j < Fixed_IPs.Size(); j++)
                        {
                            if(Fixed_IPs[j].HasMember("subnet_ID")&&Fixed_IPs[j]["subnet_ID"].IsString())
                            {
                                subnet_ID=Fixed_IPs[j]["subnet_ID"].GetString();
                            }
                            if(Fixed_IPs[j].HasMember("IP")&&Fixed_IPs[j]["IP"].IsString())
                            {
                                IP=ip2number(Fixed_IPs[j]["IP"].GetString());
                            }
                            CreatedPort->add_fixed_IP(IP,subnet_ID);
                        }
                    }

					if(Verify_CreatedPort(*CreatedPort)==PORT_VERIFY_CTEATED_SUCCESS)
					{
						G_NetworkingEventReporter.report_C_Port(CreatedPort,PERSTANCE_TRUE);
						returnResult =RETURN_OK;
					}
					else
					{
						delete CreatedPort;
						returnResult =RETURN_ERR;
					}
                }
            }
        }
        else
        {
            returnResult =RETURN_ERR;
        }
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::GET_R_Port(CRestRequest* p_request, CRestResponse* p_response)
{
    string body;
    INT1 json_tmp[1024]={0};
    StringBuffer strBuff;
    Writer<StringBuffer> writer(strBuff);

    list<Base_Port *> portList ;
    G_PortMgr.listPorts(portList);
    list<Base_Port *>::iterator itor = portList.begin();

    writer.StartObject();
    writer.Key("PortList");
    {
        writer.StartArray();
        {
             while(itor != portList.end())
            {
                writer.StartObject();
                {
//-------------------------------------------------------------------------------------------------------
                    writer.Key("ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_ID().c_str(),(*itor)->get_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("name");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_name().c_str(),(*itor)->get_name().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("tenant_ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_tenant_ID().c_str(),(*itor)->get_tenant_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("device_owner");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_device_owner().c_str(),(*itor)->get_device_owner().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("network_ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_network_ID().c_str(),(*itor)->get_network_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
					writer.Key("device_ID");
                    memset(json_tmp,0,1024);
                    memcpy(json_tmp,(*itor)->get_device_ID().c_str(),(*itor)->get_device_ID().size());
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("MAC");
                    memset(json_tmp,0,1024);
                    string MAC_string ="";
                    MACArraytoString(MAC_string,(*itor)->get_MAC());
                    MAC_string.copy(json_tmp,MAC_string.size(),0);
                    json_tmp[MAC_string.size()]='\0';
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                    writer.Key("status");
                    memset(json_tmp,0,1024);
                    //TBD
                    writer.String(json_tmp);
//-------------------------------------------------------------------------------------------------------
                  
//-------------------------------------------------------------------------------------------------------
                    writer.Key("Fixed_IPs");
                    {
                        memset(json_tmp,0,1024);
                        list<Fixed_IP *> Fixed_IPs ;
                        if((*itor)->get_Fixed_IPs(Fixed_IPs))
                        {
                            list<Fixed_IP *>::iterator itor_Fixed_IP = Fixed_IPs.begin();
                            writer.StartArray();
                            {
                                while(itor_Fixed_IP != Fixed_IPs.end())
                                {
                                    writer.StartObject();
                                    {
                                        writer.Key("subnet_ID");
                                        memset(json_tmp,0,1024);
                                        memcpy(json_tmp,((*itor_Fixed_IP)->get_subnet_ID()).c_str(),((*itor_Fixed_IP)->get_subnet_ID()).size());
                                        writer.String(json_tmp);

                                        writer.Key("IP");
                                        memset(json_tmp,0,1024);
                                        number2ip(((*itor_Fixed_IP)->get_IP()),json_tmp);
                                        writer.String(json_tmp);
                                    }
                                    writer.EndObject();
                                    itor_Fixed_IP ++;
                                }
                            }
                            writer.EndArray();
                        }
                    }
//-------------------------------------------------------------------------------------------------------
                }
                writer.EndObject();
                itor++;
            }
        }
        writer.EndArray();

        writer.Key("retCode");
        writer.Int(0);
        writer.Key("retMsg");
        writer.String("OK");
    }
    writer.EndObject();
	
    body.append(strBuff.GetString());
    p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
}
void CNetworkingRestApi::PUT_U_Port(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;

    string ID;
    string name;
    string tenant_ID;
    string device_owner;
	string device_ID;
    string network_ID;
    string string_MAC;
    string subnet_ID;
    UINT4 IP;
    UINT1 status;
    UINT1 MAC[6]={0,0,0,0,0,0};


    string body = p_request->getBody();

    Document document;
    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult =RETURN_ERR;
    }
    else
    {
		ID="";
		name="";
		tenant_ID="";
		device_owner="";
		network_ID="";
		device_ID="";
		string_MAC="";
		subnet_ID="";
		IP=INADDR_NONE;
		status=BASE_PORT_STATUS_ERROR;
		MAC[5]=0;MAC[1]=0;MAC[2]=0;MAC[3]=0;MAC[4]=0;MAC[5]=0;

		if(document.HasMember("ID")&&document["ID"].IsString())
		{
			ID=document["ID"].GetString();
		}
		if(document.HasMember("name")&&document["name"].IsString())
		{
			name=document["name"].GetString();
		}
		if(document.HasMember("tenant_ID")&&document["tenant_ID"].IsString())
		{
			tenant_ID=document["tenant_ID"].GetString();
		}
		if(document.HasMember("device_owner")&&document["device_owner"].IsString())
		{
			device_owner=document["device_owner"].GetString();
		}
		if(document.HasMember("network_ID")&&document["network_ID"].IsString())
		{
			network_ID=document["network_ID"].GetString();
		}
		if(document.HasMember("device_ID")&&document["device_ID"].IsString())
		{
			device_ID=document["device_ID"].GetString();
		}
		if(document.HasMember("MAC")&&document["MAC"].IsString())
		{
			string_MAC=document["MAC"].GetString();
			MACStringtoArray(string_MAC,MAC);
		}
		if(document.HasMember("status"))//TBD
		{}
		Base_Port* UpdatedPort =new Base_Port(ID,name,tenant_ID,network_ID,device_ID,device_owner,status,MAC);
		if(document.HasMember("Fixed_IPs")&&document["Fixed_IPs"].IsArray())
		{
			const Value &Fixed_IPs =document["Fixed_IPs"];
			for (SizeType j = 0; j < Fixed_IPs.Size(); j++)
			{
				if(Fixed_IPs[j].HasMember("subnet_ID")&&Fixed_IPs[j]["subnet_ID"].IsString())
				{
					subnet_ID=Fixed_IPs[j]["subnet_ID"].GetString();
				}
				if(Fixed_IPs[j].HasMember("IP")&&Fixed_IPs[j]["IP"].IsString())
				{
					IP=ip2number(Fixed_IPs[j]["IP"].GetString());
				}
				UpdatedPort->add_fixed_IP(IP,subnet_ID);
			}
		}
		if(Verify_UpdatedPort(*UpdatedPort)==PORT_VERIFY_UPDATED_SUCCESS)
		{
			G_NetworkingEventReporter.report_U_Port(UpdatedPort,PERSTANCE_TRUE);
			returnResult =RETURN_OK;
		}
		else
		{
			delete UpdatedPort;
			returnResult =RETURN_ERR;
		}
  
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
		p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::DELETE_D_Port(CRestRequest* p_request, CRestResponse* p_response)
{
    UINT1 returnResult =RETURN_ERR;
    string body = p_request->getBody();
    Document document;

    if (document.Parse(body.c_str()).HasParseError())
    {
        returnResult= RETURN_ERR;
    }
    else
    {
        if (document.HasMember("ID") && document["ID"].IsString())
        {
            if (document["ID"].GetString() != NULL)
            {
                Base_Port* NeedDeletePort =G_PortMgr.targetPort_ByID(document["ID"].GetString());
                if(NeedDeletePort)
                {
                    G_NetworkingEventReporter.report_D_Port(NeedDeletePort,PERSTANCE_TRUE);
                    returnResult =RETURN_OK;
                }
                else
                {
                    returnResult= RETURN_OK;
                }
            }
            else
            {
                returnResult= RETURN_ERR;
            }
        }
        else
        {
            returnResult= RETURN_ERR;
        }
    }
	body.append("");
    if(returnResult==RETURN_OK)
	{
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_OK, body);
	}
	else
	{
        p_response->setResponse(p_request->getVersion(), bnc::restful::STATUS_FORBIDDEN, body);
	}
}
void CNetworkingRestApi::DELETE_D_Ports(CRestRequest* p_request, CRestResponse* p_response)
{
}

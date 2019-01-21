#include "CControl.h"
#include "CFlowMgr.h"
#include "Service_ExternalDetecting.h"

ArpFlood_info::ArpFlood_info(UINT4 p_src_IP,UINT1* p_src_MAC,UINT4 p_dst_IP)
{
    src_IP=p_src_IP;
    memcpy(src_MAC,p_src_MAC,6);
    dst_IP=p_dst_IP;
}
UINT1 ArpFlood_info::checkifMatched(UINT4 p_src_IP,UINT1* p_src_MAC,UINT4 p_dst_IP)
{
    if((src_IP==p_src_IP)&&(dst_IP==p_dst_IP)&&(memcmp(src_MAC,p_src_MAC,6)==0))
    {
        return RETURN_TRUE;
    }
    else
    {
        return RETURN_FALSE;
    }
}
UINT4 ArpFlood_info::get_src_IP(void)
{
	return src_IP;
}
UINT1* ArpFlood_info::get_src_MAC(void)
{
	return src_MAC;
}
UINT4 ArpFlood_info::get_dst_IP(void)
{
	return dst_IP;
}

Service_ExternalDetecting* Service_ExternalDetecting::Instance =NULL;

Service_ExternalDetecting* Service_ExternalDetecting::Get_Instance(void)
{
    if(Instance == NULL)
    {
        Instance =new Service_ExternalDetecting();
    }

    return Instance;
}

Service_ExternalDetecting::Service_ExternalDetecting()
{
	pthread_mutex_init(&BaseSrvMutex, NULL);
	assert(ExternalDetectingTimer.schedule(EXTERNAL_DETECT_INTERVAL*3,EXTERNAL_DETECT_INTERVAL,timedTask,NULL)==BNC_OK);
}
void Service_ExternalDetecting::init(void)
{
	//do nothing, just init instance
}

Service_ExternalDetecting::~Service_ExternalDetecting()
{
    pthread_mutex_destroy(&BaseSrvMutex);
}

UINT1 Service_ExternalDetecting::CheckARPReply_UpdateExternal(UINT4 p_reply_dst_IP,UINT1* p_reply_dst_MAC,UINT4 p_reply_src_IP,UINT1 * p_reply_src_MAC,UINT8 p_switch_DPID,UINT4 p_switch_port)
{
	BOOL bMatchResult = FALSE;
	list<ArpFlood_info *>::iterator itor;
    lock_Mutex();
    itor = floods.begin();
    UINT1 result =RETURN_FALSE;
    while (itor != floods.end())
    {    	
        if ((*itor)->checkifMatched(p_reply_dst_IP,p_reply_dst_MAC,p_reply_src_IP) == RETURN_TRUE)
        {
        	//cout<<__FUNCTION__<<"=============================================="<<__LINE__<<" p_reply_src_IP= "<<hex<<p_reply_src_IP<<endl;
            result=RETURN_TRUE;
			G_ExternalMgr.updateExternal_Bygateway_IP(p_reply_src_IP,p_reply_src_MAC,p_switch_DPID,p_switch_port, &bMatchResult);
            break;
        }
        itor++;
    }
    unlock_Mutex();
    if((result==RETURN_TRUE)/*&&(TRUE != bMatchResult)*/)
    {
    	CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(p_switch_DPID);
		CFlowMgr::getInstance()->install_modifine_ExternalSwitch_Table_flow(sw);
		CFlowMgr::getInstance()->install_modifine_ExternalSwitch_ExPort_flow(sw, p_switch_port);
		//by: ?????????MAC????
		CFlowMgr::getInstance()->install_fabric_external_output_flow(sw,p_switch_port,p_reply_src_MAC,p_reply_dst_IP,1);
		//by: ????????IP????
		CFlowMgr::getInstance()->install_fabric_external_output_flow(sw,p_switch_port,p_reply_src_MAC,p_reply_dst_IP,2);
        deleteNode_ByFlood(*itor);
    }
    return result;
}

void  Service_ExternalDetecting::timedTask(void* param)
{
	//cout<<__FUNCTION__<<"=============================================="<<__LINE__<<endl;
    Service_ExternalDetecting::Get_Instance()->generateFloodNode();
    Service_ExternalDetecting::Get_Instance()->generateARPFlood();
}

UINT1 Service_ExternalDetecting::generateFloodNode(void)
{
	//cout<<__FUNCTION__<<"=============================================="<<__LINE__<<endl;
    list<Base_External *> externals;
    list<Base_External *>::iterator itor_external;
    list<Base_Port *> ports;
    list<Base_Port *>::iterator itor_port;
    UINT4 src_IP=INADDR_NONE;

    G_ExternalMgr.listExternal(externals);
    G_PortMgr.targetPort_Bydevice_owner("network:router_gateway",ports);
    Service_ExternalDetecting::clearNodes();

    itor_external=externals.begin();
    while(itor_external!=externals.end())
    {
		//cout<<__FUNCTION__<<"=============================================="<<__LINE__<<endl;
        itor_port=ports.begin();
        while(itor_port!=ports.end())
        {
			//cout<<__FUNCTION__<<"=============================================="<<__LINE__<<endl;
            src_IP=(*itor_port)->search_IP_by_subnetID((*itor_external)->get_subnet_ID());
            if(src_IP!=INADDR_NONE)
            {
				//cout<<__FUNCTION__<<"=============================================="<<__LINE__<<endl;
                insertNode_ByInfo(src_IP,(*itor_port)->get_MAC(),(*itor_external)->get_gateway_IP());
                break;
            }
            itor_port++;
        }
        itor_external++;
    }
    return RETURN_OK;
}

UINT1 Service_ExternalDetecting::generateARPFlood(void)
{
	//UINT1 mac[MAC_LEN] = {0};
	//cout<<__FUNCTION__<<"=============================================="<<__LINE__<<endl;
	list<ArpFlood_info *>::iterator itor;
    lock_Mutex();
    itor = floods.begin();
    while (itor != floods.end())
    {
        CArpFloodMgr::getInstance()->AddArpRequestNode((*itor)->get_dst_IP(),(*itor)->get_src_IP(),(*itor)->get_src_MAC());
		//cout<<__FUNCTION__<<"==============="<<__LINE__<<" dstip=" << hex<< (*itor)->get_dst_IP() <<" srcip="<<hex <<(*itor)->get_src_IP() <<endl;
		//memcpy(mac, (*itor)->get_src_MAC(), MAC_LEN);
		//cout<<"MAC = "<<mac[0]<<":"<<mac[1]<<":"<<mac[2]<<":"<<mac[3]<<":"<<mac[4]<<":"<<mac[5]<<endl;
		itor++;
    }
    unlock_Mutex();
    return RETURN_OK;
}

UINT1 Service_ExternalDetecting::insertNode_ByInfo(UINT4 p_src_IP,UINT1* p_src_MAC,UINT4 p_dst_IP)
{
    ArpFlood_info* CreatedFlood =new ArpFlood_info(p_src_IP,p_src_MAC,p_dst_IP);
    if(CreatedFlood)
    {
        lock_Mutex();
        floods.push_front(CreatedFlood);
        unlock_Mutex();
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }
}
UINT1 Service_ExternalDetecting::deleteNode_ByFlood(ArpFlood_info * p_flood)
{
    if(p_flood)
    {
        lock_Mutex();
        floods.remove(p_flood);
        unlock_Mutex();
        delete p_flood;
        return RETURN_OK;
    }
    else
    {
        return RETURN_ERR;
    }
}
UINT1 Service_ExternalDetecting::clearNodes(void)
{
    lock_Mutex();
    floods.clear();
    unlock_Mutex();
    return RETURN_OK;
}

void Service_ExternalDetecting::lock_Mutex(void)
{
    pthread_mutex_lock(&BaseSrvMutex);
}
void Service_ExternalDetecting::unlock_Mutex(void)
{
    pthread_mutex_unlock(&BaseSrvMutex);
}

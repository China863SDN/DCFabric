
#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CConf.h"
#include "CTimer.h"
#include "CControl.h"
#include "CServer.h"
#include "CSwitch.h"
#include "CFloatingIp.h"
#include "CFloatingIpMgr.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "BaseExternal.h"
#include "BaseExternalManager.h"
#include "CRouter.h"
#include "CRouterGateMgr.h"
#include "COfMsgUtil.h"
#include "CSubnet.h"
#include "CSubnetMgr.h"
#include "CNetwork.h"
#include "CNetworkMgr.h"
#include "CFloatingFlow.h"
#include "CFlowDefine.h"
#include "CFlowMod.h"
#include "CFlowMgr.h"


CFloatingFlowInstall* CFloatingFlowInstall::m_pInstance = NULL;


static void proactive_floating_check_tx_timer(void* param)
{
	if (!CControl::getInstance()->isL3ModeOn())
	{
		return ;
	}

	INT4 ret = 0;
	INT4 flowinstall_flag = 0; 
	LOG_DEBUG("proactive_floating_check_tx_timer - START");
	CFloatingIpMap& floatingIpMap = CFloatingIpMgr::getInstance()->getFloatingIpListMapHeader();

	STL_FOR_LOOP(floatingIpMap, iter)
	{
		if((0 != iter->first)&&(NULL != iter->second)&&(iter->second->getFloatingIp())&&(iter->second->getFixedIp())&&(0 == iter->second->get_flowinstalled()))
		{
			ret = CFloatingFlowInstall::getInstance()->create_proactive_floating_flows_by_floating(iter->second);
			flowinstall_flag = (BNC_OK == ret)? 1:0;
			iter->second->set_flowinstalled(flowinstall_flag);
		}
		
	}

	LOG_DEBUG("proactive_floating_check_tx_timer - STOP");
}
INT4 CFloatingFlowInstall::Init()
{
	const INT1 *value = NULL;
	value = CConf::getInstance()->getConfig("openstack", "hosttrack_times");
    m_FlowInstall_CheckTime = (value == NULL) ? 10: atol(value);
    LOG_DEBUG_FMT("CFloatingFlowInstall time %u", m_FlowInstall_CheckTime);
	
	if(0 == m_FlowInstall_CheckTime)
	{
		LOG_ERROR("CFloatingFlowInstall time is wrong!!!");
		return BNC_ERR;
	}
	return m_FlowInstall_timer.schedule(m_FlowInstall_CheckTime,m_FlowInstall_CheckTime, proactive_floating_check_tx_timer, this);
}


INT4 CFloatingFlowInstall::process_floating_internal_subnet_flow(CSmartPtr<CHost>& fixed_port, CSubnet* subnet, INT4 type)
{
	if (fixed_port.isNull()||(fixed_port->getSw().isNull()) || (NULL == subnet) || (0 == strlen(subnet->get_cidr().c_str()))) 
	{
        return BNC_ERR;
    }

	UINT4 ip = 0;
    UINT4 mask = 0;
    cidr_str_to_ip_prefix(subnet->get_cidr().c_str(), &ip, &mask);
	CNetwork* networkNode = CNetworkMgr::getInstance()->findNetworkById(subnet->get_networkid());
	if ((ip) && (mask) && (mask <=32)) 
	{
        if(networkNode&& (FALSE == networkNode->get_external())) 
		{
            if ((FLOATING_DEL == type) && (CFloatingIpMgr::getInstance()->judge_sw_in_use(fixed_port->getfixIp()))) 
			{
				//by:yhy 如果正在使用中,且要求删除,则返回操作失败
                return BNC_ERR;
            }
           CFlowMgr::getInstance()->install_fabric_floating_internal_subnet_flow(fixed_port->getSw(), type, ip, mask);
        }
        else 
		{
           CFlowMgr::getInstance()->install_fabric_floating_internal_subnet_flow(fixed_port->getSw(), FLOATING_DEL, ip, mask);
        }
    }
	return BNC_OK;
}

INT4 CFloatingFlowInstall::process_proactive_internal_subnet_flow_by_fixed_port(CSmartPtr<CHost>& fixed_port, INT4 type)
{
	if (fixed_port.isNull()) 
	{
		return BNC_ERR;
	}
	
	CSubnet* subnet = NULL;
	CSubnetMap& node_p = CSubnetMgr::getInstance()->getSubnetListMapHead();

	STL_FOR_LOOP(node_p,iter)
	{
		if(NULL != iter->second)
		{
			subnet = iter->second;
			process_floating_internal_subnet_flow(fixed_port, subnet, type);
		}
	}

	return BNC_OK;
}

Base_External * CFloatingFlowInstall::get_externalport_by_floatingip(CFloatingIp* fip)
{
	if(NULL == fip)
	{
		return NULL;
	}

	CRouter*  router =   CRouterGateMgr::getInstance()->FindRouterNodeByHostIp(fip->getFixedIp());
	if(NULL == router)
	{
		//LOG_ERROR_FMT("can't find router fixed_ip=0x%x floating_ip=0x%x !!!", fip->getFixedIp(), fip->getFloatingIp());
		return NULL;
	}
		
	return G_ExternalMgr.targetExternal_Bysubnet_ID(router->getSubnetid());
}

INT4 CFloatingFlowInstall::create_proactive_floating_with_host_flows(CSmartPtr<CHost>& fixed_port, CSmartPtr<CSwitch>& ext_sw, Base_External* ext_port, UINT4 floatingip)
{
	LOG_DEBUG_FMT("floatingip=0x%x !!!",floatingip);
	if(fixed_port.isNull()||(ext_sw.isNull())||(NULL == ext_port))
	{
		return BNC_ERR;
	}
	if(fixed_port->getSw().isNull()||(0 == fixed_port->getPortNo()))
	{
		CSmartPtr<CHost> gateway_p = CHostMgr::getInstance()->getHostGateway(fixed_port);
		if(gateway_p.isNotNull())
		{
			LOG_DEBUG_FMT("gateway_p=0x%x !!!",gateway_p->getIp());
			COfMsgUtil::sendOfp13ArpRequest(gateway_p->getIp(),fixed_port->getIp(),gateway_p->getMac());
		}
		else
		{
			LOG_DEBUG_FMT("controller ip=0x%x !!!",CServer::getInstance()->getControllerIp());
			COfMsgUtil::sendOfp13ArpRequest(CServer::getInstance()->getControllerIp(),fixed_port->getIp(),CServer::getInstance()->getControllerMac());
		}

		return BNC_ERR;
	}

	UINT4 fixed_vlan_id = fixed_port->getSw()->getTag();
    UINT4 ext_vlan_id   = ext_sw->getTag();

    if ((0 == fixed_vlan_id) || (0 == ext_vlan_id)) 
	{
	   LOG_ERROR_FMT("vlan tag can't be 0 !!! fixed_vlan_id=%d  ext_vlan_id=%d",fixed_vlan_id, ext_vlan_id);
       return  BNC_ERR;
    }
	if(ext_port->get_switch_DPID() != fixed_port->getSw()->getDpid())
	{
	//by: 查找从ext_port到fixed_port应该从哪个port出去
    UINT4 outport = CControl::getInstance()->getTopoMgr().get_out_port_between_switch(ext_port->get_switch_DPID(), fixed_port->getSw()->getDpid());
    if (0 == outport) 
	{
		LOG_ERROR("can't find path between ext_port and fixed_port !!!");
        return BNC_ERR;
    }
	if( BNC_ERR == process_proactive_internal_subnet_flow_by_fixed_port(fixed_port, FLOATING_ADD))
	{
		LOG_ERROR("can't process internal subnet flow between fixed_port !!!");
        return BNC_ERR;
	}

	
    CFlowMgr::getInstance()->install_proactive_floating_host_to_external_flow(fixed_port->getSw(), FLOATING_ADD, fixed_port->getfixIp(), fixed_port->getMac(), floatingip, ext_port->get_gateway_MAC(), ext_vlan_id);

	//by: 下发流表(匹配IP为g_reserve_ip的包上送控制器)
    if(BNC_ERR == CFlowMgr::getInstance()->install_add_fabric_controller_flow(fixed_port->getSw()))
	{
		LOG_ERROR("can't install flow to controller !!!");
        return BNC_ERR;
	}

	
	//by: 下发浮动ip相关包添加vlan流表(此处是对pica8下达)
    CFlowMgr::getInstance()->install_floatingip_set_vlan_in_flow(ext_sw, floatingip, fixed_port->getfixIp(), fixed_port->getMac(), fixed_vlan_id, outport);


	if(BNC_ERR == CFlowMgr::getInstance()->install_add_FloatingIP_ToFixIP_OutputToHost_flow(fixed_port, floatingip))
	{
		LOG_ERROR("can't install flow for floating to fix host !!!");
        return BNC_ERR;
		}
	}
	else
	{
	}
	//by: 在fixed_port所在交换机上下发匹配fixed_port的mac的转发至对应port的流表
    CFlowMgr::getInstance()->install_fabric_output_flow(fixed_port->getSw(), fixed_port->getMac(), fixed_port->getPortNo());
	LOG_DEBUG("create_proactive_floating_with_host_flows");
	return BNC_OK;
}
INT4 CFloatingFlowInstall::create_proactive_floating_flows_by_floating(CFloatingIp* fip)
{
	INT4 ret = BNC_ERR;
	if(NULL == fip)
	{
		return BNC_ERR;
	}

	Base_External *epp = get_externalport_by_floatingip(fip);
	if(NULL == epp)
	{
		//LOG_ERROR("can't find external port!!!");
		return BNC_ERR;
	}

	CSmartPtr<CSwitch>  ext_sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(epp->get_switch_DPID());
	if(ext_sw.isNull())
	{
		LOG_ERROR_FMT("can't find external switch!!! floatingip=0x%x fixip=0x%x ", fip->getFloatingIp(), fip->getFixedIp());
		return BNC_ERR;
	}

	CSmartPtr<CHost> fix_port = CHostMgr::getInstance()->findHostByIp(fip->getFixedIp());
	if(fix_port.isNull())
	{
		LOG_ERROR_FMT("can't find host by floating ip 0x%x !!!", fip->getFloatingIp());
		return BNC_ERR;
	}
	if (bnc::host::HOST_NORMAL == fix_port->getHostType()) 
	{
		ret = create_proactive_floating_with_host_flows(fix_port, ext_sw, epp, fip->getFloatingIp());
	}
	
	return ret;
}
INT4 CFloatingFlowInstall::remove_proactive_floating_flows_by_floating(CFloatingIp* fip)
{
	if(NULL == fip)
	{
		return BNC_ERR;
	}
LOG_DEBUG_FMT("fip->getFloatingIp()=0x%x",fip->getFloatingIp());
	Base_External* ext_port = NULL;
	CSmartPtr<CHost> fixed_port = CHostMgr::getInstance()->findHostByIp(fip->getFixedIp());
	if(fixed_port.isNull())
	{
		return BNC_ERR;
	}
	ext_port = G_ExternalMgr.targetExternal_Bysubnet_ID(fixed_port->getSubnetId());
	if(NULL == ext_port )
	{
		return BNC_ERR;
	}

	CSmartPtr<CSwitch> ext_sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(ext_port->get_switch_DPID());
	if(ext_sw.isNotNull())
	{
		CFlowMgr::getInstance()->delete_fabric_flow_by_ip(ext_sw, fip->getFloatingIp(), FABRIC_TABLE_QOS_INVM);
	}

	if((bnc::host::HOST_NORMAL == fixed_port->getHostType())&&fixed_port->getSw().isNotNull())
	{
		UINT1 zero_mac[6] = {0};
		CFlowMgr::getInstance()->install_proactive_floating_host_to_external_flow(fixed_port->getSw(), FLOATING_DEL, fixed_port->getfixIp(), fixed_port->getMac(), fip->getFloatingIp(), zero_mac, 0);
		CFlowMgr::getInstance()->install_remove_FloatingIP_ToFixIP_OutputToHost_flow(fixed_port, fip->getFloatingIp());
	}
	
	fip->set_flowinstalled(0);
	
	return 0;
}


CFloatingFlowInstall* CFloatingFlowInstall::getInstance()
{
	if(NULL == m_pInstance)
	{
		m_pInstance = new CFloatingFlowInstall();
		if(NULL == m_pInstance)
		{
			exit(-1);
		}
		m_pInstance->Init();
	}
	return m_pInstance;
}

#include "log.h"
#include "comm-util.h"
#include "COpenstackFloatingip.h"
#include "COpenstackMgr.h"
#include "BaseFloating.h"
#include "BaseFloatingManager.h"
#include "CFloatingIp.h"
#include "CFloatingIpMgr.h"
#include "CNotifyFloatingIp.h"

CNotifyFloatingIp::CNotifyFloatingIp()
{
	
}

CNotifyFloatingIp::~CNotifyFloatingIp()
{
	
}

void CNotifyFloatingIp::notifyAddFloatingIp(COpenstackFloatingip * floatingip)
{
	LOG_ERROR_FMT("%s %d", FN, LN);
	if(NULL == floatingip)
		return;
	CFloatingIp * floatingipNode = new CFloatingIp(floatingip->getFloatingNetId(), ip2number(floatingip->getFixedIp().c_str()), ip2number(floatingip->getFloatingIp().c_str()));
	floatingipNode->set_flowinstalled(0);
	floatingipNode->set_checkstatus(0);
	CFloatingIpMgr::getInstance()->addFloatingIpNode(floatingipNode);
	LOG_ERROR_FMT("floatingip->getFloatingIp=%s floatingip->getFixedIp()=%s",floatingip->getFloatingIp().c_str(),floatingip->getFixedIp().c_str());
}

void CNotifyFloatingIp::notifyDelFloatingIp(const  string & floatingip_id) 
{
	LOG_ERROR_FMT("%s %d", FN, LN);
	COpenstackFloatingip * floatingip = COpenstackMgr::getInstance()->getOpenstack()->getResource()->findOpenstackFloatingip(floatingip_id);
	if(NULL != floatingip)
	{
		CFloatingIpMgr::getInstance()->delFloatingIpNode(ip2number(floatingip->getFloatingIp().c_str()));
	}
	Base_Floating* basefloatingip = G_FloatingMgr.targetFloating_ByID(floatingip_id);
	LOG_ERROR_FMT("floatingip_id=%s basefloatingip=0x%p", floatingip_id.c_str(), basefloatingip);
	if(NULL != basefloatingip)
	{
		CFloatingIpMgr::getInstance()->delFloatingIpNode(basefloatingip->get_floating_IP());
	}
}

void CNotifyFloatingIp::notifyUpdateFloatingIp(COpenstackFloatingip * floatingip)
{
	LOG_INFO_FMT("%s %d", FN, LN);
	if(NULL == floatingip)
		return;
	CFloatingIp * floatingipNode = CFloatingIpMgr::getInstance()->findFloatingIpNodeByfloatingIp(ip2number(floatingip->getFloatingIp().c_str()));
	if(NULL != floatingipNode)
	{
		floatingipNode->setFixedIp(ip2number(floatingip->getFixedIp().c_str()));
		//LfloatingipNode->setFloatingIp(ip2number(floatingip->getFloatingIp().c_str()));
		
		LOG_ERROR_FMT("LfloatingipNode->getFloatingIp=0x%x LfloatingipNode->getFixedIp()=0x%x",floatingipNode->getFloatingIp(),floatingipNode->getFixedIp());
	}
	
}

void CNotifyFloatingIp::notifyAddFloatingIp(Base_Floating * floatingip)
{
	LOG_INFO_FMT("%s %d", FN, LN);
	if(NULL == floatingip)
		return;
	CFloatingIp * floatingipNode = new CFloatingIp(floatingip->get_network_ID(), floatingip->get_fixed_IP(), floatingip->get_floating_IP());
	floatingipNode->set_flowinstalled(0);
	floatingipNode->set_checkstatus(0);
	CFloatingIpMgr::getInstance()->addFloatingIpNode(floatingipNode);
	LOG_INFO_FMT("floatingip->get_floating_IP=0x%x floatingip->get_fixed_IP()=0x%x",floatingip->get_floating_IP(),floatingip->get_fixed_IP());
}

void CNotifyFloatingIp::notifyUpdateFloatingIp(Base_Floating * floatingip)
{
	
	LOG_INFO_FMT("%s %d", FN, LN);
	if(NULL == floatingip)
		return;
	CFloatingIp * floatingipNode = CFloatingIpMgr::getInstance()->findFloatingIpNodeByfloatingIp(floatingip->get_floating_IP());
	if(NULL != floatingipNode)
	{
		floatingipNode->setFixedIp(floatingip->get_fixed_IP());
		//floatingipNode->setFloatingIp(floatingip->get_floating_IP());
		
		LOG_INFO_FMT("floatingip->getFloatingIp=0x%x floatingip->getFixedIp()=0x%x",floatingipNode->getFloatingIp(),floatingipNode->getFixedIp());
	}
	
}


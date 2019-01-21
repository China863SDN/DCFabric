/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2016, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the BNC SDN Controller. BNC SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
*                                                                             *
*   File Name   : COpenstackExternalMgr.cpp *
*   Author      : bnc mmzhang               *
*   Create Date : 2016-9-18                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "COpenstackExternalMgr.h"
#include "log.h"
#include "CConf.h"
#include "CServer.h"
#include "CControl.h"
#include "openflow-common.h"
#include "comm-util.h"
#include "CHostMgr.h"
#include "COpenstackMgr.h"


static const UINT4 external_min_seq = 1;
static const UINT4 external_max_seq = 20;
COpenstackExternalMgr* COpenstackExternalMgr::m_pInstance = 0;

//默认发送Request时间间隔
static const INT4 DEFAULT_EXTERNAL_REQUEST_INTERVAL = 300;
//Request发送间�?
static INT4 iRequestExternalInterval;

UINT1 BROADCASTMAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

COpenstackExternalMgr::COpenstackExternalMgr()
{

}

COpenstackExternalMgr::~COpenstackExternalMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

COpenstackExternalMgr* COpenstackExternalMgr::getInstance()
{
	if ( NULL == m_pInstance )
	{
		m_pInstance = new COpenstackExternalMgr();
		m_pInstance->init();
	}
	return m_pInstance;
}

void COpenstackExternalMgr::init()
{
    if (COpenstackMgr::getInstance()->getOpenstack())
    {
        LOG_INFO("Start External manager.");
        m_pInstance->loadExternalConfig();
        //m_pInstance->loadExternalResource();

        const INT1* interval = CConf::getInstance()->getConfig("openstack", "external_request_interval");
        iRequestExternalInterval = (NULL == interval) ? DEFAULT_EXTERNAL_REQUEST_INTERVAL : atoi(interval);
        //起线程定时询问external的mac地址以便更新
        pthread_create(&m_externalThreadId,NULL,requestExternalMacPeriodically,(void*)this);

    }
}

void* COpenstackExternalMgr::requestExternalMacPeriodically(void* param)
{
    //setCpuAffinity(CServer::getInstance()->getCpuId());

	prctl(PR_SET_NAME, (unsigned long)"ReqExtMac");  

    while (TRUE)
    {
        COpenstackExternalMgr::getInstance()->requestExternalListPeriodically(param);
        sleep(iRequestExternalInterval);
    }
    return NULL;
}

void* COpenstackExternalMgr::requestExternalListPeriodically(void* param)
{
    m_oMutex.lock();
    std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
    for (;iter != m_listOpenstackExternal.end();++iter)
    {
       // arp_t arp_pkt;
        //CEtherArpHandler::createArpPacket(&arp_pkt,1,(*iter)->getExternalOuterInterfaceMac(),BROADCASTMAC,
         //       (*iter)->getExternalOuterInterfaceIp(),(*iter)->getExternalGatewayIp());
        CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid((*iter)->getExternalDpid());
        INT1 ret[1024] = {0};
        dpidUint8ToStr((*iter)->getExternalDpid(),ret);
        LOG_INFO_FMT(" external dpid is %s",ret);
        if (sw.isNull())
        {
            LOG_INFO("can not find sw");
        }
        else
        {
            INT1 ret[1024] = {0};
            dpidUint8ToStr(sw->getDpid(),ret);
            LOG_INFO_FMT(" external dpid is %s",ret);
            LOG_INFO_FMT(" sw tag is %d",sw->getTag());
            LOG_INFO_FMT("external portno is %d",(*iter)->getExternalPort());
        }
        if (sw.isNotNull() && (*iter)->getExternalPort() != 0)
        {
            LOG_INFO("transmit packet");
           // COfPacketInHandler::forward(sw, (*iter)->getExternalPort(), sizeof(arp_pkt), &arp_pkt);
        }
    }
    m_oMutex.unlock();
    return NULL;
}

void COpenstackExternalMgr::loadExternalConfig()
{
    LOG_INFO("read OpenstackExternal config.");
    UINT4 seq_num = 0;
	for (seq_num = external_min_seq ; seq_num <= external_max_seq; seq_num++)
	{
		std::string subnet_id;
		UINT4 external_gateway_ip = 0;
		UINT4 external_outer_interface_ip = 0;
		UINT1 external_outer_interface_mac[6] = {0};
		UINT1 external_gateway_mac[6] = {0};
		UINT8 external_dpid = 0;
		UINT4 external_port = 0;

		char para_title[48] = {0};
		sprintf(para_title, "external_switch_%d", seq_num);
//		LOG_INFO_FMT("para title is %s",para_title);


		//读取external gateway ip
//		INT1 ret[1024] = {0};
		const INT1* result = CConf::getInstance()->getConfig(para_title,"external_gateway_ip");
		external_gateway_ip = (NULL == result) ? 0 : ip2number(result);
//		number2ip(external_gateway_ip,ret);
//		LOG_INFO_FMT("external gateway ip is %s",ret);

		//读取external_gateway_mac
		result = CConf::getInstance()->getConfig(para_title,"external_gateway_mac");
		if (result)
		{
			memset(external_gateway_mac, 0, 6);
			macstr2hex(result, external_gateway_mac);
//			memset(ret, 0, 6);
//			mac2str(external_gateway_mac,ret);
//			LOG_INFO_FMT("external gateway mac is %s",ret);
		}
		else
		{
			memset(external_gateway_mac, 0, 6);
		}

		//读取external_outer_interface_ip
		result = CConf::getInstance()->getConfig(para_title,"external_outer_interface_ip");
		external_outer_interface_ip = (NULL == result) ? 0 : ip2number(result);
//		LOG_INFO_FMT("external outer interface ip is %d",external_outer_interface_ip);
//		number2ip(external_outer_interface_ip,ret);
//		LOG_INFO_FMT("external_outer_interface_ip is %s",ret);

		//读取external_outer_interface_mac
		result = CConf::getInstance()->getConfig(para_title,"external_outer_interface_mac");
		if (result)
		{
			memset(external_outer_interface_mac, 0, 6);
			macstr2hex(result, external_outer_interface_mac);
//			memset(ret, 0, 6);
//			mac2str(external_outer_interface_mac,ret);
//			LOG_INFO_FMT("external interface mac is %s",ret);
		}
		else
		{
			memset(external_outer_interface_mac, 0, 6);
		}

		//读取external_dpid
		result = CConf::getInstance()->getConfig(para_title,"external_dpid");
		dpidStr2Uint8(result,&external_dpid);
//		UINT1 dpid_tmp[8] = {0};
//		memset(ret, 0, 8);
//		dpidUint8ToStr(external_dpid,ret);
//		LOG_INFO_FMT("read the external dpid result is %s",ret);


		//读取external_port
		result = CConf::getInstance()->getConfig(para_title,"external_port");
		external_port = (NULL == result) ? 0 : atoi(result);
		//LOG_INFO_FMT("external port is %d", external_port);

		//读取subnet_id
        result = CConf::getInstance()->getConfig(para_title,"external_subnet_id");
        if (NULL != result)
        {
            subnet_id = result;
        }

        //LOG_INFO_FMT("read the external subnet id result is %s",subnet_id.c_str());


		addOpenstackExternal(subnet_id,external_gateway_ip,external_outer_interface_ip,
				external_gateway_mac,external_outer_interface_mac,external_dpid,external_port);

	}
}


//void COpenstackExternalMgr::loadExternalResource()
//{
//    m_oMutex.lock();
//    std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
//    for (;iter != m_listOpenstackExternal.end();++iter)
//    {
//        LOG_INFO("load external config.");
//
//        std::string subnet_id;
//        UINT4 external_gateway_ip = 0;
//        UINT1 external_outer_interface_mac[6] = {0};
//        UINT1 external_gateway_mac[6] = {0};
//        UINT8 external_dpid = 0;
//        UINT4 external_port = 0;
//
//        BOOL ret = COpenstackMgr::getInstance()->getExternalInfo((*iter)->getExternalOuterInterfaceIp(),
//                                                            external_outer_interface_mac,
//                                                            external_gateway_ip,subnet_id);
//        if (ret)
//        {
//            updateOpenstackExternal((*iter),subnet_id,external_gateway_ip,
//                            external_gateway_mac,external_outer_interface_mac,external_dpid,external_port);
//        }
//    }
//    m_oMutex.unlock();
//    return;
//}


COpenstackExternal* COpenstackExternalMgr::findOpenstackExternalByOuterIp(const UINT4 outerInterfaceIp)
{
    if (outerInterfaceIp)
    {
        m_oMutex.lock();
        std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
        for (;iter != m_listOpenstackExternal.end();++iter)
        {
            if ((*iter)->getExternalOuterInterfaceIp() == outerInterfaceIp)
            {
                m_oMutex.unlock();
                return *iter;
            }

        }
        m_oMutex.unlock();
    }
	return NULL;
}

COpenstackExternal* COpenstackExternalMgr::findOpenstackExternalByOuterMac(const UINT1* outerInterfaceMac)
{
    if (outerInterfaceMac)
    {
        m_oMutex.lock();
        std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
        for (;iter != m_listOpenstackExternal.end();++iter)
        {
            if (memcmp((*iter)->getExternalOuterInterfaceMac(),outerInterfaceMac,6))
            {
                m_oMutex.unlock();
                return *iter;
            }
        }
        m_oMutex.unlock();
    }
	return NULL;
}

COpenstackExternal* COpenstackExternalMgr::findOpenstackExternalBySubnetId(const std::string& subnetid)
{
    if (!subnetid.empty())
    {
        m_oMutex.lock();
        std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
        for (;iter != m_listOpenstackExternal.end();++iter)
        {
            if (subnetid == (*iter)->getExternalSubnetId())
            {
                m_oMutex.unlock();
                return *iter;
            }
        }
        m_oMutex.unlock();
    }
    return NULL;
}

COpenstackExternal* COpenstackExternalMgr::findOpenstackExternalAny()
{
    m_oMutex.lock();
    std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
    for (;iter != m_listOpenstackExternal.end();++iter)
    {
        m_oMutex.unlock();
        return *iter;
    }
    m_oMutex.unlock();

    return NULL;
}



void COpenstackExternalMgr::addOpenstackExternal( std::string external_subnet_id,
		UINT4 external_gateway_ip,
		UINT4 external_outer_interface_ip,
		UINT1 external_gateway_mac[6],
		UINT1 external_outer_interface_Mac[6],
		UINT8 external_dpid,
		UINT4 external_port)
{
	// 判断external_outer_ip是否�?
	if (external_outer_interface_ip)
	{
		COpenstackExternal* ret = findOpenstackExternalByOuterIp(external_outer_interface_ip);
		if (ret)
		{
			ret = updateOpenstackExternal(ret,external_subnet_id,external_gateway_ip,
				external_gateway_mac,external_outer_interface_Mac,external_dpid,external_port);
		}
		else
		{
			ret = createOpenstackExternal(external_subnet_id,external_gateway_ip,external_outer_interface_ip,
				external_gateway_mac,external_outer_interface_Mac,external_dpid,external_port);
		}
	}

}

void COpenstackExternalMgr::removeOpenstackExternalByOuterIp(const UINT4 outerInterfaceIp)
{
    if (outerInterfaceIp)
    {
        m_oMutex.lock();
        std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
        for (; iter != m_listOpenstackExternal.end(); ++iter)
        {
           if ((*iter)->getExternalOuterInterfaceIp() == outerInterfaceIp)
           {
               m_listOpenstackExternal.remove(*iter);
               break;
           }
        }
        m_oMutex.unlock();
    }
}

void COpenstackExternalMgr::removeOpenstackExternalBySubnetId(const std::string subnetid)
{
    if (!subnetid.empty())
    {
        m_oMutex.lock();
        std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
        for (; iter != m_listOpenstackExternal.end(); ++iter)
        {
           if ((*iter)->getExternalSubnetId() == subnetid)
           {
               m_listOpenstackExternal.remove(*iter);
               break;
           }
        }
        m_oMutex.unlock();
    }
}



COpenstackExternal* COpenstackExternalMgr::createOpenstackExternal(std::string external_subnet_id,
		UINT4 external_gateway_ip,
		UINT4 external_outer_interface_ip,
		UINT1 external_gateway_mac[6],
		UINT1 external_outer_interface_Mac[6],
		UINT8 external_dpid,
		UINT4 external_port)
{
	COpenstackExternal* ret = new COpenstackExternal(external_subnet_id,
			 external_gateway_ip,
			 external_outer_interface_ip,
			 external_gateway_mac,
			 external_outer_interface_Mac,
			 external_dpid,
			 external_port,
			 FALSE);
	if (ret)
	{
	    m_oMutex.lock();
		ret->isValid();
		m_listOpenstackExternal.push_back(ret);
		m_oMutex.unlock();
		return ret;
	}
	return NULL;
}

COpenstackExternal* COpenstackExternalMgr::updateOpenstackExternal(COpenstackExternal* openstackExternal,
		  	  	  	std::string external_subnet_id,
					UINT4 external_gateway_ip,
					UINT1 external_gateway_mac[6],
					UINT1 external_outer_interface_Mac[6],
					UINT8 external_dpid,
					UINT4 external_port)
{
	UINT1 ext_zero_mac[6] = {0};
	if (!external_subnet_id.empty())
	{
		 openstackExternal->setExternalSubnetId(external_subnet_id);
	}
	if (external_gateway_ip)
	{
		openstackExternal->setExternalGatewayIp(external_gateway_ip);
	}
	if ((external_gateway_mac) && (0 != memcmp(ext_zero_mac,external_gateway_mac, 6)))
	{
		openstackExternal->setExternalGatewayMac(external_gateway_mac);
	}
	if ((external_outer_interface_Mac) && (0 != memcmp(ext_zero_mac,external_outer_interface_Mac, 6)))
	{
		openstackExternal->setExternalOuterInterfaceMac(external_outer_interface_Mac);
	}
	if (external_dpid)
	{
		openstackExternal->setExternalDpid(external_dpid);
	}
	if (external_port)
	{
		openstackExternal->setExternalPort(external_port);
	}
	openstackExternal->isValid();
	return openstackExternal;
}

void COpenstackExternalMgr::updateOpenstackExternalBySubnetId(std::string external_subnet_id,UINT4 external_gateway_ip)
{
    if (!external_subnet_id.empty() && external_gateway_ip)
    {
        m_oMutex.lock();
        std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
        for (;iter != m_listOpenstackExternal.end();++iter)
        {
           if ((*iter)->getExternalSubnetId() == external_subnet_id)
           {
              (*iter)->setExternalGatewayIp(external_gateway_ip);
              (*iter)->isValid();
           }
        }
        m_oMutex.unlock();
    }
}

void COpenstackExternalMgr::updateOpenstackExternalAll(UINT4 external_gateway_ip,UINT1* external_gateway_mac)
{
    if ((0 == external_gateway_ip) || (NULL == external_gateway_mac) || (m_listOpenstackExternal.empty()))
    {
        return ;
    }

    m_oMutex.lock();
    std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
    for (;iter != m_listOpenstackExternal.end();++iter)
    {
        if ((*iter) && ((*iter)->getExternalGatewayIp() == external_gateway_ip))
        {
            updateOpenstackExternal((*iter), std::string(""),0,external_gateway_mac,0,0,0);
        }
    }
    m_oMutex.unlock();
    return;
}

BOOL COpenstackExternalMgr::isExternalSwitch(UINT8 dpid)
{
    BOOL ret = FALSE;

    m_oMutex.lock();
    std::list<COpenstackExternal*>::iterator iter = m_listOpenstackExternal.begin();
    for (;iter != m_listOpenstackExternal.end();++iter)
    {
        if ((*iter) && ((*iter)->getExternalDpid() == dpid))
        {
            ret = TRUE;
        }
    }
    m_oMutex.unlock();

    return ret;
}



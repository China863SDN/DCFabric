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
*   File Name   : COpenstackMgr.cpp         *
*   Author      : bnc xflu                  *
*   Create Date : 2016-8-31                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include <sys/prctl.h> 
#include "log.h"
#include "comm-util.h"
#include "COpenstackMgr.h"
#include "bnc-type.h"
#include "CConf.h"
#include "CServer.h"
#include "CControl.h"
#include "COpenstack.h"
#include "COpenstackResource.h"

COpenstackMgr* COpenstackMgr::m_pInstance = 0;
static UINT4 m_iReloadInterval;                 ///< 重新读取Openstack信息间隔

COpenstackMgr::COpenstackMgr()
:m_openstack(NULL)
{
}

COpenstackMgr::~COpenstackMgr()
{
	if(NULL != m_pInstance)
	{
		delete  m_pInstance;
		m_pInstance = NULL;
	}
}

COpenstackMgr* COpenstackMgr::getInstance()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new COpenstackMgr();
        m_pInstance->init();
    }

    return m_pInstance;
}

void COpenstackMgr::init()
{
	INT4 openstack_on = 0;
    // 读取openstack启动信息
    const INT1* result =  CConf::getInstance()->getConfig("openstack", "openstack_on");
    openstack_on = (result == NULL) ? 0 : atoi(result);

    LOG_INFO_FMT("Initialize Openstack Manager: %d", openstack_on);

    // 如果openstack服务启动
    if (openstack_on&&CControl::getInstance()->isL3ModeOn())
    {
        LOG_INFO("start openstack");

        UINT4 server_ip;
        UINT4 server_port_no;
        UINT4 token_ip;
        UINT4 token_port_no;
        INT1 tenant_name[40] = {0};
        INT1 user_name[40] = {0};
        INT1 password[40] = {0};

        // 读取openstack服务器ip
        result = CConf::getInstance()->getConfig("openstack", "openstack_ip");
        server_ip = (result == NULL) ? 0 : ip2number(result);

        // 读取openstack服务器端口
        result = CConf::getInstance()->getConfig("openstack", "openstack_port");
        server_port_no = (result == NULL) ? 0 : atoi(result);

        // 读取openstack服务器ip
        result = CConf::getInstance()->getConfig("openstack", "token_ip");
        token_ip = (result == NULL) ? 0 : ip2number(result);

        // 读取openstack服务器端口
        result = CConf::getInstance()->getConfig("openstack", "token_port");
        token_port_no = (result == NULL) ? 0 : atoi(result);

        // 读取openstack信息
        result = CConf::getInstance()->getConfig("openstack", "tenant_name");
        if (NULL != result)
        {
            memcpy(tenant_name, result, 40);
        }

        // 读取openstack信息
        result = CConf::getInstance()->getConfig("openstack", "user_name");
        if (NULL != result)
        {
            memcpy(user_name, result, 40);
        }

        // 读取openstack信息
        result = CConf::getInstance()->getConfig("openstack", "password");
        if (NULL != result)
        {
            memcpy(password, result, 40);
        }

        // 创建openstack对象
        m_openstack = new COpenstack(server_ip, server_port_no, token_ip, token_port_no,
                std::string(tenant_name), std::string(user_name), std::string(password));

        // 初始化openstack
        m_openstack->init();

        // 读取openstack定时更新配置
        result = CConf::getInstance()->getConfig("openstack", "reload_interval");
        m_iReloadInterval = (NULL == result) ? 0 : atoi(result);

        // 如果配置了定时更新, 启动openstack信息定时更新线程
        pthread_create(&m_reloadThreadId, NULL, reloadResource, NULL);

        // 注册Openstack RestApi
    }
}


void* COpenstackMgr::reloadResource(void* param)
{
	prctl(PR_SET_NAME, (unsigned long)"ReloadResource");  

    // 如果有设置
    if (m_iReloadInterval)
    {
        // 定时读取信息
        while (TRUE)
        {
            COpenstackMgr::getInstance()->getOpenstack()->loadResources();
            sleep(m_iReloadInterval);
        }
    }
    // 如果没有设置
    else
    {
        // 只读取一次
        COpenstackMgr::getInstance()->getOpenstack()->loadResources();
    }

    return NULL;
}


//BOOL COpenstackMgr::getExternalInfo(const UINT4 outer_ip,UINT1* outer_Mac,UINT4& gateway_ip,std::string& subnet_id )
//{
//    if(outer_ip)
//    {
//        COpenstackPort* port = getOpenstack()->getResource()->findOpenstackPortByIp(outer_ip);
//        if (port)
//        {
//            port->getMac(outer_Mac);
//            subnet_id = port->getFixedFristSubnetId();
//            COpenstackSubnet* subnet = getOpenstack()->getResource()->findOpenstackSubnet(subnet_id);
//            gateway_ip = ip2number(subnet->getGatewayIp().c_str());
//            return TRUE;
//        }
//    }
//    return FALSE;
//}



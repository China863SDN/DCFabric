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
*   File Name   : dcfabric.cpp           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "openflow-common.h"
#include "dcfabric.h"
#include "CConf.h"
#include "CControl.h"
#include "CServer.h"
#include "log.h"
#include "COpenstackMgr.h"
#include "CProxyRes.h"
#include "NetworkingPersistence.h"
#include "Service_ExternalDetecting.h"
#include "CClusterService.h"
#include "CFirewallPolicy.h"
#include "CPortforwardPolicy.h"
#include "CQosPolicyNotifier.h"
#include "SwitchManageMode.h"
#include "SwitchManagePresistence.h"
#include "CHostMgr.h"
#include "CFlowMgr.h"

#define PRODUCT "DCFabric"
#ifndef VERSION
#define VERSION 0x020101
#endif
#define START_DATE __DATE__  // compile date.
#define START_TIME __TIME__  // compile time.

static void showCopyRight()
{
    INT1 stars[] = {"*****************************************************************" };
    INT1 spaces[] = {"*                                                               *" };
    INT1 ver[81], info[81], author[81], company[81], copyright[81];
    UINT4 len = strlen(stars);

    sprintf(ver, "*                        %s v%d.%d.%d                         *", PRODUCT,
            (int)(VERSION & 0xFF0000) >> 16, (int) (VERSION & 0xFF00) >> 8, (int) (VERSION & 0xFF));

    if (strlen(ver) != len)
    {
        ver[len - 1] = '*';
        ver[len] = '\0';
    }

    sprintf(info, "*               Started at %8s, on%12s             *", START_TIME, START_DATE);
    sprintf(author, "*             BNC: ZhaoLiangZhi, YangLei                  *");
    sprintf(company, "*          Copyright BNC 2017-10 ~ 2099-10           *");
    sprintf(copyright, "*                  (c) All Rights Reserved.                     *");

    printf("\n");
    printf("%s\n", stars);
    printf("%s\n", spaces);
    printf("%s\n", ver);
    printf("%s\n", info);
    printf("%s\n", spaces);
    printf("%s\n", author);
    printf("%s\n", spaces);
    printf("%s\n", company);
    printf("%s\n", copyright);
    printf("%s\n", stars);
    printf("\n");

    LOG_INFO_FMT("%s\n", stars);
    LOG_INFO_FMT("%s\n", spaces);
    LOG_INFO_FMT("%s\n", ver);
    LOG_INFO_FMT("%s\n", info);
    LOG_INFO_FMT("%s\n", spaces);
    LOG_INFO_FMT("%s\n", author);
    LOG_INFO_FMT("%s\n", spaces);
    LOG_INFO_FMT("%s\n", company);
    LOG_INFO_FMT("%s\n", copyright);
    LOG_INFO_FMT("%s\n", stars);
}

static void startServer()
{
    //TBD

    while (1)
    {
        sleep(30);
    }
}

int main(int argc, char** argv)
{
    showCopyRight();

    //initLogger();

    if (BNC_OK != CConf::getInstance()->loadConf())
	{
		LOG_ERROR("load Conf file failed!");
        return -1;
	}

    if (CLog::getInstance()->init() != BNC_OK)
    {
        LOG_ERROR("init CLog failed!");
        return -1;
    }
	if(SwitchManagePresistence::GetInstance()->read_switch_manage_config() != BNC_OK)
	{
		LOG_ERROR("read switch manage section failed!");
        //return -1;
	}

#if 0
    if (CSyncMgr::getInstance()->init() != BNC_OK)
    {
        LOG_WARN("init CSyncMgr failed!");
        //return -1;
    }
    if (CClusterMgr::getInstance()->init() != BNC_OK)
    {
        LOG_WARN("init CClusterMgr failed!");
        //return -1;
    }
#endif

    if (CServer::getInstance()->init() != BNC_OK)
    {
        LOG_ERROR("init CServer failed!");
        return -1;
    }

    if (CControl::getInstance()->init() != BNC_OK)
    {
        LOG_ERROR("init CControl failed!");
        return -1;
    }

    if (CHostMgr::getInstance()->init() != BNC_OK)
    {
        LOG_WARN("init CHostMgr failed!");
        //return -1;
    }

    if (CClusterService::getInstance()->init() != BNC_OK)
    {
        LOG_WARN("init CClusterService failed!");
        //return -1;
    }

    if (CProxyResMgr::getInstance()->init() != BNC_OK)
    {
        LOG_ERROR("Openstack server init failure.");
        return -1;
    }

    if (NetworkingPersistence::Get_Instance()->init() != BNC_OK)
    {
        LOG_WARN("init NetworkingPersistence failed!");
        //return -1;
    }

	Service_ExternalDetecting::Get_Instance()->init();

    if (CFirewallPolicy::getInstance()->init() != BNC_OK)
    {
        LOG_WARN("init CFirewallPolicy failed!");
        //return -1;
    }
    if (CPortforwardPolicy::getInstance()->init() != BNC_OK)
    {
        LOG_WARN("init CPortforwardPolicy failed!");
        //return -1;
    }
	if (CQosPolicyNotifier::GetInstance()->init() != BNC_OK)
    {
        LOG_WARN("init CQosPolicyNotifier failed!");
        //return -1;
    }
	if (CFlowMgr::getInstance()->init() != BNC_OK)
    {
        LOG_WARN("init CFlowMgr failed!");
        //return -1;
    }

    startServer();

    return 0;
}

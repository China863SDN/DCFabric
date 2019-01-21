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
*   File Name   : CHostTrack.cpp           *
*   Author      : bnc cyyang           *
*   Create Date : 2016-7-6           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CConf.h"
#include "CTimer.h"
#include "COfMsgUtil.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "CControl.h"
#include "CHostTrack.h"
#include "CServer.h"

const static INT4 TRACK_INTERVAL_SECOND = 30;

static void track(void* param)
{
	if (!CControl::getInstance()->isL3ModeOn())
	{
		return;
	}

	CHostList& hostlist = CHostMgr::getInstance()->getHostList();
	STL_FOR_LOOP(hostlist, iter)
	{
        CSmartPtr<CHost> host = *iter;
		if (host.isNotNull()
            && (bnc::host::HOST_NORMAL == host->getHostType())
            && ((host->getSw().isNull()) || (0 == host->getPortNo())))
		{
			CSmartPtr<CHost> gateway = CHostMgr::getInstance()->getHostGateway(host);
			if (gateway.isNotNull())
			{
				COfMsgUtil::sendOfp13ArpRequest(gateway->getIp(), host->getIp(), gateway->getMac());
			}
			else
			{
				COfMsgUtil::sendOfp13ArpRequest(CServer::getInstance()->getControllerIp(), 
                    host->getIp(), CServer::getInstance()->getControllerMac());
			}
		}
	}
}

CHostTrack::CHostTrack()
{
}

CHostTrack::~CHostTrack()
{
}

INT4 CHostTrack::init()
{
	const INT1* pConf = CConf::getInstance()->getConfig("openstack", "host_track_interval");
    INT4 interval = (pConf == NULL) ? TRACK_INTERVAL_SECOND: atol(pConf);
    LOG_INFO_FMT("host_track_interval %d", interval);

	if (0 == interval)
        return BNC_ERR;

	return m_timer.schedule(interval, interval, track, this);
}


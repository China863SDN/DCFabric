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
*   File Name   : CHostMgr.h		*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-27         *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#ifndef _CHOSTMGR_H
#define _CHOSTMGR_H

#include <list>
#include "CHost.h"
#include "CHostDefine.h"
#include "COpenstackDefine.h"
#include "CHostTrack.h"
#include "CSmartPtr.h"

typedef std::list<CSmartPtr<CHost> > CHostList;

class CHostMgr
{
public:
	static CHostMgr* getInstance();
	
	~CHostMgr();

	INT4 init();

	CSmartPtr<CHost> addHost(CSmartPtr<CSwitch> sw, 
                             UINT4 portNo, 
                             const UINT1* mac, 
                             UINT4 ip, 
                             UINT8 dpid);
	CSmartPtr<CHost> addHost(INT4 type,
                             const UINT1* mac,
                             UINT4 ip,
                             const std::string & tenant_id,
                             const std::string & network_id,
                             const std::string & subnet_id,
                             const std::string & port_id);
    CSmartPtr<CHost> addHost(INT4 type,
                             const UINT1* mac,
                             const std::vector<UINT4> & ipList,
                             //UINT4 ip,
                             UINT8 dpid,
                             UINT4 port,
                             const std::string & subnetId,
                             const std::string & tenantId,
                             UINT4 fixedIp);
	BOOL delHostByMac(const UINT1* mac);
	BOOL delHostBySw(const CSmartPtr<CSwitch> sw);
	CSmartPtr<CHost> findHostByMac(const UINT1* mac);
	CSmartPtr<CHost> findHostByIp(UINT4 ip);
	CSmartPtr<CHost> findHostByIPAndMac(const UINT4 ip, const UINT1* mac);
	CSmartPtr<CHost> findHostByDpidAndPort(const UINT8 dpid, const UINT4 port);

	CHostList& getHostList()     { return m_list; }

	CSmartPtr<CHost> getHostGateway(const CSmartPtr<CHost>& host);
	CSmartPtr<CHost> getHostGatewayByHostIp(UINT4 ip);
	CSmartPtr<CHost> getHostDhcp(const CSmartPtr<CHost>& host);
    UINT4 getExternalHostPort(const CSmartPtr<CHost>& host) const;

    CSmartPtr<CSwitch> getExternalHostSw(const CSmartPtr<CHost>& host) const;

private:
	CHostMgr();

private:
	static CHostMgr* m_instance;

	CHostList  m_list;
	CRWLock    m_lock;
    CHostTrack m_track;
};

#endif

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
*   File Name   : COpenstackExternal.cpp    *
*   Author      : bnc mmzhang               *
*   Create Date : 2016-9-18                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include"COpenstackExternal.h"
#include "comm-util.h"
#include "log.h"



COpenstackExternal::COpenstackExternal():
	m_uiExternalGatewayIp(0),
	m_uiExternalOuterInterfaceIp(0),
	m_ucExternalDpid(0),
	m_uiExternalPort(0),
	m_bValid(FALSE)
{

}

COpenstackExternal::~COpenstackExternal()
{

}

COpenstackExternal::COpenstackExternal(std::string external_subnet_id,
	UINT4 external_gateway_ip,
	UINT4 external_outer_interface_ip,
	UINT1* external_gateway_mac,
	UINT1* external_outer_interface_Mac,
	UINT8 external_dpid,
	UINT4 external_port,
	BOOL valid)
{
	m_strExternalSubnetId = external_subnet_id;
	m_uiExternalGatewayIp = external_gateway_ip;
	if (external_gateway_mac)
	{
	    memcpy(m_ucExternalGatewayMac,external_gateway_mac, 6);
	}
	m_uiExternalOuterInterfaceIp = external_outer_interface_ip;
	if (external_outer_interface_Mac)
	{
	    memcpy(m_ucExternalOuterInterfaceMac,external_outer_interface_Mac, 6);
	}
	m_ucExternalDpid = external_dpid;
	m_uiExternalPort = external_port;
	m_bValid = valid;
}

BOOL COpenstackExternal::isValid()
{
    /*
     * 打印每次更新后的情况
     */
//    INT1 ret[1024] = {0};
//    number2ip(m_uiExternalOuterInterfaceIp,ret);
//    LOG_INFO_FMT("external outer interface ip is %s",ret);
//    memset(ret,0,8);
//    number2ip(m_uiExternalGatewayIp,ret);
//    LOG_INFO_FMT("external gateway ip is %s",ret);
//
//    memset(ret, 0, 8);
//    mac2str(m_ucExternalOuterInterfaceMac,ret);
//    LOG_INFO_FMT("external interface mac is %s",ret);
//    memset(ret,0,8);
//    mac2str(m_ucExternalGatewayMac,ret);
//    LOG_INFO_FMT("external gateway mac is %s",ret);


//    LOG_INFO_FMT("external subnet_id is %s",m_strExternalSubnetId.c_str());


	UINT1 ext_zero_mac[6] = {0};
	if ((0 != m_uiExternalPort)
		&& (0 != m_ucExternalDpid)
		&& (0 != memcmp(ext_zero_mac,m_ucExternalOuterInterfaceMac, 6))
		&& (0 != m_uiExternalOuterInterfaceIp)
		&& (0 != memcmp(ext_zero_mac,m_ucExternalGatewayMac, 6))
	    && (0 != m_uiExternalGatewayIp)
		&& ( !m_strExternalSubnetId.empty()))
	{
		m_bValid = TRUE;
	}
	else
	{
		m_bValid = FALSE;
	}
	return m_bValid;
}


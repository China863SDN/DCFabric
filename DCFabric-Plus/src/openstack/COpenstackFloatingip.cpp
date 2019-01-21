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
*   File Name   : COpenstackFloatingip.cpp  *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#include "COpenstackFloatingip.h"



COpenstackFloatingip::COpenstackFloatingip()
{

}

COpenstackFloatingip::~COpenstackFloatingip()
{


}





BOOL COpenstackFloatingip::Compare(COpenstackFloatingip* floatingip)
{

	if(NULL == floatingip)
	{
		return FALSE;
	}
	if((0 != floatingip->m_strPortId.compare(this->m_strPortId))||
		(0 != floatingip->m_strId.compare(this->m_strId))||
		(0 != floatingip->m_strFixedIp.compare(this->m_strFixedIp))||
		(0 != floatingip->m_strFloatingIp.compare(this->m_strFloatingIp))||
		(0 != floatingip->m_strFloatingNetId.compare(this->m_strFloatingNetId))||
		(0 != floatingip->m_strTenantId.compare(this->m_strTenantId)))
	{
		return FALSE;
	}
	return TRUE;
}	

BOOL COpenstackFloatingip::SetObjectValue(COpenstackFloatingip* floatingip)
{
	if(NULL == floatingip)
	{
		return FALSE;
	}
	this->m_strId = floatingip->m_strId;
	this->m_strPortId = floatingip->m_strPortId;
	this->m_strFixedIp = floatingip->m_strFixedIp;
	this->m_strFloatingIp = floatingip->m_strFloatingIp;
	this->m_strFloatingNetId = floatingip->m_strFloatingNetId;
	this->m_strTenantId = floatingip->m_strTenantId;
	this->m_strRouterId = floatingip->m_strRouterId;
	this->m_strStatus = floatingip->m_strStatus;
	this->m_strTenantId = floatingip->m_strTenantId;
	
	return TRUE;
}

